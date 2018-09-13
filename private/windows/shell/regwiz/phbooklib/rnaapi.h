//-----------------------------------------------------------------------------
// RNAAPI class
//
// This class provides a series of cover function for the RNAPH/RASAPI32 dlls
//
// Created 1-29-96	ChrisK

//-----------------------------------------------------------------------------
// TYPEDEF
typedef DWORD (WINAPI* PFNRASENUMDEVICES)(LPRASDEVINFO lpRasDevInfo, LPDWORD lpcb, LPDWORD lpcDevices);
typedef DWORD (WINAPI* PFNRASVALIDATEENTRYNAE)(LPSTR lpszPhonebook, LPSTR lpszEntry);
typedef DWORD (WINAPI* PFNRASSETENTRYPROPERTIES)(LPSTR lpszPhonebook, LPSTR lpszEntry, LPBYTE lpbEntryInfo, DWORD dwEntryInfoSize, LPBYTE lpbDeviceInfo, DWORD dwDeviceInfoSize);
typedef DWORD (WINAPI* PFNRASGETENTRYPROPERTIES)(LPSTR lpszPhonebook, LPSTR lpszEntry, LPBYTE lpbEntryInfo, LPDWORD lpdwEntryInfoSize, LPBYTE lpbDeviceInfo, LPDWORD lpdwDeviceInfoSize);

//-----------------------------------------------------------------------------
// CLASS
// ############################################################################
class CRNAAPI
{
public:
	void far * operator new( size_t cb ) { return GlobalAlloc(GPTR,cb); };
	void operator delete( void far * p ) {GlobalFree(p); };

	CRNAAPI();
	~CRNAAPI();

	DWORD RasEnumDevices(LPRASDEVINFO, LPDWORD, LPDWORD);
	DWORD RasValidateEntryName(LPSTR,LPSTR);
	DWORD RasSetEntryProperties(LPSTR lpszPhonebook, LPSTR lpszEntry,
								LPBYTE lpbEntryInfo, DWORD dwEntryInfoSize,
								LPBYTE lpbDeviceInfo, DWORD dwDeviceInfoSize);
	DWORD RasGetEntryProperties(LPSTR lpszPhonebook, LPSTR lpszEntry,
								LPBYTE lpbEntryInfo, LPDWORD lpdwEntryInfoSize,
								LPBYTE lpbDeviceInfo, LPDWORD lpdwDeviceInfoSize);


private:
	BOOL LoadApi(LPSTR, FARPROC*);

	HINSTANCE m_hInst;
	HINSTANCE m_hInst2;

	PFNRASENUMDEVICES m_fnRasEnumDeviecs;
	PFNRASVALIDATEENTRYNAE m_fnRasValidateEntryName;
	PFNRASSETENTRYPROPERTIES m_fnRasSetEntryProperties;
	PFNRASGETENTRYPROPERTIES m_fnRasGetEntryProperties;
};
