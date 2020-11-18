#include "main.h"
#include "net/9p.h"
#include <stdarg.h>

struct msg9pState {
	uint32_t msize;
	uint8_t* version;
	struct mapMap* map;
	struct msg9pQid root;

	uint32_t connid; /* current connection */
};

struct msg9pFidMap {
	uint8_t free;
	uint32_t fid;
	uint32_t nfid;
	union {
		struct msg9pQid qid;
		struct msg9pFidMap* map;
	};
};

/* temp struct, only needed for initialization */
struct msg9pFileInit {
	uint8_t* filename;
	uint8_t* ftype;
	void* (*callback) (uint32_t nargs, ...);
};

#define LOGSTACK_SIZE 8
#define LOG_MAXMESSAGE_SIZE 128
uint8_t* logStack[LOGSTACK_SIZE];
/* TODO: make this prettier */
int32_t msg9p_fidMap_newConnection(struct msg9pFidMap** map);
void msg9plog(uint8_t* origin, uint8_t* fmt, ...);

struct msg9p server_handle_version(struct msg9p msg, struct msg9pState* state);
struct msg9p server_handle_attach(struct msg9p msg, struct msg9pState* state);
struct msg9p server_handle_walk(struct msg9p msg, struct msg9pState* state);
struct msg9p server_handle_open(struct msg9p msg, struct msg9pState* state);
struct msg9p server_handle_create(struct msg9p msg, struct msg9pState* state);
struct msg9p server_handle_read(struct msg9p msg, struct msg9pState* state);
struct msg9p server_error(uint8_t* errtype, uint8_t* errmsg);

struct msg9p error_server_not_client(struct msg9p msg, struct msg9pState* state);
struct msg9p error_not_implemented(struct msg9p msg, struct msg9pState* state);

#define NCONNECTIONS 3
#define NMAXFILES 10
struct msg9pFidMap* fidMap;

#define NMAP 17
struct msg9p (*msgMap[NMAP]) (struct msg9p msg, struct msg9pState* state) = {
	[TVERSION] = server_handle_version,
	[RVERSION] = error_server_not_client,

	[TAUTH]    = error_not_implemented,
	[RAUTH]    = error_server_not_client,

	[RERROR]   = error_server_not_client,

	[TFLUSH]   = error_not_implemented,
	[RFLUSH]   = error_not_implemented,

	[TATTACH]  = server_handle_attach,
	[RATTACH]  = error_server_not_client,

	[TWALK]  = server_handle_walk,
	[RWALK]  = error_server_not_client,

	[TOPEN]  = server_handle_open,
	[ROPEN]  = error_server_not_client,

	[TCREATE] = server_handle_create,
	[RCREATE] = error_server_not_client,

	[TREAD] = server_handle_read,
	[RREAD] = error_server_not_client
};

#define DIR "DIR"
#define FILE "FILE"
#define READ "READ"

#define NFILES 7
struct msg9pFileInit filemapInit[] = {
	{"/chunk", DIR, NULL},
	{"/chunk/next", FILE, NULL},
	{"/chunk/prev", FILE, NULL},
	{"/chunk/current", FILE, NULL},
	{"/chunk/mode", FILE, NULL},
	{"/files", DIR, NULL},
	{"/chunk/num", FILE, NULL}
};

#define NMESSAGES 7
struct msg9p messages[] = {
	{.type=TVERSION, .msize=127, .version={.length=8, .string="9p2000.H"}},
	{.type=TAUTH, .afid=0x03, .uname={.length=3, .string="jan"}, .aname={.length=12, .string="HackedPixels"}},
	{.type=TATTACH, .fid=0x02, .afid=0x03, .uname={.length=3, .string="jan"}, .aname={.length=12, .string="HackedPixels"}},
	{.type=TWALK, .fid=0x02, .newfid=0x03, .nwname=1},
	{.type=TCREATE, .fid=0x03, .name={.length=4, .string="test"}, .perm=TYPE_DIR, .mode=0x00},
	{.type=TWALK, .fid=0x02, .newfid=0x04, .nwname=1},
	{.type=TREAD, .fid=0x03, .offset=0x00, .count=0x07}
};

