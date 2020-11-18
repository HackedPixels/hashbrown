#include "9p.h"

#define RVLONG(v, x) v = ((uint64_t) (*(x++)) | (uint64_t) (*(x++)) << 8 | (uint64_t) (*(x++)) << 16 | (uint64_t) (*(x++)) << 24 \
	| (uint64_t) (*(x++)) << 32 | (uint64_t) (*(x++)) << 40 | (uint64_t) (*(x++)) << 48 | (uint64_t) (*(x++)) << 56)
#define RLONG(v, x)  v = (*(x++) | (*(x++) << 8) | (*(x++) << 16) | (*(x++) << 24))
#define RSHORT(v, x) v = (*(x++) | (*(x++) << 8));
#define RCHAR(v, x)  v = (*(x++))
#define RSTRING(v, x) { uint8_t* c; v.length = (*(x++) | (*(x++) << 8)); v.string = malloc(v.length+1); c = v.string; \
	while (c - v.string < v.length) {*c = *(x++); c++; }}
#define RQID(v, x)   { RCHAR(v.type, x); RLONG(v.version, x); RVLONG(v.path, x); }
#define RARRAY(v, x, n) { uint8_t* c; c = v; while (c - v < n) { *c = *(x++); }}

#define WVLONG(v, x)  { *(x++) = v; *(x++) = v >> 9; *(x++) = v >> 16; *(x++) = v >> 24; \
		*(x++) = v >> 32; *(x++) = v >> 40; *(x++) = v >> 48; *(x++) = v >> 56; }
#define WLONG(v, x)   { *(x++) = v; *(x++) = v >> 8; *(x++) = v >> 16; *(x++) = v >> 24; }
#define WSHORT(v, x)  { *(x++) = v; *(x++) = v >> 8; }
#define WCHAR(v,x)    { *(x++) = v; }
#define WSTRING(v, x) { uint8_t* c; *(x++) = v.length; *(x++) = v.length >> 8; c = v.string; \
	while (c - v.string < v.length) { *(x++) = *(c++); }}
#define WQID(v,x)     { WCHAR(v.type, x); WLONG(v.version, x); WVLONG(v.path, x); }
#define WARRAY(v, x, n) { uint8_t* c; c = v; while (c - v < n) { *(x++) = *c; }}

void
msg9p_convertS2M(uint8_t** str, struct msg9p* msg)
{
	uint8_t* walk; /* this is used to walk the message string */
	int slen; /* helper variable for RSTRING() macro */
	
	walk = *str;

	RLONG(msg->size, walk); /* read message length */
	RCHAR(msg->type, walk); /* read message type */
	RSHORT(msg->tag, walk); /* read message tag */

	switch (msg->type)
	{
		/* VERSION */
		case TVERSION:
		case RVERSION:
			RLONG(msg->msize, walk);
			RSTRING(msg->version, walk);
		break;

		/* AUTHENTICATION */
		case TAUTH:
			RLONG(msg->afid, walk);
			RSTRING(msg->uname, walk);
			RSTRING(msg->aname, walk);
		break;
		case RAUTH:
			RQID(msg->qid, walk);
		break;

		/* ERROR */
		case RERROR:
			RSTRING(msg->ename, walk);
		break;

		/* FLUSH */
		case TFLUSH:
			RSHORT(msg->oldtag, walk);
		break;
		case RFLUSH:
		break;

		/* ATTACH */
		case TATTACH:
			RLONG(msg->fid, walk);
			RLONG(msg->afid, walk);
			RSTRING(msg->uname, walk);
			RSTRING(msg->aname, walk);
		break;
		case RATTACH:
			RQID(msg->qid, walk);
		break;

		/* WALK */
		case TWALK:
			RLONG(msg->fid, walk);
			RLONG(msg->newfid, walk);
			RSHORT(msg->nwname, walk);

			msg->wname = malloc(msg->nwname * sizeof(msg9pString));
			for (int n = 0; n < msg->nwname; n++)
			{
				RSTRING(msg->wname[n], walk);
			}
		break;
		case RWALK:
			RSHORT(msg->nwqid, walk);
			msg->wqid = malloc(msg->nwqid * sizeof(msg9pQid));
			for (int n = 0; n < msg->nwqid; n++)
			{
				RQID(msg->wqid[n], walk);
			}
		break;

		/* OPEN */
		case TOPEN:
			RLONG(msg->fid, walk);
			RCHAR(msg->mode, walk);
		break;
		case ROPEN:
			RQID(msg->qid, walk);
			RLONG(msg->iounit, walk);
		break;

		/* CREATE */
		case TCREATE:
			RLONG(msg->fid, walk);
			RSTRING(msg->name, walk);
			RLONG(msg->perm, walk);
			RCHAR(msg->mode, walk);
		break;
		case RCREATE:
			RQID(msg->qid, walk);
			RLONG(msg->iounit, walk);
		break;

		/* READ */
		case TREAD:
			RLONG(msg->fid, walk);
			RVLONG(msg->offset, walk);
			RLONG(msg->count, walk);
		break;
		case RREAD:
			RLONG(msg->count, walk);
			msg->data = malloc(msg->count);
			RARRAY(msg->data, walk, msg->count);
		break;

		/* WRITE */
		case TWRITE:
			RLONG(msg->fid, walk);
			RVLONG(msg->offset, walk);
			RLONG(msg->count, walk);
			msg->data = malloc(msg->count);
			RARRAY(msg->data, walk, msg->count);
		break;
		case RWRITE:
			RLONG(msg->count, walk);
		break;

		/* CLUNK, REMOVE, STAT */
		case TCLUNK:
		case TREMOVE:
		case TSTAT:
			RLONG(msg->fid, walk);
		break;
		case RCLUNK:
		case RREMOVE:
		case RWSTAT:
		break;
		case RSTAT:
			RSTRING(msg->stat, walk);
		break;
		case TWSTAT:
			RLONG(msg->fid, walk);
			RSTRING(msg->stat, walk);
		break;
	}
}

