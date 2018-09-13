/*++

    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.

Module Name:

    qshelpr.cpp

Abstract:

    Helper functions for managing the WSAQUERYSET data structure.  The external
    entry points exported by this module are the following:

    WSAComputeQuerySetSizeA()
    WSAComputeQuerySetSizeW()
    WSABuildQuerySetBufferA()
    WSABuildQuerySetBufferW()

Author:

    Paul Drews (drewsxpa@ashland.intel.com) 11-Jan-1996

Revision History:

    most-recent-revision-date email-name
        description

    11-Jan-1996  drewsxpa@ashland.intel.com
        created

--*/


#include "precomp.h"




static
INT
ComputeAddrInfoSize(
    IN LPCSADDR_INFO  lpAddr
    )
/*++
Routine Description:

    This  procedure  computes  the required size, in bytes, of a buffer to hold
    the indicated CSADDR_INFO structure if it were packed into a single buffer.

Arguments:

    lpAddr - Supplies the CSADDR_INFO structure.  This structure in turn may be
             organized  as  separately-allocated  pieces  or as a single packed
             buffer.

Return Value:

    The function returns the required size, in bytes, of the packed buffer.

Implementation:

    size_so_far = sizeof(CSADDR_INFO);
    for each <lp_indirect_thing> loop
        if (lp_indirect_thing != NULL) then
            size_so_far += Compute<indirect_thing>Size(lp_indirect_thing);
        endif
    end loop
--*/
{
    INT size_so_far;

    size_so_far = sizeof(CSADDR_INFO);

    // SOCKET_ADDRESS LocalAddr;
    if (lpAddr->LocalAddr.lpSockaddr != NULL) {
        size_so_far += lpAddr->LocalAddr.iSockaddrLength + (sizeof(DWORD_PTR) -1);
    }

    // SOCKET_ADDRESS RemoteAddr;
    if (lpAddr->RemoteAddr.lpSockaddr != NULL) {
        size_so_far += lpAddr->RemoteAddr.iSockaddrLength + (sizeof(DWORD_PTR) -1);
    }

    // INT iSocketType;
    // no further action required

    // INT iProtocol;
    // no further action required

    return(size_so_far);

} // ComputeAddrInfoSize




static
INT
ComputeAddrInfoArraySize(
    IN DWORD          dwNumAddrs,
    IN LPCSADDR_INFO  lpAddrBuf
    )
/*++
Routine Description:

    This  procedure  computes  the required size, in bytes, of a buffer to hold
    the  indicated  array  of  CSADDR_INFO  structures if it were packed into a
    single buffer.

Arguments:

    dwNumAddrs - Supplies the number of CSADDR_INFO structures in the array.

    lpAddrBuf  - Supplies   the   array   of   CSADDR_INFO  structures.   These
                 structures  in  turn  may be organized as separately-allocated
                 pieces or as a single packed buffer.

Return Value:

    The function returns the required size, in bytes, of the packed buffer.

--*/
{
    INT size_so_far;
    DWORD i;

    size_so_far = 0;
    for (i = 0; i < dwNumAddrs; i++) {
        size_so_far += ComputeAddrInfoSize(
            & lpAddrBuf[i]);
    }

    return(size_so_far);

} // ComputeAddrInfoArraySize




static
INT
ComputeBlobSize(
    IN LPBLOB  lpBlob
    )
/*++
Routine Description:

    This  procedure  computes  the required size, in bytes, of a buffer to hold
    the indicated BLOB structure if it were packed into a single buffer.

Arguments:

    lpBlob  - Supplies  the  BLOB  structure.   This  structure  in turn may be
              organized  as  separately-allocated  pieces or as a single packed
              buffer.

Return Value:

    The function returns the required size, in bytes, of the packed buffer.

--*/
{
    INT size_so_far;

    size_so_far = sizeof(BLOB);
    if (lpBlob->pBlobData != NULL) {
        size_so_far += lpBlob->cbSize;
    }

    return(size_so_far);

} // ComputeBlobSize




INT
WSAAPI
WSAComputeQuerySetSizeA(
    IN LPWSAQUERYSETA lpSourceQuerySet
    )
/*++
Routine Description:

    This  procedure  computes  the required size, in bytes, of a buffer to hold
    the indicated lpSourceQuerySet if it were packed into a single buffer.

    Note that on many items, there is additional added value. This is
    the maximum "fill" that will result in coercing this into
    another structure. Since the repacking does not do so in
    any particular, it is possible address alignment will require
    fill.

Arguments:

    lpSourceQuerySet - Supplies  the query set for which the packed-buffer size
                       should  be  computed.   The  supplied  query  set may be
                       organized  as separately-allocated pieces or as a single
                       packed buffer.

Return Value:

    The function returns the required size, in bytes, of the packed buffer.

Implementation Notes:

    size_so_far = sizeof(WSAQUERYSETA);
    for each <lp_indirect_thing> loop
        if (lp_indirect_thing != NULL) then
            size_so_far += Compute<indirect_thing>SizeA(lp_indirect_thing);
        endif
    end loop
--*/
{
    INT size_so_far;

    size_so_far = sizeof(WSAQUERYSETA);

    // DWORD           dwSize;
    // no further action required

    // LPSTR            lpszServiceInstanceName;
    if (lpSourceQuerySet->lpszServiceInstanceName != NULL) {
        size_so_far += lstrlen(lpSourceQuerySet->lpszServiceInstanceName)
            + sizeof(DWORD_PTR);
    }

    // LPGUID          lpServiceClassId;
    if (lpSourceQuerySet->lpServiceClassId != NULL) {
        size_so_far += sizeof(GUID) + (sizeof(DWORD_PTR) -1);
    }

    // LPWSAVERSION      lpVersion;
    if (lpSourceQuerySet->lpVersion != NULL) {
        size_so_far += sizeof(WSAVERSION) + (sizeof(DWORD_PTR) -1);
    }

    // LPSTR             lpszComment;
    if (lpSourceQuerySet->lpszComment != NULL) {
        size_so_far += lstrlen(lpSourceQuerySet->lpszComment)
            + sizeof(DWORD_PTR);
    }

    // DWORD           dwNameSpace;
    // no further action required

    // LPGUID          lpNSProviderId;
    if (lpSourceQuerySet->lpNSProviderId != NULL) {
        size_so_far += sizeof(GUID) + (sizeof(DWORD_PTR) -1);
    }

    // LPSTR             lpszContext;
    if (lpSourceQuerySet->lpszContext != NULL) {
        size_so_far += lstrlen(lpSourceQuerySet->lpszContext)
            + sizeof(DWORD_PTR);
    }

    // LPSTR             lpszQueryString;
    if (lpSourceQuerySet->lpszQueryString != NULL) {
        size_so_far += lstrlen(lpSourceQuerySet->lpszQueryString)
            + sizeof(DWORD_PTR);
    }

    // DWORD           dwNumberOfProtocols;
    // no further action required

    // LPAFPROTOCOLS   lpafpProtocols;
    if (lpSourceQuerySet->lpafpProtocols != NULL) {
        size_so_far += sizeof(AFPROTOCOLS) *
            lpSourceQuerySet->dwNumberOfProtocols + (sizeof(DWORD_PTR) -1);
    }

    // DWORD           dwNumberOfCsAddrs;
    // no further action required

    // LPCSADDR_INFO   lpcsaBuffer;
    if (lpSourceQuerySet->lpcsaBuffer != NULL) {
        size_so_far += ComputeAddrInfoArraySize(
            lpSourceQuerySet->dwNumberOfCsAddrs,   // dwNumAddrs
            lpSourceQuerySet->lpcsaBuffer) + (sizeof(DWORD_PTR) -1);        // lpAddrBuf
    }

    // LPBLOB          lpBlob;
    if (lpSourceQuerySet->lpBlob != NULL) {
        size_so_far += ComputeBlobSize(
            lpSourceQuerySet->lpBlob) + (sizeof(DWORD_PTR) -1);
    }

    return(size_so_far);

} // WSAComputeQuerySetSizeA




