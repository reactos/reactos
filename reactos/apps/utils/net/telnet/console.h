#ifndef _APPS_NET_TELNET_CONSOLE_H
#define _APPS_NET_TELNET_CONSOLE_H 
void console_title_connecting (
	char const* pszHostName,
	const int nPort
	);
void console_title_connected (
	char const* pszHostName,
	const int nPort
	);
void console_title_not_connected (void);
#endif /* _APPS_NET_TELNET_CONSOLE_H */
