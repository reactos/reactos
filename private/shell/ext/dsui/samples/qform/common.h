#ifndef __common_h
#define __common_h


/*-----------------------------------------------------------------------------
/ Exit macros for macro
/   - these assume that a label "exit_gracefully:" prefixes the prolog
/     to your function
/----------------------------------------------------------------------------*/

#if !DSUI_DEBUG

#define ExitGracefully(hr, result, text)            \
            {  hr = result; goto exit_gracefully; }

#define FailGracefully(hr, text)                    \
	    { if ( FAILED(hr) ) { goto exit_gracefully; } }

#else

#define ExitGracefully(hr, result, text)            \
            { OutputDebugString(TEXT(text)); hr = result; goto exit_gracefully; }

#define FailGracefully(hr, text)                    \
	    { if ( FAILED(hr) ) { OutputDebugString(TEXT(text)); goto exit_gracefully; } }

#endif

/*-----------------------------------------------------------------------------
/ Interface helper macros
/----------------------------------------------------------------------------*/
#define DoRelease(pInterface)                       \
        { if ( pInterface ) { pInterface->Release(); pInterface = NULL; } }


/*-----------------------------------------------------------------------------
/ String helper macros
/----------------------------------------------------------------------------*/
#define StringByteCopy(pDest, iOffset, sz)          \
        { memcpy(&(((LPBYTE)pDest)[iOffset]), sz, StringByteSize(sz)); }
       
#define StringByteSize(sz)                          \
        ((lstrlen(sz)+1)*sizeof(TCHAR))


/*-----------------------------------------------------------------------------
/ Other helpful macros
/----------------------------------------------------------------------------*/
#define ByteOffset(base, offset)                    \
        (((LPBYTE)base)+offset)

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif


#endif
