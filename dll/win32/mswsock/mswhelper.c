
#include "precomp.h"

#include <winuser.h>
#include <winnls.h>
#include <wchar.h>
#include <sal.h>

#include "mswhelper.h"

#define MSW_BUFSIZE 512
#define MAX_ARRAY_SIZE 5

void
mswBufferInit(_Inout_ PMSW_BUFFER mswBuf,
              _In_ BYTE* buffer,
              _In_ DWORD bufferSize)
{
    RtlZeroMemory(mswBuf, sizeof(*mswBuf));
    RtlZeroMemory(buffer, bufferSize);
    mswBuf->bytesMax = bufferSize;
    mswBuf->buffer = buffer;
    mswBuf->bufendptr = buffer;
    mswBuf->bufok = TRUE;
}

BOOL
mswBufferCheck(_Inout_ PMSW_BUFFER mswBuf,
               _In_ DWORD count)
{
    if (mswBuf->bytesUsed + count <= mswBuf->bytesMax)
        return TRUE;

    mswBuf->bufok = FALSE;
    return FALSE;
}

BOOL
mswBufferIncUsed(_Inout_ PMSW_BUFFER mswBuf,
                 _In_ DWORD count)
{
    if (!mswBufferCheck(mswBuf, count))
        return FALSE;

    mswBuf->bytesUsed += count;
    mswBuf->bufendptr += count;
    return TRUE;
}

BYTE*
mswBufferEndPtr(_Inout_ PMSW_BUFFER mswBuf)
{
    return mswBuf->bufendptr;
}

BOOL
mswBufferAppend(_Inout_ PMSW_BUFFER mswBuf,
                _In_ void *dataToAppend,
                _In_ DWORD dataSize)
{
    if (!mswBufferCheck(mswBuf, dataSize))
        return FALSE;

    RtlCopyMemory(mswBuf->bufendptr, dataToAppend, dataSize);
    mswBuf->bytesUsed += dataSize;
    mswBuf->bufendptr += dataSize;

    return TRUE;
}

BOOL mswBufferAppendStrA(_Inout_ PMSW_BUFFER mswBuf,
                         _In_ char* str)
{
    return mswBufferAppend(mswBuf, str, strlen(str) + sizeof(char));
}

BOOL
mswBufferAppendStrW(_Inout_ PMSW_BUFFER mswBuf,
                    _In_ WCHAR* str)
{
    int bytelen = (wcslen(str) + 1) * sizeof(WCHAR);
    return mswBufferAppend(mswBuf, str, bytelen);
}

BOOL
mswBufferAppendPtr(_Inout_ PMSW_BUFFER mswBuf,
                   _In_ void* ptr)
{
    return mswBufferAppend(mswBuf, &ptr, sizeof(ptr));
}

/* lst = pointer to pointer of items

   *lst[0] = 1st item
   *lst[1] = 2nd item
   ...
   lst[n] = NULL = End of List

   itemLength = data in Bytes for each item.

   ptrofs = delta relative to mswBuf.buffer
*/
BOOL
mswBufferAppendLst(_Inout_ PMSW_BUFFER mswBuf,
                   _In_ void **lst,
                   _In_ DWORD itemByteLength,
                   _In_opt_ int ptrofs)
{
    DWORD lstItemCount;
    DWORD lstByteSize;
    DWORD_PTR lstDataPos;
    DWORD i1;
    UINT_PTR *ptrSrcLstPos;

    /* calculate size of list */
    ptrSrcLstPos = (UINT_PTR*)lst;
    lstItemCount = 0;
    while (*ptrSrcLstPos != (UINT_PTR)NULL)
    {
        lstItemCount++;
        ptrSrcLstPos++;
    }

    lstByteSize = ((lstItemCount + 1) * sizeof(UINT_PTR)) + /* item-pointer + null-ptr (for end) */
                  (lstItemCount * itemByteLength); /* item-data */

    if (mswBuf->bytesUsed + lstByteSize > mswBuf->bytesMax)
        return FALSE;

    /* calculate position for the data of the first item */
    lstDataPos = ((lstItemCount + 1) * sizeof(UINT_PTR)) +
                 (DWORD_PTR)mswBufferEndPtr(mswBuf);
    /* add ptrofs */
    lstDataPos += ptrofs;

    /* write array of Pointer to data */
    for (i1 = 0; i1 < lstItemCount; i1++)
    {
        if (!mswBufferAppendPtr(mswBuf, (void*)lstDataPos))
            return FALSE;

        lstDataPos += sizeof(UINT_PTR);
    }

    /* end of list */
    if (!mswBufferAppendPtr(mswBuf, NULL))
        return FALSE;

    /* write data */
    ptrSrcLstPos = (UINT_PTR*)lst;
    for (i1 = 0; i1 < lstItemCount; i1++)
    {
        mswBufferAppend(mswBuf, *(BYTE**)ptrSrcLstPos, itemByteLength);
        ptrSrcLstPos++;
    }
    return mswBuf->bufok;
}

