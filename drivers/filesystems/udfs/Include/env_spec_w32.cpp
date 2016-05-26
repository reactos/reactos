////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#ifndef UDF_FORMAT_MEDIA
ULONG   LockMode       = 0;
BOOLEAN opt_invalidate_volume = FALSE;
#endif //UDF_FORMAT_MEDIA

#ifndef CDRW_W32
#ifndef UDF_FORMAT_MEDIA
BOOLEAN open_as_device = FALSE;
#endif //UDF_FORMAT_MEDIA
#ifdef USE_SKIN_MODEL

PSKIN_API SkinAPI = NULL;

PSKIN_API
SkinLoad(
    PWCHAR path,
    HINSTANCE hInstance,      // handle to current instance
    HINSTANCE hPrevInstance,  // handle to previous instance
    int nCmdShow              // show state
    )
{
    HMODULE hm;
    PSKIN_API Skin;
    PSKIN_API (__stdcall *SkinInit) (VOID);

    hm = LoadLibraryW(path);
    if(!hm)
        return NULL;
    SkinInit = (PSKIN_API(__stdcall *)(void))GetProcAddress(hm, "SkinInit");
    if(!SkinInit)
        return NULL;
    Skin = SkinInit();
    if(!Skin)
        return NULL;
    Skin->Init(hInstance, hPrevInstance, nCmdShow);
    return Skin;
}


#endif //USE_SKIN_MODEL

#ifdef _BROWSE_UDF_
#ifndef LIBUDF

extern PVCB Vcb;

#endif // LIBUDF
#endif //_BROWSE_UDF_

#ifdef LIBUDF
#define _lphUdf  ((PUDF_VOL_HANDLE_I)(DeviceObject->lpContext))
#endif //LIBUDF
#ifdef LIBUDFFMT
#define _lphUdf  (DeviceObject->cbio)
#endif //LIBUDFFMT

#ifndef CDRW_W32

NTSTATUS
UDFPhSendIOCTL(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer ,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer ,
    IN ULONG OutputBufferLength,
    IN BOOLEAN OverrideVerify,
    OUT PVOID Iosb OPTIONAL
    )
{
    ULONG real_read;
#if !defined(LIBUDF) && !defined(LIBUDFFMT)
    ULONG ret;

    ULONG RC = DeviceIoControl(DeviceObject->h,IoControlCode,
                                InputBuffer,InputBufferLength,
                                OutputBuffer,OutputBufferLength,
                                &real_read,NULL);

    if (!RC) {
        ret = GetLastError();
    }
    return RC ? 1 : -1;

#else // LIBUDF

    ULONG RC = _lphUdf->lpIOCtlFunc(_lphUdf->lpParameter,IoControlCode,
                                InputBuffer,InputBufferLength,
                                OutputBuffer,OutputBufferLength,
                                &real_read);

    return RC;

#endif // LIBUDF

} // end UDFPhSendIOCTL()


NTSTATUS
UDFPhReadSynchronous(
    PDEVICE_OBJECT DeviceObject,  // the physical device object
    PVOID          Buffer,
    ULONG          Length,
    LONGLONG       Offset,
    PULONG         ReadBytes,
    ULONG          Flags
    )
{

#if !defined(LIBUDF) && !defined(LIBUDFFMT)

    NTSTATUS    RC;
//    KdPrint(("UDFPhRead: Length: %x Lba: %lx\n",Length>>0xb,Offset>>0xb));
    LONG HiOffs = (ULONG)(Offset >> 32);

    RC = SetFilePointer(DeviceObject->h,(ULONG)Offset,&HiOffs,FILE_BEGIN);
    if(RC == INVALID_SET_FILE_POINTER) {
        if(GetLastError() != NO_ERROR) {
            KdPrint(("UDFPhReadSynchronous: error %x\n", GetLastError()));
            return STATUS_END_OF_FILE;
        }
    }
    RC = ReadFile(DeviceObject->h,Buffer,Length,ReadBytes,NULL);
    if(NT_SUCCESS(RC) &&
        (!(*ReadBytes))) {
        RC = GetLastError();
        return STATUS_END_OF_FILE;
    }
    return STATUS_SUCCESS;

#else // LIBUDF

    return _lphUdf->lpReadFunc(_lphUdf->lpParameter,
                              Buffer,
                              Length,
                              Offset,
                              ReadBytes);

#endif //defined LIBUDF || defined LIBUDFFMT

} // end UDFPhReadSynchronous()


