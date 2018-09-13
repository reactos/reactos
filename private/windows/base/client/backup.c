//
//        Copyright (c) 1991  Microsoft Corporation & Maynard Electornics
//
//        Module Name:
//
//            backup.c
//
//        Abstract:
//
//            This module implements Win32 Backup APIs
//
//        Author:
//
//            Steve DeVos (@Maynard)    2 March, 1992   15:38:24
//
//        Revision History:

#include <basedll.h>
#pragma hdrstop

#include <windows.h>


#define CWCMAX_STREAMNAME        512
#define CB_NAMELESSHEADER        FIELD_OFFSET(WIN32_STREAM_ID, cStreamName)

typedef struct
{
    DWORD BufferSize;
    DWORD AllocSize;
    BYTE *Buffer;
} BUFFER;

//
//  BACKUPCONTEXT is the structure used to note the state of the backup.
//  

typedef struct
{
    //
    //  Public header describing current stream. Since this structure precedes
    //  a variable-length stream name, we must reserve space for that name
    //  following the header.
    //
    
    WIN32_STREAM_ID head;
    union {
         WCHAR            awcName[CWCMAX_STREAMNAME];
    } ex ;

    LARGE_INTEGER    cbSparseOffset ;

    //
    //  Offset in the current segment of the backup stream.  This includes
    //  the size of the above header (including variable length name).
    //

    LONGLONG        liStreamOffset;

    //
    //  BackupRead machine state
    //
    
    DWORD            StreamIndex;
    
    //
    //  Calculated size of the above header.
    //

    DWORD            cbHeader;
    
    //
    //  Handle to alternate data stream
    //

    HANDLE            hAlternate;

    //
    //  Buffers
    //

    BUFFER          DataBuffer;         //  Data buffer
    DWORD           dwSparseMapSize ;   //  size of the sparse file map
    DWORD           dwSparseMapOffset ; //  offset into the sparse map
    BOOLEAN         fSparseBlockStart ; //  TRUE if start of sparse block
    BOOLEAN         fSparseHandAlt  ;   //  TRUE if sparse stream is alt stream

    DWORD           iNameBuffer;        //  Offset into stream name buffer
    BUFFER          StreamNameBuffer;   //  Stream name buffer
    BOOLEAN            NamesReady;         //  TRUE if stream name buffer has data in it
    
    BOOLEAN            fStreamStart;       //  TRUE if start of new stream
    BOOLEAN            fMultiStreamType;   //  TRUE if stream type has > 1 stream hdr
    BOOLEAN            fAccessError;       //  TRUE if access to a stream was denied
    DWORD              fAttribs;           //  object attributes...
    BOOLEAN            fRemoteData;        //  TRUE if object is migrated.
} BACKUPCONTEXT;


//
//  BACKUPIOFRAME describes the current user BackupRead/Write request
//

typedef struct
{
    BYTE   *pIoBuffer;
    DWORD  *pcbTransferred;
    DWORD   cbRequest;
    BOOLEAN fProcessSecurity;
} BACKUPIOFRAME;


#define CBMIN_BUFFER  1024

#define BufferOverflow(s) \
    ((s) == STATUS_BUFFER_OVERFLOW || (s) == STATUS_BUFFER_TOO_SMALL)

int mwStreamList[] =
{
    BACKUP_SECURITY_DATA,
    BACKUP_REPARSE_DATA,
    BACKUP_DATA,
    BACKUP_EA_DATA,
    BACKUP_ALTERNATE_DATA,
    BACKUP_OBJECT_ID,
    BACKUP_INVALID,
};



__inline VOID *
BackupAlloc (DWORD cb)
/*++

Routine Description:

    This is an internal routine that wraps heap allocation with tags.

Arguments:

    cb - size of block to allocate

Return Value:

    pointer to allocated memory or NULL

--*/
{
    return RtlAllocateHeap( RtlProcessHeap( ), MAKE_TAG( BACKUP_TAG ), cb );
}


__inline VOID
BackupFree (IN VOID *pv)
/*++

Routine Description:

    This is an internal routine that wraps heap freeing.

Arguments:

    pv - memory to be freed

Return Value:

    None.

--*/
{
    RtlFreeHeap( RtlProcessHeap( ), 0, pv );
}


BOOL
GrowBuffer (IN OUT BUFFER *Buffer, IN DWORD cbNew)
/*++

Routine Description:

    Attempt to grow the buffer in the backup context.

Arguments:

    Buffer - pointer to buffer
    
    cbNew - size of buffer to allocate

Return Value:

    TRUE if buffer was successfully allocated.

--*/
{
    VOID *pv;

    if ( Buffer->AllocSize < cbNew ) {
         pv = BackupAlloc( cbNew );
    
         if (pv == NULL) {
             SetLastError( ERROR_NOT_ENOUGH_MEMORY );
             return FALSE;                                                     
         }
    
         RtlCopyMemory( pv, Buffer->Buffer, Buffer->BufferSize );
         
         BackupFree( Buffer->Buffer );
    
         Buffer->Buffer = pv;
         Buffer->AllocSize = cbNew ;
     }
    
     Buffer->BufferSize = cbNew;

     return TRUE;
}

__inline VOID
FreeBuffer (IN OUT BUFFER *Buffer)
/*++

Routine Description:

    Free the buffer

Arguments:

    Buffer - pointer to buffer
    
Return Value:

    Nothing

--*/
{
    if (Buffer->Buffer != NULL) {
        BackupFree( Buffer->Buffer );
        Buffer->Buffer = NULL;
    }
}

VOID ResetAccessDate( HANDLE hand )
{
        
   LONGLONG tmp_time = -1 ;
   FILETIME *time_ptr ;

   time_ptr = (FILETIME *)(&tmp_time);

   if (hand != INVALID_HANDLE_VALUE) {
       SetFileTime( hand,
             time_ptr, 
             time_ptr, 
             time_ptr ) ; 

   }
   
}


VOID
FreeContext (IN OUT LPVOID *lpContext)
/*++

Routine Description:

    Free a backup context and release all resources assigned to it.

Arguments:

    lpContext - pointer to pointer backup context

Return Value:

    None.

--*/
{
    BACKUPCONTEXT *pbuc = *lpContext;

    if (pbuc != INVALID_HANDLE_VALUE) {
        
        FreeBuffer( &pbuc->DataBuffer );
        FreeBuffer( &pbuc->StreamNameBuffer );
        
        ResetAccessDate( pbuc->hAlternate ) ;
        if (pbuc->hAlternate != INVALID_HANDLE_VALUE) {

            CloseHandle( pbuc->hAlternate );
        }
        
        BackupFree(pbuc);
        
        *lpContext = INVALID_HANDLE_VALUE;
    }
}


BACKUPCONTEXT *
AllocContext (IN DWORD cbBuffer)
/*++

Routine Description:

    Allocate a backup context with a buffer of a specified size

Arguments:

    cbBuffer - desired length of the buffer

Return Value:

    pointer to initialized backupcontext or NULL if out of memory.

--*/
{
    BACKUPCONTEXT *pbuc;

    pbuc = BackupAlloc( sizeof( *pbuc ));

    if (pbuc != NULL) {
        RtlZeroMemory( pbuc, sizeof( *pbuc ));
        pbuc->fStreamStart = TRUE;

        if (cbBuffer != 0 && !GrowBuffer( &pbuc->DataBuffer, cbBuffer )) {
            BackupFree( pbuc );
            pbuc = NULL;
        }
    }
    
    if (pbuc == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }

    return(pbuc);
}



LONGLONG
ComputeRemainingSize (IN BACKUPCONTEXT *pbuc)
/*++

Routine Description:

    (Re)Compute the number of bytes required to store the current 
    stream.  This needs to take into account the header length as
    well.

Arguments:

    pbuc - backup context

Return Value:

    Amount of data still remaining to transfer.  Includes header
    size.

--*/
{
    LARGE_INTEGER ret_size ;

    ret_size.QuadPart = pbuc->cbHeader + pbuc->head.Size.QuadPart 
                               - pbuc->liStreamOffset;

    //
    // since the internally we treat the sparse buffer offset 
    // as part of the header and since the caller need to see it
    // as part of the data, this code make the internal correction.
    //
    if ( pbuc->head.dwStreamId == BACKUP_SPARSE_BLOCK  ) {

         ret_size.QuadPart -= sizeof(LARGE_INTEGER) ;
    }

    return ret_size.QuadPart ; 
}


DWORD
ComputeRequestSize (BACKUPCONTEXT *pbuc, DWORD cbrequest)
/*++

Routine Description:

    Given a transfer size request, return the number of
    bytes remaining that can safely be returned to the
    caller

Arguments:

    pbuc - context of call
    
    cbRequest - desired transfer size

Return Value:

    amount of data available to return.

--*/
{
    LONGLONG licbRemain;

    licbRemain = ComputeRemainingSize( pbuc );
    
    return (DWORD) min( cbrequest, licbRemain );
}


VOID
ReportTransfer(BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif, DWORD cbtransferred)
/*++

Routine Description:

    Note that a transfer has occurred and update contexts

Arguments:

    pbuc - context of call
    
    pbif - BACKUPIOFRAME of call detailing call
    
    cbtransferred - amount successfully transferred

Return Value:

    None.

--*/
{
    pbuc->liStreamOffset += cbtransferred;
    *pbif->pcbTransferred += cbtransferred;
    pbif->cbRequest -= cbtransferred;
    pbif->pIoBuffer += cbtransferred;
}



