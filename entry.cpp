#include "entry.h"

#include <cstdint>
#include <cstring>

#include "constants.h"

int needed_blocks(int total_size, const int needed_size) {
  int needed_blocks = 0;

  while (total_size > 0) {
    needed_blocks++;
    total_size -= needed_size;
  }
  return needed_blocks;
}

void fs_obj::get_directory(FS *fs, fs_obj::directory_t *dir, const uint16_t &blk_index) {
  uint8_t block[ENTRY_SIZE] = {0};
  uint32_t temp = 0;
  int i, current_i;

  Disk *disk = fs->get_disk();
  int16_t *fat = fs->get_fat();

  disk->read(blk_index, block);
  // Extract directory attributes
  for (i = 0; i < F_NAME_SIZE; i++) {
    dir->attributes.file_name[i] = block[i];
  }

  current_i = i;

  for (i = current_i + F_SIZE_SIZE - 1; i >= F_NAME_SIZE; --i) {
    temp = (temp << ((F_NAME_SIZE + F_SIZE_SIZE) - i)) | block[i];
  }

  current_i = i + F_SIZE_SIZE;
  dir->attributes.size = temp;
  temp = 0;

  for (i = current_i + F_FIRST_BLOCK_SIZE - 1; i >= current_i; --i) {
    temp = (temp << ((current_i + F_FIRST_BLOCK_SIZE) - i)) | block[i];
  }

  dir->attributes.first_blk = dir->attributes.first_blk | temp;

  current_i = current_i + F_FIRST_BLOCK_SIZE;
  dir->attributes.type = block[++current_i];
  dir->attributes.access_rights = block[++current_i];

  // TODO: handle error
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

void fs_obj::get_directory(FS *fs, directory_t *dir, directory_t *parent_dir, const char *name) {}
