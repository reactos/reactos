/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/dnsapi/dnsapi/query.c
 * PURPOSE:     DNSAPI functions built on the ADNS library.
 * PROGRAMER:   Art Yerkes
 * UPDATE HISTORY:
 *              12/15/03 -- Created
 */

#include <windows.h>
#include <WinError.h>
#include <WinDNS.h>
#include <internal/windns.h>
#include <string.h>

/* DnsQuery ****************************
 * Begin a DNS query, and allow the result to be placed in the application
 * supplied result pointer.  The result can be manipulated with the record
 * functions.  
 *
 * Name                 -- The DNS object to be queried.
 * Type                 -- The type of records to be returned.  These are 
 *                          listed in windns.h
 * Options              -- Query options.  DNS_QUERY_STANDARD is the base
 *                          state, and every other option takes precedence.
 *                          multiple options can be combined.  Listed in
 *                          windns.h
 * Servers              -- List of alternate servers (optional)
 * QueryResultSet       -- Pointer to the result pointer that will be filled
 *                          when the response is available.
 * Reserved             -- Response as it appears on the wire.  Optional.
 */

char *xstrsave(const char *str) {
  char *p;
  
  p= RtlAllocateHeap( RtlGetProcessHeap(), 0, strlen(str)+1 );
  strcpy(p,str);
  return p;
}

DNS_STATUS WINAPI DnsQuery_A
( LPCSTR Name,
  WORD Type,
  DWORD Options,
  PIP4_ARRAY Servers,
  PDNS_RECORDA *QueryResultSet,
  PVOID *Reserved ) {
  adns_state astate;
  int quflags = 0;
  int adns_error;
  adns_answer *answer;

  *QueryResultSet = 0;

  switch( Type ) {
  case DNS_TYPE_A:
    adns_error = adns_init( &astate,
			    adns_if_noenv |
			    adns_if_noerrprint |
			    adns_if_noserverwarn,
			    0 );
    if( adns_error != adns_s_ok ) {
      adns_finish( astate );
      return DnsIntTranslateAdnsToDNS_STATUS( adns_error );
    }

    adns_error = adns_synchronous( astate, 
				   Name, 
				   adns_r_addr, 
				   quflags, 
				   &answer );
				   
    if( adns_error != adns_s_ok ) {
      adns_finish( astate );
      return DnsIntTranslateAdnsToDNS_STATUS( adns_error );
    }

    *QueryResultSet = (PDNS_RECORDA)RtlAllocateHeap( RtlGetProcessHeap(), 0, 
						     sizeof( DNS_RECORDA ) );
    (*QueryResultSet)->wType = Type;
    (*QueryResultSet)->pName = xstrsave( Name );
    (*QueryResultSet)->wDataLength = sizeof(DNS_A_DATA);
    (*QueryResultSet)->Data.A.IpAddress = 
      answer->rrs.addr->addr.inet.sin_addr.s_addr;
    adns_finish( astate );
    return ERROR_SUCCESS;
    
  default:
    return DNS_ERROR_NO_MEMORY; /* XXX arty: find a better error code. */
  }
}

static PCHAR DnsWToC( const WCHAR *WideString ) {
  int chars = wcstombs( NULL, WideString, 0 );
  PCHAR out = RtlAllocateHeap( RtlGetProcessHeap(), 0, chars + 1 );
  wcstombs( out, WideString, chars + 1 );
  return out;
}

static PWCHAR DnsCToW( const CHAR *NarrowString ) {
  int chars = mbstowcs( NULL, NarrowString, 0 );
  PWCHAR out = RtlAllocateHeap( RtlGetProcessHeap(), 0, 
				(chars + 1) * sizeof(WCHAR) );
  mbstowcs( out, NarrowString, chars + 1 );
  return out;
}

