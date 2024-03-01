#include "entry.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "constants.h"

void extract_attr(fs_obj::dir_entry *attributes, uint8_t *block) {
  int i, current_i;
  uint32_t temp = 0;

  for (i = 0; i < F_NAME_SIZE; i++) {
    attributes->file_name[i] = block[i];
  }
  current_i = i;

  for (i = current_i + F_SIZE_SIZE - 1; i >= F_NAME_SIZE; --i) {
    temp = (temp << ((F_NAME_SIZE + F_SIZE_SIZE) - i)) | block[i];
  }
  current_i = i + F_SIZE_SIZE;
  attributes->size = temp;
  temp = 0;

  for (i = current_i + F_FIRST_BLOCK_SIZE - 1; i >= current_i; --i) {
    temp = (temp << ((current_i + F_FIRST_BLOCK_SIZE) - i)) | block[i];
  }

  attributes->first_blk = attributes->first_blk | temp;

  current_i = current_i + F_FIRST_BLOCK_SIZE;
  attributes->type = block[++current_i];
  attributes->access_rights = block[++current_i];
}

void insert_attr(fs_obj::dir_entry *attributes, uint8_t *block) {
  int block_i, i;

  block_i = 0;

  for (i = 0; i < F_NAME_SIZE; i++) {
    block[block_i] = attributes->file_name[i];
    block_i++;
  }
}

void fs_obj::get_directory(FS *fs, fs_obj::directory_t *dir, const uint16_t &blk_index) {
  uint8_t block[ENTRY_SIZE] = {0};
  uint32_t temp = 0;
  int i, current_i;

  Disk *disk = fs->get_disk();
  int16_t *fat = fs->get_fat();

  disk->read(blk_index, block);
  // Extract directory attributes
  extract_attr(&dir->attributes, block);
  // TODO handle error
  if (dir->attributes.type != 1) {
    return;
  }
  // Extract directory content
  int16_t fat_index = dir->attributes.first_blk;
  int internal_i = 0;
  fs_obj::dir_child *temp_child;
  int dir_child_size = sizeof(fs_obj::dir_child);
  char file_name[56];

  do {
    if (fat_index != dir->attributes.first_blk) {
      disk->read(fat_index, block);
    }

    // FIXME: for loop wont work
    for (i = ENTRY_ATTRIBUTE_SIZE; i < (dir->attributes.size + ENTRY_ATTRIBUTE_SIZE); i++) {
      if (internal_i < 56)
        file_name[internal_i] = block[i];
      else if (internal_i == 56)
        temp = (temp << 8) | block[i + 1];
      else
        temp = (temp << 8) | block[i - 1];

      internal_i++;

      if (internal_i == dir_child_size) {
        temp_child = new dir_child;

        strncpy(temp_child->file_name, file_name, 56);
        temp_child->first_blk = temp;
        temp = 0;

        dir->children.push_back(temp_child);

        internal_i = 0;
      }
    }

    fat_index = fat[fat_index];
  } while (fat_index != -1);
}

void fs_obj::get_directory(FS *fs, directory_t *dir, directory_t *parent_dir, const char *name) {
  int16_t child_index;

  for (fs_obj::dir_child *child : parent_dir->children) {
    if (strcmp(name, child->file_name) == 0) {
      child_index = child->first_blk;
      break;
    }
  }

  fs_obj::get_directory(fs, dir, child_index);
  dir->attributes.parent_blk = parent_dir->attributes.first_blk;
}

void fs_obj::get_file(FS *fs, file_t *file, const uint16_t &blk_index) {
  uint8_t block[ENTRY_SIZE] = {0};
  int16_t fat_index;
  int i;

  Disk *disk = fs->get_disk();
  int16_t *fat = fs->get_fat();

  disk->read(blk_index, block);

  extract_attr(&file->attributes, block);

  fat_index = file->attributes.first_blk;
  // FIXME: for loop wont  work.
  do {
    for (i = ENTRY_ATTRIBUTE_SIZE; i < (file->attributes.size + ENTRY_ATTRIBUTE_SIZE); i++) {
      file->content += block[i];
    }
    fat_index = fat[fat_index];
  } while (fat_index != -1);
}

void fs_obj::get_file(FS *fs, file_t *file, directory_t *parent_dir, const char name[56]) {
  int16_t child_index;

  for (fs_obj::dir_child *child : parent_dir->children) {
    if (strcmp(name, child->file_name) == 0) {
      child_index = child->first_blk;
      break;
    }
  }

  fs_obj::get_file(fs, file, child_index);
  file->attributes.parent_blk = parent_dir->attributes.first_blk;
}

void fs_obj::followPath(FS *fs, file_t *file, const std::string &path) {
  if (path.size() == 0) return;  // TODO: show error and abort.

  directory_t dir, temp_dir;
  std::string temp, file_name;
  std::vector<std::string> directories;

  int i;

  if (path[0] != '/') {
    // FIXME: after updating fs working dir fix this.
    fs_obj::get_directory(fs, &dir, fs->get_working_dir_blk_index());
  } else {
    fs_obj::get_directory(fs, &dir, 0);
  }

  for (i = 0; i < path.size(); i++) {
    if (i == 0 && path[0] == '/') i++;
    if (path[i] == '/') {
      directories.push_back(temp);
      temp.clear();
      i++;
    }

    temp += path[i];
  }

  file_name = temp;

  for (std::string name : directories) {
    fs_obj::get_directory(fs, &temp_dir, &dir, name.c_str());
    dir = temp_dir;
  }

  fs_obj::get_file(fs, file, &dir, file_name.c_str());
}

void fs_obj::followPath(FS *fs, directory_t *searched_dir, const std::string &path) {
  if (path.size() == 0) return;  // TODO: show error and abort.

  directory_t dir, temp_dir;
  std::string temp, dir_name;
  std::vector<std::string> directories;

  int i;

  if (path[0] != '/') {
    // FIXME: after updating fs working dir fix this.
    fs_obj::get_directory(fs, &dir, fs->get_working_dir_blk_index());
  } else {
    fs_obj::get_directory(fs, &dir, 0);
  }

  for (i = 0; i < path.size(); i++) {
    if (i == 0 && path[0] == '/') i++;
    if (path[i] == '/') {
      directories.push_back(temp);
      temp.clear();
      i++;
    }

    temp += path[i];
  }

  dir_name = temp;

  for (std::string name : directories) {
    fs_obj::get_directory(fs, &temp_dir, &dir, name.c_str());
    dir = temp_dir;
  }

  if (dir_name != "/") {
    *searched_dir = dir;
  } else {
    printf("Here: %s\n", dir_name.c_str());
    fs_obj::get_directory(fs, searched_dir, &dir, dir_name.c_str());
  }
}

void fs_obj::create_dir(FS *fs, directory_t *dir) { uint8_t block[ENTRY_SIZE]; }

void fs_obj::create_file(FS *fs, file_t *file) {}

void fs_obj::get_parent(FS *fs, fs_obj::directory_t *dir, const fs_obj::dir_entry *entry) { fs_obj::get_directory(fs, dir, entry->parent_blk); }
