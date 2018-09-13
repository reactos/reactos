;----------------------------------------------------------------------------
;  MRCI.INC - Microsoft Real-Time Compression Interface definitions
;
;  Microsoft Confidential
;  Copyright (C) Microsoft Corporation 1992
;  All Rights Reserved.
;
;  History:
;    18-Jul-92 bens	 Initial version.
;    25-Jul-92 billkru	 Adapted for use by MS-Flash & ROMDOS environment.
;----------------------------------------------------------------------------
intMRCIROM	   equ	   1Ah	   ; ROM MRCI interrupt number
mrciDETECTROM	   equ	   0B001h  ; ROM intMRCI AX for detecting MRCI server
intMRCI 	 equ	 2Fh	   ; MRCI interrupt number
mrciDETECT	 equ	 4a12h	   ; intMRCI AX for detecting MRCI server

mrciBackPatch	 equ	 4a13h

;----------------------------------------------------------------------------
;  sigOLD_CX, sigOLD_DX, sigNEW_CX, sigNEW_DX - MRCI Server detection
;
;  These values are used to verify that the response from issuing
;  intMRCI is coming from an MRCI server, and not some other piece
;  of code.
;
;  The *old* values are passed on the mrciQUERY call, and the server
;  must change CX/DX to the *new* values, so that the caller can
;  trust that the MRCI server was responding, and not some other
;  interrupt hooker.
;
;  The Server uses this code sequence to transform CX/DX:
;
;	    ;------------- entry: cx='ab'  dx='cd'
;	    xchg    ch,cl	; cx='ba'  dx='cd'
;	    xchg    dh,dl	; cx='ba'  dx='dc'
;	    xchg    dx,cx	; cx='dc'  dx='ba'
;----------------------------------------------------------------------------
sigOLD_CX   equ 'MR'
sigOLD_DX   equ 'CI'

sigNEW_CX   equ 'IC'
sigNEW_DX   equ 'RM'

;----------------------------------------------------------------------------
;    mcXXXX - flag values passed to MRCI operations
;
;	MRCCompress and MRCDecompress take a flag to indicate whether the
;	client is a *system* component (and hence may call with InDOS
;	set), or an *application*.
;
;	If a system client calls MRCCompress/MRCDecompress with mcSYSTEM,
;	then it is the clients responsibility to ensure that one of the
;	following is true:
;	    1)	InDOS is set
;	    2)	The call is made at interrupt time.
;
;	FAILING TO FOLLOW THE ABOVE RULE WILL LIKELY RESULT IN A SYSTEM
;	HANG AND LOSS OF USER DATA.
;----------------------------------------------------------------------------
mcAPPLICATION		equ	0	; Client is an application
mcSYSTEM		equ	1	; Client is a file system driver

;----------------------------------------------------------------------------
;  MRCI Server Busy flag values
;----------------------------------------------------------------------------
MRCI_SERVER_IDLE	equ	0

;----------------------------------------------------------------------------
;    MRCINFO - MRC Information data structure
;
;	A pointer to an MRCINFO structure is returned from MRCQuery, and sent
;	on MRCNotifyLoad.  This structure contains information on the MRCI
;	server and its capabilities.
;----------------------------------------------------------------------------
MRCINFO 		struc
mi_lVendor		dd	?	; A 4-byte vendor ID.
					; Microsoft's vendor ID is "MSFT".

mi_wVendorVersion	dw	?	; Version number of the MRC server.
					; High byte is major number, low byte is minor.
					; EXAMPLES: v3.20 = 0314h, v10.01 = 0A01h

mi_wMRCIVersion 	dw	?	; Version number of the MRCI supported by
					; this server.

mi_pfnOperate		dd	?	; Far pointer of the server compression entry
					; point.

mi_flCapability 	dw	?	; Bit field of server capabilities
					; See micapXXX for bit definitions

mi_flHWAssist		dw	?	; Bit field of hardware assisted
					; server capabilities.	One-to-one
					; correspondance with mi_flCapability
					; bits.  A bit set in this field
					; indicates the corresponding
					; capability is hardware assisted.

mi_cbMax		dw	?	; Maximum number of bytes that the compression
					; services provider can compress or decompress.
					; Requests to compress or decompress buffers in
					; excess of this length will fail.
					; All MRCI servers must support at least
					; 8192 byte (8Kb) blocks.
MRCINFO 		ends

;----------------------------------------------------------------------------
;    micapXXXXX - bit flags for MRCINFO.mi_flCapabilities
;
;	These define both the capabilities of the Server, and also double
;	as *operation* codes passed to the mi_pfnOperate entry point in
;	the server.
;			111111
;			5432109876543210
;----------------------------------------------------------------------------
micapNONE	equ	0000000000000000b	; No capabilities
micapSTANDARD	equ	0000000000000001b	; Standard compress
micapDECOMPRESS equ	0000000000000010b	; Standard compress
micapUPDATE	equ	0000000000000100b	; Update compress
micapSUPER	equ	0000000000001000b	; SuperCompress(tm)
micapSUPERUPDATE equ	0000000000010000b	; Update SuperCompress(tm)
micapINCDECOMP	equ	0000000000100000b	; Incremental Decompress
						; Remaining bits (6..14) are RESERVED and must be 0
micapREADONLY	equ	1000000000000000b	; MRCINFO structure is read-only
micapDEINSTALL	equ	1111111111111111b	; Server deinstall service

