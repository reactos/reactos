//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       cBFile.cpp
//
//  Contents:   Microsoft Internet Security
//
//  History:    24-Oct-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "stack.hxx"
#include    "cbfile.hxx"

cBFile_::cBFile_(CRITICAL_SECTION *pCriticalSection, WCHAR *pwszBFilePath, WCHAR *pwszBFileBaseName,
                 DWORD cbKey, DWORD cbData, SHORT sVersion, BOOL *pfCreatedOK)
{
    SECURITY_ATTRIBUTES  sa;
    SECURITY_ATTRIBUTES* psa = NULL;
    SECURITY_DESCRIPTOR  sd;

    SID_IDENTIFIER_AUTHORITY    siaWorldSidAuthority    = SECURITY_WORLD_SID_AUTHORITY;
    PSID                        psidEveryone            = NULL;
    DWORD                       dwAclSize;
    PACL                        pDacl                   = NULL;
    DWORD dwErr = 0;

    pCritical       = pCriticalSection;
    fDirty          = FALSE;
    fReadOnly       = FALSE;
    hKFile          = INVALID_HANDLE_VALUE;
    hDFile          = INVALID_HANDLE_VALUE;
    pbKMap          = NULL;
    pbDMap          = NULL;
    cbDMap          = 0;
    hDMutex         = NULL;

    fUseRecNumAsKey = FALSE;

    *pfCreatedOK    = TRUE;  // will be set to FALSE in case of ANY failure

    pwszPath        = pwszBFilePath;
    pwszBaseName    = pwszBFileBaseName;

    memset(&sHeader, 0x00, sizeof(BFILE_HEADER));
    memset(&sRecord, 0x00, sizeof(BFILE_RECORD));

    sHeader.sIntVersion = BFILE_VERSION_1;
    sHeader.sVersion    = (DWORD)sVersion;
    sHeader.cbKey       = cbKey;
    sHeader.cbData      = cbData;

    if ( FIsWinNT() == TRUE )
    {
        if (!AllocateAndInitializeSid(
                &siaWorldSidAuthority,
                1,
                SECURITY_WORLD_RID,
                0, 0, 0, 0, 0, 0, 0,
                &psidEveryone))
        {
            goto ErrorReturn;
        }

        //
        // compute size of ACL
        //
        dwAclSize = 
            sizeof(ACL) +
            ( sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) ) +
            GetLengthSid(psidEveryone);

        //
        // allocate storage for Acl
        //
        if (NULL == (pDacl = (PACL) new BYTE[dwAclSize]))
        {
            goto MemoryError;
        }

        if (!InitializeAcl(pDacl, dwAclSize, ACL_REVISION))
        {
            goto ErrorReturn;
        }

        if (!AddAccessAllowedAce(
                pDacl,
                ACL_REVISION,
                SYNCHRONIZE,// | MUTEX_MODIFY_STATE,
                psidEveryone))
        {
            goto ErrorReturn;
        }

        if ( InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION ) == FALSE )
        {
            goto ErrorReturn;
        }

        if ( SetSecurityDescriptorDacl( &sd, TRUE, pDacl, FALSE ) == FALSE )
        {
            goto ErrorReturn;
        }

        sa.nLength = sizeof( SECURITY_ATTRIBUTES );
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;

        psa = &sa;
    }

    hDMutex = CreateMutexU( psa, FALSE, pwszBFileBaseName );
    if ( hDMutex == NULL )
    {
        hDMutex = OpenMutexU( SYNCHRONIZE, FALSE, pwszBFileBaseName );
        if ( hDMutex == NULL )
        {
            dwErr = GetLastError();
            goto ErrorReturn;
        }
    }

    fInitialized    = TRUE;     // set it now becuase OpenFiles uses "Lock".

    if (!(this->OpenFiles()))
    {
        goto FileError;
    }

    sRecord.cbKey       = sHeader.cbKey;
    sRecord.cbData      = sHeader.cbData;

    if (!(sRecord.pvKey = (void *)new BYTE[cbKey + sizeof(DWORD)]) ||
        !(sRecord.pvData = (void *)new BYTE[cbData + sizeof(DWORD)]))
    {
        goto MemoryError;
    }

    sRecord.pvData = (BYTE *)sRecord.pvData + sizeof(DWORD);    // we use the first dword for rec # (see addrec)

    memset(sRecord.pvKey, 0x00, cbKey);
    memset(sRecord.pvData, 0x00, cbData);

