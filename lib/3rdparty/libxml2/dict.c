/*
 * dict.c: dictionary of reusable strings, just used to avoid allocation
 *         and freeing operations.
 *
 * Copyright (C) 2003 Daniel Veillard.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS AND
 * CONTRIBUTORS ACCEPT NO RESPONSIBILITY IN ANY CONCEIVABLE MANNER.
 *
 * Author: daniel@veillard.com
 */

#define IN_LIBXML
#include "libxml.h"

#include <string.h>
#include <libxml/tree.h>
#include <libxml/dict.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlerror.h>
#include <libxml/globals.h>

#define MAX_HASH_LEN 4
#define MIN_DICT_SIZE 128
#define MAX_DICT_HASH 8 * 2048

/* #define ALLOW_REMOVAL */
/* #define DEBUG_GROW */

/*
 * An entry in the dictionnary
 */
typedef struct _xmlDictEntry xmlDictEntry;
typedef xmlDictEntry *xmlDictEntryPtr;
struct _xmlDictEntry {
    struct _xmlDictEntry *next;
    const xmlChar *name;
    int len;
    int valid;
};

typedef struct _xmlDictStrings xmlDictStrings;
typedef xmlDictStrings *xmlDictStringsPtr;
struct _xmlDictStrings {
    xmlDictStringsPtr next;
    xmlChar *free;
    xmlChar *end;
    int size;
    int nbStrings;
    xmlChar array[1];
};
/*
 * The entire dictionnary
 */
struct _xmlDict {
    int ref_counter;
    xmlRMutexPtr mutex;

    struct _xmlDictEntry *dict;
    int size;
    int nbElems;
    xmlDictStringsPtr strings;

    struct _xmlDict *subdict;
};

/*
 * A mutex for modifying the reference counter for shared
 * dictionaries.
 */
static xmlRMutexPtr xmlDictMutex = NULL;

/*
 * Whether the dictionary mutex was initialized.
 */
static int xmlDictInitialized = 0;

/**
 * xmlInitializeDict:
 *
 * Do the dictionary mutex initialization.
 * this function is not thread safe, initialization should
 * preferably be done once at startup
 */
static int xmlInitializeDict(void) {
    if (xmlDictInitialized)
        return(1);

    if ((xmlDictMutex = xmlNewRMutex()) == NULL)
        return(0);

    xmlDictInitialized = 1;
    return(1);
}

/**
 * xmlDictCleanup:
 *
 * Free the dictionary mutex.
 */
void
xmlDictCleanup(void) {
    if (!xmlDictInitialized)
        return;

    xmlFreeRMutex(xmlDictMutex);

    xmlDictInitialized = 0;
}

/*
 * xmlDictAddString:
 * @dict: the dictionnary
 * @name: the name of the userdata
 * @len: the length of the name, if -1 it is recomputed
 *
 * Add the string to the array[s]
 *
 * Returns the pointer of the local string, or NULL in case of error.
 */
static const xmlChar *
xmlDictAddString(xmlDictPtr dict, const xmlChar *name, int namelen) {
    xmlDictStringsPtr pool;
    const xmlChar *ret;
    int size = 0; /* + sizeof(_xmlDictStrings) == 1024 */

    pool = dict->strings;
    while (pool != NULL) {
	if (pool->end - pool->free > namelen)
	    goto found_pool;
	if (pool->size > size) size = pool->size;
	pool = pool->next;
    }
    /*
     * Not found, need to allocate
     */
    if (pool == NULL) {
        if (size == 0) size = 1000;
	else size *= 4; /* exponential growth */
        if (size < 4 * namelen) 
	    size = 4 * namelen; /* just in case ! */
	pool = (xmlDictStringsPtr) xmlMalloc(sizeof(xmlDictStrings) + size);
	if (pool == NULL)
	    return(NULL);
	pool->size = size;
	pool->nbStrings = 0;
	pool->free = &pool->array[0];
	pool->end = &pool->array[size];
	pool->next = dict->strings;
	dict->strings = pool;
    }
found_pool:
    ret = pool->free;
    memcpy(pool->free, name, namelen);
    pool->free += namelen;
    *(pool->free++) = 0;
    return(ret);
}

