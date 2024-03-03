#ifndef __FS_ENTRY__
#define __FS_ENTRY__

#include <cstdint>
#include <string>
#include <vector>

#include "fs.h"

namespace fs_obj {
struct dir_entry {
  char file_name[56] = {0};   // Name of entry
  uint32_t size = 0;          // Content size
  uint16_t first_blk = 0;     // First block  index
  uint16_t parent_blk = 0;    // Parent disk index
  uint8_t type = 0;           // File or directory
  uint8_t access_rights = 0;  // Who can access
};
struct dir_child {
  char file_name[56] = {0};  // Name of child
  uint16_t first_blk = 0;    // first block on disk
};
struct directory_t {
  dir_entry attributes;               // Directory attributes
  std::vector<dir_child *> children;  // The directorys children
};
struct file_t {
  dir_entry attributes;  // File attributes
  std::string content;   // File content
};

/* Get a directory from disk with fat index
 * @param FS *fs filesystem
 * @param directory_t *dir directory obj
 * @param const uint16_t &blk_index fat index
 */
void get_directory(FS *fs, directory_t *dir, const uint16_t &blk_index);
/* Get a directory from disk with parent
 * @param FS *fs filesystem
 * @param directory_t *dir directory obj
 * @param directory_t *parent_dir parent directory to dir
 * @param const char name[56] name of searched directory
 * */
void get_directory(FS *fs, directory_t *dir, directory_t *parent_dir, const char name[56]);

/* Get a file from disk with fat index
 * @param FS *fs filesystem
 * @param file_t *file the loaded file
 * @param const uint16_t &blk_index fat index
 * */
void get_file(FS *fs, file_t *file, const uint16_t &blk_index);
/* Get file from disk with parent
 * @param FS *fs filesystem
 * @param file_t *file the loaded file
 * @param directory_t *parent_dir parent directory to the file
 * @param const char name[56] name of the search file
 * */
void get_file(FS *fs, file_t *file, directory_t *parent_dir, const char name[56]);

/* Follows the given path and searches for a file.
 * @param FS *fs filesystem
 * @param file_t *file the found file, will be nullptr if path doesn't exist
 * @param const std::String &path the path to follow
 * */
void followPath(FS *fs, file_t *file, const std::string &path);
/* Follows the given path and searches for a directory.
 * @param FS *fs filesystem
 * @param directory_t *dir the found directory, will be nullptr if path doesn't exist
 * @param const std::string &path the path to follow
 * */
void followPath(FS *fs, directory_t *searched_dir, const std::string &path);

/* Format the given directory and write it to disk
 * @param FS *fs filesystem
 * @param directory_t *dir directory to be written to disk
 * @param directory_t *parent parent to the directory which needs to be updated
 */
void create_dir(FS *fs, directory_t *dir, directory_t *parent);
/* Format the given file and write it to disk
 * @param FS *fs filesystem
 * @param file_t *file file to be written to disk
 * @param directory_t *parent parent to the file which needs to be updated
 */
void create_file(FS *fs, file_t *file, directory_t *parent);
}  // namespace fs_obj

#endif  // !__FS_ENTRY__
