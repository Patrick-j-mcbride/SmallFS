#include <helpers.h>

void get_superblock(sfs_superblock *super)
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
