#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "chunk.h"

#define FILE_ACCESSWINDOW_SIZE 128

#define OK 0

#define EFILE_NOT_DEFINED -1
#define EFILE_FOPEN_DEAD  -2
#define EFILE_MALLOC_DEAD -3
#define EFILE_TRUNC_FAIL  -4
#define EFILE_MMAP_FAIL   -5

struct file {
	char* name;
	uint64_t length; // how big the file is
	char* content; // the loaded file (mmap :))

	uint64_t nchunks; /* number of chunks in file */
	struct chunk* chunks; /* the file in chunks. */

	int fp; // file pointer for future use?
};

int file_size(struct file* f); /* grab the size of the file */
int file_chunk(struct file* f); /* chunk file into smaller bits (make it editable) */
int file_load(struct file* f); /* load filename to content */
int file_load_x(struct file* f, int extend); /* extend file size and reload memory map */
int file_unload(struct file* f); /* unmap file and close file pointer */
