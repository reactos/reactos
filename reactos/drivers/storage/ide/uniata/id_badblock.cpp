/*++

Copyright (C) 2006 VorontSOFT

Module Name:
    id_badblock.cpp

Abstract:
    This is the artificial badblock simulation part of the
    miniport driver for ATA/ATAPI IDE controllers with Busmaster DMA support

Author:
    Nikolai Vorontsov (NickViz)

Environment:
    kernel mode only

Notes:

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Revision History:
    2006/08/03 Initial implementation.
    2006/08/06 Added registry work.
    2007/03/27 Added device serial to registry value name instead of LUN.

--*/

#include "stdafx.h"

//#define MAX_BADBLOCKS_ITEMS 512



//SBadBlockRange    arrBadBlocks[MAX_BADBLOCKS_ITEMS];
//ULONG            nBadBlocks = 0;

LIST_ENTRY BBList;
BOOLEAN BBListInited = FALSE;

// RtlQueryRegistryValues callback function
static NTSTATUS __stdcall
BadBlockQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    PSBadBlockListItem cur;
    PLIST_ENTRY      link;
    ULONG i;
    // The ValueType should be REG_SZ
    // The ValueData is UNICODE string of the following format:
    //  "badblocks_start_from_lba  badblocks_end_at_lba"

    KdPrint(( "BadBlockQueryRoutine: S/N:%S\n type %#x, len %#x\n", ValueName, ValueType, ValueLength));

    if(!BBListInited)
        return STATUS_SUCCESS;

    if((ValueType == REG_BINARY)  &&            // STRING
        ValueLength &&            // At least "0 0 0"
        !(ValueLength % sizeof(SBadBlockRange)))    // There is free space for the record
    {
        cur = NULL;
        link = BBList.Flink;
        while(link != &BBList) {
            cur = CONTAINING_RECORD( link, SBadBlockListItem, List);
            link = link->Flink;
            if(!wcscmp(cur->SerNumStr, ValueName)) {
                KdPrint(( "already loaded\n"));
                if(cur->LunExt) {
                    cur->LunExt->nBadBlocks   = 0;
                    cur->LunExt->arrBadBlocks = NULL;
                    cur->LunExt->bbListDescr  = NULL;
                    cur->LunExt = NULL;
                }
                break;
            }
        }

        if(!cur) {
            cur = (PSBadBlockListItem)ExAllocatePool(NonPagedPool, sizeof(SBadBlockListItem));
            if(!cur)
                return STATUS_SUCCESS;
        } else {
            if(cur->arrBadBlocks) {
                ExFreePool(cur->arrBadBlocks);
                cur->arrBadBlocks = NULL;
            }
        }
        cur->arrBadBlocks = (SBadBlockRange*)ExAllocatePool(NonPagedPool, ValueLength);
        if(!cur->arrBadBlocks) {
            ExFreePool(cur);
            return STATUS_SUCCESS;
        }
        RtlCopyMemory(cur->arrBadBlocks, ValueData, ValueLength);
        wcsncpy(cur->SerNumStr, ValueName, 127);
        cur->SerNumStr[127] = 0;
        cur->nBadBlocks = ValueLength/sizeof(SBadBlockRange);
        cur->LunExt = NULL;
        InitializeListHead(&cur->List);
        InsertTailList(&BBList, &(cur->List));
        for(i=0; i<cur->nBadBlocks; i++) {
            KdPrint(( "BB: %I64x - %I64x\n", cur->arrBadBlocks[i].m_lbaStart, cur->arrBadBlocks[i].m_lbaEnd-1));
        }
    }
    return STATUS_SUCCESS;
} // end BadBlockQueryRoutine()