CommonReturn:

    if (pDacl != NULL)
    {
        delete[] pDacl;
    }

    if (psidEveryone)
    {
        FreeSid(psidEveryone);
    }
    return;

ErrorReturn:
        fInitialized    = FALSE;
        *pfCreatedOK    = FALSE;
        goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, MemoryError,       ERROR_NOT_ENOUGH_MEMORY);
    SET_ERROR_VAR_EX(DBG_SS, FileError,         ERROR_FILE_NOT_FOUND);
}

cBFile_::~cBFile_(void)
{
    if (fDirty)
    {
        this->Sort();
    }

    if (pbKMap)
    {
        UnmapViewOfFile(pbKMap);
    }

    if (pbDMap)
    {
        UnmapViewOfFile(pbDMap);
    }

    if (hDFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hDFile);
    }

    if (hKFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hKFile);
    }

    if (hDMutex != NULL)
    {
        CloseHandle(hDMutex);
    }

    DELETE_OBJECT(sRecord.pvKey);

    if (sRecord.pvData)
    {
        sRecord.pvData = (BYTE *)sRecord.pvData - sizeof(DWORD);    // we bumped the pointer in the constructer, put it back..

        delete sRecord.pvData;
    }
}

BOOL cBFile_::Initialize(void)
{
    return(fInitialized);
}

DWORD cBFile_::GetNumKeys(void)
{
    DWORD   dwRet;

    dwRet = GetFileSize(this->hKFile, NULL);

    if (dwRet > 0)
    {
        dwRet /= BFILE_KEYSIZE;

        return(dwRet);
    }

    return(0);
}

void cBFile_::setKey(void *pvInKey)
{
    memcpy(sRecord.pvKey, pvInKey, sRecord.cbKey);
}

void cBFile_::setData(void *pvInData)
{
    memcpy(sRecord.pvData, pvInData, sRecord.cbData);
}

BOOL cBFile_::Find(void)
{
    DWORD   cbDOff;
    DWORD   dwLastGood;
    void    *pvKey;
    BOOL    fRet;

    pvKey = new BYTE[sRecord.cbKey];

    if (!(pvKey))
    {
        goto MemoryError;
    }

    memcpy(pvKey, sRecord.pvKey, sRecord.cbKey);

    if (this->BinaryFind(&cbDOff))
    {
        dwLastGood = this->dwFirstNextRecNum;

        while (this->GetPrev(dwLastGood))
        {
            if (memcmp(pvKey, sRecord.pvKey, sRecord.cbKey) != 0)
            {
                break;
            }

            dwLastGood = this->dwFirstNextRecNum;
        }

        if (dwLastGood > 0)
        {
            dwLastGood--;

            delete pvKey;

            return(this->GetNext(dwLastGood));
        }

        delete pvKey;

        __try {

        return(this->GetDataRecord(cbDOff));

        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            this->UnmapAll();
            SetLastError(GetExceptionCode());
        }

        return(FALSE);
    }

    fRet = FALSE;

    CommonReturn:
        if (pvKey)
        {
            delete pvKey;
        }

        return(fRet);

    ErrorReturn:
        fRet = FALSE;
        goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, MemoryError,       ERROR_NOT_ENOUGH_MEMORY);
}

BOOL cBFile_::GetFirst(void)
{
    BOOL    fRet;

    this->dwFirstNextRecNum = 0;

    if (!(this->pbKMap))
    {
        return(FALSE);
    }

    __try {

    this->ReadHeader();

    DWORD       cbOff;

    memcpy(sRecord.pvKey, &this->pbKMap[0], sRecord.cbKey);
    memcpy(&cbOff, &this->pbKMap[sRecord.cbKey], sizeof(DWORD));

    if (!(this->GetDataRecord(cbOff)))
    {
        goto cBFileCorrupt;
    }

    fRet = TRUE;

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        this->UnmapAll();
        SetLastError(GetExceptionCode());
        goto cBFileCorrupt;
    }

    CommonReturn:
        return(fRet);

    ErrorReturn:
        fRet = FALSE;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, cBFileCorrupt);
}

