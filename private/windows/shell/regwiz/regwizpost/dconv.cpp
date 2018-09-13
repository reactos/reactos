//
//	File : DCONV.CPP
//  Date : 04/25/97
//	Author : Suresh Krishnan
//  This file has modules responsible for making the  buffer data to be transmitted to the IIS
//
//  MDF 1 : 05/07/97   Modified the Table as per StevBush modifications in the BackEnd
//  MFD 2 : 03/03/97   Added Division Name and UserID for FE screens
//                     Phone number will be prefixed with Area Code
//  MFD3  : 04/29/98   Added additional 4 fileds to be sent to the backend as a part of
//                     Taxanomy Screen changes.
//                     Fields risen from 60 to 64. Fileds added
//					   SoftwareRole,InfluenceLevel,EngagementLevel,SkillLevel
//  MFD4  : 07/21/98   Additional 2 fields  to be sent to the backend SCSIAdapter
//					   and ComputerManufacturer & Model of SystemInventory
//  MFD5  : 08/1/98    The MathCoprocessor and Color Depth are no longer necessary to be sent to the back end
//                     so the items that are sent to the back end are 66-2 = 64
//  MFD6  : 08/21/98   Added AreaCode and MiddleName( Middle Initial) fo posting
//                     So number of Fields for posting 64+2 = 66
//                     From now on Area code will be sent as a seperate field  to the backend
//                     the logic involved in prefixing with phone number will be removed
//
//	MFD7 :  03/10/99  HWID no longer will be Transmitted to the Backend 
//			  	      Total Entries to Backend 65	
// 
//
#include <mbstring.h>
#include "RW_Common.h"
#include "resource.h"
#include <tchar.h>
#include "dconv.h"
#include "cntryinf.h"
#include "RegWizMain.h"



extern BOOL bOemDllLoaded;
extern HINSTANCE  hOemDll;

#define  NAME_VALUE_SEPERATOR   _T("=")    // Seperator between name and its value
#define  NAME_SEPERATOR         _T("&") 	 // Seperator between the names
#define  RW_BLANK               _T(' ')      // The padding character
#define  RW_WITH_URL			1
#define  PHONE_NUMBER_SEPERATOR _T("-")

#define MAX_NAME_LENGTH    64	 // The Max size of the of the name field
#define VARIABLE_LEN       -2
#define REG_SEPERATOR   TEXT("\\")
#define MAX_TBL_ENTRIES   65     //  No of name fields to be sent to the IIS
#define MAX_REG_VAL_BUF_SZ    300 // The Maximum  size of a value associated with name field
#define STRCONVERT_MAXLEN  1024

static  TCHAR * GetNameString ( TCHAR **pRet,UINT iId,HINSTANCE hIns) ;
int  GetRootOfRegWizRegistry(HINSTANCE hInstance , PHKEY  phKey);
#ifdef _UNICODE
	char* ConvertToMB (TCHAR * szW)
	{
		static char achA[STRCONVERT_MAXLEN];
		WideCharToMultiByte(CP_ACP, 0, szW, -1, achA, STRCONVERT_MAXLEN, NULL, NULL);
		return achA;
	}
#else
	char* ConvertToMB (TCHAR * szW)
	{
		return szW;
	}
#endif

// Information processig functions
void	RW_Dummy (_TCHAR * tcSrc, _TCHAR * tcDes , HINSTANCE hIns )
{
};

void 	RW_LanguageTranslate (_TCHAR * tcSrc, _TCHAR * tcDes , HINSTANCE hIns )
{
	LANGID langID = GetSystemDefaultLangID();
	VerLanguageName(langID,tcDes,MAX_REG_VAL_BUF_SZ);
}

void 	RW_CreateDateProcess (_TCHAR * tcSrc, _TCHAR * tcDes , HINSTANCE hIns )
{
};

void 	RW_RegisterDateProcess (_TCHAR * tcSrc, _TCHAR * tcDes , HINSTANCE hIns )
{
};

void    RW_MailingAddressProcess (_TCHAR * tcSrc, _TCHAR * tcDes , HINSTANCE hIns )
{
	size_t iSrcLen = _tcslen(tcSrc);
	if(iSrcLen)
	{
		_tcscpy(tcDes,TEXT("1"));
	}
	else
	{
		_tcscpy(tcDes,TEXT("2"));
	}
}