void
InitBadBlocks(
    IN PHW_LU_EXTENSION LunExt
    )
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];    // Main record and zero filled end of array marker
    WCHAR DevSerial[128];
    ULONG Length;
    PLIST_ENTRY      link;
    PSBadBlockListItem   cur;
    // Read from the registry necessary badblock pairs and fill in arrBadBlocks array
    // HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\UniATA\Parameters\BadBlocks

    if(!LunExt) {
        // init
        KdPrint(( "InitBadBlocks general\n"));
        if(!BBListInited) {
            InitializeListHead(&BBList);
            BBListInited = TRUE;
        }

        QueryTable[0].QueryRoutine    = BadBlockQueryRoutine;
        QueryTable[0].Flags           = RTL_QUERY_REGISTRY_REQUIRED;
        QueryTable[0].Name            = NULL;   // If Name is NULL, the QueryRoutine function 
                                                //  specified for this table entry is called 
                                                //  for all values associated with the current 
                                                //  registry key. 
        QueryTable[0].EntryContext    = NULL;
        QueryTable[0].DefaultType     = REG_NONE;
        QueryTable[0].DefaultData     = 0;
        QueryTable[0].DefaultLength   = 0;

        RtlZeroMemory(QueryTable + 1, sizeof(RTL_QUERY_REGISTRY_TABLE));    // EOF

        NTSTATUS status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                                L"UniATA\\Parameters\\BadBlocks",
                                                QueryTable, 0, 0);

        KdPrint(( "InitBadBlocks returned: %#x\n", status));
    } else {

        KdPrint(( "InitBadBlocks local\n"));
        Length = EncodeVendorStr(DevSerial, (PUCHAR)LunExt->IdentifyData.ModelNumber, sizeof(LunExt->IdentifyData.ModelNumber));
        DevSerial[Length] = '-';
        Length++;
        Length += EncodeVendorStr(DevSerial+Length, LunExt->IdentifyData.SerialNumber, sizeof(LunExt->IdentifyData.SerialNumber));

        KdPrint(( "LunExt %#x\n", LunExt));
        KdPrint(( "S/N:%S\n", DevSerial));

        LunExt->nBadBlocks = 0;
        LunExt->arrBadBlocks = NULL;

        link = BBList.Flink;
        while(link != &BBList) {
            cur = CONTAINING_RECORD( link, SBadBlockListItem, List);
            link = link->Flink;
            if(cur->LunExt == LunExt) {
                KdPrint(( "  deassociate BB list (by LunExt)\n"));
                cur->LunExt->nBadBlocks   = 0;
                cur->LunExt->arrBadBlocks = NULL;
                cur->LunExt->bbListDescr  = NULL;
                cur->LunExt = NULL;
            } else
            if(!wcscmp(cur->SerNumStr, DevSerial)) {
                KdPrint(( "  deassociate BB list (by Serial)\n"));
                if(cur->LunExt) {
                    cur->LunExt->nBadBlocks   = 0;
                    cur->LunExt->arrBadBlocks = NULL;
                    cur->LunExt->bbListDescr  = NULL;
                    cur->LunExt = NULL;
                }
            }
        }

        if(!(LunExt->DeviceFlags & DFLAGS_ATAPI_DEVICE)) {

            link = BBList.Flink;
            while(link != &BBList) {
                cur = CONTAINING_RECORD( link, SBadBlockListItem, List);
                link = link->Flink;
                if(!wcscmp(cur->SerNumStr, DevSerial)) {
                    KdPrint(( "found BB:List\n"));
                    cur->LunExt = LunExt;
                    LunExt->arrBadBlocks = cur->arrBadBlocks;
                    LunExt->nBadBlocks   = cur->nBadBlocks;
                    LunExt->bbListDescr  = cur;
                    return;
                }
            }
        }
    }
    return;
} // end InitBadBlocks()


void
ForgetBadBlocks(
    IN PHW_LU_EXTENSION LunExt
    )
{
    if(LunExt->bbListDescr) {
        LunExt->bbListDescr->LunExt = NULL;
        LunExt->nBadBlocks   = 0;
        LunExt->arrBadBlocks = NULL;
        LunExt->bbListDescr  = NULL;
    }
} // end ForgetBadBlocks()

bool
CheckIfBadBlock(
    IN PHW_LU_EXTENSION LunExt,
//    IN UCHAR command,
    IN ULONGLONG lba,
    IN ULONG count
    )
{
    if (LunExt->nBadBlocks == 0)
        return false;
/*
    // this is checked by caller
    if(!(AtaCommandFlags[command] & ATA_CMD_FLAG_LBAsupp)) {
        return false;
*/
    ULONG nBadBlocks = LunExt->nBadBlocks;
    SBadBlockRange*  arrBadBlocks = LunExt->arrBadBlocks;

    // back transform for possibly CHS'ed LBA
    lba = UniAtaCalculateLBARegsBack(LunExt, lba);

    for (ULONG i = 0; i < nBadBlocks; i++)
    {
        if (lba + count > arrBadBlocks->m_lbaStart  &&  
                    lba < arrBadBlocks->m_lbaEnd) {
            KdPrint(( "listed BB @ %I64x\n", lba));
            return true;
        }
        arrBadBlocks++;
    }

    return false;

} // end CheckIfBadBlock()
