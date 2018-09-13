;	mdequ.inc -- MagicDrv general equate file
;/*
; *			 Microsoft Confidential
; *			 Copyright (C) Microsoft Corporation 1992
; *			 All Rights Reserved.
; */

;	History:
;
;	    20-Jul-1992 chuckst 	initial version
;

;CODE_CSUM_SNAP equ 0ffffh	; defined: save image of code on csum err

;	Now that CSUM is the default, the build won't work without
;	POPUP being true.  However, I'm leaving the POPUP conditionals
;	in to help identify that code, just in case we ever want to
;	remove it and let CSUM faults simply hang.

POPUP	=	0ffffh

	ifdef DEBUG
POPUP	=	0ffffh		; POPUPs mandatory for DEBUG build
	endif


; Define the following variable to generate the standalone
; version of DBLSPACE.BIN, which loads as a device driver under
; MS-DOS 5.0 or greater.
;
;STANDALONE	 equ	 0ffffh

switch_n	equ	0001h
switch_f	equ	0002h

fatal_error_0	equ	0	; re-entered at strat
fatal_error_1	equ	1	; irpt without 1 strat
fatal_error_2	equ	2	; 2 irpts per strat
fatal_error_3	equ	3	; get_sqztor, number too low
fatal_error_4	equ	4	; put_sqztor, number too low
fatal_error_5	equ	5	; get_sqztor, number too high
fatal_error_6	equ	6	; put_sqztor, number too high
fatal_error_7	equ	7	; DOS system I/O too high
fatal_error_8	equ	8	; DOS system I/O too low
fatal_error_9	equ	9	; HEAP I/O too high
fatal_error_10	equ	10	; HEAP I/O too low
fatal_error_11	equ	11	; BITFAT I/O too high
fatal_error_12	equ	12	; BITFAT I/O too low
fatal_error_13	equ	13	; MDFAT I/O too high
fatal_error_14	equ	14	; MDFAT I/O too low
fatal_error_15	equ	15	; Bad signature check on CVF fragment list
fatal_error_16	equ	16	; Invalid int2f subfn (for forcing popup)
fatal_error_17	equ	17	; free to free sqztor
fatal_error_18	equ	18	; unfree to used sqztor
fatal_error_19	equ	19	; bitfat memory corrupt
fatal_error_20	equ	20	; mdfat memory corrupt
fatal_error_21	equ	21	; file fragment list corrupt
fatal_error_22	equ	22	; code block corrupt
fatal_error_23	equ	23	; write to invalid BitFAT page (0f0h)
fatal_error_24	equ	24	; DBLSPACE.BIN stack overrun

CSUM_CODE	equ	1
CSUM_FFLIST	equ	2
CSUM_BFAT_USE	equ	4
CSUM_BFAT_WRITE equ	8
CSUM_BFAT	equ	CSUM_BFAT_USE+CSUM_BFAT_WRITE
CSUM_MDFAT_USE	equ	16
CSUM_MDFAT_WRITE equ	32
CSUM_MDFAT	equ	CSUM_MDFAT_USE+CSUM_MDFAT_WRITE
CSUM_STACK_CHK  equ     64      ; Set => check for stack overrun
CSUM_ALL        equ     CSUM_CODE+CSUM_FFLIST+CSUM_BFAT+CSUM_MDFAT+CSUM_STACK_CHK


CSUM_XOR	equ	4fd0h	; this value is XOR'd with all internal
;				; data structure words before the checksum
;				; is performed.  This is because of the
;				; fact that 0ffffh is minus 0 in one's
;				; complement, and thus the checksum would
;				; not detect 0s changing to 0ffffhs, and
;				; vice versa.  By doing this XOR, we map
;				; the two 'identical' values to 4fd0h and
;				; 0b02fh, which are much less likely to
;				; occur.  Only a trashing bug which did
;				; a properly aligned NOT function on one
;				; of these two words would not be detected.
CODE_CSUM_INTERVAL equ 10	; checksum code one out of this many
;				; times driver is entered.

;;
;; Equates
;;

multMagicdrv	equ	4a11h	; Int2f multiplex number

DRVMAP_CPR_BIT	equ	80h	; bit indicating mounted CVF in drv_map array

cr		=	13
lf		=	10
tab		=	9
ctrlz		=	1ah

CVF_SEQ_SWAP	equ	0	; Sequence number which implies SWAP
CVF_SEQ_MAX	equ	255	; Maximum allowable sequence number

MAX_ACTIVATES	equ	24	; 26 drive letters, less two floppies

BIG_CLUSTER	equ	8192		;Size of BIG cluster (8K)
SMALL_CLUSTER	equ	4096		;Size of SMALL cluster (4K)

BPTR	equ	BYTE PTR

MAX_FULL		equ	32  ;;64;Minimum of free sectors b4 we declare this disk as full
MAX_WARN		equ	256	;Minimum free sectors to generate an 'almost full' warning

;; temp_buf_list values
ORIGINAL_DATA		equ	1	;This sector has an original data
NEW_DATA		equ	2	;This sector contains new data

MAX_DEMO_HIWORD_SIZE	equ	48	;Maximum high word for demo file
MAX_SECTOR_SIZE 	equ	2048	;Maximum host sector size (2K)

IO_PART_SPACE		equ	2048	;Page size for sqztor table
PART_CLUSTER_SIZE_POW2	equ	9	;How many clusters per one page (DWORD per cluster)

BIT_FAT_SPACE		equ	2048	;Page size of sec_list table (free space)
SECTORS_PER_BIT_FAT_POW2 equ	14	;In power 2 if sectors
SECTORS_PER_BIT_FAT	equ	16384	;Number of bits (sectors) in one page

