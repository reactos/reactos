#ifndef _APPS_NET_TELNET_H
#define _APPS_NET_TELNET_H
//Global Handles
extern HANDLE StandardInput;
extern HANDLE StandardOutput;
extern HANDLE StandardError;

extern char const* sockmsg(int ecode);
extern void err(char const* s,...);

extern void telnet(char const* pszHostName,const short nPort);
extern void vm(SOCKET,unsigned char);

// terminal handlers:
void ansi(SOCKET server,unsigned char data);
void nvt(SOCKET server,unsigned char data);

// helpsock
char const* sockmsg(int ecode);

// command shell
int shell(char const* pszHostName,const int nPort);

#endif /* ndef _APPS_NET_TELNET_H */
