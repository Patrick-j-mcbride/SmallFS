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
#include <helpers.h>

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

vector<ls_data> get_files_in_root(sfs_inode *rootnode, bool l)
{
    vector<ls_data> files;
    int files_in_root = rootnode->size / sizeof(sfs_dirent);
    int ct = 0;
    sfs_dirent *dirents = (sfs_dirent *)malloc(sizeof(sfs_dirent) * 4);
    sfs_dirent *dirent;
    for (int i = 0; i < 5; i++)
    {
        driver_read(dirents, rootnode->direct[i]);
        for (int i = 0; i < 4; i++)
        {
            ls_data data;
            dirent = &dirents[i];
            if (!l)
            {
                printf("%s\n", dirent->name);
            }
            data.name = dirent->name;
            data.inode = dirent->inode;
            files.push_back(data);
            ct++;
            if (ct == files_in_root)
            {
                return files;
            }
        }
    }
    return files;
    free(dirents);
}

string get_type(int type)
{
    if (type == 0)
    {
        return "f";
    }
    else if (type == 1)
    {
        return "d";
    }
    else
    {
        return "u";
    }
}

void ls(char *diskname, bool l)
{
    char raw_superblock[128];
    sfs_superblock *super = (sfs_superblock *)raw_superblock;

    driver_attach_disk_image(diskname, 128);
    get_superblock(super);
    // get the first 2 inodes
    sfs_inode *root_inodes = (sfs_inode *)malloc(2 * sizeof(sfs_inode));
    // get the root node
    sfs_inode *rootnode = &root_inodes[0];
    // read the nodes
    driver_read(root_inodes, super->inodes);

    // get the files in the root
    vector<ls_data> files = get_files_in_root(rootnode, l);

    if (l)
    {
        sfs_inode *inodes = (sfs_inode *)malloc(2 * sizeof(sfs_inode));
        printf("total %d\n", files.size());
        for (int i = 0; i < files.size(); i++)
        {
            sfs_inode *node = &inodes[0];
            driver_read(node, super->inodes + files[i].inode);
            string type = get_type(node->type);
            cout << type << files[i].name << " " << node->perm << endl;
        }
    }

    driver_detach_disk_image();
    free(root_inodes);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    { // No arguments
        printf("Usage: <diskname> <-l>(optional)");
        return 1;
    }
    else if (argc == 2)
    { // No -l
        char *diskname = argv[1];
        ls(diskname, false);
    }
    else if (argc == 3)
    { // -l
        char *diskname = argv[1];
        if (strcmp(argv[2], "-l") != 0)
        {
            printf("Usage: <diskname> <-l>(optional)");
            return 1;
        }
        ls(diskname, true);
    }
    else
    {
        printf("Usage: <diskname> <-l>(optional)");
        return 1;
    }
}