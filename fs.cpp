#include <iostream>
#include "fs.h"
#include <vector>
#include <string.h>
#include <stdint.h>

// Loads the fat table.
void 
FS::load_fat()
{
    uint8_t block[BLOCK_SIZE];
    uint16_t cell;

    int index, fat_index;
    fat_index = 0;

    this->disk.read(FAT_BLOCK, block);

    for (index = 0; index < BLOCK_SIZE; index += 2) {
        cell = block[index + 1];
        cell = (cell << 8) | block[index];

        fat[fat_index] = cell;
        fat_index++;
    }
}

// Takes the current state of the fat table and writes it to disk
void 
FS::update_fat()
{
    uint8_t block[BLOCK_SIZE];
    uint8_t l_cell, r_cell;
    uint16_t buffer;
    
    int index_f, index_b;
    index_b = 0;

    for (index_f = 0; index_f < BLOCK_SIZE / 2; index_f++) {
        buffer = this->fat[index_f];

        r_cell = buffer & 0xff;

        buffer = buffer >> 8;
        l_cell = buffer & 0xff;

        block[index_b++] = l_cell;
        block[index_b++] = r_cell;
    }
    
    this->disk.write(FAT_BLOCK, block);
    this->load_fat();
}

void FS::empty_array(uint8_t *arr, const int &size)
{
    int index;

    for (index = 0; index < size; index++)
        arr[index] = 0x00;
}

void FS::fill_attr_array(uint8_t *attr, const int &size, dir_entry *entry)
{
    int index, current_size, next_size, size_size, first_blk_size;
    uint8_t cell;
    uint32_t buffer;

    current_size = 0;
    next_size = 56;
    size_size = 4;
    first_blk_size = 2;

    for (index = current_size; index < next_size; index++) {
        cell = entry->file_name[index];
        attr[index] = cell;
    }

    current_size = next_size;
    next_size += size_size;

    for (index = current_size; index < next_size; index++) {
        buffer = entry->size >> (8 * (index - current_size));
        cell = buffer & 0xff;
        attr[index] = cell;
    }

    current_size = next_size;
    next_size += first_blk_size;

    for (index = current_size; index < next_size; index++) {
        buffer = entry->first_blk >> (8 * (index - current_size));
        cell = buffer & 0xff;
        attr[index] = cell;
    }

    current_size = next_size;

    attr[current_size++] = entry->type;
    attr[current_size++] = entry->access_rights;

}

dir_entry *FS::follow_path(const path_obj *path)
{
    std::vector<dir_child*> children;
    bool dir_exists, entry_found;
    int index, path_index;
    dir_entry* dir;

    dir_exists = true;
    entry_found = false;

    path_index = 0;

    if (path->start == START_ROOT)
        dir = read_block_attr(ROOT_BLOCK);
    else if (path->start == START_WDIR && this->working_dir != nullptr)
        dir = this->working_dir;
    else
        return nullptr;

    if (path->dirs.size() == 0)
        return dir;

    while (dir_exists && !entry_found) {
        children = read_cont_dir(dir);
        dir_exists = false;

        for (index = 0; index < children.size() && !dir_exists; index++) {
            if (strcmp(children[index]->file_name, path->dirs[path_index].c_str()) == 0 && path_index == path->dirs.size()) {
                dir = read_block_attr(children[index]->index);
                dir_exists = true;
                entry_found = true;
            } else if (strcmp(children[index]->file_name, path->dirs[path_index].c_str()) == 0) {
                dir = read_block_attr(children[index]->index);
                dir_exists = true;
            }
        }
    }

    if (!dir_exists)
        dir = nullptr;

    return dir;
}

