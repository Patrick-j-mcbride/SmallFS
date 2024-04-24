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

void ls(char *diskname, bool l)
{
    Disk disk = Disk(diskname, 128);
    disk.get_files_in_root();
    if (!l)
    {
        disk.print_filenames();
        return;
    }
    else if (l)
    {
        disk.print_file_info();
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    { // No arguments
        cout << "Usage: <diskname> <-l>(optional)" << endl;
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
            cout << "Usage: <diskname> <-l>(optional)" << endl;
            return 1;
        }
        ls(diskname, true);
    }
    else
    {
        cout << "Usage: <diskname> <-l>(optional)" << endl;
        return 1;
    }
}