INT
WSAAPI
WSAComputeQuerySetSizeW(
    IN LPWSAQUERYSETW lpSourceQuerySet
    )
/*++
Routine Description:

    This  procedure  computes  the required size, in bytes, of a buffer to hold
    the indicated lpSourceQuerySet if it were packed into a single buffer.

    See comment on A form of this for information about the computations
    and how address alignment fill is handled.

Arguments:

    lpSourceQuerySet - Supplies  the query set for which the packed-buffer size
                       should  be  computed.   The  supplied  query  set may be
                       organized  as separately-allocated pieces or as a single
                       packed buffer.

Return Value:

    The function returns the required size, in bytes, of the packed buffer.

Implementation Notes:

    size_so_far = sizeof(WSAQUERYSETW);
    for each <lp_indirect_thing> loop
        if (lp_indirect_thing != NULL) then
            size_so_far += Compute<indirect_thing>SizeW(lp_indirect_thing);
        endif
    end loop
--*/
{
    INT size_so_far;

    size_so_far = sizeof(WSAQUERYSETW);

    // DWORD           dwSize;
    // no further action required

    // LPWSTR            lpszServiceInstanceName;
    if (lpSourceQuerySet->lpszServiceInstanceName != NULL) {
        size_so_far += (wcslen(lpSourceQuerySet->lpszServiceInstanceName)
            + 1) * sizeof(WCHAR);
        size_so_far += (sizeof(DWORD_PTR) -1);
    }

    // LPGUID          lpServiceClassId;
    if (lpSourceQuerySet->lpServiceClassId != NULL) {
        size_so_far += sizeof(GUID) + (sizeof(DWORD_PTR) -1);
    }

    // LPWSAVERSION      lpVersion;
    if (lpSourceQuerySet->lpVersion != NULL) {
        size_so_far += sizeof(WSAVERSION) + (sizeof(DWORD_PTR) -1);
    }

    // LPWSTR             lpszComment;
    if (lpSourceQuerySet->lpszComment != NULL) {
        size_so_far += (wcslen(lpSourceQuerySet->lpszComment)
            + 1) * sizeof(WCHAR);
        size_so_far += (sizeof(DWORD_PTR) -1);
    }

    // DWORD           dwNameSpace;
    // no further action required

    // LPGUID          lpNSProviderId;
    if (lpSourceQuerySet->lpNSProviderId != NULL) {
        size_so_far += sizeof(GUID) + (sizeof(DWORD_PTR) -1);
    }

    // LPWSTR             lpszContext;
    if (lpSourceQuerySet->lpszContext != NULL) {
        size_so_far += (wcslen(lpSourceQuerySet->lpszContext)
            + 1) * sizeof(WCHAR);
        size_so_far += (sizeof(DWORD_PTR) -1);
    }

    // LPWSTR             lpszQueryString;
    if (lpSourceQuerySet->lpszQueryString != NULL) {
        size_so_far += (wcslen(lpSourceQuerySet->lpszQueryString)
            + 1) * sizeof(WCHAR);
        size_so_far += (sizeof(DWORD_PTR) -1);
    }

    // DWORD           dwNumberOfProtocols;
    // no further action required

    // LPAFPROTOCOLS   lpafpProtocols;
    if (lpSourceQuerySet->lpafpProtocols != NULL) {
        size_so_far += sizeof(AFPROTOCOLS) *
            lpSourceQuerySet->dwNumberOfProtocols + 2;
    }

    // DWORD           dwNumberOfCsAddrs;
    // no further action required

    // LPCSADDR_INFO   lpcsaBuffer;
    if (lpSourceQuerySet->lpcsaBuffer != NULL) {
        size_so_far += ComputeAddrInfoArraySize(
            lpSourceQuerySet->dwNumberOfCsAddrs,   // dwNumAddrs
            lpSourceQuerySet->lpcsaBuffer) + 2;        // lpAddrBuf
    }

    // LPBLOB          lpBlob;
    if (lpSourceQuerySet->lpBlob != NULL) {
        size_so_far += ComputeBlobSize(
            lpSourceQuerySet->lpBlob) + 2;
    }

    return(size_so_far);

} // WSAComputeQuerySetSizeW



// The  following  small class is used to manage the free space at the tail end
// of a packed WSAQUERYSET buffer as it is being built.

class SPACE_MGR {
public:
    SPACE_MGR(
        IN INT    MaxBytes,
        IN LPVOID Buf
        );

    ~SPACE_MGR(
        );

    LPVOID
    TakeSpaceBYTE(
        IN INT  NumBytes
        );

    LPVOID
    TakeSpaceWORD(
        IN INT  NumBytes
        );

    LPVOID
    TakeSpaceDWORD(
        IN INT  NumBytes
        );

    LPVOID
    TakeSpaceDWORD_PTR(
        IN INT  NumBytes
	);


private:

    LPVOID
    TakeSpace(
        IN INT  NumBytes,
        IN INT  alignment
        );

    INT    m_MaxBytes;
        // The  maximum  number  of bytes that can be used in the entire buffer
        // (i.e., the size of the buffer).

    LPVOID m_Buf;
        // Pointer to the beginning of the buffer.

    INT    m_BytesUsed;
        // The  number  of  bytes that have been allocated out of the buffer so
        // far.

}; // class SPACE_MGR

typedef SPACE_MGR * LPSPACE_MGR;


SPACE_MGR::SPACE_MGR(
    IN INT    MaxBytes,
    IN LPVOID Buf
    )
/*++
Routine Description:

    This  procedure  is the constructor for a SPACE_MGR object.  It initializes
    the object to indicate that zero bytes have so far been consumed.

Arguments:

    MaxBytes - Supplies  the  starting  number of bytes available in the entire
               buffer.

    Buf      - Supplies the pointer to the beginning of the buffer.

Return Value:

    Implictly Returns the pointer to the newly allocated SPACE_MGR object.
--*/
{
    m_MaxBytes  = MaxBytes;
    m_Buf       = Buf;
    m_BytesUsed = 0;
}  // SPACE_MGR::SPACE_MGR




SPACE_MGR::~SPACE_MGR(
    )
/*++
Routine Description:

    This procedure is the destructor for the SPACE_MGR object.  Note that it is
    the caller's responsibility to deallocate the actual buffer as appropriate.

Arguments:

    none

Return Value:

    none
--*/
{
    m_Buf = NULL;
}  // SPACE_MGR::~SPACE_MGR

inline
LPVOID
SPACE_MGR::TakeSpaceBYTE(
    IN INT  NumBytes
    )
{
    return(TakeSpace(NumBytes, 1));
}

