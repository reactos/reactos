/*****************************************************************/ 
/**                  Microsoft Windows for Workgroups                **/
/**              Copyright (C) Microsoft Corp., 1991-1992            **/
/*****************************************************************/ 

/*
    strload.cxx
    NLS/DBCS-aware string class:  LoadString methods

    This file contains the implementation of the LoadString methods
    for the NLS_STR class.  It is separate so that clients of NLS_STR who
    do not use this operator need not link to it.

    FILE HISTORY:
        rustanl    01/31/91    Created
        beng    02/07/91    Uses lmui.hxx
        gregj    03/10/92    Added caching to speed up PM ext
        gregj    04/22/93    #ifdef'd out caching to save space
*/

#include "npcommon.h"

extern "C"
{
    #include <netlib.h>
}

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <npassert.h>

#include <npstring.h>
#include <shlwapi.h>


// we define this before we include mluisupp.h so we get code
// instead of just prototypes
#define MLUI_INIT
#include <mluisupp.h>

#ifdef RESOURCE_STRING_CACHING

/*************************************************************************

    NAME:        RESOURCE_CACHE

    SYNOPSIS:    Caches a loaded resource string

    INTERFACE:    RESOURCE_CACHE()
                    Construct from an existing NLS_STR.  Automatically
                    links itself into its list.

                FindResourceCache()    [friend function]
                    Finds an ID in the list.  If not found, returns NULL,
                    else automatically promotes itself to the head of the
                    list.

                AddResourceCache()    [friend function]

                Set()
                    Sets the cached message ID and string.

                all NLS_STR methods

    PARENT:        NLS_STR

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        gregj    03/10/92    Created

**************************************************************************/

class RESOURCE_CACHE : public NLS_STR
{
friend const RESOURCE_CACHE *FindResourceCache( USHORT idMsg, RESOURCE_CACHE **ppLast,
                                                RESOURCE_CACHE **ppTrail );
friend void AddResourceCache( USHORT idMsg, const NLS_STR *nls, RESOURCE_CACHE *pLast,
                              RESOURCE_CACHE *pTrail );
protected:
    USHORT _idMsg;
    RESOURCE_CACHE *_pFwd;
    static RESOURCE_CACHE *_pList;
    static INT _cItems;
    void Promote();

public:
    RESOURCE_CACHE( USHORT idMsg, const NLS_STR &nls );
    ~RESOURCE_CACHE();
    RESOURCE_CACHE & operator=( const NLS_STR & nlsSource )
    { return (RESOURCE_CACHE&) NLS_STR::operator=( nlsSource ); }

    void Set( USHORT idMsg, const NLS_STR &nls );
};

RESOURCE_CACHE *RESOURCE_CACHE::_pList = NULL;
INT RESOURCE_CACHE::_cItems = 0;

#define RESCACHE_MAX    8    /* max number of strings to cache */

RESOURCE_CACHE::RESOURCE_CACHE( USHORT idMsg, const NLS_STR &nls )
        : NLS_STR( MAX_RES_STR_LEN + 1 ),
          _idMsg( idMsg ),
          _pFwd( NULL )
{
    if (QueryError() != WN_SUCCESS)
        return;

    *this = nls;            /* copy contents */

    _pFwd = _pList;
    _pList = this;            /* make this the new list head */

    _cItems++;                /* and count this one */
}

RESOURCE_CACHE::~RESOURCE_CACHE()
{
    RESOURCE_CACHE *pThis, *pTrail;

    for (pThis = _pList, pTrail = NULL;
         pThis != this && pThis != NULL;
         pTrail = pThis, pThis = pThis->_pFwd)
        ;

    if (pThis == NULL)
        return;

    if (pTrail == NULL)
        _pList = _pFwd;
    else
        pTrail->_pFwd = _pFwd;

    _cItems--;
}


void RESOURCE_CACHE::Promote()
{
    RESOURCE_CACHE *pThis, *pTrail;

    for (pThis = _pList, pTrail = NULL;
         pThis != this && pThis != NULL;
         pTrail = pThis, pThis = pThis->_pFwd)
        ;

    if (pThis == NULL)            /* item not found??? */
        _cItems++;                /* try to keep count accurate */
    else if (pTrail == NULL)
        return;                    /* already at list head, no change */
    else
        pTrail->_pFwd = _pFwd;    /* remove item from list */

    _pFwd = _pList;
    _pList = this;                /* make this the new list head */
}


const RESOURCE_CACHE *FindResourceCache( USHORT idMsg, RESOURCE_CACHE **ppLast,
                                         RESOURCE_CACHE **ppTrail )
{
    RESOURCE_CACHE *pThis, *pTrail;

    if (RESOURCE_CACHE::_pList == NULL) {    /* empty list? */
        *ppLast = NULL;
        *ppTrail = NULL;
        return NULL;
    }

    for (pThis = RESOURCE_CACHE::_pList;
         pThis->_pFwd != NULL && pThis->_idMsg != idMsg;
         pTrail = pThis, pThis = pThis->_pFwd)
        ;

    if (pThis->_idMsg != idMsg) {    /* item not found? */
        *ppLast = pThis;            /* return ptr to last item */
        *ppTrail = pTrail;            /* and to its predecessor */
        return NULL;
    }

    pThis->Promote();                /* item found, promote it */
    return pThis;                    /* and return it */
}