BOOL cBFile_::GetNext(DWORD dwCurRec)
{
    BOOL    fRet;

    if (!(this->pbKMap))
    {
        return(FALSE);
    }

    if (dwCurRec == 0xffffffff)
    {
        dwCurRec = this->dwFirstNextRecNum;
    }

    DWORD       cbOff;

    dwCurRec++;

    if (((dwCurRec + 1) * BFILE_KEYSIZE) > sHeader.cbSortedEOF)
    {
        goto cBFileNoNext;
    }

    __try {

    memcpy(sRecord.pvKey, &this->pbKMap[dwCurRec * BFILE_KEYSIZE], sRecord.cbKey);
    memcpy(&cbOff, &this->pbKMap[(dwCurRec * BFILE_KEYSIZE) + sRecord.cbKey], sizeof(DWORD));

    if (!(this->GetDataRecord(cbOff)))
    {
        goto cBFileCorrupt;
    }

    this->dwFirstNextRecNum = dwCurRec;

    fRet = TRUE;

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        this->UnmapAll();
        SetLastError(GetExceptionCode());
        goto cBFileCorrupt;
    }

    CommonReturn:
        return(fRet);

    ErrorReturn:
        fRet = FALSE;
        this->dwFirstNextRecNum = 0;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, cBFileNoNext);
    TRACE_ERROR_EX(DBG_SS, cBFileCorrupt);
}

BOOL cBFile_::GetPrev(DWORD dwCurRec)
{
    BOOL    fRet;

    if (!(this->pbKMap) || (sHeader.cbSortedEOF == 0))
    {
        goto cBFileNoPrev;
    }

    if (dwCurRec == 0xffffffff)
    {
        dwCurRec = this->dwFirstNextRecNum;
    }

    DWORD       cb;
    DWORD       cbOff;

    if (dwCurRec < 1)
    {
        goto cBFileNoPrev;
    }

    dwCurRec--;

    if (((dwCurRec + 1) * BFILE_KEYSIZE) >= sHeader.cbSortedEOF)
    {
        goto cBFileNoPrev;
    }

    __try {

    memcpy(sRecord.pvKey, &this->pbKMap[dwCurRec * BFILE_KEYSIZE], sRecord.cbKey);
    memcpy(&cbOff, &this->pbKMap[(dwCurRec * BFILE_KEYSIZE) + sRecord.cbKey], sizeof(DWORD));

    if (!(this->GetDataRecord(cbOff)))
    {
        goto cBFileCorrupt;
    }

    this->dwFirstNextRecNum = dwCurRec;

    fRet = TRUE;

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        this->UnmapAll();
        SetLastError(GetExceptionCode());
        goto cBFileCorrupt;
    }

    CommonReturn:
        return(fRet);

    ErrorReturn:
        fRet = FALSE;
        this->dwFirstNextRecNum = 0;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, cBFileNoPrev);
    TRACE_ERROR_EX(DBG_SS, cBFileCorrupt);
}

BOOL cBFile_::Update(void)
{
    DWORD   cbDOff;

    if ( fReadOnly == TRUE )
    {
        SetLastError( ERROR_ACCESS_DENIED );
        return( FALSE );
    }

    if (this->BinaryFind(&cbDOff))
    {
        this->UpdateDataRecord(cbDOff);

        return(TRUE);
    }

    return(FALSE);
}

BOOL cBFile_::Add(void)
{
    if ( fReadOnly == TRUE )
    {
        SetLastError( ERROR_ACCESS_DENIED );
        return( FALSE );
    }

    this->AddDirtyKey();

    return(TRUE);
}

BOOL cBFile_::Lock(void)
{
    WaitForSingleObject( this->hDMutex, INFINITE );
    return(TRUE);
}

BOOL cBFile_::Unlock(void)
{
    ReleaseMutex( this->hDMutex );
    return( TRUE );
}