/*
 * xmlDictAddQString:
 * @dict: the dictionnary
 * @prefix: the prefix of the userdata
 * @name: the name of the userdata
 * @len: the length of the name, if -1 it is recomputed
 *
 * Add the QName to the array[s]
 *
 * Returns the pointer of the local string, or NULL in case of error.
 */
static const xmlChar *
xmlDictAddQString(xmlDictPtr dict, const xmlChar *prefix,
                 const xmlChar *name, int namelen)
{
    xmlDictStringsPtr pool;
    const xmlChar *ret;
    int size = 0; /* + sizeof(_xmlDictStrings) == 1024 */
    int plen;

    if (prefix == NULL) return(xmlDictAddString(dict, name, namelen));
    plen = xmlStrlen(prefix);

    pool = dict->strings;
    while (pool != NULL) {
	if (pool->end - pool->free > namelen)
	    goto found_pool;
	if (pool->size > size) size = pool->size;
	pool = pool->next;
    }
    /*
     * Not found, need to allocate
     */
    if (pool == NULL) {
        if (size == 0) size = 1000;
	else size *= 4; /* exponential growth */
        if (size < 4 * namelen) 
	    size = 4 * namelen; /* just in case ! */
	pool = (xmlDictStringsPtr) xmlMalloc(sizeof(xmlDictStrings) + size);
	if (pool == NULL)
	    return(NULL);
	pool->size = size;
	pool->nbStrings = 0;
	pool->free = &pool->array[0];
	pool->end = &pool->array[size];
	pool->next = dict->strings;
	dict->strings = pool;
    }
found_pool:
    ret = pool->free;
    memcpy(pool->free, prefix, plen);
    pool->free += plen;
    *(pool->free++) = ':';
    namelen -= plen + 1;
    memcpy(pool->free, name, namelen);
    pool->free += namelen;
    *(pool->free++) = 0;
    return(ret);
}

/*
 * xmlDictComputeKey:
 * Calculate the hash key
 */
static unsigned long
xmlDictComputeKey(const xmlChar *name, int namelen) {
    unsigned long value = 0L;
    
    if (name == NULL) return(0);
    value = *name;
    value <<= 5;
    if (namelen > 10) {
        value += name[namelen - 1];
        namelen = 10;
    }
    switch (namelen) {
        case 10: value += name[9];
        case 9: value += name[8];
        case 8: value += name[7];
        case 7: value += name[6];
        case 6: value += name[5];
        case 5: value += name[4];
        case 4: value += name[3];
        case 3: value += name[2];
        case 2: value += name[1];
        default: break;
    }
    return(value);
}

/*
 * xmlDictComputeQKey:
 * Calculate the hash key
 */
static unsigned long
xmlDictComputeQKey(const xmlChar *prefix, const xmlChar *name, int len)
{
    unsigned long value = 0L;
    int plen;
    
    if (prefix == NULL)
        return(xmlDictComputeKey(name, len));

    plen = xmlStrlen(prefix);
    if (plen == 0)
	value += 30 * (unsigned long) ':';
    else
	value += 30 * (*prefix);
    
    if (len > 10) {
        value += name[len - (plen + 1 + 1)];
        len = 10;
	if (plen > 10)
	    plen = 10;
    }
    switch (plen) {
        case 10: value += prefix[9];
        case 9: value += prefix[8];
        case 8: value += prefix[7];
        case 7: value += prefix[6];
        case 6: value += prefix[5];
        case 5: value += prefix[4];
        case 4: value += prefix[3];
        case 3: value += prefix[2];
        case 2: value += prefix[1];
        case 1: value += prefix[0];
        default: break;
    }
    len -= plen;
    if (len > 0) {
        value += (unsigned long) ':';
	len--;
    }
    switch (len) {
        case 10: value += name[9];
        case 9: value += name[8];
        case 8: value += name[7];
        case 7: value += name[6];
        case 6: value += name[5];
        case 5: value += name[4];
        case 4: value += name[3];
        case 3: value += name[2];
        case 2: value += name[1];
        case 1: value += name[0];
        default: break;
    }
    return(value);
}