NTSTATUS
UDFPhWriteSynchronous(
    PDEVICE_OBJECT     DeviceObject,  // the physical device object
    PVOID          Buffer,
    ULONG          Length,
    LONGLONG       Offset,
    PULONG         WrittenBytes,
    ULONG          Flags
    )
{
#if !defined(LIBUDF) && !defined(LIBUDFFMT)

    NTSTATUS    RC = STATUS_SUCCESS;
    LONG HiOffs = (ULONG)(Offset >> 32);
    PVOID Buffer2 = NULL;
    PVOID Buffer3 = NULL;

    RC = SetFilePointer(DeviceObject->h,(ULONG)Offset,&HiOffs,FILE_BEGIN);
    if(RC == INVALID_SET_FILE_POINTER) {
        if(GetLastError() != NO_ERROR) {
            KdPrint(("UDFPhWriteSynchronous: error %x\n", GetLastError()));
            return STATUS_END_OF_FILE;
        }
    }

    Buffer2 = ExAllocatePool(NonPagedPool, Length+0x10000);
    Buffer3 = (PVOID)( ((ULONG)Buffer2 + 0xffff) & ~0xffff);
    RtlCopyMemory(Buffer3, Buffer, Length);

    RC = WriteFile(DeviceObject->h,Buffer3,Length,WrittenBytes,NULL);
    if(!RC ||
        !(*WrittenBytes)) {
        RC = GetLastError();
        KdPrint(("UDFPhWriteSynchronous: EOF, error %x\n", RC));
        RC = STATUS_END_OF_FILE;
    } else {
        RC = STATUS_SUCCESS;
    }

    if(Buffer2) ExFreePool(Buffer2);

    return RC;

#else // LIBUDF

    return _lphUdf->lpWriteFunc(_lphUdf->lpParameter,
                              Buffer,
                              Length,
                              Offset,
                              WrittenBytes);

#endif // LIBUDF

} // end UDFPhWriteSynchronous()

#if 0
NTSTATUS
UDFPhWriteVerifySynchronous(
    PDEVICE_OBJECT  DeviceObject,   // the physical device object
    PVOID           Buffer,
    ULONG           Length,
    LONGLONG        Offset,
    PULONG          WrittenBytes,
    ULONG           Flags
    )
{
    NTSTATUS RC;
    PUCHAR v_buff = NULL;
    ULONG ReadBytes;

    RC = UDFPhWriteSynchronous(DeviceObject, Buffer, Length, Offset, WrittenBytes, 0);
    if(!Verify)
        return RC;
    v_buff = (PUCHAR)DbgAllocatePool(NonPagedPool, Length);
    if(!v_buff)
        return RC;

    RC = UDFPhSendIOCTL( IOCTL_CDRW_SYNC_CACHE, DeviceObject,
                    NULL,0, NULL,0, FALSE, NULL);

    RC = UDFPhReadSynchronous(DeviceObject, v_buff, Length, Offset, &ReadBytes, 0);
    if(!NT_SUCCESS(RC)) {
        BrutePoint();
        DbgFreePool(v_buff);
        return RC;
    }
    if(RtlCompareMemory(v_buff, Buffer, ReadBytes) == Length) {
        DbgFreePool(v_buff);
        return RC;
    }
    BrutePoint();
    DbgFreePool(v_buff);
    return STATUS_LOST_WRITEBEHIND_DATA;
} // end UDFPhWriteVerifySynchronous()
#endif

VOID
set_image_size(
    HANDLE h,
//    ULONG LBA)
    int64  len)
{
    LONG offh = (ULONG)(len >> 32);
                        //( (LONGLONG)LBA >> (32-Vcb->BlockSizeBits) );

    SetFilePointer((HANDLE)h, (ULONG)(len /*(LBA << Vcb->BlockSizeBits)*/ ), &offh, FILE_BEGIN);
    SetEndOfFile(h);
    offh = 0;
    SetFilePointer((HANDLE)h, 0, &offh, FILE_BEGIN);
} // end set_image_size()

int64
get_file_size(
    HANDLE h
    )
{
    LONG hsz = 0;
    LONG lsz;

    lsz = SetFilePointer(h, 0, &hsz, FILE_END);
    return (((int64)hsz) << 32) | lsz;
} // end get_file_size()

int64
set_file_pointer(
    HANDLE h,
    int64 sz
    )
{
    ULONG hsz = (ULONG)(sz >> 32);
    ULONG lsz = (ULONG)sz;

    lsz = SetFilePointer(h, lsz, (PLONG)&hsz, FILE_BEGIN);
    return (((int64)hsz) << 32) | lsz;
} // end set_file_pointer()

#endif //CDRW_W32

#ifndef LIBUDF

