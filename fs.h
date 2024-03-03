#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "disk.h"

#ifndef __FS_H__
#define __FS_H__

#define ROOT_BLOCK 0
#define FAT_BLOCK 1
#define FAT_FREE 0
#define FAT_EOF -1

#define TYPE_FILE 0
#define TYPE_DIR 1
#define READ 0x04
#define WRITE 0x02
#define EXECUTE 0x01

#define START_ROOT 0xff
#define START_WDIR 0x00

#define ENTRY_CONTENT_SIZE 4032
#define ENTRY_ATTRIBUTE_SIZE 64

#define REMOVE_DIR_CHILD 0x00
#define ADD_DIR_CHILD 0xff

struct dir_entry {
  char file_name[56];     // name of the file / sub-directory
  uint32_t size;          // size of the file in bytes
  uint16_t first_blk;     // index in the FAT for the first block of the file
  uint8_t type;           // directory (1) or file (0)
  uint8_t access_rights;  // read (0x04), write (0x02), execute (0x01)
};

struct path_obj {
  uint8_t start;                  // Where to start, Root (0xff), working dir (0x00)
  std::vector<std::string> dirs;  // List of directory names
  std::string end;                // The final directory.
};

struct dir_child {
  char file_name[56];
  uint16_t index;
};

class FS {
 private:
  Disk disk;
  dir_entry *working_dir;
  // size of a FAT entry is 2 bytes
  int16_t fat[BLOCK_SIZE / 2];

  void load_fat();
  void update_fat();
  void empty_array(uint8_t *arr, const int &size);
  void fill_attr_array(uint8_t *attr, const int &size, dir_entry *entry);

  dir_entry *follow_path(const path_obj *path);
  dir_entry *get_child(const dir_entry *parent, const std::string &name);
  void create_dir_entry(struct dir_entry *entry, const std::string file_content, dir_entry *parent, const int &fat_index = -1);
  void update_dir_content(dir_entry *entry, dir_child *child, const uint8_t &task = ADD_DIR_CHILD);

  void write_block(uint8_t attr[ENTRY_ATTRIBUTE_SIZE], uint8_t cont[ENTRY_CONTENT_SIZE], unsigned block_no);

  dir_entry *read_block_attr(uint16_t block_index);

  std::vector<dir_child *> read_cont_dir(const dir_entry *directory);
  std::string read_cont_file(const dir_entry *entry);

  int format_path(std::string &path_s, path_obj *path);

  int calc_needed_blocks(const unsigned long &size);

 public:
  FS();
  ~FS();

  Disk *get_disk();

  int16_t *get_fat();

  int16_t get_working_dir_blk_index();

  // formats the disk, i.e., creates an empty file system
  int format();
  // create <filepath> creates a new file on the disk, the data content is
  // written on the following rows (ended with an empty row)
  int create(std::string filepath);
  // cat <filepath> reads the content of a file and prints it on the screen
  int cat(std::string filepath);
  // ls lists the content in the current directory (files and sub-directories)
  int ls();

  // cp <sourcepath> <destpath> makes an exact copy of the file
  // <sourcepath> to a new file <destpath>
  int cp(std::string sourcepath, std::string destpath);
  // mv <sourcepath> <destpath> renames the file <sourcepath> to the name
  // <destpath>, or moves the file <sourcepath> to the directory <destpath> (if
  // dest is a directory)
  int mv(std::string sourcepath, std::string destpath);
  // rm <filepath> removes / deletes the file <filepath>
  int rm(std::string filepath);
  // append <filepath1> <filepath2> appends the contents of file <filepath1> to
  // the end of file <filepath2>. The file <filepath1> is unchanged.
  int append(std::string filepath1, std::string filepath2);

  // mkdir <dirpath> creates a new sub-directory with the name <dirpath>
  // in the current directory
  int mkdir(std::string dirpath);
  // cd <dirpath> changes the current (working) directory to the directory named
  // <dirpath>
  int cd(std::string dirpath);
  // pwd prints the full path, i.e., from the root directory, to the current
  // directory, including the current directory name
  int pwd();

  // chmod <accessrights> <filepath> changes the access rights for the
  // file <filepath> to <accessrights>.
  int chmod(std::string accessrights, std::string filepath);
};

#endif  // __FS_H__
