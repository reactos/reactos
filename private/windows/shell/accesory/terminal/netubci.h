/*****************************************************************************/
/* UBNETCI: definitions for Ungermann-Bass Command interpreter interface     */
/*****************************************************************************/

/*****************************************************************************/
/* Constant Definitions                                                      */
/*****************************************************************************/

#define INT_UBNETCI           0x6B

#define UBC_CALL_WRITE        0x00           /* int 6B calls... */
#define UBC_CALL_READ         0x01
#define UBC_CALL_CONTROL      0x06
#define UBC_CALL_STATUS       0x07
#define UBC_CALL_READBREAK    0x08

#define UBC_PORT_COM1         0x00
#define UBC_PORT_COM2         0x01

#define UBC_CNTRL_CMD_BREAK   0x02
#define UBC_CNTRL_CMD_DISCON  0x04
#define UBC_CNTRL_CMD_HOLD    0x06
#define UBC_CNTRL_CMD_ENABLEXON  0x08           /* slc swat */
#define UBC_CNTRL_CMD_DISABLEXON 0x10

#define UBC_STAT_IDLE         0xFF
#define UBC_STAT_CI           0x00
#define UBC_STAT_NET          0x01


/*****************************************************************************/
/* Forward Procedure Definitions                                             */
/*****************************************************************************/

VOID UBC_exitSerial();
VOID UBC_resetSerial(recTrmParams *, BOOL);  /* mbbx 2.01.141 */

BOOL UBC_mdmConnect();
VOID UBC_modemReset();
VOID UBC_modemSendBreak(INT);
INT NEAR UBC_ReadComm(LPSTR, INT);
VOID UBC_modemBytes();
INT NEAR UBC_WriteComm(LPSTR, INT);
BOOL UBC_modemWrite(LPSTR, INT);
/* WORD NEAR UBC_CallNetCI(BYTE, WORD, LPBYTE, WORD);   tge gold 006 */
WORD UBC_CallNetCI(BYTE, WORD, LPBYTE, WORD);
WORD UBC_LOW_CallNetCI(WORD, WORD, WORD, WORD);