/**
 * xmlDictCreate:
 *
 * Create a new dictionary
 *
 * Returns the newly created dictionnary, or NULL if an error occured.
 */
xmlDictPtr
xmlDictCreate(void) {
    xmlDictPtr dict;

    if (!xmlDictInitialized)
        if (!xmlInitializeDict())
            return(NULL);
 
    dict = xmlMalloc(sizeof(xmlDict));
    if (dict) {
        dict->ref_counter = 1;

        dict->size = MIN_DICT_SIZE;
	dict->nbElems = 0;
        dict->dict = xmlMalloc(MIN_DICT_SIZE * sizeof(xmlDictEntry));
	dict->strings = NULL;
	dict->subdict = NULL;
        if (dict->dict) {
            if ((dict->mutex = xmlNewRMutex()) != NULL) {
                memset(dict->dict, 0, MIN_DICT_SIZE * sizeof(xmlDictEntry));
                return(dict);
            }
            xmlFree(dict->dict);
        }
        xmlFree(dict);
    }
    return(NULL);
}

/**
 * xmlDictCreateSub:
 * @sub: an existing dictionnary
 *
 * Create a new dictionary, inheriting strings from the read-only
 * dictionnary @sub. On lookup, strings are first searched in the
 * new dictionnary, then in @sub, and if not found are created in the
 * new dictionnary.
 *
 * Returns the newly created dictionnary, or NULL if an error occured.
 */
xmlDictPtr
xmlDictCreateSub(xmlDictPtr sub) {
    xmlDictPtr dict = xmlDictCreate();
  
    if ((dict != NULL) && (sub != NULL)) {
        dict->subdict = sub;
	xmlDictReference(dict->subdict);
    }
    return(dict);
}

/**
 * xmlDictReference:
 * @dict: the dictionnary
 *
 * Increment the reference counter of a dictionary
 *
 * Returns 0 in case of success and -1 in case of error
 */
int
xmlDictReference(xmlDictPtr dict) {
    if (!xmlDictInitialized)
        if (!xmlInitializeDict())
            return(-1);

    if (dict == NULL) return -1;
    xmlRMutexLock(xmlDictMutex);
    dict->ref_counter++;
    xmlRMutexUnlock(xmlDictMutex);
    return(0);
}

/**
 * xmlDictGrow:
 * @dict: the dictionnary
 * @size: the new size of the dictionnary
 *
 * resize the dictionnary
 *
 * Returns 0 in case of success, -1 in case of failure
 */
static int
xmlDictGrow(xmlDictPtr dict, int size) {
    unsigned long key;
    int oldsize, i;
    xmlDictEntryPtr iter, next;
    struct _xmlDictEntry *olddict;
#ifdef DEBUG_GROW
    unsigned long nbElem = 0;
#endif
  
    if (dict == NULL)
	return(-1);
    if (size < 8)
        return(-1);
    if (size > 8 * 2048)
	return(-1);

    oldsize = dict->size;
    olddict = dict->dict;
    if (olddict == NULL)
        return(-1);
  
    dict->dict = xmlMalloc(size * sizeof(xmlDictEntry));
    if (dict->dict == NULL) {
	dict->dict = olddict;
	return(-1);
    }
    memset(dict->dict, 0, size * sizeof(xmlDictEntry));
    dict->size = size;

    /*	If the two loops are merged, there would be situations where
	a new entry needs to allocated and data copied into it from 
	the main dict. So instead, we run through the array twice, first
	copying all the elements in the main array (where we can't get
	conflicts) and then the rest, so we only free (and don't allocate)
    */
    for (i = 0; i < oldsize; i++) {
	if (olddict[i].valid == 0) 
	    continue;
	key = xmlDictComputeKey(olddict[i].name, olddict[i].len) % dict->size;
	memcpy(&(dict->dict[key]), &(olddict[i]), sizeof(xmlDictEntry));
	dict->dict[key].next = NULL;
#ifdef DEBUG_GROW
	nbElem++;
#endif
    }

    for (i = 0; i < oldsize; i++) {
	iter = olddict[i].next;
	while (iter) {
	    next = iter->next;

	    /*
	     * put back the entry in the new dict
	     */

	    key = xmlDictComputeKey(iter->name, iter->len) % dict->size;
	    if (dict->dict[key].valid == 0) {
		memcpy(&(dict->dict[key]), iter, sizeof(xmlDictEntry));
		dict->dict[key].next = NULL;
		dict->dict[key].valid = 1;
		xmlFree(iter);
	    } else {
	    	iter->next = dict->dict[key].next;
	    	dict->dict[key].next = iter;
	    }

#ifdef DEBUG_GROW
	    nbElem++;
#endif

	    iter = next;
	}
    }

    xmlFree(olddict);

#ifdef DEBUG_GROW
    xmlGenericError(xmlGenericErrorContext,
	    "xmlDictGrow : from %d to %d, %d elems\n", oldsize, size, nbElem);
#endif

    return(0);
}