void	RW_ValidateTapiCountryCode (_TCHAR * tcSrc, _TCHAR * tcDes , HINSTANCE hIns )
{
	DWORD dwTapiCntryId;
	DWORD dwCode = _ttol(tcSrc);
	dwTapiCntryId = gTapiCountryTable.GetTapiIDForTheCountryIndex(dwCode);
	_stprintf(tcDes,_T("%d"),dwTapiCntryId);


};

void	RW_TranslateCountryCode(_TCHAR * tcSrc, _TCHAR * tcDes , HINSTANCE hIns )
{
	DWORD dwCode = _ttol(tcSrc); // Convert the Current code in string to long
	_tcscpy(tcDes, gTapiCountryTable.GetCountryName(dwCode));

}

void	RW_ParsePhoneNumber(_TCHAR * tcSrc, _TCHAR * tcDes , HINSTANCE hIns )
{
	HKEY	hKey;
	DWORD   infoSize;
	TCHAR   szR[48];
	TCHAR   szParam[256];
	LONG    lRegStatus;
	TCHAR   szInBuf[256];
	//
	// As a part of FE screen changes it is necessary to Prefix the Area code
	// before the phone number
	hKey = NULL;
	if(GetRootOfRegWizRegistry(hIns, &hKey) ) {
		return;
		// Not able to open the Registry Tree for Area Code
		// so simply return ...
	}
	infoSize = 48;
	LoadString( hIns, IDS_AREACODE_KEY,
						szR,
						sizeof(szR)/ sizeof (TCHAR));
	lRegStatus = RegQueryValueEx(hKey,&szR[1],NULL,0,(LPBYTE)szParam,&infoSize);
	if(tcSrc[0] !=  _T('\0')){
		_tcscpy(szInBuf,tcSrc);
	}else {
		szInBuf[0] = _T('\0');
	}

	if (lRegStatus != ERROR_SUCCESS){
		   	return; //  	RWZ_INVALID_INFORMATION;
	}else {
		if(szParam[0] != _T('\0')) {
			_tcscpy(tcDes,szParam); // Area Code
			_tcscat(tcDes,PHONE_NUMBER_SEPERATOR);     // Separator
			_tcscat(tcDes,szInBuf);
			
		}else {
			//No need to do any thing
			;
		}
		if(hKey)
		RegCloseKey(hKey);
	}
	

}

void	RW_PrcsProductId (LPTSTR tcSrc, LPTSTR tcDes , HINSTANCE hIns )
{
	_TCHAR	seps[] = _T("-");
	LPTSTR	token;
	LPTSTR	buf;
	if(*tcSrc == 0 ) {
		*tcDes=0;
		return;
	}

   buf = new _TCHAR[_tcslen(tcSrc) * sizeof(_TCHAR) +sizeof(_TCHAR)];	
   token = _tcstok( tcSrc, seps );
   _tcscpy(buf,token);

   token = _tcstok( NULL, seps );
   while( token != NULL )
   {
	/* Get next token: */
	_tcscat(buf,token);
	token = _tcstok( NULL, seps );
   }
	
	_tcscpy(tcDes,buf);
	
	delete[] buf;
	
};

//
// void	RW_ParseTotalSize (_TCHAR * tcSrc, _TCHAR * tcDes  )
// This function retrives the the Size  wich is founs as the first  token in tcSrc
// After the end of this function the tcDes will be assiged with the Size
//
void	RW_ParseTotalSize (_TCHAR * tcSrc, _TCHAR * tcDes , HINSTANCE hIns )
{
	int isBlankPassed=0;
	TCHAR tcSteps[]   = TEXT(" ,\t\n");
	if(*tcSrc == 0 )
	{
		*tcDes=0;
		return;
	}

	// since the value in tcSrc is "RAM UNIT" so it is encouh if is pass only the first word
	TCHAR  *tcToken;
    tcToken =_tcstok(tcSrc, tcSteps);
	_tcscpy(tcDes,tcToken);

}

//
// void	RW_ParseUnits (_TCHAR * tcSrc, _TCHAR * tcDes  )
// This function retrives the the unit name wich is founs as the second token in tcSrc
// After the end of this function the tcDes will be assiged with the Units
//
void	RW_ParseUnits (_TCHAR * tcSrc, _TCHAR * tcDes , HINSTANCE hIns )
{
	int isBlankPassed=0;
	TCHAR tcSteps[]   = TEXT(" ,\t\n\0");
	if(*tcSrc == 0 )
	{
		*tcDes=0;
		return;
	}
	// since the value in tcSrc is "RAM UNIT" so it is encouh if is pass only the first word
	TCHAR  *tcToken;
    tcToken =_tcstok(tcSrc, tcSteps); // get the size
    tcToken = _tcstok( NULL, tcSteps); // get the units
	_tcscpy(tcDes,tcToken);

}

