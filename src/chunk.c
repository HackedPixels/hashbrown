#include "chunk.h"

/*
void chunk_context_insert(char** content, struct chunk** context, struct chunk chunk, int where);
*/

int
chunk_context_change(char** content, int len, struct chunk** context, int clen, struct chunk chunk, char* text)
{
	struct chunk ochunk; /* origin-chunk, we need to update from this */
	int doffset; /* delta offset */

	doffset = 0;
	/* check if the chunk matches the context */
	if ((*context)[chunk.context].offset != chunk.offset) { return ECHUNK_CONTEXT; }

	ochunk = chunk_change(content, len, chunk, text);
	ochunk.context = chunk.context;

	doffset = chunk.length - ochunk.length;

	(*context)[ochunk.context] = ochunk;
	for (int i = (ochunk.context+1); i < clen; i++)	{
		(*context)[i].offset -= doffset;
	}

	return OK;
}

struct chunk
chunk_change(char** content, int len, struct chunk chunk, char* text)
{
	struct chunk nchunk;
	int slen;

	slen = strlen(text);
	nchunk.offset = chunk.offset;
	nchunk.length = slen;
	nchunk.context = 0; /* we have no idea about the context */

	memmove(
		(*content) + chunk.offset + slen,
		(*content) + chunk.offset + chunk.length,
		len - (chunk.offset) - (chunk.length - slen)
	);
	memcpy(
		(*content) + chunk.offset,
		text,
		slen
	);

	return nchunk;
}

int
chunk_context_delete(char** content, int len, struct chunk** context, int clen, struct chunk chunk)
{
	struct chunk* tcontext; /* Temporary context for realloc */
	struct chunk dchunk; /* we need to delete this chunk later */
	int doffset; /* delta offset */

	doffset = 0;
	/* check if the chunk matches the context */
	if ((*context)[chunk.context].offset != chunk.offset) { return ECHUNK_CONTEXT; }

	dchunk = chunk_change(content, len, chunk, "");
	dchunk.context = chunk.context;

	doffset = chunk.length - dchunk.length;

	memmove(
		(*context) + dchunk.context,
		(*context) + (dchunk.context + 1),
		(clen - dchunk.offset - 1) * sizeof(struct chunk)
	);

	*context = realloc(*context, (clen - 1) * sizeof(struct chunk));
//	if (tcontext == NULL) { printf("ECHUNK_CONTEXT_DEAD\n"); return ECHUNK_CONTEXT_DEAD;  }

	for (int i = (dchunk.context); i < clen; i++)	{
		(*context)[i].offset -= doffset;
		(*context)[i].context -= 1;
	}

	return OK;
}

void
chunk_delete(char** content, int len, struct chunk chunk)
{
	struct chunk nchunk;
	nchunk = chunk_change(content, len, chunk, "");
}

/* Read the string starting at <linenum> with <offset> until <length> */
void
chunk_context_read(char** content, struct chunk** context, struct chunk chunk, char** s)
{
	memcpy(*s,
		(*content) + ((*context)[chunk.context].offset + chunk.offset),
		chunk.length == 0 ? (*context)[chunk.context].length : chunk.length
	);
	(*s)[chunk.length == 0 ? (*context)[chunk.context].length : chunk.length] = 0;
}
