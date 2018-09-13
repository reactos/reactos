/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hiveload.c

Abstract:

    This module implements procedures to read a hive into memory, applying
    logs, etc.

    NOTE:   Alternate image loading is not supported here, that is
            done by the boot loader.

Author:

    Bryan M. Willman (bryanwi) 30-Mar-92

Environment:


Revision History:
    Dragos C. Sambotin (dragoss) 25-Jan-99
        Implementation of bin-size chunk loading of hives.
    Dragos C. Sambotin (dragoss) 10-Apr-99
        64K IO reads when loading the hive

--*/

#include    "cmp.h"

#define         IO_BUFFER_SIZE  0x10000  //64K

typedef enum _RESULT {
    NotHive,
    Fail,
    NoMemory,
    HiveSuccess,
    RecoverHeader,
    RecoverData
} RESULT;

RESULT
HvpGetHiveHeader(
    PHHIVE          Hive,
    PHBASE_BLOCK    *BaseBlock,
    PLARGE_INTEGER  TimeStamp
    );

RESULT
HvpGetLogHeader(
    PHHIVE          Hive,
    PHBASE_BLOCK    *BaseBlock,
    PLARGE_INTEGER  TimeStamp
    );

RESULT
HvpRecoverData(
    PHHIVE          Hive,
    BOOLEAN         ReadOnly,
    PHCELL_INDEX    TailDisplay OPTIONAL
    );

NTSTATUS
HvpReadFileImageAndBuildMap(
                            PHHIVE  Hive,
                            ULONG   Length,
                            PHCELL_INDEX TailDisplay OPTIONAL
                            );

VOID
HvpDelistBinFreeCells(
    PHHIVE  Hive,
    PHBIN   Bin,
    HSTORAGE_TYPE Type,
    PHCELL_INDEX TailDisplay OPTIONAL
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvLoadHive)
#pragma alloc_text(PAGE,HvpGetHiveHeader)
#pragma alloc_text(PAGE,HvpGetLogHeader)
#pragma alloc_text(PAGE,HvpRecoverData)
#pragma alloc_text(PAGE,HvpReadFileImageAndBuildMap)
#endif


extern struct {
    PHHIVE      Hive;
    ULONG       Status;
    ULONG       Space;
    HCELL_INDEX MapPoint;
    PHBIN       BinPoint;
} HvCheckHiveDebug;


NTSTATUS
HvLoadHive(
    PHHIVE  Hive,
    PHCELL_INDEX TailDisplay OPTIONAL
    )
