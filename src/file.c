#include "file.h"

int file_size(struct file* f) {
	if (f->fp < 0) { return EFILE_FOPEN_DEAD; };

	f->length = lseek(f->fp, 0, SEEK_END);
	lseek(f->fp, 0, SEEK_SET);
/*
	fseek(f->fp, 0, SEEK_END);
	f->length = ftell(f->fp);
	fseek(f->fp, 0, SEEK_SET);
*/

	return OK;
}

int file_load(struct file* f) {
	int err_store;

	err_store = 0;

	/* check if a file is defined to load */
	if (f->name == NULL) { return EFILE_NOT_DEFINED; }
	if ((f->fp = open(f->name, O_RDWR | O_CREAT, (mode_t)0600)) < 0) { return EFILE_FOPEN_DEAD; }

	/* get file size */
	if ((err_store = file_size(f)) != OK) { return err_store; }
//	if ((f->content = malloc(f->length)) == NULL) { return EFILE_MALLOC_DEAD; }
	f->content = mmap(0, FILE_ACCESSWINDOW_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, f->fp, 0);

	return OK;
}

int
file_load_x(struct file* f, int extend)
{
	char* nmmap; /* new memory map */
	/* TODO: make a const for extending */
	/* TODO: make error code for ftruncate */
	if (ftruncate(f->fp, f->length + FILE_ACCESSWINDOW_SIZE) != 0) { return EFILE_TRUNC_FAIL; }
	nmmap = mmap(0, FILE_ACCESSWINDOW_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, f->fp, 0);
	if (nmmap == MAP_FAILED) { return EFILE_MMAP_FAIL; }
	munmap(f->content, FILE_ACCESSWINDOW_SIZE);
	f->content = nmmap;

	return OK;
}

int file_unload(struct file* f) {
	munmap(f->content, f->length);
	close(f->fp);

	return OK;
}

int file_chunk(struct file* f) {
	char* s; /* use this to walk the file */
	char* p; /* this is to remember the start of line */
	uint32_t nchunks; /* current number of chunks */
	struct chunk* chunks; /* use this for realloc */

	nchunks = 0;
	s = f->content;
	p = s;

	f->chunks = NULL;
	while (*(s) != 0)
	{
		if (*s == '\n')
		{
			f->chunks = realloc(f->chunks, (nchunks + 1) * sizeof(struct chunk));

			f->chunks[nchunks].context = nchunks;
			f->chunks[nchunks].offset = p - f->content;
			f->chunks[nchunks].length = (s+1) - p;

			nchunks++;
			p = (s+1);
		}
		s++;
	}

	f->nchunks = (nchunks - 1);

	return OK;
}