void AddResourceCache( USHORT idMsg, const NLS_STR *nls, RESOURCE_CACHE *pLast,
                        RESOURCE_CACHE *pTrail )
{
    if (RESOURCE_CACHE::_cItems < RESCACHE_MAX) {    /* cache not full, make a new entry */
        RESOURCE_CACHE *pNew = new RESOURCE_CACHE( idMsg, *nls );
                            /* automatically adds itself to the list */
    }
    else {
        if (pTrail != NULL) {        /* if not already first item */
            pTrail->_pFwd = pLast->_pFwd;    /* unlink from list */
            pLast->_pFwd = RESOURCE_CACHE::_pList;    /* and move to front */
            RESOURCE_CACHE::_pList = pLast;
        }

        pLast->Set( idMsg, *nls );    /* set new contents */
    }
}


void RESOURCE_CACHE::Set( USHORT idMsg, const NLS_STR &nls )
{
    *this = nls;
    _idMsg = idMsg;
}

#endif    /* RESOURCE_STRING_CACHING */


/*******************************************************************

    NAME:        NLS_STR::LoadString

    SYNOPSIS:    Loads a string from a resource file.

    ENTRY:

    EXIT:        Returns an error value, which is WN_SUCCESS on success.

    NOTES:        Requires that owner alloc strings must have an allocated
                size enough to fit strings of length MAX_RES_STR_LEN.
                This is requires even if the programmer thinks the string
                to be loaded is very small.  The reason is that after the
                string has been localized, the string length bound is not
                known.    Hence, the programmer always needs to allocate
                MAX_RES_STR_LEN + 1 bytes, which is guaranteed to be
                enough.

    HISTORY:
        rustanl    01/31/91    Created
        beng    07/23/91    Allow on erroneous string
        gregj    04/22/93    #ifdef'd out caching to save space

********************************************************************/

USHORT NLS_STR::LoadString( USHORT usMsgID )
{
    if (QueryError())
        return (USHORT) QueryError();

    //    Impose requirement on owner alloc'd strings (see function header).
    UIASSERT( !IsOwnerAlloc() ||
              ( QueryAllocSize() >= MAX_RES_STR_LEN + 1 ));

    if ( ! IsOwnerAlloc())
    {
        //  Resize the buffer to be big enough to hold any message.
        //  If the buffer is already this big, realloc will do nothing.
        if ( ! realloc( MAX_RES_STR_LEN + 1 ))
        {
            return WN_OUT_OF_MEMORY;
        }
    }

    //    At this point, we have a buffer which is big enough.
    UIASSERT( QueryAllocSize() >= MAX_RES_STR_LEN );

#ifdef RESOURCE_STRING_CACHING
    RESOURCE_CACHE *pLast, *pTrail;
    const RESOURCE_CACHE *prc = FindResourceCache( usMsgID, &pLast, &pTrail );
    if (prc != NULL) {
        *this = *prc;        /* copy contents */
        return WN_SUCCESS;    /* all done */
    }
#endif    /* RESOURCE_STRING_CACHING */

    int cbCopied = MLLoadStringA(usMsgID, (LPSTR)QueryPch(),
                                 QueryAllocSize());
    if ( cbCopied == 0 )
    {
        return WN_BAD_VALUE;
    }

    _cchLen = cbCopied;
    IncVers();

#ifdef RESOURCE_STRING_CACHING
    AddResourceCache( usMsgID, this, pLast, pTrail );
#endif    /* RESOURCE_STRING_CACHING */

    return WN_SUCCESS;
}


/*******************************************************************

    NAME:        NLS_STR::LoadString

    SYNOPSIS:    Loads a string from a resource file, and then inserts
                some parameters into it.

    ENTRY:

    EXIT:        Returns an error value, which is WN_SUCCESS on success.

    NOTES:        This method is provides a simple way to call the above
                LoadString and InsertParams consecutively.

    HISTORY:
        rustanl    01/31/91    Created

********************************************************************/

USHORT NLS_STR::LoadString( USHORT usMsgID,
                            const NLS_STR * apnlsParamStrings[] )
{
    USHORT usErr = LoadString( usMsgID );

    if ( usErr == WN_SUCCESS )
    {
        usErr = InsertParams( apnlsParamStrings );
    }

    return usErr;
}


#ifdef EXTENDED_STRINGS
/*******************************************************************

    NAME:        RESOURCE_STR::RESOURCE_STR

    SYNOPSIS:    Constructs a nls-string from a resource ID.

    ENTRY:        idResource

    EXIT:        Successful construct, or else ReportError

    NOTES:        This string may not be owner-alloc!  For owner-alloc,
                cons up a new one and copy this into it.

    HISTORY:
        beng    07/23/91    Created

********************************************************************/

RESOURCE_STR::RESOURCE_STR( UINT idResource )
    : NLS_STR()
{
    UIASSERT(!IsOwnerAlloc());

    USHORT usErr = LoadString(idResource);
    if (usErr)
        ReportError(usErr);
}
#endif