#ifndef UDF_FORMAT_MEDIA

ULONG
write(
    PVCB Vcb,
    HANDLE h,
    PCHAR buff,
    ULONG len)
{
    ULONG written;
    LONG offh = 0;
    ULONG offl = SetFilePointer((HANDLE)h, 0, &offh, FILE_CURRENT);
//    ULONG Lba = (ULONG)((((LONGLONG)offh << 32) + offl) >> Vcb->BlockSizeBits);

    UDFWriteData(Vcb, FALSE, (((LONGLONG)offh)<<32)+offl, len, FALSE, buff, &written);

    SetFilePointer((HANDLE)h, offl, &offh, FILE_BEGIN);
    offh = 0;
    SetFilePointer((HANDLE)h, written, &offh, FILE_CURRENT);

    return written;
} // end write()
#endif //UDF_FORMAT_MEDIA

#endif // LIBUDF

#endif //CDRW_W32

#ifdef NT_NATIVE_MODE

BOOL
Privilege(
    LPTSTR pszPrivilege, 
    BOOL bEnable
    )
{
#ifndef NT_NATIVE_MODE
    HANDLE           hToken;
    TOKEN_PRIVILEGES tp;

    // obtain the token, first check the thread and then the process
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, TRUE, &hToken)) {
        if (GetLastError() == ERROR_NO_TOKEN) {
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    // get the luid for the privilege
    if (!LookupPrivilegeValue(NULL, pszPrivilege, &tp.Privileges[0].Luid)) {
        CloseHandle(hToken);
        return FALSE;
    }

    tp.PrivilegeCount = 1;

    if (bEnable)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // enable or disable the privilege
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, 0)) {
        CloseHandle(hToken);
        return FALSE;
    }

    if (!CloseHandle(hToken))
        return FALSE;

#endif //NT_NATIVE_MODE

    return TRUE;

} // end Privilege()
#endif //NT_NATIVE_MODE

#ifndef LIBUDF