BOOL
mswBufferAppendStrLstA(_Inout_ PMSW_BUFFER mswBuf,
                       _In_ void **lst,
                       _In_opt_ int ptrofs)
{
    DWORD lstItemLen[MAX_ARRAY_SIZE];
    DWORD lstItemCount;
    DWORD lstByteSize;
    DWORD_PTR lstDataPos;
    DWORD lstDataSize;
    DWORD i1;
    UINT_PTR *ptrSrcLstPos;

    /* calculate size of list */
    ptrSrcLstPos = (UINT_PTR*)lst;
    lstItemCount = 0;
    lstDataSize = 0;

    while (*ptrSrcLstPos != (UINT_PTR)NULL)
    {
        if (lstItemCount >= MAX_ARRAY_SIZE)
            return FALSE;

        i1 = strlen((char*)*ptrSrcLstPos) + sizeof(char);
        lstItemLen[lstItemCount] = i1;
        lstItemCount++;
        lstDataSize += i1;
        ptrSrcLstPos++;
    }

    lstByteSize = ((lstItemCount + 1) * sizeof(UINT_PTR)) + /* item-pointer + null-ptr (for end) */
                  lstDataSize; /* item-data */

    if (mswBuf->bytesUsed + lstByteSize > mswBuf->bytesMax)
        return FALSE;

    /* calculate position for the data of the first item */
    lstDataPos = ((lstItemCount + 1) * sizeof(UINT_PTR)) +
                 (DWORD_PTR)mswBufferEndPtr(mswBuf);

    /* add ptrofs */
    lstDataPos += ptrofs;

    for (i1 = 0; i1 < lstItemCount; i1++)
    {
        if (!mswBufferAppendPtr(mswBuf, (void*)lstDataPos))
            return FALSE;

        lstDataPos += lstItemLen[i1];
    }

    /* end of list */
    if (!mswBufferAppendPtr(mswBuf, NULL))
        return FALSE;

    /* write data */
    ptrSrcLstPos = (UINT_PTR*)lst;
    for (i1 = 0; i1 < lstItemCount; i1++)
    {
        if (!mswBufferAppendStrA(mswBuf, *(char**)ptrSrcLstPos))
            return FALSE;

        ptrSrcLstPos++;
    }
    return mswBuf->bufok;
}

BOOL
mswBufferAppendBlob_Hostent(_Inout_ PMSW_BUFFER mswBuf,
                            _Inout_ LPWSAQUERYSETW lpRes,
                            _In_ char** hostAliasesA,
                            _In_ char* hostnameA,
                            _In_ DWORD ip4addr)
{
    PHOSTENT phe;
    void* lst[2];
    BYTE* bytesOfs;

    /* blob */
    lpRes->lpBlob = (LPBLOB)mswBufferEndPtr(mswBuf);

    if (!mswBufferIncUsed(mswBuf, sizeof(*lpRes->lpBlob)))
        return FALSE;

    /* cbSize will be set later */
    lpRes->lpBlob->cbSize = 0;
    lpRes->lpBlob->pBlobData = mswBufferEndPtr(mswBuf);

    /* hostent */
    phe = (PHOSTENT)lpRes->lpBlob->pBlobData;
    bytesOfs = mswBufferEndPtr(mswBuf);

    if (!mswBufferIncUsed(mswBuf, sizeof(*phe)))
        return FALSE;

    phe->h_addrtype = AF_INET;
    phe->h_length = 4; /* 4 Byte (IPv4) */

    /* aliases */
    phe->h_aliases = (char**)(mswBufferEndPtr(mswBuf) - bytesOfs);

    if (hostAliasesA)
    {
        if (!mswBufferAppendStrLstA(mswBuf,
                                    (void**)hostAliasesA,
                                    -(LONG_PTR)bytesOfs))
            return FALSE;
    }
    else
    {
        if (!mswBufferAppendPtr(mswBuf, NULL))
            return FALSE;
    }

    /* addr_list */
    RtlZeroMemory(lst, sizeof(lst));

    lst[0] = (void*)&ip4addr;

    phe->h_addr_list = (char**)(mswBufferEndPtr(mswBuf) - bytesOfs);

    if (!mswBufferAppendLst(mswBuf, lst, 4, -(LONG_PTR)bytesOfs))
        return FALSE;

    /* name */
    phe->h_name = (char*)(mswBufferEndPtr(mswBuf) - bytesOfs);

    if (!mswBufferAppendStrA(mswBuf, hostnameA))
        return FALSE;

    lpRes->lpBlob->cbSize = (DWORD)(mswBufferEndPtr(mswBuf) - bytesOfs);
    return mswBuf->bufok;
}

