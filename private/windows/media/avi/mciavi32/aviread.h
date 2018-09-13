/*
 * Copyright (C) Microsoft Corporation 1992. All rights reserved.
 */


/*
 * aviread.h	interface functions to read module providing asynchronous
 *		reading of mmio files.
 *		Only in WIN32 case.
 *
 * 		Creating an avird object causes a worker thread to start
 *		up and callback to a caller-defined function to read blocks
 *		and queue them.
 *		The caller can then call avird_getnextbuffer to get each buffer
 * 		in turn, and once finished with them, should call
 *		avird_emptybuffer: this will notify the avird object that
 *		the buffer is no longer wanted and can be used to read
 *		ahead further blocks.
 */

#ifdef _WIN32

/* handles to avird */
typedef struct avird_header *	HAVIRD;

/* caller passes us a AVIRD_FUNC pointer to a function that
 * will fill the buffer. It takes four args: the buffer to be filled,
 * a dword instance data (containing the mmio handle or npMCI or whatever),
 * a long giving the
 * size of the block to read, and a pointer to the long where it should
 * return the size of the next block. This function will be never
 * be called out of sequence, so assuming the file pointer is at the
 * correct place before calling avird_startread, blocks will be read
 * in sequence by this function (on the worker thread). The function should
 * return FALSE if the read failed in any way.
 */
typedef BOOL (*AVIRD_FUNC)(PBYTE pData, DWORD_PTR dwInstanceData, long lSize, long * plNextSize);

/*
 * start an avird operation and return a handle to use in subsequent
 * calls. This will cause an asynchronous read (achieved using a separate
 * thread) to start reading the next few buffers. it will not read past
 * nblocks assuming that filler will start at firstblock.
 */
HAVIRD avird_startread(AVIRD_FUNC func, DWORD_PTR dwInstanceData, long lFirstSize,
			int firstblock, int nblocks);


/*
 * return the next buffer from an HAVIRD object. also set plSize to
 * the size of the buffer. Returns NULL if there was an error reading
 * the buffer.
 */
PBYTE avird_getnextbuffer(HAVIRD havird, long * plSize);

/*
 * return a buffer that has been finished with (is now empty)
 */
void avird_emptybuffer(HAVIRD havird, PBYTE pBuffer);

/*
 * delete an avird object. the worker thread will be stopped and all
 * data allocated will be freed. The HAVIRD handle is no longer valid after
 * this call.
 */
void avird_endread(HAVIRD havird);


#endif /* _WIN32 */
