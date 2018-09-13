/*****************************************************************************
 *
 *	ftpinet.h - Wrapper for WININET stuff
 *
 *****************************************************************************/

#ifndef _FTPINET_H
#define _FTPINET_H


HINTERNET GetWininetSessionHandle(void);
void UnloadWininet(void);


#endif // _FTPINET_H