inline
LPVOID
SPACE_MGR::TakeSpaceWORD(
    IN INT  NumBytes
    )
{
    return(TakeSpace(NumBytes, 2));
}

inline
LPVOID
SPACE_MGR::TakeSpaceDWORD(
    IN INT  NumBytes
    )
{
    return(TakeSpace(NumBytes, 4));
}

inline
LPVOID
SPACE_MGR::TakeSpaceDWORD_PTR(
    IN INT NumBytes
    )
{
    return(TakeSpace(NumBytes, sizeof(ULONG_PTR)));
}


LPVOID
SPACE_MGR::TakeSpace(
    IN INT  NumBytes,
    IN INT  align
    )
/*++
Routine Description:

    This  procedure  allocates  the  indicated number of bytes from the buffer,
    returning a pointer to the beginning of the allocated space.  The procedure
    assumes  that  the  caller  does not attempt to allocate more space than is
    available, although it does an internal consistency check.

    Pre-alignment of the buffer is made based on the value of align.

Arguments:

    NumBytes - Supplies the number of bytes to be allocated from the buffer.

Return Value:

    The procedure returns the pointer to the beginning of the allocated portion
    of the buffer.
--*/
{
    LPVOID  return_value;
    PCHAR   charbuf;

    //
    // get the aligment right first. The alignment value MUST be
    // an integral power of 2.
    //

    m_BytesUsed = (m_BytesUsed + align - 1) & ~(align - 1);

    assert((NumBytes + m_BytesUsed) <= m_MaxBytes);

    charbuf = (PCHAR) m_Buf;
    return_value = (LPVOID) & charbuf[m_BytesUsed];
    m_BytesUsed += NumBytes;

    return(return_value);

}  // SPACE_MGR::TakeSpace




static
LPWSAQUERYSETA
CopyQuerySetDirectA(
    IN OUT  LPSPACE_MGR     SpaceMgr,
    IN      LPWSAQUERYSETA  Source
    )
/*++
Routine Description:

    This  procedure copies the "direct" portion of the indicated LPWSAQUERYSETA
    structure  into  the  managed buffer.  Pointer values in the direct portion
    are  copied,  however  no  "deep" copy is done of the objects referenced by
    those pointers.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Source   - Supplies the source data to be copied.

Return Value:

    The  function returns a pointer to the beginning of the newly copied target
    values.   This  value  is  typically  used  as the "Target" in a subsequent
    CopyQuerySetIndirectA.
--*/
{
    LPWSAQUERYSETA  Target;

    Target = (LPWSAQUERYSETA) SpaceMgr->TakeSpaceDWORD_PTR(
        sizeof(WSAQUERYSETA));
    *Target = *Source;

    return(Target);

} // CopyQuerySetDirectA




LPBLOB
CopyBlobDirect(
    IN OUT LPSPACE_MGR  SpaceMgr,
    IN     LPBLOB       Source
    )
/*++
Routine Description:

    This  procedure  copies  the  "direct"  portion  of  the  indicated  LPBLOB
    structure  into  the  managed buffer.  Pointer values in the direct portion
    are  copied,  however  no  "deep" copy is done of the objects referenced by
    those pointers.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Source   - Supplies the source data to be copied.

Return Value:

    The  function returns a pointer to the beginning of the newly copied target
    values.   This  value  is  typically  used  as the "Target" in a subsequent
    CopyBlobIndirect.
--*/
{
    LPBLOB Target;

    Target = (LPBLOB) SpaceMgr->TakeSpaceDWORD_PTR(
        sizeof(BLOB));
    *Target = *Source;

    return(Target);

} // CopyBlobDirect




VOID
CopyBlobIndirect(
    IN OUT LPSPACE_MGR  SpaceMgr,
    IN OUT LPBLOB       Target,
    IN     LPBLOB       Source)
/*++
Routine Description:

    This  procedure  does  a  full-depth copy of the "indirect" portions of the
    indicated LPBLOB structure into the managed buffer.  Space for the indirect
    portions  is  allocated  from  the  managed  buffer.  Pointer values in the
    "direct"  portion  of the target LPBLOB structure are updated to point into
    the managed buffer at the correct location.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Target   - Supplies  the  starting values of the "direct" portion.  Returns
               the "direct" portion values with all pointers updated.

    Source   - Supplies the source data to be copied.

Return Value:

    none
--*/
{
    if ((Source->pBlobData != NULL) &&
        (Source->cbSize != 0)) {
        Target->pBlobData = (BYTE *) SpaceMgr->TakeSpaceDWORD_PTR(
            Source->cbSize);
        CopyMemory(
            (PVOID) Target->pBlobData,
            (PVOID) Source->pBlobData,
            Source->cbSize);
    }
    else {
        Target->pBlobData = NULL;
        // And force the buffer to be well-formed
        Target->cbSize = 0;
    }

} // CopyBlobIndirect




static
LPCSADDR_INFO
CopyAddrInfoArrayDirect(
    IN OUT LPSPACE_MGR    SpaceMgr,
    IN     DWORD          NumAddrs,
    IN     LPCSADDR_INFO  Source
    )
/*++
Routine Description:

    This  procedure  copies the "direct" portion of the indicated LPCSADDR_INFO
    array  into  the  managed buffer.  Pointer values in the direct portion are
    copied,  however  no "deep" copy is done of the objects referenced by those
    pointers.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    NumAddrs - Supplies the number of CSADDR_INFO structures in the array to be
               copied.

    Source   - Supplies the source data to be copied.

Return Value:

    The  function returns a pointer to the beginning of the newly copied target
    values.   This  value  is  typically  used  as the "Target" in a subsequent
    CopyAddrInfoArrayIndirect.
--*/
{
    LPCSADDR_INFO  Target;

    Target = (LPCSADDR_INFO) SpaceMgr->TakeSpaceDWORD_PTR(
        NumAddrs * sizeof(CSADDR_INFO));
    CopyMemory(
        (PVOID) Target,
        (PVOID) Source,
        NumAddrs * sizeof(CSADDR_INFO));

    return(Target);

} // CopyAddrInfoArrayDirect




static
VOID
CopyAddrInfoArrayIndirect(
    IN OUT LPSPACE_MGR  SpaceMgr,
    IN OUT LPCSADDR_INFO Target,
    IN     DWORD         NumAddrs,
    IN     LPCSADDR_INFO Source
    )
