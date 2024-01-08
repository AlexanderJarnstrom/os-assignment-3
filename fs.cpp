#include <iostream>
#include "fs.h"
#include <vector>
#include <string.h>

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

// Creates a file on the disk
void 
FS::create_dir_entry(dir_entry &entry, const std::string file_content, const int& fat_index)
{
    int index, name_size, size_size, first_blk_size, current_size, next_size, free_spots;
    int needed_files_count, file_content_size, needed_blocks, found_blocks, block_index;
    uint8_t cell, attr[ENTRY_ATTRIBUTE_SIZE], cont[ENTRY_CONTENT_SIZE];
    uint32_t buffer;

    name_size = 56;
    size_size = 4;
    first_blk_size = 2;
    current_size = 0;
    next_size = name_size;

    file_content_size = file_content.size();
    needed_blocks = calc_needed_blocks(file_content_size);

    found_blocks = 0;

    int free_blocks[needed_blocks];

    for (index = 0; index < ENTRY_ATTRIBUTE_SIZE; index++)
        attr[index] = 0;

    for (index = 0; index < ENTRY_CONTENT_SIZE; index++)
        cont[index] = 0;

    // Find empty block.

    if (fat_index == -1) 
        for (index = 0; index < BLOCK_SIZE / 2 && found_blocks < needed_blocks; index++)
            if (fat[index] == FAT_FREE) {
                free_blocks[found_blocks] = index;

                if (found_blocks == 0)
                    entry.first_blk = index;

                found_blocks++;
            }
    // add file info to array.

    for (index = current_size; index < next_size; index++) {
        cell = entry.file_name[index];
        attr[index] = cell;
    }

    current_size = next_size;
    next_size += size_size;

    for (index = current_size; index < next_size; index++) {
        buffer = entry.size >> (8 * (index - current_size));
        cell = buffer & 0xff;
        attr[index] = cell;
    }

    current_size = next_size;
    next_size += first_blk_size;

    for (index = current_size; index < next_size; index++) {
        buffer = entry.first_blk >> (8 * (index - current_size));
        cell = buffer & 0xff;
        attr[index] = cell;
    }

    current_size = next_size;

    attr[current_size++] = entry.type;
    attr[current_size++] = entry.access_rights;

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

    
}

// Updates the directorys' children and the size of the entry
void 
FS::update_dir_content(dir_entry *entry, dir_child *child, const uint8_t &task)
{
    std::vector<dir_child*> children;
    uint8_t cont[ENTRY_CONTENT_SIZE], attr[ENTRY_ATTRIBUTE_SIZE], cell;
    int32_t buffer;

    int index; // Global variables.
    int name_size, size_size, first_blk_size, current_size, next_size; // attribute variables.
    int dir_child_size, cont_size, internal_index, child_index; // cont variables.

    // Make sure the new arrays are truly empty.
    for (index = 0; index < ENTRY_CONTENT_SIZE; index++)
        cont[index] = 0x00;

    for (index = 0; index < ENTRY_ATTRIBUTE_SIZE; index++)
        attr[index] = 0x00;

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
    printf("Entry size: %d\n", entry->size);

    // Define attr variables
    name_size = 56;
    size_size = 4;
    first_blk_size = 2;
    current_size = 0;
    next_size = name_size;

    // Adding attributes to 'attr' for later use.
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

    // Define cont variables
    dir_child_size = sizeof(dir_child);
    cont_size = dir_child_size * children.size();
    internal_index = 0;
    child_index = 0;

    // Adding content to 'cont' for later use.
    // TODO: Handler dirs with too large content.

    for (index = 0; index < ENTRY_CONTENT_SIZE && child_index < children.size(); index++) {
        if (internal_index < name_size) {
            cont[index] = children[child_index]->file_name[internal_index];
        } else {
            buffer = children[child_index]->index >> (8 * (internal_index - name_size));
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
}

// Takes the attributes and content arrays and writes them to disk.
void 
FS::write_block(uint8_t attr[ENTRY_ATTRIBUTE_SIZE], uint8_t cont[ENTRY_CONTENT_SIZE], unsigned block_no)
{
    int index;
    uint8_t block[BLOCK_SIZE];

    for (index = 0; index < BLOCK_SIZE; index++)
        block[index] = 0;

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

    for (index = 0; index < BLOCK_SIZE; index ++) 
        block[index] = 0; 

    // TODO: handle error code -1
    this->disk.read(block_index, block);

    for (index = 0; index < BLOCK_SIZE; index++)
        if (index < ENTRY_ATTRIBUTE_SIZE)
            attr[index] = block[index];

    for (index = 0; index < next_size; index++)
        file_name[index] = attr[index];

    current_index = next_size;
    next_size += size_size;

    strcpy(entry->file_name, file_name);

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

            strcpy(temp_child->file_name, file_name);
            temp_child->index = temp;
            temp = 0;

            children.push_back(temp_child);

            internal_index = 0;
        }

    }

    return children;
}