/*++

Routine Description:

    Hive must be fully initialized, in particular, file handles
    must be set up.  This routine is not intended for loading hives
    from images already in memory.

    This routine will apply whatever fixes are available for errors
    in the hive image.  In particular, if a log exists, and is applicable,
    this routine will automatically apply it.

    ALGORITHM:

        call HvpGetHiveHeader()

        if (NoMemory or NoHive)
            return failure

        if (RecoverData or RecoverHeader) and (no log)
            return falure

        if (RecoverHeader)
            call HvpGetLogHeader
            if (fail)
                return failure
            fix up baseblock

        Read Data

        if (RecoverData or RecoverHeader)
            HvpRecoverData
            return STATUS_REGISTRY_RECOVERED

        clean up sequence numbers

        return success OR STATUS_REGISTRY_RECOVERED

    If STATUS_REGISTRY_RECOVERED is returned, then

        If (Log) was used, DirtyVector and DirtyCount are set,
            caller is expected to flush the changes (using a
            NEW log file)

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    TailDisplay - array containing the tail ends of the free cell lists - optional

Return Value:

    STATUS:

        STATUS_INSUFFICIENT_RESOURCES   - memory alloc failure, etc
        STATUS_NOT_REGISTRY_FILE        - bad signatures and the like
        STATUS_REGISTRY_CORRUPT         - bad signatures in the log,
                                          bad stuff in both in alternate,
                                          inconsistent log

        STATUS_REGISTRY_IO_FAILED       - data read failed

        STATUS_RECOVERED                - successfully recovered the hive,
                                          a semi-flush of logged data
                                          is necessary.

        STATUS_SUCCESS                  - it worked, no recovery needed

--*/
{
    PHBASE_BLOCK    BaseBlock;
    ULONG           result1;
    ULONG           result2;
    NTSTATUS        status;
    LARGE_INTEGER   TimeStamp;
    ULONG           FileOffset;
    PHBIN           pbin;
    BOOLEAN         ReadOnlyFlagCopy;

    ASSERT(Hive->Signature == HHIVE_SIGNATURE);

    BaseBlock = NULL;
    result1 = HvpGetHiveHeader(Hive, &BaseBlock, &TimeStamp);

    //
    // bomb out for total errors
    //
    if (result1 == NoMemory) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit1;
    }
    if (result1 == NotHive) {
        status = STATUS_NOT_REGISTRY_FILE;
        goto Exit1;
    }

    
    ReadOnlyFlagCopy = Hive->ReadOnly;
    //
    // if recovery needed, and no log, bomb out
    //
    if ( ((result1 == RecoverData) ||
          (result1 == RecoverHeader)) )
    {
        //
        // recovery needed
        //
        if(Hive->Log == FALSE)
        {
            //
            // no log ==> bomb out
            //
            status = STATUS_REGISTRY_CORRUPT;
            goto Exit1;
        } else {
            //
            // TRICK: simulate hive as read-only; Free cells will not 
            // be enlisted in HvpReadFileImageAndBuildMap; Instead, they
            // will be enlisted in HvpRecoverData, when we are sure we have 
            // the right info loaded up into memory
            //
            Hive->ReadOnly = TRUE;
        }
    }

    //
    // need to recover header using log, so try to get it from log
    //
    if (result1 == RecoverHeader) {
        result2 = HvpGetLogHeader(Hive, &BaseBlock, &TimeStamp);
        if (result2 == NoMemory) {
            status =  STATUS_INSUFFICIENT_RESOURCES;
            goto Exit1;
        }
        if (result2 == Fail) {
            status = STATUS_REGISTRY_CORRUPT;
            goto Exit1;
        }
        BaseBlock->Type = HFILE_TYPE_PRIMARY;
    }
    Hive->BaseBlock = BaseBlock;
    Hive->Version = Hive->BaseBlock->Minor;

    //
    // at this point, we have a sane baseblock.  we may or may not still
    // need to apply data recovery
    //
    status = HvpReadFileImageAndBuildMap(Hive,BaseBlock->Length,TailDisplay);
    
    
    //
    // if STATUS_REGISTRY_CORRUPT and RecoverData don't bail out, keep recovering
    //
    if( !NT_SUCCESS(status) && ((status != STATUS_REGISTRY_CORRUPT) || (result1 != RecoverData)) ) {
        goto Exit2;
    }
    
    //
    // apply data recovery if we need it
    //
    status = STATUS_SUCCESS;
    if ( (result1 == RecoverHeader) ||      // -> implies recover data
         (result1 == RecoverData) )
    {
        //
        // recover data will enllist the free cells as well and 
        // will restore the original read-only state of the hive
        //
        result2 = HvpRecoverData(Hive,ReadOnlyFlagCopy,TailDisplay);
        if (result2 == NoMemory) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit2;
        }
        if (result2 == Fail) {
            status = STATUS_REGISTRY_CORRUPT;
            goto Exit2;
        }
        status = STATUS_REGISTRY_RECOVERED;
    }

    BaseBlock->Sequence2 = BaseBlock->Sequence1;
    return status;


Exit2:
    //
    // Clean up the bins already allocated 
    //
    HvpFreeAllocatedBins( Hive );

    //
    // Clean up the directory table
    //
    HvpCleanMap( Hive );

Exit1:
    if (BaseBlock != NULL) {
        (Hive->Free)(BaseBlock, sizeof(HBASE_BLOCK));
    }

    Hive->BaseBlock = NULL;
    Hive->DirtyCount = 0;
    return status;
}

NTSTATUS
HvpReadFileImageAndBuildMap(
                            PHHIVE  Hive,
                            ULONG   Length,
                            PHCELL_INDEX TailDisplay OPTIONAL
                            )

