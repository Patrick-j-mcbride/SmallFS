#include "fs.hpp"
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sys/stat.h>

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
    driver_read(ptr, block_num);
}

void Disk::write(void *ptr, uint32_t block_num)
{
    driver_write(ptr, block_num);
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
}

void Disk::get_root()
{
    root_store = (sfs_inode *)malloc(2 * sizeof(sfs_inode));
    driver_read(root_store, super->inodes);
    this->rootnode = &root_store[0];
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
                if (dirents != NULL)
                {
                    free(dirents);
                }
                return;
            }
        }
    }
    if (dirents != NULL)
    {
        free(dirents);
    }
}

void Disk::put_file_in_root(char *filename, uint32_t inode_num)
{
    int files_in_root = (rootnode->size / sizeof(sfs_dirent)) + 1;
    int ct = 0;
    int block_idx = 0;
    int loc;
    sfs_dirent *dirents = (sfs_dirent *)malloc(sizeof(sfs_dirent) * 4);
    sfs_dirent *dirent;

    while (ct < files_in_root)
    {
        loc = get_file_block(rootnode, block_idx, (char *)dirents);
        for (int i = 0; i < 4; i++)
        {
            dirent = &dirents[i];
            ct++;
            if (ct == files_in_root)
            {
                dirent->inode = inode_num;
                strcpy(dirent->name, filename);
                driver_write(dirents, (uint32_t)loc);
                // get a fresh copy of rootnode
                root_store = (sfs_inode *)malloc(2 * sizeof(sfs_inode));
                driver_read(root_store, super->inodes);
                this->rootnode = &root_store[0];
                // set the new size of the root node
                rootnode->size += sizeof(sfs_dirent);
                // write the new root node to the disk
                driver_write(root_store, super->inodes);
                if (dirents != NULL)
                {
                    free(dirents);
                }
                return;
            }
        }
        block_idx++;
    }
    if (dirents != NULL)
    {
        free(dirents);
    }
}

void Disk::print_filenames()
{
    for (int i = 0; i < (int)filedata.size(); i++)
    {
        cout << filedata[i].name << endl;
    }
}

