#ifndef __FS_ENTRY__
#define __FS_ENTRY__

#include "disk.h"

#include <cstdint>
#include <string>
#include <vector>

namespace fs {
struct dir_entry {
  char file_name[56];     //Name of entry
  uint32_t size;          //Content size 
  uint16_t first_blk;     //First block  index 
  uint16_t parent_blk;    //Parent disk index
  uint8_t type;           //File or directory
  uint8_t access_rights;  //Who can access
};
struct dir_child {
  char file_name[56]; //Name of child
  uint16_t first_bl;  //first block on disk
};
struct directory_t {
  dir_entry attributes;             //Directory attributes
  std::vector<dir_child> children;  //The directorys children
};
struct file_t {
  dir_entry attributes; //File attributes
  std::string content;  //File content
};

  void get_directory(Disk *disk, directory_t *dir, const uint16_t& blk_index);
  void get_directory(Disk *disk, directory_t *dir, directory_t *parent_dir, const char name[56]);

  void get_file(Disk* disk, file_t *file, const uint16_t& blk_index);
  void get_file(Disk *disk, file_t *file, directory_t *parent_dir, const char name[56]);

  void get_parent(directory_t *dir, const dir_entry *entry);
}

#endif // !__FS_ENTRY__
