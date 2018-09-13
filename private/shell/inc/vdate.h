//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: vdate.h - Debug argument validation helpers
//
// History:
//  06-16-94    Davepl  Created
//
//---------------------------------------------------------------------------

#ifdef DEBUG

__inline void FUNC_VDATEINPUTBUF(void * pBuffer,
                                 size_t cElementSize,
                                 size_t cCount,
                                 int    iLine,
                                 char * pszFile)
{
    if (IsBadWritePtr(pBuffer, cElementSize * cCount))
    {
        char sazOutput[MAX_PATH * 2];
        wnsprintfA(sazOutput, SIZECHARS(sazOutput), "Buffer failed validation at line %d in %s\n", iLine, pszFile);
        OutputDebugStringA(sazOutput);
        RIP(FALSE);
    }
}

#define VDATEINPUTBUF(ptr, type, count) FUNC_VDATEINPUTBUF(ptr,                 \
                                                           sizeof(type),        \
                                                           count,               \
                                                           __LINE__,            \
                                                           __FILE__)

#else

#define VDATEINPUTBUF(ptr, type, const)

#endif
