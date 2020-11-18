#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __CHUNK_H__
#define __CHUNK_H__
struct chunk {
	int context; /* linenumber */
	int offset;
	int length;
};
#endif

#define OK 0

#define ECHUNK_CONTEXT -1
#define ECHUNK_CONTEXT_DEAD -2

/* use these */
int chunk_context_change(char** content, int len, struct chunk** context, int clen, struct chunk chunk, char* text);
void chunk_context_insert(char** content, struct chunk** context, struct chunk chunk, int where);
int chunk_context_delete(char** content, int len, struct chunk** context, int clen, struct chunk chunk);

void chunk_context_read(char** content, struct chunk** context, struct chunk chunk, char** s);

/* DONT USE THESE, they dont update the chunk stack and will break your text */
/* change a single chunk, returns the new chunk */
struct chunk chunk_change(char** content, int len, struct chunk chunk, char* text);
void chunk_delete(char** content, int len, struct chunk chunk);