//
// RegWizInfoDetails
// This  structure is used to create a table which has the Namefiled,
// the value reference in the Resource  which is to  be used to retrive from registry
// and Function  to process the value
//
typedef struct  RegWizInfoDetails
{
	int    m_iIndex;
	TCHAR   m_czName[MAX_NAME_LENGTH];
	int    m_iLen;
	int    m_ResourceIndex;
	int    m_iParam;
	void (*m_fp)(_TCHAR * tcSrc, _TCHAR * tcDes, HINSTANCE hIns);
} _RegWizInfoDetails ;

static RegWizInfoDetails  sRegWizInfoTbl[MAX_TBL_ENTRIES] =
{
{ 1,    _T("RegWizVer")			,8,				IDS_INFOKEY30,		0, RW_Dummy },
{ 2,    _T("CodePage")			,5,				-1 ,				0, RW_Dummy },
{ 3,    _T("LangCode")			,5,				IDS_INFOKEY34 ,		0, RW_Dummy },
{ 4,    _T("LangName")			,30,			IDS_INFOKEY34 ,		0, RW_LanguageTranslate },
{ 5,    _T("CreatedDate")		,10,			-1 ,				0, RW_CreateDateProcess    },
{ 6,    _T("RegDate")			,10,			IDS_INFOKEY33,		0, RW_RegisterDateProcess },
{ 7,    _T("FName")				,VARIABLE_LEN,	IDS_INFOKEY1 ,		0, RW_Dummy },
{ 8,    _T("LName")				,VARIABLE_LEN,	IDS_INFOKEY2 ,		0, RW_Dummy },
{ 9,    _T("CompanyName")		,VARIABLE_LEN,	IDS_INFOKEY3,		0, RW_Dummy },
{ 10,   _T("AddrType")			,1,				IDS_INFOKEY3,		0, RW_MailingAddressProcess },
{ 11,   _T("Addr1")				,VARIABLE_LEN , IDS_INFOKEY4,		0, RW_Dummy },
{ 12,   _T("Addr2")				,VARIABLE_LEN,  IDS_INFOKEY5,		0, RW_Dummy },
{ 13,   _T("City")				,VARIABLE_LEN,  IDS_INFOKEY6,		0, RW_Dummy },
{ 14,   _T("State")				,VARIABLE_LEN , IDS_INFOKEY7,		0, RW_Dummy },
{ 15,   _T("Zip")				,VARIABLE_LEN , IDS_INFOKEY8,		0, RW_Dummy },
{ 16,   _T("CountryCode")		,4 ,            IDS_INFOKEY9 ,		0, RW_ValidateTapiCountryCode  },
{ 17,   _T("Country")			,60,            IDS_INFOKEY9 ,		0, RW_TranslateCountryCode },
{ 18,   _T("Phone")				,VARIABLE_LEN , IDS_INFOKEY10,		0, RW_Dummy },
{ 19,   _T("NoOther")			,1 ,			IDS_INFOKEY11,		0, RW_Dummy },
{ 20,   _T("Product")			,255,			IDS_INFOKEY28,		0, RW_Dummy },
{ 21,   _T("PID")				,20,			IDS_INFOKEY12,		0, RW_PrcsProductId },
{ 22,   _T("OEM")				,255 ,			IDS_INFOKEY29,		0, RW_Dummy },
{ 23,   _T("SysInv")			,1,				IDS_INFOKEY26,		0, RW_Dummy },
{ 24,   _T("OS")				,40,			IDS_INFOKEY25,		0, RW_Dummy },
{ 25,   _T("CPU")				,20 ,			IDS_INFOKEY13,		0, RW_Dummy },
//{ 26,   _T("MathCo")			,1  ,			IDS_INFOKEY14,		0, RW_Dummy },
{ 27,   _T("TotalRAM")			,8 ,			IDS_INFOKEY15,		0, RW_ParseTotalSize},
{ 28,   _T("RAMUnits")			,2 ,			IDS_INFOKEY15,		0, RW_ParseUnits},
{ 29,   _T("TotalDisk")			,8 ,			IDS_INFOKEY16,		0, RW_ParseTotalSize},
{ 30,   _T("DiskUnits")			,2 ,			IDS_INFOKEY16,		0, RW_ParseUnits},
{ 31,   _T("RemoveableMedia")	,60 ,			IDS_INFOKEY17,		0, RW_Dummy },
//{ 32,   _T("DisplayRes")		,16 ,			IDS_INFOKEY18 ,		0, RW_Dummy },
{ 33,   _T("DisplayColorDepth")	,8 ,			IDS_INFOKEY19 ,		0, RW_Dummy },
{ 34,   _T("PointingDevice")	,75,			IDS_INFOKEY20 ,		0, RW_Dummy },
{ 35,   _T("Network")			,75 ,			IDS_INFOKEY21 ,		0, RW_Dummy },
{ 36,   _T("Modem")				,75 ,			IDS_INFOKEY22 ,		0, RW_Dummy },
{ 37,   _T("Sound")				,60 ,			IDS_INFOKEY23 ,		0, RW_Dummy },
{ 38,   _T("CDROM")				,40 ,			IDS_INFOKEY24 ,		0, RW_Dummy },
{ 39,   _T("ProdInv")			,1 ,			IDS_INFOKEY27 ,		0, RW_Dummy },
{ 40,   _T("InvProd1")			,75,			IDS_PRODUCTBASEKEY, 1, RW_Dummy },
{ 41,   _T("InvProd2")			,75,			IDS_PRODUCTBASEKEY, 2, RW_Dummy },
{ 42,   _T("InvProd3")			,75,			IDS_PRODUCTBASEKEY, 3, RW_Dummy },
{ 43,   _T("InvProd4")			,75,			IDS_PRODUCTBASEKEY, 4, RW_Dummy },
{ 44,   _T("InvProd5")			,75,			IDS_PRODUCTBASEKEY, 5, RW_Dummy },
{ 45,   _T("InvProd6")			,75,			IDS_PRODUCTBASEKEY, 6, RW_Dummy },
{ 46,   _T("InvProd7")			,75,			IDS_PRODUCTBASEKEY, 7, RW_Dummy },
{ 47,   _T("InvProd8")			,75,			IDS_PRODUCTBASEKEY, 8, RW_Dummy },
{ 48,   _T("InvProd9")			,75,			IDS_PRODUCTBASEKEY, 9, RW_Dummy },
{ 49,   _T("InvProd10")			,75,			IDS_PRODUCTBASEKEY, 10,RW_Dummy },
{ 50,   _T("InvProd11")			,75,			IDS_PRODUCTBASEKEY, 11,RW_Dummy },
{ 51,   _T("InvProd12")			,75,			IDS_PRODUCTBASEKEY, 12,RW_Dummy },
{ 52,   _T("EmailName")			,50,            IDS_INFOKEY35,		0, RW_Dummy },
{ 53,   _T("Reseller")			,30,            IDS_INFOKEY36,		0, RW_Dummy },
{ 54,   _T("ResellerCity")		,20,            IDS_INFOKEY37,		0, RW_Dummy },
{ 55,   _T("ResellerState")		,3,             IDS_INFOKEY38,		0, RW_Dummy },
//{ 56,   _T("HWID")				,32,            IDS_INFOKEY39,		0, RW_Dummy },
{ 57,   _T("MSID")				,32,            IDS_INFOKEY40,		0, RW_Dummy },
{ 58,   _T("Extension")			,32,            IDS_INFOKEY41,		0, RW_Dummy },
{ 59,   _T("DivisionName")      ,50,            IDS_DIVISIONNAME_KEY,   0, RW_Dummy },
{ 60,   _T("UserID")            ,50,            IDS_USERID_KEY,     0, RW_Dummy },
{ 61,   _T("SoftwareRole")		,2,             IDS_BUSINESSQ1,		0, RW_Dummy },
{ 62,   _T("InfluenceLevel")	,2,             IDS_HOMEQ1,		0, RW_Dummy },
{ 63,   _T("EngagementLevel")   ,2,             IDS_HOMEQ2,   0, RW_Dummy },
{ 64,   _T("SkillLevel")        ,2,             IDS_HOMEQ3,     0, RW_Dummy },
{ 65,   _T("SCSIAdapter")       ,75,            IDS_SCSI_ADAPTER,     0, RW_Dummy },
{ 66,   _T("ComputerManf")     ,256,            IDS_COMPUTER_MODEL,     0, RW_Dummy },
{ 67,   _T("AreaCode")       ,30,            IDS_AREACODE_KEY,     0, RW_Dummy },
{ 68,   _T("Mname")     ,75,            IDS_MIDDLE_NAME,     0, RW_Dummy }
};





