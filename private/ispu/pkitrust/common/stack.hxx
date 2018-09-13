//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       Stack.hxx
//
//  History:    25-Mar-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef STACK_HXX
#define STACK_HXX

typedef struct StackStruct_
{
    DWORD           cbData;
    void            *pvData;
    StackStruct_    *psNext;
} StackStruct_;

#define     STACK_SORTTYPE_BINARY       0x01
#define     STACK_SORTTYPE_PWSZ         0x02
#define     STACK_SORTTYPE_PSZ          0x03
#define     STACK_SORTTYPE_PWSZ_I	0x04
#define     STACK_SORTTYPE_PSZ_I	0x05

class Stack_
{
    public:
        Stack_(CRITICAL_SECTION *pCriticalSection);
        virtual ~Stack_(void);

        BOOL    Add(DWORD cbData, void *pvData);
        void    *Add(DWORD cbData);

        //              position starts @ zero!
        void    *Get(DWORD dwPosition, DWORD *cbData = NULL);
        //              this one assumes this->Sort() has been called!
        void    *Get(DWORD cbStartIn_pvData, DWORD cbLengthIn_pvData, BYTE fbType, void *pvMemberOf_pvData);

        DWORD   Count(void) { return(dwStackCount); }

        void    Sort(DWORD cbStartIn_pvData, DWORD cbLengthIn_pvData, BYTE fbTypeIn);


    protected:
        StackStruct_    *psBottom;      // this will point to ppsSorted[0] if sorted.
        StackStruct_    **ppsSorted;    // this will be null if no sort
        StackStruct_    **ppsGet;

    private:
        DWORD           dwStackCount;

        CRITICAL_SECTION    *pSortCriticalSection;

        BOOL InitGetStackIfNecessary ();

        VOID FlushGetStack ();

};

#endif // STACK_HXX

