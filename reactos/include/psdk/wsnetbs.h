#ifndef _WSNETBS_H
#define _WSNETBS_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#define NETBIOS_NAME_LENGTH	16
#define NETBIOS_UNIQUE_NAME	0
#define NETBIOS_GROUP_NAME	1
#define NETBIOS_TYPE_QUICK_UNIQUE	2
#define NETBIOS_TYPE_QUICK_GROUP	3

#ifndef RC_INVOKED
typedef struct sockaddr_nb {
	short	snb_family;
	u_short	snb_type;
	char	snb_name[NETBIOS_NAME_LENGTH];
} SOCKADDR_NB, *PSOCKADDR_NB, *LPSOCKADDR_NB;
#define SET_NETBIOS_SOCKADDR(_snb,_type,_name,_port) \
{ \
	register int _i; \
	register char *_n = (_name); \
	register PSOCKADDR_NB _s = (_snb); \
	_s->snb_family = AF_NETBIOS; \
	_s->snb_type = (_type); \
	for (_i=0; _n[_i] != '\0' && _i<NETBIOS_NAME_LENGTH-1; _i++) { \
		_s->snb_name[_i] = _n[_i]; \
	} \
	for (; _i<NETBIOS_NAME_LENGTH-1; _i++) { \
		_s->snb_name[_i] = ' '; \
	} \
	_s->snb_name[NETBIOS_NAME_LENGTH-1] = (_port); \
}
#endif   /* RC_INVOKED */
#endif
