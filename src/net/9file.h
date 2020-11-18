#include <stdint.h>

#include "9map.h"

typedef struct msg9pQid {
	uint8_t type;
	uint32_t version;
	uint64_t path;
} msg9pQid;

typedef struct msg9pFile {
	uint8_t* name;
	uint32_t ncontent;
	msg9pQid* content;
	uint8_t* strcontent;
	void* (*callback) (uint32_t nargs, ...);
} msg9pFile;

typedef struct msg9pString {
	uint16_t length;
	uint8_t* string;
} msg9pString;

#define REWRITE (1<<0)
#define CREATE (1<<1)
#define TYPE_DIR (1<<2)
#define TYPE_FILE (1<<3)

#define IS_ROOT (-1)

/* mode :
	- REWRITE = VERSION:+1
	- CREATE  = VERSION:1, PATH:+1
	- TYPE_DIR  = TYPE:0
	- TYPE_FILE =  TYPE:1
*/
uint32_t msg9pManageQid(struct msg9pQid* qid, uint8_t mode);
int32_t msg9pFilePathResolve(msg9pString** rs, uint8_t* path);

void msg9pQidToStr(uint8_t* str, struct msg9pQid qid);
void msg9pFilePrint(struct msg9pFile* f);

msg9pQid msg9pFileFindPath(struct mapMap* map, msg9pQid pwd, uint8_t* path);
msg9pQid msg9pFileCreatePath(struct mapMap* map, msg9pQid pwd, uint8_t* path, uint32_t type);
msg9pQid msg9pFileCreateRoot(struct mapMap* map);
msg9pQid msg9pFileDeletePath(struct mapMap* map, msg9pQid pwd, uint8_t* path);
msg9pQid msg9pFileTouchPath(struct mapMap* map, msg9pQid pwd, uint8_t* path);
/*
void createFile(uint8_t* qid);
*/