//
//  PadWithBlanks(TCHAR **pSrc, TCHAR ** pDes,int iLen)
//
//  This function adds space to the soiurce string so the resultant string is of the length
//  specified by iLen
//
//
//
void PadWithBlanks(TCHAR *pSrc, TCHAR * pDes,int iLen)
{
	int iSrcLen;

	iSrcLen = _tcslen(pSrc) * sizeof(_TCHAR);
	if(iLen < 0)
	{
		//  if it is  variable length then copy the string and return
		_tcscpy(pDes,pSrc);
		return;
	}
	
	//
	// Try to copy till iLen
	// the extra +1 is added to add a null terminator after copy
	_tcsnccpy(pDes,pSrc,iLen+ sizeof(_TCHAR));

	if(iSrcLen < iLen )
	{
		// the source string is less than the expected maximum length
		for(int iIndex = iSrcLen;iIndex < iLen;iIndex++)
		{
			pDes[iIndex] = RW_BLANK; // Adds blank
		}
		pDes[iIndex] = _T('\0');
	}
	
}

// int  GetRootOfRegWizRegistry(HINSTANCE hInstance , PHKEY  phKey)
//
// Description :
//	This function Opens the User  Information value in the registry for
//  regWizard configured values to be read
//
//	Return Information:
//	This function return the HANDLE of the registry in phKey and returns 0
//  If the Key is not found the  function returns 1
//
//


