/**	SVC Defines
 *
 *	Revision history:
 *
 *	sudeepb 15-May-1991 Created
 *
 *	williamh 25-Sept-1992 Added UMB support BOPs
 */


/* XMSSVC - XMS SVC calls.
 *
 *	 This macro is used by himem.sys
 *
 */

/* ASM
include bop.inc

xmssvc	macro	func
	BOP	BOP_XMS
	db	func
	endm
*/

#define XMS_A20 			0x00
#define XMS_MOVEBLOCK			0x01
#define XMS_ALLOCBLOCK			0x02
#define XMS_FREEBLOCK			0x03
#define XMS_SYSPAGESIZE			0x04
#define XMS_EXTMEM			0x05
#define XMS_INITUMB			0x06
#define XMS_REQUESTUMB			0x07
#define XMS_RELEASEUMB			0x08
#define XMS_NOTIFYHOOKI15               0x09
#define XMS_QUERYEXTMEM                 0x0a
#define XMS_REALLOCBLOCK                0x0b
#define XMS_LASTSVC                     0x0c

extern BOOL XMSInit (int argc, char *argv[]);