/**
 * xmlDictFree:
 * @dict: the dictionnary
 *
 * Free the hash @dict and its contents. The userdata is
 * deallocated with @f if provided.
 */
void
xmlDictFree(xmlDictPtr dict) {
    int i;
    xmlDictEntryPtr iter;
    xmlDictEntryPtr next;
    int inside_dict = 0;
    xmlDictStringsPtr pool, nextp;

    if (dict == NULL)
	return;

    if (!xmlDictInitialized)
        if (!xmlInitializeDict())
            return;

    /* decrement the counter, it may be shared by a parser and docs */
    xmlRMutexLock(xmlDictMutex);
    dict->ref_counter--;
    if (dict->ref_counter > 0) {
        xmlRMutexUnlock(xmlDictMutex);
        return;
    }

    xmlRMutexUnlock(xmlDictMutex);

    if (dict->subdict != NULL) {
        xmlDictFree(dict->subdict);
    }

    if (dict->dict) {
	for(i = 0; ((i < dict->size) && (dict->nbElems > 0)); i++) {
	    iter = &(dict->dict[i]);
	    if (iter->valid == 0)
		continue;
	    inside_dict = 1;
	    while (iter) {
		next = iter->next;
		if (!inside_dict)
		    xmlFree(iter);
		dict->nbElems--;
		inside_dict = 0;
		iter = next;
	    }
	    inside_dict = 0;
	}
	xmlFree(dict->dict);
    }
    pool = dict->strings;
    while (pool != NULL) {
        nextp = pool->next;
	xmlFree(pool);
	pool = nextp;
    }
    xmlFreeRMutex(dict->mutex);
    xmlFree(dict);
}

/**
 * xmlDictLookup:
 * @dict: the dictionnary
 * @name: the name of the userdata
 * @len: the length of the name, if -1 it is recomputed
 *
 * Add the @name to the dictionnary @dict if not present.
 *
 * Returns the internal copy of the name or NULL in case of internal error
 */
