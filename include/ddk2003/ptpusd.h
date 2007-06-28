
const DWORD ESCAPE_PTP_CLEAR_STALLS         = 0x0200; 
const DWORD ESCAPE_PTP_VENDOR_COMMAND       = 0x0100;
const DWORD ESCAPE_PTP_ADD_OBJ_CMD          = 0x0010;
const DWORD ESCAPE_PTP_REM_OBJ_CMD          = 0x0020;
const DWORD ESCAPE_PTP_ADD_OBJ_RESP         = 0x0040;
const DWORD ESCAPE_PTP_REM_OBJ_RESP         = 0x0080;
const DWORD ESCAPE_PTP_ADDREM_PARM1         = 0x0000;
const DWORD ESCAPE_PTP_ADDREM_PARM2         = 0x0001;
const DWORD ESCAPE_PTP_ADDREM_PARM3         = 0x0002;
const DWORD ESCAPE_PTP_ADDREM_PARM4         = 0x0003;
const DWORD ESCAPE_PTP_ADDREM_PARM5         = 0x0004;
const DWORD PTP_MAX_PARAMS                  = 5;
const DWORD SIZEOF_REQUIRED_VENDOR_DATA_IN = sizeof(PTP_VENDOR_DATA_IN) - 1;
const DWORD SIZEOF_REQUIRED_VENDOR_DATA_OUT = sizeof(PTP_VENDOR_DATA_OUT) - 1;
const DWORD PTP_NEXTPHASE_READ_DATA = 3;
const DWORD PTP_NEXTPHASE_WRITE_DATA = 4;
const DWORD PTP_NEXTPHASE_NO_DATA = 5;


#pragma pack(push, Old, 1)

typedef struct _PTP_VENDOR_DATA_IN
{
    WORD OpCode;
    DWORD SessionId;
    DWORD TransactionId;
    DWORD Params[PTP_MAX_PARAMS];
    DWORD NumParams;
    DWORD NextPhase;
    BYTE VendorWriteData[1];
} PTP_VENDOR_DATA_IN, *PPTP_VENDOR_DATA_IN;

typedef struct _PTP_VENDOR_DATA_OUT
{
    WORD ResponseCode;
    DWORD SessionId;
    DWORD TransactionId;
    DWORD Params[PTP_MAX_PARAMS];
    BYTE VendorReadData[1];
} PTP_VENDOR_DATA_OUT, *PPTP_VENDOR_DATA_OUT;

#pragma pack(pop, Old)

