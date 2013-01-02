#ifndef _FBT_HW_H_
#define _FBT_HW_H_

#include <winioctl.h>

// HW Driver Abstraction layer
class CBTHW
{
public:
	// The driver is opened for OVERLAPPED I/O
    CBTHW();
    virtual ~CBTHW();

	// Set the driver instances symbolic name
    void    SetDeviceName(LPCTSTR szDeviceName);
    DWORD	GetDeviceName(LPTSTR szBuffer, DWORD dwBufferSize);

	// Open a handle to the driver instance
    virtual DWORD   Attach(LPCSTR szDeviceName);
    virtual DWORD   Detach();
    HANDLE  GetDriverHandle();
    BOOL    IsAttached() {return GetDriverHandle()!=INVALID_HANDLE_VALUE;}

	// Send a command to the driver
    DWORD	SendCommand(DWORD dwCommand, LPVOID lpInBuffer=NULL, DWORD dwInBufferSize=0, LPVOID lpOutBuffer=NULL, DWORD dwOutBufferSize=0, OVERLAPPED *pOverlapped=NULL);
	DWORD	SendData(LPVOID lpBuffer, DWORD dwBufferSize, DWORD *dwBytesSent, OVERLAPPED *pOverlapped);
	DWORD	GetData(LPVOID lpBuffer, DWORD dwBufferSize, DWORD *dwBytesRead, OVERLAPPED *pOverlapped);

protected:
    HANDLE  m_hDriver;
    TCHAR   m_szDeviceName[1024];

};


#endif // _FBT_HW_H_