int main(int argc, char** argv) {
	struct mapMap* map;      /* File storage */
	struct msg9pQid rootQid; /* Qid of filesystem root */
	struct msg9pQid newQid;  /* Qid of newly created files (for logging) */
	uint8_t qidStr[30];      /* Buffer for Qid Strings (for logging) */
	uint32_t fmode;
	struct msg9pFile* file;
	struct msg9pState globalState;

	messages[3].wname = malloc(sizeof(msg9pString));
	messages[3].wname[0].length = 6;
	messages[3].wname[0].string = strdup("/chunk");

	messages[5].wname = malloc(sizeof(msg9pString));
	messages[5].wname[0].length = 11;
	messages[5].wname[0].string = strdup("/test");

	printf("\x1b[1;35m*\x1b[m\x1b[1m Creating file system... \x1b[m\n");
	globalState.map = mapCreate(32);
	printf("\t\x1b[36;1m>\x1b[m \x1b[33m%s\x1b[m %d\n",
		"Map size:", 32);
	globalState.root = msg9pFileCreateRoot(globalState.map);
	msg9pQidToStr(qidStr, globalState.root);
	printf("\t\x1b[36;1m>\x1b[m \x1b[33m%-20s\x1b[m (%s)\n",
		"ROOT is ", qidStr);

	printf("\x1b[1;35m*\x1b[m\x1b[1m Populating file system... \x1b[m\n");
	for (int findex = 0; findex < NFILES; findex++)
	{
		if (strcmp(filemapInit[findex].ftype, DIR) == 0) fmode = TYPE_DIR;
		if (strcmp(filemapInit[findex].ftype, FILE) == 0) fmode = TYPE_FILE;

		newQid = msg9pFileCreatePath(globalState.map, globalState.root, filemapInit[findex].filename, fmode);
		msg9pQidToStr(qidStr, newQid);
		printf("\t\x1b[36;1m>\x1b[m \x1b[33m%-20s\x1b[m (%s)\n",
			filemapInit[findex].filename, qidStr);

		/* add a callback */
		file = mapSearch(globalState.map, qidStr);
		file->callback = filemapInit[findex].callback;
	}

	printf("\x1b[1;35m*\x1b[m\x1b[1m Starting network server... \x1b[m\n");
	fidMap = malloc(NCONNECTIONS * sizeof(struct msg9pFidMap));

	/* initialize log */
	for (int lindex = 0; lindex < LOGSTACK_SIZE; lindex++)
	{
		logStack[lindex] = NULL;
	}

	/* initialize connections */
	for (int cindex = 0; cindex < NCONNECTIONS; cindex++)
	{
		fidMap[cindex].free = 1;
	}

	struct msg9p cachedMsg;
	for (int mindex = 0; mindex < NMESSAGES; mindex++)
	{
		printf("\t\x1b[36;1m>\x1b[m \x1b[33m%-20s\x1b[m ",
			"reading:");
		msg9p_printM(messages[mindex]);
		/* incoming messages */
		cachedMsg = msgMap[messages[mindex].type] (messages[mindex], &globalState);

		/* process log */
		for (int lindex = 0; lindex < LOGSTACK_SIZE; lindex++)
		{
			if (logStack[lindex] == NULL) { break; }
			printf("\t| \x1b[2m%s\x1b[m\n", logStack[lindex]);
			free(logStack[lindex]);
			logStack[lindex] = NULL;
		}

		/* outgoing messages */
		printf("\t\x1b[36;1m>\x1b[m \x1b[33m%-20s\x1b[m ",
			"writing:");
		msg9p_printM(cachedMsg);
	}

	msg9pFilePrint((struct msg9pFile*) mapSearch(globalState.map, "01 00000001 0000000000000002"));
}

void
msg9plog(uint8_t* origin, uint8_t* fmt, ...)
{
	static uint32_t logCount = 0;
	uint8_t* message;
	uint32_t messagelen;
	va_list argp;
	va_start(argp, fmt);

	/* Spool log */
	if (logStack[0] == NULL)
	{
		logCount = 0;
	}

	/* TODO: add error checking */
	message = malloc(LOG_MAXMESSAGE_SIZE);
	messagelen = vsnprintf(message, LOG_MAXMESSAGE_SIZE, fmt, argp);

	logStack[logCount] = message;
	if (logCount < LOGSTACK_SIZE)
	{
		logCount++;
	}
	
	va_end(argp);
}

#define SERVER_MAX_BUFFER_SIZE 128
#define SERVER_VERSION "9p2000.H"
struct msg9p
server_handle_version(struct msg9p msg, struct msg9pState* state)
{
	struct msg9p rmsg;

	if (SERVER_MAX_BUFFER_SIZE < msg.msize)
	{
		msg9plog("server_handle_version", "msg.msize: %d", msg.msize);
		return server_error("EMSIZETOOBIG", "requesting bigger buffer than supported");
	}
	
	if (strncmp(SERVER_VERSION, msg.version.string, msg.version.length) != 0)
	{
		msg9plog("server_handle_version", "Client requested unsupported protocol: %.*s",
			msg.version.length, msg.version.string);
		return server_error("EVERSION", "unsupported protocol version");
	}

	state->msize = msg.msize;
	state->version = strdup(msg.version.string);

	return rmsg;
}

