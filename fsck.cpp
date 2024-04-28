#include <iostream>
#include <fs.hpp>

using namespace std;

void fix(char *diskname)
{
    Disk disk = Disk(diskname, 128);
    disk.fix_disk(true);
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