int  GetRootOfRegWizRegistry(HINSTANCE hInstance , PHKEY  phKey)
{
	TCHAR uszPartialKey[128];
	TCHAR uszRegKey[128];
	uszRegKey[0] = _T('\0');
	
	int resSize = LoadString(hInstance,IDS_KEY2,uszRegKey,128);
	_tcscat(uszRegKey,REG_SEPERATOR);
	resSize = LoadString(hInstance,IDS_KEY3,uszPartialKey,128);
	_tcscat(uszRegKey,uszPartialKey);
	_tcscat(uszRegKey,REG_SEPERATOR);
	resSize = LoadString(hInstance,IDS_KEY4,uszPartialKey,128);
	_tcscat(uszRegKey,uszPartialKey);
	
	#ifdef USE_INFO_SUBKEYS
		_tcscat(uszRegKey,REG_SEPERATOR);
	#endif

	LONG regStatus =RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		uszRegKey,
		0,
		KEY_ALL_ACCESS,
		phKey);

	if (regStatus != ERROR_SUCCESS)
	{
		return 1; // error
	}
	else
	{
		return 0; // Success
	}
}

//
// TCHAR * GetNameString ( TCHAR **pRet,UINT uId,HINSTANCE hIns)
//
//  Description :
//
//	This function retrives the Name field  for Transmission from the Resource
//  Some names in the  resouce is prefixed with '_' so this function  removes the
//  prefix character.
//
//  Return Information:
//  This function returns  NULL if the string is not found in the resource
//	It accepts the address of a TCHAR pointer  ,the string address is assigned in the
//  pointer.  It also returns the same address as a return Status
//
//  Note :
//  It is not required to delete the pointer returned by this function as it is static
//

TCHAR * GetNameString ( TCHAR **pRet,UINT uId,HINSTANCE hIns)
{
	static TCHAR tczRetValue[MAX_NAME_LENGTH ];
	int  iLoad = LoadString( hIns, uId,
						tczRetValue,
						sizeof(tczRetValue)/ sizeof (TCHAR));
	if(!iLoad )
	{
		pRet = NULL;
		return NULL;
	}

	if( tczRetValue[0]   == _T('_')   )
	{
		*pRet = &tczRetValue[1];
		return &tczRetValue[1];
	}
	else
	{
		*pRet = &tczRetValue[0];
		return &tczRetValue[0];
	}

}

//
//
// URL Encoding

/*===================================================================
URLEncodeLen

Return the storage requirements for a URL-Encoded string

Parameters:
	szSrc  - Pointer to the string to URL Encode

Returns:
	the number of bytes required to encode the string
===================================================================*/

int URLEncodeLen(const char *szSrc)
{
	int cbURL = 1;		// add terminator now
	while (*szSrc)
	{
		if (*szSrc & 0x80)				// encode foreign characters
			cbURL += 3;
		else
		if (*szSrc == ' ')			// encoded space requires only one character
			++cbURL;
		else
		if (! isalnum(*szSrc))		// encode non-alphabetic characters
			cbURL += 3;
		else
			++cbURL;

		++szSrc;
	}
	return cbURL;
}



