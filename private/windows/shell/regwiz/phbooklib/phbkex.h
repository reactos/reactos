#ifndef _PHBKEX
#define _PHBKEX

#define TYPE_SIGNUP_ANY			0x82
#define TYPE_SIGNUP_TOLLFREE	0x83
#define TYPE_SIGNUP_TOLL		0x82
#define TYPE_REGULAR_USAGE		0x42

#define MASK_SIGNUP_ANY			0xB2
#define MASK_SIGNUP_TOLLFREE	0xB3
#define MASK_SIGNUP_TOLL		0xB3
#define MASK_REGULAR_USAGE		0x73

#define cbAreaCode	6			// maximum number of characters in an area code, not including \0
#define cbCity 19				// maximum number of chars in city name, not including \0
#define cbAccessNumber 15		// maximum number of chars in phone number, not including \0
#define cbStateName 31 			// maximum number of chars in state name, not including \0
								// check this against state.pbk delivered by mktg
#define cbBaudRate 6			// maximum number of chars in a baud rate, not including \0
#define cbDataCenter 12			// max length of data center string

typedef struct
{
	DWORD	dwIndex;								// index number
	BYTE	bFlipFactor;							// for auto-pick
	BYTE	fType;									// phone number type
	WORD	wStateID;								// state ID
	DWORD	dwCountryID;							// TAPI country ID
	DWORD	dwAreaCode;								// area code or NO_AREA_CODE if none
	DWORD	dwConnectSpeedMin;						// minimum baud rate
	DWORD	dwConnectSpeedMax;						// maximum baud rate
	char	szCity[cbCity + sizeof('\0')];			// city name
	char	szAccessNumber[cbAccessNumber + sizeof('\0')];	// access number
	char	szDataCenter[cbDataCenter + sizeof('\0')];				// data center access string
	char	szAreaCode[cbAreaCode + sizeof('\0')];					//Keep the actual area code string around.
} ACCESSENTRY, FAR  *PACCESSENTRY; 	// ae

typedef struct tagSUGGESTIONINFO
{
	DWORD	dwCountryID;
	DWORD	wAreaCode;
	DWORD	wExchange;
	WORD	wNumber;
	BYTE	fType;
	BYTE	bMask;
	PACCESSENTRY *rgpAccessEntry;
} SUGGESTINFO,FAR *PSUGGESTINFO;

#endif _PHBKEX