/*++
Routine Description:

    This  procedure  does  a  full-depth copy of the "indirect" portions of the
    indicated  LPCSADDR_INFO  array  into  the  managed  buffer.  Space for the
    indirect  portions is allocated from the managed buffer.  Pointer values in
    the "direct" portion of the target LPCSADDR_INFO array are updated to point
    into the managed buffer at the correct location.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Target   - Supplies  the  starting values of the "direct" portion.  Returns
               the "direct" portion values with all pointers updated.

    NumAddrs - Supplies the number of CSADDR_INFO structures in the array to be
               copied.

    Source   - Supplies the source data to be copied.

Return Value:

    none
--*/
{
    DWORD i;

    for (i = 0; i < NumAddrs; i++) {
        // SOCKET_ADDRESS LocalAddr ;
        if ((Source[i].LocalAddr.lpSockaddr != NULL) &&
            (Source[i].LocalAddr.iSockaddrLength != 0)) {
            Target[i].LocalAddr.lpSockaddr =
                (LPSOCKADDR) SpaceMgr->TakeSpaceDWORD_PTR(
                    Source[i].LocalAddr.iSockaddrLength);
            CopyMemory(
                (PVOID) Target[i].LocalAddr.lpSockaddr,
                (PVOID) Source[i].LocalAddr.lpSockaddr,
                Source[i].LocalAddr.iSockaddrLength);
        }
        else {
            Target[i].LocalAddr.lpSockaddr = NULL;
            // And force the buffer to be well-formed
            Target[i].LocalAddr.iSockaddrLength = 0;
        }

        // SOCKET_ADDRESS RemoteAddr ;
        if ((Source[i].RemoteAddr.lpSockaddr != NULL) &&
            (Source[i].RemoteAddr.iSockaddrLength != 0)) {
            Target[i].RemoteAddr.lpSockaddr =
                (LPSOCKADDR) SpaceMgr->TakeSpaceDWORD_PTR(
                     Source[i].RemoteAddr.iSockaddrLength);
            CopyMemory(
                (PVOID) Target[i].RemoteAddr.lpSockaddr,
                (PVOID) Source[i].RemoteAddr.lpSockaddr,
                Source[i].RemoteAddr.iSockaddrLength);
        }
        else {
            Target[i].RemoteAddr.lpSockaddr = NULL;
            // And force the buffer to be well-formed
            Target[i].RemoteAddr.iSockaddrLength = 0;
        }

        // INT iSocketType ;
        // no action required

        // INT iProtocol ;
        // no action required

    } // for i

} // CopyAddrInfoArrayIndirect




static
VOID
CopyQuerySetIndirectA(
    IN OUT  LPSPACE_MGR     SpaceMgr,
    IN OUT  LPWSAQUERYSETA  Target,
    IN      LPWSAQUERYSETA  Source
    )
/*++
Routine Description:

    This  procedure  does  a  full-depth copy of the "indirect" portions of the
    indicated  LPWSAQUERYSETA structure into the managed buffer.  Space for the
    indirect  portions is allocated from the managed buffer.  Pointer values in
    the  "direct" portion of the target LPWSAQUERYSETA structure are updated to
    point into the managed buffer at the correct location.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Target   - Supplies  the  starting values of the "direct" portion.  Returns
               the "direct" portion values with all pointers updated.

    Source   - Supplies the source data to be copied.

Return Value:

    none
--*/
{

    // DWORD           dwSize;
    // no action required

    // LPSTR            lpszServiceInstanceName;
    if (Source->lpszServiceInstanceName != NULL) {
        Target->lpszServiceInstanceName = (LPSTR) SpaceMgr->TakeSpaceBYTE(
            lstrlen(Source->lpszServiceInstanceName) + 1);
        lstrcpy(
            Target->lpszServiceInstanceName,
            Source->lpszServiceInstanceName);
    }
    else {
        Target->lpszServiceInstanceName = NULL;
    }

    // LPGUID          lpServiceClassId;
    if (Source->lpServiceClassId != NULL) {
        Target->lpServiceClassId = (LPGUID) SpaceMgr->TakeSpaceDWORD_PTR(
            sizeof(GUID));
        *(Target->lpServiceClassId) = *(Source->lpServiceClassId);
    }
    else {
        Target->lpServiceClassId = NULL;
    }

    // LPWSAVERSION      lpVersion;
    if (Source->lpVersion != NULL) {
        Target->lpVersion = (LPWSAVERSION) SpaceMgr->TakeSpaceDWORD_PTR(
            sizeof(WSAVERSION));
        *(Target->lpVersion) = *(Source->lpVersion);
    }
    else {
        Target->lpVersion = NULL;
    }

    // LPSTR             lpszComment;
    if (Source->lpszComment != NULL) {
        Target->lpszComment = (LPSTR) SpaceMgr->TakeSpaceBYTE(
            lstrlen(Source->lpszComment) + 1);
        lstrcpy(
            Target->lpszComment,
            Source->lpszComment);
    }
    else {
        Target->lpszComment = NULL;
    }

    // DWORD           dwNameSpace;
    // no action required

    // LPGUID          lpNSProviderId;
    if (Source->lpNSProviderId != NULL) {
        Target->lpNSProviderId = (LPGUID) SpaceMgr->TakeSpaceDWORD_PTR(
            sizeof(GUID));
        *(Target->lpNSProviderId) = *(Source->lpNSProviderId);
    }
    else {
        Target->lpNSProviderId = NULL;
    }

    // LPSTR             lpszContext;
    if (Source->lpszContext != NULL) {
        Target->lpszContext = (LPSTR) SpaceMgr->TakeSpaceBYTE(
            lstrlen(Source->lpszContext) + 1);
        lstrcpy(
            Target->lpszContext,
            Source->lpszContext);
    }
    else {
        Target->lpszContext = NULL;
    }

    // LPSTR             lpszQueryString;
    if (Source->lpszQueryString != NULL) {
        Target->lpszQueryString = (LPSTR) SpaceMgr->TakeSpaceBYTE(
            lstrlen(Source->lpszQueryString) + 1);
        lstrcpy(
            Target->lpszQueryString,
            Source->lpszQueryString);
    }
    else {
        Target->lpszQueryString = NULL;
    }

    // DWORD           dwNumberOfProtocols;
    // no action required

    // LPAFPROTOCOLS   lpafpProtocols;
    if ((Source->lpafpProtocols != NULL) &&
        (Source->dwNumberOfProtocols != 0)) {
        Target->lpafpProtocols = (LPAFPROTOCOLS) SpaceMgr->TakeSpaceDWORD_PTR(
            Source->dwNumberOfProtocols * sizeof(AFPROTOCOLS));
        CopyMemory (
            (PVOID) Target->lpafpProtocols,
            (PVOID) Source->lpafpProtocols,
            Source->dwNumberOfProtocols * sizeof(AFPROTOCOLS));
    }
    else {
        Target->lpafpProtocols = NULL;
        // And force the target buffer to be well-formed
        Target->dwNumberOfProtocols = 0;
    }

    // DWORD           dwNumberOfCsAddrs;
    // no action required

    // LPCSADDR_INFO   lpcsaBuffer;
    if ((Source->lpcsaBuffer != NULL) &&
        (Source->dwNumberOfCsAddrs != 0)) {
        Target->lpcsaBuffer = CopyAddrInfoArrayDirect(
            SpaceMgr,
            Source->dwNumberOfCsAddrs,
            Source->lpcsaBuffer);
        CopyAddrInfoArrayIndirect(
            SpaceMgr,
            Target->lpcsaBuffer,
            Source->dwNumberOfCsAddrs,
            Source->lpcsaBuffer);
    }
    else {
        Target->lpcsaBuffer = NULL;
        // And force the target buffer to be well-formed
        Target->dwNumberOfCsAddrs = 0;
    }

    // LPBLOB          lpBlob;
    if (Source->lpBlob != NULL) {
        Target->lpBlob = CopyBlobDirect(
            SpaceMgr,
            Source->lpBlob);
        CopyBlobIndirect(
            SpaceMgr,
            Target->lpBlob,
            Source->lpBlob);
    }
    else {
        Target->lpBlob = NULL;
    }

} // CopyQuerySetIndirectA