/*++

Routine Description:

    Read the hive from the file and allocate storage for the hive
    image in chunks of HBINs. Build the hive map "on the fly".
        Optimized to read chunks of 64K from the file.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Length - the length of the hive, in bytes

    TailDisplay - array containing the tail ends of the free cell lists - optional

Return Value:

    STATUS:

        STATUS_INSUFFICIENT_RESOURCES   - memory alloc failure, etc

        STATUS_REGISTRY_IO_FAILED       - data read failed

        STATUS_REGISTRY_CORRUPT         - base block is corrupt

        STATUS_SUCCESS                  - it worked

--*/
{
    ULONG           FileOffset;
    NTSTATUS        Status = STATUS_SUCCESS;
    PHBIN           Bin;                        // current bin
    ULONG           BinSize;            // size of the current bin
    ULONG           BinOffset;          // current offset inside current bin
    ULONG           BinFileOffset;  // physical offset of the bin in the file (used for consistency checking)
    ULONG           BinDataInBuffer;// the amount of data needed to be copied in the current bin available in the buffer
    ULONG           BinDataNeeded;  // 
    PUCHAR                      IOBuffer;
    ULONG           IOBufferSize;       // valid data in IOBuffer (only at the end of the file this is different than IO_BUFFER_SIZE)
    ULONG           IOBufferOffset;     // current offset inside IOBuffer

    //
    // Init the map
    //
    Status = HvpInitMap(Hive);

    if( !NT_SUCCESS(Status) ) {
        //
        // return failure 
        //
        return Status;
    }

    //
    // Allocate a IO_BUFFER_SIZE for I/O operations from paged pool. 
        // It will be freed at the end of the function.
    //
    IOBuffer = (PUCHAR)ExAllocatePool(PagedPool, IO_BUFFER_SIZE);
    if (IOBuffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        HvpCleanMap( Hive );
        return Status;
    }

    //
    // Start right after the hive header
    //
    FileOffset = HBLOCK_SIZE;
    BinFileOffset = FileOffset;
    Bin = NULL;

    //
        // outer loop : reads IO_BUFFER_SIZE chunks from the file
        //
        while( FileOffset < (Length + HBLOCK_SIZE) ) {
                //
                // we are at the begining of the IO buffer
        //
        IOBufferOffset = 0;

        //
        // the buffer size will be either IO_BufferSize, or the amount 
        // uread from the file (when this is smaller than IO_BUFFER_SIZE)
        //
        IOBufferSize = Length + HBLOCK_SIZE - FileOffset;
        IOBufferSize = ( IOBufferSize > IO_BUFFER_SIZE ) ? IO_BUFFER_SIZE : IOBufferSize;
        
        ASSERT( (IOBufferSize % HBLOCK_SIZE) == 0 );
        
        //
        // read data from the file
        //
        if ( ! (Hive->FileRead)(
                        Hive,
                        HFILE_TYPE_PRIMARY,
                        &FileOffset,
                        (PVOID)IOBuffer,
                        IOBufferSize
                        )
           )
        {
            Status = STATUS_REGISTRY_IO_FAILED;
            goto ErrorExit;
        }
        
        //
        // inner loop: breaks the buffer into bins
        //
        while( IOBufferOffset < IOBufferSize ) {

            if( Bin == NULL ) {
                //
                // this is the beginning of a new bin
                // perform bin validation and allocate the bin
                //
                // temporary bin points to the current location inside the buffer
                Bin = (PHBIN)(IOBuffer + IOBufferOffset);
                //
                // Check the validity of the bin header
                //
                BinSize = Bin->Size;
                if ( (BinSize > Length)                         ||
                     (BinSize < HBLOCK_SIZE)                    ||
                     (Bin->Signature != HBIN_SIGNATURE)         ||
                     (Bin->FileOffset != (BinFileOffset - HBLOCK_SIZE) )) {
                    //
                    // Bin is bogus
                    //
                    Bin = (PHBIN)(Hive->Allocate)(HBLOCK_SIZE, TRUE);
                    if (Bin == NULL) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto ErrorExit;
                    }
                    //
                    // copy the data already read in the first HBLOCK of the bin
                    //
                    RtlCopyMemory(Bin,(IOBuffer + IOBufferOffset), HBLOCK_SIZE);
                    
                    Status = STATUS_REGISTRY_CORRUPT;
                    HvCheckHiveDebug.Hive = Hive;
                    HvCheckHiveDebug.Status = 0xA001;
                    HvCheckHiveDebug.Space = Length;
                    HvCheckHiveDebug.MapPoint = BinFileOffset;
                    HvCheckHiveDebug.BinPoint = Bin;
            
                    //goto ErrorExit;
                    //
                    // DO NOT EXIT; Fix this bin header and go on. RecoverData should fix it.
                    // If not, CmCheckRegistry called later will prevent loading of an invalid hive
                    //
                    // NOTE: Still, mess the signature, to make sure that if this particular bin doesn't get recovered, 
                    //       we'll fail the hive loading request.
                    //
                    Bin->Signature = 0; //TRICK!!!!
                    BinSize = Bin->Size = HBLOCK_SIZE;
                    Bin->FileOffset = BinFileOffset - HBLOCK_SIZE;

                    //
                    // simulate as the entire bin is a used cell
                    //
                    ((PHCELL)((PUCHAR)Bin + sizeof(HBIN)))->Size = sizeof(HBIN) - BinSize; //TRICK!!!!
                    //
                    // Now that we have the entire bin in memory, Enlist It!
                    //
                    Status = HvpEnlistBinInMap(Hive, Length, Bin, BinFileOffset - HBLOCK_SIZE, TailDisplay);

                    if( !NT_SUCCESS(Status) ) {
                        goto ErrorExit;
                    }
                    
                    //
                    // Adjust the offsets
                    //
                    BinFileOffset += Bin->Size;
                    IOBufferOffset += Bin->Size;
                    
                    //
                    // another bin is on his way 
                    //
                    Bin = NULL;
                } else {
                    //
                    // bin is valid; allocate a pool chunk of the right size
                    //
                    Bin = (PHBIN)(Hive->Allocate)(BinSize, TRUE);
                    if (Bin == NULL) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto ErrorExit;
                    }
            
                    //
                    // the chunk is allocated; set the offset inside the bin and continue
                    // the next iteration of the inner loop will start by copying data in this bin
                    //
                    BinOffset = 0;
                }
            } else {
                //
                // if we are here, the bin is allocated, the BinSize and BinOffset are set
                // We have to calculate how much for this bin is available in the buffer,
                // and copy it. If we finished with this bin, enlist it and mark the begining of a new one
                //
                ASSERT( Bin != NULL );
                BinDataInBuffer = (IOBufferSize - IOBufferOffset);
                BinDataNeeded = (BinSize - BinOffset);
                
                if( BinDataInBuffer >= BinDataNeeded ) {
                    //
                    // we have available more than what we need; Finish the bin
                    //
                    RtlCopyMemory(((PUCHAR)Bin + BinOffset),(IOBuffer + IOBufferOffset), BinDataNeeded);
                    //
                    // enlist it
                    //
                    Status = HvpEnlistBinInMap(Hive, Length, Bin, BinFileOffset - HBLOCK_SIZE, TailDisplay);

                    if( !NT_SUCCESS(Status) ) {
                        goto ErrorExit;
                    }
                    //
                    // Adjust the offsets
                    //
                    BinFileOffset += BinSize;
                    IOBufferOffset += BinDataNeeded;
                    //
                    // mark the begining of a new bin
                    //
                    Bin = NULL;
                } else {
                    //
                    // we do not have all bin data in the buffer
                    // copy what we can 
                    //
                    RtlCopyMemory(((PUCHAR)Bin + BinOffset),(IOBuffer + IOBufferOffset), BinDataInBuffer);
                    
                    //
                    // adjust the offsets; this should be the last iteration of the inner loop
                    //
                    BinOffset += BinDataInBuffer;
                    IOBufferOffset += BinDataInBuffer;

                    // 
                    // if we are here, the buffer must have beed exausted  
                    //
                    ASSERT( IOBufferOffset == IOBufferSize );
                }
            }
        }
        }

    //
    // if we got here, we shouldn't have a bin under construction
    //
    ASSERT( Bin == NULL );

        //
        // Free the buffer used for I/O operations
        //
        ExFreePool(IOBuffer);

    return Status;

