#include <iostream>
#include <vector>
#include <cstring>

#ifdef __cplusplus
extern "C"
{
#endif

#include <driver.h>
#include <sfs_superblock.h>
#include <sfs_inode.h>
#include <sfs_dir.h>
#include <helpers.h>

#ifdef __cplusplus
}
#endif

void copy_file_out(char *diskname, char *filename)
{
    char raw_superblock[128];
    sfs_superblock *super = (sfs_superblock *)raw_superblock;

    driver_attach_disk_image(diskname, 128);
    get_superblock(super);
    // get the first inode
    sfs_inode *inode = (sfs_inode *)malloc(sizeof(sfs_inode));
    driver_read(inode, super->inodes);
    printf("Root inode found at block %d\n", super->inodes);
    // output the type for the inode
    if (inode->type == FT_DIR)
    {
        printf("Root inode is a directory\n");
    }
    else
    {
        printf("Root inode is a file\n");
    }

    driver_detach_disk_image();
}

int main(int argc, char **argv)
{
    if (argc < 3)
    { // No arguments
        printf("Usage: <diskname> <filename>");
        return 1;
    }
    else if (argc == 3)
    {
        char *diskname = argv[1];
        char *filename = argv[2];

        copy_file_out(diskname, filename);
    }
    else
    {
        printf("Usage: <diskname> <filename>");
        return 1;
    }
}