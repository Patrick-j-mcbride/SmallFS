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
    // print_superblock();
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
    int block_idx = 0;
    sfs_dirent *dirents = (sfs_dirent *)malloc(sizeof(sfs_dirent) * 4);
    sfs_dirent *dirent;

    while (ct < files_in_root)
    {
        get_file_block(rootnode, block_idx, (char *)dirents);
        block_idx++;
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
    uint32_t triple_ptrs[32] = {0};
    uint32_t double_ptrs[32] = {0};

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
        driver_read(ptrs, inode->tindirect);                                          // Read triple indirect block pointers
        driver_read(triple_ptrs, ptrs[(block_num - 5 - 32 - (32 * 32)) / (32 * 32)]); // Read level 3 pointers

        int second_level_index = ((block_num - 5 - 32 - (32 * 32)) % (32 * 32)) / 32;
        driver_read(double_ptrs, triple_ptrs[second_level_index]); // Read level 2 pointers

        int direct_index = (block_num - 5 - 32 - (32 * 32)) % 32;
        driver_read(block, double_ptrs[direct_index]); // Read direct block
    }
    else
    {
        cout << "Block number out of range" << endl;
        exit(1);
    }
}

void Disk::print_superblock()
{
    cout << "Filesystem Magic Number: " << super->fsmagic << endl;
    cout << "Filesystem Type String: " << super->fstypestr << endl;
    cout << "Block Size (bytes): " << super->block_size << endl;
    cout << "Sectors Per Block: " << super->sectorsperblock << endl;
    cout << "Superblock Location: " << super->superblock << endl;
    cout << "Total Number of Blocks: " << super->num_blocks << endl;
    cout << "First Block of Free Block Bitmap: " << super->fb_bitmap << endl;
    cout << "Number of Blocks in Free Block Bitmap: " << super->fb_bitmapblocks << endl;
    cout << "Number of Unused Blocks: " << super->blocks_free << endl;
    cout << "Total Number of Inodes: " << super->num_inodes << endl;
    cout << "First Block of Free Inode Bitmap: " << super->fi_bitmap << endl;
    cout << "Number of Blocks in Free Inode Bitmap: " << super->fi_bitmapblocks << endl;
    cout << "Number of Unused Inodes: " << super->inodes_free << endl;
    cout << "Number of Blocks in the Inode Table: " << super->num_inode_blocks << endl;
    cout << "First Block of the Inode Table: " << super->inodes << endl;
    cout << "First Block of the Root Directory: " << super->rootdir << endl;
    cout << "Open Files Count: " << super->open_count << endl;
}

void Disk::print_file_info()
{
    for (int i = 0; i < filedata.size(); i++)
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

        filedata[i].owner = inode->owner;
        filedata[i].group = inode->group;
        filedata[i].ctime = inode->ctime;
        filedata[i].mtime = inode->mtime;
        filedata[i].atime = inode->atime;
        filedata[i].perm = inode->perm;
        filedata[i].type = inode->type;
        filedata[i].refcount = inode->refcount;
        filedata[i].size = inode->size;
        filedata[i].direct[NUM_DIRECT] = inode->direct[NUM_DIRECT];
        filedata[i].indirect = inode->indirect;
        filedata[i].dindirect = inode->dindirect;
        filedata[i].tindirect = inode->tindirect;
        filedata[i].print_ls_al_info();
    }
}

void FileData::print_ls_al_info()
{
    string output_str = "";
    // Create the permissions string
    string perm_str = "";

    switch (type)
    {
    case FT_NORMAL:
        perm_str += "-";
        break;
    case FT_DIR:
        perm_str += "d";
        break;
    case FT_CHAR_SPEC:
        perm_str += "c";
        break;
    case FT_BLOCK_SPEC:
        perm_str += "b";
        break;
    case FT_PIPE:
        perm_str += "p";
        break;
    case FT_SOCKET:
        perm_str += "s";
        break;
    case FT_SYMLINK:
        perm_str += "l";
        break;
    default:
        perm_str += "?";
        break;
    }
    // perm holds the permissions values for the file
    // Other is the last 3 bits of the permissions
    // Group is the next 3 bits of the permissions
    // Owner is the next 3 bits of the permissions
    // The remaining leftmost bits are unused
    // Using the bitwise AND operator to get each bit from left to right
    // and adding the corresponding character to the permissions string
    // Must convert perm to an int to use bitwise AND

    perm_str += (perm & 0x100) ? "r" : "-";
    perm_str += (perm & 0x80) ? "w" : "-";
    perm_str += (perm & 0x40) ? "x" : "-";
    perm_str += (perm & 0x20) ? "r" : "-";
    perm_str += (perm & 0x10) ? "w" : "-";
    perm_str += (perm & 0x8) ? "x" : "-";
    perm_str += (perm & 0x4) ? "r" : "-";
    perm_str += (perm & 0x2) ? "w" : "-";
    perm_str += (perm & 0x1) ? "x" : "-";
    // add a space after the permissions string
    perm_str += " ";
    output_str += perm_str;
    // add the number of hard links to the file
    output_str += to_string(refcount) + " ";

    cout << output_str << name << size << endl;
}