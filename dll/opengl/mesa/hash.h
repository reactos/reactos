/* $Id: hash.h,v 1.2 1997/09/22 02:33:07 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.5
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: hash.h,v $
 * Revision 1.2  1997/09/22 02:33:07  brianp
 * added HashRemove() and HashFirstEntry()
 *
 * Revision 1.1  1997/08/22 01:15:10  brianp
 * Initial revision
 *
 */


#ifndef HASH_H
#define HASH_H


#include "GL/gl.h"


struct HashTable;



extern struct HashTable *NewHashTable(void);

extern void DeleteHashTable(struct HashTable *table);

extern void *HashLookup(const struct HashTable *table, GLuint key);

extern void HashInsert(struct HashTable *table, GLuint key, void *data);

extern void HashRemove(struct HashTable *table, GLuint key);

extern GLuint HashFirstEntry(const struct HashTable *table);

extern void HashPrint(const struct HashTable *table);

extern GLuint HashFindFreeKeyBlock(const struct HashTable *table, GLuint numKeys);


#endif
