/*
 *  MSGCIDX.CXX
 *  
 *  Create and extend PR_CONVERSATION_INDEX
 */

#include <padhead.hxx>

#define cbConvIndexHdr          22
#define cbConvIndexComponent    5
#define bConvIndexRes           (BYTE)1

/*
 *  ExtractLastFileTime()
 *  
 *  Purpose:
 *  
 *      Parses an existing covnersation index and extracts the last
 *      FILETIME value contained in the index.
 */
VOID
ExtractLastFileTime (LPBYTE lpb, ULONG cb, FILETIME FAR * lpft)
{
    FILETIME ft;
    FILETIME ftCur;
    LPBYTE lpbEnd;

    //  Lets do some verification on the key
    //
    Assert (!IsBadReadPtr (lpb, (UINT)cb));
    Assert (!IsBadWritePtr (lpft, sizeof(FILETIME)));
    Assert (*lpb == bConvIndexRes);
    Assert (cb >= cbConvIndexHdr);
    Assert (!((cb - cbConvIndexHdr) % cbConvIndexComponent));

    //  Rebuild the header time\date into FILETIME format
    //
    ft.dwHighDateTime = (((DWORD)(lpb[1])) << 16) |
                        (((DWORD)(lpb[2])) << 8) |
                        ((DWORD)(lpb[3]));

    ft.dwLowDateTime = (((DWORD)(lpb[4])) << 24) |
                       (((DWORD)(lpb[5])) << 16);

    //  See where the last child chunk ends
    //
    lpbEnd = lpb + cb;
    lpb += cbConvIndexHdr;

    //  Now go through the child chunks to compute
    //  for the last FILETIME using the delta
    //
    while (lpb < lpbEnd)
    {
        //  Convert the delta of the current child
        //  chunk into the FILETIME format.  Use the
        //  delta code in the first bit to get the
        //  real delta.
        //
        //  Delta code : 1 = mask 10000000 = 0x80
        //
        if ((*lpb & 0x80) == 0x80)
        {
            //  Mask out the first bit used for the delta code
            //  *lpb | 0x7F;
            //
            ftCur.dwHighDateTime = (((DWORD)(lpb[0] & 0x7F)) << 15) |
                                   (((DWORD)(lpb[1])) << 7) |
                                   (((DWORD)(lpb[2])) >> 1);

            ftCur.dwLowDateTime = (((DWORD)(lpb[2])) << 31) |
                                  (((DWORD)(lpb[3])) << 23);

            ft = FtAddFt (ft, ftCur);
        }
        else
        {
            ftCur.dwHighDateTime = (((DWORD)(lpb[0] & 0x7F)) << 10) |
                                   (((DWORD)(lpb[1])) << 2) |
                                   (((DWORD)(lpb[2])) >> 6);

            ftCur.dwLowDateTime = (((DWORD)(lpb[2])) << 26) |
                                  (((DWORD)(lpb[3])) << 18);

            ft = FtAddFt (ft, ftCur);
        }

        // Advance to next child
        //
        lpb += cbConvIndexComponent;
    }

    //  If all went well, we sould have ended up at
    //  lpbEnd
    //
    Assert (lpb == lpbEnd);
    *lpft = ft;
    return;
}

/*
 *  ScFillConvHeader()
 *  
 *  Purpose:
 *  
 *      Fills in the header of a conversation index.  This function is
 *      called when a new conversation index is created.
 *  
 *  Assumptions:
 *  
 *      The buffer passed in should be big enough to hold cbConvIndexHdr
 *      bytes (22 bytes).
 */
