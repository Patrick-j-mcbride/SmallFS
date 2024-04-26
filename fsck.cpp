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

void fix(char *diskname)
{
    Disk disk = Disk(diskname, 128);
    // disk.fix_disk();
}

int main(int argc, char **argv)
{
    if (argc < 2)
    { // No arguments
        cout << "Usage: <diskname>" << endl;
        return 1;
    }
    else if (argc == 2)
    {
        char *diskname = argv[1];

        fix(diskname);
    }
    else
    {
        cout << "Usage: <diskname>" << endl;
        return 1;
    }
}