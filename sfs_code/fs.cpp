#include "fs.hpp"
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
Disk::Disk(char *image_file_name, int block_size)
{
    this->image_file_name = image_file_name;
    this->block_size = block_size;
    attach_disk();
    get_super();
    get_root();
}
Disk::~Disk()
{
    detach_disk();
}
void Disk::attach_disk()
{
    driver_attach_disk_image(this->image_file_name, this->block_size);
}
void Disk::detach_disk()
{
    driver_detach_disk_image();
}
void Disk::read(void *ptr, uint32_t block_num)
{
    read(ptr, block_num);
}
void Disk::write(void *ptr, uint32_t block_num)
{
    write(ptr, block_num);
}
void Disk::get_super()
{
    int found = 0;
    int block_num = 0;

    while (found == 0)
    {
        driver_read(super, block_num);
        if (super->fsmagic == VMLARIX_SFS_MAGIC && !strcmp(super->fstypestr, VMLARIX_SFS_TYPESTR))
        {
            found = 1;
        }
        else
        {
            block_num++;
        }
    }
}
void Disk::get_root()
{
    sfs_inode *root_inodes = (sfs_inode *)malloc(2 * sizeof(sfs_inode));
    driver_read(root_inodes, super->inodes);
    this->rootnode = &root_inodes[0];
}

void Disk::get_files_in_root()
{
    int files_in_root = rootnode->size / sizeof(sfs_dirent);
    int ct = 0;
    sfs_dirent *dirents = (sfs_dirent *)malloc(sizeof(sfs_dirent) * 4);
    sfs_dirent *dirent;
    for (int i = 0; i < 5; i++)
    {
        driver_read(dirents, rootnode->direct[i]);
        for (int i = 0; i < 4; i++)
        {
            FileData data;
            dirent = &dirents[i];
            data.name = dirent->name;
            data.inode = dirent->inode;
            filedata.push_back(data);
            ct++;
            if (ct == files_in_root)
            {
                return;
            }
        }
    }
    free(dirents);
}
void Disk::print_filenames()
{
    for (int i = 0; i < filedata.size(); i++)
    {
        cout << filedata[i].name << endl;
    }
}
bool Disk::file_exists(char *filename)
{
    for (int i = 0; i < filedata.size(); i++)
    {
        if (filedata[i].name == filename)
        {
            return true;
        }
    }
    return false;
}
void Disk::copy_file_out(char *filename)
{
    for (int i = 0; i < filedata.size(); i++)
    {
        if (filedata[i].name == filename)
        {
            int idx = 0;
            sfs_inode *inodes = (sfs_inode *)malloc(2 * sizeof(sfs_inode));

            if (filedata[i].inode % 2 != 0)
            {
                idx = 1;
            }
            uint32_t bloknm = super->inodes + (filedata[i].inode / 2);

            sfs_inode *inode = &inodes[idx];
            // get the address of the inode table
            driver_read(inodes, bloknm);

            int blocks = inode->size / block_size;
            if (inode->size % block_size != 0)
            {
                blocks++;
            }
            char *data = (char *)malloc(inode->size);
            char *block = (char *)malloc(block_size);
            for (int i = 0; i < blocks; i++)
            {
                get_file_block(inode, i, block);
                memcpy(data + i * block_size, block, block_size);
            }
            FILE *file = fopen(filename, "w");
            fwrite(data, inode->size, 1, file);
            fclose(file);
            return;
        }
    }
}
void Disk::get_file_block(sfs_inode *inode, int block_num, char *block)
{
    uint32_t ptrs[32] = {0};
    if (block_num < 5)
    {
        driver_read(block, inode->direct[block_num]);
    }
    else if (block_num < 37)
    {
        driver_read(ptrs, inode->indirect);
        driver_read(block, ptrs[block_num - 5]);
    }
    else if (block_num < 5 + 32 + (32 * 32))
    {
        driver_read(ptrs, inode->dindirect);
        driver_read(ptrs, ptrs[(block_num - 37) / 32]);
        driver_read(block, ptrs[(block_num - 37) % 32]);
    }
    else if (block_num < 5 + 32 + (32 * 32) + (32 * 32 * 32))
    {
        driver_read(ptrs, inode->tindirect);
        driver_read(ptrs, ptrs[(block_num - 5 - 32 - (32 * 32)) / (32 * 32)]);
        driver_read(ptrs, ptrs[(block_num - 5 - 32 - (32 * 32)) / 32]);
        driver_read(block, ptrs[(block_num - 5 - 32 - (32 * 32)) % 32]);
    }
    else
    {
        cout << "Block number out of range" << endl;
        exit(1);
    }
}
