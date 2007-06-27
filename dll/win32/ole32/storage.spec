# Compound Storage DLL.
# (FIXME: some methods are commented out. Commenting them in _WILL_
#  result in dataloss. Do it at your own risk.)

1 pascal StgCreateDocFileA(str long long ptr) StgCreateDocFile16
2 stub StgCreateDocFileOnILockBytes
# 2 pascal StgCreateDocFileOnILockBytes(ptr long long ptr) StgCreateDocFileOnILockBytes16
3 pascal StgOpenStorage(str ptr long ptr long ptr) StgOpenStorage16
4 pascal StgOpenStorageOnILockBytes(segptr ptr long long long ptr) StgOpenStorageOnILockBytes16
5 pascal StgIsStorageFile(str) StgIsStorageFile16
6 pascal StgIsStorageILockBytes(segptr) StgIsStorageILockBytes16
7 stub StgSetTimes
#8 WEP
#9 ___EXPORTEDSTUB
103 stub DllGetClassObject

# Storage Interface functions. Starting at 500
# these are not exported in the real storage.dll, we use them
# as 16->32 relays. They use the cdecl calling convention.

# IStorage
500 cdecl IStorage16_QueryInterface(ptr ptr ptr) IStorage16_fnQueryInterface
501 cdecl IStorage16_AddRef(ptr) IStorage16_fnAddRef
502 cdecl IStorage16_Release(ptr) IStorage16_fnRelease
#503 cdecl IStorage16_CreateStream(ptr str long long long ptr) IStorage16_fnCreateStream
503 stub  IStorage16_CreateStream

504 cdecl IStorage16_OpenStream(ptr str ptr long long ptr) IStorage16_fnOpenStream
#505 cdecl IStorage16_CreateStorage(ptr str long long long ptr) IStorage16_fnCreateStorage
505 stub  IStorage16_CreateStorage
506 cdecl IStorage16_OpenStorage(ptr str ptr long ptr long ptr) IStorage16_fnOpenStorage
507 cdecl IStorage16_CopyTo(ptr long ptr ptr ptr) IStorage16_fnCopyTo
508 stub  IStorage16_MoveElementTo
509 cdecl IStorage16_Commit(ptr long) IStorage16_fnCommit
510 stub  IStorage16_Revert
511 stub  IStorage16_EnumElements
512 stub  IStorage16_DestroyElement
513 stub  IStorage16_RenameElement
514 stub  IStorage16_SetElementTimes
515 stub  IStorage16_SetClass
516 stub  IStorage16_SetStateBits
517 cdecl IStorage16_Stat(ptr ptr long) IStorage16_fnStat

# IStream
518 cdecl IStream16_QueryInterface(ptr ptr ptr) IStream16_fnQueryInterface
519 cdecl IStream16_AddRef(ptr) IStream16_fnAddRef
520 cdecl IStream16_Release(ptr) IStream16_fnRelease
521 cdecl IStream16_Read(ptr ptr long ptr) IStream16_fnRead
#522 cdecl IStream16_Write(ptr ptr long ptr) IStream16_fnWrite
522 stub  IStream16_Write
523 cdecl IStream16_Seek(ptr double long ptr) IStream16_fnSeek
524 stub  IStream16_SetSize
525 stub  IStream16_CopyTo
526 stub  IStream16_Commit
527 stub  IStream16_Revert
528 stub  IStream16_LockRegion
529 stub  IStream16_UnlockRegion
530 stub  IStream16_Stat
531 stub  IStream16_Clone