// Gets all the content of a file.
std::string 
FS::read_cont_file(const dir_entry *entry)
{
    uint8_t block[BLOCK_SIZE];
    int index;

    char content[entry->size];

    this->disk.read(entry->first_blk, block);

    for (index = ENTRY_ATTRIBUTE_SIZE; index < BLOCK_SIZE && index < (entry->size + ENTRY_ATTRIBUTE_SIZE); index++)
        content[index - ENTRY_ATTRIBUTE_SIZE] = block[index];

    return content;
}

// Splits up a string into a path_obj
path_obj 
FS::format_path(std::string &path_s)
{
    // TODO: Handle error.
    if (path_s.empty())
        exit(1);

    path_obj path;
    std::string temp, entry_name;
    int index;

    if (path_s[0] == '/')
        path.start = START_ROOT;
    else
        path.start = START_WDIR;

    for (index = 0; index < path_s.size(); index++) {
        if (index == 0 && path.start == START_ROOT)
            index++;
        
        if (path_s[index] == '/') {
            path.dirs.push_back(entry_name);
            entry_name.clear();
        } else {
            temp = path_s[index];
            entry_name.append(temp);
        }
    }

    path.dirs.push_back(entry_name);
    entry_name.clear();

    path.end = path.dirs[path.dirs.size() - 1];
    path.dirs.pop_back();

    return path;
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
}

FS::~FS()
{

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

    for (index = 0; index < BLOCK_SIZE; index++)
        block[index] = 0x00;

    for (index = 0; index < cap; index++)
        this->disk.write(index, block);

    // Create root dir.

    struct dir_entry root_dir;

    strcpy(root_dir.file_name, "/");
    root_dir.first_blk = ROOT_BLOCK;
    root_dir.access_rights = WRITE + READ;
    root_dir.type = TYPE_DIR;
    root_dir.size = 0;

    this->create_dir_entry(root_dir, "", ROOT_BLOCK);

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

    std::cout << "Enter content:\n";

    while (std::getline(std::cin, buffer) && !buffer.empty()) {
        input.append(buffer);
        input.append("\n");
    }

    // TODO: check if disk is full.
    
    struct dir_entry file;

    path_obj path = format_path(filepath);

    // TODO: Follow the path.

    for (index = 0; index < 56; index++)
        if (index < filepath.size()) {
            file.file_name[index] = path.end[index];
        } else
            file.file_name[index] = 0;
    

    file.size = input.size();
    file.type = TYPE_FILE;
    file.access_rights = WRITE + READ;

    this->create_dir_entry(file, input);



    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(std::string filepath)
{
    dir_entry* entry = read_block_attr(2);
    std::cout << read_cont_file(entry) << std::endl;

    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int
FS::ls()
{
    std::cout << "FS::ls()\n";
    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";
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
