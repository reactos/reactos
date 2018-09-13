//--------------------------------------------------------------------
// Copyright (c)1997 Microsoft Corporation, All Right Reserved.
//
// dict.cxx
//
//--------------------------------------------------------------------

#include <windows.h>
#include <dict.h>

DICTIONARY::DICTIONARY()
{
    int iDictSlots;

    cDictSlots = INITIAL_DICTIONARY_SLOTS;
    DictKeys = InitialDictKeys;
    DictItems = InitialDictItems;
    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)
        {
        DictKeys [iDictSlots] = (void *) 0;
        DictItems[iDictSlots] = (void *) 0;
        }
}

DICTIONARY::~DICTIONARY()
{
    if (DictKeys != InitialDictKeys)
        {
        // ASSERT(DictItems != InitialDictItems);

        delete DictKeys;
        delete DictItems;
        }
}

int DICTIONARY::Insert( void * Key,
                        void * Item )
{
    int iDictSlots;
    void * * NewDictKeys;
    void * * NewDictItems;

    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)
        {
        if (DictKeys[iDictSlots] == (void *) 0)
            {
            DictKeys[iDictSlots] = Key;
            DictItems[iDictSlots] = Item;
            return(0);
            }
        }
    // Otherwise, we need to expand the size of the dictionary.
    NewDictKeys = (void * *)
                    new unsigned char [sizeof(void *) * cDictSlots * 2];
    NewDictItems = (void * *)
                    new unsigned char [sizeof(void *) * cDictSlots * 2];
    if (NewDictKeys == (void *) 0)
        {
        delete NewDictItems;
        return(-1);
        }
    if (NewDictItems == (void *) 0)
        {
        delete NewDictKeys;
        return(-1);
        }

    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)
        {
        NewDictKeys[iDictSlots] = DictKeys[iDictSlots];
        NewDictItems[iDictSlots] = DictItems[iDictSlots];
        }
    cDictSlots *= 2;
    NewDictKeys[iDictSlots] = Key;
    NewDictItems[iDictSlots] = Item;
    for (iDictSlots++; iDictSlots < cDictSlots; iDictSlots++)
        {
        NewDictKeys[iDictSlots] = (void *) 0;
        NewDictItems[iDictSlots] = (void *) 0;
        }
    if (DictKeys != InitialDictKeys)
        {
        // ASSERT(DictItems != InitialDictItems);

        delete DictKeys;
        delete DictItems;
        }

    DictKeys = NewDictKeys;
    DictItems = NewDictItems;

    return(0);
}

void *DICTIONARY::Delete ( void * Key )
{
    int iDictSlots;

    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)
        {
        if (DictKeys[iDictSlots] == Key)
            {
            void * Item = DictItems[iDictSlots];

            DictKeys [iDictSlots] = (void *) 0;
            DictItems[iDictSlots] = (void *) 0;

            return Item;
            }
        }
    return((void *) 0);
}

void *DICTIONARY::Find ( void * Key )
{
    int iDictSlots;

    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)
        {
        if (DictKeys[iDictSlots] == Key)
            {
            return(DictItems[iDictSlots]);
            }
        }
    return((void *) 0);
}

void DICTIONARY::Update( void * Key,
                         void * Item )
{
    int iDictSlots;

    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)
        {
        if (DictKeys[iDictSlots] == Key)
            {
            DictItems[iDictSlots] = Item ;
            return;
            }
        }

    // ASSERT(0) ;
}

void *DICTIONARY::Next( BOOL fRemove )
{
    for ( ; iNextItem < cDictSlots; iNextItem++)
        {
        if (DictKeys[iNextItem])
            {
            if (fRemove)
                {
                DictKeys[iNextItem] = 0;
                }

            return(DictItems[iNextItem++]);
            }
        }

    iNextItem = 0;
    return 0;
}
