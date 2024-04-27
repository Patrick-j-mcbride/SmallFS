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

    uint32_t owner;
    uint32_t group;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t atime;
    uint16_t perm;
    uint8_t type;
    uint8_t refcount;
    uint64_t size;
    uint32_t direct[NUM_DIRECT];
    uint32_t indirect;
    uint32_t dindirect;
    uint32_t tindirect;
    void print_ls_al_info();
    string date_from_time(uint32_t time);
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
    void copy_file_in(char *filename);
    int get_file_block(sfs_inode *inode, int block_num, char *block);
    void print_superblock();
    void print_inode(sfs_inode *inode);
    void print_file_info();
    void print_inode_bitmap();
    void print_block_bitmap();
    void put_file_in_root(char *filename, uint32_t inode_num);
    int get_free_inode();
    void fix_disk(bool verbose);
    vector<int> get_free_blocks_list();
    uint32_t get_num_free_blocks();
    uint32_t get_num_free_inodes();
    void print_disk_info();
    uint16_t formatPermissions(mode_t mode);
    void add_file_data(sfs_inode *inode, vector<int> free_blocks, char *data, int blocks);
    void update_free_blocks_list(vector<int> used_blocks);

    vector<FileData> filedata;
    char *image_file_name;
    sfs_inode *rootnode;
    int block_size;
    char raw_superblock[128];
    sfs_superblock *super = (sfs_superblock *)raw_superblock;
    int superblock_num;
    sfs_inode *root_store;
};

#endif