#ifndef HELPERS_H
#define HELPERS_H

#include <driver.h>
#include <sfs_superblock.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void get_superblock(sfs_superblock *super);

#endif