/*===================================================================
URLEncode

URL Encode a string by changing space characters to '+' and escaping
non-alphanumeric characters in hex.

Parameters:
	szDest - Pointer to the buffer to store the URLEncoded string
	szSrc  - Pointer to the source buffer

Returns:
	A pointer to the NUL terminator is returned.
===================================================================*/

char *URLEncode(char *szDest, const char *szSrc)
{
	char hex[] = "0123456789ABCDEF";

	while (*szSrc)
	{
		if (*szSrc == ' ')
		{
			*szDest++ = '+';
			++szSrc;
		}
		else
		if (!isalnum(*szSrc) || (*szSrc & 0x80))
		{
			*szDest++ = '%';
			*szDest++ = hex[BYTE(*szSrc) >> 4];
			*szDest++ = hex[*szSrc++ & 0x0F];
		}
		else
			*szDest++ = *szSrc++;
	}

	*szDest = '\0';
	return szDest;
}



//
// This class is used to store the RegWiz Info in URL encode format
// This class is construected with the TxBuffer pointer
// The AppendToBuffer converts the string to URL encoded form and adds to the TxBuffer
// if the TxBuffer size  is less then it only computesthe size  and does not transfer
// the buffer contents
//

class RegWizTxBuffer
{

public :

	char *m_pBuf;
	int   m_iSizeExceeded; // set
	DWORD *m_pdInitialSize;
	DWORD m_dCurrentIndex;

	RegWizTxBuffer(char *tcTxBuf, DWORD * pRetLen)
	{
		m_pBuf = tcTxBuf; // Initial Pointer of the Destination Buffer
		m_pdInitialSize = pRetLen; // The Destination Buffer Size
		m_dCurrentIndex = 0; // Current Index of the number of bytes of information transmitted
		m_iSizeExceeded = 0;
		m_pBuf[0] = '\0';
	}

	void  AppendToBuffer(TCHAR *tcTxBuf, int iIsUrl=0)
	{
		int iLen=0;
		
		#ifdef _UNICODE
			unsigned char *mbpTxBuf;
		#endif

		// The TxBuffer has to be converted to MultiByte in case
		// Convert to MultByte

		#ifdef _UNICODE
			mbpTxBuf = (unsigned char *)ConvertToMB (tcTxBuf); // Convert to MultiByte
			iLen =  _mbslen (mbpTxBuf);
		#else
			iLen = _tcsclen(tcTxBuf);
		#endif

		if(iIsUrl)
		{
			#ifdef _UNICODE
				iLen= URLEncodeLen((const char *)mbpTxBuf);
			#else
				iLen = URLEncodeLen(tcTxBuf);
			#endif
		}

		if( m_dCurrentIndex + iLen >= *m_pdInitialSize )
		{
			// continue counting the lengthn required
			m_iSizeExceeded = 1;
	 	}
		else
		{
			if( iIsUrl)
			{
				#ifdef _UNICODE
					URLEncode(m_pBuf+m_dCurrentIndex,(const char *)mbpTxBuf);
				#else
					// for MBCS and SBCS
					URLEncode(m_pBuf+m_dCurrentIndex,tcTxBuf);
				#endif
			}
			else
			{
				#ifdef _UNICODE
					strcat(m_pBuf,(const char *)mbpTxBuf);
				#else
					_tcscat(m_pBuf,tcTxBuf);
				#endif
			}
		;

		}
		if(iIsUrl)
		m_dCurrentIndex +=  iLen-1;
		else
		m_dCurrentIndex +=  iLen;

	}

	int  IsValidBuffer()
	{
		m_pBuf[m_dCurrentIndex] = '\0';
		*m_pdInitialSize = m_dCurrentIndex;
		return m_iSizeExceeded ;
	}

};


