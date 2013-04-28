/* FTP.h
 *
 * Copyright (c) 1996-2001 Mike Gleason, NCEMRSoft.
 * All rights reserved.
 *
 */

/* FTP.c */
void MyInetAddr(char *, size_t, char **, int);
int GetOurHostName(char *, size_t);
void CloseControlConnection(const FTPCIPtr);
int SetKeepAlive(const FTPCIPtr, int);
int SetLinger(const FTPCIPtr, int, int);
int SetTypeOfService(const FTPCIPtr, int, int);
int SetInlineOutOfBandData(const FTPCIPtr, int);
int OpenControlConnection(const FTPCIPtr, char *, unsigned int);
void CloseDataConnection(const FTPCIPtr);
int SetStartOffset(const FTPCIPtr, longest_int);
int OpenDataConnection(const FTPCIPtr, int);
int AcceptDataConnection(const FTPCIPtr);
void HangupOnServer(const FTPCIPtr);
void SendTelnetInterrupt(const FTPCIPtr);
