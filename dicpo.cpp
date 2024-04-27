#include <stdio.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <fs.hpp>

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

void copy_file_out(char *diskname, char *filename)
{
    Disk disk = Disk(diskname, 128);
    disk.get_files_in_root();
    if (disk.file_exists(filename))
    {
        disk.copy_file_out(filename);
    }
    else
    {
        cout << "File does not exist" << endl;
        exit(1);
    }
}

int main(int argc, char **argv)
{
    if (argc < 3)
    { // No arguments
        cout << "Usage: <diskname> <filename>" << endl;
        exit(1);
    }
    else if (argc == 3)
    {
        char *diskname = argv[1];
        char *filename = argv[2];

        copy_file_out(diskname, filename);
    }
    else
    {
        cout << "Usage: <diskname> <filename>" << endl;
        exit(1);
    }
}