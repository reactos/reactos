#ifndef _IPHLP_RES_H
#define _IPHLP_RES_H

typedef struct _IPHLP_RES_INFO {
    DWORD riCount;
    LPSOCKADDR riAddressList;
} IPHLP_RES_INFO, *PIPHLP_RES_INFO;

PIPHLP_RES_INFO getResInfo();
VOID disposeResInfo( PIPHLP_RES_INFO InfoPtr );

#endif/*_IPHLP_RES_H*/