INT
WSAAPI
WSABuildQuerySetBufferA(
    IN  LPWSAQUERYSETA lpSourceQuerySet,
    IN  DWORD dwPackedQuerySetSize,
    OUT LPWSAQUERYSETA lpPackedQuerySet
    )
/*++
Routine Description:

    This  procedure  copies  a  source  WSAQUERYSET  into  a target WSAQUERYSET
    buffer.  The target WSAQUERYSET buffer is assembled in "packed" form.  That
    is,  all  pointers  in  the  WSAQUERYSET  are  to locations within the same
    supplied buffer.

Arguments:

    lpSourceQuerySet     - Supplies  the  source  query set to be copied to the
                           target  buffer.   The  supplied  query  set  may  be
                           organized  as  separately-allocated  pieces  or as a
                           single packed buffer.

    dwPackedQuerySetSize - Supplies the size, in bytes, of the target query set
                           buffer.

    lpPackedQuerySet     - Returns the packed copied query set.

Return Value:

    ERROR_SUCCESS - The function succeeded.

    SOCKET_ERROR  - The function failed.  A specific error code can be obtained
                    from WSAGetLastError().

Implementation Notes:

    If (target buffer is big enough) then
        space_mgr = new buffer_space_manager(...);
        start_direct = CopyQuerySetDirectA(
            space_mgr,
            (LPVOID) lpSourceQuerySet);
        CopyQuerySetIndirectA(
            space_mgr,
            start_direct,
            lpSourceQuerySet);
        delete space_mgr;
        result = ERROR_SUCCESS;
    else
        result = SOCKET_ERROR;
    endif

--*/
{
    INT          return_value;
    INT          space_required;
    BOOL         ok_to_continue;

    ok_to_continue = TRUE;

    space_required = WSAComputeQuerySetSizeA(
        lpSourceQuerySet);
    if ((DWORD) space_required > dwPackedQuerySetSize) {
        SetLastError(WSAEFAULT);
        ok_to_continue = FALSE;
    }

    SPACE_MGR    space_mgr(
        dwPackedQuerySetSize,
        lpPackedQuerySet);

    if (ok_to_continue) {
        LPWSAQUERYSETA  Target;
        Target = CopyQuerySetDirectA(
            & space_mgr,        // SpaceMgr
            lpSourceQuerySet);  // Source
        CopyQuerySetIndirectA(
            & space_mgr,        // SpaceMgr
            Target,             // Target
            lpSourceQuerySet);  // Source
    }

    if (ok_to_continue) {
        return_value = ERROR_SUCCESS;
    }
    else {
        return_value = SOCKET_ERROR;
    }
    return(return_value);

} // WSABuildQuerySetBufferA




static
LPWSAQUERYSETW
CopyQuerySetDirectW(
    IN OUT  LPSPACE_MGR     SpaceMgr,
    IN      LPWSAQUERYSETW  Source
    )
/*++
Routine Description:

    This  procedure copies the "direct" portion of the indicated LPWSAQUERYSETW
    structure  into  the  managed buffer.  Pointer values in the direct portion
    are  copied,  however  no  "deep" copy is done of the objects referenced by
    those pointers.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Source   - Supplies the source data to be copied.

Return Value:

    The  function returns a pointer to the beginning of the newly copied target
    values.   This  value  is  typically  used  as the "Target" in a subsequent
    CopyQuerySetIndirectW.
--*/
{
    LPWSAQUERYSETW  Target;

    Target = (LPWSAQUERYSETW) SpaceMgr->TakeSpaceDWORD_PTR(
        sizeof(WSAQUERYSETW));
    *Target = *Source;

    return(Target);

} // CopyQuerySetDirectW




VOID
CopyQuerySetIndirectW(
    IN OUT  LPSPACE_MGR     SpaceMgr,
    IN OUT  LPWSAQUERYSETW  Target,
    IN      LPWSAQUERYSETW  Source
    )
/*++
Routine Description:

    This  procedure  does  a  full-depth copy of the "indirect" portions of the
    indicated  LPWSAQUERYSETW structure into the managed buffer.  Space for the
    indirect  portions is allocated from the managed buffer.  Pointer values in
    the  "direct" portion of the target LPWSAQUERYSETW structure are updated to
    point into the managed buffer at the correct location.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Target   - Supplies  the  starting values of the "direct" portion.  Returns
               the "direct" portion values with all pointers updated.

    Source   - Supplies the source data to be copied.

Return Value:

    none
--*/
{

    // DWORD           dwSize;
    // no action required

    // LPWSTR            lpszServiceInstanceName;
    if (Source->lpszServiceInstanceName != NULL) {
        Target->lpszServiceInstanceName = (LPWSTR) SpaceMgr->TakeSpaceWORD(
            (wcslen(Source->lpszServiceInstanceName) + 1) * sizeof(WCHAR));
        wcscpy(
            Target->lpszServiceInstanceName,
            Source->lpszServiceInstanceName);
    }
    else {
        Target->lpszServiceInstanceName = NULL;
    }

    // LPGUID          lpServiceClassId;
    if (Source->lpServiceClassId != NULL) {
        Target->lpServiceClassId = (LPGUID) SpaceMgr->TakeSpaceDWORD_PTR(
            sizeof(GUID));
        *(Target->lpServiceClassId) = *(Source->lpServiceClassId);
    }
    else {
        Target->lpServiceClassId = NULL;
    }

    // LPWSAVERSION      lpVersion;
    if (Source->lpVersion != NULL) {
        Target->lpVersion = (LPWSAVERSION) SpaceMgr->TakeSpaceDWORD_PTR(
            sizeof(WSAVERSION));
        *(Target->lpVersion) = *(Source->lpVersion);
    }
    else {
        Target->lpVersion = NULL;
    }

    // LPWSTR             lpszComment;
    if (Source->lpszComment != NULL) {
        Target->lpszComment = (LPWSTR) SpaceMgr->TakeSpaceWORD(
            (wcslen(Source->lpszComment) + 1) * sizeof(WCHAR));
        wcscpy(
            Target->lpszComment,
            Source->lpszComment);
    }
    else {
        Target->lpszComment = NULL;
    }

    // DWORD           dwNameSpace;
    // no action required

    // LPGUID          lpNSProviderId;
    if (Source->lpNSProviderId != NULL) {
        Target->lpNSProviderId = (LPGUID) SpaceMgr->TakeSpaceDWORD_PTR(
            sizeof(GUID));
        *(Target->lpNSProviderId) = *(Source->lpNSProviderId);
    }
    else {
        Target->lpNSProviderId = NULL;
    }

    // LPWSTR             lpszContext;
    if (Source->lpszContext != NULL) {
        Target->lpszContext = (LPWSTR) SpaceMgr->TakeSpaceWORD(
            (wcslen(Source->lpszContext) + 1) * sizeof(WCHAR));
        wcscpy(
            Target->lpszContext,
            Source->lpszContext);
    }
    else {
        Target->lpszContext = NULL;
    }

    // LPWSTR             lpszQueryString;
    if (Source->lpszQueryString != NULL) {
        Target->lpszQueryString = (LPWSTR) SpaceMgr->TakeSpaceWORD(
            (wcslen(Source->lpszQueryString) + 1) * sizeof(WCHAR));
        wcscpy(
            Target->lpszQueryString,
            Source->lpszQueryString);
    }
    else {
        Target->lpszQueryString = NULL;
    }

    // DWORD           dwNumberOfProtocols;
    // no action required

    // LPAFPROTOCOLS   lpafpProtocols;
    if ((Source->lpafpProtocols != NULL) &&
        (Source->dwNumberOfProtocols != 0)) {
        Target->lpafpProtocols = (LPAFPROTOCOLS) SpaceMgr->TakeSpaceDWORD_PTR(
            Source->dwNumberOfProtocols * sizeof(AFPROTOCOLS));
        CopyMemory (
            (PVOID) Target->lpafpProtocols,
            (PVOID) Source->lpafpProtocols,
            Source->dwNumberOfProtocols * sizeof(AFPROTOCOLS));
    }
    else {
        Target->lpafpProtocols = NULL;
        // And force the target buffer to be well-formed
        Target->dwNumberOfProtocols = 0;
    }

    // DWORD           dwNumberOfCsAddrs;
    // no action required

    // LPCSADDR_INFO   lpcsaBuffer;
    if ((Source->lpcsaBuffer != NULL) &&
        (Source->dwNumberOfCsAddrs != 0)) {
        Target->lpcsaBuffer = CopyAddrInfoArrayDirect(
            SpaceMgr,
            Source->dwNumberOfCsAddrs,
            Source->lpcsaBuffer);
        CopyAddrInfoArrayIndirect(
            SpaceMgr,
            Target->lpcsaBuffer,
            Source->dwNumberOfCsAddrs,
            Source->lpcsaBuffer);
    }
    else {
        Target->lpcsaBuffer = NULL;
        // And force the target buffer to be well-formed
        Target->dwNumberOfCsAddrs = 0;
    }

    // LPBLOB          lpBlob;
    if (Source->lpBlob != NULL) {
        Target->lpBlob = CopyBlobDirect(
            SpaceMgr,
            Source->lpBlob);
        CopyBlobIndirect(
            SpaceMgr,
            Target->lpBlob,
            Source->lpBlob);
    }
    else {
        Target->lpBlob = NULL;
    }

} // CopyQuerySetIndirectW




