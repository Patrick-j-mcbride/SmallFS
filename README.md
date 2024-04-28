# SmallFS

**Author:** Patrick McBride

**Co-Author:** Larry Pyeatt

**Description:**

SmallFS is a suite of tools designed for interacting with a small file system stored entirely within a single file. This project provides a simple way to manage files on a small-scale file system through a set of command-line utilities.

This project includes all extra credit features: fix the file system and copy files to the disk.

## Building the Project

The project utilizes a Makefile that compiles the source with the `-Wall` flag to ensure all warnings are shown. To build the project, simply use the provided Makefile by running the following command:

```
make
```

This command builds four executables: `dils`, `dicpo`, `dicpi`, and `fsck`.

## Usage

### dils

**Usage:** `<diskname> <-l> (optional)`

Lists files in the file system on the specified disk. If the `-l` flag is provided, it displays all file information. Without the `-l` flag, only file names are shown.

### dicpo

**Usage:** `<diskname> <filename>`

Copies a file from the specified disk to the current directory.

### dicpi

**Usage:** `<diskname> <filename>`

Copies a file from the current directory to the specified disk. It also automatically fixes any file system errors using the `fsck` utility before copying. Note: the command will fail if the file is too large for the disk.

### fsck

**Usage:** `<diskname>`

This is only provided as a standalone utility for testing purposes as it is called internally by `dicpi`. However, it can be run separately to check the file system on the specified disk for any errors and correct them if necessary. Running `fsck` as a standalone utility will tell you what errors were found and what was fixed.

Checks the file system on the specified disk for any errors and corrects them if necessary. This utility is used internally by `dicpi` and can also be run separately for testing purposes.

### Credit

The following files were written by Larry Pyeatt:

`bitmaps.c`
`bitmaps.h`
`driver.c`
`driver.h`
`sfs_dir.h`
`sfs_inode.h`
`sfs_superblock.h`
