/*
**********************************************************************
*   Copyright (C) 2002-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   10/11/02    aliu        Creation.
**********************************************************************
*/

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/uclean.h"
#include "cmemory.h"
#include "cstring.h"
#include "filestrm.h"
#include "uarrsort.h"
#include "unewdata.h"
#include "uoptions.h"
#include "uprops.h"
#include "propname.h"
#include "uassert.h"

#include <stdio.h>

U_NAMESPACE_USE

// TODO: Clean up and comment this code.

//----------------------------------------------------------------------
// BEGIN DATA
// 
// This is the raw data to be output.  We define the data structure,
// then include a machine-generated header that contains the actual
// data.

#include "unicode/uchar.h"
#include "unicode/uscript.h"
#include "unicode/unorm.h"

class AliasName {
public:
    const char* str;
    int32_t     index;

    AliasName(const char* str, int32_t index);

    int compare(const AliasName& other) const;

    UBool operator==(const AliasName& other) const {
        return compare(other) == 0;
    }

    UBool operator!=(const AliasName& other) const {
        return compare(other) != 0;
    }
};

AliasName::AliasName(const char* _str,
               int32_t _index) :
    str(_str),
    index(_index)
{
}

int AliasName::compare(const AliasName& other) const {
    return uprv_comparePropertyNames(str, other.str);
}

class Alias {
public:
    int32_t     enumValue;
    int32_t     nameGroupIndex;

    Alias(int32_t enumValue,
             int32_t nameGroupIndex);

    int32_t getUniqueNames(int32_t* nameGroupIndices) const;
};

Alias::Alias(int32_t anEnumValue,
                   int32_t aNameGroupIndex) :
    enumValue(anEnumValue),
    nameGroupIndex(aNameGroupIndex)
{
}

class Property : public Alias {
public:
    int32_t         valueCount;
    const Alias* valueList;

    Property(int32_t enumValue,
                       int32_t nameGroupIndex,
                       int32_t valueCount,
                       const Alias* valueList);
};

Property::Property(int32_t _enumValue,
                                       int32_t _nameGroupIndex,
                                       int32_t _valueCount,
                                       const Alias* _valueList) :
    Alias(_enumValue, _nameGroupIndex),
    valueCount(_valueCount),
    valueList(_valueList)
{
}

// *** Include the data header ***
#include "data.h"

/* return a list of unique names, not including "", for this property
 * @param stringIndices array of at least MAX_NAMES_PER_GROUP
 * elements, will be filled with indices into STRING_TABLE
 * @return number of indices, >= 1
 */
int32_t Alias::getUniqueNames(int32_t* stringIndices) const {
    int32_t count = 0;
    int32_t i = nameGroupIndex;
    UBool done = FALSE;
    while (!done) {
        int32_t j = NAME_GROUP[i++];
        if (j < 0) {
            done = TRUE;
            j = -j;
        }
        if (j == 0) continue; // omit "" entries
        UBool dupe = FALSE;
        for (int32_t k=0; k<count; ++k) {
            if (stringIndices[k] == j) {
                dupe = TRUE;
                break;
            }
            // also do a string check for things like "age|Age"
            if (STRING_TABLE[stringIndices[k]] == STRING_TABLE[j]) {
                //printf("Found dupe %s|%s\n",
                //       STRING_TABLE[stringIndices[k]].str,
                //       STRING_TABLE[j].str);
                dupe = TRUE;
                break;
            }
        }
        if (dupe) continue; // omit duplicates
        stringIndices[count++] = j;
    }
    return count;
}

// END DATA
//----------------------------------------------------------------------

#define MALLOC(type, count) \
  (type*) uprv_malloc(sizeof(type) * count)

