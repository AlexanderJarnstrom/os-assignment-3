#include "entry.h"
#include "constants.h"

#include <cstdint>

void
fs::get_directory(Disk *disk, directory_t* dir, const uint16_t &blk_index) {
  uint8_t block[ENTRY_SIZE] = {0};
  uint32_t temp;
  int i;

  disk->read(blk_index, block);
 
  for (i = 0; i < F_NAME_SIZE; i++) {
    dir->attributes.file_name[i] = block[i];
  }

  for (i = F_NAME_SIZE; i < F_SIZE_SIZE; i++) {
    
  }
}
