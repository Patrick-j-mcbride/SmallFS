#include <iostream>
#include <fs.hpp>

using namespace std;

void copy_file_in(char *diskname, char *filename)
{
    Disk disk0 = Disk(diskname, 128);
    disk0.fix_disk(false);
    // call the destructor
    disk0.~Disk();
    Disk disk = Disk(diskname, 128);
    disk.copy_file_in(filename);
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

        copy_file_in(diskname, filename);
    }
    else
    {
        cout << "Usage: <diskname> <filename>" << endl;
        exit(1);
    }
}