ErrorExit:
    //
    // Free the buffer used for I/O operations
    //
    ExFreePool(IOBuffer);

    return Status;
}


RESULT
HvpGetHiveHeader(
    PHHIVE          Hive,
    PHBASE_BLOCK    *BaseBlock,
    PLARGE_INTEGER  TimeStamp
    )
/*++

Routine Description:

    Examine the base block sector and possibly the first sector of
    the first bin, and decide what (if any) recovery needs to be applied
    based on what we find there.

    ALGORITHM:

        read BaseBlock from offset 0
        if ( (I/O error)    OR
             (checksum wrong) )
        {
            read bin block from offset HBLOCK_SIZE (4k)
            if (2nd I/O error)
                return NotHive
            }
            check bin sign., offset.
            if (OK)
                return RecoverHeader, TimeStamp=from Link field
            } else {
                return NotHive
            }
        }

        if (wrong type or signature or version or format)
            return NotHive
        }

        if (seq1 != seq2) {
            return RecoverData, TimeStamp=BaseBlock->TimeStamp, valid BaseBlock
        }

        return ReadData, valid BaseBlock

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    FileType - HFILE_TYPE_PRIMARY or HFILE_TYPE_ALTERNATE - which copy
            of the hive to read from.

    BaseBlock - supplies pointer to variable to receive pointer to
            HBASE_BLOCK, if we can successfully read one.

    TimeStamp - pointer to variable to receive time stamp (serial number)
            of hive, be it from the baseblock or from the Link field
            of the first bin.

Return Value:

    RESULT code

--*/
{
    PHBASE_BLOCK    buffer;
    BOOLEAN         rc;
    ULONG           FileOffset;
    ULONG           Alignment;

    ASSERT(sizeof(HBASE_BLOCK) >= (HSECTOR_SIZE * Hive->Cluster));

    //
    // allocate buffer to hold base block
    //
    *BaseBlock = NULL;
    buffer = (PHBASE_BLOCK)((Hive->Allocate)(sizeof(HBASE_BLOCK), TRUE));
    if (buffer == NULL) {
        return NoMemory;
    }
    //
    // Make sure the buffer we got back is cluster-aligned. If not, try
    // harder to get an aligned buffer.
    //
    Alignment = Hive->Cluster * HSECTOR_SIZE - 1;
    if (((ULONG_PTR)buffer & Alignment) != 0) {
        (Hive->Free)(buffer, sizeof(HBASE_BLOCK));
        buffer = (PHBASE_BLOCK)((Hive->Allocate)(PAGE_SIZE, TRUE));
        if (buffer == NULL) {
            return NoMemory;
        }
    }
    RtlZeroMemory((PVOID)buffer, sizeof(HBASE_BLOCK));

    //
    // attempt to read base block
    //
    FileOffset = 0;
    rc = (Hive->FileRead)(Hive,
                          HFILE_TYPE_PRIMARY,
                          &FileOffset,
                          (PVOID)buffer,
                          HSECTOR_SIZE * Hive->Cluster);

    if ( (rc == FALSE)  ||
         (HvpHeaderCheckSum(buffer) != buffer->CheckSum)) {
        //
        // base block is toast, try the first block in the first bin
        //
        FileOffset = HBLOCK_SIZE;
        rc = (Hive->FileRead)(Hive,
                              HFILE_TYPE_PRIMARY,
                              &FileOffset,
                              (PVOID)buffer,
                              HSECTOR_SIZE * Hive->Cluster);

        if ( (rc == FALSE) ||
             ( ((PHBIN)buffer)->Signature != HBIN_SIGNATURE)           ||
             ( ((PHBIN)buffer)->FileOffset != 0)
           )
        {
            //
            // the bin is toast too, punt
            //
            (Hive->Free)(buffer, sizeof(HBASE_BLOCK));
            return NotHive;
        }

        //
        // base block is bogus, but bin is OK, so tell caller
        // to look for a log file and apply recovery
        //
        *TimeStamp = ((PHBIN)buffer)->TimeStamp;
        (Hive->Free)(buffer, sizeof(HBASE_BLOCK));
        return RecoverHeader;
    }

    //
    // base block read OK, but is it valid?
    //
    if ( (buffer->Signature != HBASE_BLOCK_SIGNATURE)   ||
         (buffer->Type != HFILE_TYPE_PRIMARY)           ||
         (buffer->Major != HSYS_MAJOR)                  ||
         (buffer->Minor > HSYS_MINOR)                   ||
         ((buffer->Major == 1) && (buffer->Minor == 0)) ||
         (buffer->Format != HBASE_FORMAT_MEMORY)
       )
    {
        //
        // file is simply not a valid hive
        //
        (Hive->Free)(buffer, sizeof(HBASE_BLOCK));
        return NotHive;
    }

    //
    // see if recovery is necessary
    //
    *BaseBlock = buffer;
    *TimeStamp = buffer->TimeStamp;
    if ( (buffer->Sequence1 != buffer->Sequence2) ) {
        return RecoverData;
    }

    return HiveSuccess;
}