DNS_STATUS WINAPI DnsQuery_W
( LPCWSTR Name,
  WORD Type,
  DWORD Options,
  PIP4_ARRAY Servers,
  PDNS_RECORDW *QueryResultSet,
  PVOID *Reserved ) {
  int i;
  PCHAR Buffer;
  DNS_STATUS Status;
  PDNS_RECORDA QueryResultWide;
  PDNS_RECORDW ConvertedRecord = 0, LastRecord = 0;

  Buffer = DnsWToC( Name );

  Status = DnsQuery_A( Buffer, Type, Options, Servers, &QueryResultWide,
		       Reserved );

  while( Status == ERROR_SUCCESS && QueryResultWide ) {
    switch( QueryResultWide->wType ) {
    case DNS_TYPE_A:
    case DNS_TYPE_WKS:
    case DNS_TYPE_AAAA:
    case DNS_TYPE_KEY:
      ConvertedRecord = RtlAllocateHeap( RtlGetProcessHeap(), 0, 
					 sizeof(DNS_RECORDA) );
      ConvertedRecord->pName = DnsCToW( QueryResultWide->pName );
      ConvertedRecord->wType = QueryResultWide->wType;
      ConvertedRecord->wDataLength = QueryResultWide->wDataLength;
      memcpy( ConvertedRecord, QueryResultWide, 
	      QueryResultWide->wDataLength );
      break;

    case DNS_TYPE_CNAME:
    case DNS_TYPE_PTR:
    case DNS_TYPE_NS:
    case DNS_TYPE_MB:
    case DNS_TYPE_MD:
    case DNS_TYPE_MF:
    case DNS_TYPE_MG:
    case DNS_TYPE_MR:
      ConvertedRecord = RtlAllocateHeap( RtlGetProcessHeap(), 0, 
					 sizeof(DNS_RECORDA) );
      ConvertedRecord->pName = DnsCToW( QueryResultWide->pName );
      ConvertedRecord->wType = QueryResultWide->wType;
      ConvertedRecord->wDataLength = sizeof(DNS_PTR_DATAA);
      ConvertedRecord->Data.PTR.pNameHost = 
	DnsCToW( QueryResultWide->Data.PTR.pNameHost );
      break;
    case DNS_TYPE_MINFO:
    case DNS_TYPE_RP:
      ConvertedRecord = RtlAllocateHeap( RtlGetProcessHeap(), 0, 
					 sizeof(DNS_RECORDA) );
      ConvertedRecord->pName = DnsCToW( QueryResultWide->pName );
      ConvertedRecord->wType = QueryResultWide->wType;
      ConvertedRecord->wDataLength = sizeof(DNS_MINFO_DATAA);
      ConvertedRecord->Data.MINFO.pNameMailbox =
	DnsCToW( QueryResultWide->Data.MINFO.pNameMailbox );
      ConvertedRecord->Data.MINFO.pNameErrorsMailbox =
	DnsCToW( QueryResultWide->Data.MINFO.pNameErrorsMailbox );
      break;

    case DNS_TYPE_MX:
    case DNS_TYPE_AFSDB:
    case DNS_TYPE_RT:
      ConvertedRecord = RtlAllocateHeap( RtlGetProcessHeap(), 0, 
					 sizeof(DNS_RECORDA) );
      ConvertedRecord->pName = DnsCToW( QueryResultWide->pName );
      ConvertedRecord->wType = QueryResultWide->wType;
      ConvertedRecord->wDataLength = sizeof(DNS_MX_DATAW);
      ConvertedRecord->Data.MX.pNameExchange = 
	DnsCToW( QueryResultWide->Data.MX.pNameExchange );
      ConvertedRecord->Data.MX.wPreference =
	QueryResultWide->Data.MX.wPreference;
      break;

    case DNS_TYPE_TXT:
    case DNS_TYPE_HINFO:
    case DNS_TYPE_ISDN:
      ConvertedRecord = RtlAllocateHeap( RtlGetProcessHeap(), 0, 
					 sizeof(DNS_TXT_DATAW) + 
					 QueryResultWide->
					 Data.TXT.dwStringCount );
      ConvertedRecord->pName = DnsCToW( QueryResultWide->pName );
      ConvertedRecord->wType = QueryResultWide->wType;
      ConvertedRecord->wDataLength = 
	sizeof(DNS_TXT_DATAW) + 
	(sizeof(PWCHAR) * QueryResultWide->Data.TXT.dwStringCount);
      ConvertedRecord->Data.TXT.dwStringCount = 
	QueryResultWide->Data.TXT.dwStringCount;
      for( i = 0; i < ConvertedRecord->Data.TXT.dwStringCount; i++ ) {
	ConvertedRecord->Data.TXT.pStringArray[i] = 
	  DnsCToW( QueryResultWide->Data.TXT.pStringArray[i] );
      }
      break;

    case DNS_TYPE_NULL:
      ConvertedRecord = RtlAllocateHeap( RtlGetProcessHeap(), 0, 
					 sizeof(DNS_NULL_DATA) + 
					 QueryResultWide->
					 Data.Null.dwByteCount );
      ConvertedRecord->pName = DnsCToW( QueryResultWide->pName );
      ConvertedRecord->wType = QueryResultWide->wType;
      ConvertedRecord->wDataLength =
	sizeof(DNS_NULL_DATA) + QueryResultWide->Data.Null.dwByteCount;
      ConvertedRecord->Data.Null.dwByteCount = 
	QueryResultWide->Data.Null.dwByteCount;
      memcpy( &ConvertedRecord->Data.Null.Data, 
	      &QueryResultWide->Data.Null.Data,
	      QueryResultWide->Data.Null.dwByteCount );
      break;

    case DNS_TYPE_SIG:
      ConvertedRecord = RtlAllocateHeap( RtlGetProcessHeap(), 0, 
					 sizeof(DNS_RECORDA) );
      ConvertedRecord->pName = DnsCToW( QueryResultWide->pName );
      ConvertedRecord->wType = QueryResultWide->wType;
      ConvertedRecord->wDataLength = sizeof(DNS_SIG_DATAA);
      memcpy( &ConvertedRecord->Data.SIG,
	      &QueryResultWide->Data.SIG,
	      sizeof(QueryResultWide->Data.SIG) );
      ConvertedRecord->Data.SIG.pNameSigner =
	DnsCToW( QueryResultWide->Data.SIG.pNameSigner );
      break;

    case DNS_TYPE_NXT:
      ConvertedRecord = RtlAllocateHeap( RtlGetProcessHeap(), 0, 
					 sizeof(DNS_RECORDA) );
      ConvertedRecord->pName = DnsCToW( QueryResultWide->pName );
      ConvertedRecord->wType = QueryResultWide->wType;
      ConvertedRecord->wDataLength = sizeof(DNS_NXT_DATAA);
      memcpy( &ConvertedRecord->Data.NXT,
	      &QueryResultWide->Data.NXT,
	      sizeof(QueryResultWide->Data.NXT) );
      ConvertedRecord->Data.NXT.pNameNext =
	DnsCToW( QueryResultWide->Data.NXT.pNameNext );
      break;

    case DNS_TYPE_SRV:
      ConvertedRecord = RtlAllocateHeap( RtlGetProcessHeap(), 0, 
					 sizeof(DNS_RECORDA) );
      ConvertedRecord->pName = DnsCToW( QueryResultWide->pName );
      ConvertedRecord->wType = QueryResultWide->wType;
      ConvertedRecord->wDataLength = sizeof(DNS_SRV_DATAA);
      memcpy( &ConvertedRecord->Data.SRV,
	      &QueryResultWide->Data.SRV,
	      sizeof(QueryResultWide->Data.SRV) );
      ConvertedRecord->Data.SRV.pNameTarget =
	DnsCToW( QueryResultWide->Data.SRV.pNameTarget );
      break;
    }

    if( LastRecord ) {
      LastRecord->pNext = ConvertedRecord;
      LastRecord = LastRecord->pNext;
    } else {
      LastRecord = *QueryResultSet = ConvertedRecord;
    }
  }

  LastRecord->pNext = 0;

  /* The name */
  RtlFreeHeap( RtlGetProcessHeap(), 0, Buffer );

  return Status;
}