INT
WSAAPI
WSABuildQuerySetBufferW(
    IN  LPWSAQUERYSETW lpSourceQuerySet,
    IN  DWORD dwPackedQuerySetSize,
    OUT LPWSAQUERYSETW lpPackedQuerySet
    )
/*++
Routine Description:

    This  procedure  copies  a  source  WSAQUERYSET  into  a target WSAQUERYSET
    buffer.  The target WSAQUERYSET buffer is assembled in "packed" form.  That
    is,  all  pointers  in  the  WSAQUERYSET  are  to locations within the same
    supplied buffer.

Arguments:

    lpSourceQuerySet     - Supplies  the  source  query set to be copied to the
                           target  buffer.   The  supplied  query  set  may  be
                           organized  as  separately-allocated  pieces  or as a
                           single packed buffer.

    dwPackedQuerySetSize - Supplies the size, in bytes, of the target query set
                           buffer.

    lpPackedQuerySet     - Returns the packed copied query set.

Return Value:

    ERROR_SUCCESS - The function succeeded.

    SOCKET_ERROR  - The function failed.  A specific error code can be obtained
                    from WSAGetLastError().
--*/
{
    INT          return_value;
    INT          space_required;
    BOOL         ok_to_continue;

    ok_to_continue = TRUE;

    space_required = WSAComputeQuerySetSizeW(
        lpSourceQuerySet);
    if ((DWORD) space_required > dwPackedQuerySetSize) {
        SetLastError(WSAEFAULT);
        ok_to_continue = FALSE;
    }

    SPACE_MGR  space_mgr(
        dwPackedQuerySetSize,
        lpPackedQuerySet);

    if (ok_to_continue) {
        LPWSAQUERYSETW  Target;
        Target = CopyQuerySetDirectW(
            & space_mgr,        // SpaceMgr
            lpSourceQuerySet);  // Source
        CopyQuerySetIndirectW(
            & space_mgr,        // SpaceMgr
            Target,             // Target
            lpSourceQuerySet);  // Source
    }

    if (ok_to_continue) {
        return_value = ERROR_SUCCESS;
    }
    else {
        return_value = SOCKET_ERROR;
    }
    return(return_value);

} // WSABuildQuerySetBufferW




LPWSTR
wcs_dup_from_ansi(
    IN LPSTR  Source
    )
/*++
Routine Description:

    This  procedure  is intended for internal use only within this module since
    it  requires the caller to use the same memory management strategy that the
    procedure uses internally.

    The procedure allocates a Unicode string and initializes it with the string
    converted  from  the  supplied  Ansi  source  string.   It  is the caller's
    responsibility  to  eventually deallocate the returned Unicode string using
    the C++ "delete" operator.

Arguments:

    Source - Supplies the Ansi string to be duplicated into Unicode form.

Return Value:

    The  procedure  returns  the newly allocated and initialized Unicode string
    pointer.   It  caller  must eventually deallocate this string using the C++
    "delete" opertor.  The procedure returns NULL if memory allocation fails.
--*/
{
    INT     len_guess;
    BOOL    still_trying;
    LPWSTR  return_string;

    assert( Source != NULL );

    // An  initial guess length of zero is required, since that is the only way
    // we  can coax the conversion fuction to ignore the buffer and tell us the
    // length  required.   Note  that  "length"  is in terms of the destination
    // characters   whatever  byte-width  they  have.   Presumably  the  length
    // returned from a conversion function includes the terminator.

    len_guess = 0;
    still_trying = TRUE;
    return_string = NULL;

    while (still_trying) {
        int  chars_required;

        chars_required = MultiByteToWideChar(
            CP_ACP,         // CodePage (Ansi)
            0,              // dwFlags
            Source,         // lpMultiByteStr
            -1,             // cchMultiByte
            return_string,  // lpWideCharStr
            len_guess);     // cchWideChar
        if (chars_required > len_guess) {
            // retry with new size
            len_guess = chars_required;
            delete return_string;
            return_string = new WCHAR[len_guess];
            if (return_string == NULL) {
                still_trying = FALSE;
            }
        }
        else if (chars_required > 0) {
            // success
            still_trying = FALSE;
        }
        else {
            // utter failure
            delete return_string;
            return_string = NULL;
            still_trying = FALSE;
        }
    } // while still_trying

    return(return_string);

} // wcs_dup_from_ansi




LPSTR
ansi_dup_from_wcs(
    IN LPWSTR  Source
    )