void
msg9p_convertM2S(struct msg9p* msg, uint8_t** str)
{
	uint8_t* walk; /* this is used to walk the message string */
	int slen; /* helper variable for RSTRING() macro */
	
	walk = *str;

	walk += 4; /* skip length field */
	WCHAR(msg->type, walk); /* read message type */
	WSHORT(msg->tag, walk); /* read message tag */

	switch (msg->type)
	{
		/* VERSION */
		case TVERSION:
		case RVERSION:
			WLONG(msg->msize, walk);
			WSTRING(msg->version, walk);
		break;

		/* AUTHENTICATION */
		case TAUTH:
			WLONG(msg->afid, walk);
			WSTRING(msg->uname, walk);
			WSTRING(msg->aname, walk);
		break;
		case RAUTH:
			WQID(msg->qid, walk);
		break;

		/* ERROR */
		case RERROR:
			WSTRING(msg->ename, walk);
		break;

		/* FLUSH */
		case TFLUSH:
			WSHORT(msg->oldtag, walk);
		break;
		case RFLUSH:
		break;

		/* ATTACH */
		case TATTACH:
			WLONG(msg->fid, walk);
			WLONG(msg->afid, walk);
			WSTRING(msg->uname, walk);
			WSTRING(msg->aname, walk);
		break;
		case RATTACH:
			WQID(msg->qid, walk);
		break;

		/* WALK */
		case TWALK:
			WLONG(msg->fid, walk);
			WLONG(msg->newfid, walk);
			WSHORT(msg->nwname, walk);

			for (int n = 0; n < msg->nwname; n++)
			{
				WSTRING(msg->wname[n], walk);
			}
		break;
		case RWALK:
			WSHORT(msg->nwqid, walk);
			for (int n = 0; n < msg->nwqid; n++)
			{
				WQID(msg->wqid[n], walk);
			}
		break;

		/* OPEN */
		case TOPEN:
			WLONG(msg->fid, walk);
			WCHAR(msg->mode, walk);
		break;
		case ROPEN:
			WQID(msg->qid, walk);
			WLONG(msg->iounit, walk);
		break;

		/* CREATE */
		case TCREATE:
			WLONG(msg->fid, walk);
			WSTRING(msg->name, walk);
			WLONG(msg->perm, walk);
			WCHAR(msg->mode, walk);
		break;
		case RCREATE:
			WQID(msg->qid, walk);
			WLONG(msg->iounit, walk);
		break;

		/* READ */
		case TREAD:
			WLONG(msg->fid, walk);
			WVLONG(msg->offset, walk);
			WLONG(msg->count, walk);
		break;
		case RREAD:
			WLONG(msg->count, walk);
			WARRAY(msg->data, walk, msg->count);
		break;

		/* WRITE */
		case TWRITE:
			WLONG(msg->fid, walk);
			WVLONG(msg->offset, walk);
			WLONG(msg->count, walk);
			WARRAY(msg->data, walk, msg->count);
		break;
		case RWRITE:
			WLONG(msg->count, walk);
		break;

		/* CLUNK, REMOVE, STAT */
		case TCLUNK:
		case TREMOVE:
		case TSTAT:
			WLONG(msg->fid, walk);
		break;
		case RCLUNK:
		case RREMOVE:
		case RWSTAT:
		break;
		case RSTAT:
			WSTRING(msg->stat, walk);
		break;
		case TWSTAT:
			WLONG(msg->fid, walk);
			WSTRING(msg->stat, walk);
		break;
	}

	/* write length field */
	int wlen = walk - *str;
	walk = *str;
	msg->size = wlen;
	WLONG(wlen, walk);
}