DNS_STATUS WINAPI DnsQuery_UTF8
( LPCSTR Name,
  WORD Type,
  DWORD Options,
  PIP4_ARRAY Servers,
  PDNS_RECORDA *QueryResultSet,
  PVOID *Reserved ) {
  return DnsQuery_UTF8( Name, Type, Options, Servers, QueryResultSet, 
			Reserved );
}

void DnsIntFreeRecordList( PDNS_RECORDA ToDelete ) {
  int i;
  PDNS_RECORDA next = 0;

  while( ToDelete ) {
    if( ToDelete->pName ) 
      RtlFreeHeap( RtlGetProcessHeap(), 0, ToDelete->pName );
    switch( ToDelete->wType ) {
    case DNS_TYPE_CNAME:
    case DNS_TYPE_PTR:
    case DNS_TYPE_NS:
    case DNS_TYPE_MB:
    case DNS_TYPE_MD:
    case DNS_TYPE_MF:
    case DNS_TYPE_MG:
    case DNS_TYPE_MR:
      RtlFreeHeap( RtlGetProcessHeap(), 0, ToDelete->Data.PTR.pNameHost );
      break;
    case DNS_TYPE_MINFO:
    case DNS_TYPE_RP:
      RtlFreeHeap( RtlGetProcessHeap(), 0, 
		   ToDelete->Data.MINFO.pNameMailbox );
      RtlFreeHeap( RtlGetProcessHeap(), 0,
		   ToDelete->Data.MINFO.pNameErrorsMailbox );
      break;

    case DNS_TYPE_MX:
    case DNS_TYPE_AFSDB:
    case DNS_TYPE_RT:
      RtlFreeHeap( RtlGetProcessHeap(), 0, ToDelete->Data.MX.pNameExchange );
      break;

    case DNS_TYPE_TXT:
    case DNS_TYPE_HINFO:
    case DNS_TYPE_ISDN:
      for( i = 0; i < ToDelete->Data.TXT.dwStringCount; i++ ) {
	RtlFreeHeap( RtlGetProcessHeap(), 0, 
		     ToDelete->Data.TXT.pStringArray[i] );
      }
      RtlFreeHeap( RtlGetProcessHeap(), 0, ToDelete->Data.TXT.pStringArray );
      break;

    case DNS_TYPE_SIG:
      RtlFreeHeap( RtlGetProcessHeap(), 0, ToDelete->Data.SIG.pNameSigner );
      break;

    case DNS_TYPE_NXT:
      RtlFreeHeap( RtlGetProcessHeap(), 0, ToDelete->Data.NXT.pNameNext );
      break;

    case DNS_TYPE_SRV:
      RtlFreeHeap( RtlGetProcessHeap(), 0, ToDelete->Data.SRV.pNameTarget );
      break;
    }

    next = ToDelete->pNext;
    RtlFreeHeap( RtlGetProcessHeap(), 0, ToDelete );
    ToDelete = next;
  }
}