RESULT
HvpGetLogHeader(
    PHHIVE          Hive,
    PHBASE_BLOCK    *BaseBlock,
    PLARGE_INTEGER  TimeStamp
    )
/*++

Routine Description:

    Read and validate log file header.  Return it if it's valid.

    ALGORITHM:

        read header
        if ( (I/O error) or
           (wrong signature,
            wrong type,
            seq mismatch
            wrong checksum,
            wrong timestamp
           )
            return Fail
        }
        return baseblock, OK

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    BaseBlock - supplies pointer to variable to receive pointer to
            HBASE_BLOCK, if we can successfully read one.

    TimeStamp - pointer to variable holding TimeStamp, which must
            match the one in the log file.

Return Value:

    RESULT

--*/
{
    PHBASE_BLOCK    buffer;
    BOOLEAN         rc;
    ULONG           FileOffset;

    ASSERT(sizeof(HBASE_BLOCK) == HBLOCK_SIZE);
    ASSERT(sizeof(HBASE_BLOCK) >= (HSECTOR_SIZE * Hive->Cluster));

    //
    // allocate buffer to hold base block
    //
    *BaseBlock = NULL;
    buffer = (PHBASE_BLOCK)((Hive->Allocate)(sizeof(HBASE_BLOCK), TRUE));
    if (buffer == NULL) {
        return NoMemory;
    }
    RtlZeroMemory((PVOID)buffer, HSECTOR_SIZE);

    //
    // attempt to read base block
    //
    FileOffset = 0;
    rc = (Hive->FileRead)(Hive,
                          HFILE_TYPE_LOG,
                          &FileOffset,
                          (PVOID)buffer,
                          HSECTOR_SIZE * Hive->Cluster);

    if ( (rc == FALSE)                                              ||
         (buffer->Signature != HBASE_BLOCK_SIGNATURE)               ||
         (buffer->Type != HFILE_TYPE_LOG)                           ||
         (buffer->Sequence1 != buffer->Sequence2)                   ||
         (HvpHeaderCheckSum(buffer) != buffer->CheckSum)            ||
         (TimeStamp->LowPart != buffer->TimeStamp.LowPart)          ||
         (TimeStamp->HighPart != buffer->TimeStamp.HighPart)) {
        //
        // Log is unreadable, invalid, or doesn't apply the right hive
        //
        (Hive->Free)(buffer, sizeof(HBASE_BLOCK));
        return Fail;
    }

    *BaseBlock = buffer;
    return HiveSuccess;
}