int32_t
msg9p_fidMap_newConnection(struct msg9pFidMap** map)
{
	uint32_t connid;
	struct msg9pFidMap* fmap;

	fmap = *map;
	
	for (connid = 0; connid < NCONNECTIONS; connid++)
	{
		if (fmap[connid].free) { break; }
	}
	if (!fmap[connid].free) {
		msg9plog("msg9p_fidMap_newConnection", "Connection stack full");
		return -1;
	}
	
	fmap[connid].free = 0;
	fmap[connid].map = malloc(NMAXFILES * sizeof(struct msg9pFidMap));
	for (int i = 0; i < NMAXFILES; i++)
	{
		fmap[connid].map[i].free = 1;
	}

	return connid;

}

struct msg9p
server_handle_attach(struct msg9p msg, struct msg9pState* state)
{
	struct msg9p rmsg;
	int32_t connid;

	rmsg.type = RATTACH;

	msg9plog("server_handle_attach", "Attaching: %.*s (%.*s)",
		msg.uname.length, msg.uname.string,
		msg.aname.length, msg.aname.string);

	connid = msg9p_fidMap_newConnection(&fidMap);
	if (connid < 0) {
		return server_error("ENOCONNECT", "connection stack full.");
	}

	/* attach to root node */
	fidMap[connid].map[0].free = 0;
	fidMap[connid].map[0].nfid = 1;
	fidMap[connid].map[0].fid  = msg.fid;
	fidMap[connid].map[0].qid  = state->root;
	rmsg.aqid = state->root;

	msg9plog("server_handle_attach", "FidMap[%d]: (%02x %08x %016x)",
			connid,
			fidMap[connid].map[0].qid.type,
			fidMap[connid].map[0].qid.version,
			fidMap[connid].map[0].qid.path
		);

	return rmsg;
}

struct msg9p
server_handle_walk(struct msg9p msg, struct msg9pState* state)
{
	struct msg9p rmsg;
	struct msg9pFidMap* fmap;
	struct msg9pQid startQid;
	struct msg9pQid foundQid;
	uint32_t findex;

	/* Prepare response */
	rmsg.type = RWALK;

	/* Find Qid we need to start off */
	fmap = fidMap[state->connid].map;

	/* Check if fids are different */
	if (msg.fid == msg.newfid)
	{
		return server_error("EFIDTWIN", "cannot reassign a fid");
	}

	/*msg9plog("server_handle_walk", "looking up %d ...", msg.fid);*/
	for (findex = 0; findex < fmap->nfid; findex++)
	{

		if (fmap[findex].fid == msg.fid) {
			msg9plog("server_handle_walk", "FID %d is referring to (%02x %08x %016x)", 
				fmap[findex].fid,
				fmap[findex].qid.type,
				fmap[findex].qid.version,
				fmap[findex].qid.path
			);
			startQid = fmap[findex].qid;
			break;
		}
	}

	if (fmap[findex].fid != msg.fid)
	{
		return server_error("EFIDNOTTHERE", "that fid doesnt exist");
	}


	/* now start search */
	foundQid = msg9pFileFindPath(state->map, startQid, msg.wname[0].string);
	/* TODO: add error checking if file not found */
	for (findex = 0; findex < NMAXFILES; findex++)
	{
		if (fmap[findex].free) { break; }
	}
	if (!fmap[findex].free) {
		return server_error("EMAXFIDS", "Max fids reached for connection");
	}
	
	fmap->nfid++;
	fmap[findex].free = 0;
	fmap[findex].fid = msg.newfid;
	fmap[findex].qid = foundQid;
	msg9plog("server_handle_walk", "NEWFID %d is referring to (%02x %08x %016x)", 
			fmap[findex].fid,
			fmap[findex].qid.type,
			fmap[findex].qid.version,
			fmap[findex].qid.path
	);

	rmsg.nwqid = 1;
	rmsg.wqid = malloc(sizeof(struct msg9pQid));
	rmsg.wqid[0] = foundQid;

/*
	struct msg9pFile* file;
	uint8_t qidBuf[30];
	msg9pQidToStr(qidBuf, foundQid);
	file = mapSearch(state->map, qidBuf);
	msg9pFilePrint(file);
*/

	return rmsg;
}

struct msg9p
server_handle_open(struct msg9p msg, struct msg9pState* state)
{
	return server_error("ENOTNEEDED", "doesnt need topen");
}