void cBFile_::SpeedSort(void)
{
    Stack_      *pcStack;

    if ( fReadOnly == TRUE )
    {
        assert( FALSE && "A sort should not be forced in read-only mode." );
        return;
    }

    pcStack = NULL;

    if (!(this->pbKMap))
    {
        goto RemapError;
    }

    if (!(this->Lock()))
    {
        goto LockFileError;
    }

    //
    // load dirty in memory and sort it
    //
    DWORD       cbCurKey;
    DWORD       cbTotal;

    if (!(pcStack = new Stack_(pCritical)))
    {
        goto MemoryError;
    }

    cbTotal     = GetFileSize(this->hKFile, NULL);
    cbCurKey    = sHeader.cbSortedEOF;

    __try {

    while (cbCurKey < cbTotal)
    {
        pcStack->Add(BFILE_KEYSIZE, &this->pbKMap[cbCurKey]);

        cbCurKey += BFILE_KEYSIZE;
    }

    pcStack->Sort(0, sRecord.cbKey, STACK_SORTTYPE_BINARY);

    //
    //  shuffle into sorted
    //
    DWORD   dwIdx;
    DWORD   dwInsertion;
    DWORD   cbFreeSpace;
    BYTE    *pbKey;
    BYTE    *pbCurrentKey;
    BYTE    *pbStartFreeSpace;

    pbStartFreeSpace    = &this->pbKMap[sHeader.cbSortedEOF];
    cbFreeSpace         = pcStack->Count() * BFILE_KEYSIZE;

    dwIdx = (long)pcStack->Count() - 1;

    while (pbKey = (BYTE *)pcStack->Get(dwIdx))
    {
        //
        //  get the starting point of our "window"
        //
        dwInsertion = this->GetInsertionPoint(pbKey);

        //
        //  move the old data to the current free space "window"
        //
        memmove(&this->pbKMap[dwInsertion + cbFreeSpace], &this->pbKMap[dwInsertion],
                sHeader.cbSortedEOF - dwInsertion);

        //
        //  after this, the insersion point has free space the size of the "window" - key size
        //
        cbFreeSpace -= BFILE_KEYSIZE;

        //
        // add the new key to the end of the "free space" window.
        //
        memcpy(&this->pbKMap[dwInsertion + cbFreeSpace], pbKey, BFILE_KEYSIZE);

        //
        //  keep the end of the search up to date for these insertions....
        //
        sHeader.cbSortedEOF = dwInsertion;

        if (dwIdx == 0)
        {
            break;
        }

        dwIdx--;
    }

    sHeader.cbSortedEOF     = cbTotal;
    sHeader.fDirty          = FALSE;
    fDirty                  = FALSE;

    this->UpdateHeader();

    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        this->UnmapAll();
        SetLastError( GetExceptionCode() );
        goto RemapError;
    }

    ErrorReturn:
        this->Unlock();
        DELETE_OBJECT(pcStack);
        return;

    TRACE_ERROR_EX(DBG_SS, LockFileError);
    TRACE_ERROR_EX(DBG_SS, RemapError);

    SET_ERROR_VAR_EX(DBG_SS, MemoryError,       ERROR_NOT_ENOUGH_MEMORY);
}

DWORD cBFile_::GetInsertionPoint(void *pvIn)
{
    DWORD   dwEnd;
    DWORD   dwMiddle;
    DWORD   dwStart;
    DWORD   dwHalf;
    DWORD   dwCur;
    DWORD   dwRet;
    BYTE    *pb;
    int     cmp;

    dwStart     = 0;
    dwEnd       = sHeader.cbSortedEOF / BFILE_KEYSIZE;
    dwMiddle    = dwEnd / 2L;
    dwHalf      = dwMiddle;
    dwCur       = 0;
    if (sHeader.cbSortedEOF >= BFILE_KEYSIZE)
    {
        dwRet   = sHeader.cbSortedEOF - BFILE_KEYSIZE;
    }
    else
    {
        dwRet   = 0;
    }

    for EVER
    {
        pb = this->pbKMap + (dwMiddle * BFILE_KEYSIZE);

        cmp = memcmp(pvIn, pb, sRecord.cbKey);

        if (cmp == 0)
        {
            dwRet = (DWORD)(pb - this->pbKMap);
            break;
        }

        if ( (dwMiddle == 0) ||
             (dwMiddle == (sHeader.cbSortedEOF / BFILE_KEYSIZE)) ||
             ((dwHalf == 0) && (dwMiddle == dwStart)) )
        {
            DWORD   dwFind;

            dwFind  = dwRet;
            cmp     = (-1);

            while ((cmp < 0) && ((dwFind + BFILE_KEYSIZE) <= sHeader.cbSortedEOF))
            {
                cmp = memcmp(pvIn, &this->pbKMap[dwFind], sRecord.cbKey);

                dwRet = dwFind;

                dwFind += BFILE_KEYSIZE;
            }

            break;
        }

        if (cmp < 0)
        {
            dwEnd   = dwMiddle;
            dwRet   = (dwMiddle * BFILE_KEYSIZE);
        }
        else
        {
            dwStart = dwMiddle;
        }

        dwHalf      = (dwEnd - dwStart) / 2L;
        dwMiddle    = dwStart + dwHalf;
    }

    return(dwRet);
}

