#ifndef WINDNS_INTERNAL_H
#define WINDNS_INTERNAL_H

#include "adns.h"

typedef struct
{
    adns_state State;
} WINDNS_CONTEXT, *PWINDNS_CONTEXT;

DNS_STATUS DnsIntTranslateAdnsToDNS_STATUS(int Status);
void DnsIntFreeRecordList(PDNS_RECORD ToFree);

#endif//WINDNS_INTERNAL_H