VOID
BackupReadBuffer (BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    Perform a read to user buffer from the buffer in the backup
    context.

Arguments:

    pbuc - context of call
    
    pbif - frame describing desired user BackupRead request

Return Value:

    None.

--*/
{
    DWORD cbrequest;
    BYTE *pb;

    //
    //  Determine size of allowable transfer and pointer to source
    //  data
    //
    
    cbrequest = ComputeRequestSize( pbuc, pbif->cbRequest );
    pb = &pbuc->DataBuffer.Buffer[ pbuc->liStreamOffset - pbuc->cbHeader ];

    //
    //  Move the data to the user's buffer
    //
    
    RtlCopyMemory(pbif->pIoBuffer, pb, cbrequest);

    //
    //  Update statistics
    //
    
    ReportTransfer(pbuc, pbif, cbrequest);
}



BOOL
BackupReadStream (HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    Perform a read to user buffer from the stream.

Arguments:

    hFile - handle to file for transfer
    
    pbuc - context of call
    
    pbif - frame describing BackupRead request

Return Value:

    True if transfer succeeded..

--*/
{
    DWORD cbrequest;
    DWORD cbtransferred;
    BOOL fSuccess;

    if (pbuc->fSparseBlockStart) {

        PFILE_ALLOCATED_RANGE_BUFFER range_buf ;
        LARGE_INTEGER licbFile ;

        range_buf = (PFILE_ALLOCATED_RANGE_BUFFER)(pbuc->DataBuffer.Buffer + pbuc->dwSparseMapOffset) ;

        pbuc->head.Size.QuadPart = range_buf->Length.QuadPart + sizeof(LARGE_INTEGER) ;

        pbuc->head.dwStreamId = BACKUP_SPARSE_BLOCK ;
        pbuc->head.dwStreamAttributes = STREAM_SPARSE_ATTRIBUTE;

        pbuc->head.dwStreamNameSize = 0;

        pbuc->cbHeader = CB_NAMELESSHEADER + sizeof( LARGE_INTEGER ) ;

        pbuc->cbSparseOffset = range_buf->FileOffset ;

        RtlCopyMemory( pbuc->head.cStreamName, &pbuc->cbSparseOffset, sizeof( LARGE_INTEGER ) ) ;

        pbuc->fSparseBlockStart = FALSE;

        licbFile.HighPart = 0;

        licbFile.HighPart = range_buf->FileOffset.HighPart;

        licbFile.LowPart = SetFilePointer( hFile,
                              range_buf->FileOffset.LowPart,
                              &licbFile.HighPart,
                              FILE_BEGIN );

        if ( licbFile.QuadPart != range_buf->FileOffset.QuadPart ) {
            pbuc->fAccessError = TRUE;
            return FALSE ;
        } else {
            return TRUE ;
        }
    }    


    if (pbuc->liStreamOffset < pbuc->cbHeader) {

       return TRUE ;
    }

    cbrequest = ComputeRequestSize( pbuc, pbif->cbRequest );

    fSuccess = ReadFile( hFile, pbif->pIoBuffer, cbrequest, &cbtransferred, NULL );

    if (cbtransferred != 0) {
        
        ReportTransfer( pbuc, pbif, cbtransferred );
    
    } else if (fSuccess && cbrequest != 0) {
        
        SetLastError( ERROR_IO_DEVICE );
        fSuccess = FALSE;
    }
    
    return(fSuccess);
}



BOOL
BackupGetSparseMap (HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    Reads the sparse data map.

Arguments:

    hFile - handle to file for transfer
    
    pbuc - context of call
    
    pbif - frame describing BackupRead request

Return Value:

    True if transfer succeeded..

--*/
{
     FILE_ALLOCATED_RANGE_BUFFER  req_buf ;
     PFILE_ALLOCATED_RANGE_BUFFER last_ret_buf ;
     DWORD     out_buf_size ;
     DWORD     data_size = 4096 ;
     IO_STATUS_BLOCK iosb ;
     LARGE_INTEGER   file_size ;
     NTSTATUS        Status ;
     BOOLEAN         empty_file = FALSE ;

     req_buf.FileOffset.QuadPart = 0 ;

     pbuc->dwSparseMapSize   = 0 ;
     pbuc->dwSparseMapOffset = 0 ;
     pbuc->fSparseBlockStart = FALSE ;

     req_buf.Length.LowPart = GetFileSize( hFile, 
                                           &req_buf.Length.HighPart );

     file_size = req_buf.Length ;

     do {
          if ( GrowBuffer( &pbuc->DataBuffer, 
                           data_size ) ) {
          
               iosb.Information = 0 ;

               Status = NtFsControlFile( hFile,
                                NULL,  // overlapped event handle
                                NULL,  // Apc routine
                                NULL,  // overlapped structure
                                &iosb,
                                FSCTL_QUERY_ALLOCATED_RANGES,   
                                &req_buf,
                                sizeof( req_buf ),
                                pbuc->DataBuffer.Buffer + pbuc->dwSparseMapSize,
                                pbuc->DataBuffer.AllocSize - pbuc->dwSparseMapSize ) ;

               out_buf_size = 0 ;

               if ((Status == STATUS_BUFFER_OVERFLOW) || NT_SUCCESS( Status ) ) {
                    out_buf_size = (DWORD)iosb.Information ;
                    if ( out_buf_size == 0 ) {
                         empty_file = TRUE ;
                    }
               }

               if ( out_buf_size != 0 ) {
                    pbuc->dwSparseMapSize += out_buf_size ;

                    last_ret_buf = 
                         (PFILE_ALLOCATED_RANGE_BUFFER)(pbuc->DataBuffer.Buffer +
                                                    pbuc->dwSparseMapSize -
                                                    sizeof(FILE_ALLOCATED_RANGE_BUFFER)) ;

                    req_buf.FileOffset = last_ret_buf->FileOffset ;
                    req_buf.FileOffset.QuadPart += last_ret_buf->Length.QuadPart ;

                    //
                    // if we can't fit any more in the buffer lets increase
                    //   the size and get more data otherwise assume were done.
                    //
                    if ( pbuc->dwSparseMapSize + sizeof(FILE_ALLOCATED_RANGE_BUFFER) >
                         pbuc->DataBuffer.AllocSize ) {
                         data_size += 4096 ;

                    } else {

                         break ;
                    }

               } else {

                    // reallocate for one more buffer entry
                    if ( out_buf_size + sizeof(FILE_ALLOCATED_RANGE_BUFFER) > data_size ) {
                         data_size += 4096 ;
                         continue ;
                    }

                    break ;
               }
          
          } else {

               pbuc->dwSparseMapSize = 0 ;
               break ;

          }
               
     } while ( TRUE ) ;

     //
     // if there are RANGE_BUFFERS and it isn't simply the whole file, then
     //     go into sparse read mode.
     //

     if ( empty_file || (pbuc->dwSparseMapSize >= sizeof( FILE_ALLOCATED_RANGE_BUFFER) ) ) {

          last_ret_buf = (PFILE_ALLOCATED_RANGE_BUFFER)(pbuc->DataBuffer.Buffer ) ;

          if ( empty_file ||
               ( last_ret_buf->FileOffset.QuadPart != 0 ) ||
               ( last_ret_buf->Length.QuadPart != file_size.QuadPart ) ) {


               // first lets add a record for the EOF marker 
               pbuc->dwSparseMapSize += sizeof(FILE_ALLOCATED_RANGE_BUFFER) ;
               last_ret_buf = 
                      (PFILE_ALLOCATED_RANGE_BUFFER)(pbuc->DataBuffer.Buffer +
                                                 pbuc->dwSparseMapSize -
                                                    sizeof(FILE_ALLOCATED_RANGE_BUFFER)) ;

               last_ret_buf->FileOffset.QuadPart = file_size.QuadPart ;
               last_ret_buf->Length.QuadPart = 0 ;

               pbuc->fSparseBlockStart = TRUE ;
               return TRUE ;
          }
     } 

     pbuc->dwSparseMapSize = 0 ;
     return FALSE ;
}
     

BOOL
BackupReadData (HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    Read default data for a user BackupRead request.

Arguments:

    hFile - handle to file for transfer
    
    pbuc - context of call
    
    pbif - frame describing BackupRead request

Return Value:

    True if transfer succeeded..

--*/
{
    LARGE_INTEGER licbFile ;

    //
    //  If the context is not initialized for this transfer,
    //  set up based on file size.
    //
    
    if (pbuc->fStreamStart) {

        if (pbuc->fAttribs & FILE_ATTRIBUTE_ENCRYPTED) {
            return TRUE;
        }

        if (pbuc->fAttribs & FILE_ATTRIBUTE_DIRECTORY) {
            return TRUE;
        }

        licbFile.LowPart = GetFileSize( hFile, &licbFile.HighPart );

        if (licbFile.QuadPart == 0) {
            return TRUE;
        }
        
        if (licbFile.LowPart == 0xffffffff && GetLastError() != NO_ERROR) {
            return FALSE;
        }


        pbuc->head.dwStreamId = BACKUP_DATA;
        pbuc->head.dwStreamAttributes = STREAM_NORMAL_ATTRIBUTE;

        pbuc->head.dwStreamNameSize = 0;

        pbuc->cbHeader = CB_NAMELESSHEADER;
        pbuc->fStreamStart = FALSE;

        if ( BackupGetSparseMap( hFile, pbuc, pbif ) ) {

            pbuc->head.Size.QuadPart = 0 ;
            pbuc->head.dwStreamAttributes = STREAM_SPARSE_ATTRIBUTE;

        } else {

            pbuc->head.Size = licbFile;

            licbFile.HighPart = 0;
            SetFilePointer( hFile, 0, &licbFile.HighPart, FILE_BEGIN );
        }

        
        return TRUE;
    }

    //
    //  If there's more data for us to read, then go and
    //  get it from the stream
    //
    

    return BackupReadStream( hFile, pbuc, pbif );
}



BOOL
BackupReadAlternateData(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    Perform a read to user buffer from alternate data streams.

Arguments:

    hFile - handle to base file for transfer
    
    pbuc - context of call
    
    pbif - frame describing BackupRead request

Return Value:

    True if transfer succeeded..

--*/
{
    //
    //  If we haven't started transferring alternate data streams then
    //  buffer all the stream information from the file system
    //
    
    if (pbuc->fStreamStart) {
        NTSTATUS Status;
        FILE_STREAM_INFORMATION *pfsi;
        IO_STATUS_BLOCK iosb;

        if (pbuc->fAttribs & FILE_ATTRIBUTE_ENCRYPTED) {
             if ( !(pbuc->fAttribs & FILE_ATTRIBUTE_DIRECTORY) ) {

                 return TRUE;
             }
        }

        //
        //  Loop, growing the names buffer, until it is large enough to 
        //  contain all the alternate data
        //
        
        if (!pbuc->NamesReady) {
            
            if (!GrowBuffer( &pbuc->StreamNameBuffer, 1024 ) ) {
                    
                 return FALSE;
            }
            
            while (TRUE) {
                //
                //  Resize the buffer.  If we cannot grow it, then fail.
                //
                
                Status = NtQueryInformationFile(
                            hFile,
                            &iosb,
                            pbuc->StreamNameBuffer.Buffer,
                            pbuc->StreamNameBuffer.BufferSize,
                            FileStreamInformation);

                //
                //  If we succeeded in reading some data, set the buffer
                //  up and finish initializing
                //
                
                if (NT_SUCCESS(Status) && iosb.Information != 0) {
                    pbuc->iNameBuffer = 0;
                    pbuc->NamesReady = TRUE;
                    break;
                }
                
                //
                //  If the error was not due to overflow, then skip
                //  all alternate streams
                //
                
                if (!BufferOverflow(Status)) {
                    return TRUE;        
                }

                //
                // simply inlarge the buffer and try again.
                //
                if (!GrowBuffer( &pbuc->StreamNameBuffer, 
                                 pbuc->StreamNameBuffer.BufferSize * 2)) {
                    
                    return FALSE;
                }

            }
        }

        pbuc->hAlternate = INVALID_HANDLE_VALUE;
        pbuc->fStreamStart = FALSE;
        pfsi = (FILE_STREAM_INFORMATION *) &pbuc->StreamNameBuffer.Buffer[pbuc->iNameBuffer];

        //
        //  Skip first stream if it is the default data stream.  This 
        //  code is NTFS-specific and relies on behaviour not documented anywhere.
        //
        
        if (pfsi->StreamNameLength >= 2 * sizeof(WCHAR) &&
            pfsi->StreamName[1] == ':') {
            
            if (pfsi->NextEntryOffset == 0) {
                return TRUE;                // No more, do next stream type
            }
            
            pbuc->iNameBuffer += pfsi->NextEntryOffset;
        
        }
        
        pbuc->head.Size.LowPart = 1;
    
    //
    //  If we don't have an open stream
    //

    } else if (pbuc->hAlternate == INVALID_HANDLE_VALUE) {
        NTSTATUS Status;
        PFILE_STREAM_INFORMATION pfsi;
        UNICODE_STRING strName;
        OBJECT_ATTRIBUTES oa;
        IO_STATUS_BLOCK iosb;
        DWORD reparse_flg = 0 ;

        pbuc->head.Size.QuadPart = 0;

        //
        //  Form the relative name of the stream and try to
        //  open it relative to the base file
        //
        
        pfsi = (FILE_STREAM_INFORMATION *) &pbuc->StreamNameBuffer.Buffer[pbuc->iNameBuffer];

        strName.Length = (USHORT) pfsi->StreamNameLength;
        strName.MaximumLength = strName.Length;
        strName.Buffer = pfsi->StreamName;


        if (pbuc->fAttribs & FILE_ATTRIBUTE_REPARSE_POINT ) {

             reparse_flg = FILE_OPEN_REPARSE_POINT ;

             if ( pbuc->fRemoteData ) {
//                 for now, alternate HSM streams should be opened with
//                      FILE_OPEN_REPARSE
//                  reparse_flg = FILE_OPEN_NO_RECALL ;
             }
        }

        InitializeObjectAttributes(
                 &oa,
                 &strName,
                 OBJ_CASE_INSENSITIVE,
                 hFile,
                 NULL);

        Status = NtOpenFile(
                    &pbuc->hAlternate,
                    (BasepIsPropertySetAttribute( pfsi->StreamNameLength, pfsi->StreamName ) 
                        ? FILE_READ_EA 
                        : FILE_READ_DATA) | SYNCHRONIZE,
                    &oa,
                    &iosb,
                    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | reparse_flg);

        //
        //  If we did not succeed, skip this entry and set up for another stream
        //

        if (!NT_SUCCESS( Status )) {
            pbuc->iNameBuffer += pfsi->NextEntryOffset;
            if (pfsi->NextEntryOffset != 0) {
                pbuc->head.Size.LowPart = 1;
                pbuc->fMultiStreamType = TRUE;        // more to come
            }
            SetLastError( ERROR_SHARING_VIOLATION );
            return FALSE;
        }

        // if we can't lock all records, return an error
        if (!LockFile( pbuc->hAlternate, 0, 0, 0xffffffff, 0xffffffff )) {
            SetLastError( ERROR_SHARING_VIOLATION );
            return FALSE;
        }

        //
        //  Perform common header initialization
        //
        
        pbuc->head.dwStreamAttributes = STREAM_NORMAL_ATTRIBUTE;
        pbuc->head.dwStreamNameSize = pfsi->StreamNameLength;

        pbuc->cbHeader = CB_NAMELESSHEADER + pfsi->StreamNameLength;

        RtlCopyMemory(
            pbuc->head.cStreamName,
            pfsi->StreamName,
            pfsi->StreamNameLength);

        //
        //  Advance to the next stream in the stream information block
        //
        
        if (pfsi->NextEntryOffset != 0) {
            pbuc->iNameBuffer += pfsi->NextEntryOffset;
            pbuc->fMultiStreamType = TRUE;
        }
    
        //
        //  If we are a data stream, set up for data stream copy
        //

        if (BasepIsDataAttribute( pfsi->StreamNameLength, pfsi->StreamName )) {

            pbuc->head.dwStreamId = BACKUP_ALTERNATE_DATA;


            if ( BackupGetSparseMap( pbuc->hAlternate, pbuc, pbif ) ) {

                 pbuc->head.Size.QuadPart = 0 ;
                 pbuc->head.dwStreamAttributes = STREAM_SPARSE_ATTRIBUTE;

            } else {

                pbuc->head.Size.LowPart = GetFileSize(
                    pbuc->hAlternate,
                    &pbuc->head.Size.HighPart );

            }
        
        //
        //  If we are a property set
        //
        
        } else if (BasepIsPropertySetAttribute( pfsi->StreamNameLength, pfsi->StreamName )) {

            //
            //  Attempt to read entire property set into buffer
            //
            
            PROPERTY_READ_CONTROL PropertyReadControl;
            PropertyReadControl.Op = PRC_READ_ALL;

            while (TRUE) {
                Status = NtFsControlFile(
                    pbuc->hAlternate,
                    NULL,
                    NULL,
                    NULL,
                    &iosb,
                    FSCTL_READ_PROPERTY_DATA,
                    &PropertyReadControl,
                    sizeof( PropertyReadControl ),
                    pbuc->DataBuffer.Buffer,
                    pbuc->DataBuffer.BufferSize);
        
                //
                //  If we had an unexpected error, cleanup and return
                //
        
                if (Status != STATUS_BUFFER_OVERFLOW)
                    break;
        
                //
                //  We did not allocate a sufficient buffer for this operation.
                //  Get the correct size, free up the current buffer, and retry
                //
        
                if (!GrowBuffer( &pbuc->DataBuffer, 
                                 ((PPROPERTY_OUTPUT_HEADER) pbuc->DataBuffer.Buffer)->Length )) {
                    return FALSE;
                }
            }

            //
            //  Either we have successfully read in the property set or
            //  we have a status code set.  If the latter, then return an error
            //
            
            if (!NT_SUCCESS( Status )) {
                BaseSetLastNTError( Status );
                return FALSE;
            }

            //
            //  Set up header.  
            //

            pbuc->head.dwStreamId = BACKUP_PROPERTY_DATA;
            pbuc->head.Size.QuadPart = 
                ((PPROPERTY_OUTPUT_HEADER) pbuc->DataBuffer.Buffer)->Length -
                sizeof( PROPERTY_OUTPUT_HEADER );

        }

    //
    //  If we need to return the name
    //
    } else if ( pbuc->liStreamOffset < pbuc->cbHeader) {
        return TRUE ;

    //
    //  If there is more data in this stream to transfer
    //
    
    } else if ( (pbuc->head.dwStreamId == BACKUP_ALTERNATE_DATA) ||
                (pbuc->head.dwStreamId == BACKUP_SPARSE_BLOCK) ) {
    
        return BackupReadStream( pbuc->hAlternate, pbuc, pbif );
    
    } else {
        ASSERT( pbuc->head.dwStreamId == BACKUP_PROPERTY_DATA );
        
        //
        //  Skip past the property output header
        //
        
        if (pbuc->liStreamOffset == pbuc->cbHeader) {
            pbuc->liStreamOffset += sizeof( PROPERTY_OUTPUT_HEADER );
            pbuc->head.Size.QuadPart += sizeof( PROPERTY_OUTPUT_HEADER );
        }
        
        BackupReadBuffer( pbuc, pbif );
    }
    return TRUE;
}



BOOL
BackupReadEaData(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    Perform a read to user buffer from EA data.

Arguments:

    hFile - handle to file with EAs
    
    pbuc - context of call
    
    pbif - frame describing BackupRead request

Return Value:

    True if transfer succeeded..

--*/
{
    //
    //  If we are just starting out on the EA data
    //
    
    if (pbuc->fStreamStart) {
        IO_STATUS_BLOCK iosb;

        //
        //  Loop trying to read all EA data into the buffer and
        //  resize the buffer if necessary
        //
        
        while (TRUE) {
            NTSTATUS Status;
            FILE_EA_INFORMATION fei;

            Status = NtQueryEaFile(
                        hFile,
                        &iosb,
                        pbuc->DataBuffer.Buffer,
                        pbuc->DataBuffer.BufferSize,
                        FALSE,
                        NULL,
                        0,
                        0,
                        (BOOLEAN) TRUE );
            
            //
            //  If we successfully read all the data, go complete
            //  the initialization
            //
            if (NT_SUCCESS( Status ) && iosb.Information != 0) {
                pbuc->NamesReady = TRUE;
                break;
            }

            //
            //  If we received a status OTHER than buffer overflow then
            //  skip EA's altogether
            //

            if (!BufferOverflow(Status)) {
                return TRUE;
            }

            //
            //  Get a stab at the total EA size 
            //

            Status = NtQueryInformationFile(
                        hFile,
                        &iosb,
                        &fei,
                        sizeof(fei),
                        FileEaInformation);

            //
            //  This call should never have failed (since we were able to 
            //  QueryEaFile) above.  However, if it does, skip EAs altogether
            //
            
            if (!NT_SUCCESS(Status)) {
                return TRUE;
            }
            
            //
            //  Resize the buffer to something that seems reasonable.  No guarantees
            //  about whether this will work or not...  If we couldn't grow the buffer
            //  fail this call.
            //
            
            if (!GrowBuffer( &pbuc->DataBuffer, (fei.EaSize * 5) / 4)) {
                pbuc->fAccessError = TRUE;
                return FALSE;
            }
        }

        //
        //  Set up the header for the EA stream
        //
        
        pbuc->head.dwStreamId = BACKUP_EA_DATA;
        pbuc->head.dwStreamAttributes = STREAM_NORMAL_ATTRIBUTE;
        pbuc->head.dwStreamNameSize = 0;

        pbuc->cbHeader = CB_NAMELESSHEADER;

        pbuc->head.Size.QuadPart = iosb.Information;

        pbuc->fStreamStart = FALSE;
    
    //
    //  If we have more data in the buffer to read then go
    //  copy it out.
    //
    
    } else if (pbuc->liStreamOffset >= pbuc->cbHeader) {
        BackupReadBuffer( pbuc, pbif );
    }

    return TRUE;
}


BOOL
BackupReadObjectId(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    Perform a read to user buffer from NtObject ID data.

Arguments:

    hFile - handle to file with EAs
    
    pbuc - context of call
    
    pbif - frame describing BackupRead request

Return Value:

    True if transfer succeeded..

--*/
{
    //
    //  If we are just starting out on the Object ID data
    //
    
    if (pbuc->fStreamStart) {
        IO_STATUS_BLOCK iosb;
        NTSTATUS Status ;

        if (!GrowBuffer( &pbuc->DataBuffer, 1024 ) ) {
            pbuc->fAccessError = TRUE;
            return FALSE;
        }


        Status = NtFsControlFile( hFile,
                         NULL,  // overlapped event handle
                         NULL,  // Apc routine
                         NULL,  // overlapped structure
                         &iosb,
                         FSCTL_GET_OBJECT_ID,
                         NULL,
                         0,
                         pbuc->DataBuffer.Buffer,
                         pbuc->DataBuffer.BufferSize ) ;

        if ( !NT_SUCCESS(Status) ) {
             return TRUE ;
        }

        //
        //  Set up the header for the Object ID stream
        //

        pbuc->NamesReady = TRUE;
        
        pbuc->head.dwStreamId = BACKUP_OBJECT_ID ;
        pbuc->head.dwStreamAttributes = STREAM_NORMAL_ATTRIBUTE;
        pbuc->head.dwStreamNameSize = 0;

        pbuc->cbHeader = CB_NAMELESSHEADER;

        pbuc->head.Size.QuadPart = iosb.Information;

        pbuc->fStreamStart = FALSE;
    
    //
    //  If we have more data in the buffer to read then go
    //  copy it out.
    //
    
    } else if (pbuc->liStreamOffset >= pbuc->cbHeader) {
        BackupReadBuffer( pbuc, pbif );
    }

    return TRUE;
}


BOOL
BackupReadReparseData(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    Perform a read to user buffer from Reparse tag data.

Arguments:

    hFile - handle to file with EAs
    
    pbuc - context of call
    
    pbif - frame describing BackupRead request

Return Value:

    True if transfer succeeded..

--*/
{

    IO_STATUS_BLOCK iosb;
    PREPARSE_DATA_BUFFER rp_buf_ptr ;
    NTSTATUS Status ;

    struct RP_SUMMARY {
           USHORT tag ;
           USHORT rp_size ;
    } *rp_summary_ptr =(struct RP_SUMMARY*) &(iosb.Information) ;


    //
    //  If the object is not a reparse then simply return
    //
    if ( !(pbuc->fAttribs & FILE_ATTRIBUTE_REPARSE_POINT) ) { 
         return TRUE ;
    }
 
    //
    //  If we are just starting out on the ReParse data
    //
    
    if (pbuc->fStreamStart) {

        //
        //  Loop trying to read all EA data into the buffer and
        //  resize the buffer if necessary
        //
     
        // for some reason a TOO_SMALL error is not setting the information
        //    member of the iosb....

        rp_summary_ptr->rp_size = MAXIMUM_REPARSE_DATA_BUFFER_SIZE ;

        Status = NtFsControlFile( hFile,
                         NULL,  // overlapped event handle
                         NULL,  // Apc routine
                         NULL,  // overlapped structure
                         &iosb,
                         FSCTL_GET_REPARSE_POINT,
                         NULL,
                         0,
                         pbuc->DataBuffer.Buffer,
                         pbuc->DataBuffer.BufferSize ) ;


        if ( BufferOverflow( Status ) ) {
                    
            if ( rp_summary_ptr->rp_size == 0 ) {
                 rp_summary_ptr->rp_size = MAXIMUM_REPARSE_DATA_BUFFER_SIZE ;
            }

            if (!GrowBuffer( &pbuc->DataBuffer, 
                            rp_summary_ptr->rp_size ) ) {

                 pbuc->fAccessError = TRUE;
                 return FALSE;
            }

            Status = NtFsControlFile( hFile,
                             NULL,  // overlapped event handle
                             NULL,  // Apc routine
                             NULL,  // overlapped structure
                             &iosb,
                             FSCTL_GET_REPARSE_POINT,
                             NULL,
                             0,
                             pbuc->DataBuffer.Buffer,
                             pbuc->DataBuffer.BufferSize ) ;

        }

        //
        //  If we successfully read all the data, go complete
        //  the initialization
        //
        if ( !NT_SUCCESS( Status ) ) {
            return TRUE ;
        }


        //
        //  Set up the header for the ReParse stream
        //
        
        rp_buf_ptr = (PREPARSE_DATA_BUFFER)(pbuc->DataBuffer.Buffer) ;

        if ( IsReparseTagHighLatency(rp_buf_ptr->ReparseTag) ) {
             pbuc->fRemoteData = TRUE ;
        }

        pbuc->NamesReady = TRUE;

        pbuc->head.dwStreamId = BACKUP_REPARSE_DATA;
        pbuc->head.dwStreamAttributes = STREAM_NORMAL_ATTRIBUTE;
        pbuc->head.dwStreamNameSize = 0;

        pbuc->cbHeader = CB_NAMELESSHEADER;

        if ( IsReparseTagMicrosoft( rp_buf_ptr->ReparseTag ) ) {
             pbuc->head.Size.QuadPart = rp_buf_ptr->ReparseDataLength +
                                        FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) ;
        } else {
             pbuc->head.Size.QuadPart = rp_buf_ptr->ReparseDataLength +
                                        FIELD_OFFSET(REPARSE_GUID_DATA_BUFFER, GenericReparseBuffer.DataBuffer) ;
        }

        pbuc->fStreamStart = FALSE;
    
    //
    //  If we have more data in the buffer to read then go
    //  copy it out.
    //
    
    } else if (pbuc->liStreamOffset >= pbuc->cbHeader) {
        BackupReadBuffer( pbuc, pbif );
    }

    return TRUE;
}



BOOL
BackupReadSecurityData(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
{
    //
    //  If we are to skip security then do so.
    //
    
    if (!pbif->fProcessSecurity) {
        return TRUE;
    }

    //
    //  If we are just starting out on the security data
    //
    
    if (pbuc->fStreamStart) {
        
        //
        //  Loop trying to read all security data into the buffer and
        //  resize the buffer if necessary
        //
        
        while (TRUE) {
            NTSTATUS Status;
            DWORD cbSecurityInfo;

            RtlZeroMemory( pbuc->DataBuffer.Buffer, pbuc->DataBuffer.BufferSize );

            Status = NtQuerySecurityObject(
                        hFile,
                        OWNER_SECURITY_INFORMATION |
                            GROUP_SECURITY_INFORMATION |
                            DACL_SECURITY_INFORMATION |
                            SACL_SECURITY_INFORMATION,
                        pbuc->DataBuffer.Buffer,
                        pbuc->DataBuffer.BufferSize,
                        &cbSecurityInfo );

            //
            //  If we failed but it wasn't due to buffer overflow
            //
            
            if (!NT_SUCCESS( Status ) && !BufferOverflow( Status )) {

                //
                //  Try reading everything but SACL
                //

                Status = NtQuerySecurityObject(
                            hFile,
                            OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION,
                            pbuc->DataBuffer.Buffer,
                            pbuc->DataBuffer.BufferSize,
                            &cbSecurityInfo );
            }
            
            //
            //  If we got it all, then go continue initialization
            //

            if (NT_SUCCESS( Status )) {
                pbuc->NamesReady = TRUE;
                break;
            }


            //
            //  If not due to overflowing buffer, skip security altogether
            //
            
            if (!BufferOverflow( Status )) {
                return TRUE;
            }

            //
            //  Resize the buffer to the expected size.  If we fail, fail
            //  the entire call
            //

            if (!GrowBuffer( &pbuc->DataBuffer, cbSecurityInfo )) {
                return FALSE;
            }
        }

        //
        //  Initialize the stream header
        //

        pbuc->head.dwStreamId = BACKUP_SECURITY_DATA;
        pbuc->head.dwStreamAttributes = STREAM_CONTAINS_SECURITY;
        pbuc->head.dwStreamNameSize = 0;

        pbuc->cbHeader = CB_NAMELESSHEADER;

        pbuc->head.Size.QuadPart = RtlLengthSecurityDescriptor(pbuc->DataBuffer.Buffer);

        pbuc->fStreamStart = FALSE;
    
    //
    //  If there is more data in the buffer to transfer, go
    //  do it
    //
    } else if (pbuc->liStreamOffset >= pbuc->cbHeader) {
        
        BackupReadBuffer( pbuc, pbif );
    
    }
    
    return TRUE;
}



VOID
BackupTestRestartStream(BACKUPCONTEXT *pbuc)
{
    LONGLONG licbRemain;

    licbRemain = ComputeRemainingSize( pbuc );
    if (licbRemain == 0) {

        if ( pbuc->dwSparseMapOffset != pbuc->dwSparseMapSize ) {   // only true at backup

             if ( !pbuc->fSparseBlockStart ) {
                  pbuc->dwSparseMapOffset += sizeof ( FILE_ALLOCATED_RANGE_BUFFER ) ;
             }
        }

        if ( pbuc->dwSparseMapOffset != pbuc->dwSparseMapSize ) {   // only true at backup
             pbuc->fSparseBlockStart = TRUE ;

             pbuc->cbHeader = 0 ;
             pbuc->liStreamOffset = 0;                

        } else {
             if ( !pbuc->fSparseHandAlt && (pbuc->hAlternate != NULL)) {
                 CloseHandle(pbuc->hAlternate);        // releases any locks
                 pbuc->hAlternate = NULL;
             }
             pbuc->cbHeader = 0;
             pbuc->fStreamStart = TRUE;
             pbuc->fSparseBlockStart = TRUE;

             pbuc->liStreamOffset = 0;                // for BackupWrite

             if (!pbuc->fMultiStreamType) {                // for BackupRead
                 pbuc->StreamIndex++;
                 pbuc->head.dwStreamId = mwStreamList[pbuc->StreamIndex] ;
                 pbuc->NamesReady = FALSE;
             }
        }
    }
}



//  Routine Description:
//
//    Data can be Backed up from an object using BackupRead.
//
//    This API is used to read data from an object.  After the
//    read completes, the file pointer is adjusted by the number of bytes
//    actually read.  A return value of TRUE coupled with a bytes read of
//    0 indicates that end of file has been reached.
//
//  Arguments:
//
//    hFile - Supplies an open handle to a file that is to be read.  The
//          file handle must have been created with GENERIC_READ access.
//
//    lpBuffer - Supplies the address of a buffer to receive the data read
//          from the file.
//
//    nNumberOfBytesToRead - Supplies the number of bytes to read from the
//          file.
//
//    lpNumberOfBytesRead - Returns the number of bytes read by this call.
//          This parameter is always set to 0 before doing any IO or error
//          checking.
//
//    bAbort - If TRUE, then all resources associated with the context will
//          be released.
//
//    bProcessSecurity - If TRUE, then the NTFS ACL data will be read.
//          If FALSE, then the ACL stream will be skipped.
//
//    lpContext - Points to a buffer pointer setup and maintained by
//          BackupRead.
//
//
//  Return Value:
//
//    TRUE - The operation was successul.
//
//    FALSE - The operation failed.  Extended error status is available
//          using GetLastError.
//
//
// NOTE:
// The NT File Replication Service (NTFRS) performs an MD5 checksum on the 
// stream of data returned by BackupRead().  If the sequence of file information 
// returned changes then two machines, one downlevel and one uplevel will 
// compute different MD5 checksums for the same file data.  Under certain 
// conditions this will cause needless file replication.  Bear this in mind 
// if a change in the returned data sequence is contemplated.  The sources for
// NTFRS are in \nt\private\net\svcimgs\ntrepl.
// 

BOOL WINAPI
BackupRead(
    HANDLE  hFile,
    LPBYTE  lpBuffer,
    DWORD   nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    BOOL    bAbort,
    BOOL    bProcessSecurity,
    LPVOID  *lpContext)
{
    BACKUPCONTEXT *pbuc;
    BACKUPIOFRAME bif;
    BOOL fSuccess = FALSE;
    IO_STATUS_BLOCK iosb ;

    pbuc = *lpContext;
    bif.pIoBuffer = lpBuffer;
    bif.cbRequest = nNumberOfBytesToRead;
    bif.pcbTransferred = lpNumberOfBytesRead;
    bif.fProcessSecurity = (BOOLEAN)bProcessSecurity;

    if (bAbort) {
        if (pbuc != NULL) {
            ResetAccessDate( hFile ) ;
            FreeContext(lpContext);
        }
        return TRUE;
    }
    *bif.pcbTransferred = 0;

    if (pbuc == INVALID_HANDLE_VALUE || bif.cbRequest == 0) {
        return TRUE;
    }

    if (pbuc != NULL && mwStreamList[pbuc->StreamIndex] == BACKUP_INVALID) {
        ResetAccessDate( hFile ) ;
        FreeContext(lpContext);
        return TRUE;
    }

    // Allocate our Context Control Block on first call.

    if (pbuc == NULL) {
        pbuc = AllocContext(CBMIN_BUFFER);        // Alloc initial buffer

        // ok, we allocated the context, Lets initialize it.
        if (pbuc != NULL) {
            NTSTATUS Status ;
            FILE_BASIC_INFORMATION fbi;

            Status = NtQueryInformationFile(
                        hFile,
                        &iosb,
                        &fbi,
                        sizeof(fbi),
                        FileBasicInformation );

            if ( NT_SUCCESS( Status ) ) {
               pbuc->fAttribs = fbi.FileAttributes ;
            } else {
               BaseSetLastNTError( Status );
               return FALSE ;
            }

        }
          
    }

    if (pbuc != NULL) {
        *lpContext = pbuc;

        do {

            if (pbuc->fStreamStart) {
                pbuc->head.Size.QuadPart = 0;

                pbuc->liStreamOffset = 0;

                pbuc->dwSparseMapOffset = 0;
                pbuc->dwSparseMapSize   = 0;

                pbuc->fMultiStreamType = FALSE;
            }
            fSuccess = TRUE;

            switch (mwStreamList[pbuc->StreamIndex]) {
                case BACKUP_DATA:
                    fSuccess = BackupReadData(hFile, pbuc, &bif);
                    break;

                case BACKUP_ALTERNATE_DATA:
                    fSuccess = BackupReadAlternateData(hFile, pbuc, &bif);
                    break;

                case BACKUP_EA_DATA:
                    fSuccess = BackupReadEaData(hFile, pbuc, &bif);
                    break;

                case BACKUP_OBJECT_ID:
                    fSuccess = BackupReadObjectId(hFile, pbuc, &bif);
                    break;

                case BACKUP_REPARSE_DATA:
                    fSuccess = BackupReadReparseData(hFile, pbuc, &bif);
                    break;

                case BACKUP_SECURITY_DATA:
                    fSuccess = BackupReadSecurityData(hFile, pbuc, &bif);
                    break;

                default:
                    pbuc->StreamIndex++;
                    pbuc->fStreamStart = TRUE;
                    break;
            }

            // if we're in the phase of reading the header, copy the header

            if (pbuc->liStreamOffset < pbuc->cbHeader) {

                DWORD cbrequest;

                //  Send the current stream header;

                cbrequest = 
                    (ULONG)min( pbuc->cbHeader - pbuc->liStreamOffset,
                                bif.cbRequest);

                RtlCopyMemory(
                    bif.pIoBuffer,
                    (BYTE *) &pbuc->head + pbuc->liStreamOffset,
                    cbrequest);

                ReportTransfer(pbuc, &bif, cbrequest);
            }

            //
            // if we are at the end of a stream then
            //          start at the beginning of the next stream
            //

            if (pbuc->liStreamOffset >= pbuc->cbHeader) {
                 BackupTestRestartStream(pbuc);
            }

        } while (fSuccess &&
                 mwStreamList[pbuc->StreamIndex] != BACKUP_INVALID &&
                 bif.cbRequest != 0);
    }
    
    if (fSuccess && *bif.pcbTransferred == 0) {
        ResetAccessDate( hFile ) ;
        FreeContext(lpContext);
    }
    
    return(fSuccess);
}



//  Routine Description:
//
//    Data can be skiped during BackupRead or BackupWrite by using
//    BackupSeek.
//
//    This API is used to seek forward from the current position the
//    specified number of bytes.  This function does not seek over a
//    stream header.  The number of bytes actually seeked is returned.
//    If a caller wants to seek to the start of the next stream it can
//    pass 0xffffffff, 0xffffffff as the amount to seek.  The number of
//    bytes actually skiped over is returned.
//
//  Arguments:
//
//    hFile - Supplies an open handle to a file that is to be read.  The
//          file handle must have been created with GENERIC_READ or
//          GENERIC_WRITE access.
//
//    dwLowBytesToSeek - Specifies the low 32 bits of the number of bytes
//          requested to seek.
//
//    dwHighBytesToSeek - Specifies the high 32 bits of the number of bytes
//          requested to seek.
//
//    lpdwLowBytesSeeked - Points to the buffer where the low 32 bits of the
//          actual number of bytes to seek is to be placed.
//
//    lpdwHighBytesSeeked - Points to the buffer where the high 32 bits of the
//          actual number of bytes to seek is to be placed.
//
//    bAbort - If true, then all resources associated with the context will
//          be released.
//
//    lpContext - Points to a buffer pointer setup and maintained by
//          BackupRead.
//
//
//  Return Value:
//
//    TRUE - The operation successfuly seeked the requested number of bytes.
//
//    FALSE - The requested number of bytes could not be seeked. The number
//          of bytes actually seeked is returned.

BOOL WINAPI
BackupSeek(
    HANDLE  hFile,
    DWORD   dwLowBytesToSeek,
    DWORD   dwHighBytesToSeek,
    LPDWORD lpdwLowBytesSeeked,
    LPDWORD lpdwHighBytesSeeked,
    LPVOID *lpContext)
{
    BACKUPCONTEXT *pbuc;
    LONGLONG licbRemain;
    LARGE_INTEGER licbRequest;
    BOOL fSuccess;

    pbuc = *lpContext;

    *lpdwHighBytesSeeked = 0;
    *lpdwLowBytesSeeked = 0;

    if (pbuc == INVALID_HANDLE_VALUE || pbuc == NULL || pbuc->fStreamStart) {
        return FALSE;
    }

    if (pbuc->liStreamOffset < pbuc->cbHeader) {
        return FALSE;
    }

    //
    // If we made it here, we are in the middle of a stream
    //

    licbRemain = ComputeRemainingSize( pbuc );

    licbRequest.LowPart = dwLowBytesToSeek;
    licbRequest.HighPart = dwHighBytesToSeek & 0x7fffffff;

    if (licbRequest.QuadPart > licbRemain) {
        licbRequest.QuadPart = licbRemain;
    }
    fSuccess = TRUE;

    switch (pbuc->head.dwStreamId) {
    case BACKUP_EA_DATA:
    case BACKUP_SECURITY_DATA:
    case BACKUP_PROPERTY_DATA:

        // assume less than 2gig of data

        break;

    case BACKUP_DATA:
    case BACKUP_ALTERNATE_DATA:
        {
            LARGE_INTEGER liCurPos;
            LARGE_INTEGER liNewPos;
            HANDLE hf;
    
            //        set up the correct handle to seek with
    
            if (pbuc->head.dwStreamId == BACKUP_DATA) {
                hf = hFile;
            }
            else {
                hf = pbuc->hAlternate;
            }
    
            // first, let's get the current position
    
            liCurPos.HighPart = 0;
            liCurPos.LowPart = SetFilePointer(
                    hf,
                    0,
                    &liCurPos.HighPart,
                    FILE_CURRENT);
    
            // Now seek the requested number of bytes
    
            liNewPos.HighPart = licbRequest.HighPart;
            liNewPos.LowPart = SetFilePointer(
                    hf,
                    licbRequest.LowPart,
                    &liNewPos.HighPart,
                    FILE_CURRENT);
    
            // Assume that we seek the requested amount because if we do not,
            // subsequent reads will fail and the caller will never be able
            // to read to the next stream.
    
            break;
        }

    default:
        break;
    }
    
    if (dwHighBytesToSeek != (DWORD) licbRequest.HighPart ||
        dwLowBytesToSeek != licbRequest.LowPart) {
        fSuccess = FALSE;
    }
    pbuc->liStreamOffset += licbRequest.QuadPart;

    *lpdwLowBytesSeeked = licbRequest.LowPart;
    *lpdwHighBytesSeeked = licbRequest.HighPart;

    BackupTestRestartStream(pbuc);

    if (!fSuccess) {
        SetLastError(ERROR_SEEK);
    }
    return(fSuccess);
}



BOOL
BackupWriteHeader(BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif, DWORD cbHeader)
/*++

Routine Description:

    This is an internal routine that fills our internal backup header
    from the user's data.

Arguments:

    pbuc - CONTEXT of call
    
    pbif - IOCONTEXT of call
    
    cbHeader - size of header to fill

Return Value:

    None.

--*/
{
    //
    //  Determine how much data we can transfer into our header.  
    //
    
    DWORD cbrequest = 
        (DWORD) min( pbif->cbRequest, cbHeader - pbuc->liStreamOffset );

    //
    //  Copy from user buffer into header
    //


    if ( pbuc->liStreamOffset+cbrequest > CWCMAX_STREAMNAME ) {
         return FALSE ;
    }

    RtlCopyMemory(
        (CHAR *) &pbuc->head + pbuc->liStreamOffset,
        pbif->pIoBuffer,
        cbrequest);

    //
    //  Update transfer statistics
    //
    
    ReportTransfer(pbuc, pbif, cbrequest);

    //
    //  If we've filled up the header, mark the header as complete
    //  even though we might need more if names are present
    //
    
    if (pbuc->liStreamOffset == cbHeader) {
        pbuc->cbHeader = cbHeader;
    }

    return TRUE ;
}



typedef enum {
    BRB_FAIL,
    BRB_DONE,
    BRB_MORE,
} BUFFERSTATUS;

BUFFERSTATUS
BackupWriteBuffer(BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    This is an internal routine that fills our internal buffer
    from the user's data.

Arguments:

    pbuc - CONTEXT of call
    
    pbif - IOCONTEXT of call
    
Return Value:

    BRB_FAIL if an error occurred (out of memory)
    
    BRB_DONE if buffer is full or was successfully filled
    
    BRB_MORE if buffer is partially full

--*/
{
    DWORD cbrequest;

    //
    //  If we're starting out on the buffer, we make sure
    //  we have a buffer to contain all of the data since
    //  the Nt calls we'll use must have all the data 
    //  present
    //

    if (pbuc->fStreamStart) {
        pbuc->fStreamStart = FALSE;

        if (pbuc->DataBuffer.BufferSize < pbuc->head.Size.QuadPart &&
            !GrowBuffer( &pbuc->DataBuffer, pbuc->head.Size.LowPart )) {

            return(BRB_FAIL);
        }
    }

    //
    //  Determine how much data from the user buffer is
    //  needed to fill our buffer
    //
    
    cbrequest = ComputeRequestSize( pbuc, pbif->cbRequest );
    
    //
    //  Fill in the next portion of the buffer
    //
    
    RtlCopyMemory(
        pbuc->DataBuffer.Buffer + pbuc->liStreamOffset - pbuc->cbHeader,
        pbif->pIoBuffer,
        cbrequest);

    //
    //  Update transfer statistics
    //
    
    ReportTransfer(pbuc, pbif, cbrequest);

    //
    //  If we've entirely filled the buffer, let our caller know
    //
    
    return ComputeRemainingSize( pbuc ) == 0 ? BRB_DONE : BRB_MORE;
}


BOOL
BackupWriteSparse(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    This is an internal routine that writes sparse block of stream data from
    the user's buffer into the output handle.  The BACKUPCONTEXT contains
    the total length of data to be output.

Arguments:

    hFile - output file handle
    
    pbuc - CONTEXT of call
    
    pbif - IOCONTEXT of call
    
Return Value:

    TRUE if data was successfully written, FALSE otherwise.

--*/
{
     LARGE_INTEGER licbFile ;
     DWORD cbrequest;
     DWORD cbtransferred;
     BOOL fSuccess;

     if ( pbuc->fSparseBlockStart ) {

         RtlCopyMemory( &pbuc->cbSparseOffset, pbuc->head.cStreamName, sizeof( LARGE_INTEGER ) ) ;

         licbFile = pbuc->cbSparseOffset;

         licbFile.LowPart = SetFilePointer( pbuc->fSparseHandAlt?pbuc->hAlternate:hFile,
                              licbFile.LowPart,
                              &licbFile.HighPart,
                              FILE_BEGIN );

         if ( licbFile.QuadPart != pbuc->cbSparseOffset.QuadPart ) {
            return FALSE ;
         }

         if ( pbuc->head.Size.QuadPart == sizeof( LARGE_INTEGER ) ) {
              SetEndOfFile(pbuc->fSparseHandAlt?pbuc->hAlternate:hFile) ;
         }    
         pbuc->fSparseBlockStart = FALSE ;
     }

     //
     //  Determine how much data from the user buffer is
     //  needed to be written into the stream and perform
     //  the transfer.
     //
     
     cbrequest = ComputeRequestSize(pbuc, pbif->cbRequest);

     fSuccess = WriteFile(
                     pbuc->fSparseHandAlt?pbuc->hAlternate:hFile,
                     pbif->pIoBuffer,
                     cbrequest,
                     &cbtransferred,
                     NULL);

     //
     //  Update transfer statistics
     //

     ReportTransfer(pbuc, pbif, cbtransferred);
     
     return(fSuccess);

     return TRUE ;
}


BOOL
BackupWriteStream(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    This is an internal routine that writes stream data from the user's
    buffer into the output handle.  The BACKUPCONTEXT contains the total
    length of data to be output.

Arguments:

    hFile - output file handle
    
    pbuc - CONTEXT of call
    
    pbif - IOCONTEXT of call
    
Return Value:

    TRUE if data was successfully written, FALSE otherwise.

--*/
{
    DWORD cbrequest;
    DWORD cbtransferred;
    BOOL fSuccess;
    IO_STATUS_BLOCK iosb;


    if ( pbuc->fStreamStart &&
       ( pbuc->head.dwStreamAttributes & STREAM_SPARSE_ATTRIBUTE ) ) {

       // if it was sparse when be backed it up make is sparse again.
       NtFsControlFile( hFile,
              NULL,  // overlapped event handle
              NULL,  // Apc routine
              NULL,  // overlapped structure
              &iosb,
              FSCTL_SET_SPARSE ,
              NULL,
              0,
              NULL,
              0 ) ;
    }

    //
    //  Determine how much data from the user buffer is
    //  needed to be written into the stream and perform
    //  the transfer.
    //
    
    cbrequest = ComputeRequestSize(pbuc, pbif->cbRequest);

    fSuccess = WriteFile(
                    hFile,
                    pbif->pIoBuffer,
                    cbrequest,
                    &cbtransferred,
                    NULL);

    //
    //  Update transfer statistics
    //
    
    ReportTransfer(pbuc, pbif, cbtransferred);
    
    return(fSuccess);
}



BOOL
BackupWriteAlternateData(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    This is an internal routine that overwrites an alternate data stream with
    data from the user's buffer.  

Arguments:

    hFile - handle to the file itself.  This is not the handle to the stream
        being overwritten.
    
    pbuc - CONTEXT of call.  This contains the name of the stream.
    
    pbif - IOCONTEXT of call
    
Return Value:

    TRUE if data was successfully written, FALSE otherwise.

--*/
{
    //
    //  If we are just starting out on this stream then attempt to
    //  overwrite the new stream.
    //
    
    if (pbuc->fStreamStart) {
        NTSTATUS Status;
        UNICODE_STRING strName;
        OBJECT_ATTRIBUTES oa;
        IO_STATUS_BLOCK iosb;
        DWORD reparse_flg = 0 ;

        strName.Length = (USHORT) pbuc->head.dwStreamNameSize;
        strName.MaximumLength = strName.Length;
        strName.Buffer = pbuc->head.cStreamName;

        if (pbuc->hAlternate != INVALID_HANDLE_VALUE) {
             CloseHandle(pbuc->hAlternate);        
             pbuc->hAlternate = INVALID_HANDLE_VALUE;
             pbuc->fSparseHandAlt = FALSE ;
        }


        if (pbuc->fAttribs & FILE_ATTRIBUTE_REPARSE_POINT ) {
             reparse_flg = FILE_OPEN_REPARSE_POINT ;
             if ( pbuc->fRemoteData ) {
//                 for now, alternate HSM streams should be opened with
//                      FILE_OPEN_REPARSE
//                 reparse_flg = FILE_OPEN_NO_RECALL ;
             }
        }

        InitializeObjectAttributes(
                &oa,
                &strName,
                OBJ_CASE_INSENSITIVE,
                hFile,
                NULL);

        Status = NtCreateFile(
                    &pbuc->hAlternate,
                    FILE_WRITE_DATA | SYNCHRONIZE,
                    &oa,
                    &iosb,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OVERWRITE_IF,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | reparse_flg,
                    NULL,
                    0L);

        //
        //  If we failed, map the error, record the failure, and return failure.
        //
        
        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            pbuc->fAccessError = TRUE;
            return FALSE;
        }

        if ( pbuc->head.dwStreamAttributes & STREAM_SPARSE_ATTRIBUTE ) {
           pbuc->fSparseHandAlt = TRUE ;

           // if it was sparse when be backed it up make is sparse again.
           NtFsControlFile( pbuc->hAlternate,
                  NULL,  // overlapped event handle
                  NULL,  // Apc routine
                  NULL,  // overlapped structure
                  &iosb,
                  FSCTL_SET_SPARSE ,
                  NULL,
                  0,
                  NULL,
                  0 ) ;
        }

        pbuc->fStreamStart = FALSE;
    }

    //
    //  If we have no handle for the transfer, record this failure
    //  and return failure.
    //
    
    if (pbuc->hAlternate == INVALID_HANDLE_VALUE) {
        pbuc->fAccessError = TRUE;
        return FALSE;
    }
    
    //
    //  Let the normal stream copy perform the transfer
    //
    
    return BackupWriteStream( pbuc->hAlternate, pbuc, pbif );
}



BOOL
BackupWriteEaData(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    This is an internal routine that writes EA data on the file from 
    the user's buffer

Arguments:

    hFile - handle to output file
    
    pbuc - CONTEXT of call
    
    pbif - IOCONTEXT of call
    
Return Value:

    TRUE if EA data was successfully written, FALSE otherwise.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;

    //
    //  Attempt to fill up the buffer from the input.
    //
    
    switch (BackupWriteBuffer( pbuc, pbif )) {
    default:
    case BRB_FAIL:
        return FALSE;

    case BRB_MORE:
        return TRUE;

    case BRB_DONE:
        break;
    }

    //
    //  The buffer is now completely filled with our EA data.  Set the
    //  EA data on the file.
    //
    
    Status = NtSetEaFile(
                hFile,
                &iosb,
                pbuc->DataBuffer.Buffer,
                pbuc->head.Size.LowPart );

    //
    //  If we failed, map the error and return failure
    //
    
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return FALSE;
    }
    
    return TRUE;
}


BOOL
BackupWriteReparseData(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    This is an internal routine that writes Reparse data on the file from 
    the user's buffer

Arguments:

    hFile - handle to output file
    
    pbuc - CONTEXT of call
    
    pbif - IOCONTEXT of call
    
Return Value:

    TRUE if EA data was successfully written, FALSE otherwise.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    DWORD *rp_tag_ptr ;

    //
    //  Attempt to fill up the buffer from the input.
    //
    
    switch (BackupWriteBuffer( pbuc, pbif )) {
    default:
    case BRB_FAIL:
        return FALSE;

    case BRB_MORE:
        return TRUE;

    case BRB_DONE:
        break;
    }

    //
    //  The buffer is now completely filled with our Reparse data.  Set the
    //  Reparse data on the file.
    //


    rp_tag_ptr = (DWORD *)(pbuc->DataBuffer.Buffer) ;
    
    pbuc->fAttribs |= FILE_ATTRIBUTE_REPARSE_POINT ;

    if ( IsReparseTagHighLatency(*rp_tag_ptr) ) {
         pbuc->fRemoteData = TRUE ;
    }

    Status = NtFsControlFile( hFile,
                     NULL,  // overlapped event handle
                     NULL,  // Apc routine
                     NULL,  // overlapped structure
                     &iosb,
                     FSCTL_SET_REPARSE_POINT,
                     pbuc->DataBuffer.Buffer,
                     pbuc->head.Size.LowPart,
                     NULL,
                     0 ) ;
    
    //
    //  If we failed, map the error and return failure
    //
    
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return FALSE;
    }
    
    return TRUE;
}


BOOL
BackupWriteObjectId(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    This is an internal routine that writes the Object IDa on the file from 
    the user's buffer. Birth ids are made reborn. i.e. the volume id component
    of the birth id is changed to the current volume's id, and the object id
    component of the birth id is changed to the current object id.

Arguments:

    hFile - handle to output file
    
    pbuc - CONTEXT of call
    
    pbif - IOCONTEXT of call
    
Return Value:

    TRUE if Object ID was successfully written, FALSE otherwise.

--*/
{
    IO_STATUS_BLOCK iosb;
    NTSTATUS  Status ;
    FILE_FS_OBJECTID_INFORMATION fsobOID;
    GUID guidZero;

    //
    //  Attempt to fill up the buffer from the input.
    //
    
    switch (BackupWriteBuffer( pbuc, pbif )) {
    default:
    case BRB_FAIL:
        return FALSE;

    case BRB_MORE:
        return TRUE;

    case BRB_DONE:
        break;
    }

    //
    // Zero out the birth ID (the extended 48 bytes)
    //

    memset(&pbuc->DataBuffer.Buffer[sizeof(GUID)], 0, 3*sizeof(GUID));

    //
    //  Set the ID on the file.
    //
    
    Status = NtFsControlFile( hFile,
                     NULL,  // overlapped event handle
                     NULL,  // Apc routine
                     NULL,  // overlapped structure
                     &iosb,
                     FSCTL_SET_OBJECT_ID,
                     pbuc->DataBuffer.Buffer,
                     pbuc->head.Size.LowPart,
                     NULL,
                     0);


    //
    //  Ignore errors
    //

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
    }

    return( TRUE );
    
}




BOOL
BackupWriteSecurityData(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    This is an internal routine that sets security information on the 
    file from data in the user's buffer.

Arguments:

    hFile - handle to output file
    
    pbuc - CONTEXT of call
    
    pbif - IOCONTEXT of call
    
Return Value:

    TRUE if security was successfully written, FALSE otherwise.

--*/
{
    NTSTATUS Status;
    SECURITY_INFORMATION si;

    //
    //  Attempt to fill up the buffer from the input.
    //
    
    switch (BackupWriteBuffer(pbuc, pbif)) {
    default:
    case BRB_FAIL:
        return FALSE;

    case BRB_MORE:
        return TRUE;

    case BRB_DONE:
        break;
    }

    //
    //  The buffer is now completely filled with our security data.  If we 
    //  are to ignore it, then return success
    //
    
    if (!pbif->fProcessSecurity) {
        return TRUE;
    }
    
    //  
    //  Find out what security information is present so we know what to 
    //  set.

    si = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION;

    if (((PISECURITY_DESCRIPTOR) pbuc->DataBuffer.Buffer)->Control & SE_DACL_PRESENT) {
        si |= DACL_SECURITY_INFORMATION;
    }

    if (((PISECURITY_DESCRIPTOR) pbuc->DataBuffer.Buffer)->Control & SE_SACL_PRESENT) {
        si |= SACL_SECURITY_INFORMATION;
    }

    Status = NtSetSecurityObject( hFile, si, pbuc->DataBuffer.Buffer );

    if (!NT_SUCCESS( Status )) {

        NTSTATUS Status2;

        //
        //  If that didn't work, the caller is probably not running as Backup
        //  Operator, so we can't set the owner and group.  Keep the current
        //  status code, and attempt to set the DACL and SACL while ignoring
        //  failures.
        //

        if (si & SACL_SECURITY_INFORMATION) {
            NtSetSecurityObject(
                        hFile,
                        SACL_SECURITY_INFORMATION,
                        pbuc->DataBuffer.Buffer );
        }

        if (si & DACL_SECURITY_INFORMATION) {
            Status = NtSetSecurityObject(
                            hFile,
                            DACL_SECURITY_INFORMATION,
                            pbuc->DataBuffer.Buffer);
        }

        Status2 = NtSetSecurityObject(
                            hFile,
                            OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION,
                            pbuc->DataBuffer.Buffer);

        if (NT_SUCCESS(Status)) {
            Status = Status2;
        }
    }

    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }
    
    return TRUE;
}



BOOL
BackupWritePropertyData(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    This is an internal routine that sets property data on the file from
    data in the user's buffer.

Arguments:

    hFile - handle to file.
    
    pbuc - CONTEXT of call.  This contains the name of the property
        set.
    
    pbif - IOCONTEXT of call
    
Return Value:

    TRUE if property data was successfully restored, FALSE otherwise.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING strName;
    OBJECT_ATTRIBUTES oa;

    //
    //  Attempt to fill up the buffer from the input.  When we are
    //  doing this the first time, make sure we account for a 
    //  PROPERTY_WRITE_CONTROL header.
    //
    
    if (pbuc->fStreamStart) {
        pbuc->head.Size.QuadPart += sizeof( PROPERTY_WRITE_CONTROL );
        ASSERT( pbuc->liStreamOffset == 0 );
        pbuc->liStreamOffset += sizeof( PROPERTY_WRITE_CONTROL );
    }
    
    switch (BackupWriteBuffer( pbuc, pbif )) {
    default:
    case BRB_FAIL:
        return FALSE;

    case BRB_MORE:
        return TRUE;

    case BRB_DONE:
        break;
    }

    //
    //  The buffer is now completely filled with our property data.  Attempt
    //  to create/open the stream.
    //
    
    if (pbuc->hAlternate != INVALID_HANDLE_VALUE) {
         CloseHandle(pbuc->hAlternate);        
         pbuc->hAlternate = INVALID_HANDLE_VALUE;
    }

    strName.Length = (USHORT) pbuc->head.dwStreamNameSize;
    strName.MaximumLength = strName.Length;
    strName.Buffer = pbuc->head.cStreamName;

    InitializeObjectAttributes(
            &oa,
            &strName,
            OBJ_CASE_INSENSITIVE,
            hFile,
            NULL);

    Status = NtCreateFile(
                &pbuc->hAlternate,
                FILE_WRITE_EA | SYNCHRONIZE,
                &oa,
                &iosb,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OVERWRITE_IF,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                NULL,
                0L);

    //
    //  If we failed, map the error, record the failure, and return failure.
    //
    
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        pbuc->fAccessError = TRUE;
        return FALSE;
    }

    //
    //  The buffer has the format of PRC_READ_ALL property buffer.  This
    //  means that there is enough room for us to perform a PWC_WRITE_ALL
    //  as well.
    //

    //
    //  Write all property data back out
    //

    ((PPROPERTY_WRITE_CONTROL) pbuc->DataBuffer.Buffer)->Op = PWC_WRITE_ALL;

    Status = NtFsControlFile( pbuc->hAlternate,
                              NULL,
                              NULL,
                              NULL,
                              &iosb,
                              FSCTL_WRITE_PROPERTY_DATA,
                              pbuc->DataBuffer.Buffer,
                              pbuc->head.Size.LowPart,
                              NULL,
                              0 );

    //
    //  If we failed, map the error and return failure
    //
    
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return FALSE;
    }
    
    return TRUE;
}


BOOL
BackupWriteLinkData(HANDLE hFile, BACKUPCONTEXT *pbuc, BACKUPIOFRAME *pbif)
/*++

Routine Description:

    This is an internal routine that establishes links based on the
    user's data.

Arguments:

    hFile - handle of file being restored
    
    pbuc - CONTEXT of call
    
    pbif - IOCONTEXT of call
    
Return Value:

    TRUE if link was successfully established, FALSE otherwise.

--*/
{
    FILE_LINK_INFORMATION *pfli;
    WCHAR *pwc;
    WCHAR *pwcSlash;
    INT cbName;
    INT cSlash;
    WCHAR wcSave;
    BOOL fSuccess;

    //
    //  Attempt to fill up the buffer from the input.
    //
    
    switch (BackupWriteBuffer(pbuc, pbif)) {
    default:
    case BRB_FAIL:
        return FALSE;

    case BRB_MORE:
        return TRUE;

    case BRB_DONE:
        break;
    }

    //
    //  The buffer is now completely filled with our link data.  
    //  Find the last component of the name.
    //
    
    cSlash = 0;
    pwcSlash = NULL;
    pwc = (WCHAR *) pbuc->DataBuffer.Buffer;
    cbName = sizeof(WCHAR);

    while (*pwc != L'\0') {
        if (*pwc == L'\\') {
            pwcSlash = pwc;
            cSlash++;
            cbName = 0;
        }
        pwc++;
        cbName += sizeof(WCHAR);
    }

    pfli = BackupAlloc( sizeof(*pfli) + cbName );

    if (pfli == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    RtlCopyMemory( pfli->FileName, pwcSlash + 1, cbName );
    pfli->FileNameLength = cbName - sizeof(WCHAR);
    if (cSlash > 1) {
        wcSave = L'\\';
    }
    else {
        wcSave = *pwcSlash++;
    }
    *pwcSlash = L'\0';

    //
    //  Open the parent of the link target
    //
    
    pfli->RootDirectory = CreateFileW(
        (WCHAR *) pbuc->DataBuffer.Buffer,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL| FILE_FLAG_BACKUP_SEMANTICS,
        NULL );

    *pwcSlash = wcSave;
    pfli->ReplaceIfExists = TRUE;

    fSuccess = TRUE;

    if (pfli->RootDirectory == INVALID_HANDLE_VALUE) {
        SetLastError( ERROR_FILE_NOT_FOUND );
        fSuccess = FALSE;
    }
    else {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;

        Status = NtSetInformationFile(
                    hFile,
                    &iosb,
                    pfli,
                    sizeof(*pfli) + cbName,
                    FileLinkInformation );

        CloseHandle( pfli->RootDirectory );
        pfli->RootDirectory = INVALID_HANDLE_VALUE;
        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            fSuccess = FALSE;
        } else {
            if (iosb.Information == FILE_OVERWRITTEN) {
                SetLastError( ERROR_ALREADY_EXISTS );
            } else {
                SetLastError( 0 );
            }
        }
    }
    
    BackupFree( pfli );
    
    return fSuccess;
}



//  Routine Description:
//
//    Data can be written to a file using BackupWrite.
//
//    This API is used to Restore data to an object.  After the
//    write completes, the file pointer is adjusted by the number of bytes
//    actually written.
//
//    Unlike DOS, a NumberOfBytesToWrite value of zero does not truncate
//    or extend the file.  If this function is required, SetEndOfFile
//    should be used.
//
//  Arguments:
//
//    hFile - Supplies an open handle to a file that is to be written.  The
//          file handle must have been created with GENERIC_WRITE access to
//          the file.
//
//    lpBuffer - Supplies the address of the data that is to be written to
//          the file.
//
//    nNumberOfBytesToWrite - Supplies the number of bytes to write to the
//          file. Unlike DOS, a value of zero is interpreted a null write.
//
//    lpNumberOfBytesWritten - Returns the number of bytes written by this
//          call. Before doing any work or error processing, the API sets this
//          to zero.
//
//    bAbort - If true, then all resources associated with the context will
//          be released.
//
//    bProcessSecurity - If TRUE, then the NTFS ACL data will be written.
//          If FALSE, then the ACL stream will be ignored.
//
//    lpContext - Points to a buffer pointer setup and maintained by
//          BackupRead.
//
//
//  Return Value:
//
//    TRUE - The operation was a success.
//
//    FALSE - The operation failed.  Extended error status is
//          available using GetLastError.

BOOL WINAPI
BackupWrite(
    HANDLE  hFile,
    LPBYTE  lpBuffer,
    DWORD   nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    BOOL    bAbort,
    BOOL    bProcessSecurity,
    LPVOID  *lpContext)
{
    BACKUPCONTEXT *pbuc;
    BACKUPIOFRAME bif;
    BOOL fSuccess = FALSE;

    pbuc = *lpContext;
    bif.pIoBuffer = lpBuffer;
    bif.cbRequest = nNumberOfBytesToWrite;
    bif.pcbTransferred = lpNumberOfBytesWritten;
    bif.fProcessSecurity = (BOOLEAN)bProcessSecurity;

    //
    // Allocate our Context Control Block on first call.
    //

    if (bAbort) {
        if (pbuc != NULL) {
            FreeContext(lpContext);
        }
        return TRUE;
    }

    *bif.pcbTransferred = 0;
    if (pbuc == INVALID_HANDLE_VALUE) {
        return TRUE;
    }

    // Allocate our Context Control Block on first call.

    if (pbuc == NULL) {
        pbuc = AllocContext(0);                        // No initial buffer

        //
        //  If we have no space then return failure
        //
        
        if (pbuc == NULL) {
            return FALSE;           
        }

    }

    *lpContext = pbuc;

    do {
        DWORD cbrequest;
        LONGLONG licbRemain;

        //
        //  If we do not have a complete header, go
        //  fill it in.
        //
        
        if (pbuc->cbHeader == 0) {

            pbuc->fMultiStreamType = TRUE ;    //restore does not auto inc stream index.
            pbuc->fStreamStart = TRUE ;

            BackupWriteHeader(pbuc, &bif, CB_NAMELESSHEADER) ;

        }

        //
        //  If no more data, then exit
        //
        
        if (bif.cbRequest == 0) {
            return TRUE;
        }

        //
        //  If a stream name was expected, go read it in
        //
        
        if (pbuc->cbHeader == CB_NAMELESSHEADER &&
            pbuc->head.dwStreamNameSize != 0) {

            if ( !BackupWriteHeader(
                    pbuc,
                    &bif,
                    pbuc->cbHeader + pbuc->head.dwStreamNameSize) )
            {
                 SetLastError( ERROR_INVALID_DATA );
                 return FALSE ;
            }

            //
            //  If no more data then exit
            //
            
            if (bif.cbRequest == 0) {
                return TRUE;
            }
        } 

     
        if ( ( pbuc->cbHeader == CB_NAMELESSHEADER ) &&
             ( pbuc->head.dwStreamId == BACKUP_SPARSE_BLOCK ) ) {
      
            BackupWriteHeader(
                pbuc,
                &bif,
                pbuc->cbHeader + sizeof(LARGE_INTEGER) );

            //
            //  If no more data then exit
            //
            
            if (bif.cbRequest == 0) {

               if ( pbuc->cbHeader == CB_NAMELESSHEADER ) {
                   return TRUE;
               }
            }
        }
        
        //
        //  Determine amount of data remaining in user buffer
        //  that can be transferred as part of this section
        //  of the backup stream
        //

        cbrequest = ComputeRequestSize(pbuc, bif.cbRequest);

        //
        //  Determine total amount of data left in this section
        //  of backup stream.
        //
        
        licbRemain = ComputeRemainingSize( pbuc );

        //
        //  If we had an error in the transfer and we're done
        //  doing the transfer then pretend that we successfully 
        //  completed the section.
        //
        
        if (pbuc->fAccessError && licbRemain == 0) {

            ReportTransfer(pbuc, &bif, cbrequest);
            continue;
        }
        
        //
        //  Begin or continue the transfer of data.  We assume that there
        //  are no errors
        //
        
        pbuc->fAccessError = FALSE;

        switch (pbuc->head.dwStreamId) {

        case BACKUP_SPARSE_BLOCK :
            fSuccess = BackupWriteSparse( hFile, pbuc, &bif ) ;
            break ;

        case BACKUP_DATA:
            fSuccess = BackupWriteStream( hFile, pbuc, &bif );
            break;

        case BACKUP_ALTERNATE_DATA:
            fSuccess = BackupWriteAlternateData( hFile, pbuc, &bif );
            break;

        case BACKUP_EA_DATA:
            fSuccess = BackupWriteEaData( hFile, pbuc, &bif );
            break;

        case BACKUP_OBJECT_ID:
            fSuccess = BackupWriteObjectId( hFile, pbuc, &bif );
            break;

        case BACKUP_REPARSE_DATA:
            fSuccess = BackupWriteReparseData( hFile, pbuc, &bif );
            break;

        case BACKUP_SECURITY_DATA:
            fSuccess = BackupWriteSecurityData( hFile, pbuc, &bif );
            break;

        case BACKUP_LINK:
            fSuccess = BackupWriteLinkData( hFile, pbuc, &bif );
            break;

        case BACKUP_PROPERTY_DATA:
            fSuccess = BackupWritePropertyData( hFile, pbuc, &bif );
            break;
        
        default:
            SetLastError(ERROR_INVALID_DATA);
            fSuccess = FALSE;
            break;
        }

        BackupTestRestartStream(pbuc);
    } while (fSuccess && bif.cbRequest != 0);

    if (fSuccess && *bif.pcbTransferred == 0) {
        FreeContext(lpContext);
    }
    
    return(fSuccess);
}