BOOL
mswBufferAppendBlob_Servent(_Inout_ PMSW_BUFFER mswBuf,
                            _Inout_ LPWSAQUERYSETW lpRes,
                            _In_ char* serviceNameA,
                            _In_ char** serviceAliasesA,
                            _In_ char* protocolNameA,
                            _In_ WORD port)
{
    PSERVENT pse;
    BYTE* bytesOfs;

    /* blob */
    lpRes->lpBlob = (LPBLOB)mswBufferEndPtr(mswBuf);

    if (!mswBufferIncUsed(mswBuf, sizeof(*lpRes->lpBlob)))
        return FALSE;

    lpRes->lpBlob->cbSize = 0;//later
    lpRes->lpBlob->pBlobData = mswBufferEndPtr(mswBuf);

    /* servent */
    pse = (LPSERVENT)lpRes->lpBlob->pBlobData;
    bytesOfs = mswBufferEndPtr(mswBuf);

    if (!mswBufferIncUsed(mswBuf, sizeof(*pse)))
        return FALSE;

    pse->s_aliases = (char**)(mswBufferEndPtr(mswBuf) - bytesOfs);

    if (serviceAliasesA)
    {
        if (!mswBufferAppendStrLstA(mswBuf,
                                    (void**)serviceAliasesA,
                                    -(LONG_PTR)bytesOfs))
            return FALSE;
    }
    else
    {
        if (!mswBufferAppendPtr(mswBuf, NULL))
            return FALSE;
    }

    pse->s_name = (char*)(mswBufferEndPtr(mswBuf) - bytesOfs);

    if (!mswBufferAppendStrA(mswBuf, serviceNameA))
        return FALSE;

    pse->s_port = htons(port);

    pse->s_proto = (char*)(mswBufferEndPtr(mswBuf) - bytesOfs);

    if (!mswBufferAppendStrA(mswBuf, protocolNameA))
        return FALSE;

    lpRes->lpBlob->cbSize = (DWORD)(mswBufferEndPtr(mswBuf) - bytesOfs);
    return mswBuf->bufok;
}