bool Disk::file_exists(char *filename)
{
    for (int i = 0; i < (int)filedata.size(); i++)
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
    for (int i = 0; i < (int)filedata.size(); i++)
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
            char *data = (char *)malloc(inode->size);
            char *block = (char *)malloc(block_size);
            if (blocks == 0)
            {
                get_file_block(inode, 0, block);
                memcpy(data, block, inode->size);
            }
            else
            {
                for (int j = 0; j < blocks; j++)
                {
                    get_file_block(inode, j, block);
                    if (j == blocks - 1 && (inode->size % block_size) != 0)
                    {
                        memcpy(data + j * block_size, block, block_size); // Copy the last full block
                        // Only copy the relevant portion of the last block
                        j++;
                        get_file_block(inode, j, block);
                        memcpy(data + j * block_size, block, inode->size % block_size);
                    }
                    else
                    {
                        memcpy(data + j * block_size, block, block_size);
                    }
                }
            }
            FILE *file = fopen(filename, "w");
            fwrite(data, inode->size, 1, file);
            fclose(file);
            if (inodes != NULL)
            {
                free(inodes);
            }
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
    struct stat fileStat;
    if (stat(filename, &fileStat) < 0)
    {
        cout << "Could not get permissions for the file" << endl;
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    int orig_size = size;

    fseek(file, 0, SEEK_SET);
    // check if size alligned with block size
    if (size % block_size != 0)
    {
        size = size + (block_size - (size % block_size));
    }
    int blocks = size / block_size;
    char *data = (char *)malloc(size);
    fread(data, size, 1, file);
    fclose(file);
    int remaining_size = size - orig_size;
    if (remaining_size > 0)
    {
        memset(data + orig_size, 0, remaining_size);
    }

    // Check if there are enough free blocks on the disk image
    vector<int> free_blocks = get_free_blocks_list();
    if ((int)free_blocks.size() < (int)blocks)
    {
        cout << "Not enough free blocks on the disk image: exiting..." << endl;
        exit(1);
    }

    // Find a free inode
    int inode_num = get_free_inode();
    if (inode_num == -1)
    {
        cout << "No free inodes on the disk image: exiting..." << endl;
        exit(1);
    }

    int idx = 0;
    sfs_inode *inodes = (sfs_inode *)malloc(2 * sizeof(sfs_inode));

    if (inode_num % 2 != 0)
    {
        idx = 1;
    }
    uint32_t bloknm = super->inodes + (inode_num / 2);

    sfs_inode *inode = &inodes[idx];
    // get the empty inode
    driver_read(inodes, bloknm);
    inode->owner = (uint32_t)0;
    inode->group = (uint32_t)0;
    // Set the creation time to the current time
    inode->ctime = (uint32_t)time(NULL);
    inode->mtime = (uint32_t)time(NULL);
    inode->atime = (uint32_t)time(NULL);
    inode->perm = formatPermissions(fileStat.st_mode);
    inode->type = FT_NORMAL;
    inode->refcount = (uint8_t)1;
    inode->size = (uint64_t)orig_size;
    add_file_data(inode, free_blocks, data, blocks);
    put_file_in_root(filename, inode_num);
    driver_write(inodes, bloknm);
    fix_disk(false);
    if (inodes != NULL)
    {
        free(inodes);
    }
    if (data != NULL)
    {
        free(data);
    }
}

void Disk::add_file_data(sfs_inode *inode, vector<int> free_blocks, char *data, int blocks)
{
    uint32_t ptrs[32] = {0};
    uint32_t double_ptrs[32] = {0};
    uint32_t triple_ptrs[32] = {0};

    vector<int> used_blocks;
    int block_idx = 0;

    // Write direct blocks
    for (int i = 0; i < blocks; i++)
    {
        if (i < NUM_DIRECT) // Direct blocks
        {
            inode->direct[i] = free_blocks[block_idx++];
            driver_write(data + i * block_size, inode->direct[i]);
            used_blocks.push_back(inode->direct[i]);
        }
        else if (i < NUM_DIRECT + 32) // Single indirect blocks
        {
            if (inode->indirect == 0) // Allocate indirect block if not already allocated
            {
                inode->indirect = free_blocks[block_idx++];
                used_blocks.push_back(inode->indirect);
                memset(ptrs, 0, sizeof(ptrs)); // Initialize block pointers
            }
            else
            {
                driver_read(ptrs, inode->indirect);
            }

            ptrs[i - NUM_DIRECT] = free_blocks[block_idx++];
            driver_write(data + (i * block_size), ptrs[i - NUM_DIRECT]);
            driver_write(ptrs, inode->indirect); // Update the indirect block pointers
            used_blocks.push_back(ptrs[i - NUM_DIRECT]);
        }
        else if (i < NUM_DIRECT + 32 + 32 * 32) // Double indirect blocks
        {
            if (inode->dindirect == 0) // Allocate double indirect block if not already allocated
            {
                inode->dindirect = free_blocks[block_idx++];
                used_blocks.push_back(inode->dindirect);
                memset(ptrs, 0, sizeof(ptrs));
            }
            else
            {
                driver_read(ptrs, inode->dindirect);
            }

            int index1 = (i - NUM_DIRECT - 32) / 32;
            if (ptrs[index1] == 0)
            {
                ptrs[index1] = free_blocks[block_idx++];
                used_blocks.push_back(ptrs[index1]);
                memset(double_ptrs, 0, sizeof(double_ptrs));
            }
            else
            {
                driver_read(double_ptrs, ptrs[index1]);
            }

            double_ptrs[(i - NUM_DIRECT - 32) % 32] = free_blocks[block_idx++];
            driver_write(data + (i * block_size), double_ptrs[(i - NUM_DIRECT - 32) % 32]);
            driver_write(double_ptrs, ptrs[index1]); // Update double indirect block pointers
            driver_write(ptrs, inode->dindirect);    // Update the double indirect block
            used_blocks.push_back(double_ptrs[(i - NUM_DIRECT - 32) % 32]);
        }
        else if (i < NUM_DIRECT + 32 + 32 * 32 + 32 * 32 * 32) // Triple indirect blocks
        {
            if (inode->tindirect == 0) // Allocate triple indirect block if not already allocated
            {
                inode->tindirect = free_blocks[block_idx++];
                used_blocks.push_back(inode->tindirect);
                memset(ptrs, 0, sizeof(ptrs));
            }
            else
            {
                driver_read(ptrs, inode->tindirect);
            }

            int index1 = (i - NUM_DIRECT - 32 - 32 * 32) / (32 * 32);
            if (ptrs[index1] == 0)
            {
                ptrs[index1] = free_blocks[block_idx++];
                used_blocks.push_back(ptrs[index1]);
                memset(triple_ptrs, 0, sizeof(triple_ptrs));
            }
            else
            {
                driver_read(triple_ptrs, ptrs[index1]);
            }

            int index2 = ((i - NUM_DIRECT - 32 - 32 * 32) % (32 * 32)) / 32;
            if (triple_ptrs[index2] == 0)
            {
                triple_ptrs[index2] = free_blocks[block_idx++];
                used_blocks.push_back(triple_ptrs[index2]);
                memset(double_ptrs, 0, sizeof(double_ptrs));
            }
            else
            {
                driver_read(double_ptrs, triple_ptrs[index2]);
            }

            double_ptrs[(i - NUM_DIRECT - 32 - 32 * 32) % 32] = free_blocks[block_idx++];
            driver_write(data + (i * block_size), double_ptrs[(i - NUM_DIRECT - 32 - 32 * 32) % 32]);
            driver_write(double_ptrs, triple_ptrs[index2]); // Update the third level block pointers
            driver_write(triple_ptrs, ptrs[index1]);        // Update the second level block pointers
            driver_write(ptrs, inode->tindirect);           // Update the triple indirect block
            used_blocks.push_back(double_ptrs[(i - NUM_DIRECT - 32 - 32 * 32) % 32]);
        }
        else
        {
            std::cerr << "Block number out of range. Exiting..." << std::endl;
            exit(1);
        }
    }
    update_free_blocks_list(used_blocks);
}

int Disk::get_file_block(sfs_inode *inode, int block_num, char *block)
{
    uint32_t ptrs[32] = {0};
    uint32_t triple_ptrs[32] = {0};
    uint32_t double_ptrs[32] = {0};

    if (block_num < 5)
    {
        driver_read(block, inode->direct[block_num]);
        return inode->direct[block_num];
    }
    else if (block_num < 37)
    {
        driver_read(ptrs, inode->indirect);      // Read indirect block pointers
        driver_read(block, ptrs[block_num - 5]); // Read direct block
        return ptrs[block_num - 5];              // Return direct block
    }
    else if (block_num < 5 + 32 + (32 * 32))
    {
        driver_read(ptrs, inode->dindirect);
        driver_read(ptrs, ptrs[(block_num - 37) / 32]);
        driver_read(block, ptrs[(block_num - 37) % 32]);
        return ptrs[(block_num - 37) % 32];
    }
    else if (block_num < 5 + 32 + (32 * 32) + (32 * 32 * 32))
    {
        driver_read(ptrs, inode->tindirect);                                          // Read triple indirect block pointers
        driver_read(triple_ptrs, ptrs[(block_num - 5 - 32 - (32 * 32)) / (32 * 32)]); // Read level 3 pointers

        int second_level_index = ((block_num - 5 - 32 - (32 * 32)) % (32 * 32)) / 32;
        driver_read(double_ptrs, triple_ptrs[second_level_index]); // Read level 2 pointers

        int direct_index = (block_num - 5 - 32 - (32 * 32)) % 32;
        driver_read(block, double_ptrs[direct_index]); // Read direct block
        return double_ptrs[direct_index];
    }
    else
    {
        cout << "Block number out of range: exiting..." << endl;
        exit(1);
    }
}

void Disk::print_superblock()
{
    cout << left << setw(100) << setfill('=') << "========SUPERBLOCK=INFO" << endl;
    cout << "uint32_t fsmagic;          " << "Filesystem Magic Number:               " << super->fsmagic << endl;
    cout << "char fstypestr[32];        " << "Filesystem Type String:                " << super->fstypestr << endl;
    cout << "uint32_t block_size;       " << "Block Size (bytes):                    " << super->block_size << endl;
    cout << "uint32_t sectorsperblock;  " << "Sectors Per Block:                     " << super->sectorsperblock << endl;
    cout << "uint32_t superblock;       " << "Superblock Location:                   " << super->superblock << endl;
    cout << "uint32_t num_blocks;       " << "Total Number of Blocks:                " << super->num_blocks << endl;
    cout << "uint32_t fb_bitmap;        " << "First Block of Free Block Bitmap:      " << super->fb_bitmap << endl;
    cout << "uint32_t fb_bitmapblocks;  " << "Number of Blocks in Free Block Bitmap: " << super->fb_bitmapblocks << endl;
    cout << "uint32_t blocks_free;      " << "Number of Unused Blocks:               " << super->blocks_free << endl;
    cout << "uint32_t num_inodes;       " << "Total Number of Inodes:                " << super->num_inodes << endl;
    cout << "uint32_t fi_bitmap;        " << "First Block of Free Inode Bitmap:      " << super->fi_bitmap << endl;
    cout << "uint32_t fi_bitmapblocks;  " << "Number of Blocks in Free Inode Bitmap: " << super->fi_bitmapblocks << endl;
    cout << "uint32_t inodes_free;      " << "Number of Unused Inodes:               " << super->inodes_free << endl;
    cout << "uint32_t num_inode_blocks; " << "Number of Blocks in the Inode Table:   " << super->num_inode_blocks << endl;
    cout << "uint32_t inodes;           " << "First Block of the Inode Table:        " << super->inodes << endl;
    cout << "uint32_t rootdir;          " << "First Block of the Root Directory:     " << super->rootdir << endl;
    cout << "uint32_t open_count;       " << "Open Files Count:                      " << super->open_count << endl;
    cout << left << setw(100) << setfill('=') << "" << endl;
}

void Disk::print_inode(sfs_inode *inode)
{
    cout << left << setw(100) << setfill('=') << "========INODE=INFO" << endl;
    cout << "uint32_t owner;            " << "Owner:                                " << inode->owner << endl;
    cout << "uint32_t group;            " << "Group:                                " << inode->group << endl;
    cout << "uint32_t ctime;            " << "Creation Time:                        " << inode->ctime << endl;
    cout << "uint32_t mtime;            " << "Modification Time:                    " << inode->mtime << endl;
    cout << "uint32_t atime;            " << "Access Time:                          " << inode->atime << endl;
    cout << "uint16_t perm;             " << "Permissions:                          " << inode->perm << endl;
    cout << "uint8_t type;              " << "Type:                                 " << (int)inode->type << endl;
    cout << "uint8_t refcount;          " << "Reference Count:                      " << (int)inode->refcount << endl;
    cout << "uint64_t size;             " << "Size:                                 " << inode->size << endl;
    cout << "uint32_t direct[5];        " << "Direct Block Pointers:                ";
    for (int i = 0; i < NUM_DIRECT; i++)
    {
        cout << inode->direct[i] << " ";
    }
    cout << endl;
    cout << "uint32_t indirect;         " << "Indirect Block Pointer:               " << inode->indirect << endl;
    cout << "uint32_t dindirect;        " << "Double Indirect Block Pointer:        " << inode->dindirect << endl;
    cout << "uint32_t tindirect;        " << "Triple Indirect Block Pointer:        " << inode->tindirect << endl;
    cout << left << setw(100) << setfill('=') << "" << endl;
}

void Disk::print_file_info()
{
    sfs_inode *inodes = (sfs_inode *)malloc(2 * sizeof(sfs_inode));
    for (int i = 0; i < (int)filedata.size(); i++)
    {
        int idx = 0;

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
    if (inodes != NULL)
    {
        free(inodes);
    }
}

void FileData::print_ls_al_info()
{
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

    cout << perm_str;

    cout << right << setw(2) << to_string(refcount) << " ";

    cout << right << setw(6) << to_string(owner) << " ";

    cout << right << setw(6) << to_string(group) << " ";

    cout << right << setw(7) << to_string(size) << " ";

    cout << date_from_time(ctime) << " ";

    cout << name << endl;
}

uint16_t Disk::formatPermissions(mode_t mode)
{
    uint16_t formattedPerms = 0;

    // Extract permissions for owner, group, and others
    uint16_t ownerPerms = (mode & S_IRWXU) >> 6; // Shift user bits to position 8-6
    uint16_t groupPerms = (mode & S_IRWXG) >> 3; // Shift group bits to position 5-3
    uint16_t otherPerms = (mode & S_IRWXO);      // Other bits are already in position 2-0

    // Assemble the formatted permissions
    formattedPerms |= ownerPerms << 6;
    formattedPerms |= groupPerms << 3;
    formattedPerms |= otherPerms;

    return formattedPerms;
}

string FileData::date_from_time(uint32_t time)
{
    // Convert uint32_t time to time_t
    time_t raw_time = time;
    // use localtime
    struct tm *timeinfo = localtime(&raw_time);

    // Uncomment for UTC
    // struct tm *timeinfo = gmtime(&raw_time);

    // Buffer to store the formatted date and time
    char buffer[80];

    // Format the date and time: "Mon 2 Jan 2006 15:04"
    strftime(buffer, sizeof(buffer), "%b %d %H:%M %Y", timeinfo);

    // Return the formatted string
    return string(buffer);
}

int Disk::get_free_inode()
{
    bitmap_t *bitmaps = (bitmap_t *)malloc(block_size);
    for (int i = 0; i < (int)super->fi_bitmapblocks; i++)
    {
        driver_read(bitmaps, super->fi_bitmap + i);
        for (int j = 0; j < 8; j++)
        {
            bitmap_t bitmap = bitmaps[j];
            for (int k = 0; k < 32; k++)
            {
                uint8_t bit = get_bit(&bitmap, (uint32_t)k);
                if (bit == 0)
                {
                    set_bit(&bitmaps[j], (uint32_t)k);
                    driver_write(bitmaps, super->fi_bitmap + i);
                    if (bitmaps != NULL)
                    {
                        free(bitmaps);
                    }
                    return i * 1024 + j * 32 + k;
                }
            }
        }
    }
    if (bitmaps != NULL)
    {
        free(bitmaps);
    }
    return -1;
}

void Disk::update_free_blocks_list(vector<int> used_blocks)
{
    int block_idx = 0;
    bitmap_t *bitmap = (bitmap_t *)malloc(block_size);
    for (int i = 0; i < (int)super->fb_bitmapblocks; i++)
    {
        driver_read(bitmap, super->fb_bitmap + i);
        for (int j = 0; j < 8; j++)
        {
            for (int k = 0; k < 32; k++)
            {
                if (block_idx == (int)used_blocks.size())
                {
                    if (bitmap != NULL)
                    {
                        free(bitmap);
                    }
                    return; // All used blocks have been updated
                }
                else if (used_blocks[block_idx] == i * 1024 + j * 32 + k)
                {
                    set_bit(&bitmap[j], (uint32_t)k);
                    driver_write(bitmap, super->fb_bitmap + i);
                    block_idx++;
                }
            }
        }
    }

    if (bitmap != NULL)
    {
        free(bitmap);
    }
}

vector<int> Disk::get_free_blocks_list()
{
    vector<int> free_blocks;
    bitmap_t *bitmap = (bitmap_t *)malloc(block_size);
    for (int i = 0; i < (int)super->fb_bitmapblocks; i++)
    {
        driver_read(bitmap, super->fb_bitmap + i);
        for (int j = 0; j < 8; j++)
        {
            for (int k = 0; k < 32; k++)
            {
                uint8_t bit = get_bit(&bitmap[j], (uint32_t)k);
                if (bit == 0)
                {
                    free_blocks.push_back(i * 1024 + j * 32 + k);
                }
            }
        }
    }
    free_blocks.pop_back();
    if (bitmap != NULL)
    {
        free(bitmap);
    }
    return free_blocks;
}

void Disk::fix_disk(bool verbose)
{
    bool fixed = false;

    uint32_t num_free_blocks = get_num_free_blocks();
    uint32_t num_free_inodes = get_num_free_inodes();

    if (!verbose)
    {
        super->rootdir = super->inodes;
        super->blocks_free = num_free_blocks;
        super->inodes_free = num_free_inodes;
        driver_write(super, superblock_num);
        return;
    }

    if (super->inodes != super->rootdir)
    {
        cout << "Root directory pointer is incorrect" << endl;
        cout << "Correcting the root directory inode number" << endl;
        super->rootdir = super->inodes;
        fixed = true;
    }

    if (num_free_blocks != super->blocks_free)
    {
        cout << "Number of free blocks in superblock is incorrect" << endl;
        cout << "Correcting the number of free blocks in superblock" << endl;
        super->blocks_free = num_free_blocks;
        fixed = true;
    }
    if (num_free_inodes != super->inodes_free)
    {
        cout << "Number of free inodes in superblock is incorrect" << endl;
        cout << "Correcting the number of free inodes in superblock" << endl;
        super->inodes_free = num_free_inodes;
        fixed = true;
    }

    if (fixed)
    {
        cout << "Writing the corrected superblock to the disk image" << endl;
        driver_write(super, superblock_num);
    }
    else
    {
        cout << "Superblock is correct, no fix needed: exiting..." << endl;
        return;
    }

    driver_write(super, superblock_num);
    cout << "Disk image fixed successfully: exiting..." << endl;
}

void Disk::print_inode_bitmap()
{
    cout << left << setw(40) << setfill('=') << "========INODE=BITMAP" << endl;
    bitmap_t *bitmaps = (bitmap_t *)malloc(block_size);
    for (int i = 0; i < (int)super->fi_bitmapblocks; i++)
    {
        driver_read(bitmaps, super->fi_bitmap + i);
        for (int j = 0; j < 8; j++)
        {
            string bitmap_str = "";
            bitmap_t bitmap = bitmaps[j];
            for (int k = 0; k < 32; k++)
            {
                uint8_t bit = get_bit(&bitmap, (uint32_t)k);
                bitmap_str += to_string(bit);
            }
            cout << bitmap_str << endl;
        }
        cout << left << setw(32) << setfill('=') << "" << endl;
    }
    cout << left << setw(40) << setfill('=') << "" << endl;
    if (bitmaps != NULL)
    {
        free(bitmaps);
    }
}

void Disk::print_block_bitmap()
{
    cout << left << setw(40) << setfill('=') << "========BLOCK=BITMAP" << endl;
    bitmap_t *bitmaps = (bitmap_t *)malloc(block_size);
    for (int i = 0; i < (int)super->fb_bitmapblocks; i++)
    {
        driver_read(bitmaps, super->fb_bitmap + i);
        for (int j = 0; j < 8; j++)
        {
            string bitmap_str = "";
            bitmap_t bitmap = bitmaps[j];
            for (int k = 0; k < 32; k++)
            {
                uint8_t bit = get_bit(&bitmap, (uint32_t)k);
                bitmap_str += to_string(bit);
            }
            cout << bitmap_str << endl;
        }
        cout << left << setw(32) << setfill('=') << "" << endl;
    }
    cout << left << setw(40) << setfill('=') << "" << endl;
    if (bitmaps != NULL)
    {
        free(bitmaps);
    }
}

uint32_t Disk::get_num_free_blocks()
{
    uint32_t num_free_blocks = 0;
    bitmap_t *bitmap = (bitmap_t *)malloc(block_size);
    for (int i = 0; i < (int)super->fb_bitmapblocks; i++)
    {
        driver_read(bitmap, super->fb_bitmap + i);
        for (int j = 0; j < 8; j++)
        {
            for (int k = 0; k < 32; k++)
            {
                uint8_t bit = get_bit(&bitmap[j], k);
                if (bit == 0)
                {
                    num_free_blocks++;
                }
            }
        }
    }
    if (num_free_blocks > 0) // Decrement the number of free blocks by 1 to account the extra bit
    {
        num_free_blocks--;
    }
    if (bitmap != NULL)
    {
        free(bitmap);
    }
    return num_free_blocks;
}

uint32_t Disk::get_num_free_inodes()
{
    uint32_t num_free_inodes = 0;
    bitmap_t *bitmaps = (bitmap_t *)malloc(block_size);
    for (int i = 0; i < (int)super->fi_bitmapblocks; i++)
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
                    num_free_inodes++;
                }
            }
        }
    }
    if (num_free_inodes > 0) // Decrement the number of free inodes by 1 to account the extra bit
    {
        num_free_inodes--;
    }
    if (bitmaps != NULL)
    {
        free(bitmaps);
    }
    return num_free_inodes;
}

void Disk::print_disk_info()
{
    print_superblock();
    print_inode(rootnode);
    print_block_bitmap();
    print_inode_bitmap();
}