int PrepareRegWizTxbuffer(HINSTANCE hIns, char *tcTxBuf, DWORD * pRetLen)
{
	int				iRetValue;
	HKEY			hKey;
	LONG			lRegStatus;
	TCHAR			*szR;
	TCHAR			tczTmp[10];
	TCHAR			szParam[MAX_REG_VAL_BUF_SZ]; // ?? chk in case of Unicode
	unsigned long	infoSize;
	RegWizTxBuffer  TxferBuf(tcTxBuf,pRetLen);

	iRetValue		= RWZ_NOERROR;
	infoSize		= MAX_REG_VAL_BUF_SZ;

	if(GetRootOfRegWizRegistry(hIns, &hKey) )
	{
		iRetValue = RWZ_NO_INFO_AVAILABLE;
		// No User Information is Available So Abort the program
	}
	else
	{
		// Process for all the information entries
		 for(int i =0;i <  MAX_TBL_ENTRIES ;i++)
		 {
			infoSize = MAX_REG_VAL_BUF_SZ;
			szParam[0] = '\0';
		
			#ifdef _LOG_IN_FILE			
		 		RW_DEBUG << "\n" << i+1  << "\t"  << ConvertToMB (sRegWizInfoTbl[i].m_czName) << "\t" << flush;
			#endif
			
			TxferBuf.AppendToBuffer(sRegWizInfoTbl[i].m_czName,RW_WITH_URL);
			
			TxferBuf.AppendToBuffer(NAME_VALUE_SEPERATOR);

			if( sRegWizInfoTbl[i].m_ResourceIndex < 1)
			{
				 // continue processing
			}
			else
			{
				if( GetNameString(&szR,sRegWizInfoTbl[i].m_ResourceIndex,hIns ) )
				{

					if(	sRegWizInfoTbl[i].m_iParam )
					{
						//  This block is  for appending Product name index
						_itot(sRegWizInfoTbl[i].m_iParam,tczTmp,10);
						_tcscat(szR,_T(" ")); // add a single blank
						_tcscat(szR,tczTmp);
						#ifdef _LOG_IN_FILE
							RW_DEBUG  << ConvertToMB (szR) << "\t";
						#endif
					}
					lRegStatus = RegQueryValueEx(hKey,szR,NULL,0,(  LPBYTE )   szParam,&infoSize);

					if (lRegStatus != ERROR_SUCCESS)
					{
					   	return 	RWZ_INVALID_INFORMATION;
					}

					#ifdef _LOG_IN_FILE
						RW_DEBUG  << "[" <<ConvertToMB (szParam )<< "]\t"   << flush;
					#endif
					
				}
				else
				{
 					//
					//  This condition can happen if information is not found in the Resource
					//  If the function  entres this block the it is necessary to verify the
					//  the resource string in the table mapping
					//
					return 	RWZ_INTERNAL_ERROR;

				}
			}

			if(szParam)
			{
				(*sRegWizInfoTbl[i].m_fp)(szParam,szParam,hIns); // Invoke processing Function
				//PadWithBlanks(szParam,szRet,sRegWizInfoTbl[i].m_iLen); // Add balnks
			}

			//TxferBuf.AppendToBuffer(szRet,RW_WITH_URL); with padded
			TxferBuf.AppendToBuffer(szParam,RW_WITH_URL);
			//
			//
			// Skip Name key seperator for the last entry
			if(i!= MAX_TBL_ENTRIES-1)
				TxferBuf.AppendToBuffer(NAME_SEPERATOR);

		 }// end of for loop
 	}

	if(TxferBuf.IsValidBuffer())
	{

		iRetValue=RWZ_BUFFER_SIZE_INSUFFICIENT;
	}
	
	return iRetValue;

}


