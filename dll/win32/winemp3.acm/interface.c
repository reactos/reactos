/*
 * Copyright (c) Michael Hipp and other authors of the mpglib project.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "mpg123.h"
#include "mpglib.h"

WINE_DEFAULT_DEBUG_CHANNEL(mpeg3);

BOOL InitMP3(struct mpstr *mp)
{
	static int init = 0;

	memset(mp,0,sizeof(struct mpstr));

	mp->framesize = 0;
	mp->fsizeold = -1;
	mp->bsize = 0;
	mp->head = mp->tail = NULL;
	mp->fr.single = -1;
	mp->bsnum = 0;
	mp->synth_bo = 1;
	mp->fr.mp = mp;

	if(!init) {
		init = 1;
		make_decode_tables(32767);
		init_layer2();
		init_layer3(SBLIMIT);
	}

	return !0;
}

void ClearMP3Buffer(struct mpstr *mp)
{
	struct buf *b,*bn;

	b = mp->tail;
	while(b) {
		HeapFree(GetProcessHeap(), 0, b->pnt);
		bn = b->next;
		HeapFree(GetProcessHeap(), 0, b);
		b = bn;
	}
	mp->tail = NULL;
	mp->head = NULL;
	mp->bsize = 0;
}

static struct buf *addbuf(struct mpstr *mp,const unsigned char *buf,int size)
{
	struct buf *nbuf;

	nbuf = HeapAlloc(GetProcessHeap(), 0, sizeof(struct buf));
	if(!nbuf) {
		WARN("Out of memory!\n");
		return NULL;
	}
	nbuf->pnt = HeapAlloc(GetProcessHeap(), 0, size);
	if(!nbuf->pnt) {
		HeapFree(GetProcessHeap(), 0, nbuf);
		WARN("Out of memory!\n");
		return NULL;
	}
	nbuf->size = size;
	memcpy(nbuf->pnt,buf,size);
	nbuf->next = NULL;
	nbuf->prev = mp->head;
	nbuf->pos = 0;

	if(!mp->tail) {
		mp->tail = nbuf;
	}
	else {
	  mp->head->next = nbuf;
	}

	mp->head = nbuf;
	mp->bsize += size;

	return nbuf;
}

static void remove_buf(struct mpstr *mp)
{
  struct buf *buf = mp->tail;

  mp->tail = buf->next;
  if(mp->tail)
    mp->tail->prev = NULL;
  else {
    mp->tail = mp->head = NULL;
  }

  HeapFree(GetProcessHeap(), 0, buf->pnt);
  HeapFree(GetProcessHeap(), 0, buf);

}

static int read_buf_byte(struct mpstr *mp)
{
	unsigned int b;

	int pos;

	pos = mp->tail->pos;
	while(pos >= mp->tail->size) {
		remove_buf(mp);
		pos = mp->tail->pos;
	}

	b = mp->tail->pnt[pos];
	mp->bsize--;
	mp->tail->pos++;


	return b;
}

static void read_head(struct mpstr *mp)
{
	unsigned long head;

	head = read_buf_byte(mp);
	head <<= 8;
	head |= read_buf_byte(mp);
	head <<= 8;
	head |= read_buf_byte(mp);
	head <<= 8;
	head |= read_buf_byte(mp);

	mp->header = head;
}

int decodeMP3(struct mpstr *mp,const unsigned char *in,int isize,unsigned char *out,
		int osize,int *done)
{
	int len;

	if(osize < 4608) {
		ERR("Output buffer too small\n");
		return MP3_ERR;
	}

	if(in) {
		if(addbuf(mp,in,isize) == NULL) {
			return MP3_ERR;
		}
	}

	/* First decode header */
	if(mp->framesize == 0) {
		int ret;
		if(mp->bsize < 4) {
			return MP3_NEED_MORE;
		}
		read_head(mp);
		while (!(ret = decode_header(&mp->fr,mp->header)) && mp->bsize)
		{
			mp->header = mp->header << 8;
			mp->header |= read_buf_byte(mp);
		}

		if (!ret) {
			return MP3_NEED_MORE;
		}
		mp->framesize = mp->fr.framesize;
	}

	if(mp->fr.framesize > mp->bsize)
		return MP3_NEED_MORE;

	wordpointer = mp->bsspace[mp->bsnum] + 512;
	mp->bsnum = (mp->bsnum + 1) & 0x1;
	bitindex = 0;

	len = 0;
	while(len < mp->framesize) {
		int nlen;
		int blen = mp->tail->size - mp->tail->pos;
		if( (mp->framesize - len) <= blen) {
                  nlen = mp->framesize-len;
		}
		else {
                  nlen = blen;
                }
		memcpy(wordpointer+len,mp->tail->pnt+mp->tail->pos,nlen);
                len += nlen;
                mp->tail->pos += nlen;
		mp->bsize -= nlen;
                if(mp->tail->pos == mp->tail->size) {
                   remove_buf(mp);
                }
	}

	*done = 0;
	if(mp->fr.error_protection)
           getbits(16);
        switch(mp->fr.lay) {
          case 1:
	    do_layer1(&mp->fr,out,done);
            break;
          case 2:
	    do_layer2(&mp->fr,out,done);
            break;
          case 3:
	    do_layer3(&mp->fr,out,done);
            break;
        }

	mp->fsizeold = mp->framesize;
	mp->framesize = 0;

	return MP3_OK;
}

int set_pointer(struct mpstr *mp, long backstep)
{
  unsigned char *bsbufold;
  if(mp->fsizeold < 0 && backstep > 0) {
    /* This is not a bug if we just did seeking, the first frame is dropped then */
    WARN("Can't step back %ld!\n",backstep);
    return MP3_ERR;
  }
  bsbufold = mp->bsspace[mp->bsnum] + 512;
  wordpointer -= backstep;
  if (backstep)
    memcpy(wordpointer,bsbufold+mp->fsizeold-backstep,backstep);
  bitindex = 0;
  return MP3_OK;
}
