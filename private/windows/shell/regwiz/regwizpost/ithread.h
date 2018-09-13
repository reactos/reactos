#ifndef __InternetThread_h__
#define __InternetThread_h__

#include "ATKinternet.h"
//#include <tchar.h>



//  CInternetThread Class...manages worker thread which POSTS to 
//	Register.msn.com.
//
class CInternetThread
{
public:
   // Construction
   //
   CInternetThread();
   ~CInternetThread();



   // Re-initialized Internet functions. Used after changing access type.
   //
   void ResetSession();
      
   // Manage Buffer where HTML text is placed.
   //
	LPCSTR		GetBuffer()		{	return m_strBuffer; }
	void		SetBuffer(LPSTR strBuffer);
	void		SetBuffer(LPSTR strBuffer, DWORD dwLen);
	DWORD  PostData(HWND hWnd);

	void Initialize(HINSTANCE hIns)
	{
		m_hInstance = hIns;
		m_strIISServer = new TCHAR[256];
		m_strPath      = new TCHAR[256];
		m_UserName     = new TCHAR[256];
		m_Password     = new TCHAR[256];
		m_bPostWithSSL = TRUE;

		LoadString(m_hInstance, IDS_HTTP_SERVER,
			m_strIISServer, 255);
		LoadString(m_hInstance, IDS_HTTP_SERVER_PATH,
			m_strPath, 255);
		LoadString(m_hInstance, IDS_HTTP_USERNAME,
			m_UserName, 255);
		LoadString(m_hInstance, IDS_HTTP_PASSWORD, 
			m_Password, 255);
	}

	//
	//
	void SetHInstance ( HINSTANCE hIns) 
	{
			m_hInstance = hIns;
		//Initialize(hIns);
	}

	HINSTANCE GetHInstance ()
	{  
		return m_hInstance;
	}

	// Proxy Server name.
	//
	BOOL GetSystemProxyServer(  PCHAR szProxy, 
								DWORD dwBufferLength,
								int *ipProxyPort);
	// This gets proxy using ICW call 
	LPCTSTR GetProxyServer()	{	return m_strProxyServer; }
	void SetProxyServer(LPSTR strProxyServer, int iProxyPort);
	void GetSystemProxySettings( PCHAR szProxy, 
							   DWORD dwBufferLength);
	void SetSystemProxySettings( PCHAR szProxy ); 
   // HTTP Server name.
   //
	LPCTSTR GetIISServer()		{	return m_strIISServer; }
	void SetIISServer(LPTSTR strIISServer);

   // HTTP Server Path
   //
	LPCTSTR GetServerPath()		{	return m_strPath; }
	void SetServerPath(LPTSTR strPath);
	
	void SetSSLFlag(BOOL bFlag)	{	m_bPostWithSSL = bFlag;}
	
   // POST the Data in m_strBuffer into 
   //
   //DWORD  PostData(HWND hWnd);

   // Access Type: **** At present Not used  ***
   //
   //int	GetAccessTypeIndex();
   //void SetAccessTypeIndex(int index);

   //
   // General ICW DLL loading  related functions
   HINSTANCE LoadInetCfgDll();
   BOOL InstallModem(HWND hwnd);
   void UnLoadInetCfgDll(); 
//private:
   // Worker thread calls _PostDataWorker.
   static UINT PostDataThread(LPVOID pvThread) ;

   // This is where the actually work is done.
   UINT  _PostDataWorker(HWND hWnd);
   UINT	 GetBackEndResult(HINTERNET hConnect);


   LPTSTR	m_strIISServer;
   LPTSTR	m_strPath;
   LPTSTR	m_strProxyServer;
   DWORD	m_dwAccessType;
   BOOL		m_bPostWithSSL;

  
   LPSTR	 m_strBuffer;		// Buffer to be POSTed to Register.msn.com
   DWORD	 m_dwBufferLen;		// Buffer Len
   HINSTANCE m_hInstance;
   HINTERNET m_hSession;
   LPTSTR    m_UserName;
   LPTSTR    m_Password;
   HINSTANCE m_hICWDllInstance;
};

// Working Thread which does all the actually internet work.
//
UINT PostDataThread(LPVOID pvThreadData);
#endif 


// How to use this class
// 
//
////////// Check if connectivity to a an IIS exists //////////////////////
// i)  Call CInternetThread.SetProxyServer(szProxy) to set the Proxy if any exists.
// ii) Call CInternetThread.SetIISServer(szIISServer) to set the IP Address (URL)
//	   of the Internet Server.
// iii)Call CInternetThread.InternetConnectivityExists() which will return TRUE
//    if connectivity to the ISS server (szIISServer in 1) exists, else FALSE.
//
//
///////// Perform an HTTP Post to an IIS ////////////////////////////////
// i)  Call CInternetThread.SetProxyServer(szProxy) to set the Proxy if any exists.
// ii) Call CInternetThread.SetIISServer(szIISServer) to set the IP Address (URL)
//	   of the Internet Server.
// iii)Call CInternetThread.InternetConnectivityExists() which will return TRUE
//     if connectivity to the ISS (szIISServer in 1) exists, else FALSE.
// iv) Call CInternetThread.SetBuffer(szBuffer) to set the Data that has to be
//	   POSTed of the Internet Server.
// v)  Call CInternetThread.PostData() which will return TRUE, if the Data has been
//	   POSTed successfully to the IIS.
//
//




