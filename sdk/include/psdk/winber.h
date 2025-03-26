/*
  winber.h - Header file for the Windows LDAP Basic Encoding Rules API

  Written by Filip Navara <xnavara@volny.cz>

  References:
    The C LDAP Application Program Interface
    http://www.watersprings.org/pub/id/draft-ietf-ldapext-ldap-c-api-05.txt

    Lightweight Directory Access Protocol Reference
    http://msdn.microsoft.com/library/en-us/netdir/ldap/ldap_reference.asp (DEAD_LINK)

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef _WINBER_H
#define _WINBER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINBERAPI
#define WINBERAPI DECLSPEC_IMPORT
#endif

typedef struct berelement BerElement;
typedef ULONG ber_len_t;
#include <pshpack4.h>
typedef struct berval {
	ber_len_t bv_len;
	char *bv_val;
} BerValue, LDAP_BERVAL, *PLDAP_BERVAL, BERVAL, *PBERVAL;
#include <poppack.h>

typedef ULONG ber_tag_t;
typedef INT ber_int_t;
typedef UINT ber_uint_t;
typedef INT ber_slen_t;

#define LBER_ERROR ((ber_tag_t)-1)
#define LBER_DEFAULT ((ber_tag_t)-1)
#define LBER_USE_DER 0x01

WINBERAPI BerElement *ber_init(const BerValue*);
WINBERAPI int ber_printf(BerElement*,const char*,...);
WINBERAPI int ber_flatten(BerElement*,BerValue**);
WINBERAPI ber_tag_t ber_scanf(BerElement*,const char*,...);
WINBERAPI ber_tag_t ber_peek_tag(BerElement*,ber_len_t*);
WINBERAPI ber_tag_t ber_skip_tag(BerElement*,ber_len_t*);
WINBERAPI ber_tag_t ber_first_element(BerElement*,ber_len_t*,char**);
WINBERAPI ber_tag_t ber_next_element(BerElement*,ber_len_t*,char*);
WINBERAPI void ber_bvfree(BerValue*);
WINBERAPI void ber_bvecfree(BerValue**);
WINBERAPI void ber_free(BerElement*,int);
WINBERAPI BerValue *ber_bvdup(BerValue*);
WINBERAPI BerElement *ber_alloc_t(int);

#ifdef __cplusplus
}
#endif
#endif /* _WINBER_H */