extern "C"
ULONG
MyLockVolume(
    HANDLE h,
    ULONG* pLockMode // OUT
    )
{
    ULONG RC;
    ULONG returned;

    (*pLockMode) = -1;
#ifndef CDRW_W32
    RC = DeviceIoControl(h,IOCTL_UDF_LOCK_VOLUME_BY_PID,NULL,0,NULL,0,&returned,NULL);
    if(RC) {
        (*pLockMode) = IOCTL_UDF_LOCK_VOLUME_BY_PID;
        return STATUS_SUCCESS;
    }
#endif //CDRW_W32

    RC = DeviceIoControl(h,FSCTL_LOCK_VOLUME,NULL,0,NULL,0,&returned,NULL);
    if(RC) {
        (*pLockMode) = FSCTL_LOCK_VOLUME;
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
} // MyLockVolume()

extern "C"
ULONG
MyUnlockVolume(
    HANDLE h,
    ULONG* pLockMode // IN
    )
{
    ULONG returned;

#ifndef CDRW_W32
    if((*pLockMode) == IOCTL_UDF_LOCK_VOLUME_BY_PID) {
        return DeviceIoControl(h,IOCTL_UDF_UNLOCK_VOLUME_BY_PID,NULL,0,NULL,0,&returned,NULL);
    }
#endif //CDRW_W32

    return DeviceIoControl(h,FSCTL_UNLOCK_VOLUME,NULL,0,NULL,0,&returned,NULL);

} // MyUnlockVolume()

void
my_retrieve_vol_type(
#ifndef CDRW_W32
    PVCB Vcb,
#endif
    PWCHAR fn
    )
{
#ifndef CDRW_W32
    if(wcslen(fn) == 2 && fn[1] == ':') {
        ULONG DevType = GetDriveTypeW(fn);
        KdPrint(("  DevType %x\n", DevType));
        switch(DevType) {
        case DRIVE_CDROM:
            Vcb->PhDeviceType = FILE_DEVICE_CD_ROM;
            break;
        default:
            Vcb->PhDeviceType = FILE_DEVICE_DISK;
            break;
        }
    }
    if(wcslen(fn) == 2 && fn[1] == ';') {
        UserPrint(("Warrning: File name is similar to drive letter.\n"
                   "  Don't you type semicolon ';' instead of colon ':' ?\n"));
    }
#endif //CDRW_W32
} // end my_retrieve_vol_type()


#ifdef NT_NATIVE_MODE
#define GetLastError()    ((ULONG)(-1))
#endif //NT_NATIVE_MODE

#define MAX_INVALIDATE_VOLUME_RETRY 8

extern "C"
HANDLE
my_open(
#ifndef CDRW_W32
    PVCB Vcb,
#endif
    PWCHAR fn
    )
{
    HANDLE h/*, h2*/;
    WCHAR deviceNameBuffer[0x200];
    WCHAR FSNameBuffer[0x200];
//    CCHAR RealDeviceName[0x200];
//    WCHAR DeviceName[MAX_PATH+1];
    ULONG RC;
    ULONG retry;
    ULONG i;
    BOOLEAN CantLock = FALSE;
    PULONG pLockMode;
#ifdef NT_NATIVE_MODE
    IO_STATUS_BLOCK ioStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING uniFilename;
#endif //NT_NATIVE_MODE
    ULONG returned;

#ifndef CDRW_W32
#ifdef UDF_FORMAT_MEDIA
    PUDFFmtState fms = Vcb->fms;
    fms->
#endif
        open_as_device = TRUE;
#endif //CDRW_W32

    pLockMode = &
#ifdef UDF_FORMAT_MEDIA
        fms->
#endif
        LockMode;

    // make several retries to workaround smart applications,
    // those attempts to work with volume immediately after arrival
    retry = 1 +
#ifdef UDF_FORMAT_MEDIA
        fms->
#endif
            opt_invalidate_volume ? 0 : MAX_INVALIDATE_VOLUME_RETRY;

#ifndef NT_NATIVE_MODE
    swprintf(deviceNameBuffer, L"%ws\\", fn);
    KdPrint(("my_open: %S\n", fn));
    i = sizeof(FSNameBuffer)/sizeof(FSNameBuffer[0]);
    if(GetVolumeInformationW(deviceNameBuffer, NULL, 0, 
        &returned, &returned, &returned, FSNameBuffer, i)) {
        KdPrint(("my_open: FS: %S\n", FSNameBuffer));
        if(!wcscmp(FSNameBuffer, L"Unknown")) {
            retry++;
        }
    } else {
        KdPrint(("my_open: FS: ???\n"));
    }
    KdPrint(("my_open: retry %d times\n", retry));

#endif //NT_NATIVE_MODE

    do {
    // open as device
#ifndef NT_NATIVE_MODE
    swprintf(deviceNameBuffer, L"\\\\.\\%ws", fn);
    if(wcslen(fn) == 2 && fn[1] == ';') {
        UserPrint(("Warrning: File name is similar to drive letter.\n"
                   "  Don't you type semicolon ';' instead of colon ':' ?\n"));
    }
    h = (HANDLE)(-1);
    for(i=0; i<4; i++) {
        if(h == ((HANDLE)-1)) {
            h = CreateFileW(deviceNameBuffer, GENERIC_READ | GENERIC_WRITE,
                           ((i & 1) ? 0 : FILE_SHARE_READ) | ((i & 2) ? 0 : FILE_SHARE_WRITE),
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,  NULL);
            if(h != ((HANDLE)-1)) {
                KdPrint(("  opened i=%x\n", i));
            }
        }
    }
#else //NT_NATIVE_MODE
    uniFilename.Length = swprintf(deviceNameBuffer, L"\\??\\%ws", fn);
    uniFilename.Buffer = deviceNameBuffer;
    uniFilename.Length *= sizeof(WCHAR);
    uniFilename.MaximumLength = uniFilename.Length + sizeof(WCHAR);

    h = (HANDLE)(-1);
    for(i=0; i<4; i++) {
        InitializeObjectAttributes(&ObjectAttributes, &uniFilename, OBJ_CASE_INSENSITIVE, NULL, NULL);
        if(h == ((HANDLE)-1)) {
            RC = NtCreateFile(&h,
                                     GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                                     &ObjectAttributes,
                                     &ioStatus,
                                     NULL,
                                     FILE_ATTRIBUTE_NORMAL,
                                     ((i & 1) ? 0 : FILE_SHARE_READ) | ((i & 2) ? 0 : FILE_SHARE_WRITE),
                                     FILE_OPEN,
                                     FILE_SYNCHRONOUS_IO_NONALERT | FILE_COMPLETE_IF_OPLOCKED | FILE_WRITE_THROUGH | FILE_NO_INTERMEDIATE_BUFFERING,
                                     NULL,
                                     0);
            if(!NT_SUCCESS(RC)) {
                KdPrint(("  opened i2=%x\n", i));
                h = ((HANDLE)-1);
            }
        }
    }
#endif //NT_NATIVE_MODE
    if(h != ((HANDLE)-1)) {
#ifndef CDRW_W32
#ifdef UDF_FORMAT_MEDIA
        if(fms->opt_flush || fms->opt_probe) {
            return h;
        }
#endif //UDF_FORMAT_MEDIA
        my_retrieve_vol_type(Vcb, fn);
#else
        my_retrieve_vol_type(fn);
#endif //CDRW_W32
        if(!NT_SUCCESS(MyLockVolume(h,pLockMode))) {
#ifndef CDRW_W32
            if(retry < MAX_INVALIDATE_VOLUME_RETRY) {
                retry++;
                if(!Privilege(SE_TCB_NAME, TRUE)) {
                    KdPrint(("SE_TCB privilege not held\n"));
                } else
                if(DeviceIoControl(h,FSCTL_INVALIDATE_VOLUMES,&h,sizeof(h),NULL,0,&returned,NULL) ) {
                    KdPrint(("  FSCTL_INVALIDATE_VOLUMES ok, status %x\n", GetLastError()));
                    CloseHandle(h);
                    continue;
                } else {
//#ifndef CDRW_W32
                    KdPrint(("  FSCTL_INVALIDATE_VOLUMES failed, error %x\n", GetLastError()));
                    RC = GetLastError();
                    if(DeviceIoControl(h,IOCTL_UDF_INVALIDATE_VOLUMES,&h,sizeof(h),NULL,0,&returned,NULL) ) {
                        KdPrint(("  IOCTL_UDF_INVALIDATE_VOLUMES ok, status %x\n", GetLastError()));
                        CloseHandle(h);
                        continue;
                    }
                    KdPrint(("  IOCTL_UDF_INVALIDATE_VOLUMES, error %x\n", GetLastError()));
//#endif //CDRW_W32
                }
                UserPrint(("can't lock volume, retry\n"));
                CloseHandle(h);
                continue;
            }
#endif //CDRW_W32
            UserPrint(("can't lock volume\n"));
#ifndef NT_NATIVE_MODE
            // In native mode the volume can be not mounted yet !!!
            CantLock = TRUE;
            CloseHandle(h);
            h = NULL;
            goto try_as_file;
#endif //NT_NATIVE_MODE
        }
//#ifndef CDRW_W32
        if(!DeviceIoControl(h,FSCTL_ALLOW_EXTENDED_DASD_IO,NULL,0,NULL,0,&returned,NULL)) {
            KdPrint(("Warning: can't allow extended DASD i/o\n"));
        }
//#endif //CDRW_W32

        KdPrint(("  opened, h=%x\n", h));
        return h;
    }
    RC = GetLastError();

#ifndef NT_NATIVE_MODE
    h = CreateFileW(deviceNameBuffer, GENERIC_READ,
                   FILE_SHARE_READ,
                   NULL,
                   OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL,  NULL);
#else //NT_NATIVE_MODE
    RC = NtCreateFile(&h,
                             GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                             &ObjectAttributes,
                             &ioStatus,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ,
                             FILE_OPEN,
                             FILE_SYNCHRONOUS_IO_NONALERT | FILE_COMPLETE_IF_OPLOCKED | FILE_WRITE_THROUGH,
                             NULL,
                             0);
    if(!NT_SUCCESS(RC)) {
        h = ((HANDLE)-1);
    }
#endif //NT_NATIVE_MODE
    if(h != ((HANDLE)-1)) {

        KdPrint(("  opened R/O, h=%x\n", h));
#ifndef CDRW_W32
        my_retrieve_vol_type(Vcb, fn);
#else
        my_retrieve_vol_type(fn);
#endif

        UserPrint(("read-only open\n"));
        if(!NT_SUCCESS(MyLockVolume(h,pLockMode))) {
#ifndef CDRW_W32
            if(retry < MAX_INVALIDATE_VOLUME_RETRY) {
                retry++;
                if(!Privilege(SE_TCB_NAME, TRUE)) {
                    KdPrint(("SE_TCB privilege not held\n"));
                } else
                if(DeviceIoControl(h,FSCTL_INVALIDATE_VOLUMES,&h,sizeof(h),NULL,0,&returned,NULL) ) {
                    CloseHandle(h);
                    continue;
                }
                UserPrint(("can't lock read-only volumem retry"));
                CloseHandle(h);
                continue;
            }
#endif //CDRW_W32
            UserPrint(("can't lock read-only volume"));
#ifndef NT_NATIVE_MODE
            CantLock = TRUE;
            CloseHandle(h);
            h = NULL;
            goto try_as_file;
#endif //NT_NATIVE_MODE
        }
//        write_cdfs = TRUE;
//        DeviceIoControl(h,FSCTL_DISMOUNT_VOLUME,NULL,0,NULL,0,&returned,NULL);
        return h;
    }
#ifndef NT_NATIVE_MODE
try_as_file:
#endif //NT_NATIVE_MODE

#ifndef CDRW_W32
#ifdef UDF_FORMAT_MEDIA
    fms->
#endif
    open_as_device = FALSE;
    // open as plain file
    Vcb->PhDeviceType = FILE_DEVICE_DISK;
#endif //CDRW_W32

    UserPrint(("try image file\n"));
#ifndef NT_NATIVE_MODE
    h = CreateFileW(fn, GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ,
                   NULL,
                   CREATE_ALWAYS,
                   FILE_ATTRIBUTE_NORMAL,  NULL);
#else //NT_NATIVE_MODE
    RC = NtCreateFile(&h,
                             GENERIC_READ | SYNCHRONIZE,
                             &ObjectAttributes,
                             &ioStatus,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ,
                             FILE_OPEN,
                             FILE_SYNCHRONOUS_IO_NONALERT | FILE_COMPLETE_IF_OPLOCKED | FILE_WRITE_THROUGH,
                             NULL,
                             0);
    if(!NT_SUCCESS(RC)) {
        h = ((HANDLE)-1);
    }
#endif //NT_NATIVE_MODE
    if(h == ((HANDLE)-1)) {

        RC = GetLastError();
        if(CantLock) {
#ifndef CDRW_W32
            my_exit(
#ifdef UDF_FORMAT_MEDIA
                fms,
#endif
                MKUDF_CANT_LOCK_VOL);
#else
            return NULL;
#endif //CDRW_W32
        }
#ifndef CDRW_W32
        UserPrint(("error opening device or image file"));
        my_exit(
#ifdef UDF_FORMAT_MEDIA
                fms,
#endif
            MKUDF_CANT_OPEN_FILE);
#else
        return NULL;
#endif //CDRW_W32
    }
    KdPrint(("  opened as file, h=%x\n", h));
    break;

    } while(TRUE);
    return h;
} // end my_open()

#endif //LIBUDF

#ifndef CDRW_W32

uint64
udf_lseek64(
    HANDLE fd,
    uint64 offset,
    int whence)
{
    LONG offh = (ULONG)(offset>>32);
    LONG offl;
    offl = SetFilePointer(fd, (ULONG)offset, &offh, whence);
    if(offl == -1 && offh == -1) {
        return -1;
    }
    return (((uint64)offh) << 32) | (uint64)offl;
} // end udf_lseek64()

#ifdef LIBUDFFMT
BOOLEAN
udf_get_sizes(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG* blocks
    )
{
    ULONG bs;
    int64 sz;
    ULONG RC;

    RC = _lphUdf->lpGetSizeFunc(_lphUdf->lpParameter, &sz, &bs);

    (*blocks) = (ULONG)(sz/bs);

    return(OS_SUCCESS(RC));
}
#endif //LIBUDFFMT
    
#include "string_lib.cpp"

#ifdef _BROWSE_UDF_
#ifndef LIBUDF

ULONG
UDFGetDevType(
    PDEVICE_OBJECT DeviceObject
    )
{
    if(DeviceObject && DeviceObject == Vcb->TargetDeviceObject) {
        return Vcb->PhDeviceType;
    }
    return FILE_DEVICE_DISK;
} // end UDFGetDevType()

#else  // LIBUDF

ULONG
UDFGetDevType(
    PDEVICE_OBJECT DeviceObject
    )
{
#define lphUdf  ((PUDF_VOL_HANDLE_I)(DeviceObject->lpContext))
    return lphUdf->bHddDevice ? FILE_DEVICE_DISK : FILE_DEVICE_CD_ROM;
#undef lphUdf
} // end UDFGetDevType()

#endif // LIBUDF

#endif //_BROWSE_UDF_

#endif //CDRW_W32

#ifndef NT_NATIVE_MODE

#ifdef PRINT_DBG_CONSOLE
CHAR dbg_print_tmp_buff[2048];

BOOLEAN was_enter = TRUE;

extern "C"
VOID
PrintDbgConsole(
    PCHAR DebugMessage,
    ...
    )
{
    int len;
    va_list ap;
    va_start(ap, DebugMessage);

    if(was_enter) {
        strcpy(&dbg_print_tmp_buff[0], JS_DBG_PREFIX);
        len = _vsnprintf(&dbg_print_tmp_buff[sizeof(JS_DBG_PREFIX)-1], 2047-sizeof(JS_DBG_PREFIX), DebugMessage, ap);
    } else {
        len = _vsnprintf(&dbg_print_tmp_buff[0], 2047, DebugMessage, ap);
    }
    dbg_print_tmp_buff[2047] = 0;
    if(len > 0 &&
       (dbg_print_tmp_buff[len-1] == '\n' ||
        dbg_print_tmp_buff[len-1] == '\r') ) {
        was_enter = TRUE;
    } else {
        was_enter = FALSE;
    }

    OutputDebugString(&dbg_print_tmp_buff[0]);

    va_end(ap);

} // end PrintDbgConsole()
#else // PRINT_DBG_CONSOLE
VOID
PrintDbgConsole(
    PCHAR DebugMessage,
    ...
    )
{
} // end ClassDebugPrint()
#endif //PRINT_DBG_CONSOLE

BOOLEAN
RtlTimeFieldsToTime(
    IN PTIME_FIELDS TimeFields,
    IN PLARGE_INTEGER Time
    )
{
    SYSTEMTIME st;

    st.wYear         = TimeFields->Year;
    st.wMonth        = TimeFields->Month;
    st.wDayOfWeek    = 0;
    st.wDay          = TimeFields->Day;
    st.wHour         = TimeFields->Hour;
    st.wMinute       = TimeFields->Minute;
    st.wSecond       = TimeFields->Second;
    st.wMilliseconds = TimeFields->Milliseconds;

    return SystemTimeToFileTime(&st, (PFILETIME)Time);
} // end RtlTimeFieldsToTime()

BOOLEAN
RtlTimeToTimeFields(
    IN PLARGE_INTEGER Time,
    IN PTIME_FIELDS TimeFields
    )
{
    SYSTEMTIME st;
    BOOLEAN retval;

    retval = FileTimeToSystemTime((PFILETIME)Time, &st);

    TimeFields->Year         = st.wYear;
    TimeFields->Month        = st.wMonth;
    TimeFields->Weekday      = st.wDayOfWeek;
    TimeFields->Day          = st.wDay;
    TimeFields->Hour         = st.wHour;
    TimeFields->Minute       = st.wMinute;
    TimeFields->Second       = st.wSecond;
    TimeFields->Milliseconds = st.wMilliseconds;

    return retval;
} // end ()

#endif //NT_NATIVE_MODE

#ifdef USE_THREAD_HEAPS

HANDLE MemLock = NULL;

VOID
ExInitThreadPools()
{
    MemLock = CreateMutex(NULL, 0, NULL);
}

VOID
ExDeInitThreadPools()
{
    if(MemLock)
        CloseHandle(MemLock);
}

#define MAX_THREADS_WITH_OWN_POOL   128

typedef struct _THREAD_POOL_LIST_ITEM {
    HANDLE HeapHandle;
    ULONG  ThreadId;
} THREAD_POOL_LIST_ITEM, *PTHREAD_POOL_LIST_ITEM;

ULONG LastThreadPool = -1;
THREAD_POOL_LIST_ITEM ThreadPoolList[MAX_THREADS_WITH_OWN_POOL];

extern "C"
PVOID
#ifdef KERNEL_MODE_MM_BEHAVIOR
_ExAllocatePool_(
#else
ExAllocatePool(
#endif
    ULONG MemoryType,
    ULONG Size
    )
{
    ULONG i;
    ULONG ThreadId = GetCurrentThreadId();
    BOOLEAN found = FALSE;

    WaitForSingleObject(MemLock,-1);

    for(i=0; i<(LastThreadPool+1); i++) {
        if(ThreadPoolList[i].ThreadId == ThreadId) {
            found = TRUE;
            break;
        }
    }
    if(found) {
        ReleaseMutex(MemLock);
        return HeapAlloc(ThreadPoolList[i].HeapHandle, HEAP_NO_SERIALIZE, Size);
    }
    for(i=0; i<(LastThreadPool+1); i++) {
        if(ThreadPoolList[i].ThreadId == -1) {
            break;
        }
    }
    if(i>=MAX_THREADS_WITH_OWN_POOL) {
        ReleaseMutex(MemLock);
        return NULL;
    }
    ThreadPoolList[i].ThreadId   = ThreadId;
    ThreadPoolList[i].HeapHandle = HeapCreate(HEAP_NO_SERIALIZE, 128*PAGE_SIZE, 0);
    if(!ThreadPoolList[i].HeapHandle) {
        ThreadPoolList[i].ThreadId = -1;
        ReleaseMutex(MemLock);
        return NULL;
    }

    if(i+1 > LastThreadPool+1)
        LastThreadPool = i;

    ReleaseMutex(MemLock);
        
    return HeapAlloc(ThreadPoolList[i].HeapHandle, HEAP_NO_SERIALIZE, Size);

} // end ExAllocatePool()

extern "C"
VOID
#ifdef KERNEL_MODE_MM_BEHAVIOR
_ExFreePool_(
#else
ExFreePool(
#endif
    PVOID Addr
    )
{
    ULONG ThreadId = GetCurrentThreadId();
    ULONG i;

    WaitForSingleObject(MemLock,-1);
    for(i=0; i<(LastThreadPool+1); i++) {
        if(ThreadPoolList[i].ThreadId == ThreadId) {
            break;
        }
    }
    if(i+1 > LastThreadPool+1) {
        // Not found
        BrutePoint();
        //__asm int 3;
        return;
    }
    HeapFree(ThreadPoolList[i].HeapHandle, HEAP_NO_SERIALIZE, Addr);

    ReleaseMutex(MemLock);

} // end ExFreePool()

extern "C"
VOID
ExFreeThreadPool()
{
    ULONG ThreadId = GetCurrentThreadId();
    ULONG i;

    WaitForSingleObject(MemLock,-1);
    for(i=0; i<(LastThreadPool+1); i++) {
        if(ThreadPoolList[i].ThreadId == ThreadId) {
            break;
        }
    }
    if(i+1 > LastThreadPool+1) {
        // Not found
        BrutePoint();
        //__asm int 3;
        return;
    }
    HeapDestroy(ThreadPoolList[i].HeapHandle);
    ThreadPoolList[i].HeapHandle = INVALID_HANDLE_VALUE;
    ThreadPoolList[i].ThreadId   = -1;

    ReleaseMutex(MemLock);
}

#endif //USE_THREAD_HEAPS

#if defined(KERNEL_MODE_MM_BEHAVIOR)
extern "C"
PVOID
ExAllocatePool(
    ULONG MemoryType,
    ULONG Size
    )
{
    PVOID Addr;
    PVOID uAddr;
    if(Size < PAGE_SIZE) {
#ifdef USE_THREAD_HEAPS
        Addr = _ExAllocatePool_(MemoryType, Size+8);
#else
        Addr = GlobalAlloc(GMEM_DISCARDABLE, Size+8);
#endif
        if(!Addr)
            return NULL;
        uAddr = ((PCHAR)Addr)+8;
    } else {
#ifdef USE_THREAD_HEAPS
        Addr = _ExAllocatePool_(MemoryType, Size+PAGE_SIZE*2);
#else
        Addr = GlobalAlloc(GMEM_DISCARDABLE, Size+PAGE_SIZE*2);
#endif
        if(!Addr)
            return NULL;
        uAddr = (PVOID)(((ULONG)(((PCHAR)Addr)+PAGE_SIZE)) & ~(PAGE_SIZE-1));
    }
    *(((PULONG)uAddr)-2) = (ULONG)Addr;
    *(((PULONG)uAddr)-1) = 0xFEDCBA98;
    return uAddr;
} // end ExAllocatePool()

extern "C"
VOID
ExFreePool(
    PVOID uAddr
    )
{
    PVOID Addr;

    if(*(((PULONG)uAddr)-1) == 0xFEDCBA98) {
        Addr = (PVOID)(*(((PULONG)uAddr)-2));
#ifdef USE_THREAD_HEAPS
        _ExFreePool_(Addr);
#else
        GlobalFree(Addr);
#endif
        return;
    }
    BrutePoint();
} // end ExFreePool()
#endif //defined(KERNEL_MODE_MM_BEHAVIOR) || defined(NT_NATIVE_MODE)

#ifdef _lphUdf
#undef _lphUdf
#endif //_lphUdf

extern "C"
BOOLEAN
ProbeMemory(
    PVOID   MemPtr,
    ULONG   Length,
    BOOLEAN ForWrite
    )
{
    ULONG i;
    UCHAR a;
    if(!MemPtr && !Length)
        return TRUE;
    if(!MemPtr || !Length)
        return FALSE;
    _SEH2_TRY {
        a = ((PCHAR)MemPtr)[Length-1];
        if(ForWrite) {
            ((PCHAR)MemPtr)[Length-1] = a;
        }
        for(i=0; i<Length; i+=PAGE_SIZE) {
            a = ((PCHAR)MemPtr)[i];
            if(ForWrite) {
                ((PCHAR)MemPtr)[i] = a;
            }
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    } _SEH2_END;
    return TRUE;
} // end ProbeMemory()

#ifdef NT_NATIVE_MODE
#include "env_spec_nt.cpp"
#endif //NT_NATIVE_MODE
