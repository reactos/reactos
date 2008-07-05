/* usio.h */

#define kUNewFailed (-9)
#define kUBindFailed (-10)
#define kUListenFailed (-11)

/* Don't change the following line -- it is modified by the Configure script. */
#define UAccept UAcceptS

#ifndef UAccept
#	if defined(NO_SIGNALS) || defined(WIN32) || defined(_WINDOWS)
#		define UAccept UAcceptS
#	else
#		define UAccept UAcceptA
#	endif
#endif

/* UAcceptA.c */
int UAcceptA(int, struct sockaddr_un *const, int *, int);

/* UAcceptS.c */
int UAcceptS(int, struct sockaddr_un *const, int *, int);

/* UBind.c */
int UBind(int, const char *const, const int, const int);
int UListen(int, int);

/* UConnect.c */
int UConnect(int, const struct sockaddr_un *const, int, int);

/* UConnectByName.c */
int UConnectByName(int, const char *const, const int);

/* UNew.c */
int MakeSockAddrUn(struct sockaddr_un *, const char *const);
int UNewStreamClient(void);
int UNewDatagramClient(void);
int UNewStreamServer(const char *const, const int, const int, int);
int UNewDatagramServer(const char *const, const int, const int);

/* URecvfrom.c */
int URecvfrom(int, char *const, size_t, int, struct sockaddr_un *const, int *, int);

/* USendto.c */
int USendto(int, const char *const, size_t, int, const struct sockaddr_un *const, int, int);

/* USendtoByName.c */
int USendtoByName(int, const char *const, size_t, int, const char *const, int);
