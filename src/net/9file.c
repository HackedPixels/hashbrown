#include "9file.h"

uint32_t
msg9pManageQid(struct msg9pQid* qid, uint8_t mode)
{
	static uint64_t path = 1;


	if (mode & REWRITE) {
		qid->version += 1;
		if (mode & TYPE_DIR || mode & TYPE_FILE)
		{
			qid->type = (mode & TYPE_DIR) ? 1 : 0;
			return 0;
		}
		return 0;
	}
/*	if ((mode & CREATE) && (qid->path == (uint64_t) 0))  { */
	if ((mode & CREATE)) {
		qid->type = (mode & TYPE_DIR) ? 1 : 0;
		qid->version = 0x01;
		path++;
		qid->path = path;

		return 0;
	}
	return 1;
}

msg9pQid
msg9pFileFindPath(struct mapMap* map, msg9pQid pwd, uint8_t* path)
{
	uint8_t qidStr[30];
	uint32_t nrpath;
	struct msg9pString* rpath;
	struct msg9pFile* wfile;
	struct msg9pFile* temp;
	struct msg9pQid qid;

	nrpath = msg9pFilePathResolve(&rpath, path);
	if (nrpath == IS_ROOT) {
		printf("ISROOT\n");
		return pwd;
	}

	/* now get our pwd file */
	msg9pQidToStr(qidStr, pwd);
	wfile = (struct msg9pFile*) mapSearch(map, qidStr);

	/* now we need to walk the path until we arrive at n*/
	for (int i = 0; i < nrpath; i++)
	{
		uint32_t contentMaxIndex;

		for (int cindex = 0; cindex < wfile->ncontent; cindex++)
		{
			qid = wfile->content[cindex];
			msg9pQidToStr(qidStr, wfile->content[cindex]);
			temp = (struct msg9pFile*) mapSearch(map, qidStr);

//			printf(":%d> %s ?= %s\n", cindex, rpath[i].string, temp->name);
			
			if (strncmp(temp->name, rpath[i].string, rpath[i].length) == 0)
			{
//				printf("Jumping to: %s (%s)\n", rpath[i].string, qidStr);
				wfile = temp;
				break;
			} else {
//				printf("Checking next...\n");
				continue;
			}
		}
	}

//	printf("RETURN: %02x %08x %016x\n", qid.type, qid.version, qid.path);
	return qid;
}

msg9pQid
msg9pFileCreateRoot(struct mapMap* map)
{
	struct msg9pFile* file;
	struct msg9pQid qid = {.type=0x01, .version=0x01, .path=0x01};
	uint8_t qidStr[30];
	
	file = malloc(sizeof(struct msg9pFile));

	file->name = strdup("ROOT");
	file->ncontent = 1;
	file->content = malloc(sizeof(msg9pQid));
	file->content[0] = qid;

	msg9pQidToStr(qidStr, qid);
	mapInsert(map, qidStr, file, sizeof(struct msg9pFile));

	return qid;
}

void
msg9pFilePrint(struct msg9pFile* f)
{
	uint8_t qidStr[30];

	msg9pQidToStr(qidStr, f->content[0]);
	printf("\x1b[1mFile:\x1b[m %s\n", f->name);
	printf("\x1b[1m..  :\x1b[m %s\n", qidStr);
	for (int i = 1; i < f->ncontent; i++) {
		msg9pQidToStr(qidStr, f->content[i]);
		printf("\x1b[1mc[%d]:\x1b[m %s\n", i, qidStr);
	}
}

msg9pQid
msg9pFileCreatePath(struct mapMap* map, msg9pQid pwd, uint8_t* path, uint32_t type)
{
	uint8_t qidStr[30];
	int32_t nrpath;            /* length of resolved path */
	struct msg9pString* rpath; /* resolved path */
	struct msg9pQid fqid;      /* qid of new file */
	struct msg9pQid pqid;      /* parent qid */
	struct msg9pFile* wfile;   /* working file, entry file */
	struct msg9pFile* temp;    /* a file to work with, walking content */

	nrpath = msg9pFilePathResolve(&rpath, path);
	if (nrpath == IS_ROOT) {
		printf("ISROOT\n");
		return fqid;
	}

	/* now get our pwd file */
	msg9pQidToStr(qidStr, pwd);
	wfile = (struct msg9pFile*) mapSearch(map, qidStr);
	pqid = pwd;

	/* now we need to walk the path until we arrive at n-1 */
	for (int i = 0; i < nrpath-1; i++)
	{
		for (int cindex = 1; cindex < wfile->ncontent; cindex++)
		{
			msg9pQidToStr(qidStr, wfile->content[cindex]);
			temp = (struct msg9pFile*) mapSearch(map, qidStr);

			if (strncmp(temp->name, rpath[i].string, rpath[i].length) == 0)
			{
//				printf("Jumping to: %s (%s)\n", rpath[i].string, qidStr);
				pqid = wfile->content[cindex];
				wfile = temp;
			}
		}
	}

	/* extend the content */
	wfile->ncontent++;
	wfile->content = realloc(wfile->content, wfile->ncontent * sizeof(struct msg9pQid));

	/* create new Qid */
	msg9pManageQid(&fqid, CREATE | type);
	msg9pQidToStr(qidStr, fqid);
//	printf("NQID: %s\n", qidStr);

	/* create the new file */
	temp = malloc(sizeof(struct msg9pFile));

	temp->name = strndup(rpath[nrpath-1].string, rpath[nrpath-1].length); /* set name to end of path */
//	printf("NAME: %s\n", temp->name);
//	printf("PQID: %02x %08x %016x\n", pqid.type, pqid.version, pqid.path);
	temp->content = malloc(sizeof(struct msg9pQid));
	temp->content[0] = pqid;
	temp->ncontent = 1;

	mapInsert(map, qidStr, temp, sizeof(struct msg9pFile));

	/* assign it */
	wfile->content[wfile->ncontent-1] = fqid;

	//msg9pFilePrint(wfile);



//	printf("\n");

	return fqid;
}

int32_t
msg9pFilePathResolve(msg9pString** rs, uint8_t* path)
{
	uint32_t nelems;
	uint8_t* wpath, *token;
	uint32_t i;

	nelems = 0;
	wpath = strdup(path);

	/* count elements */
	for (i = 0; path[i] != 0; i++) {
		if (path[i] == '/') {
			nelems++;
		}
	}

	*rs = malloc(nelems * sizeof(msg9pString));
	if (strlen(wpath) == 1) {
		(*rs)[i].length = 1;
		(*rs)[i].string = strdup("/");
		return IS_ROOT;
	}

	/* now assign them */
	token = strtok(wpath, "/");
	for (i = 0; i < nelems; i++) {
		(*rs)[i].length = strlen(token);
		(*rs)[i].string = strdup(token);
		token = strtok(NULL, "/");
	}

	return (int32_t) nelems;
}

void
msg9pQidToStr(uint8_t* str, struct msg9pQid qid)
{
	snprintf(str, 29, "%02x %08x %016x", qid.type, qid.version, qid.path);
}