void die(const char* msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

//----------------------------------------------------------------------

/**
 * A list of Alias objects.
 */
class AliasList {
public:
    virtual ~AliasList();
    virtual const Alias& operator[](int32_t i) const = 0;
    virtual int32_t count() const = 0;
};

AliasList::~AliasList() {}

/**
 * A single array.
 */
class AliasArrayList : public AliasList {
    const Alias* a;
    int32_t n;
public:
    AliasArrayList(const Alias* _a, int32_t _n) {
        a = _a;
        n = _n;
    }
    virtual const Alias& operator[](int32_t i) const {
        return a[i];
    }
    virtual int32_t count() const {
        return n;
    }
};

/**
 * A single array.
 */
class PropertyArrayList : public AliasList {
    const Property* a;
    int32_t n;
public:
    PropertyArrayList(const Property* _a, int32_t _n) {
        a = _a;
        n = _n;
    }
    virtual const Alias& operator[](int32_t i) const {
        return a[i];
    }
    virtual int32_t count() const {
        return n;
    }
};

//----------------------------------------------------------------------

/**
 * An element in a name index.  It maps a name (given by index) into
 * an enum value.
 */
class NameToEnumEntry {
public:
    int32_t nameIndex;
    int32_t enumValue;
    NameToEnumEntry(int32_t a, int32_t b) { nameIndex=a; enumValue=b; }
};

// Sort function for NameToEnumEntry (sort by name)
U_CFUNC int32_t
compareNameToEnumEntry(const void * /*context*/, const void* e1, const void* e2) {
    return
        STRING_TABLE[((NameToEnumEntry*)e1)->nameIndex].
            compare(STRING_TABLE[((NameToEnumEntry*)e2)->nameIndex]);
}

//----------------------------------------------------------------------

/**
 * An element in an enum index.  It maps an enum into a name group entry
 * (given by index).
 */
class EnumToNameGroupEntry {
public:
    int32_t enumValue;
    int32_t nameGroupIndex;
    EnumToNameGroupEntry(int32_t a, int32_t b) { enumValue=a; nameGroupIndex=b; }
    
    // are enumValues contiguous for count entries starting with this one?
    // ***!!!*** we assume we are in an array and look at neighbors ***!!!***
    UBool isContiguous(int32_t count) const {
        const EnumToNameGroupEntry* p = this;
        for (int32_t i=1; i<count; ++i) {
            if (p[i].enumValue != (this->enumValue + i)) {
                return FALSE;
            }
        }
        return TRUE;
    }
};

// Sort function for EnumToNameGroupEntry (sort by name index)
U_CFUNC int32_t
compareEnumToNameGroupEntry(const void * /*context*/, const void* e1, const void* e2) {
    return ((EnumToNameGroupEntry*)e1)->enumValue - ((EnumToNameGroupEntry*)e2)->enumValue;
}

//----------------------------------------------------------------------

/**
 * An element in the map from enumerated property enums to value maps.
 */
class EnumToValueEntry {
public:
    int32_t enumValue;
    EnumToNameGroupEntry* enumToName;
    int32_t enumToName_count;
    NameToEnumEntry* nameToEnum;
    int32_t nameToEnum_count;

    // are enumValues contiguous for count entries starting with this one?
    // ***!!!*** we assume we are in an array and look at neighbors ***!!!***
    UBool isContiguous(int32_t count) const {
        const EnumToValueEntry* p = this;
        for (int32_t i=1; i<count; ++i) {
            if (p[i].enumValue != (this->enumValue + i)) {
                return FALSE;
            }
        }
        return TRUE;
    }
};

// Sort function for EnumToValueEntry (sort by enum)
U_CFUNC int32_t
compareEnumToValueEntry(const void * /*context*/, const void* e1, const void* e2) {
    return ((EnumToValueEntry*)e1)->enumValue - ((EnumToValueEntry*)e2)->enumValue;
}

//----------------------------------------------------------------------
// BEGIN Builder

#define IS_VALID_OFFSET(x) (((x)>=0)&&((x)<=MAX_OFFSET))

class Builder {
    // header:
    PropertyAliases header;

    // 0:
    NonContiguousEnumToOffset* enumToName;
    int32_t enumToName_size;
    Offset enumToName_offset;

    // 1: (deleted)

    // 2:
    NameToEnum* nameToEnum;
    int32_t nameToEnum_size;
    Offset nameToEnum_offset;

    // 3:
    NonContiguousEnumToOffset* enumToValue;
    int32_t enumToValue_size;
    Offset enumToValue_offset;

    // 4:
    ValueMap* valueMap;
    int32_t valueMap_size;
    int32_t valueMap_count;
    Offset valueMap_offset;

    // for any i, one of valueEnumToName[i] or valueNCEnumToName[i] is
    // NULL and one is not.  valueEnumToName_size[i] is the size of
    // the non-NULL one.  i=0..valueMapCount-1
    // 5a:
    EnumToOffset** valueEnumToName;
    // 5b:
    NonContiguousEnumToOffset** valueNCEnumToName;
    int32_t* valueEnumToName_size;
    Offset* valueEnumToName_offset;
    // 6:
    // arrays of valueMap_count pointers, sizes, & offsets
    NameToEnum** valueNameToEnum;
    int32_t* valueNameToEnum_size;
    Offset* valueNameToEnum_offset;

    // 98:
    Offset* nameGroupPool;
    int32_t nameGroupPool_count;
    int32_t nameGroupPool_size;
    Offset nameGroupPool_offset;

    // 99:
    char* stringPool;
    int32_t stringPool_count;
    int32_t stringPool_size;
    Offset stringPool_offset;
    Offset* stringPool_offsetArray; // relative to stringPool

    int32_t total_size; // size of everything

    int32_t debug;

public:

    Builder(int32_t debugLevel);
    ~Builder();

    void buildTopLevelProperties(const NameToEnumEntry* propName,
                                 int32_t propNameCount,
                                 const EnumToNameGroupEntry* propEnum,
                                 int32_t propEnumCount);

    void buildValues(const EnumToValueEntry* e2v,
                     int32_t count);

    void buildStringPool(const AliasName* propertyNames,
                         int32_t propertyNameCount,
                         const int32_t* nameGroupIndices,
                         int32_t nameGroupIndicesCount);

    void fixup();

    int8_t* createData(int32_t& length) const;

private:

    static EnumToOffset* buildEnumToOffset(const EnumToNameGroupEntry* e2ng,
                                           int32_t count,
                                           int32_t& size);
    static NonContiguousEnumToOffset*
        buildNCEnumToNameGroup(const EnumToNameGroupEntry* e2ng,
                               int32_t count,
                               int32_t& size);

    static NonContiguousEnumToOffset*
        buildNCEnumToValue(const EnumToValueEntry* e2v,
                           int32_t count,
                           int32_t& size);

    static NameToEnum* buildNameToEnum(const NameToEnumEntry* nameToEnum,
                                       int32_t count,
                                       int32_t& size);

    Offset stringIndexToOffset(int32_t index, UBool allowNeg=FALSE) const;
    void fixupNameToEnum(NameToEnum* n);
    void fixupEnumToNameGroup(EnumToOffset* e2ng);
    void fixupNCEnumToNameGroup(NonContiguousEnumToOffset* e2ng);

    void computeOffsets();
    void fixupStringPoolOffsets();
    void fixupNameGroupPoolOffsets();
    void fixupMiscellaneousOffsets();

    static int32_t align(int32_t a);
    static void erase(void* p, int32_t size);
};

Builder::Builder(int32_t debugLevel) {
    debug = debugLevel;
    enumToName = 0;
    nameToEnum = 0;
    enumToValue = 0;
    valueMap_count = 0;
    valueMap = 0;
    valueEnumToName = 0;
    valueNCEnumToName = 0;
    valueEnumToName_size = 0;
    valueEnumToName_offset = 0;
    valueNameToEnum = 0;
    valueNameToEnum_size = 0;
    valueNameToEnum_offset = 0;
    nameGroupPool = 0;
    stringPool = 0;
    stringPool_offsetArray = 0;
}

Builder::~Builder() {
    uprv_free(enumToName);
    uprv_free(nameToEnum);
    uprv_free(enumToValue);
    uprv_free(valueMap);
    for (int32_t i=0; i<valueMap_count; ++i) {
        uprv_free(valueEnumToName[i]);
        uprv_free(valueNCEnumToName[i]);
        uprv_free(valueNameToEnum[i]);
    }
    uprv_free(valueEnumToName);
    uprv_free(valueNCEnumToName);
    uprv_free(valueEnumToName_size);
    uprv_free(valueEnumToName_offset);
    uprv_free(valueNameToEnum);
    uprv_free(valueNameToEnum_size);
    uprv_free(valueNameToEnum_offset);
    uprv_free(nameGroupPool);
    uprv_free(stringPool);
    uprv_free(stringPool_offsetArray);
}

int32_t Builder::align(int32_t a) {
    U_ASSERT(a >= 0);
    int32_t k = a % sizeof(int32_t);
    if (k == 0) {
        return a;
    }
    a += sizeof(int32_t) - k;
    return a;
}

void Builder::erase(void* p, int32_t size) {
    U_ASSERT(size >= 0);
    int8_t* q = (int8_t*) p;
    while (size--) {
        *q++ = 0;
    }
}

EnumToOffset* Builder::buildEnumToOffset(const EnumToNameGroupEntry* e2ng,
                                         int32_t count,
                                         int32_t& size) {
    U_ASSERT(e2ng->isContiguous(count));
    size = align(EnumToOffset::getSize(count));
    EnumToOffset* result = (EnumToOffset*) uprv_malloc(size);
    erase(result, size);
    result->enumStart = e2ng->enumValue;
    result->enumLimit = e2ng->enumValue + count;
    Offset* p = result->getOffsetArray();
    for (int32_t i=0; i<count; ++i) {
        // set these to NGI index values
        // fix them up to NGI offset values
        U_ASSERT(IS_VALID_OFFSET(e2ng[i].nameGroupIndex));
        p[i] = (Offset) e2ng[i].nameGroupIndex; // FIXUP later
    }
    return result;
}

NonContiguousEnumToOffset*
Builder::buildNCEnumToNameGroup(const EnumToNameGroupEntry* e2ng,
                                int32_t count,
                                int32_t& size) {
    U_ASSERT(!e2ng->isContiguous(count));
    size = align(NonContiguousEnumToOffset::getSize(count));
    NonContiguousEnumToOffset* nc = (NonContiguousEnumToOffset*) uprv_malloc(size);
    erase(nc, size);
    nc->count = count;
    EnumValue* e = nc->getEnumArray();
    Offset* p = nc->getOffsetArray();
    for (int32_t i=0; i<count; ++i) {
        // set these to NGI index values
        // fix them up to NGI offset values
        e[i] = e2ng[i].enumValue;
        U_ASSERT(IS_VALID_OFFSET(e2ng[i].nameGroupIndex));
        p[i] = (Offset) e2ng[i].nameGroupIndex; // FIXUP later
    }
    return nc;
}

NonContiguousEnumToOffset*
Builder::buildNCEnumToValue(const EnumToValueEntry* e2v,
                            int32_t count,
                            int32_t& size) {
    U_ASSERT(!e2v->isContiguous(count));
    size = align(NonContiguousEnumToOffset::getSize(count));
    NonContiguousEnumToOffset* result = (NonContiguousEnumToOffset*) uprv_malloc(size);
    erase(result, size);
    result->count = count;
    EnumValue* e = result->getEnumArray();
    for (int32_t i=0; i<count; ++i) {
        e[i] = e2v[i].enumValue;
        // offset must be set later
    }
    return result;
}

/**
 * Given an index into the string pool, return an offset.  computeOffsets()
 * must have been called already.  If allowNegative is true, allow negatives
 * and preserve their sign.
 */
Offset Builder::stringIndexToOffset(int32_t index, UBool allowNegative) const {
    // Index 0 is ""; we turn this into an Offset of zero
    if (index == 0) return 0;
    if (index < 0) {
        if (allowNegative) {
            return -Builder::stringIndexToOffset(-index);
        } else {
            die("Negative string pool index");
        }
    } else {
        if (index >= stringPool_count) {
            die("String pool index too large");
        }
        Offset result = stringPool_offset + stringPool_offsetArray[index];
        U_ASSERT(result >= 0 && result < total_size);
        return result;
    }
    return 0; // never executed; make compiler happy
}

NameToEnum* Builder::buildNameToEnum(const NameToEnumEntry* nameToEnum,
                                     int32_t count,
                                     int32_t& size) {
    size = align(NameToEnum::getSize(count));
    NameToEnum* n2e = (NameToEnum*) uprv_malloc(size);
    erase(n2e, size);
    n2e->count = count;
    Offset* p = n2e->getNameArray();
    EnumValue* e = n2e->getEnumArray();
    for (int32_t i=0; i<count; ++i) {
        // set these to SP index values
        // fix them up to SP offset values
        U_ASSERT(IS_VALID_OFFSET(nameToEnum[i].nameIndex));
        p[i] = (Offset) nameToEnum[i].nameIndex; // FIXUP later
        e[i] = nameToEnum[i].enumValue;
    }
    return n2e;
}


void Builder::buildTopLevelProperties(const NameToEnumEntry* propName,
                                      int32_t propNameCount,
                                      const EnumToNameGroupEntry* propEnum,
                                      int32_t propEnumCount) {
    enumToName = buildNCEnumToNameGroup(propEnum,
                                        propEnumCount,
                                        enumToName_size);
    nameToEnum = buildNameToEnum(propName,
                                 propNameCount,
                                 nameToEnum_size);
}

void Builder::buildValues(const EnumToValueEntry* e2v,
                          int32_t count) {
    int32_t i;
    
    U_ASSERT(!e2v->isContiguous(count));

    valueMap_count = count;

    enumToValue = buildNCEnumToValue(e2v, count,
                                     enumToValue_size);

    valueMap_size = align(count * sizeof(ValueMap));
    valueMap = (ValueMap*) uprv_malloc(valueMap_size);
    erase(valueMap, valueMap_size);

    valueEnumToName = MALLOC(EnumToOffset*, count);
    valueNCEnumToName = MALLOC(NonContiguousEnumToOffset*, count);
    valueEnumToName_size = MALLOC(int32_t, count);
    valueEnumToName_offset = MALLOC(Offset, count);
    valueNameToEnum = MALLOC(NameToEnum*, count);
    valueNameToEnum_size = MALLOC(int32_t, count);
    valueNameToEnum_offset = MALLOC(Offset, count);

    for (i=0; i<count; ++i) {
        UBool isContiguous =
            e2v[i].enumToName->isContiguous(e2v[i].enumToName_count);
        valueEnumToName[i] = 0;
        valueNCEnumToName[i] = 0;
        if (isContiguous) {
            valueEnumToName[i] = buildEnumToOffset(e2v[i].enumToName,
                                                   e2v[i].enumToName_count,
                                                   valueEnumToName_size[i]);
        } else {
            valueNCEnumToName[i] = buildNCEnumToNameGroup(e2v[i].enumToName,
                                                          e2v[i].enumToName_count,
                                                          valueEnumToName_size[i]);
        }
        valueNameToEnum[i] =
            buildNameToEnum(e2v[i].nameToEnum,
                            e2v[i].nameToEnum_count,
                            valueNameToEnum_size[i]);
    }
}

void Builder::buildStringPool(const AliasName* propertyNames,
                              int32_t propertyNameCount,
                              const int32_t* nameGroupIndices,
                              int32_t nameGroupIndicesCount) {
    int32_t i;

    nameGroupPool_count = nameGroupIndicesCount;
    nameGroupPool_size = sizeof(Offset) * nameGroupPool_count;
    nameGroupPool = MALLOC(Offset, nameGroupPool_count);

    for (i=0; i<nameGroupPool_count; ++i) {
        // Some indices are negative.
        int32_t a = nameGroupIndices[i];
        if (a < 0) a = -a;
        U_ASSERT(IS_VALID_OFFSET(a));
        nameGroupPool[i] = (Offset) nameGroupIndices[i];
    }

    stringPool_count = propertyNameCount;
    stringPool_size = 0;
    // first string must be "" -- we skip it
    U_ASSERT(*propertyNames[0].str == 0);
    for (i=1 /*sic*/; i<propertyNameCount; ++i) {
        stringPool_size += (int32_t)(uprv_strlen(propertyNames[i].str) + 1);
    }
    stringPool = MALLOC(char, stringPool_size);
    stringPool_offsetArray = MALLOC(Offset, stringPool_count);
    Offset soFar = 0;
    char* p = stringPool;
    stringPool_offsetArray[0] = -1; // we don't use this entry
    for (i=1 /*sic*/; i<propertyNameCount; ++i) {
        const char* str = propertyNames[i].str;
        int32_t len = (int32_t)uprv_strlen(str);
        uprv_strcpy(p, str);
        p += len;
        *p++ = 0;
        stringPool_offsetArray[i] = soFar;
        soFar += (Offset)(len+1);
    }
    U_ASSERT(soFar == stringPool_size);
    U_ASSERT(p == (stringPool + stringPool_size));
}

// Confirm that PropertyAliases is a POD (plain old data; see C++
// std).  The following union will _fail to compile_ if
// PropertyAliases is _not_ a POD.  (Note: We used to use the offsetof
// macro to check this, but that's not quite right, so that test is
// commented out -- see below.)
typedef union {
    int32_t i;
    PropertyAliases p;
} PropertyAliasesPODTest;

void Builder::computeOffsets() {
    int32_t i;
    Offset off = sizeof(header);

    if (debug>0) {
        printf("header   \t offset=%4d  size=%5d\n", 0, off);
    }

    // PropertyAliases must have no v-table and must be
    // padded (if necessary) to the next 32-bit boundary.
    //U_ASSERT(offsetof(PropertyAliases, enumToName_offset) == 0); // see above
    U_ASSERT(sizeof(header) % sizeof(int32_t) == 0);

    #define COMPUTE_OFFSET(foo) COMPUTE_OFFSET2(foo,int32_t)

    #define COMPUTE_OFFSET2(foo,type) \
      if (debug>0)\
        printf(#foo "\t offset=%4d  size=%5d\n", off, (int)foo##_size);\
      foo##_offset = off;\
      U_ASSERT(IS_VALID_OFFSET(off + foo##_size));\
      U_ASSERT(foo##_offset % sizeof(type) == 0);\
      off = (Offset) (off + foo##_size);

    COMPUTE_OFFSET(enumToName);     // 0:
    COMPUTE_OFFSET(nameToEnum);     // 2:
    COMPUTE_OFFSET(enumToValue);    // 3:
    COMPUTE_OFFSET(valueMap);       // 4:
        
    for (i=0; i<valueMap_count; ++i) {
        if (debug>0) {
            printf(" enumToName[%d]\t offset=%4d  size=%5d\n",
                   (int)i, off, (int)valueEnumToName_size[i]);
        }

        valueEnumToName_offset[i] = off;   // 5:
        U_ASSERT(IS_VALID_OFFSET(off + valueEnumToName_size[i]));
        off = (Offset) (off + valueEnumToName_size[i]);

        if (debug>0) {
            printf(" nameToEnum[%d]\t offset=%4d  size=%5d\n",
                   (int)i, off, (int)valueNameToEnum_size[i]);
        }

        valueNameToEnum_offset[i] = off;   // 6:
        U_ASSERT(IS_VALID_OFFSET(off + valueNameToEnum_size[i]));
        off = (Offset) (off + valueNameToEnum_size[i]);
    }
    
    // These last two chunks have weaker alignment needs
    COMPUTE_OFFSET2(nameGroupPool,Offset); // 98:
    COMPUTE_OFFSET2(stringPool,char);      // 99:

    total_size = off;
    if (debug>0) printf("total                         size=%5d\n\n", (int)total_size);
    U_ASSERT(total_size <= (MAX_OFFSET+1));
}

void Builder::fixupNameToEnum(NameToEnum* n) {
    // Fix the string pool offsets in n
    Offset* p = n->getNameArray();
    for (int32_t i=0; i<n->count; ++i) {
        p[i] = stringIndexToOffset(p[i]);
    }
}

void Builder::fixupStringPoolOffsets() {
    int32_t i;
    
    // 2:
    fixupNameToEnum(nameToEnum);

    // 6:
    for (i=0; i<valueMap_count; ++i) {
        fixupNameToEnum(valueNameToEnum[i]);
    }

    // 98:
    for (i=0; i<nameGroupPool_count; ++i) {
        nameGroupPool[i] = stringIndexToOffset(nameGroupPool[i], TRUE);
    }
}

void Builder::fixupEnumToNameGroup(EnumToOffset* e2ng) {
    EnumValue i;
    int32_t j;
    Offset* p = e2ng->getOffsetArray();
    for (i=e2ng->enumStart, j=0; i<e2ng->enumLimit; ++i, ++j) {
        p[j] = nameGroupPool_offset + sizeof(Offset) * p[j];
    }
}

void Builder::fixupNCEnumToNameGroup(NonContiguousEnumToOffset* e2ng) {
    int32_t i;
    /*EnumValue* e = e2ng->getEnumArray();*/
    Offset* p = e2ng->getOffsetArray();
    for (i=0; i<e2ng->count; ++i) {
        p[i] = nameGroupPool_offset + sizeof(Offset) * p[i];
    }    
}

void Builder::fixupNameGroupPoolOffsets() {
    int32_t i;

    // 0:
    fixupNCEnumToNameGroup(enumToName);

    // 1: (deleted)

    // 5:
    for (i=0; i<valueMap_count; ++i) {
        // 5a:
        if (valueEnumToName[i] != 0) {
            fixupEnumToNameGroup(valueEnumToName[i]);
        }
        // 5b:
        if (valueNCEnumToName[i] != 0) {
            fixupNCEnumToNameGroup(valueNCEnumToName[i]);
        }
    }
}

void Builder::fixupMiscellaneousOffsets() {
    int32_t i;

    // header:
    erase(&header, sizeof(header));
    header.enumToName_offset = enumToName_offset;
    header.nameToEnum_offset = nameToEnum_offset;
    header.enumToValue_offset = enumToValue_offset;
    // header meta-info used by Java:
    U_ASSERT(total_size > 0 && total_size < 0x7FFF);
    header.total_size = (int16_t) total_size;
    header.valueMap_offset = valueMap_offset;
    header.valueMap_count = (int16_t) valueMap_count;
    header.nameGroupPool_offset = nameGroupPool_offset;
    header.nameGroupPool_count = (int16_t) nameGroupPool_count;
    header.stringPool_offset = stringPool_offset;
    header.stringPool_count = (int16_t) stringPool_count - 1; // don't include "" entry

    U_ASSERT(valueMap_count <= 0x7FFF);
    U_ASSERT(nameGroupPool_count <= 0x7FFF);
    U_ASSERT(stringPool_count <= 0x7FFF);

    // 3:
    Offset* p = enumToValue->getOffsetArray();
    /*EnumValue* e = enumToValue->getEnumArray();*/
    U_ASSERT(valueMap_count == enumToValue->count);
    for (i=0; i<valueMap_count; ++i) {
        p[i] = (Offset)(valueMap_offset + sizeof(ValueMap) * i);
    }

    // 4:
    for (i=0; i<valueMap_count; ++i) {
        ValueMap& v = valueMap[i];
        v.enumToName_offset = v.ncEnumToName_offset = 0;
        if (valueEnumToName[i] != 0) {
            v.enumToName_offset = valueEnumToName_offset[i];
        }
        if (valueNCEnumToName[i] != 0) {
            v.ncEnumToName_offset = valueEnumToName_offset[i];
        }
        v.nameToEnum_offset = valueNameToEnum_offset[i];
    }
}

void Builder::fixup() {
    computeOffsets();
    fixupStringPoolOffsets();
    fixupNameGroupPoolOffsets();
    fixupMiscellaneousOffsets();
}

int8_t* Builder::createData(int32_t& length) const {
    length = total_size;
    int8_t* result = MALLOC(int8_t, length);
    
    int8_t* p = result;
    int8_t* limit = result + length;
    
    #define APPEND2(x, size)   \
      U_ASSERT((p+size)<=limit); \
      uprv_memcpy(p, x, size); \
      p += size

    #define APPEND(x) APPEND2(x, x##_size)

    APPEND2(&header, sizeof(header));
    APPEND(enumToName);
    APPEND(nameToEnum);
    APPEND(enumToValue);
    APPEND(valueMap);
 
    for (int32_t i=0; i<valueMap_count; ++i) {
        U_ASSERT((valueEnumToName[i] != 0 && valueNCEnumToName[i] == 0) ||
               (valueEnumToName[i] == 0 && valueNCEnumToName[i] != 0));
        if (valueEnumToName[i] != 0) {
            APPEND2(valueEnumToName[i], valueEnumToName_size[i]);
        }
        if (valueNCEnumToName[i] != 0) {
            APPEND2(valueNCEnumToName[i], valueEnumToName_size[i]);
        }
        APPEND2(valueNameToEnum[i], valueNameToEnum_size[i]);
    }

    APPEND(nameGroupPool);
    APPEND(stringPool);

    if (p != limit) {
        fprintf(stderr, "p != limit; p = %p, limit = %p", p, limit);
        exit(1);
    }
    return result;
}

// END Builder
//----------------------------------------------------------------------

/* UDataInfo cf. udata.h */
static UDataInfo dataInfo = {
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    sizeof(UChar),
    0,

    {PNAME_SIG_0, PNAME_SIG_1, PNAME_SIG_2, PNAME_SIG_3},
    {PNAME_FORMAT_VERSION, 0, 0, 0},                 /* formatVersion */
    {VERSION_0, VERSION_1, VERSION_2, VERSION_3} /* Unicode version */
};

class genpname {

    // command-line options
    UBool useCopyright;
    UBool verbose;
    int32_t debug;

public:
    int      MMain(int argc, char *argv[]);

private:
    NameToEnumEntry* createNameIndex(const AliasList& list,
                                     int32_t& nameIndexCount);

    EnumToNameGroupEntry* createEnumIndex(const AliasList& list);

    int32_t  writeDataFile(const char *destdir, const Builder&);
};

int main(int argc, char *argv[]) {
    UErrorCode status = U_ZERO_ERROR;
    u_init(&status);
    if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
        // Note: u_init() will try to open ICU property data.
        //       failures here are expected when building ICU from scratch.
        //       ignore them.
        fprintf(stderr, "genpname: can not initialize ICU.  Status = %s\n",
            u_errorName(status));
        exit(1);
    }

    genpname app;
    U_MAIN_INIT_ARGS(argc, argv);
    int retVal = app.MMain(argc, argv);
    u_cleanup();
    return retVal;
}

static UOption options[]={
    UOPTION_HELP_H,
    UOPTION_HELP_QUESTION_MARK,
    UOPTION_COPYRIGHT,
    UOPTION_DESTDIR,
    UOPTION_VERBOSE,
    UOPTION_DEF("debug", 'D', UOPT_REQUIRES_ARG),
};

NameToEnumEntry* genpname::createNameIndex(const AliasList& list,
                                           int32_t& nameIndexCount) {

    // Build name => enum map

    // This is an n->1 map.  There are typically multiple names
    // mapping to one enum.  The name index is sorted in order of the name,
    // as defined by the uprv_compareAliasNames() function.

    int32_t i, j;
    int32_t count = list.count();
    
    // compute upper limit on number of names in the index
    int32_t nameIndexCapacity = count * MAX_NAMES_PER_GROUP;
    NameToEnumEntry* nameIndex = MALLOC(NameToEnumEntry, nameIndexCapacity);

    nameIndexCount = 0;
    int32_t names[MAX_NAMES_PER_GROUP];
    for (i=0; i<count; ++i) {
        const Alias& p = list[i];
        int32_t n = p.getUniqueNames(names);
        for (j=0; j<n; ++j) {
            U_ASSERT(nameIndexCount < nameIndexCapacity);
            nameIndex[nameIndexCount++] =
                NameToEnumEntry(names[j], p.enumValue);
        }
    }

    /*
     * use a stable sort to ensure consistent results between
     * genpname.cpp and the propname.cpp swapping code
     */
    UErrorCode errorCode = U_ZERO_ERROR;
    uprv_sortArray(nameIndex, nameIndexCount, sizeof(nameIndex[0]),
                   compareNameToEnumEntry, NULL, TRUE, &errorCode);
    if (debug>1) {
        printf("Alias names: %d\n", (int)nameIndexCount);
        for (i=0; i<nameIndexCount; ++i) {
            printf("%s => %d\n",
                   STRING_TABLE[nameIndex[i].nameIndex].str,
                   (int)nameIndex[i].enumValue);
        }
        printf("\n");
    }
    // make sure there are no duplicates.  for a sorted list we need
    // only compare adjacent items.  Alias.getUniqueNames() has
    // already eliminated duplicate names for a single property, which
    // does occur, so we're checking for duplicate names between two
    // properties, which should never occur.
    UBool ok = TRUE;
    for (i=1; i<nameIndexCount; ++i) {
        if (STRING_TABLE[nameIndex[i-1].nameIndex] ==
            STRING_TABLE[nameIndex[i].nameIndex]) {
            printf("Error: Duplicate names in property list: \"%s\", \"%s\"\n",
                   STRING_TABLE[nameIndex[i-1].nameIndex].str,
                   STRING_TABLE[nameIndex[i].nameIndex].str);
            ok = FALSE;
        }
    }
    if (!ok) {
        die("Two or more duplicate names in property list");
    }

    return nameIndex;
}

EnumToNameGroupEntry* genpname::createEnumIndex(const AliasList& list) {

    // Build the enum => name map

    // This is a 1->n map.  Each enum maps to 1 or more names.  To
    // accomplish this the index entry points to an element of the
    // NAME_GROUP array.  This is the short name (which may be empty).
    // From there, subsequent elements of NAME_GROUP are alternate
    // names for this enum, up to and including the first one that is
    // negative (negate for actual index).

    int32_t i, j, k;
    int32_t count = list.count();
    
    EnumToNameGroupEntry* enumIndex = MALLOC(EnumToNameGroupEntry, count);
    for (i=0; i<count; ++i) {
        const Alias& p = list[i];
        enumIndex[i] = EnumToNameGroupEntry(p.enumValue, p.nameGroupIndex);
    }

    UErrorCode errorCode = U_ZERO_ERROR;
    uprv_sortArray(enumIndex, count, sizeof(enumIndex[0]),
                   compareEnumToNameGroupEntry, NULL, FALSE, &errorCode);
    if (debug>1) {
        printf("Property enums: %d\n", (int)count);
        for (i=0; i<count; ++i) {
            printf("%d => %d: ",
                   (int)enumIndex[i].enumValue,
                   (int)enumIndex[i].nameGroupIndex);
            UBool done = FALSE;
            for (j=enumIndex[i].nameGroupIndex; !done; ++j) {
                k = NAME_GROUP[j];
                if (k < 0) {
                    k = -k;
                    done = TRUE;
                }
                printf("\"%s\"", STRING_TABLE[k].str);
                if (!done) printf(", ");
            }
            printf("\n");
        }
        printf("\n");
    }
    return enumIndex;
}

int genpname::MMain(int argc, char* argv[])
{
    int32_t i, j;
    UErrorCode status = U_ZERO_ERROR;

    u_init(&status);
    if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
        fprintf(stderr, "Error: u_init returned %s\n", u_errorName(status));
        status = U_ZERO_ERROR;
    }


    /* preset then read command line options */
    options[3].value=u_getDataDirectory();
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);

    /* error handling, printing usage message */
    if (argc<0) {
        fprintf(stderr,
            "error in command line argument \"%s\"\n",
            argv[-argc]);
    }

    debug = options[5].doesOccur ? (*options[5].value - '0') : 0;

    if (argc!=1 || options[0].doesOccur || options[1].doesOccur ||
       debug < 0 || debug > 9) {
        fprintf(stderr,
            "usage: %s [-options]\n"
            "\tcreate " PNAME_DATA_NAME "." PNAME_DATA_TYPE "\n"
            "options:\n"
            "\t-h or -? or --help  this usage text\n"
            "\t-v or --verbose     turn on verbose output\n"
            "\t-c or --copyright   include a copyright notice\n"
            "\t-d or --destdir     destination directory, followed by the path\n"
            "\t-D or --debug 0..9  emit debugging messages (if > 0)\n",
            argv[0]);
        return argc<0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }

    /* get the options values */
    useCopyright=options[2].doesOccur;
    verbose = options[4].doesOccur;

    // ------------------------------------------------------------
    // Do not sort the string table, instead keep it in data.h order.
    // This simplifies data swapping and testing thereof because the string
    // table itself need not be sorted during swapping.
    // The NameToEnum sorter sorts each such map's string offsets instead.

    if (debug>1) {
        printf("String pool: %d\n", (int)STRING_COUNT);
        for (i=0; i<STRING_COUNT; ++i) {
            if (i != 0) {
                printf(", ");
            }
            printf("%s (%d)", STRING_TABLE[i].str, (int)STRING_TABLE[i].index);
        }
        printf("\n\n");
    }

    // ------------------------------------------------------------
    // Create top-level property indices

    PropertyArrayList props(PROPERTY, PROPERTY_COUNT);
    int32_t propNameCount;
    NameToEnumEntry* propName = createNameIndex(props, propNameCount);
    EnumToNameGroupEntry* propEnum = createEnumIndex(props);

    // ------------------------------------------------------------
    // Create indices for the value list for each enumerated property

    // This will have more entries than we need...
    EnumToValueEntry* enumToValue = MALLOC(EnumToValueEntry, PROPERTY_COUNT);
    int32_t enumToValue_count = 0;
    for (i=0, j=0; i<PROPERTY_COUNT; ++i) {
        if (PROPERTY[i].valueCount == 0) continue;
        AliasArrayList values(PROPERTY[i].valueList,
                              PROPERTY[i].valueCount);
        enumToValue[j].enumValue = PROPERTY[i].enumValue;
        enumToValue[j].enumToName = createEnumIndex(values);
        enumToValue[j].enumToName_count = PROPERTY[i].valueCount;
        enumToValue[j].nameToEnum = createNameIndex(values,
                                                    enumToValue[j].nameToEnum_count);
        ++j;
    }
    enumToValue_count = j;

    uprv_sortArray(enumToValue, enumToValue_count, sizeof(enumToValue[0]),
                   compareEnumToValueEntry, NULL, FALSE, &status);

    // ------------------------------------------------------------
    // Build PropertyAliases layout in memory

    Builder builder(debug);

    builder.buildTopLevelProperties(propName,
                                    propNameCount,
                                    propEnum,
                                    PROPERTY_COUNT);
    
    builder.buildValues(enumToValue,
                        enumToValue_count);

    builder.buildStringPool(STRING_TABLE,
                            STRING_COUNT,
                            NAME_GROUP,
                            NAME_GROUP_COUNT);

    builder.fixup();

    ////////////////////////////////////////////////////////////
    // Write the output file
    ////////////////////////////////////////////////////////////
    int32_t wlen = writeDataFile(options[3].value, builder);
    if (verbose) {
        fprintf(stdout, "Output file: %s.%s, %ld bytes\n",
            U_ICUDATA_NAME "_" PNAME_DATA_NAME, PNAME_DATA_TYPE, (long)wlen);
    }

    return 0; // success
}

int32_t genpname::writeDataFile(const char *destdir, const Builder& builder) {
    int32_t length;
    int8_t* data = builder.createData(length);

    UNewDataMemory *pdata;
    UErrorCode status = U_ZERO_ERROR;

    pdata = udata_create(destdir, PNAME_DATA_TYPE, PNAME_DATA_NAME, &dataInfo,
                         useCopyright ? U_COPYRIGHT_STRING : 0, &status);
    if (U_FAILURE(status)) {
        die("Unable to create data memory");
    }

    udata_writeBlock(pdata, data, length);

    int32_t dataLength = (int32_t) udata_finish(pdata, &status);
    if (U_FAILURE(status)) {
        die("Error writing output file");
    }
    if (dataLength != length) {
        die("Written file doesn't match expected size");
    }

    return dataLength;
}

//eof