;----------------------------------------------------------------------------
;  MRCREQUEST - MRC compress/decompress Request packet
;
;  This structure is used to pass parameters to the server for
;  compress/decompress operations.
;
;  General Notes
;  -------------
;  1) <mr_pbSrc,mr_cbSrc> and <mr_pbDst,mr_cbDst> MUST NOT OVERLAP!
;
;  2) The safest practice is for mr_cbSrc and mr_cbDst to be identical
;     (except for decompress where mr_cbSrc is ignored and mr_cbDst is
;      the count of bytes to uncompress).
;
;  Details on Structure Members
;  ----------------------------
;  mr_pbSrc
;    This points to the *source* buffer.
;
;    On a *compress* operation, the contents of this buffer are
;    *uncompressed* data.
;
;    On an *update* operation, the contents of this buffer starting
;    at offset mr_offUpdate is valid, uncompressed data.  The contents
;    from the start of the buffer to the byte just prior to
;    mr_OffUpdate are ignored.
;
;    On a *decompress* operation, the contents of this buffer are
;    *compressed* data.
;
;  mr_cbSrc
;    This is the size of the *source* buffer.
;
;    This is the amount of data to be compressed or decompressed.
;    NOTE: For a decompress operation, this value may be ignored
;	   by many implentations, since the MRC Format encoding has
;	   a special code to indicate the end of the compressed data.
;
;  mr_offUpdate
;    NOTE: This is used for Update(Super)Compress only.
;
;    On INPUT, this is the offset (same segment as mr_pbSrc) in the
;    *source* buffer where the changed uncompressed data starts.
;
;    On OUTPUT, this is the offset (same segment as mr_pbDst) in the
;    *destination* buffer where the changed compressed data starts.
;
;  mr_pbDst
;    This points to the *destination* buffer.
;
;    On a *compress* operation, this buffer receives the *compressed*
;    result of the operation.
;
;    On an *update* operation, the contents of this buffer MUST be
;    the compressed representation of the uncompressed data in the
;    *source* buffer.  After the operation is completed, this buffer
;    receives the *compressed* result of the operation.
;
;    On a *decompress* operation, this buffer receives the
;    *uncompressed* result of the operation.
;
;  mr_cbDst
;    On a *compress* operation and on INPUT, this is the size of
;    the *destination* buffer.	If, during the operation, the
;    Server would overflow this buffer length, then the operation
;    fails and the server returns the error MRCI_ERROR_BUFFER_OVERFLOW.
;
;    On a *decompress* operation and on INPUT, this is what the
;    *decompressed* size should be.  If, during the operation, the
;    Server would overflow this length, then the operation fails
;    and the server returns the error MRCI_ERROR_BUFFER_OVERFLOW.
;
;    On OUTPUT, the Server updates this field with the actual size
;    of the resulting compressed/uncompressed data.
;
;  mr_cbChunk
;    This is information that the Server compress routines can use to
;    "early out" of the compression as early as possible.
;
;    Valid values are 1 (client is interested in savings as small as
;    1 byte) to 32767.	DblSpace passes 512, and MS-Flash passes 1.
;
;    This field is most easily explained by giving an example:
;
;    Example:
;      DblSpace does space allocation in chunks of 512 bytes (the
;      common sector size on a disk).
;
;      The compression server can use this information for two
;      optimizations:
;
;      1) If the Server cannot compress the uncompressed data
;	  enough to save at least 512 bytes, then the data is
;	  *incompressible* as far as DblSpace is concerned, even
;	  if it could be compressed to save fewer than 512 bytes.
;
;      2) While compressing, if the Server gets to a point where
;	  the remaining uncompressed data is of such a length that
;	  it can be encoded simply (without table lookups, etc.)
;	  and not cross a 512 byte boundary, then the Server can
;	  do the simple encoding.
;
;      It is likely that these optimizations will be hard to perform
;      quickly in software, but it is possible that hardware can do
;      these optimizations without any performance loss.
;----------------------------------------------------------------------------
MRCREQUEST	struc
mr_pbSrc	dd	?	; Pointer to source buffer
mr_cbSrc	dw	?	; Size of source buffer, in bytes
mr_offUpdate	dw	?	; For UpdateCompress only: offset of (source) destination
				; buffer where changed (un)compressed data starts.
mr_pbDst	dd	?	; Pointer to destination buffer
mr_cbDst	dw	?	; Size of destination buffer, in bytes
mr_cbChunk	dw	?	; Client compressed data storage chunk size (see above!)
mr_dwIncDecomp	dd	?	; Incremental Decompression state
MRCREQUEST	ends


;----------------------------------------------------------------------------
;  MRCI_ERROR_XXX definitions
;
;  Error codes returned from a request
;----------------------------------------------------------------------------
MRCI_ERROR_NONE 	    equ     0	; No error
MRCI_ERROR_NOT_SUPPORTED    equ     1	; Unsupported operation requested
MRCI_ERROR_BUSY 	    equ     2	; Server is busy
MRCI_ERROR_BUFFER_OVERFLOW  equ     3	; Destination buffer too small
MRCI_ERROR_NOT_COMPRESSIBLE equ     4	; Data could not be compressed
MRCI_ERROR_BAD_MRC_FORMAT   equ     5	; Compressed data format is bad


;---------------------------------------------------------------------------
; DblSpace MRCI Server constants
;---------------------------------------------------------------------------


MRCIVERSION	equ	0100h		; MRCI version supported by this server
MYVERSION	equ	0100h		; Version of this server/driver

MYVENDOR	equ	5446534Dh	; "MSFT" (reversed for byte ordering)
MYVENDORhi	equ	5446h		; High word of MYVENDOR
MYVENDORlo	equ	534Dh		; Low word of MYVENDOR

; Max buffer size set to 32k to avoid problems with compression code and
; buffer offsets near 0E000h.  See comments in rcomp.asm for details.

MYCBMAX 	equ	(32*1024)	; Maximum compression buffer size

;  This server's capabilities.

micapMINE	=	micapSTANDARD or micapDECOMPRESS or micapINCDECOMP


MYHWASSIST	equ	0		;we have no hardware assist