SCODE
ScFillConvHeader (LPBYTE rgb, ULONG cb)
{
    SCODE sc = S_OK;
    SYSTEMTIME st;
    FILETIME ft;
    GUID guid;

    Assert (cb >= cbConvIndexHdr);
    Assert (!IsBadWritePtr (rgb, cbConvIndexHdr));

    //  (Ha). Put the reserved byte
    //
    rgb[0] = bConvIndexRes;
        
    //  (Hb). Put the current time
    //
    GetSystemTime (&st);
    SystemTimeToFileTime (&st, &ft);

    //  Construct the date\time one byte at a time
    //
    rgb[1] = (BYTE) ((ft.dwHighDateTime & 0x00FF0000) >> 16);
    rgb[2] = (BYTE) ((ft.dwHighDateTime & 0x0000FF00) >> 8);
    rgb[3] = (BYTE) (ft.dwHighDateTime & 0x000000FF);

    //  Drop the rightmost least significant 2 bytes
    //
    rgb[4] = (BYTE) ((ft.dwLowDateTime & 0xFF000000) >> 24);
    rgb[5] = (BYTE) ((ft.dwLowDateTime & 0x00FF0000) >> 16);

    //  (Hc). Now put the GUID
    //      {
    //          DWORD Data1;
    //          WORD  Data2;
    //          WORD  Data3;
    //          BYTE  Data4[8];
    //      } GUID;
    //
    sc = GetScode (CoCreateGuid (&guid));
    if (!FAILED (sc))
    {       
        //  Again, lets do it one byte at a time
        //
        rgb[6] = (BYTE) ((guid.Data1 & 0xFF000000) >> 24);
        rgb[7] = (BYTE) ((guid.Data1 & 0x00FF0000) >> 16);
        rgb[8] = (BYTE) ((guid.Data1 & 0x0000FF00) >> 8);
        rgb[9] = (BYTE) ((guid.Data1 & 0x000000FF));
        rgb[10] = (BYTE) ((guid.Data2 & 0xFF00) >> 8);
        rgb[11] = (BYTE) ((guid.Data2 & 0x00FF));
        rgb[12] = (BYTE) ((guid.Data3 & 0xFF00) >> 8);
        rgb[13] = (BYTE) ((guid.Data3 & 0x00FF));
    }

    //  Slurp the rest across
    //
    CopyMemory (&rgb[14], &guid.Data4, 8);
    //DebugTraceSc (ScFillConvHeader(), sc);
    return sc;
}

/*
 *  ScAddConversationIndex()
 *  
 *  Purpose:
 *  
 *      Given the conversation index to a message, this function will
 *      create the conversation of a child message to the original.  If
 *      the no original is suplied, then an index is created that would
 *      signify the start of a new thread.
 */