void *cBFile_::GetDumpKey(DWORD dwIdx, void *pvRetKey, DWORD *pdwRecOffset)
{
    DWORD   dwOffset;

    dwOffset = dwIdx * BFILE_KEYSIZE;

    if (dwOffset >= GetFileSize(this->hKFile, NULL))
    {
        return(NULL);
    }

    __try {

    memcpy(pvRetKey, &this->pbKMap[dwOffset], sRecord.cbKey);

    memcpy(pdwRecOffset, &this->pbKMap[dwOffset + sRecord.cbKey], sizeof(DWORD));

    return(pvRetKey);

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        this->UnmapAll();
        SetLastError(GetExceptionCode());
    }

    return(NULL);
}

BOOL cBFile_::GetHeader(BFILE_HEADER *psHeader)
{
    __try {

    this->ReadHeader();

    memcpy(psHeader, &sHeader, sizeof(BFILE_HEADER));

    return(TRUE);

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        this->UnmapAll();
        SetLastError(GetExceptionCode());
    }

    return(FALSE);
}

void cBFile_::Sort(void)
{
    if ( fReadOnly == TRUE )
    {
        assert( FALSE && "A sort should not be forced in read-only mode." );
        return;
    }

    EnterCriticalSection(pCritical);

    this->RemapKey();
    this->RemapData();

#if 0
    if (sHeader.cbSortedEOF > 0)
    {
        this->SpeedSort();
        LeaveCriticalSection(pCritical);
        return;
    }
#endif

    BYTE        *pb;
    Stack_      *pcStack;

    pcStack = NULL;
    pb      = NULL;

    if (!(this->Lock()))
    {
        goto LockFileError;
    }

    DWORD       dwLen;
    DWORD       cbFile;
    DWORD       dwIdx;
    DWORD       i;
    BYTE        *pbStack;

    if (!(pcStack = new Stack_(pCritical)))
    {
        goto MemoryError;
    }

    dwLen = BFILE_KEYSIZE;

    if (!(pb = new BYTE[dwLen]))
    {
        goto MemoryError;
    }

    cbFile = GetFileSize(this->hKFile, NULL);

    __try {

    for (i = 0; i < cbFile; i += dwLen)
    {
        pcStack->Add(dwLen, &this->pbKMap[i]);
    }

    pcStack->Sort(0, sRecord.cbKey, STACK_SORTTYPE_BINARY);

    dwIdx   = 0;
    i       = 0;
    while (pbStack = (BYTE *)pcStack->Get(dwIdx))
    {
        if ((i + dwLen) > cbFile)
        {
            goto FileSizeError;
        }

        memcpy(&this->pbKMap[i], pbStack, dwLen);

        dwIdx++;
        i += dwLen;
    }


    sHeader.cbSortedEOF     = cbFile;
    sHeader.fDirty          = FALSE;
    fDirty                  = FALSE;

    this->UpdateHeader();

    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        this->UnmapAll();
        SetLastError( GetExceptionCode() );
    }

    ErrorReturn:
        this->Unlock();

        DELETE_OBJECT(pcStack);
        DELETE_OBJECT(pb);

        LeaveCriticalSection(pCritical);
        return;

    TRACE_ERROR_EX(DBG_SS, FileSizeError);
    TRACE_ERROR_EX(DBG_SS, LockFileError);

    SET_ERROR_VAR_EX(DBG_SS, MemoryError,       ERROR_NOT_ENOUGH_MEMORY);
}

