#ifndef KERNEL_STAT_H
#define KERNEL_STAT_H

#include "types.h"

#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

struct stat {
	short type;  // Type of file
	int dev;     // File system's disk device
	uint ino;    // Inode number
	short nlink; // Number of links to file
	uint size;   // Size of file in bytes
};

// struct fileblks {
// 	int blocks[12 + 128];    // NDIRECT + NINDIRECT
// 	int num_blocks;        // Ukupan broj zauzetih blokova
// 	int last_block_free;  // Preostalo bajtova u poslednjem bloku
// };

#endif // KERNEL_STAT_H
