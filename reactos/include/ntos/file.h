/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/ps.h
 * PURPOSE:      Filesystem declarations used by all the parts of the 
 *               system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */


#ifndef __INCLUDE_FILE_H
#define __INCLUDE_FILE_H

#ifndef __USE_W32API

#define GENERIC_READ	(0x80000000L)
#define GENERIC_WRITE	(0x40000000L)
#define FILE_READ_DATA            ( 0x0001 )    /* file & pipe */
#define FILE_LIST_DIRECTORY       ( 0x0001 )    /* directory */

#define FILE_WRITE_DATA           ( 0x0002 )    /* file & pipe */
#define FILE_ADD_FILE             ( 0x0002 )    /* directory */

#define FILE_APPEND_DATA          ( 0x0004 )    /* file */
#define FILE_ADD_SUBDIRECTORY     ( 0x0004 )    /* directory */
#define FILE_CREATE_PIPE_INSTANCE ( 0x0004 )    /* named pipe */

#define FILE_READ_EA              ( 0x0008 )    /* file & directory */
#define FILE_READ_PROPERTIES      FILE_READ_EA

#define FILE_WRITE_EA             ( 0x0010 )    /* file & directory */
#define FILE_WRITE_PROPERTIES     FILE_WRITE_EA

#define FILE_EXECUTE              ( 0x0020 )    /* file */
#define FILE_TRAVERSE             ( 0x0020 )    /* directory */

#define FILE_DELETE_CHILD         ( 0x0040 )    /* directory */

#define FILE_READ_ATTRIBUTES      ( 0x0080 )    /* all */

#define FILE_WRITE_ATTRIBUTES     ( 0x0100 )    /* all */

#define FILE_SHARE_DELETE	(4)
#define FILE_SHARE_READ	(1)
#define FILE_SHARE_WRITE	(2)
#define CONSOLE_TEXTMODE_BUFFER	(1)
#define CREATE_NEW	(1)
#define CREATE_ALWAYS	(2)
#define OPEN_EXISTING	(3)
#define OPEN_ALWAYS	(4)
#define TRUNCATE_EXISTING	(5)
#define FILE_ATTRIBUTE_ARCHIVE	(32)
#define FILE_ATTRIBUTE_COMPRESSED	(2048)
#define FILE_ATTRIBUTE_DEVICE   (64)
#define FILE_ATTRIBUTE_NORMAL	(128)
#define FILE_ATTRIBUTE_DIRECTORY	(16)
#define FILE_ATTRIBUTE_ENCRYPTED (16384)
#define FILE_ATTRIBUTE_HIDDEN	(2)
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED (8192)
#define FILE_ATTRIBUTE_OFFLINE  (4096)
#define FILE_ATTRIBUTE_READONLY	(1)
#define FILE_ATTRIBUTE_REPARSE_POINT (1024)
#define FILE_ATTRIBUTE_SPARSE_FILE (512)
#define FILE_ATTRIBUTE_SYSTEM	(4)
#define FILE_ATTRIBUTE_TEMPORARY	(256)
#define FILE_ATTRIBUTE_VALID_FLAGS	(0x00007fb7)
#define FILE_ATTRIBUTE_VALID_SET_FLAGS	(0x000031a7)
#define FILE_FLAG_WRITE_THROUGH		(0x80000000)
#define FILE_FLAG_OVERLAPPED		(0x40000000)
#define FILE_FLAG_NO_BUFFERING		(0x20000000)
#define FILE_FLAG_RANDOM_ACCESS		(0x10000000)
#define FILE_FLAG_SEQUENTIAL_SCAN	(0x08000000)
#define FILE_FLAG_DELETE_ON_CLOSE	(0x04000000)
#define FILE_FLAG_BACKUP_SEMANTICS	(0x02000000)
#define FILE_FLAG_POSIX_SEMANTICS	(0x01000000)

/* GetVolumeInformation */
#define FS_CASE_IS_PRESERVED	(2)
#define FS_CASE_SENSITIVE	(1)
#define FS_UNICODE_STORED_ON_DISK	(4)
#define FS_PERSISTENT_ACLS	(8)
#define FS_FILE_COMPRESSION	(16)
#define FS_VOL_IS_COMPRESSED	(32768)

/* NtQueryVolumeInformationFile */
#define FILE_CASE_SENSITIVE_SEARCH	(0x00000001)
#define FILE_CASE_PRESERVED_NAMES	(0x00000002)
#define FILE_UNICODE_ON_DISK		(0x00000004)
#define FILE_PERSISTENT_ACLS		(0x00000008)
#define FILE_FILE_COMPRESSION		(0x00000010)
#define FILE_VOLUME_IS_COMPRESSED	(0x00008000)

#define FILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1FF)

#define FILE_GENERIC_READ         (STANDARD_RIGHTS_READ     |\
                                   FILE_READ_DATA           |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_READ_EA             |\
                                   SYNCHRONIZE)


#define FILE_GENERIC_WRITE        (STANDARD_RIGHTS_WRITE    |\
                                   FILE_WRITE_DATA          |\
                                   FILE_WRITE_ATTRIBUTES    |\
                                   FILE_WRITE_EA            |\
                                   FILE_APPEND_DATA         |\
                                   SYNCHRONIZE)

#define FILE_GENERIC_EXECUTE      (STANDARD_RIGHTS_EXECUTE  |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_EXECUTE             |\
                                   SYNCHRONIZE)

#endif /* !__USE_W32API */

#endif /* __INCLUDE_FILE_H */