void cBFile_::AddDirtyKey(void)
{
    DWORD   cbOff;
    DWORD   cb;

    if ( fReadOnly == TRUE )
    {
        assert( FALSE && "Add dirty key should not occur in read-only mode." );
        return;
    }

    this->Lock();

    __try {

    this->ReadHeader();

    sHeader.dwLastRecNum++;
    sRecord.dwRecNum = sHeader.dwLastRecNum;

    if ((cbOff = this->AddDataRecord()) != 0xffffffff)
    {
        if (fUseRecNumAsKey)
        {
            memcpy(sRecord.pvKey, &sRecord.dwRecNum, sizeof(DWORD));
        }

        memcpy((BYTE *)sRecord.pvKey + sRecord.cbKey, &cbOff, sizeof(DWORD));   // speed...
        SetFilePointer(this->hKFile, 0, NULL, FILE_END);
        WriteFile(this->hKFile, sRecord.pvKey, sRecord.cbKey + sizeof(DWORD), &cb, NULL);
        // WriteFile(this->hKFile, &cbOff, sizeof(DWORD), &cb, NULL);

        sHeader.fDirty          = TRUE;
        this->fDirty            = TRUE;

        this->UpdateHeader();
    }

    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        this->UnmapAll();
        SetLastError( GetExceptionCode() );
    }

    this->Unlock();
}

BOOL cBFile_::GetDataRecord(DWORD cbDataOffset)
{
    DWORD   cb;

    memset(sRecord.pvData, 0x00, sRecord.cbData);

    if (this->cbDMap < (cbDataOffset + sizeof(DWORD) + sRecord.cbData))
    {
        return(FALSE);
    }

    memcpy(&sRecord.dwRecNum, &this->pbDMap[cbDataOffset], sizeof(DWORD));
    memcpy(sRecord.pvData, &this->pbDMap[cbDataOffset + sizeof(DWORD)], sRecord.cbData);

    return(TRUE);
}

void cBFile_::UpdateDataRecord(DWORD cbDataOffset)
{
    DWORD   cb;

    if ( fReadOnly == TRUE )
    {
        assert( FALSE && "Update data record should not occur in read-only mode." );
        return;
    }

    this->Lock();

    if (this->cbDMap < (cbDataOffset + sizeof(DWORD) + sRecord.cbData))
    {
        this->Unlock();
        return;
    }

    __try {

    memcpy(&this->pbDMap[cbDataOffset + sizeof(DWORD)], sRecord.pvData, sRecord.cbData);

    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        this->UnmapAll();
        SetLastError( GetExceptionCode() );
    }

    this->Unlock();
}

DWORD cBFile_::AddDataRecord(void)
{
    DWORD   cbRet;

    if ( fReadOnly == TRUE )
    {
        SetLastError( ERROR_ACCESS_DENIED );
        return( 0xFFFFFFFF );
    }

    //
    // no lock here because it is called from add key which
    // does a lock
    //

    if ((cbRet = SetFilePointer(this->hDFile, 0, NULL, FILE_END)) != 0xffffffff)
    {
        DWORD   cb;
        BYTE    *pv;

        pv = (BYTE *)sRecord.pvData - sizeof(DWORD);
        memcpy(pv, &sRecord.dwRecNum, sizeof(DWORD));

        WriteFile(this->hDFile, pv, sizeof(DWORD) + sRecord.cbData, &cb, NULL);
        // WriteFile(this->hDFile, sRecord.pvData, sRecord.cbData, &cb, NULL);
    }

    return(cbRet);
}