const xmlChar *
xmlDictLookup(xmlDictPtr dict, const xmlChar *name, int len) {
    unsigned long key, okey, nbi = 0;
    xmlDictEntryPtr entry;
    xmlDictEntryPtr insert;
    const xmlChar *ret;

    if ((dict == NULL) || (name == NULL))
	return(NULL);

    if (len < 0)
        len = xmlStrlen(name);

    /*
     * Check for duplicate and insertion location.
     */
    okey = xmlDictComputeKey(name, len);
    key = okey % dict->size;
    if (dict->dict[key].valid == 0) {
	insert = NULL;
    } else {
	for (insert = &(dict->dict[key]); insert->next != NULL;
	     insert = insert->next) {
#ifdef __GNUC__
	    if (insert->len == len) {
		if (!memcmp(insert->name, name, len))
		    return(insert->name);
	    }
#else
	    if ((insert->len == len) &&
	        (!xmlStrncmp(insert->name, name, len)))
		return(insert->name);
#endif
	    nbi++;
	}
#ifdef __GNUC__
	if (insert->len == len) {
	    if (!memcmp(insert->name, name, len))
		return(insert->name);
	}
#else
	if ((insert->len == len) &&
	    (!xmlStrncmp(insert->name, name, len)))
	    return(insert->name);
#endif
    }

    if (dict->subdict) {
	key = okey % dict->subdict->size;
	if (dict->subdict->dict[key].valid != 0) {
	    xmlDictEntryPtr tmp;

	    for (tmp = &(dict->subdict->dict[key]); tmp->next != NULL;
		 tmp = tmp->next) {
#ifdef __GNUC__
		if (tmp->len == len) {
		    if (!memcmp(tmp->name, name, len))
			return(tmp->name);
		}
#else
		if ((tmp->len == len) &&
		    (!xmlStrncmp(tmp->name, name, len)))
		    return(tmp->name);
#endif
		nbi++;
	    }
#ifdef __GNUC__
	    if (tmp->len == len) {
		if (!memcmp(tmp->name, name, len))
		    return(tmp->name);
	    }
#else
	    if ((tmp->len == len) &&
		(!xmlStrncmp(tmp->name, name, len)))
		return(tmp->name);
#endif
	}
	key = okey % dict->size;
    }

    ret = xmlDictAddString(dict, name, len);
    if (ret == NULL)
        return(NULL);
    if (insert == NULL) {
	entry = &(dict->dict[key]);
    } else {
	entry = xmlMalloc(sizeof(xmlDictEntry));
	if (entry == NULL)
	     return(NULL);
    }
    entry->name = ret;
    entry->len = len;
    entry->next = NULL;
    entry->valid = 1;


    if (insert != NULL) 
	insert->next = entry;

    dict->nbElems++;

    if ((nbi > MAX_HASH_LEN) &&
        (dict->size <= ((MAX_DICT_HASH / 2) / MAX_HASH_LEN)))
	xmlDictGrow(dict, MAX_HASH_LEN * 2 * dict->size);
    /* Note that entry may have been freed at this point by xmlDictGrow */

    return(ret);
}

/**
 * xmlDictExists:
 * @dict: the dictionnary
 * @name: the name of the userdata
 * @len: the length of the name, if -1 it is recomputed
 *
 * Check if the @name exists in the dictionnary @dict.
 *
 * Returns the internal copy of the name or NULL if not found.
 */
const xmlChar *
xmlDictExists(xmlDictPtr dict, const xmlChar *name, int len) {
    unsigned long key, okey, nbi = 0;
    xmlDictEntryPtr insert;

    if ((dict == NULL) || (name == NULL))
	return(NULL);

    if (len < 0)
        len = xmlStrlen(name);

    /*
     * Check for duplicate and insertion location.
     */
    okey = xmlDictComputeKey(name, len);
    key = okey % dict->size;
    if (dict->dict[key].valid == 0) {
	insert = NULL;
    } else {
	for (insert = &(dict->dict[key]); insert->next != NULL;
	     insert = insert->next) {
#ifdef __GNUC__
	    if (insert->len == len) {
		if (!memcmp(insert->name, name, len))
		    return(insert->name);
	    }
#else
	    if ((insert->len == len) &&
	        (!xmlStrncmp(insert->name, name, len)))
		return(insert->name);
#endif
	    nbi++;
	}
#ifdef __GNUC__
	if (insert->len == len) {
	    if (!memcmp(insert->name, name, len))
		return(insert->name);
	}
#else
	if ((insert->len == len) &&
	    (!xmlStrncmp(insert->name, name, len)))
	    return(insert->name);
#endif
    }

    if (dict->subdict) {
	key = okey % dict->subdict->size;
	if (dict->subdict->dict[key].valid != 0) {
	    xmlDictEntryPtr tmp;

	    for (tmp = &(dict->subdict->dict[key]); tmp->next != NULL;
		 tmp = tmp->next) {
#ifdef __GNUC__
		if (tmp->len == len) {
		    if (!memcmp(tmp->name, name, len))
			return(tmp->name);
		}
#else
		if ((tmp->len == len) &&
		    (!xmlStrncmp(tmp->name, name, len)))
		    return(tmp->name);
#endif
		nbi++;
	    }
#ifdef __GNUC__
	    if (tmp->len == len) {
		if (!memcmp(tmp->name, name, len))
		    return(tmp->name);
	    }
#else
	    if ((tmp->len == len) &&
		(!xmlStrncmp(tmp->name, name, len)))
		return(tmp->name);
#endif
	}
	key = okey % dict->size;
    }

    /* not found */
    return(NULL);
}