/*++
Routine Description:

    This  procedure  is intended for internal use only within this module since
    it  requires the caller to use the same memory management strategy that the
    procedure uses internally.

    The  procedure  allocates an Ansi string and initializes it with the string
    converted  from  the  supplied  Unicode  source string.  It is the caller's
    responsibility  to eventually deallocate the returned Ansi string using the
    C++ "delete" operator.

Arguments:

    Source - Supplies the Unicode string to be duplicated into Ansi form.

Return Value:

    The  procedure  returns  the  newly  allocated  and initialized Ansi string
    pointer.   It  caller  must eventually deallocate this string using the C++
    "delete" opertor.  The procedure returns NULL if memory allocation fails.
--*/
{
    INT     len_guess;
    BOOL    still_trying;
    LPSTR   return_string;

    assert( Source != NULL );

    // An  initial guess length of zero is required, since that is the only way
    // we  can coax the conversion fuction to ignore the buffer and tell us the
    // length  required.   Note  that  "length"  is in terms of the destination
    // characters   whatever  byte-width  they  have.   Presumably  the  length
    // returned from a conversion function includes the terminator.

    len_guess = 0;
    still_trying = TRUE;
    return_string = NULL;

    while (still_trying) {
        int  chars_required;

        chars_required = WideCharToMultiByte(
            CP_ACP,        // CodePage (Ansi)
            0,             // dwFlags
            Source,        // lpWideCharStr
            -1,            // cchWideChar
            return_string, // lpMultiByteStr
            len_guess,     // cchMultiByte
            NULL,          // lpDefaultChar
            NULL);         // lpUsedDefaultChar
        if (chars_required > len_guess) {
            // retry with new size
            len_guess = chars_required;
            delete return_string;
            return_string = new CHAR[len_guess];
            if (return_string == NULL) {
                still_trying = FALSE;
            }
        }
        else if (chars_required > 0) {
            // success
            still_trying = FALSE;
        }
        else {
            // utter failure
            delete return_string;
            return_string = NULL;
            still_trying = FALSE;
        }
    } // while still_trying

    return(return_string);

} // ansi_dup_from_wcs




INT
MapAnsiQuerySetToUnicode(
    IN     LPWSAQUERYSETA  Source,
    IN OUT LPDWORD         lpTargetSize,
    OUT    LPWSAQUERYSETW  Target
    )
/*++
Routine Description:

    This  procedure  takes  an  Ansi  LPWSAQUERYSETA  and  builds an equivalent
    Unicode LPWSAQUERYSETW packed structure.

Arguments:

    Source       - Supplies  the  source query set structure to be copied.  The
                   source  structure  may  be in packed or separately-allocated
                   form.

    lpTargetSize - Supplies  the  size, in bytes, of the Target buffer.  If the
                   function  fails  due to insufficient Target buffer space, it
                   returns  the  required  size of the Target buffer.  In other
                   situations, lpTargetSize is not updated.

    Target       - Returns   the   equivalent   Unicode  LPWSAQUERYSETW  packed
                   structure.   This  value  is  ignored if lpTargetSize is not
                   enough  to  hold the resulting structure.  It may be NULL if
                   lpTargetSize is 0.

Return Value:

    ERROR_SUCCESS - The function was successful

    WSAEFAULT     - The  function  failed  due to insufficient buffer space and
                    lpTargetSize was updated with the required size.

    other         - If  the  function  fails  in  any  other way, it returns an
                    appropriate WinSock 2 error code.

Implementation:

    compute size required for copy of source;
    allocate a source copy buffer;
    build source copy;
    cast source copy to Unicode version;
    for each source string requiring conversion loop
        allocate and init with converted string;
        over-write string pointer with allocated;
    end loop
    compute size required for unicode version;
    if (we have enough size) then
        flatten unicode version into target
        return_value = ERROR_SUCCESS
    else
        *lpTargetSize = required unicode size
    endif
    for each allocated converted string loop
        delete converted string
    end loop
    delete source copy buffer
--*/
{
    INT             return_value;
    LPWSAQUERYSETA  src_copy_A;
    LPWSAQUERYSETW  src_copy_W;
    DWORD           src_size_A;
    DWORD           needed_size_W;
    INT             build_result_A;
    INT             build_result_W;
    BOOL            ok_to_continue;
    LPWSTR          W_string1;
    LPWSTR          W_string2;
    LPWSTR          W_string3;

    ok_to_continue = TRUE;
    return_value = ERROR_SUCCESS;

    // prepare for error-case memory cleanup
    src_copy_A = NULL;
    W_string1 = NULL;
    W_string2 = NULL;
    W_string3 = NULL;

    src_size_A = WSAComputeQuerySetSizeA(Source);
    src_copy_A = (LPWSAQUERYSETA) new char[src_size_A];
    if (src_copy_A == NULL) {
        return_value = WSA_NOT_ENOUGH_MEMORY;
        ok_to_continue = FALSE;
    }
    if (ok_to_continue) {
        build_result_A = WSABuildQuerySetBufferA(
            Source,      // lpSourceQuerySet
            src_size_A,  // dwPackedQuerySetSize
            src_copy_A); // lpPackedQuerySet
        if (build_result_A != ERROR_SUCCESS) {
            return_value = GetLastError();
            ok_to_continue = FALSE;
        }
    } // if (ok_to_continue)

    if (ok_to_continue) {
        // In  the following cast, we are taking advantage of the fact that the
        // layout of fields in the WSAQUERYSETA and WSAQUERYSETW are identical.
        // If  this  were not the case, we would have to assemble an equivalent
        // query  set  of the other type field by field.  Since the layouts are
        // the  same,  we  can  simply  alter  our  local  copy  in-place  with
        // converted, separately allocated strings.
        src_copy_W = (LPWSAQUERYSETW) src_copy_A;

        if( src_copy_A->lpszServiceInstanceName != NULL ) {
            W_string1 = wcs_dup_from_ansi(
                src_copy_A->lpszServiceInstanceName);
            if (W_string1 == NULL) {
                return_value = WSA_NOT_ENOUGH_MEMORY;
                ok_to_continue = FALSE;
            }
        }
        src_copy_W->lpszServiceInstanceName = W_string1;
    } // if (ok_to_continue)

    if (ok_to_continue) {
        if( src_copy_A->lpszComment != NULL ) {
            W_string2 = wcs_dup_from_ansi(
                src_copy_A->lpszComment);
            if (W_string2 == NULL) {
                return_value = WSA_NOT_ENOUGH_MEMORY;
                ok_to_continue = FALSE;
            }
        }
        src_copy_W->lpszComment = W_string2;
    } // if (ok_to_continue)

    if (ok_to_continue) {
        if( src_copy_A->lpszContext != NULL ) {
            W_string3 = wcs_dup_from_ansi(
                src_copy_A->lpszContext);
            if (W_string3 == NULL) {
                return_value = WSA_NOT_ENOUGH_MEMORY;
                ok_to_continue = FALSE;
            }
        }
        src_copy_W->lpszContext = W_string3;
    } // if (ok_to_continue)

    if (ok_to_continue) {
        if( src_copy_A->lpszQueryString != NULL ) {
            W_string3 = wcs_dup_from_ansi(
                src_copy_A->lpszQueryString);
            if (W_string3 == NULL) {
                return_value = WSA_NOT_ENOUGH_MEMORY;
                ok_to_continue = FALSE;
            }
        }
        src_copy_W->lpszQueryString = W_string3;
    } // if (ok_to_continue)

    // Now  we  have  a  converted  query set, but it is composed of separately
    // allocated pieces attached to our locally-allocated buffer.

    if (ok_to_continue) {
        needed_size_W = WSAComputeQuerySetSizeW(src_copy_W);
        if (needed_size_W > (* lpTargetSize)) {
            * lpTargetSize = needed_size_W;
            return_value = WSAEFAULT;
            ok_to_continue = FALSE;
        }
    }

    if (ok_to_continue) {
        build_result_W = WSABuildQuerySetBufferW(
            src_copy_W,      // lpSourceQuerySet
            * lpTargetSize,  // dwPackedQuerySetSize
            Target);         // lpPackedQuerySet
        if (build_result_W != ERROR_SUCCESS) {
            return_value = GetLastError();
            ok_to_continue = FALSE;
        }
    }

    // clean up the temporarily-allocated memory
    delete W_string3;
    delete W_string2;
    delete W_string1;
    delete src_copy_A;

    return(return_value);

} // MapAnsiQuerySetToUnicode




