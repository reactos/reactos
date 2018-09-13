/* xfer_tsc.h -- a file containing the transfe status termination codes
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

/* Transfer Status Codes -- to indicate the final status of an individual
 *	  file transfer.
 * Note: There is a table called transfer_status_msg in transfer.c with an
 *		entry for each of these defines. If new constants are added here,
 *		the table should be expanded as well.
 */
#define 	TSC_OK			   0	// transfer completed normally
#define 	TSC_RMT_CANNED	   1	// remote system sent cancel request
#define 	TSC_USER_CANNED    2	// user hit abort key
#define 	TSC_LOST_CARRIER   3	// carrier lost during transfer
#define 	TSC_ERROR_LIMIT    4	// error limit exceeded
#define 	TSC_NO_RESPONSE    5	// remote system stopped responding
#define 	TSC_OUT_OF_SEQ	   6	// wrong number packet or resp. received
#define 	TSC_BAD_FORMAT	   7	// seemingly valid packet had bad format
#define 	TSC_TOO_MANY	   8	// user specified filename as dest, but
									//	  sender sent more than one file
#define 	TSC_DISK_FULL	   9	// no more room to put file
#define 	TSC_CANT_OPEN	  10	// couldn't find or open file
#define 	TSC_DISK_ERROR	  11	// hard err. or read/write error occurred
#define 	TSC_NO_MEM		  12	// not enough memory to complete transfer
#define 	TSC_FILE_EXISTS   13	// can't receive 'cause no permission to
									//		overwrite
#define 	TSC_COMPLETE	  14	// transfer session is complete
#define 	TSC_CANT_START	  15	// couldn't complete transfer setup, prob.
									//		bad option, disk error, etc.
#define 	TSC_OLDER_FILE	  16	// /N option but rcv'd file older
#define 	TSC_NO_FILETIME   17	// /N option but sender sent no file time
#define 	TSC_WONT_CANCEL   18	// sender failed to cancel transfer when
									//		requested to
#define 	TSC_GEN_FAILURE   19	// generic transfer failure message
#define 	TSC_NO_VSCAN	  20	// Virus scanning can't be loaded
#define 	TSC_VIRUS_DETECT  21	// Virus detected
#define		TSC_USER_SKIP	  22	// User skipped the file
#define 	TSC_REFUSE		  23	// User refused
#define     TSC_FILEINUSE     24    // File is opened and can't be renamed
#define 	TSC_MAX 		  25	// number of TSC codes