// Creates a file on the disk
void 
FS::create_dir_entry(dir_entry *entry, const std::string file_content, dir_entry* parent, const int& fat_index)
{
    int index, next_size, free_spots;
    int needed_files_count, file_content_size, needed_blocks, found_blocks, block_index;
    uint8_t cell, attr[ENTRY_ATTRIBUTE_SIZE], cont[ENTRY_CONTENT_SIZE];
    uint32_t buffer;

    file_content_size = file_content.size();
    needed_blocks = calc_needed_blocks(file_content_size);

    found_blocks = 0;

    int free_blocks[needed_blocks];

    empty_array(attr, ENTRY_ATTRIBUTE_SIZE);
    empty_array(cont, ENTRY_CONTENT_SIZE);

    // Find empty block.

    if (fat_index == -1) 
        for (index = 0; index < BLOCK_SIZE / 2 && found_blocks < needed_blocks; index++)
            if (fat[index] == FAT_FREE) {
                free_blocks[found_blocks] = index;

                if (found_blocks == 0)
                    entry->first_blk = index;

                found_blocks++;
            }
    
    fill_attr_array(attr, ENTRY_ATTRIBUTE_SIZE, entry);

    // Write blocks

    if (file_content.empty() && fat_index != -1) {
        this->write_block(attr, cont, fat_index);
    } else {
        index = 0;
        block_index = 0;
        int letters_left = file_content_size;

        for (char letter : file_content) {
            letters_left--;
            cont[index++] = letter;

            if (index == ENTRY_CONTENT_SIZE || letters_left == 0) {
                this->write_block(attr, cont, free_blocks[block_index]);
                index = 0;

                if (letters_left == 0) {
                    this->fat[free_blocks[block_index]] = FAT_EOF;
                } else {
                    this->fat[free_blocks[block_index]] = free_blocks[block_index + 1];
                }

                block_index++;
                this->update_fat();

                for (int i = 0; i < ENTRY_CONTENT_SIZE; i++) {
                    cont[i] = 0;
                }
            }
        }
    }

    // update parent.

    if (parent != nullptr) {
        dir_child child;
        strncpy(child.file_name, entry->file_name, 56);
        child.index = entry->first_blk;
        update_dir_content(parent, &child);
    }
}

// Updates the directorys' children and the size of the entry
void 
FS::update_dir_content(dir_entry *entry, dir_child *child, const uint8_t &task)
{
    std::vector<dir_child*> children;
    uint8_t cont[ENTRY_CONTENT_SIZE], attr[ENTRY_ATTRIBUTE_SIZE], cell;
    int32_t buffer;

    int index; // Global variables.
    int dir_child_size, internal_index, child_index; // cont variables.

    // Make sure the new arrays are truly empty.
    
    empty_array(attr, ENTRY_ATTRIBUTE_SIZE);
    empty_array(cont, ENTRY_CONTENT_SIZE);

    for (index = 0; index < ENTRY_CONTENT_SIZE; index++)
        std::cout << cont[index];

    std::cout << std::endl << std::endl;

    // TODO: handle remove child.
    if (task == REMOVE_DIR_CHILD)
        return;

    // Add new child to array.
    children = read_cont_dir(entry);

    for (dir_child* exi_child : children) {
        if (strcmp(exi_child->file_name, child->file_name) == 0) {
            printf("File named '%s' already exists.\n", child->file_name);
            return;
        }
    }

    children.push_back(child);

    entry->size = sizeof(dir_child) * children.size();

    fill_attr_array(attr, ENTRY_ATTRIBUTE_SIZE, entry);

    // Define cont variables
    dir_child_size = sizeof(dir_child);
    internal_index = 0;
    child_index = 0;

    // Adding content to 'cont' for later use.
    // TODO: Handler dirs with too large content.

    for (index = 0; index < entry->size; index++) {
        if (internal_index < 56) {
            cont[index] = children[child_index]->file_name[internal_index];
        } else {
            buffer = children[child_index]->index >> (8 * (internal_index - 56));
            cell = buffer & 0xff;
            cont[index] = cell;
        }

        internal_index++;

        if (internal_index == dir_child_size) {
            child_index++;
            internal_index = 0;
        }
    }

    write_block(attr, cont, entry->first_blk);

    for (dir_child* child : children)
        delete child;
}

// Takes the attributes and content arrays and writes them to disk.
void 
FS::write_block(uint8_t attr[ENTRY_ATTRIBUTE_SIZE], uint8_t cont[ENTRY_CONTENT_SIZE], unsigned block_no)
{
    int index;
    uint8_t block[BLOCK_SIZE];

    for (index = 0; index < BLOCK_SIZE; index++)
        block[index] = 0x00;

    for (index = 0; index < ENTRY_ATTRIBUTE_SIZE; index++)
        block[index] = attr[index];
    
    for (index = 0; index < ENTRY_CONTENT_SIZE; index++)
        block[index + ENTRY_ATTRIBUTE_SIZE] = cont[index];
    
    this->disk.write(block_no, block);
}

