#include "fs.hpp"
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <ctime>
#include <iomanip>

#ifdef __cplusplus
extern "C"
{
#endif

#include <driver.h>
#include <sfs_superblock.h>
#include <sfs_inode.h>
#include <sfs_dir.h>
#include <bitmap.h>

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
            superblock_num = block_num;
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

void Disk::copy_file_in(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        cout << filename << " does not exist" << endl;
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    int blocks = size / block_size;
    if (size % block_size != 0)
    {
        blocks++;
    }
    fseek(file, 0, SEEK_SET);
    char *data = (char *)malloc(size);
    fread(data, size, 1, file);
    fclose(file);

    // Find a free inode
    int inode_num = find_free_inode();
    cout << "Inode number: " << inode_num << endl;
    if (inode_num == -1)
    {
        cout << "No free inodes on the disk image" << endl;
        exit(1);
    }
    vector<int> free_blocks = find_free_blocks(blocks);
    cout << "Free blocks: " << free_blocks.size() << endl;
    if (free_blocks.size() < blocks)
    {
        cout << "Not enough free blocks on the disk image" << endl;
        exit(1);
    }

    cout << "Size of file: " << size << endl;
    cout << "Number of blocks: " << blocks << endl;
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
    cout << left << setw(40) << setfill('=') << "========SUPERBLOCK=INFO" << endl;
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
    cout << left << setw(40) << setfill('=') << "" << endl;
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
        filedata[i].direct[0] = inode->direct[0];
        filedata[i].direct[1] = inode->direct[1];
        filedata[i].direct[2] = inode->direct[2];
        filedata[i].direct[3] = inode->direct[3];
        filedata[i].direct[4] = inode->direct[4];
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
    cout << perm_str;

    cout << right << setw(3) << to_string(refcount) << " ";

    cout << right << setw(4) << to_string(owner) << " ";

    cout << right << setw(4) << to_string(group) << " ";

    cout << right << setw(8) << to_string(size) << " ";

    cout << date_from_time(ctime) << " ";

    cout << name << endl;
}

string FileData::date_from_time(uint32_t time)
{
    // Convert uint32_t time to time_t
    time_t raw_time = time;

    struct tm *timeinfo = gmtime(&raw_time);

    // Buffer to store the formatted date and time
    char buffer[80];

    // Format the date and time: "Mon 2 Jan 2006 15:04"
    strftime(buffer, sizeof(buffer), "%b %d %Y %H:%M", timeinfo);

    // Return the formatted string
    return string(buffer);
}

int Disk::find_free_inode()
{
    bitmap_t *bitmaps = (bitmap_t *)malloc(block_size);
    for (int i = 0; i < super->fi_bitmapblocks; i++)
    {
        driver_read(bitmaps, super->fi_bitmap + i);
        for (int j = 0; j < 8; j++)
        {
            bitmap_t bitmap = bitmaps[j];
            for (int k = 0; k < 32; k++)
            {
                uint8_t bit = get_bit(&bitmap, k);
                if (bit == 0)
                {
                    return i * 1024 + j * 32 + k;
                }
            }
        }
    }
    return -1;
}

vector<int> Disk::find_free_blocks(int num_blocks)
{
    vector<int> free_blocks;
    bitmap_t *bitmap = (bitmap_t *)malloc(block_size);
    for (int i = 0; i < super->fb_bitmapblocks; i++)
    {
        driver_read(bitmap, super->fb_bitmap + i);
        for (int j = 0; j <= 1024; j++)
        {
            uint8_t bit = get_bit(bitmap, j);
            if (bit == 0)
            {
                free_blocks.push_back(i * 1024 + j);
            }
        }
    }
    return free_blocks;
}