INT
MapUnicodeQuerySetToAnsi(
    IN     LPWSAQUERYSETW  Source,
    IN OUT LPDWORD         lpTargetSize,
    OUT    LPWSAQUERYSETA  Target
    )
/*++
Routine Description:

    This  procedure  takes  a Unicode   LPWSAQUERYSETW  and  builds an equivalent
    Ansi LPWSAQUERYSETA packed structure.

Arguments:

    Source       - Supplies  the  source query set structure to be copied.  The
                   source  structure  may  be in packed or separately-allocated
                   form.

    lpTargetSize - Supplies  the  size, in bytes, of the Target buffer.  If the
                   function  fails  due to insufficient Target buffer space, it
                   returns  the  required  size of the Target buffer.  In other
                   situations, lpTargetSize is not updated.

    Target       - Returns the equivalent Ansi LPWSAQUERYSETA packed structure.
                   This  value is ignored if lpTargetSize is not enough to hold
                   the  resulting structure.  It may be NULL if lpTargetSize is
                   0.

Return Value:

    ERROR_SUCCESS - The function was successful

    WSAEFAULT     - The  function  failed  due to insufficient buffer space and
                    lpTargetSize was updated with the required size.

    other         - If  the  function  fails  in  any  other way, it returns an
                    appropriate WinSock 2 error code.
--*/
{
    INT             return_value;
    LPWSAQUERYSETW  src_copy_W;
    LPWSAQUERYSETA  src_copy_A;
    DWORD           src_size_W;
    DWORD           needed_size_A;
    INT             build_result_W;
    INT             build_result_A;
    BOOL            ok_to_continue;
    LPSTR           A_string1;
    LPSTR           A_string2;
    LPSTR           A_string3;

    ok_to_continue = TRUE;
    return_value = ERROR_SUCCESS;

    // prepare for error-case memory cleanup
    src_copy_W = NULL;
    A_string1 = NULL;
    A_string2 = NULL;
    A_string3 = NULL;

    src_size_W = WSAComputeQuerySetSizeW(Source);
    src_copy_W = (LPWSAQUERYSETW) new char[src_size_W];
    if (src_copy_W == NULL) {
        return_value = WSA_NOT_ENOUGH_MEMORY;
        ok_to_continue = FALSE;
    }
    if (ok_to_continue) {
        build_result_W = WSABuildQuerySetBufferW(
            Source,      // lpSourceQuerySet
            src_size_W,  // dwPackedQuerySetSize
            src_copy_W); // lpPackedQuerySet
        if (build_result_W != ERROR_SUCCESS) {
            return_value = GetLastError();
            ok_to_continue = FALSE;
        }
    } // if (ok_to_continue)

    if (ok_to_continue) {
        // In  the following cast, we are taking advantage of the fact that the
        // layout of fields in the WSAQUERYSETA and WSAQUERYSETW are identical.
        // If  this  were not the case, we would have to assemble an equivalent
        // query  set  of the other type field by field.  Since the layouts are
        // the  same,  we  can  simply  alter  our  local  copy  in-place  with
        // converted, separately allocated strings.
        src_copy_A = (LPWSAQUERYSETA) src_copy_W;

        if( src_copy_W->lpszServiceInstanceName != NULL ) {
            A_string1 = ansi_dup_from_wcs(
                src_copy_W->lpszServiceInstanceName);
            if (A_string1 == NULL) {
                return_value = WSA_NOT_ENOUGH_MEMORY;
                ok_to_continue = FALSE;
            }
        }
        src_copy_A->lpszServiceInstanceName = A_string1;
    } // if (ok_to_continue)

    if (ok_to_continue) {
        if( src_copy_W->lpszComment != NULL ) {
            A_string2 = ansi_dup_from_wcs(
                src_copy_W->lpszComment);
            if (A_string2 == NULL) {
                return_value = WSA_NOT_ENOUGH_MEMORY;
                ok_to_continue = FALSE;
            }
        }
        src_copy_A->lpszComment = A_string2;
    } // if (ok_to_continue)

    if (ok_to_continue) {
        if( src_copy_W->lpszContext != NULL ) {
            A_string3 = ansi_dup_from_wcs(
                src_copy_W->lpszContext);
            if (A_string3 == NULL) {
                return_value = WSA_NOT_ENOUGH_MEMORY;
                ok_to_continue = FALSE;
            }
        }
        src_copy_A->lpszContext = A_string3;
    } // if (ok_to_continue)

    if (ok_to_continue) {
        if( src_copy_W->lpszQueryString != NULL ) {
            A_string3 = ansi_dup_from_wcs(
                src_copy_W->lpszQueryString);
            if (A_string3 == NULL) {
                return_value = WSA_NOT_ENOUGH_MEMORY;
                ok_to_continue = FALSE;
            }
        }
        src_copy_A->lpszQueryString = A_string3;
    } // if (ok_to_continue)

    // Now  we  have  a  converted  query set, but it is composed of separately
    // allocated pieces attached to our locally-allocated buffer.

    if (ok_to_continue) {
        needed_size_A = WSAComputeQuerySetSizeA(src_copy_A);
        if (needed_size_A > (* lpTargetSize)) {
            * lpTargetSize = needed_size_A;
            return_value = WSAEFAULT;
            ok_to_continue = FALSE;
        }
    }

    if (ok_to_continue) {
        build_result_A = WSABuildQuerySetBufferA(
            src_copy_A,      // lpSourceQuerySet
            * lpTargetSize,  // dwPackedQuerySetSize
            Target);         // lpPackedQuerySet
        if (build_result_A != ERROR_SUCCESS) {
            return_value = GetLastError();
            ok_to_continue = FALSE;
        }
    }

    // clean up the temporarily-allocated memory
    delete A_string3;
    delete A_string2;
    delete A_string1;
    delete src_copy_W;

    return(return_value);

} // MapUnicodeQuerySetToAnsi




INT
CopyQuerySetA(
    IN LPWSAQUERYSETA  Source,
    OUT LPWSAQUERYSETA *Target
    )
{
    DWORD dwSize = WSAComputeQuerySetSizeA(Source);

    *Target = (LPWSAQUERYSETA)new BYTE[dwSize];
    if (*Target == NULL)
        return WSA_NOT_ENOUGH_MEMORY;
    return WSABuildQuerySetBufferA(Source, dwSize, *Target);
} // CopyQuerySetA




INT
CopyQuerySetW(
    IN LPWSAQUERYSETW  Source,
    OUT LPWSAQUERYSETW *Target
    )
{
    DWORD dwSize = WSAComputeQuerySetSizeW(Source);

    *Target = (LPWSAQUERYSETW)new BYTE[dwSize];
    if (*Target == NULL)
        return WSA_NOT_ENOUGH_MEMORY;
    return WSABuildQuerySetBufferW(Source, dwSize, *Target);
} // CopyQuerySetW