DBL_SIGN		equ	0	;Where the 'DBL' signature can be located
VOL_STAMP		equ	6	;Where unique vol stamp is found
RETRACT_SIGNATURE	equ	1	; signature is last full sector, ie:
;					;  end of file (rounded down to sector)
;					;  minus 1

RH			equ	ds:[bx] ;Addressability to Request Header structure

MAX_DISK_UNITS		equ	15	;Maximumm of extended disk we can handle

cMINFILEFRAGMENTS	equ	50	;Minimum file fragment heap size
cDEFFILEFRAGMENTS	equ	700	;Default  "      "	 "   "
cMAXFILEFRAGMENTS	equ	10000	;Maximum  "      "	 "   "

; cluster_to_Sqztor equates. See globals definition
NO_FLAGS_MASK		equ	001FH	;Just the entry, without flags
RESERVED1_MASK		equ	0020H	;Reserved bit just after 21-bit sector#
PHYSICAL_SIZE_MASK	equ	03C0H	;Only the physical size bits
PHYSICAL_SIZE_OFFSET	equ	6	;THe physical size bits offset
PLAINTEXT_MASK		equ	3C00H	;Only the plaintext size bits
PLAINTEXT_OFFSET	equ	10	;Plaintext size bits offset
UNCODED_CLUSTER_MASK	equ	4000H	;Uncompress cluster bit
USED_CLUSTER_MASK	equ	8000H	;Used cluster bit

PARA_SIZE_POW2		equ	4	;Number of bytes in one 8088 paragraph
STACK_SIZE		equ	768	;Size of our stack

; Device driver equates
READ_DISK_CMD		equ	4
WRITE_DISK_CMD		equ	8
WRITE_VERIFY_CMD	equ	9

STAT_WRITP		equ	8000H		;Write protect violation
STAT_DFULL		equ	8027h		;Disk full error
STAT_NRDY		equ	8002H		;Device not ready
STAT_NOCOM		equ	8003H		;Unkown command
STAT_CRC		equ	8004H		;CRC error
STAT_SNF		equ	8008H		;sector not found error
STAT_SEEK		equ	8006H		;Seek error
STAT_WRITE		equ	800AH		;Write FAULT
STAT_READ		equ	800BH		;Read fault
STAT_GEN		equ	800CH		;General failore

DOS	   EQU	21H		;DOS request INT
DOS_PCHR   EQU	02H		;print character function

;Constants for compression/Decompression alg.
MAX_BITS	equ	12		;Maximum bits (2048 entries)
TABLESIZE	equ	2048		;Hash table size

SPECIAL_EOS	equ	4415		;End of Sector code


;Generic IOCTL return codes
NO_ERROR	    equ 0		;No error
LETTER_BOUNDRY_ERROR equ 1		;Letter out of bounds
UNIT_USED_ERROR     equ 2		;Unit already used
NO_FREE_SLOT_ERROR  equ 3		;No reserved slot available
STAMP_ERROR	    equ 4		;Stamp error on extended disk


;Cluster signature
;Compressed clusters has the signature:
; 44 53 00 00	-  Cluster compressed real time
; 44 53 00 01	-  Cluster SuperCompressed offline
; 44 53 00 02	-  Cluster XCompressed real time
;
; NOTE: If you load these signature as words, they look like:
;
;   low  word = 5344h
;   high word = 0000h or 0100h

; XCMP defines a different REAL_TIME_COMPRESSED signature so a XCMP and
; non XCMP driver can be used on the same CVF and not get confused trying
; to UpdateCompress a XCMP block (XCMP does not support UpdateCompress).

SIG_SIZE		equ	4	; 4 byte signature
STANDARD_COMPRESSED	equ  0000h	; High word of stamp
SUPER_COMPRESSED	equ  0100h	; High word of stamp
X_COMPRESSED		equ  0200h	; High word of stamp
REAL_TIME_COMPRESSED	equ  X_COMPRESSED ; Stamp used for real-time compress

;;
;; We always keep at least 64k of extra sqztors around so a
;; file overwrite won't expand to more sqztors that are available.
;; Of course this is a raw heuristic since these should also
;; be contingous.  For more info, see dospatch.asm and
;; low_dlb.asm referenece to FUDGESTOR
FUDGESTOR	equ	128


; eXperimental compression code uses a lookup table to find past
; occurances of individual characters.	The table is currently
; 256 rows (1 for each possible byte value) X N (tracks N occurances
; of each character).


cltROWS EQU	256		; lookup table 256 rows x
cltCOLS EQU	8		;   N columns


	ifdef	POPUP

PUP_FILE_SIZE	equ	20*1024
MAX_PUP_FRAGLIST equ	6*(1+PUP_FILE_SIZE/1024) ; support >=1K clusters
PUP_QUEUE_SIZE	equ	4096

	endif

; mcsNONE indicates there is no saved Media Check status.  This value
; must not be equated to a valid media check result (-1, 0, 1).

mcsNONE 	equ	80h


; cAM_HOOKS is the number of block device drivers that can be hooked
; by the automount code.  Each hook adds approx 30 bytes, so keeping
; this value low is significant.  A MS-DOS 6 system that does not
; load additional block device driver in config.sys uses 1 hook (A:,
; B:, C:, ... are all serviced by one device driver).

cAM_HOOKS	equ	5

para_adjust	equ	-16		; we'll add this to certain vars
;					;  in the automount device headers
;					;  and related variables when we
;					;  access them with low_stub_seg+1,
;					;  to work around a Norton SpeeDrv bug.


STACK_GUARD_VALUE equ	1234h		; unlikely value to use when checking
					;   for stack overrun
