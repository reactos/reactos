//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       Stack.cpp
//
//  History:    31-Mar-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "Stack.hxx"

Stack_::Stack_(CRITICAL_SECTION *pCS)
{
    dwStackCount            = 0;
    psBottom                = NULL;
    ppsSorted               = NULL;
    ppsGet                  = NULL;
    pSortCriticalSection    = pCS;
}

Stack_::~Stack_(void)
{
    if (psBottom)
    {
        StackStruct_    *psCurrent;

        for (;;)
        {
            psCurrent = psBottom->psNext;

            delete psBottom->pvData;
            delete psBottom;

            psBottom    = psCurrent;

            if (!(psBottom))
            {
                break;
            }
        }
    }

    DELETE_OBJECT(ppsSorted);

    FlushGetStack();
}

BOOL Stack_::Add(DWORD cbData, void *pvData)
{
    BYTE    *pb;

    if (pb = (BYTE *)this->Add(cbData))
    {
        memcpy(pb, pvData, cbData);
        return(TRUE);
    }

    return(FALSE);
}

void *Stack_::Add(DWORD cbData)
{
    StackStruct_    *psWork;

    psWork = new StackStruct_;

    if (!(psWork))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    psWork->psNext = psBottom;

    psWork->pvData = new BYTE[cbData];

    if (!(psWork->pvData))
    {
        delete psWork;

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    psWork->cbData = cbData;

    psBottom = psWork;

    dwStackCount++;

    FlushGetStack();

    return((void *)psWork->pvData);
}

void *Stack_::Get(DWORD dwPosition, DWORD *cbData)
{
    DWORD           i;
    StackStruct_    *psRet;

    if (!(psBottom))
    {
        return(NULL);
    }

    if (dwPosition > (dwStackCount - 1))
    {
        return(NULL);
    }

    if ( InitGetStackIfNecessary() == TRUE )
    {
        DWORD Index = dwPosition;

        if ( cbData != NULL )
        {
            *cbData = ppsGet[ Index ]->cbData;
        }

        return( ppsGet[ Index ]->pvData );
    }

    psRet = psBottom;

    if (cbData)
    {
        *cbData = 0;
    }

    for (i = 0; i < dwPosition; i++)
    {
        psRet = psRet->psNext;
    }

    if ((psRet) && (psRet->pvData))
    {
        if (cbData)
        {
            *cbData = psRet->cbData;
        }

        return(psRet->pvData);
    }

    return(NULL);
}

void *Stack_::Get(DWORD cbStartIn_pvData, DWORD cbLengthIn_pvData, BYTE fbType, void *pvMemberOf_pvData)
{
    DWORD   dwEnd;
    DWORD   dwMiddle;
    DWORD   dwStart;
    DWORD   dwHalf;
    DWORD   dwCur;
    void    *pv;
    int     cmp;

    dwStart     = 0;
    dwEnd       = this->Count();
    dwHalf = dwMiddle    = dwEnd / 2L;
    dwCur       = 0;

    for EVER
    {
        if (dwCur++ > this->Count())
        {
            break;
        }

        pv = this->Get(dwMiddle);

        if (!(pv))
        {
            break;
        }

        switch (fbType)
        {
	    case STACK_SORTTYPE_PWSZ:
                cmp = wcscmp(*(WCHAR **)((BYTE *)pv + cbStartIn_pvData), (WCHAR *)pvMemberOf_pvData);
                break;

            case STACK_SORTTYPE_PSZ:
                cmp = strcmp(*(char **)((BYTE *)pv + cbStartIn_pvData), (char *)pvMemberOf_pvData);
                break;

	    case STACK_SORTTYPE_PWSZ_I:
		cmp = _wcsicmp(*(WCHAR **)((BYTE *)pv + cbStartIn_pvData), (WCHAR *)pvMemberOf_pvData);
                break;

	    case STACK_SORTTYPE_PSZ_I:
		cmp = _stricmp(*(char **)((BYTE *)pv + cbStartIn_pvData), (char *)pvMemberOf_pvData);
                break;

            default:
                cmp = memcmp((BYTE *)pv + cbStartIn_pvData, (BYTE *)pvMemberOf_pvData, cbLengthIn_pvData);
                break;
        }

        if (cmp == 0)
        {
            return(pv);
        }

        if ((dwMiddle == 0) || (dwMiddle == this->Count()) ||
            ((dwHalf == 0) && (dwMiddle == dwStart)))
        {
            break;
        }

        if (cmp < 0)
        {
            dwEnd   = dwMiddle;
        }
        else
        {
            dwStart = dwMiddle;
        }

        dwHalf      = (dwEnd - dwStart) / 2L;
        dwMiddle    = dwStart + dwHalf;
    }

    return(NULL);
}

static DWORD            cbSortOffset;
static DWORD            cbSortLength;
static BYTE             fbType;

int __cdecl Stack_compare(const void *arg1, const void *arg2)
{
    StackStruct_    **p1 = (StackStruct_ **)arg1;
    StackStruct_    **p2 = (StackStruct_ **)arg2;

    switch (fbType)
    {
	case STACK_SORTTYPE_PWSZ:
                return(wcscmp(*(WCHAR **)((BYTE *)(*p1)->pvData + cbSortOffset),
                              *(WCHAR **)((BYTE *)(*p2)->pvData + cbSortOffset)));


        case STACK_SORTTYPE_PSZ:
                return(strcmp(*(char **)((BYTE *)(*p1)->pvData + cbSortOffset),
                              *(char **)((BYTE *)(*p2)->pvData + cbSortOffset)));

	case STACK_SORTTYPE_PWSZ_I:
		return(_wcsicmp(*(WCHAR **)((BYTE *)(*p1)->pvData + cbSortOffset),
                              *(WCHAR **)((BYTE *)(*p2)->pvData + cbSortOffset)));


	case STACK_SORTTYPE_PSZ_I:
		return(_stricmp(*(char **)((BYTE *)(*p1)->pvData + cbSortOffset),
                              *(char **)((BYTE *)(*p2)->pvData + cbSortOffset)));

        case STACK_SORTTYPE_BINARY:
        default:
            break;
    }

    return(memcmp((void **)((BYTE *)(*p1)->pvData + cbSortOffset),
                  (void **)((BYTE *)(*p2)->pvData + cbSortOffset), cbSortLength));
}

void Stack_::Sort(DWORD cbOffset_pvData, DWORD cbLength_pvData, BYTE fbTypeIn)
{
    int     i;

    if (!(psBottom))
    {
        return;
    }

    //
    //  assign out offset & length for compare function
    //
    DELETE_OBJECT(ppsSorted);   // wipe the last one.

    if (!(ppsSorted = new StackStruct_ * [this->Count()]))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return;
    }

    //
    //  assign the contiguous array to be sorted.
    //
    ppsSorted[0] = psBottom;

    for (i = 1; i < (int)this->Count(); i++)
    {
        ppsSorted[i] = ppsSorted[i - 1]->psNext;
    }

    //
    //  let's do the sort!
    //

    EnterCriticalSection(pSortCriticalSection);

    cbSortOffset    = cbOffset_pvData;
    cbSortLength    = cbLength_pvData;
    fbType          = fbTypeIn;

    qsort( (void *)ppsSorted, (size_t)this->Count(), sizeof(StackStruct_ *), Stack_compare);

    LeaveCriticalSection(pSortCriticalSection);

    //
    //  now, we need to adjust out psNext's
    //

    for (i = 0; i < (int)(this->Count() - 1); i++)
    {
        ppsSorted[i]->psNext = ppsSorted[i + 1];
    }

    ppsSorted[i]->psNext = NULL;

    psBottom = ppsSorted[0];

    FlushGetStack();
}

BOOL Stack_::InitGetStackIfNecessary ()
{
    DWORD         cCount;
    StackStruct_* psGet;

    if ( ppsGet != NULL )
    {
        return( TRUE );
    }

    ppsGet = new StackStruct_ * [ dwStackCount ];

    if ( ppsGet == NULL )
    {
        return( FALSE );
    }

    psGet = psBottom;

    for ( cCount = 0; cCount < dwStackCount; cCount++ )
    {
        ppsGet[ cCount ] = psGet;
        psGet = psGet->psNext;
    }

    return( TRUE );
}

VOID Stack_::FlushGetStack ()
{
    delete ppsGet;
    ppsGet = NULL;
}