BOOL
mswBufferAppendAddr_AddrInfoW(_Inout_ PMSW_BUFFER mswBuf,
                              _Inout_ LPWSAQUERYSETW lpRes,
                              _In_ DWORD ip4addr)
{
    LPCSADDR_INFO paddrinfo;
    LPSOCKADDR_IN psa;

    lpRes->dwNumberOfCsAddrs = 1;
    lpRes->lpcsaBuffer = (LPCSADDR_INFO)mswBufferEndPtr(mswBuf);

    paddrinfo = lpRes->lpcsaBuffer;

    if (!mswBufferIncUsed(mswBuf, sizeof(*paddrinfo)))
        return FALSE;

    paddrinfo->LocalAddr.lpSockaddr = (LPSOCKADDR)mswBufferEndPtr(mswBuf);

    if (!mswBufferIncUsed(mswBuf, sizeof(*paddrinfo->LocalAddr.lpSockaddr)))
        return FALSE;

    paddrinfo->RemoteAddr.lpSockaddr = (LPSOCKADDR)mswBufferEndPtr(mswBuf);

    if (!mswBufferIncUsed(mswBuf, sizeof(*paddrinfo->RemoteAddr.lpSockaddr)))
        return FALSE;

    paddrinfo->iSocketType = SOCK_DGRAM;
    paddrinfo->iProtocol = IPPROTO_UDP;

    psa = (LPSOCKADDR_IN)paddrinfo->LocalAddr.lpSockaddr;
    paddrinfo->LocalAddr.iSockaddrLength = sizeof(*psa);
    psa->sin_family = AF_INET;
    psa->sin_port = 0;
    psa->sin_addr.s_addr = 0;
    RtlZeroMemory(psa->sin_zero, sizeof(psa->sin_zero));

    psa = (LPSOCKADDR_IN)paddrinfo->RemoteAddr.lpSockaddr;
    paddrinfo->RemoteAddr.iSockaddrLength = sizeof(*psa);
    psa->sin_family = AF_INET;
    psa->sin_port = 0;
    psa->sin_addr.s_addr = ip4addr;
    RtlZeroMemory(psa->sin_zero, sizeof(psa->sin_zero));

    return TRUE;
}

/* ansicode <-> unicode */
WCHAR*
StrA2WHeapAlloc(_In_opt_ HANDLE hHeap,
                _In_ char* aStr)
{
    int aStrByteLen;
    int wStrByteLen;
    int charLen;
    int ret;
    WCHAR* wStr;

    if (aStr == NULL)
        return NULL;

    charLen = strlen(aStr) + 1;

    aStrByteLen = (charLen * sizeof(char));
    wStrByteLen = (charLen * sizeof(WCHAR));

    if (hHeap == 0)
        hHeap = GetProcessHeap();

    wStr = HeapAlloc(hHeap, 0, wStrByteLen);
    if (wStr == NULL)
    {
        HeapFree(hHeap, 0, wStr);
        return NULL;
    }

    ret = MultiByteToWideChar(CP_ACP,
                              0,
                              aStr,
                              aStrByteLen,
                              wStr,
                              charLen);

    if (ret != charLen)
    {
        HeapFree(hHeap, 0, wStr);
        return NULL;
    }
    return wStr;
}

char*
StrW2AHeapAlloc(_In_opt_ HANDLE hHeap,
                _In_ WCHAR* wStr)
{
    int charLen;
    int aStrByteLen;
    int ret;
    char* aStr;

    if (wStr == NULL)
        return NULL;

    charLen = wcslen(wStr) + 1;

    aStrByteLen = (charLen * sizeof(char));

    if (hHeap == 0)
        hHeap = GetProcessHeap();

    aStr = HeapAlloc(hHeap, 0, aStrByteLen);
    if (aStr == NULL)
    {
        HeapFree(hHeap, 0, aStr);
        return NULL;
    }

    ret = WideCharToMultiByte(CP_ACP,
                              0,
                              wStr,
                              charLen,
                              aStr,
                              aStrByteLen,
                              NULL,
                              NULL);
    if (ret != aStrByteLen)
    {
        HeapFree(hHeap, 0, aStr);
        return NULL;
    }
    return aStr;
}

WCHAR*
StrCpyHeapAllocW(_In_opt_ HANDLE hHeap,
                 _In_ WCHAR* wStr)
{
    size_t chLen;
    size_t bLen;
    WCHAR* resW;

    if (wStr == NULL)
        return NULL;

    if (hHeap == 0)
        hHeap = GetProcessHeap();

    chLen = wcslen(wStr);

    bLen = (chLen + 1) * sizeof(WCHAR);

    resW = HeapAlloc(hHeap, 0, bLen);
    RtlCopyMemory(resW, wStr, bLen);
    return resW;
}

char*
StrCpyHeapAllocA(_In_opt_ HANDLE hHeap,
                 _In_ char* aStr)
{
    size_t chLen;
    size_t bLen;
    char* resA;

    if (aStr == NULL)
        return NULL;

    if (hHeap == 0)
        hHeap = GetProcessHeap();

    chLen = strlen(aStr);

    bLen = (chLen + 1) * sizeof(char);

    resA = HeapAlloc(hHeap, 0, bLen);
    RtlCopyMemory(resA, aStr, bLen);
    return resA;
}