DWORD OemTransmitBuffer(HINSTANCE hIns,char *sztxBuffer,DWORD *nInitialSize)
{	
	BOOL bValueExceeded = FALSE;
	
	if(bOemDllLoaded == TRUE)
	{
		DWORD nCount,nLen,nCurrentLen = 0;
		HKEY hOemKey;
			
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\n OEM: Inside OemTransmitBuffer" << flush;
		#endif
		
		GetRootOfRegWizRegistry(hIns, &hOemKey);

		OEMDataCount	pOEMDataCount;

		pOEMDataCount = (OEMDataCount) GetProcAddress(hOemDll, "OEMDataCount");
		if (pOEMDataCount == NULL)
		{
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n OEM: OEMDataCount Getproc Error" << flush;
			#endif
			return RWZ_INTERNAL_ERROR;
		}
		
		nCount = (DWORD)pOEMDataCount();
		
		if(nCount > 25)
		{
			nCount = 25;
		}

		for( DWORD index = 1; index <= nCount; index++	)
		{
			_TCHAR szOEMValueName[64];
			_TCHAR szOEMBase[64];
			char szBuffer[1024];
			BOOL   bIsUnicode;
			BYTE lpValue[256] ;
			_TCHAR szValue[256];
		
			LoadString(hIns,IDS_OEMBASEKEY,szOEMBase,64);
		
			_stprintf(szOEMValueName,_T("%s_%i"),szOEMBase,index);

			#ifdef _UNICODE
			  _mbscpy((unsigned char *)szBuffer,(const unsigned char *)ConvertToMB(szOEMValueName));
			#else
			  _tcscpy(szBuffer,szOEMValueName);
			#endif
			
			
			OEMGetData	pOEMGetData;

			pOEMGetData = (OEMGetData) GetProcAddress(hOemDll, "OEMGetData");
			if (pOEMGetData == NULL)
			{
				#ifdef _LOG_IN_FILE
					RW_DEBUG << "\n OEM: OEMGetData Getproc Error"<< flush;
				#endif
				return RWZ_INTERNAL_ERROR;
			}

            nLen = sizeof(lpValue);

			pOEMGetData((WORD)index,&bIsUnicode,lpValue,(WORD)nLen);

			#ifdef _UNICODE
				if(!bIsUnicode)
				{
				  MultiByteToWideChar(CP_ACP, 0, (LPCSTR)lpValue, -1, (LPTSTR)szValue, 256);
				}
				else
				{
					_tcscpy((LPTSTR)szValue,(LPCTSTR)lpValue);
				}
			#else
				if(bIsUnicode)
				{
				  WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)lpValue, -1, (LPTSTR)szValue, 256, NULL, NULL);
				}
				else
				{
					_tcscpy((LPTSTR)szValue,(LPCTSTR)lpValue);
				}
			#endif	
			
			if(	_tcscmp((LPCTSTR)szValue,_T("")))
			{
				nLen = strlen("&");
				if( nCurrentLen + nLen >= *nInitialSize )
				{
					bValueExceeded = TRUE;
			 	}
				else
				{
					strcat(sztxBuffer,"&");
				}
				nCurrentLen +=  nLen;

				nLen = URLEncodeLen(szBuffer);
				if( nCurrentLen + nLen >= *nInitialSize )
				{
					bValueExceeded = TRUE;
			 	}
				else
				{
					URLEncode(sztxBuffer+nCurrentLen,szBuffer);
				}

				nCurrentLen +=  nLen-1;
		
				RegSetValueEx(hOemKey,szOEMValueName,NULL,REG_SZ,(CONST BYTE *)szValue,_tcslen((LPCTSTR)szValue));
				
				if(bIsUnicode)
				{
					_mbscpy((unsigned char *)szBuffer,(unsigned char *)ConvertToMB((TCHAR *)lpValue));
				}
				else
				{
					_mbscpy((unsigned char *)szBuffer,(unsigned char *)lpValue);
				}

				#ifdef _LOG_IN_FILE			
		 		 RW_DEBUG << "\n OEM " << index << "\t"<< ConvertToMB(szOEMValueName)<< "\t"
										<< szBuffer <<flush;
				#endif
				
				nLen = strlen("=");
				if( nCurrentLen + nLen >= *nInitialSize )
				{
					bValueExceeded = TRUE;
			 	}
				else
				{
					strcat(sztxBuffer,"=");
				}
				nCurrentLen +=  nLen;

				
				nLen = URLEncodeLen(szBuffer);
				if( nCurrentLen + nLen >= *nInitialSize )
				{
					#ifdef _LOG_IN_FILE
						RW_DEBUG << "\n OEM:Buffer value Exceeded" << flush;
						RW_DEBUG << "\n OEM:Current Length:" << nCurrentLen << flush;
						RW_DEBUG << "\n OEM:Length of present value:" << nLen << flush;
						RW_DEBUG << "\n OEM:Initial Size:" << *nInitialSize << flush;
					#endif
					bValueExceeded = TRUE;
			 	}
				else
				{
					URLEncode(sztxBuffer+nCurrentLen,szBuffer);
				}

				nCurrentLen +=  nLen-1;
			}				
		}
		*nInitialSize = nCurrentLen;
		if(bValueExceeded )
		{
			return RWZ_BUFFER_SIZE_INSUFFICIENT;
		}
		else
		{
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n OEM: OemTransmitBuffer Successful" << flush;
			#endif
			return RWZ_NOERROR;
		}
	}
	else
	{
		*nInitialSize = 0;
	}

	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n OEM: OemTransmitBuffer Successful" << flush;
	#endif

	return RWZ_NOERROR;
}
