#ifndef __TCP_h__
#define __TCP_h__
/**************************************************************************

   File:          icw.h
   
   Description:   

**************************************************************************/
// === Structures Required by the ICMP.DLL ====================================

typedef struct {
   unsigned char Ttl;                                           // Time To Live
   unsigned char Tos;                                        // Type Of Service
   unsigned char Flags;                                      // IP header flags
   unsigned char OptionsSize;                  // Size in bytes of options data
   unsigned char *OptionsData;                       // Pointer to options data
} IP_OPTION_INFORMATION, * PIP_OPTION_INFORMATION;


typedef struct {
   DWORD Address;                                           // Replying address
   unsigned long  Status;                                       // Reply status
   unsigned long  RoundTripTime;                         // RTT in milliseconds
   unsigned short DataSize;                                   // Echo data size
   unsigned short Reserved;                          // Reserved for system use
   void *Data;                                      // Pointer to the echo data
   IP_OPTION_INFORMATION Options;                              // Reply options
} IP_ECHO_REPLY, * PIP_ECHO_REPLY;


typedef	HANDLE (WINAPI *ICMPCREATEFILE)(VOID);
typedef	BOOL   (WINAPI *ICMPCLOSEHANDLE)(HANDLE);
typedef	DWORD  (WINAPI *ICMPSENDECHO )( 
									HANDLE, DWORD, LPVOID, WORD,
                                    PIP_OPTION_INFORMATION, LPVOID, 
                                    DWORD, DWORD 
									   );

#ifdef __cplusplus
extern "C" 
{
#endif
	DWORD  PingHost();
	BOOL Ping(LPSTR szIPAddress);
	BOOL CheckHostName(LPSTR szIISServer);

#ifdef __cplusplus
}
#endif	

#endif	// __TCP_H__
