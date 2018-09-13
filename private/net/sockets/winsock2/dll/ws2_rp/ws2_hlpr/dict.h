//--------------------------------------------------------------------
// Copyright (c)1997 Microsoft Corporation, All Right Reserved.
//
// dict.h
//
//--------------------------------------------------------------------

#ifndef _DICTIONARY_HXX__
#define _DICTIONARY_HXX__

#define INITIAL_DICTIONARY_SLOTS    4

#define DICTIONARY_MAKE_KEY( n1, n2 ) \
        (((DWORD)(n1) << 16) || ((DWORD)(n2)))

class DICTIONARY
{
protected:

    void * * DictKeys;
    void * * DictItems;
    int cDictSlots;
    int iNextItem;
    void * InitialDictKeys[INITIAL_DICTIONARY_SLOTS];
    void * InitialDictItems[INITIAL_DICTIONARY_SLOTS];

public:

    DICTIONARY();

    ~DICTIONARY();

    int // Indicates success (0), or an error (-1).
    Insert ( // Insert the item into the dictionary so that a find operation
             // using the key will return it.
        void * Key,
        void * Item
        );

    void * // Returns the item deleted from the dictionary, or 0.
    Delete ( // Delete the item named by Key from the dictionary.
        void * Key
        );

    void * // Returns the item named by key, or 0.
    Find (
        void * Key
        );

    void  // updates the item named by key.
    Update (
        void * Key,
        void *Item
        );

    void
    Reset ( // Resets the dictionary, so that when Next is called,
            // the first item will be returned.
         ) {iNextItem = 0;}

    void * // Returns the next item or 0 if at the end.
    Next (
          BOOL fRemove
          );
};

#define NEW_DICTIONARY(TYPE, KTYPE) NEW_NAMED_DICTIONARY(TYPE, TYPE, KTYPE)

#define NEW_NAMED_DICTIONARY(CLASS, TYPE, KTYPE)\
\
class TYPE;\
\
class CLASS##Dictionary : public DICTIONARY\
{\
public:\
\
    CLASS##Dictionary () {}\
    ~CLASS##Dictionary () {}\
\
    TYPE *\
    Find (KTYPE Key)\
     {return((TYPE *) DICTIONARY::Find((void *)Key));}\
\
    TYPE *\
    Delete (KTYPE Key)\
     {return((TYPE *) DICTIONARY::Delete((void *)Key));}\
\
    int\
    Insert (KTYPE Key, TYPE * Item)\
     {return(DICTIONARY::Insert((void *)Key, (void *)Item));}\
\
    void\
    Update (KTYPE Key, TYPE * Item)\
    {DICTIONARY::Update((void *)Key, (void *)Item);}\
\
    TYPE *\
    Next (BOOL fRemove = 0)\
         {return ((TYPE *) DICTIONARY::Next(fRemove));}\
}

#define NEW_BASETYPE_DICTIONARY(CLASS, TYPE, KTYPE)\
\
class CLASS##Dictionary : public DICTIONARY\
{\
public:\
\
    CLASS##Dictionary () {}\
    ~CLASS##Dictionary () {}\
\
    TYPE *\
    Find (KTYPE Key)\
     {return((TYPE *) DICTIONARY::Find((void *)Key));}\
\
    TYPE *\
    Delete (KTYPE Key)\
     {return((TYPE *) DICTIONARY::Delete((void *)Key));}\
\
    int\
    Insert (KTYPE Key, TYPE * Item)\
     {return(DICTIONARY::Insert((void *)Key, (void *)Item));}\
\
    void\
    Update (KTYPE Key, TYPE * Item)\
    {DICTIONARY::Update((void *)Key, (void *)Item);}\
\
    TYPE *\
    Next (BOOL fRemove = 0)\
         {return ((TYPE *) DICTIONARY::Next(fRemove));}\
}

#endif // _DICTIONARY_HXX__
