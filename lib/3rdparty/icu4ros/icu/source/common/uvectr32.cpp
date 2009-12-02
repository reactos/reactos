/*
******************************************************************************
* Copyright (C) 1999-2003, International Business Machines Corporation and   *
* others. All Rights Reserved.                                               *
******************************************************************************
*   Date        Name        Description
*   10/22/99    alan        Creation.
**********************************************************************
*/

#include "uvectr32.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN

#define DEFUALT_CAPACITY 8

/*
 * Constants for hinting whether a key is an integer
 * or a pointer.  If a hint bit is zero, then the associated
 * token is assumed to be an integer. This is needed for iSeries
 */
 
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UVector32)

UVector32::UVector32(UErrorCode &status) :
    count(0),
    capacity(0),
    elements(NULL)
{
    _init(DEFUALT_CAPACITY, status);
}

UVector32::UVector32(int32_t initialCapacity, UErrorCode &status) :
    count(0),
    capacity(0),
    elements(0)
{
    _init(initialCapacity, status);
}



void UVector32::_init(int32_t initialCapacity, UErrorCode &status) {
    // Fix bogus initialCapacity values; avoid malloc(0)
    if (initialCapacity < 1) {
        initialCapacity = DEFUALT_CAPACITY;
    }
    elements = (int32_t *)uprv_malloc(sizeof(int32_t)*initialCapacity);
    if (elements == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
    } else {
        capacity = initialCapacity;
    }
}

UVector32::~UVector32() {
    uprv_free(elements);
    elements = 0;
}

/**
 * Assign this object to another (make this a copy of 'other').
 */
void UVector32::assign(const UVector32& other, UErrorCode &ec) {
    if (ensureCapacity(other.count, ec)) {
        setSize(other.count);
        for (int32_t i=0; i<other.count; ++i) {
            elements[i] = other.elements[i];
        }
    }
}


UBool UVector32::operator==(const UVector32& other) {
    int32_t i;
    if (count != other.count) return FALSE;
    for (i=0; i<count; ++i) {
        if (elements[i] != other.elements[i]) {
            return FALSE;
        }
    }
    return TRUE;
}


void UVector32::setElementAt(int32_t elem, int32_t index) {
    if (0 <= index && index < count) {
        elements[index] = elem;
    }
    /* else index out of range */
}

void UVector32::insertElementAt(int32_t elem, int32_t index, UErrorCode &status) {
    // must have 0 <= index <= count
    if (0 <= index && index <= count && ensureCapacity(count + 1, status)) {
        for (int32_t i=count; i>index; --i) {
            elements[i] = elements[i-1];
        }
        elements[index] = elem;
        ++count;
    }
    /* else index out of range */
}

UBool UVector32::containsAll(const UVector32& other) const {
    for (int32_t i=0; i<other.size(); ++i) {
        if (indexOf(other.elements[i]) < 0) {
            return FALSE;
        }
    }
    return TRUE;
}

UBool UVector32::containsNone(const UVector32& other) const {
    for (int32_t i=0; i<other.size(); ++i) {
        if (indexOf(other.elements[i]) >= 0) {
            return FALSE;
        }
    }
    return TRUE;
}

UBool UVector32::removeAll(const UVector32& other) {
    UBool changed = FALSE;
    for (int32_t i=0; i<other.size(); ++i) {
        int32_t j = indexOf(other.elements[i]);
        if (j >= 0) {
            removeElementAt(j);
            changed = TRUE;
        }
    }
    return changed;
}

UBool UVector32::retainAll(const UVector32& other) {
    UBool changed = FALSE;
    for (int32_t j=size()-1; j>=0; --j) {
        int32_t i = other.indexOf(elements[j]);
        if (i < 0) {
            removeElementAt(j);
            changed = TRUE;
        }
    }
    return changed;
}

void UVector32::removeElementAt(int32_t index) {
    if (index >= 0) {
        for (int32_t i=index; i<count-1; ++i) {
            elements[i] = elements[i+1];
        }
        --count;
    }
}

void UVector32::removeAllElements(void) {
    count = 0;
}

UBool   UVector32::equals(const UVector32 &other) const {
    int      i;

    if (this->count != other.count) {
        return FALSE;
    }
    for (i=0; i<count; i++) {
        if (elements[i] != other.elements[i]) {
            return FALSE;
        }
    }
    return TRUE;
}




int32_t UVector32::indexOf(int32_t key, int32_t startIndex) const {
    int32_t i;
    for (i=startIndex; i<count; ++i) {
        if (key == elements[i]) {
            return i;
        }
    }
    return -1;
}


UBool UVector32::expandCapacity(int32_t minimumCapacity, UErrorCode &status) {
    if (capacity >= minimumCapacity) {
        return TRUE;
    } else {
        int32_t newCap = capacity * 2;
        if (newCap < minimumCapacity) {
            newCap = minimumCapacity;
        }
        int32_t* newElems = (int32_t *)uprv_malloc(sizeof(int32_t)*newCap);
        if (newElems == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return FALSE;
        }
        uprv_memcpy(newElems, elements, sizeof(elements[0]) * count);
        uprv_free(elements);
        elements = newElems;
        capacity = newCap;
        return TRUE;
    }
}

/**
 * Change the size of this vector as follows: If newSize is smaller,
 * then truncate the array, possibly deleting held elements for i >=
 * newSize.  If newSize is larger, grow the array, filling in new
 * slots with NULL.
 */
void UVector32::setSize(int32_t newSize) {
    int32_t i;
    if (newSize < 0) {
        return;
    }
    if (newSize > count) {
        UErrorCode ec = U_ZERO_ERROR;
        if (!ensureCapacity(newSize, ec)) {
            return;
        }
        for (i=count; i<newSize; ++i) {
            elements[i] = 0;
        }
    } 
    count = newSize;
}




/**
 * Insert the given integer into this vector at its sorted position
 * as defined by 'compare'.  The current elements are assumed to
 * be sorted already.
 */
void UVector32::sortedInsert(int32_t tok, UErrorCode& ec) {
    // Perform a binary search for the location to insert tok at.  Tok
    // will be inserted between two elements a and b such that a <=
    // tok && tok < b, where there is a 'virtual' elements[-1] always
    // less than tok and a 'virtual' elements[count] always greater
    // than tok.
    int32_t min = 0, max = count;
    while (min != max) {
        int32_t probe = (min + max) / 2;
        //int8_t c = (*compare)(elements[probe], tok);
        //if (c > 0) {
        if (elements[probe] > tok) {
            max = probe;
        } else {
            // assert(c <= 0);
            min = probe + 1;
        }
    }
    if (ensureCapacity(count + 1, ec)) {
        for (int32_t i=count; i>min; --i) {
            elements[i] = elements[i-1];
        }
        elements[min] = tok;
        ++count;
    }
}





U_NAMESPACE_END