struct msg9p
server_handle_create(struct msg9p msg, struct msg9pState* state)
{
	struct msg9p rmsg;
	struct msg9pFidMap* fmap;
	struct msg9pQid startQid;
	struct msg9pQid createQid;
	uint32_t findex;

	/* Prepare response */
	rmsg.type = RCREATE;

	/* Find Qid we need to start off */
	fmap = fidMap[state->connid].map;

	/* Check if that fid exists and points to a dir */
	/*msg9plog("server_handle_walk", "looking up %d ...", msg.fid);*/
	for (findex = 0; findex < fmap->nfid; findex++)
	{

		if (fmap[findex].fid == msg.fid) {
			msg9plog("server_handle_walk", "FID %d is referring to (%02x %08x %016x)", 
				fmap[findex].fid,
				fmap[findex].qid.type,
				fmap[findex].qid.version,
				fmap[findex].qid.path
			);
			startQid = fmap[findex].qid;
			/* check if dir */
			if (startQid.type != 0x01)
			{
				return server_error("ENOTDIR", "cannot create file in a file");
			}
			break;
		}
	}

	if (fmap[findex].fid != msg.fid)
	{
		return server_error("EFIDNOTTHERE", "that fid doesnt exist");
	}


	/* now create */
	createQid = msg9pFileCreatePath(state->map, startQid, msg.name.string, msg.perm);

	rmsg.qid = createQid;
	rmsg.iounit = 0x00;

	return rmsg;
}

struct msg9p
server_handle_read(struct msg9p msg, struct msg9pState* state)
{
	struct msg9pQid startQid;
	struct msg9pFidMap* fmap;
	uint32_t findex;
	struct msg9p rmsg;

	/* for dir response */
	uint32_t dindex;
	uint8_t qidStr[30];
	struct msg9pFile* dir;
	struct msg9pFile* file;
	uint8_t* fname;
	uint32_t ndcontent;
	uint8_t* dcontent;

	/* for logging only */
	uint8_t* ldcontent;

	rmsg.type = RREAD;

	fmap = fidMap[state->connid].map;

	/* Check if that fid exists and points to a dir */
	/*msg9plog("server_handle_walk", "looking up %d ...", msg.fid);*/
	for (findex = 0; findex < fmap->nfid; findex++)
	{

		if (fmap[findex].fid == msg.fid) {
			msg9plog("server_handle_walk", "FID %d is referring to (%02x %08x %016x)", 
				fmap[findex].fid,
				fmap[findex].qid.type,
				fmap[findex].qid.version,
				fmap[findex].qid.path
			);
			startQid = fmap[findex].qid;

			break;
		}
	}
	if (fmap[findex].fid != msg.fid)
	{
		return server_error("EFIDNOTTHERE", "that fid doesnt exist");
	}

	/* check if dir */
	if (startQid.type == 0x01)
	{
		msg9pQidToStr(qidStr, startQid);
		dir = (struct msg9pFile*) mapSearch(state->map, qidStr);
		/* Todo Check if empty */
		
		dcontent = NULL;
		ndcontent = 0;
		for (dindex = 1; dindex < dir->ncontent; dindex++)
		{
			msg9pQidToStr(qidStr, dir->content[dindex]);
			file = mapSearch(state->map, qidStr);

			fname = strdup(file->name);
			ndcontent += strlen(fname)+1;
			dcontent = realloc(dcontent, ndcontent);
			strncat(dcontent, fname, strlen(fname));
			strncat(dcontent, "\n", 2);

			free(fname);
		}
		ldcontent = strdup(dcontent);
		char* c = ldcontent;
		while (*c != '\0') { *c = (*c == '\n' ? ' ' : *c); c++; }
		msg9plog("server_handle_read", "DCONTENT: %s", ldcontent);
				
		free(ldcontent);
	} else {
		/* run this if file */
		msg9pQidToStr(qidStr, startQid);
		file = (struct msg9pFile*) mapSearch(state->map, qidStr);
		/* TODO: Check if empty */
		ndcontent = *((uint32_t*) ((file->callback)(3, &dcontent, msg.offset, msg.count)));
	}

	rmsg.count = ndcontent;
	rmsg.data = dcontent;

	return rmsg;
}

struct msg9p error_server_not_client(struct msg9p msg, struct msg9pState* state)
{
	return server_error("ENOTCLIENT", "server does not take R-messages");
}

struct msg9p error_not_implemented(struct msg9p msg, struct msg9pState* state)
{
	return server_error("ENOTIMPLEMENTED", "this feature is not implemented");
}

struct msg9p
server_error(uint8_t* errtype, uint8_t* errmsg)
{
	uint8_t* errtemp = "[%s]: %s";
	struct msg9p msg;
	msg9pString errbuf;
	
	errbuf.length = strlen(errtype)+strlen(errmsg)+strlen(errtemp);

	/* TODO: Add error checking */
	errbuf.string = malloc(errbuf.length);
	snprintf(errbuf.string, errbuf.length, errtemp, errtype, errmsg);
	
	msg.type = RERROR;
	msg.ename = errbuf;

	return msg;
}

