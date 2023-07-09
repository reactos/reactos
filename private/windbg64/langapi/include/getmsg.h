// getmsg.h

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

#ifndef __GETMSG_H__
#define __GETMSG_H__

char *  get_err(int);
int SetErrorFile(char *szFilename, char *szExeName, int fSearchExePath);
long SetHInstace(long hInstModule);

#endif __GETMSG_H__