RESULT
HvpRecoverData(
    PHHIVE          Hive,
    BOOLEAN         ReadOnly,
    PHCELL_INDEX    TailDisplay OPTIONAL
    )
/*++

Routine Description:

    Apply the corrections in the log file to the hive memory image.

    ALGORITHM:

        compute size of dirty vector
        read in dirty vector
        if (i/o error)
            return Fail

        skip first cluster of data (already processed as log)
        sweep vector, looking for runs of bits
            address of first bit is used to compute memory offset
            length of run is length of block to read
            assert always a cluster multiple
            file offset kept by running counter
            read
            if (i/o error)
                return fail

        return success

    NOTE:   It is assumed that the data part of the Hive has been
            read into a single contiguous block, at Image.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    ReadOnly - by the time this function is called, the hive is forced to the
            ready-only state. At the end, if recovery goes OK, we restore the 
            hive at it's original state, and enlist all free cells.

    TailDisplay - array containing the tail ends of the free cell lists - optional

Return Value:

    RESULT

--*/
{
    ULONG       Cluster;
    ULONG       ClusterSize;
    ULONG       VectorSize;
    PULONG      Vector;
    ULONG       FileOffset;
    BOOLEAN     rc;
    ULONG       Current;
    ULONG       Start;
    ULONG       End;
    ULONG       Address;
    PUCHAR      MemoryBlock;
    RTL_BITMAP  BitMap;
    ULONG       Length;
    ULONG       DirtyVectorSignature = 0;
    ULONG       i;
    PHMAP_ENTRY Me;
    PHBIN       Bin;
    PHBIN       NewBin;
    PUCHAR      SectorImage;
    PUCHAR      Source;
    PHBIN       SourceBin;
    ULONG       SectorOffsetInBin;
    ULONG       SectorOffsetInBlock;
    ULONG       BlockOffsetInBin;
    ULONG       NumberOfSectors;

    //
    // compute size of dirty vector, read and check signature, read vector
    //
    Cluster = Hive->Cluster;
    ClusterSize = Cluster * HSECTOR_SIZE;
    Length = Hive->BaseBlock->Length;
    VectorSize = (Length / HSECTOR_SIZE) / 8;       // VectorSize == Bytes
    FileOffset = ClusterSize;

    //
    // get and check signature
    //
    rc = (Hive->FileRead)(
            Hive,
            HFILE_TYPE_LOG,
            &FileOffset,
            (PVOID)&DirtyVectorSignature,
            sizeof(DirtyVectorSignature)
            );
    if (rc == FALSE) {
        return Fail;
    }

    if (DirtyVectorSignature != HLOG_DV_SIGNATURE) {
        return Fail;
    }

    //
    // get the actual vector
    //
    Vector = (PULONG)((Hive->Allocate)(ROUND_UP(VectorSize,sizeof(ULONG)), TRUE));
    if (Vector == NULL) {
        return NoMemory;
    }
    rc = (Hive->FileRead)(
            Hive,
            HFILE_TYPE_LOG,
            &FileOffset,            // dirty vector right after header
            (PVOID)Vector,
            VectorSize
            );
    if (rc == FALSE) {
        (Hive->Free)(Vector, VectorSize);
        return Fail;
    }
    FileOffset = ROUND_UP(FileOffset, ClusterSize);


    //
    // step through the diry map, reading in the corresponding file bytes
    //
    Current = 0;
    VectorSize = VectorSize * 8;        // VectorSize == bits

    RtlInitializeBitMap(&BitMap, Vector, VectorSize);

    while (Current < VectorSize) {

        //
        // find next contiguous block of entries to read in
        //
        for (i = Current; i < VectorSize; i++) {
            if (RtlCheckBit(&BitMap, i) == 1) {
                break;
            }
        }
        Start = i;

        for ( ; i < VectorSize; i++) {
            if (RtlCheckBit(&BitMap, i) == 0) {
                break;
            }
        }
        End = i;
        Current = End;

        //
        // Start == number of 1st sector, End == number of Last sector + 1
        //
        Length = (End - Start) * HSECTOR_SIZE;

        if( 0 == Length ) {
            // no more dirty blocks.
            break;
        }
        //
        // allocate a buffer to read the whole run from the file; This is a temporary
        // block that'll be freed immediately, so don't charge quota for it.
        //
        MemoryBlock = (PUCHAR)ExAllocatePoolWithTag(PagedPool, Length, CM_POOL_TAG);
        if( MemoryBlock == NULL ) {        
            goto ErrorExit;
        }

        rc = (Hive->FileRead)(
                Hive,
                HFILE_TYPE_LOG,
                &FileOffset,
                (PVOID)MemoryBlock,
                Length
                );

        ASSERT((FileOffset % ClusterSize) == 0);
        if (rc == FALSE) {
            ExFreePool(MemoryBlock);
            goto ErrorExit;
        }
        
        Source = MemoryBlock;
        //
        // copy recovered data in the right locations inside the in-memory bins
        //
        while( Start < End ) {
            Address = Start * HSECTOR_SIZE;
        
            Me = HvpGetCellMap(Hive, Address);
            VALIDATE_CELL_MAP(__LINE__,Me,Hive,Address);
    
            Bin = (PHBIN)(Me->BinAddress & HMAP_BASE);
            //
            // compute the memory address where data should be copied
            //
            SectorOffsetInBin = Address - Bin->FileOffset;
            
            if( ( SectorOffsetInBin == 0 ) && ( ((PHBIN)Source)->Size > Bin->Size ) ){
                //
                // Bin in the log file is bigger than the one in memory;
                // two or more bins must have been coalesced
                //
                ASSERT( Me->BinAddress & HMAP_NEWALLOC );
                
                SourceBin = (PHBIN)Source;

                //
                // new bin must have the right offset
                //
                ASSERT( Address == SourceBin->FileOffset );
                ASSERT( SourceBin->Signature == HBIN_SIGNATURE );
                //
                // entire bin should be dirty
                //
                ASSERT( (SourceBin->FileOffset + SourceBin->Size) <= End * HSECTOR_SIZE );

                //
                // Allocate the right size for the new bin
                //
                NewBin = (PHBIN)(Hive->Allocate)(SourceBin->Size, TRUE);
                if (NewBin == NULL) {
                    goto ErrorExit;
                }
                
                //
                // Copy the old data into the new bin and free old bins
                //
                while(Bin->FileOffset < (Address + SourceBin->Size)) {
                    
                    RtlCopyMemory((PUCHAR)NewBin + (Bin->FileOffset - Address),Bin, Bin->Size);

                    //
                    // Do not delist as we didn't enlisted  (when hive needs recovery)
                    //
                    //HvpDelistBinFreeCells(Hive,Bin,Stable,TailDisplay);

                    //
                    // Advance to the new bin
                    //
                    if( (Bin->FileOffset + Bin->Size) < Hive->BaseBlock->Length ) {
                        Me = HvpGetCellMap(Hive, Bin->FileOffset + Bin->Size);
                        VALIDATE_CELL_MAP(__LINE__,Me,Hive,Bin->FileOffset + Bin->Size);

                        //
                        // Free the old bin
                        //
                        (Hive->Free)(Bin, Bin->Size);
            
                        //
                        // the new address must be the begining of a new allocation
                        //
                        ASSERT( Me->BinAddress & HMAP_NEWALLOC );
                    
                        Bin = (PHBIN)(Me->BinAddress & HMAP_BASE);
                    } else {
                        //
                        // we are at the end of the hive here; just break out of the loop
                        //
                        ASSERT( (Address + SourceBin->Size) == Hive->BaseBlock->Length );
                        ASSERT( (Bin->FileOffset + Bin->Size) == Hive->BaseBlock->Length );
                        
                        //
                        // Free the old bin
                        //
                        (Hive->Free)(Bin, Bin->Size);
                        
                        //
                        // debug purposes only
                        //
                        ASSERT( (Bin = NULL) == NULL );

                        // bail out of while loop
                        break;
                    }
    
                }

#if DBG
                //
                // validation: bin size increase must come from coalescing of former bins
                // (i.e. bins are never split!!!)
                //
                if( Bin != NULL ) {
                    ASSERT( Bin->FileOffset == (Address + SourceBin->Size));
                } 
#endif

                //
                // Now overwrite the modified data !
                //
                
                while( (Address < (SourceBin->FileOffset + SourceBin->Size)) && (Start < End) ) {
                    RtlCopyMemory((PUCHAR)NewBin + (Address - SourceBin->FileOffset),Source, HSECTOR_SIZE);
                    
                    // 
                    // skip to the next sector
                    //
                    Start++;
                    Source += HSECTOR_SIZE;
                    Address += HSECTOR_SIZE;
                }

                //
                // first sector of the new bin is always restaured from the log file!
                //
                ASSERT(NewBin->FileOffset == SourceBin->FileOffset);
                ASSERT(NewBin->Size == SourceBin->Size);

            } else {
                //
                // Normal case: sector recovery somewhere in the middle of the bin
                //

                
                //
                // Do not delist as we didn't enlisted  (when hive needs recovery)
                //
                //HvpDelistBinFreeCells(Hive,Bin,Stable,TailDisplay);
                
                //
                // Offset should fall within bin memory layout
                //
                ASSERT( SectorOffsetInBin < Bin->Size );
            
                BlockOffsetInBin = (ULONG)((PUCHAR)Me->BlockAddress - (PUCHAR)Bin);
                SectorOffsetInBlock = SectorOffsetInBin - BlockOffsetInBin;
            
                //
                // sanity check; address should  be the same relative to eigther begining of the bin or begining of the block
                //
                ASSERT(((PUCHAR)Me->BlockAddress + SectorOffsetInBlock) == ((PUCHAR)Bin + SectorOffsetInBin));

                SectorImage = (PUCHAR)((PUCHAR)Me->BlockAddress + SectorOffsetInBlock);

                //
                // both source and destination should be valid at this point
                //
                ASSERT( SectorImage < ((PUCHAR)Bin + Bin->Size) );
                ASSERT( Source < (MemoryBlock + Length) );

                NumberOfSectors = 0;
                while( ( (SectorImage + (NumberOfSectors * HSECTOR_SIZE)) < (PUCHAR)((PUCHAR)Bin + Bin->Size) ) &&
                        ( (Start + NumberOfSectors ) < End )    ) {
                    //
                    // we are still inside the same bin;
                    // deal with all sectors inside the same bin at once
                    //
                    NumberOfSectors++;
                }

                //
                // finally, copy the memory
                //
                RtlCopyMemory(SectorImage,Source, NumberOfSectors * HSECTOR_SIZE);

                NewBin = Bin;

                //
                // skip to the next sector
                //
                Start += NumberOfSectors;
                Source += NumberOfSectors * HSECTOR_SIZE;

            }

            //
            // rebuild the map anyway
            //
            if( !NT_SUCCESS(HvpEnlistBinInMap(Hive, Length, NewBin, NewBin->FileOffset, TailDisplay)) ) {
                goto ErrorExit;
            }
        }
    
        //
        // get rid of the temporary pool
        //
        ExFreePool(MemoryBlock);
    }

    //
    // now, after we have successfully recovered, enlist all free cells
    // and restore the hive to it's original state
    //
    Hive->ReadOnly = ReadOnly;

    if( ReadOnly == FALSE ) {
        //
        // no point going through this loop if the hive is read-only
        //
        Address = 0;
        while( Address < Hive->BaseBlock->Length ) {
            Me = HvpGetCellMap(Hive, Address);
            VALIDATE_CELL_MAP(__LINE__,Me,Hive,Address);
            Bin = (PHBIN)(Me->BinAddress & HMAP_BASE);

            ASSERT( Bin->FileOffset == Address );
            ASSERT( Bin->Signature == HBIN_SIGNATURE );

            //
            // add free cells in the bin to the appropriate free lists
            //
            if ( ! HvpEnlistFreeCells(Hive, Bin, Address,TailDisplay)) {
                HvCheckHiveDebug.Hive = Hive;
                HvCheckHiveDebug.Status = 0xA004;
                HvCheckHiveDebug.Space = Bin->Size;
                HvCheckHiveDebug.MapPoint = Address;
                HvCheckHiveDebug.BinPoint = Bin;
                goto ErrorExit;
            }
        
            Address += Bin->Size;
        }
    }

    //
    // put correct dirty vector in Hive so that recovered data
    // can be correctly flushed
    //
    RtlInitializeBitMap(&(Hive->DirtyVector), Vector, VectorSize);
    Hive->DirtyCount = RtlNumberOfSetBits(&Hive->DirtyVector);
    Hive->DirtyAlloc = VectorSize * 8;
    HvMarkDirty(Hive, 0, sizeof(HBIN));  // force header of 1st bin dirty
    return HiveSuccess;

ErrorExit:
    //
    // free the dirty vector and return failure
    //
    (Hive->Free)(Vector, VectorSize);
    return Fail;
}

