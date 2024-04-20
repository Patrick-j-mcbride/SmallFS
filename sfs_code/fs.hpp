#ifndef FS_HPP
#define FS_HPP
#include <stdio.h>
#include <string>
#include <unistd.h>
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

#ifdef __cplusplus
}
#endif

using namespace std;

class ls_data
{
public:
    string name;
    uint32_t inode;
};

class FileData
{
public:
    string name;
    uint32_t inode;
};

class Disk
{
public:
    Disk(char *image_file_name, int block_size);
    ~Disk();
    void attach_disk();
    void detach_disk();
    void read(void *ptr, uint32_t block_num);
    void write(void *ptr, uint32_t block_num);
    void get_super();
    void get_root();
    void get_files_in_root();
    void print_filenames();
    bool file_exists(char *filename);
    void copy_file_out(char *filename);
    void get_file_block(sfs_inode *inode, int block_num, char *block);

    vector<FileData> filedata;
    char *image_file_name;
    sfs_inode *rootnode;
    int block_size;
    char raw_superblock[128];
    sfs_superblock *super = (sfs_superblock *)raw_superblock;
};

#endif