// Gets the attributes for the block on the given index.
dir_entry* 
FS::read_block_attr(uint16_t block_index)
{
    uint8_t block[BLOCK_SIZE], attr[ENTRY_ATTRIBUTE_SIZE];
    uint32_t temp;

    int index, name_size, size_size, blk_size, type_size, access_size;
    int next_size, current_index;

    dir_entry *entry = new dir_entry;

    name_size = 56;
    size_size = 4;
    blk_size = 2;
    type_size = 1;
    access_size = 1;

    current_index = 0;
    next_size = name_size;

    temp = 0x00000000;

    char file_name[name_size];

    empty_array(attr, ENTRY_ATTRIBUTE_SIZE);
    empty_array(block, BLOCK_SIZE);

    // TODO: handle error code -1
    this->disk.read(block_index, block);

    for (index = 0; index < ENTRY_ATTRIBUTE_SIZE; index++)
        attr[index] = block[index];

    for (index = 0; index < next_size; index++) {
        file_name[index] = attr[index];
    }

    current_index = next_size;
    next_size += size_size;

    strncpy(entry->file_name, file_name, 56);



    // Get size attribute.
    for (index = next_size - 1; index >= current_index; index--)
        temp = (temp << 8) | attr[index];

    entry->size = temp;

    current_index = next_size;
    next_size += blk_size;

    // Get first block index in FAT.
    for (index = next_size - 1; index >= current_index; index--)
        temp = (temp << 8) | attr[index];

    entry->first_blk = temp;
    
    current_index = next_size;
    next_size += type_size;

    // Get type
    entry->type = attr[current_index];

    current_index = next_size;
    next_size += type_size;

    // Gett access rights
    entry->access_rights = attr[current_index];
    
    return entry;
}

// Gets all the children from a directory.
std::vector<dir_child*> 
FS::read_cont_dir(const dir_entry *directory)
{
    printf("%s %d\n", this->working_dir->file_name, this->working_dir->size);
    uint8_t block[BLOCK_SIZE], cont[ENTRY_CONTENT_SIZE];
    uint16_t temp;
    dir_child* temp_child;
    std::vector<dir_child*> children;

    char file_name[56];
    int index, internal_index, dir_child_size;

    dir_child_size = sizeof(dir_child);
    internal_index = 0;

    this->disk.read(directory->first_blk, block);

    for (index = ENTRY_ATTRIBUTE_SIZE; index < (directory->size + ENTRY_ATTRIBUTE_SIZE); index++) {
        if (internal_index < 56)
            file_name[internal_index] = block[index];
        else if (internal_index == 56)
            temp = (temp << 8) | block[index + 1];
        else
            temp = (temp << 8) | block[index - 1];

        internal_index++;

        if (internal_index == dir_child_size) {
            temp_child = new dir_child;

            strncpy(temp_child->file_name, file_name, 56);
            temp_child->index = temp;
            temp = 0;

            children.push_back(temp_child);

            internal_index = 0;
        }

    }
    printf("%s %d\n", this->working_dir->file_name, this->working_dir->size);

    return children;
}

// Gets all the content of a file.
std::string 
FS::read_cont_file(const dir_entry *entry)
{
    uint8_t block[BLOCK_SIZE];
    bool reached_end = false;
    int index, fat_index, next_fat_index;

    fat_index = entry->first_blk;

    std::string content;

    while (!reached_end) {
        this->disk.read(fat_index, block);

        for (index = ENTRY_ATTRIBUTE_SIZE; index < BLOCK_SIZE && index < (entry->size + ENTRY_ATTRIBUTE_SIZE); index++) 
            content += block[index];
        
        if ((fat_index = fat[fat_index]) == FAT_EOF)
            reached_end = true;
    }

    return content;
}

// Splits up a string into a path_obj
int 
FS::format_path(std::string &path_s, path_obj* path)
{
    // TODO: Handle error.
    if (path_s.empty())
        return 1;

    std::string temp, entry_name;
    int index;

    if (path_s[0] == '/')
        path->start = START_ROOT;
    else
        path->start = START_WDIR;

    for (index = 0; index < path_s.size(); index++) {
        if (index == 0 && path->start == START_ROOT)
            index++;
        
        if (path_s[index] == '/') {
            path->dirs.push_back(entry_name);
            entry_name.clear();
        } else {
            temp = path_s[index];
            entry_name.append(temp);
        }
    }

    path->dirs.push_back(entry_name);
    entry_name.clear();

    path->end = path->dirs[path->dirs.size() - 1];
    path->dirs.pop_back();

    return 0;
}

// Calculates the amount of needed blocks for a file.
int 
FS::calc_needed_blocks(const unsigned long &size)
{
    int current_val, count;

    current_val = size;
    count = 0;

    while (current_val > 0) {
        count++;
        current_val -= ENTRY_CONTENT_SIZE;
    }

    return count;
}


FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";
    load_fat();
    this->working_dir = read_block_attr(ROOT_BLOCK);
}

FS::~FS()
{
    delete this->working_dir;
}