SCODE
ScAddConversationIndex (ULONG cbParent,
    LPBYTE lpbParent,
    ULONG FAR * lpcb,
    LPBYTE FAR * lppb)
{
    SCODE sc;
    DWORD dwTemp;
    SYSTEMTIME st;
    FILETIME ft;
    FILETIME ftLast;
    FILETIME ftDelta;
    HMODULE hMAPIDll = NULL;
    typedef SCODE (STDAPICALLTYPE FAR *MAPICONVIDX)(ULONG, LPBYTE, ULONG FAR *, LPBYTE FAR *);
    MAPICONVIDX lpfnMAPIConvIdx = NULL;

#ifdef _WIN32
    #define szMAPIDll "mapi32.dll"
#else
    #define szMAPIDll "mapi.dll"
#endif

    /*
     * MAPI is going to export a function that is doing the same thing as this one.
     * So if the function is present we'll use it.
     */
    hMAPIDll = GetModuleHandleA(szMAPIDll);
    if(hMAPIDll)
    {
        lpfnMAPIConvIdx = (MAPICONVIDX)GetProcAddress(hMAPIDll,
                                            szScCreateConversationIndex);
        if(lpfnMAPIConvIdx)
        {
            return (*lpfnMAPIConvIdx)(cbParent, lpbParent, lpcb, lppb);
        }
    }
    //  Ensure that the parent is what we think
    //  it should be
    //
    if ((cbParent < cbConvIndexHdr) ||
        ((cbParent - cbConvIndexHdr) % cbConvIndexComponent) ||
        (lpbParent[0] != bConvIndexRes))
    {
        cbParent = 0;
        *lpcb = cbConvIndexHdr;
    }
    else
        *lpcb = cbParent + cbConvIndexComponent;

    sc = MAPIAllocateBuffer (*lpcb, (LPVOID FAR *)lppb);
    if (!FAILED (sc))
    {
        if (cbParent == 0)
        {
            //  This is a new key, so all it ever contains
            //  is a header.  Fill it in and we are done
            //
            sc = ScFillConvHeader (*lppb, *lpcb);
            if (FAILED (sc))
            {
                MAPIFreeBuffer (*lppb);
                *lppb = NULL;
            }
        }
        else
        {
            //  First copy the old key across
            //
            CopyMemory (*lppb, lpbParent, (UINT)cbParent);

            //  (Cb).  First get the current time (we'll then get
            //  the absolute distance between the current time and
            //  the time in the last chunk)
            //
            GetSystemTime (&st);
            SystemTimeToFileTime (&st, &ft);

            //  Now get the time of the last chunk
            //  
            ExtractLastFileTime (lpbParent, cbParent, &ftLast);

            //  Now mask out the bits we don't want from the
            //  current time
            //
            ft.dwHighDateTime &= 0x00FFFFFF;
            ft.dwLowDateTime &= 0xFFFF0000;

            //  This assert is here to catch how often the
            //  5-byte time can collide and under what scenario,
            //  to see if 5 bytes + the next byte suffices to
            //  make this child chunk unique.
            //  
            Assert (!((ftLast.dwHighDateTime == ft.dwHighDateTime) &&
                (ftLast.dwLowDateTime == ft.dwLowDateTime)));

            //  Get the change in time
            //
            if ((ft.dwHighDateTime > ftLast.dwHighDateTime) ||
                ((ft.dwHighDateTime == ftLast.dwHighDateTime) &&
                 (ft.dwLowDateTime > ftLast.dwLowDateTime)))
            {
                ftDelta = FtSubFt (ft, ftLast);
            }
            else
                ftDelta = FtSubFt (ftLast, ft);

            //  If the delta is less than 1.7 yrs, use 0
            //
            if (!(ftDelta.dwHighDateTime & 0x00FE0000))
            {
                //  Just mask out the 31 bits that we
                //  want from the ftDelta
                //
                dwTemp = ((DWORD)(ftDelta.dwHighDateTime & 0x0001FFFF)) << 14 |
                         ((DWORD)(ftDelta.dwLowDateTime & 0xFFFC0000)) >> 18;

                //  Only the first byte is different
                //
                (*lppb)[cbParent] = (BYTE)((dwTemp & 0xFF000000) >> 24 );
            }
            else
            {
                //  Just mask out the 31 bits that we
                //  want from the ftDelta
                //
                dwTemp = ((DWORD)(ftDelta.dwHighDateTime & 0x003FFFFF)) << 9 |
                         ((DWORD)(ftDelta.dwLowDateTime & 0xFF800000)) >> 23;

                // Only the first byte is different
                //
                (*lppb)[cbParent] = (BYTE)(HIBYTE(HIWORD(dwTemp)) | 0x080);
            }

            //  The remaining delta bytes are the same
            //
            (*lppb)[cbParent + 1] = (BYTE) ((dwTemp & 0x00FF0000) >> 16);
            (*lppb)[cbParent + 2] = (BYTE) ((dwTemp & 0x0000FF00) >> 8);
            (*lppb)[cbParent + 3] = (BYTE) ((dwTemp & 0x000000FF) );

            //  (Cc). Next get the random number
            //  (Cd). Next get the sequence count
            //  -- we are going to use part of the tick count
            //
            (*lppb)[cbParent + 4] = (BYTE) (GetTickCount() & 0x000000FF);
        }
    }

    //DebugTraceSc (ScAddConversationIndex(), sc);
    return sc;
}

