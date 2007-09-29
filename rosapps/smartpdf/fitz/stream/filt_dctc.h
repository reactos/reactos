/*
 * Extend libjpegs error handler to use setjmp/longjmp
 */

#include <jpeglib.h>

#include <setjmp.h>

struct myerrmgr
{
	struct jpeg_error_mgr super;
	jmp_buf jb;
	char msg[JMSG_LENGTH_MAX];
};

static void myerrexit(j_common_ptr cinfo)
{
	struct myerrmgr *err = (struct myerrmgr *)cinfo->err;
	char msgbuf[JMSG_LENGTH_MAX];
	err->super.format_message(cinfo, msgbuf);
	strlcpy(err->msg, msgbuf, sizeof err->msg);
	longjmp(err->jb, 1);
}

static void myoutmess(j_common_ptr cinfo)
{
	struct myerrmgr *err = (struct myerrmgr *)cinfo->err;
	char msgbuf[JMSG_LENGTH_MAX];
	err->super.format_message(cinfo, msgbuf);
	fprintf(stderr, "ioerror: dct: %s", msgbuf);
}

static void myiniterr(struct myerrmgr *err)
{
	jpeg_std_error(&err->super);
	err->super.error_exit = myerrexit;
	err->super.output_message = myoutmess;
}