BOOL cBFile_::BinaryFind(DWORD *pcbDataOffset)
{
    if (sHeader.fDirty)
    {
        this->Sort();
    }

    if (sHeader.cbSortedEOF == 0)
    {
        return(FALSE);
    }

    if (!(this->pbKMap))
    {
        return(FALSE);
    }

    DWORD   dwEnd;
    DWORD   dwMiddle;
    DWORD   dwStart;
    DWORD   dwHalf;
    DWORD   dwCur;
    void    *pv;
    int     cmp;

    dwStart     = 0;
    dwEnd       = sHeader.cbSortedEOF / BFILE_KEYSIZE;
    dwHalf = dwMiddle    = dwEnd / 2L;
    dwCur       = 0;

    __try {

    for EVER
    {
        pv = (void *)(this->pbKMap + (dwMiddle * BFILE_KEYSIZE));

        cmp = memcmp(sRecord.pvKey, pv, sRecord.cbKey);

        if (cmp == 0)
        {
            memcpy(pcbDataOffset, (char *)pv + sRecord.cbKey, sizeof(DWORD));

            this->dwFirstNextRecNum = (DWORD)((BYTE *)pv - this->pbKMap) / BFILE_KEYSIZE;

            return(TRUE);
        }

        if ((dwMiddle == 0) || (dwMiddle == (sHeader.cbSortedEOF / BFILE_KEYSIZE)) ||
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

    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        this->UnmapAll();
        SetLastError( GetExceptionCode() );
    }

    return(FALSE);
}

void cBFile_::ReadHeader(void)
{
    if ((this->pbDMap) && (this->cbDMap >= (sizeof(BFILE_HEADER) + BFILE_SIZEOFSIG)))
    {
        memcpy(&this->sHeader, &this->pbDMap[BFILE_SIZEOFSIG], sizeof(BFILE_HEADER));
    }
    else if (SetFilePointer(this->hDFile, BFILE_SIZEOFSIG, NULL, FILE_BEGIN) != 0xffffffff)
    {
        DWORD   cb;

        ReadFile(this->hDFile, &this->sHeader, sizeof(BFILE_HEADER), &cb, NULL);
    }
    else
    {
        memset(&this->sHeader, 0x00, sizeof(BFILE_HEADER));
    }
}

BOOL cBFile_::UpdateHeader(void)
{
    if ( fReadOnly == TRUE )
    {
        SetLastError( ERROR_ACCESS_DENIED );
        return( FALSE );
    }

    if ((this->pbDMap) && (this->cbDMap >= (sizeof(BFILE_HEADER) + BFILE_SIZEOFSIG)))
    {
        memcpy(&this->pbDMap[BFILE_SIZEOFSIG], &this->sHeader, sizeof(BFILE_HEADER));
        return(TRUE);
    }

    if (SetFilePointer(this->hDFile, BFILE_SIZEOFSIG, NULL, FILE_BEGIN) != 0xffffffff)
    {
        DWORD   cbWritten;

        WriteFile(this->hDFile, &this->sHeader, sizeof(BFILE_HEADER), &cbWritten, NULL);

        return(TRUE);
    }

    return(FALSE);
}

BOOL cBFile_::OpenFiles(void)
{
    BOOL    fRet = TRUE;
    WCHAR   wszFile[MAX_PATH];
    DWORD   cbWritten;
    int     iBaseEnd;

    wcscpy(&wszFile[0], pwszPath);
    if (wszFile[wcslen(pwszPath) - 1] != L'\\')
    {
        wcscat(&wszFile[0], L"\\");
    }
    wcscat(&wszFile[0], pwszBaseName);
    iBaseEnd = wcslen(&wszFile[0]);

    wcscpy(&wszFile[iBaseEnd], BFILE_DATAEXT);

    hDFile = CreateFileU(&wszFile[0], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL, OPEN_EXISTING, 0, NULL);

    if (hDFile == INVALID_HANDLE_VALUE)
    {
        if ( GetLastError() == ERROR_ACCESS_DENIED )
        {
            hDFile = CreateFileU(&wszFile[0], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL, OPEN_EXISTING, 0, NULL);

            fReadOnly = TRUE;
        }
        else
        {
            hDFile = CreateFileU(&wszFile[0], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, CREATE_NEW, 0, NULL);
        }

        if (hDFile == INVALID_HANDLE_VALUE)
        {
            return(FALSE);
        }

        if ( fReadOnly == FALSE )
        {
            WriteFile(this->hDFile, BFILE_SIG, BFILE_SIZEOFSIG, &cbWritten, NULL);

            this->Lock();

            __try {

            fRet = this->UpdateHeader();

            }
            __except( EXCEPTION_EXECUTE_HANDLER ) {
                fRet = FALSE;
            }

            this->Unlock();

            if ( fRet == FALSE )
            {
                return( FALSE );
            }

            wcscpy(&wszFile[iBaseEnd], BFILE_KEYEXT);

            hKFile = CreateFileU(&wszFile[0], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL, OPEN_ALWAYS, 0, NULL);
        }
        else
        {
            wcscpy(&wszFile[iBaseEnd], BFILE_KEYEXT);

            hKFile = CreateFileU(&wszFile[0], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL, OPEN_EXISTING, 0, NULL);
        }
    }
    else
    {
        wcscpy(&wszFile[iBaseEnd], BFILE_KEYEXT);

        hKFile = CreateFileU(&wszFile[0], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, OPEN_ALWAYS, 0, NULL);
    }

    if ((hDFile == INVALID_HANDLE_VALUE) || (hKFile == INVALID_HANDLE_VALUE))
    {
        return(FALSE);
    }

    fRet &= this->RemapKey();
    fRet &= this->RemapData();

    if ( fRet == FALSE )
    {
        return( FALSE );
    }

    __try {

    this->ReadHeader();

    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        fRet = FALSE;
    }

    return(fRet);
}


BOOL cBFile_::RemapKey(void)
{
    LPBYTE pbOldMap;
    HANDLE hMappedFile;
    DWORD  PageAccess = PAGE_READWRITE;
    DWORD  FileMapAccess = FILE_MAP_WRITE;

    pbOldMap = this->pbKMap;

    if ( fReadOnly == TRUE )
    {
        PageAccess = PAGE_READONLY;
        FileMapAccess = FILE_MAP_READ;
    }

    hMappedFile = CreateFileMapping(this->hKFile, NULL, PageAccess, 0, 0, NULL);

    if (!(hMappedFile) || (hMappedFile == INVALID_HANDLE_VALUE))
    {
        return(TRUE);   // could be the first call!
    }

    this->pbKMap = (BYTE *)MapViewOfFile(hMappedFile, FileMapAccess, 0, 0, 0);
    if ( this->pbKMap != NULL )
    {
        if ( pbOldMap != NULL )
        {
            UnmapViewOfFile( pbOldMap );
        }

        this->cbKMap = GetFileSize(this->hKFile, NULL);
    }
    else
    {
        this->pbKMap = pbOldMap;
    }

    CloseHandle(hMappedFile);

    if ( ( pbOldMap == NULL ) && ( this->pbKMap == NULL ) )
    {
        return(FALSE);
    }

    return(TRUE);
}

BOOL cBFile_::RemapData(void)
{
    LPBYTE pbOldMap;
    HANDLE hMappedFile;
    DWORD  PageAccess = PAGE_READWRITE;
    DWORD  FileMapAccess = FILE_MAP_WRITE;

    pbOldMap = this->pbDMap;

    if ( fReadOnly == TRUE )
    {
        PageAccess = PAGE_READONLY;
        FileMapAccess = FILE_MAP_READ;
    }

    hMappedFile = CreateFileMapping(this->hDFile, NULL, PageAccess, 0, 0, NULL);

    if (!(hMappedFile) || (hMappedFile == INVALID_HANDLE_VALUE))
    {
        return(TRUE);   // could be the first call!
    }

    this->pbDMap = (BYTE *)MapViewOfFile(hMappedFile, FileMapAccess, 0, 0, 0);
    if ( this->pbDMap != NULL )
    {
        if ( pbOldMap != NULL )
        {
            UnmapViewOfFile( pbOldMap );
        }

        this->cbDMap = GetFileSize(this->hDFile, NULL);
    }
    else
    {
        this->pbDMap = pbOldMap;
    }

    CloseHandle(hMappedFile);

    if ( ( pbOldMap == NULL ) && ( this->pbDMap == NULL ) )
    {
        return(FALSE);
    }

    return(TRUE);
}

void cBFile_::UnmapAll(void)
{
    if (this->pbKMap != NULL)
    {
        UnmapViewOfFile(this->pbKMap);
        this->pbKMap = NULL;
    }

    if (this->pbDMap != NULL)
    {
        UnmapViewOfFile(this->pbDMap);
        this->pbDMap = NULL;
    }
}
