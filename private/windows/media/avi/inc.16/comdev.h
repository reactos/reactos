/*****************************************************************************\
*                                                                             *
* comdev.h -
*                                                                             *
* Version 1.0								      *
*                                                                             *
* Copyright (c) 1994, Microsoft Corp.	All rights reserved.		      *
*                                                                             *
\*****************************************************************************/


/*************************************************************************
**
** Miscelaneous definitions.
*/
typedef unsigned short ushort;
typedef unsigned char uchar;

#define NULL    0
#define FALSE   0
#define TRUE    1

#define LPTx    0x80        /* Mask to indicate cid is for LPT device   */  /*081985*/
#define LPTxMask 0x7F       /* Mask to get      cid    for LPT device   */  /*081985*/

#define PIOMAX  3           /* Max number of LPTx devices in high level */  /*081985*/
#define CDEVMAX 10          /* Max number of COMx devices in high level */
#define DEVMAX  13          /* Max number of devices in high level      */  /*081985*/

/*************************************************************************
**
** Extended Functions
**
** SETXOFF      - Causes transmit to behave as if an X-OFF character had
**                been received. Valid only if transmit X-ON/X-OFF specified
**                in the dcb.
** SETXON       - Causes transmit to behave as if an X-ON character had
**                been received. Valid only if transmit X-ON/X-OFF specified
**                in the dcb.
*************************************************************************/
#define SETXOFF         1               /* Set X-Off for output control */
#define SETXON          2               /* Set X-ON for output control  */
#define SETRTS          3               /* Set RTS high                 */
#define CLRRTS          4               /* Set RTS low                  */
#define SETDTR          5               /* Set DTR high                 */
#define CLRDTR          6               /* Set DTR low                  */
#define RESETDEV        7               /* Reset device if possible     */  /*081985*/


/*=========================================================================
;
;       qdb
;       Queue definition block. Passed to setqueue, defines the location and
;       size of the transmit and receive circular queue's used for interrupt
;       transmit and recieve processing.
;
;=========================================================================*/

typedef struct tagQDB
	{
	char _far *QueueRxAddr;								//Pointer to RX Queue, Offset
	unsigned	QueueRxSize;								//Size of RX Queue in bytes
	char _far *QueueTxAddr;								//Pointer to TX Queue, Offset
	unsigned	QueueTxSize;								//Size of TX Queue in bytes
	} QDB;