// formats the disk, i.e., creates an empty file system
int
FS::format()
{
    int index, cap;
    uint8_t block[BLOCK_SIZE];
    uint8_t cell;
    uint16_t entry;
    uint32_t val;

    cap = this->disk.get_no_blocks();

    // Set everything to zero.
    empty_array(block, BLOCK_SIZE);

    for (index = 0; index < cap; index++)
        this->disk.write(index, block);

    // Create root dir.

    struct dir_entry root_dir;

    strcpy(root_dir.file_name, "/");
    root_dir.first_blk = ROOT_BLOCK;
    root_dir.access_rights = WRITE + READ;
    root_dir.type = TYPE_DIR;
    root_dir.size = 0;

    this->create_dir_entry(&root_dir, "", nullptr, ROOT_BLOCK);

    // Create fat.
    for (index = 0; index < BLOCK_SIZE; index += 2) {
        if (index == 0 || index == 1)
            entry = FAT_EOF;
        else if (index == 2 || index == 3)
            entry = FAT_EOF;
        else
            entry = FAT_FREE;
        

        block[index] = entry & 0xff;
        entry = entry >> 8;
        block[index + 1] = entry & 0xff;
    }

    this->disk.write(FAT_BLOCK, block);

    this->load_fat();

    return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int
FS::create(std::string filepath)
{
    std::string buffer;
    std::string input;
    int needed_files, index, j;

    uint8_t block[BLOCK_SIZE];
    uint8_t cell;
    uint32_t val;

    path_obj path;

    while (std::getline(std::cin, buffer) && !buffer.empty()) {
        input.append(buffer);
        input.append("\n");
    }

    // TODO: check if disk is full.
    
    dir_entry file;

    if (format_path(filepath, &path) != 0) {
        printf("%s is not a valid path.\n", filepath);
        return 0;
    }

    // TODO: Check if works with longer paths.
    dir_entry* parent = follow_path(&path);


    

    // TODO: give reason.
    if (parent == nullptr)
        return 0;

    for (index = 0; index < 56; index++)
        if (index < filepath.size()) {
            file.file_name[index] = path.end[index];
        } else
            file.file_name[index] = 0;
    

    file.size = input.size();
    file.type = TYPE_FILE;
    file.access_rights = WRITE + READ;

    this->create_dir_entry(&file, input, parent);

    delete parent;

    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(std::string filepath)
{
    path_obj path;
    dir_entry* parent;
    dir_entry* file;
    std::vector<dir_child*> children;

    std::string content;
    std::string temp;

    if (format_path(filepath, &path) != 0) {
        printf("%s is not a valid path.\n", filepath);
        return 0;
    }

    // TODO: give reason.
    if ((parent = follow_path(&path)) == nullptr)
        return 0;

    children = read_cont_dir(parent);

    for (dir_child* child : children) {
        temp = child->file_name;
        if (temp == path.end) {
            file = read_block_attr(child->index);
            break;
        }
    }

    // TODO: give reason
    if (file->type == TYPE_DIR) {
        printf("Expected entry of type 'file', but the given path leads to a directory.\n");
        return 0;
    }

    content = read_cont_file(file);

    std::cout << content << std::endl;

    if (parent != this->working_dir)
        delete parent;

    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int
FS::ls()
{
    std::vector<dir_child*> children = read_cont_dir(this->working_dir);

    printf("%15s %10s\n", "Name", "Size");

    for (const dir_child* child : children) {
        dir_entry* child_info = read_block_attr(child->index);
        printf("%15s %10d\n", child_info->file_name, child_info->size); 
        delete child_info;
        delete child;
    }

    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";

    path_obj src_path;
    path_obj dest_path;

    if (format_path(sourcepath, &src_path) != 0) {
        printf("%s is not a valid path.\n", sourcepath);
        return 0;
    }

    if (format_path(destpath, &dest_path) != 0) {
        printf("%s is not a valid path.\n", destpath);
        return 0;
    }

    printf("from %s to %s\n", src_path.end, dest_path.end);

    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int
FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int
FS::rm(std::string filepath)
{
    std::cout << "FS::rm(" << filepath << ")\n";
    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int
FS::append(std::string filepath1, std::string filepath2)
{
    std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int
FS::mkdir(std::string dirpath)
{
    std::cout << "FS::mkdir(" << dirpath << ")\n";
    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int
FS::cd(std::string dirpath)
{
    std::cout << "FS::cd(" << dirpath << ")\n";
    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int
FS::pwd()
{
    std::cout << "FS::pwd()\n";
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int
FS::chmod(std::string accessrights, std::string filepath)
{
    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}
