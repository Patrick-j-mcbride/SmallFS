#include <stdio.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

#include <driver.h>
#include <sfs_superblock.h>
#include <sfs_inode.h>
#include <sfs_dir.h>

#ifdef __cplusplus
}
#endif


using namespace std;

// void get_file_block(sfs_inode *inode, int block_num, char *block) {
//     int block_index = inode->direct[block_num];
//     driver_read(block, block_index);
// }

void ls(char *filename, bool l) {
    char raw_superblock[128];
    sfs_superblock *super = (sfs_superblock *)raw_superblock;
    driver_attach_disk_image(filename, 128);

  /* CHANGE THE FOLLOWING CODE SO THAT IT SEARCHES FOR THE SUPERBLOCK */
  /* read block 10 from the disk image */
    driver_read(super,10);
  /* is it the filesystem superblock? */
    if(super->fsmagic == VMLARIX_SFS_MAGIC &&! strcmp(super->fstypestr,VMLARIX_SFS_TYPESTR)){
        printf("superblock found at block 10!\n");
    }
    else{
      /* read block 0 from the disk image */
        driver_read(super,0);
        if(super->fsmagic == VMLARIX_SFS_MAGIC &&! strcmp(super->fstypestr,VMLARIX_SFS_TYPESTR)){
            printf("superblock found at block 0!\n");
        }
        else{
            printf("superblock is not at block 10 or block 0\nI quit!\n");
            exit(1);
        }
    }

  /* close the disk image */
    driver_detach_disk_image();

}

int main(int argc, char **argv) {
    if (argc < 2) { // No arguments
        printf("Usage: <filename> <-l>(optional)");
        return 1;
    }
    else if (argc == 2) { // No -l
        char *filename = argv[1];
        ls(filename, false);
    }
    else if (argc == 3) { // -l
        char *filename = argv[1];
        if (strcmp(argv[2], "-l") != 0) {
            printf("Usage: <filename> <-l>(optional)");
            return 1;
        }
        ls(filename, true);
    }
    else {
        printf("Usage: <filename> <-l>(optional)");
        return 1;
    }
}