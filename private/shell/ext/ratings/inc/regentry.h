/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991-1994                    **/
/***************************************************************************/


/****************************************************************************

regentry.h

Mar. 94		JimH
Mar. 94     LenS    Added NLS_STR form of GetStringValue
Mar. 94     LenS    Added MoveToSubKey
Mar. 94     LenS    Added RegEnumValues class
Mar. 94     LenS    Added NPMachineEntries class

hard tabs at 4

Wrapper for registry access


Construct a RegEntry object by specifying the subkey (under
HKEY_CURRENT_USER by default, but can be overridden.)

All member functions are inline so there is minimal overhead.

All member functions (except the destructor) set an internal
error state which can be retrieved with GetError().
Zero indicates no error.

RegEntry works only with strings and DWORDS which are both set
using the overloaded function SetValue()

	SetValue("valuename", "string");
	SetValue("valuename", 42);
	
Values are retrieved with GetString() and GetNumber().  GetNumber()
allows you to specificy a default if the valuename doesn't exist.

DeleteValue() removes the valuename and value pair.

****************************************************************************/

#ifndef	REGENTRY_INC
#define	REGENTRY_INC

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <npstring.h>


class RegEntry
{
	public:
		RegEntry(const char *pszSubKey, HKEY hkey = HKEY_CURRENT_USER);
		~RegEntry();
		
		long	GetError()	{ return _error; }
		long	SetValue(const char *pszValue, const char *string);
		long	SetValue(const char *pszValue, unsigned long dwNumber);
		char *	GetString(const char *pszValue, char *string, unsigned long length);
		VOID    GetValue(const char *pszValueName, NLS_STR *pnlsString);
		long	GetNumber(const char *pszValue, long dwDefault = 0);
		long	DeleteValue(const char *pszValue);
		long	FlushKey();
        VOID    MoveToSubKey(const char *pszSubKeyName);
        HKEY    GetKey()    { return _hkey; }

	private:
		HKEY	_hkey;
		long	_error;
        BOOL    bhkeyValid;
};

class RegEnumValues
{
	public:
		RegEnumValues(RegEntry *pRegEntry);
		~RegEnumValues();
		long	Next();
		char *	GetName()       {return pchName;}
        DWORD   GetType()       {return dwType;}
        LPBYTE  GetData()       {return pbValue;}
        DWORD   GetDataLength() {return dwDataLength;}

	private:
        RegEntry * pRegEntry;
		DWORD   iEnum;
        DWORD   cEntries;
		CHAR *  pchName;
		LPBYTE  pbValue;
        DWORD   dwType;
        DWORD   dwDataLength;
        DWORD   cMaxValueName;
        DWORD   cMaxData;
        LONG    _error;
};

class NPMachineEntries : public RegEntry
{
    public:
		NPMachineEntries(const char *pszSectionName);
        const char * GetSectionName() { return pszSectionName; }

    private:
        const char * pszSectionName; // Warning: data not copied into object.
};

#endif
