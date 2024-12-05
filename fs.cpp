#include "fs.h"

#include <stdint.h>
#include <string.h>
#include <strings.h>

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <vector>

#include "entry.h"

// Loads the fat table.
void FS::load_fat() {
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
void FS::update_fat() {
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

void FS::empty_array(uint8_t *arr, const int &size) {
  int index;

  for (index = 0; index < size; index++) arr[index] = 0x00;
}

void FS::fill_attr_array(uint8_t *attr, const int &size, dir_entry *entry) {
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

dir_entry *FS::follow_path(const path_obj *path) {
  std::vector<dir_child *> children;
  bool dir_exists, entry_found;
  int index, path_index;
  dir_entry *dir;

  dir_exists = true;
  entry_found = false;

  path_index = 0;

  if (path->start == START_ROOT)
    dir = read_block_attr(ROOT_BLOCK);
  else if (path->start == START_WDIR && this->working_dir != nullptr)
    dir = this->working_dir;
  else
    return nullptr;

  if (path->dirs.size() == 0) return dir;

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

  if (!dir_exists) dir = nullptr;

  return dir;
}

dir_entry *FS::get_child(const dir_entry *parent, const std::string &name) {
  std::vector<dir_child *> children;
  children = read_cont_dir(parent);

  for (dir_child *entry : children)
    if (strcmp(entry->file_name, name.c_str()) == 0) {
      return read_block_attr(entry->index);
    }

  return nullptr;
}

// Creates a file on the disk
void FS::create_dir_entry(dir_entry *entry, const std::string file_content, dir_entry *parent, const int &fat_index) {
  int index, next_size, free_spots;
  int needed_files_count, file_content_size, needed_blocks, found_blocks, block_index;
  uint8_t cell, attr[ENTRY_ATTRIBUTE_SIZE], cont[ENTRY_CONTENT_SIZE];
  uint32_t buffer;

  file_content_size = file_content.size();
  needed_blocks = calc_needed_blocks(file_content_size);

  printf("Needed blocks: %d\n", needed_blocks);

  found_blocks = 0;

  int free_blocks[needed_blocks];

  // FIXME: use arr = {0}
  empty_array(attr, ENTRY_ATTRIBUTE_SIZE);
  empty_array(cont, ENTRY_CONTENT_SIZE);

  // Find empty block.

  if (fat_index == -1)
    for (index = 0; index < BLOCK_SIZE / 2 && found_blocks < needed_blocks; index++)
      if (fat[index] == FAT_FREE) {
        free_blocks[found_blocks] = index;

        if (found_blocks == 0) entry->first_blk = index;

        found_blocks++;
      }

  printf("Found empty: %d\n", free_blocks[0]);

  fill_attr_array(attr, ENTRY_ATTRIBUTE_SIZE, entry);

  // Write blocks

  if (file_content.empty() && fat_index != -1) {
    this->write_block(attr, cont, fat_index);
  } else if (entry->type == TYPE_FILE) {
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
        for (int i = 0; i < ENTRY_CONTENT_SIZE; i++) {
          cont[i] = 0;
        }
      }
    }
  } else if (entry->type == TYPE_DIR) {
    this->write_block(attr, cont, free_blocks[0]);
    this->fat[free_blocks[0]] = FAT_EOF;
  }

  update_fat();

  // update parent.

  if (parent != nullptr) {
    dir_child child;
    strncpy(child.file_name, entry->file_name, 56);
    child.index = entry->first_blk;
    update_dir_content(parent, &child);
  }
}

// Updates the directorys' children and the size of the entry
void FS::update_dir_content(dir_entry *entry, dir_child *child, const uint8_t &task) {
  std::vector<dir_child *> children;
  uint8_t cont[ENTRY_CONTENT_SIZE], attr[ENTRY_ATTRIBUTE_SIZE], cell;
  int32_t buffer;

  int index;                                        // Global variables.
  int dir_child_size, internal_index, child_index;  // cont variables.

  // Make sure the new arrays are truly empty.

  empty_array(attr, ENTRY_ATTRIBUTE_SIZE);
  empty_array(cont, ENTRY_CONTENT_SIZE);
  // Add new child to array.
  children = read_cont_dir(entry);

  // TODO: handle remove child.
  if (task == ADD_DIR_CHILD) {
    for (dir_child *exi_child : children) {
      if (strcmp(exi_child->file_name, child->file_name) == 0) {
        printf("File named '%s' already exists.\n", child->file_name);
        return;
      }
    }

    children.push_back(child);

  } else if (task == REMOVE_DIR_CHILD) {
    index = 0;
    for (dir_child *exi_child : children) {
      if (strcmp(exi_child->file_name, child->file_name) == 0) {
        children.erase(children.begin() + index);
        break;
      }

      index++;
    }
  } else {
    printf("Something went wrong.\n");
    return;
  }

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
}

// Takes the attributes and content arrays and writes them to disk.
void FS::write_block(uint8_t attr[ENTRY_ATTRIBUTE_SIZE], uint8_t cont[ENTRY_CONTENT_SIZE], unsigned block_no) {
  int index;
  uint8_t block[BLOCK_SIZE];

  for (index = 0; index < BLOCK_SIZE; index++) block[index] = 0x00;

  for (index = 0; index < ENTRY_ATTRIBUTE_SIZE; index++) block[index] = attr[index];

  for (index = 0; index < ENTRY_CONTENT_SIZE; index++) block[index + ENTRY_ATTRIBUTE_SIZE] = cont[index];

  this->disk.write(block_no, block);
}

// Gets the attributes for the block on the given index.
dir_entry *FS::read_block_attr(uint16_t block_index) {
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

  for (index = 0; index < ENTRY_ATTRIBUTE_SIZE; index++) attr[index] = block[index];

  for (index = 0; index < next_size; index++) {
    file_name[index] = attr[index];
  }

  current_index = next_size;
  next_size += size_size;

  strncpy(entry->file_name, file_name, 56);

  // Get size attribute.
  for (index = next_size - 1; index >= current_index; index--) temp = (temp << 8) | attr[index];

  entry->size = temp;

  current_index = next_size;
  next_size += blk_size;

  // Get first block index in FAT.
  for (index = next_size - 1; index >= current_index; index--) temp = (temp << 8) | attr[index];

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
std::vector<dir_child *> FS::read_cont_dir(const dir_entry *directory) {
  uint8_t block[BLOCK_SIZE], cont[ENTRY_CONTENT_SIZE];
  uint16_t temp;
  dir_child *temp_child;
  std::vector<dir_child *> children;

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

  return children;
}

// Gets all the content of a file.
std::string FS::read_cont_file(const dir_entry *entry) {
  uint8_t block[BLOCK_SIZE];
  bool reached_end = false;
  int index, fat_index, next_fat_index;

  fat_index = entry->first_blk;

  std::string content;

  while (!reached_end) {
    this->disk.read(fat_index, block);

    for (index = ENTRY_ATTRIBUTE_SIZE; index < BLOCK_SIZE && index < (entry->size + ENTRY_ATTRIBUTE_SIZE); index++) content += block[index];

    if ((fat_index = fat[fat_index]) == FAT_EOF) reached_end = true;
  }

  return content;
}

// Splits up a string into a path_obj
int FS::format_path(std::string &path_s, path_obj *path) {
  // TODO: Handle error.
  if (path_s.empty()) return 1;

  std::string temp, entry_name;
  int index;

  if (path_s[0] == '/')
    path->start = START_ROOT;
  else
    path->start = START_WDIR;

  for (index = 0; index < path_s.size(); index++) {
    if (index == 0 && path->start == START_ROOT) index++;

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
int FS::calc_needed_blocks(const unsigned long &size) {
  int current_val, count;

  current_val = size;
  count = 0;

  while (current_val > 0) {
    count++;
    current_val -= ENTRY_CONTENT_SIZE;
  }

  if (count == 0) count = 1;

  return count;
}

FS::FS() {
  std::cout << "FS::FS()... Creating file system\n";
  load_fat();
  this->working_dir = read_block_attr(ROOT_BLOCK);
}

FS::~FS() { delete this->working_dir; }

Disk *FS::get_disk() { return &this->disk; }

int16_t *FS::get_fat() { return this->fat; }

int16_t FS::get_working_dir_blk_index() { return this->working_dir->first_blk; }
// formats the disk, i.e., creates an empty file system
int FS::format() {
  int index, cap;
  uint8_t block[BLOCK_SIZE] = {0};
  uint8_t cell;
  uint16_t entry;
  uint32_t val;

  cap = this->disk.get_no_blocks();

  fs_obj::directory_t root;
  fs_obj::dir_entry root_attr;
  root_attr.first_blk = ROOT_BLOCK;
  root_attr.type = TYPE_DIR;
  root_attr.access_rights = READ | WRITE;
  strcpy(root_attr.file_name, "/");

  root.attributes = root_attr;
  fs_obj::create_dir(this, &root, nullptr);

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

  this->working_dir = read_block_attr(ROOT_BLOCK);

  return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int FS::create(std::string filepath) {
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
    printf("%s is not a valid path.\n", filepath.c_str());
    return 0;
  }

  // TODO: Check if works with longer paths.
  dir_entry *parent;
  if ((parent = follow_path(&path)) == nullptr) {
    printf("Path: %s doesnt exist\n", filepath.c_str());
    return 0;
  }

  std::cout << parent->first_blk << std::endl;

  // TODO: give reason.
  if (parent == nullptr) return 0;

  for (index = 0; index < 56; index++)
    if (index < filepath.size()) {
      file.file_name[index] = path.end[index];
    } else
      file.file_name[index] = 0;

  file.size = input.size();
  file.type = TYPE_FILE;
  file.access_rights = WRITE + READ;

  this->create_dir_entry(&file, input, parent);

  if (parent != this->working_dir) delete parent;

  return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int FS::cat(std::string filepath) {
  path_obj path;
  dir_entry *parent;
  dir_entry *file;

  std::string content;

  if (format_path(filepath, &path) != 0) {
    printf("%s is not a valid path.\n", filepath.c_str());
    return 0;
  }

  // TODO: give reason.
  if ((parent = follow_path(&path)) == nullptr) {
    printf("Couldn't follow path\n");
    return 0;
  }

  printf("%s\n", parent->file_name);

  file = get_child(parent, path.end);

  // TODO: give reason
  if (file->type == TYPE_DIR) {
    printf(
        "Expected entry of type 'file', but the given path leads to a "
        "directory.\n");
    return 0;
  }

  content = read_cont_file(file);

  std::cout << content << std::endl;

  if (parent != this->working_dir) delete parent;

  return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int FS::ls() {
  std::vector<dir_child *> children = read_cont_dir(this->working_dir);

  printf("%15s |%10s |%7s\n", "Name", "Size", "Dir");

  for (const dir_child *child : children) {
    dir_entry *child_info = read_block_attr(child->index);
    printf("%15s |%10d |%7d\n", child_info->file_name, child_info->size, child_info->type);
    delete child_info;
    delete child;
  }

  return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int FS::cp(std::string sourcepath, std::string destpath) {
  // Init variables
  path_obj src_path, dest_path;
  dir_entry *src_entry, *src_entry_parent, *dest_entry, *dest_entry_parent;

  std::string content;

  // Validate input
  if (format_path(sourcepath, &src_path) != 0) {
    printf("%s is not a valid path.\n", sourcepath.c_str());
    return 0;
  }

  if (format_path(destpath, &dest_path) != 0) {
    printf("%s is not a valid path.\n", destpath.c_str());
    return 0;
  }

  if ((src_entry_parent = follow_path(&src_path)) == nullptr) {
    printf("%s doesn't exist.\n", sourcepath.c_str());
    return 0;
  }

  if ((src_entry = get_child(src_entry_parent, src_path.end)) == nullptr) {
    printf("%s doesn't exist.\n", sourcepath.c_str());
    return 0;
  }

  if (src_entry->type == TYPE_DIR) {
    printf("%s is a directory, expected a file.\n", sourcepath.c_str());
    return 0;
  }

  if ((dest_entry_parent = follow_path(&dest_path)) == nullptr) {
    printf("%s doesn't exist.\n", destpath.c_str());
    return 0;
  }

  if ((dest_entry = get_child(dest_entry_parent, dest_path.end)) != nullptr) {
    printf("%s already exist.\n", destpath.c_str());
    return 0;
  }

  dest_entry = new dir_entry;

  // copy attributes
  strcpy(dest_entry->file_name, dest_path.end.c_str());
  dest_entry->size = src_entry->size;
  dest_entry->first_blk = src_entry->first_blk;
  dest_entry->type = src_entry->type;
  dest_entry->access_rights = src_entry->access_rights;

  // copy content
  content = read_cont_file(src_entry);

  create_dir_entry(dest_entry, content, dest_entry_parent);

  // free mem
  delete src_entry;
  delete dest_entry;

  if (src_entry_parent != this->working_dir) delete src_entry_parent;

  if (dest_entry_parent != this->working_dir) delete dest_entry_parent;

  return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name
// <destpath>, or moves the file <sourcepath> to the directory <destpath> (if
// dest is a directory)
int FS::mv(std::string sourcepath, std::string destpath) {
  std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
  return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int FS::rm(std::string filepath) {
  path_obj path;
  dir_entry *parent, *entry;
  int next_fat, current_fat;
  dir_child entry_child;

  if (format_path(filepath, &path) != 0) {
    printf("%s is not a valid path.\n", filepath.c_str());
    return 0;
  }

  if ((parent = follow_path(&path)) == nullptr) {
    printf("%s doesn't exist.\n", filepath.c_str());
    return 0;
  }

  if ((entry = get_child(parent, path.end)) == nullptr) {
    printf("%s doesn't exist.\n", filepath.c_str());
    return 0;
  }

  // delete fat index from fat table.
  current_fat = entry->first_blk;

  printf("%s\n", entry->file_name);

  while (current_fat != FAT_EOF) {
    printf("%d\n", current_fat);
    next_fat = this->fat[current_fat];
    this->fat[current_fat] = FAT_FREE;
    current_fat = next_fat;

    printf("%d\n", current_fat);
  }

  update_fat();

  printf("%d\n", current_fat);

  for (int i = 0; i < 56; i++) entry_child.file_name[i] = entry->file_name[i];

  printf("%d\n", current_fat);

  update_dir_content(parent, &entry_child, REMOVE_DIR_CHILD);

  return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int FS::append(std::string filepath1, std::string filepath2) {
  std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
  return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int FS::mkdir(std::string dirpath) {
  dir_entry directory, *parent;
  path_obj path;
  char temp[56];

  if (format_path(dirpath, &path) != 0) {
    printf("%s is not a valid path.\n", dirpath.c_str());
    return 0;
  }

  if ((parent = follow_path(&path)) == nullptr) {
    printf("%s doesn't exist.\n", dirpath.c_str());
    return 0;
  }

  printf("End: %s\nStart: %d\n", path.end.c_str(), path.start);

  for (int index = 0; index < 56; index++) {
    if (index < path.end.size())
      directory.file_name[index] = path.end[index];
    else
      directory.file_name[index] = 0x00;
  }

  directory.size = 0;
  directory.access_rights = READ | WRITE;
  directory.type = TYPE_DIR;

  create_dir_entry(&directory, "", parent);

  if (parent != this->working_dir) delete parent;

  return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named
// <dirpath>
int FS::cd(std::string dirpath) {
  dir_entry *directory, *parent;
  path_obj path;
  char temp[56];

  if (format_path(dirpath, &path) != 0) {
    printf("%s is not a valid path.\n", dirpath.c_str());
    return 0;
  }

  if ((parent = follow_path(&path)) == nullptr) {
    printf("%s doesn't exist.\n", dirpath.c_str());
    return 0;
  }

  if ((directory = get_child(parent, path.end)) == nullptr) {
    printf("%s doesn't exist.\n", dirpath.c_str());
    return 0;
  }

  if (directory->type == TYPE_FILE) {
    printf("Given path leads to a file.\n");
    return 0;
  }

  delete this->working_dir;
  this->working_dir = directory;

  printf("Current directory: %s\n", directory->file_name);

  return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int FS::pwd() {
  std::cout << "FS::pwd()\n";
  return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int FS::chmod(std::string accessrights, std::string filepath) {
  std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
  return 0;
}
