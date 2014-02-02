/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */
/*
** Author: Eric Veach, July 1994.
**
*/

#ifndef __dict_list_h_
#define __dict_list_h_

/* Use #define's so that another heap implementation can use this one */

#define DictKey		DictListKey
#define Dict		DictList
#define DictNode	DictListNode

#define dictNewDict(frame,leq)		__gl_dictListNewDict(frame,leq)
#define dictDeleteDict(dict)		__gl_dictListDeleteDict(dict)

#define dictSearch(dict,key)		__gl_dictListSearch(dict,key)
#define dictInsert(dict,key)		__gl_dictListInsert(dict,key)
#define dictInsertBefore(dict,node,key)	__gl_dictListInsertBefore(dict,node,key)
#define dictDelete(dict,node)		__gl_dictListDelete(dict,node)

#define dictKey(n)			__gl_dictListKey(n)
#define dictSucc(n)			__gl_dictListSucc(n)
#define dictPred(n)			__gl_dictListPred(n)
#define dictMin(d)			__gl_dictListMin(d)
#define dictMax(d)			__gl_dictListMax(d)



typedef void *DictKey;
typedef struct Dict Dict;
typedef struct DictNode DictNode;

Dict		*dictNewDict(
			void *frame,
			int (*leq)(void *frame, DictKey key1, DictKey key2) );
			
void		dictDeleteDict( Dict *dict );

/* Search returns the node with the smallest key greater than or equal
 * to the given key.  If there is no such key, returns a node whose
 * key is NULL.  Similarly, Succ(Max(d)) has a NULL key, etc.
 */
DictNode	*dictSearch( Dict *dict, DictKey key );
DictNode	*dictInsertBefore( Dict *dict, DictNode *node, DictKey key );
void		dictDelete( Dict *dict, DictNode *node );

#define		__gl_dictListKey(n)	((n)->key)
#define		__gl_dictListSucc(n)	((n)->next)
#define		__gl_dictListPred(n)	((n)->prev)
#define		__gl_dictListMin(d)	((d)->head.next)
#define		__gl_dictListMax(d)	((d)->head.prev)
#define	       __gl_dictListInsert(d,k) (dictInsertBefore((d),&(d)->head,(k)))


/*** Private data structures ***/

struct DictNode {
  DictKey	key;
  DictNode	*next;
  DictNode	*prev;
};

struct Dict {
  DictNode	head;
  void		*frame;
  int		(*leq)(void *frame, DictKey key1, DictKey key2);
};

#endif