/**
 * xmlDictQLookup:
 * @dict: the dictionnary
 * @prefix: the prefix 
 * @name: the name
 *
 * Add the QName @prefix:@name to the hash @dict if not present.
 *
 * Returns the internal copy of the QName or NULL in case of internal error
 */
const xmlChar *
xmlDictQLookup(xmlDictPtr dict, const xmlChar *prefix, const xmlChar *name) {
    unsigned long okey, key, nbi = 0;
    xmlDictEntryPtr entry;
    xmlDictEntryPtr insert;
    const xmlChar *ret;
    int len;

    if ((dict == NULL) || (name == NULL))
	return(NULL);

    len = xmlStrlen(name);
    if (prefix != NULL)
        len += 1 + xmlStrlen(prefix);

    /*
     * Check for duplicate and insertion location.
     */
    okey = xmlDictComputeQKey(prefix, name, len);
    key = okey % dict->size;
    if (dict->dict[key].valid == 0) {
	insert = NULL;
    } else {
	for (insert = &(dict->dict[key]); insert->next != NULL;
	     insert = insert->next) {
	    if ((insert->len == len) &&
	        (xmlStrQEqual(prefix, name, insert->name)))
		return(insert->name);
	    nbi++;
	}
	if ((insert->len == len) &&
	    (xmlStrQEqual(prefix, name, insert->name)))
	    return(insert->name);
    }

    if (dict->subdict) {
	key = okey % dict->subdict->size;
	if (dict->subdict->dict[key].valid != 0) {
	    xmlDictEntryPtr tmp;
	    for (tmp = &(dict->subdict->dict[key]); tmp->next != NULL;
		 tmp = tmp->next) {
		if ((tmp->len == len) &&
		    (xmlStrQEqual(prefix, name, tmp->name)))
		    return(tmp->name);
		nbi++;
	    }
	    if ((tmp->len == len) &&
		(xmlStrQEqual(prefix, name, tmp->name)))
		return(tmp->name);
	}
	key = okey % dict->size;
    }

    ret = xmlDictAddQString(dict, prefix, name, len);
    if (ret == NULL)
        return(NULL);
    if (insert == NULL) {
	entry = &(dict->dict[key]);
    } else {
	entry = xmlMalloc(sizeof(xmlDictEntry));
	if (entry == NULL)
	     return(NULL);
    }
    entry->name = ret;
    entry->len = len;
    entry->next = NULL;
    entry->valid = 1;

    if (insert != NULL) 
	insert->next = entry;

    dict->nbElems++;

    if ((nbi > MAX_HASH_LEN) &&
        (dict->size <= ((MAX_DICT_HASH / 2) / MAX_HASH_LEN)))
	xmlDictGrow(dict, MAX_HASH_LEN * 2 * dict->size);
    /* Note that entry may have been freed at this point by xmlDictGrow */

    return(ret);
}

/**
 * xmlDictOwns:
 * @dict: the dictionnary
 * @str: the string
 *
 * check if a string is owned by the disctionary
 *
 * Returns 1 if true, 0 if false and -1 in case of error
 * -1 in case of error
 */
int
xmlDictOwns(xmlDictPtr dict, const xmlChar *str) {
    xmlDictStringsPtr pool;

    if ((dict == NULL) || (str == NULL))
	return(-1);
    pool = dict->strings;
    while (pool != NULL) {
        if ((str >= &pool->array[0]) && (str <= pool->free))
	    return(1);
	pool = pool->next;
    }
    if (dict->subdict)
        return(xmlDictOwns(dict->subdict, str));
    return(0);
}

/**
 * xmlDictSize:
 * @dict: the dictionnary
 *
 * Query the number of elements installed in the hash @dict.
 *
 * Returns the number of elements in the dictionnary or
 * -1 in case of error
 */
int
xmlDictSize(xmlDictPtr dict) {
    if (dict == NULL)
	return(-1);
    if (dict->subdict)
        return(dict->nbElems + dict->subdict->nbElems);
    return(dict->nbElems);
}


#define bottom_dict
#include "elfgcchack.h"
