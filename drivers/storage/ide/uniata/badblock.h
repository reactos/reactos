/*++

Copyright (C) 2006 VorontSOFT

Module Name:
    badblock.h

Abstract:
    This is the artificial badblock simulation part of the
	miniport driver for ATA/ATAPI IDE controllers
    with Busmaster DMA support

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

--*/


#ifndef _BADBLOCK_H_INCLUDED_
#define _BADBLOCK_H_INCLUDED_

#pragma pack(push, 4)
typedef struct _SBadBlockRange
{
//	ULONG		m_ldev;
	ULONGLONG	m_lbaStart;
	ULONGLONG	m_lbaEnd;
} SBadBlockRange, *PSBadBlockRange;

typedef struct _SBadBlockListItem {
    LIST_ENTRY       List;
    PHW_LU_EXTENSION LunExt;
    WCHAR            SerNumStr[128];
    SBadBlockRange*  arrBadBlocks;
    ULONG            nBadBlocks;
} SBadBlockListItem, *PSBadBlockListItem;

#pragma pack(pop)

void
NTAPI
InitBadBlocks(
    IN PHW_LU_EXTENSION LunExt
    );

void
NTAPI
ForgetBadBlocks(
    IN PHW_LU_EXTENSION LunExt
    );

bool
NTAPI
CheckIfBadBlock(
    IN PHW_LU_EXTENSION LunExt,
//    IN UCHAR command,
    IN ULONGLONG lba,
    IN ULONG count
    );

#endif // _BADBLOCK_H_INCLUDED_
