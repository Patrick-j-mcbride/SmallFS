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

// This class is used to store the information of a file
class FileData
{
public:
    string name;                          // filename
    uint32_t inode;                       // inode number
    uint32_t owner;                       // owner
    uint32_t group;                       // group
    uint32_t ctime;                       // creation time
    uint32_t mtime;                       // modification time
    uint32_t atime;                       // access time
    uint16_t perm;                        // permissions
    uint8_t type;                         // file type
    uint8_t refcount;                     // reference count
    uint64_t size;                        // file size
    uint32_t direct[NUM_DIRECT];          // direct blocks
    uint32_t indirect;                    // indirect block
    uint32_t dindirect;                   // double indirect block
    uint32_t tindirect;                   // triple indirect block
    void print_ls_al_info();              // print the information of the file
    string date_from_time(uint32_t time); // convert time to date
};

// This class is holds all disk operations
class Disk
{
public:
    Disk(char *image_file_name, int block_size); // constructor
    ~Disk();                                     // destructor

    void attach_disk(); // attach the disk
    void detach_disk(); // detach the disk

    void read(void *ptr, uint32_t block_num);  // read from the disk (just a wrapper for driver_read)
    void write(void *ptr, uint32_t block_num); // write to the disk (just a wrapper for driver_write)

    void get_super();         // get the superblock
    void get_root();          // get the root inode
    void get_files_in_root(); // get the files in the root directory

    void put_file_in_root(char *filename, uint32_t inode_num); // put a file in the root directory

    void copy_file_out(char *filename); // copy a file out of the disk
    void copy_file_in(char *filename);  // copy a file into the disk

    void fix_disk(bool verbose); // fix the disk

    void add_file_data(sfs_inode *inode, vector<int> free_blocks, char *data, int blocks); // add data to a file
    void update_free_blocks_list(vector<int> used_blocks);                                 // update the free blocks list

    void print_filenames();             // print the filenames in the root directory
    void print_superblock();            // print the superblock
    void print_inode(sfs_inode *inode); // print an inode
    void print_file_info();             // print the information of the files in the root directory
    void print_inode_bitmap();          // print the inode bitmap
    void print_block_bitmap();          // print the block bitmap
    void print_disk_info();             // print the disk information

    bool file_exists(char *filename); // check if a file exists in the root directory

    int get_file_block(sfs_inode *inode, int block_num, char *block); // get a block of a file
    int get_free_inode();                                             // get a free inode

    vector<int> get_free_blocks_list(); // get the free blocks list

    uint32_t get_num_free_blocks(); // get the number of free blocks
    uint32_t get_num_free_inodes(); // get the number of free inodes

    uint16_t formatPermissions(mode_t mode); // format the permissions

    vector<FileData> filedata; // vector to store the file data

    char *image_file_name;    // name of the disk image file
    char raw_superblock[128]; // raw superblock

    sfs_inode *rootnode;   // root inode
    sfs_inode *root_store; // root inode store

    sfs_superblock *super = (sfs_superblock *)raw_superblock; // superblock

    int superblock_num; // superblock number
    int block_size;     // block size
};

#endif