char**
StrAryCpyHeapAllocA(_In_opt_ HANDLE hHeap,
                    _In_ char** aStrAry)
{
    char** aSrcPtr;
    char** aDstPtr;
    char* aDstNextStr;
    DWORD aStrByteLen[MAX_ARRAY_SIZE];
    size_t bLen;
    size_t bItmLen;
    int aCount;
    int i1;
    char** resA;

    if (hHeap == 0)
        hHeap = GetProcessHeap();

    /* Calculating size of array ... */
    aSrcPtr = aStrAry;
    bLen = 0;
    aCount = 0;

    while (*aSrcPtr != NULL)
    {
        if (aCount >= MAX_ARRAY_SIZE)
            return NULL;

        bItmLen = strlen(*aSrcPtr) + 1;
        aStrByteLen[aCount] = bItmLen;

        bLen += sizeof(*aSrcPtr) + bItmLen;

        aSrcPtr++;
        aCount++;
    }

    /* size for NULL-terminator */
    bLen += sizeof(*aSrcPtr);

    /* get memory */
    resA = HeapAlloc(hHeap, 0, bLen);

    /* copy data */
    aSrcPtr = aStrAry;
    aDstPtr = resA;

    /* pos for the first string */
    aDstNextStr = (char*)(resA + aCount + 1);
    for (i1 = 0; i1 < aCount; i1++)
    {
        bItmLen = aStrByteLen[i1];

        *aDstPtr = aDstNextStr;
        RtlCopyMemory(*aDstPtr, *aSrcPtr, bItmLen);

        aDstNextStr = (char*)((DWORD_PTR)aDstNextStr + (DWORD)bItmLen);
        aDstPtr++;
        aSrcPtr++;
    }

    /* terminate with NULL */
    *aDstPtr = NULL;

    return resA;
}

char**
StrAryCpyHeapAllocWToA(_In_opt_ HANDLE hHeap,
    _In_ WCHAR** wStrAry)
{
    WCHAR** wSrcPtr;
    char** aDstPtr;
    char* aDstNextStr;
    DWORD aStrByteLen[MAX_ARRAY_SIZE];
    int bLen;
    int bItmLen;
    int aCount;
    int i1;
    char** resA;
    int ret;
    char* aStr;

    if (hHeap == 0)
        hHeap = GetProcessHeap();

    /* Calculating size of array ... */
    wSrcPtr = wStrAry;
    bLen = 0;
    aCount = 0;

    while (*wSrcPtr != NULL)
    {
        if (aCount >= MAX_ARRAY_SIZE)
            return NULL;

        bItmLen = wcslen(*wSrcPtr) + 1;
        aStrByteLen[aCount] = bItmLen;

        bLen += sizeof(*wSrcPtr) + bItmLen;

        wSrcPtr++;
        aCount++;
    }

    /* size for NULL-terminator */
    bLen += sizeof(*wSrcPtr);

    /* get memory */
    resA = HeapAlloc(hHeap, 0, bLen);

    /* copy data */
    wSrcPtr = wStrAry;
    aDstPtr = resA;

    /* pos for the first string */
    aDstNextStr = (char*)(resA + aCount + 1);
    for (i1 = 0; i1 < aCount; i1++)
    {
        bItmLen = aStrByteLen[i1];

        *aDstPtr = aDstNextStr;

        aStr = HeapAlloc(hHeap, 0, bItmLen);
        if (aStr == NULL)
        {
            HeapFree(hHeap, 0, aStr);
            return NULL;
        }

        ret = WideCharToMultiByte(CP_ACP,
                                  0,
                                  *wSrcPtr,
                                  bItmLen,
                                  aStr,
                                  bItmLen,
                                  NULL,
                                  NULL);
        if (ret != bItmLen)
        {
            HeapFree(hHeap, 0, aStr);
            return NULL;
        }
        RtlCopyMemory(*aDstPtr, aStr, bItmLen);
        HeapFree(hHeap, 0, aStr);

        aDstNextStr = (char*)((DWORD_PTR)aDstNextStr + (DWORD)bItmLen);
        aDstPtr++;
        wSrcPtr++;
    }

    /* terminate with NULL */
    *aDstPtr = NULL;

    return resA;
}