void
msg9p_printM(struct msg9p msg)
{
	printf("%08x %02x %04x ",
		msg.size, msg.type, msg.tag);
	switch (msg.type)
	{
		/* VERSION */
		case TVERSION:
		case RVERSION:
			printf("%08x %.*s\n", msg.msize, msg.version.length, msg.version.string);
		break;

		/* AUTHENTICATION */
		case TAUTH:
			printf("%08x %.*s %.*s\n",
				msg.afid, msg.uname.length, msg.uname.string,
				msg.aname.length, msg.aname.string);
		break;
		case RAUTH:
			printf("%02x%08x%16x\n",
				msg.aqid.type, msg.aqid.version, msg.aqid.path);
		break;

		/* ERROR */
		case RERROR:
			printf("%.*s\n", msg.ename.length, msg.ename.string);
		break;

		/* FLUSH */
		case TFLUSH:
			printf("%04x\n", msg.oldtag);
		break;
		case RFLUSH:
			printf("\n");
		break;

		/* ATTACH */
		case TATTACH:
			printf("%08x %08x %.*s %.*s\n", msg.fid, msg.afid,
				msg.uname.length, msg.uname.string,
				msg.aname.length, msg.aname.string);
		break;
		case RATTACH:
			printf("(%02x %08x %016x)\n",
				msg.aqid.type, msg.aqid.version, msg.aqid.path);
		break;

		/* WALK */
		case TWALK:
			printf("%08x %08x %04x ", msg.fid, msg.newfid, msg.nwname);

			for (int n = 0; n < msg.nwname; n++)
			{
				printf("%.*s ", msg.wname[n].length, msg.wname[n].string);
			}
			printf("\n");
		break;
		case RWALK:
			printf("%04x ", msg.nwqid);
			for (int n = 0; n < msg.nwqid; n++)
			{
				printf("(%02x %08x %016x) ",
					msg.wqid[n].type, msg.wqid[n].version, msg.wqid[n].path);
			}
			printf("\n");
		break;

		/* OPEN */
		case TOPEN:
			printf("%08x %02x\n", msg.fid, msg.mode);
		break;
		case ROPEN:
			printf("(%02x %08x %016x) %08x\n",
				msg.qid.type, msg.qid.version, msg.qid.path, msg.iounit);
		break;
	
		/* CREATE */
		case TCREATE:
			printf("%08x %.*s %08x %02x\n",
				msg.fid, msg.name.length, msg.name.string,
				msg.perm, msg.mode);
		break;
		case RCREATE:
			printf("(%02x %08x %016x) %08x\n",
				msg.qid.type, msg.qid.version, msg.qid.path, msg.iounit);
		break;
		case TREAD:
			printf("%08x %016x %08x\n",
				msg.fid, msg.offset, msg.count);
		break;
		case RREAD:
			printf("%08x %s\n",
				msg.count, "[skipping data]");
		break;
		case TWRITE:
		case RWRITE:
		case TCLUNK:
		case RCLUNK:
		case TREMOVE:
		case RREMOVE:
		case TSTAT:
		case RSTAT:
		case TWSTAT:
		case RWSTAT:
		break;
	}
}
