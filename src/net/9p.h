#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "9file.h"

enum msg9pTypes {
	TVERSION, RVERSION,
	TAUTH, RAUTH,
	RERROR,
	TFLUSH, RFLUSH,
	TATTACH, RATTACH,
	TWALK, RWALK,
	TOPEN, ROPEN,
	TCREATE, RCREATE,
	TREAD, RREAD,
	TWRITE, RWRITE,
	TCLUNK, RCLUNK,
	TREMOVE, RREMOVE,
	TSTAT, RSTAT,
	TWSTAT, RWSTAT
};


struct msg9p {
	uint32_t size; /* message size */
	uint8_t  type; /* message type */
	uint16_t tag;  /* message tag */

	uint32_t msize;      /* TVERSION, RVERSION */
	msg9pString version; /* TVERSION, RVERSION */

	uint32_t afid;     /* TAUTH, TATTACH */
	msg9pString uname; /* TAUTH, TATTACH */
	msg9pString aname; /* TAUTH, TATTACH */

	msg9pQid qid; /* RATTACH, ROPEN, RCREATE */
	msg9pQid aqid; /* RAUTH */

	msg9pString ename; /* RERROR */

	uint16_t oldtag; /* TFLUSH */

	uint32_t fid;    /* TATTACH, TWALK, TOPEN, TCREATE, TREAD, TWRITE, TCLUNK, TREMOVE, TSTAT, TWSTAT */
	uint32_t newfid; /* TWALK */
	uint16_t nwname; /* TWALK */
	msg9pString* wname; /* TWALK */

	uint16_t nwqid; /* RWALK */
	msg9pQid* wqid; /* RWALK */

	uint8_t mode; /* TOPEN, TCREATE */

	uint32_t iounit; /* ROPEN, RCREATE */
	
	msg9pString name; /* TCREATE */
	uint32_t perm;    /* TCREATE */

	uint64_t offset; /* TREAD, TWRITE */
	uint32_t count;  /* TREAD, RREAD, TWRITE, RWRITE */

	uint8_t* data; /* RREAD, TWRITE */
	msg9pString stat; /* TSTAT, TWSTAT */
};

void msg9p_convertS2M(uint8_t** str, struct msg9p* msg); /* convert string to message */
void msg9p_convertM2S(struct msg9p* msg, uint8_t** str); /* convert message to string */

void msg9p_printM(struct msg9p msg);
