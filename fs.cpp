#include <iostream>
#include "fs.h"
#include <string.h>
#include <vector>

void FS::load_fat()
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

void FS::update_fat()
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

    for (index = 0; index < 56; index++) {
        cell = root_dir.file_name[index];
        block[index] = cell;
    }

    for (index = 56; index < 60; index++) {
        val = root_dir.size >> (8 * (index - 56));
        cell = val & 0xff;
        block[index] = cell;
    }

    for (index = 60; index < 62; index++) {
        val = root_dir.first_blk >> (8 * (index - 60));
        cell = val & 0xff;
        block[index] = cell;
    }

    block[++index] = root_dir.type;
    block[++index] = root_dir.access_rights;

    this->disk.write(0, block);

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

    return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int
FS::create(std::string filepath)
{
    std::string buffer;
    std::string input;
    int size, needed_files, index, j;

    input.clear();

    uint8_t block[BLOCK_SIZE];
    uint8_t cell;
    uint32_t val;

    std::cout << "Enter content:\n";

    while (std::getline(std::cin, buffer) && !buffer.empty()) {
        input.append(buffer);
        input.append("\n");
    }

    input.append("\0");

    size = input.size();
    needed_files = (size + 4031) / 4032;
    int free_spots[needed_files];
    j = 0;

    for (index = 0; index < BLOCK_SIZE / 2 && j < needed_files; index++)
        if (fat[index] == FAT_FREE) {
            free_spots[j] = index;
            j++;
        }

    // TODO: remove print.
    printf("Needed Files: %d | Free spots: %d | Free spot: %d\n", needed_files, j, free_spots[0]);
    std::cout << input << std::endl;

    // TODO: check if disk is full.
    
    struct dir_entry file;

    // TODO: Split filepath and follow it.
    strcpy(file.file_name, filepath.c_str());

    file.size = size;
    file.first_blk = free_spots[0];
    file.type = TYPE_FILE;
    file.access_rights = WRITE + READ;

    // TODO: create a sepreate func for writing file to disk.

    for (index = 0; index < 56; index++) {
        cell = file.file_name[index];
        block[index] = cell;
    }

    for (index = 56; index < 60; index++) {
        val = file.size >> (8 * (index - 56));
        cell = val & 0xff;
        block[index] = cell;
    }

    for (index = 60; index < 62; index++) {
        val = file.first_blk >> (8 * (index - 60));
        cell = val & 0xff;
        block[index] = cell;
    }

    block[++index] = file.type;
    block[++index] = file.access_rights;

    for (j = 0; j < size; j++) {
        block[index] = input[j];
        index++;
    }

    this->disk.write(free_spots[0], block);


    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";
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
