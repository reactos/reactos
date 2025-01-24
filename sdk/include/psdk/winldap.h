/*
  winldap.h - Header file for the Windows LDAP API

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

#ifndef _WINLDAP_H
#define _WINLDAP_H

#ifndef _SCHANNEL_H
#include <schannel.h>
#endif

#ifndef _WINBER_H
#include <winber.h>
#endif

#ifndef WINLDAPAPI
#define WINLDAPAPI DECLSPEC_IMPORT
#endif

#ifndef _WINLDAP_DEPRECATED
#if (( __GNUC__ == 3 && __GNUC_MINOR__ > 0 ) || __GNUC__ > 3)
#define _WINLDAP_DEPRECATED __attribute__((__deprecated__))
#else
#define _WINLDAP_DEPRECATED
#endif
#endif

#include <pshpack4.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LDAP_VERSION1	1
#define LDAP_VERSION2	2
#define LDAP_VERSION3	3
#define LDAP_VERSION	LDAP_VERSION2
#define LDAP_API_VERSION	2004
#define LDAP_VERSION_MIN	LDAP_VERSION2
#define LDAP_VERSION_MAX	LDAP_VERSION3
#define LDAP_VENDOR_NAME	"Microsoft Corporation."
#define LDAP_VENDOR_NAME_W	L"Microsoft Corporation."
#define LDAP_VENDOR_VERSION	510
#define LDAP_API_INFO_VERSION	1
#define LDAP_FEATURE_INFO_VERSION	1
#define LDAP_SUCCESS	0x00
#define LDAP_OPERATIONS_ERROR	0x01
#define LDAP_PROTOCOL_ERROR	0x02
#define LDAP_TIMELIMIT_EXCEEDED	0x03
#define LDAP_SIZELIMIT_EXCEEDED	0x04
#define LDAP_COMPARE_FALSE	0x05
#define LDAP_COMPARE_TRUE	0x06
#define LDAP_STRONG_AUTH_NOT_SUPPORTED	0x07
#define LDAP_STRONG_AUTH_REQUIRED	0x08
#define LDAP_REFERRAL_V2	0x09
#define LDAP_REFERRAL	0x0a
#define LDAP_ADMIN_LIMIT_EXCEEDED	0x0b
#define LDAP_UNAVAILABLE_CRIT_EXTENSION	0x0c
#define LDAP_CONFIDENTIALITY_REQUIRED	0x0d
#define LDAP_SASL_BIND_IN_PROGRESS	0x0e
#define LDAP_NO_SUCH_ATTRIBUTE	0x10
#define LDAP_UNDEFINED_TYPE	0x11
#define LDAP_INAPPROPRIATE_MATCHING	0x12
#define LDAP_CONSTRAINT_VIOLATION	0x13
#define LDAP_TYPE_OR_VALUE_EXISTS	0x14
#define LDAP_INVALID_SYNTAX	0x15
#define LDAP_NO_SUCH_OBJECT	0x20
#define LDAP_ALIAS_PROBLEM	0x21
#define LDAP_INVALID_DN_SYNTAX	0x22
#define LDAP_IS_LEAF	0x23
#define LDAP_ALIAS_DEREF_PROBLEM	0x24
#define LDAP_INAPPROPRIATE_AUTH	0x30
#define LDAP_INVALID_CREDENTIALS	0x31
#define LDAP_INSUFFICIENT_ACCESS	0x32
#define LDAP_BUSY	0x33
#define LDAP_UNAVAILABLE	0x34
#define LDAP_UNWILLING_TO_PERFORM	0x35
#define LDAP_LOOP_DETECT	0x36
#define LDAP_NAMING_VIOLATION	0x40
#define LDAP_OBJECT_CLASS_VIOLATION	0x41
#define LDAP_NOT_ALLOWED_ON_NONLEAF	0x42
#define LDAP_NOT_ALLOWED_ON_RDN	0x43
#define LDAP_ALREADY_EXISTS	0x44
#define LDAP_NO_OBJECT_CLASS_MODS	0x45
#define LDAP_RESULTS_TOO_LARGE	0x46
#define LDAP_AFFECTS_MULTIPLE_DSAS	0x47
#define LDAP_OTHER	0x50
#define LDAP_SERVER_DOWN	0x51
#define LDAP_LOCAL_ERROR	0x52
#define LDAP_ENCODING_ERROR	0x53
#define LDAP_DECODING_ERROR	0x54
#define LDAP_TIMEOUT	0x55
#define LDAP_AUTH_UNKNOWN	0x56
#define LDAP_FILTER_ERROR	0x57
#define LDAP_USER_CANCELLED	0x58
#define LDAP_PARAM_ERROR	0x59
#define LDAP_NO_MEMORY	0x5a
#define LDAP_CONNECT_ERROR	0x5b
#define LDAP_NOT_SUPPORTED	0x5c
#define LDAP_CONTROL_NOT_FOUND	0x5d
#define LDAP_NO_RESULTS_RETURNED	0x5e
#define LDAP_MORE_RESULTS_TO_RETURN	0x5f
#define LDAP_CLIENT_LOOP	0x60
#define LDAP_REFERRAL_LIMIT_EXCEEDED	0x61
#define LDAP_OPT_SUCCESS	LDAP_SUCCESS
#define LDAP_AUTH_METHOD_NOT_SUPPORTED	LDAP_STRONG_AUTH_NOT_SUPPORTED
#define LDAP_ATTRIBUTE_OR_VALUE_EXISTS	LDAP_TYPE_OR_VALUE_EXISTS
#define LDAP_INSUFFICIENT_RIGHTS	LDAP_INSUFFICIENT_ACCESS
#define LDAP_PARTIAL_RESULTS	LDAP_REFERRAL_V2
#define LDAP_PORT	389
#define LDAP_SSL_PORT	636
#define LDAP_GC_PORT	3268
#define LDAP_SSL_GC_PORT	3269
#define LDAP_OPT_ON	((void*)1)
#define LDAP_OPT_OFF	((void*)0)
#define LDAP_OPT_API_INFO	0x00
#define LDAP_OPT_DESC	0x01
#define LDAP_OPT_DEREF	0x02
#define LDAP_OPT_SIZELIMIT	0x03
#define LDAP_OPT_TIMELIMIT	0x04
#define LDAP_OPT_THREAD_FN_PTRS	0x05
#define LDAP_OPT_REBIND_FN	0x06
#define LDAP_OPT_REBIND_ARG	0x07
#define LDAP_OPT_REFERRALS	0x08
#define LDAP_OPT_RESTART	0x09
#define LDAP_OPT_SSL	0x0a
#define LDAP_OPT_IO_FN_PTRS	0x0b
#define LDAP_OPT_CACHE_FN_PTRS	0x0d
#define LDAP_OPT_CACHE_STRATEGY	0x0e
#define LDAP_OPT_CACHE_ENABLE	0x0f
#define LDAP_OPT_REFERRAL_HOP_LIMIT	0x10
#define LDAP_OPT_PROTOCOL_VERSION	0x11
#define LDAP_OPT_SERVER_CONTROLS	0x12
#define LDAP_OPT_CLIENT_CONTROLS	0x13
#define LDAP_OPT_API_FEATURE_INFO	0x15
#define LDAP_OPT_HOST_NAME	0x30
#define LDAP_OPT_ERROR_NUMBER	0x31
#define LDAP_OPT_ERROR_STRING	0x32
#define LDAP_OPT_SERVER_ERROR	0x33
#define LDAP_OPT_SERVER_EXT_ERROR	0x34
#define LDAP_OPT_PING_KEEP_ALIVE	0x36
#define LDAP_OPT_PING_WAIT_TIME	0x37
#define LDAP_OPT_PING_LIMIT	0x38
#define LDAP_OPT_DNSDOMAIN_NAME	0x3b
#define LDAP_OPT_GETDSNAME_FLAGS	0x3d
#define LDAP_OPT_HOST_REACHABLE	0x3e
#define LDAP_OPT_PROMPT_CREDENTIALS	0x3f
#define LDAP_OPT_TCP_KEEPALIVE	0x40
#define LDAP_OPT_REFERRAL_CALLBACK	0x70
#define LDAP_OPT_CLIENT_CERTIFICATE	0x80
#define LDAP_OPT_SERVER_CERTIFICATE	0x81
#define LDAP_OPT_AUTO_RECONNECT	0x91
#define LDAP_OPT_SSPI_FLAGS	0x92
#define LDAP_OPT_SSL_INFO	0x93
#define LDAP_OPT_REF_DEREF_CONN_PER_MSG	0x94
#define LDAP_OPT_SIGN	0x95
#define LDAP_OPT_ENCRYPT	0x96
#define LDAP_OPT_SASL_METHOD	0x97
#define LDAP_OPT_AREC_EXCLUSIVE	0x98
#define LDAP_OPT_SECURITY_CONTEXT	0x99
#define LDAP_OPT_ROOTDSE_CACHE	0x9a
#define LDAP_OPT_VERSION	LDAP_OPT_PROTOCOL_VERSION
#define LDAP_OPT_TLS	LDAP_OPT_SSL
#define LDAP_OPT_TLS_INFO	LDAP_OPT_SSL_INFO
#define LDAP_DEREF_NEVER	0x00
#define LDAP_DEREF_SEARCHING	0x01
#define LDAP_DEREF_FINDING	0x02
#define LDAP_DEREF_ALWAYS	0x03
#define LDAP_NO_LIMIT	0
#define LDAP_CONTROL_REFERRALS	"1.2.840.113556.1.4.616"
#define LDAP_CONTROL_REFERRALS_W	L"1.2.840.113556.1.4.616"
#define LDAP_CHASE_SUBORDINATE_REFERRALS	0x20U
#define LDAP_CHASE_EXTERNAL_REFERRALS	0x40U
#define LDAP_SCOPE_DEFAULT	-1
#define LDAP_SCOPE_BASE	0x0000
#define LDAP_SCOPE_ONELEVEL	0x0001
#define LDAP_SCOPE_SUBTREE	0x0002
#define LDAP_MOD_ADD	0x00
#define LDAP_MOD_DELETE	0x01
#define LDAP_MOD_REPLACE	0x02
#define LDAP_MOD_BVALUES	0x80
#define LDAP_RES_BIND	0x61
#define LDAP_RES_SEARCH_ENTRY	0x64
#define LDAP_RES_SEARCH_RESULT	0x65
#define LDAP_RES_MODIFY	0x67
#define LDAP_RES_ADD	0x69
#define LDAP_RES_DELETE	0x6b
#define LDAP_RES_MODRDN	0x6d
#define LDAP_RES_COMPARE	0x6f
#define LDAP_RES_SEARCH_REFERENCE	0x73
#define LDAP_RES_EXTENDED	0x78
#define LDAP_RES_ANY	(-1L)
#define LDAP_MSG_ONE	0x00
#define LDAP_MSG_ALL	0x01
#define LDAP_MSG_RECEIVED	0x02
#define LDAP_SERVER_SORT_OID	"1.2.840.113556.1.4.473"
#define LDAP_SERVER_SORT_OID_W	L"1.2.840.113556.1.4.473"
#define LDAP_SERVER_RESP_SORT_OID	"1.2.840.113556.1.4.474"
#define LDAP_SERVER_RESP_SORT_OID_W	L"1.2.840.113556.1.4.474"
#define LDAP_PAGED_RESULT_OID_STRING	"1.2.840.113556.1.4.319"
#define LDAP_PAGED_RESULT_OID_STRING_W	L"1.2.840.113556.1.4.319"
#define LDAP_CONTROL_VLVREQUEST	"2.16.840.1.113730.3.4.9"
#define LDAP_CONTROL_VLVREQUEST_W	L"2.16.840.1.113730.3.4.9"
#define LDAP_CONTROL_VLVRESPONSE	"2.16.840.1.113730.3.4.10"
#define LDAP_CONTROL_VLVRESPONSE_W	L"2.16.840.1.113730.3.4.10"
#define LDAP_START_TLS_OID	"1.3.6.1.4.1.1466.20037"
#define LDAP_START_TLS_OID_W	L"1.3.6.1.4.1.1466.20037"
#define LDAP_TTL_EXTENDED_OP_OID	"1.3.6.1.4.1.1466.101.119.1"
#define LDAP_TTL_EXTENDED_OP_OID_W	L"1.3.6.1.4.1.1466.101.119.1"
#define LDAP_AUTH_NONE	0x00U
#define LDAP_AUTH_SIMPLE	0x80U
#define LDAP_AUTH_SASL	0x83U
#define LDAP_AUTH_OTHERKIND	0x86U
#define LDAP_AUTH_EXTERNAL	(LDAP_AUTH_OTHERKIND | 0x20U)
#define LDAP_AUTH_SICILY	(LDAP_AUTH_OTHERKIND | 0x200U)
#define LDAP_AUTH_NEGOTIATE	(LDAP_AUTH_OTHERKIND | 0x400U)
#define LDAP_AUTH_MSN	(LDAP_AUTH_OTHERKIND | 0x800U)
#define LDAP_AUTH_NTLM	(LDAP_AUTH_OTHERKIND | 0x1000U)
#define LDAP_AUTH_DIGEST	(LDAP_AUTH_OTHERKIND | 0x4000U)
#define LDAP_AUTH_DPA	(LDAP_AUTH_OTHERKIND | 0x2000U)
#define LDAP_AUTH_SSPI	LDAP_AUTH_NEGOTIATE
#define LDAP_FILTER_AND	0xa0
#define LDAP_FILTER_OR	0xa1
#define LDAP_FILTER_NOT	0xa2
#define LDAP_FILTER_EQUALITY	0xa3
#define LDAP_FILTER_SUBSTRINGS	0xa4
#define LDAP_FILTER_GE	0xa5
#define LDAP_FILTER_LE	0xa6
#define LDAP_FILTER_APPROX	0xa8
#define LDAP_FILTER_EXTENSIBLE	0xa9
#define LDAP_FILTER_PRESENT	0x87
#define LDAP_SUBSTRING_INITIAL	0x80
#define LDAP_SUBSTRING_ANY	0x81
#define LDAP_SUBSTRING_FINAL	0x82

typedef struct ldap {
	char Reserved[76];
	PCHAR ld_host;
	ULONG ld_version;
	UCHAR ld_lberoptions;
	int ld_deref;
	int ld_timelimit;
	int ld_sizelimit;
	int ld_errno;
	PCHAR ld_matched;
	PCHAR ld_error;
} LDAP, *PLDAP;
typedef struct ldapmsg {
	ULONG lm_msgid;
	ULONG lm_msgtype;
	BerElement *lm_ber;
	struct ldapmsg *lm_chain;
	struct ldapmsg *lm_next;
	ULONG lm_time;
} LDAPMessage, *PLDAPMessage;
typedef struct l_timeval {
	LONG tv_sec;
	LONG tv_usec;
} LDAP_TIMEVAL, *PLDAP_TIMEVAL;
typedef struct ldapapiinfoA {
	int ldapai_info_version;
	int ldapai_api_version;
	int ldapai_protocol_version;
	char **ldapai_extensions;
	char *ldapai_vendor_name;
	int ldapai_vendor_version;
} LDAPAPIInfoA, *PLDAPAPIInfoA;
typedef struct ldapapiinfoW {
	int ldapai_info_version;
	int ldapai_api_version;
	int ldapai_protocol_version;
	PWCHAR *ldapai_extensions;
	PWCHAR ldapai_vendor_name;
	int ldapai_vendor_version;
} LDAPAPIInfoW, *PLDAPAPIInfoW;
typedef struct ldap_apifeature_infoA {
	int ldapaif_info_version;
	char *ldapaif_name;
	int ldapaif_version;
} LDAPAPIFeatureInfoA, *PLDAPAPIFeatureInfoA;
typedef struct ldap_apifeature_infoW {
	int ldapaif_info_version;
	PWCHAR ldapaif_name;
	int ldapaif_version;
} LDAPAPIFeatureInfoW, *PLDAPAPIFeatureInfoW;
typedef struct ldapcontrolA {
	PCHAR ldctl_oid;
	BerValue ldctl_value;
	BOOLEAN ldctl_iscritical;
} LDAPControlA, *PLDAPControlA;
typedef struct ldapcontrolW {
	PWCHAR ldctl_oid;
	BerValue ldctl_value;
	BOOLEAN ldctl_iscritical;
} LDAPControlW, *PLDAPControlW;
typedef union mod_vals_uA {
	PCHAR *modv_strvals;
	BerValue **modv_bvals;
} mod_vals_u_tA;
typedef union mod_vals_uW {
	PWCHAR *modv_strvals;
	BerValue **modv_bvals;
} mod_vals_u_tW;
typedef struct ldapmodA {
	ULONG mod_op;
	PCHAR mod_type;
	mod_vals_u_tA mod_vals;
} LDAPModA, *PLDAPModA;
typedef struct ldapmodW {
	ULONG mod_op;
	PWCHAR mod_type;
	mod_vals_u_tW mod_vals;
} LDAPModW, *PLDAPModW;

#define mod_values      mod_vals.modv_strvals
#define mod_bvalues     mod_vals.modv_bvals

typedef struct ldapsearch LDAPSearch, *PLDAPSearch;
typedef struct ldapsortkeyA {
	PCHAR sk_attrtype;
	PCHAR sk_matchruleoid;
	BOOLEAN sk_reverseorder;
} LDAPSortKeyA, *PLDAPSortKeyA;
typedef struct ldapsortkeyW {
	PWCHAR sk_attrtype;
	PWCHAR sk_matchruleoid;
	BOOLEAN sk_reverseorder;
} LDAPSortKeyW, *PLDAPSortKeyW;
typedef ULONG (__cdecl QUERYFORCONNECTION)(PLDAP,PLDAP,PWCHAR,PCHAR,ULONG,PVOID,PVOID,PLDAP*);
typedef BOOLEAN (__cdecl NOTIFYOFNEWCONNECTION)(PLDAP,PLDAP,PWCHAR,PCHAR,PLDAP,ULONG,PVOID,PVOID,ULONG);
typedef ULONG (__cdecl DEREFERENCECONNECTION)(PLDAP,PLDAP);
typedef BOOLEAN (__cdecl QUERYCLIENTCERT)(PLDAP,PSecPkgContext_IssuerListInfoEx,PCCERT_CONTEXT*);
typedef struct LdapReferralCallback {
	ULONG SizeOfCallbacks;
	QUERYFORCONNECTION *QueryForConnection;
	NOTIFYOFNEWCONNECTION *NotifyRoutine;
	DEREFERENCECONNECTION *DereferenceRoutine;
} LDAP_REFERRAL_CALLBACK, *PLDAP_REFERRAL_CALLBACK;
typedef struct ldapvlvinfo {
	int ldvlv_version;
	unsigned long ldvlv_before_count;
	unsigned long ldvlv_after_count;
	unsigned long ldvlv_offset;
	unsigned long ldvlv_count;
	BerValue *ldvlv_attrvalue;
	BerValue *ldvlv_context;
	void *ldvlv_extradata;
} LDAPVLVInfo;

/*
 * Under Microsoft WinLDAP the function ldap_error is only stub.
 * This macro uses LDAP structure to get error string and pass it to the user.
 */
#define ldap_perror(handle,message) printf("%s: %s\n", message, handle->ld_error);

WINLDAPAPI PLDAP ldap_initA(PCHAR,ULONG);
WINLDAPAPI PLDAP ldap_initW(PWCHAR,ULONG);
WINLDAPAPI PLDAP ldap_openA(const PCHAR,ULONG);
WINLDAPAPI PLDAP ldap_openW(const PWCHAR,ULONG);
WINLDAPAPI PLDAP cldap_openA(const PCHAR,ULONG);
WINLDAPAPI PLDAP cldap_openW(const PWCHAR,ULONG);
WINLDAPAPI ULONG ldap_connect(LDAP*,struct l_timeval*);
WINLDAPAPI PLDAP ldap_sslinitA(PCHAR,ULONG,int);
WINLDAPAPI PLDAP ldap_sslinitW(PWCHAR,ULONG,int);
WINLDAPAPI ULONG ldap_start_tls_sA(LDAP*,PLDAPControlA*,PLDAPControlA*);
WINLDAPAPI ULONG ldap_start_tls_sW(LDAP*,PLDAPControlW*,PLDAPControlW*);
WINLDAPAPI BOOLEAN ldap_stop_tls_s(LDAP*);
WINLDAPAPI ULONG ldap_get_optionA(LDAP*,int,void*);
WINLDAPAPI ULONG ldap_get_optionW(LDAP*,int,void*);
WINLDAPAPI ULONG ldap_set_optionA(LDAP*,int,const void*);
WINLDAPAPI ULONG ldap_set_optionW(LDAP*,int,const void*);
WINLDAPAPI ULONG ldap_control_freeA(LDAPControlA*);
WINLDAPAPI ULONG ldap_control_freeW(LDAPControlW*);
WINLDAPAPI ULONG ldap_controls_freeA(LDAPControlA**);
WINLDAPAPI ULONG ldap_controls_freeW(LDAPControlW**);
WINLDAPAPI ULONG ldap_free_controlsA(LDAPControlA**);
WINLDAPAPI ULONG ldap_free_controlsW(LDAPControlW**);
WINLDAPAPI ULONG ldap_sasl_bindA(LDAP*,const PCHAR, const PCHAR,const BERVAL*,PLDAPControlA*,PLDAPControlA*,int*);
WINLDAPAPI ULONG ldap_sasl_bindW(LDAP*,const PWCHAR, const PWCHAR,const BERVAL*,PLDAPControlW*,PLDAPControlW*,int*);
WINLDAPAPI ULONG ldap_sasl_bind_sA(LDAP*,const PCHAR,const PCHAR,const BERVAL*,PLDAPControlA*,PLDAPControlA*,PBERVAL*);
WINLDAPAPI ULONG ldap_sasl_bind_sW(LDAP*,const PWCHAR,const PWCHAR,const BERVAL*,PLDAPControlW*,PLDAPControlW*,PBERVAL*);
WINLDAPAPI ULONG ldap_simple_bindA(LDAP*,const PCHAR,const PCHAR);
WINLDAPAPI ULONG ldap_simple_bindW(LDAP*,const PWCHAR,const PCHAR);
WINLDAPAPI ULONG ldap_simple_bind_sA(LDAP*,const PCHAR,const PCHAR);
WINLDAPAPI ULONG ldap_simple_bind_sW(LDAP*,const PWCHAR,const PCHAR);
WINLDAPAPI ULONG ldap_bindA(LDAP*,const PCHAR,const PCHAR,ULONG) _WINLDAP_DEPRECATED;
WINLDAPAPI ULONG ldap_bindW(LDAP*,const PWCHAR,const PCHAR,ULONG) _WINLDAP_DEPRECATED;
WINLDAPAPI ULONG ldap_bind_sA(LDAP*,const PCHAR,const PCHAR,ULONG) _WINLDAP_DEPRECATED;
WINLDAPAPI ULONG ldap_bind_sW(LDAP*,const PWCHAR,const PCHAR,ULONG) _WINLDAP_DEPRECATED;
WINLDAPAPI ULONG ldap_unbind(LDAP*);
WINLDAPAPI ULONG ldap_unbind_s(LDAP*);
WINLDAPAPI ULONG ldap_search_extA(LDAP*,const PCHAR,ULONG,const PCHAR,PCHAR[],ULONG,PLDAPControlW*,PLDAPControlW*,ULONG,ULONG,ULONG*);
WINLDAPAPI ULONG ldap_search_extW(LDAP*,const PWCHAR,ULONG,const PWCHAR,PWCHAR[],ULONG,PLDAPControlW*,PLDAPControlW*,ULONG,ULONG,ULONG*);
WINLDAPAPI ULONG ldap_search_ext_sA(LDAP*,const PCHAR,ULONG,const PCHAR,PCHAR[],ULONG,PLDAPControlA*,PLDAPControlA*,struct l_timeval*,ULONG,LDAPMessage**);
WINLDAPAPI ULONG ldap_search_ext_sW(LDAP*,const PWCHAR,ULONG,const PWCHAR,PWCHAR[],ULONG,PLDAPControlW*,PLDAPControlW*,struct l_timeval*,ULONG,LDAPMessage**);
WINLDAPAPI ULONG ldap_searchA(LDAP*,const PCHAR,ULONG,const PCHAR,PCHAR[],ULONG);
WINLDAPAPI ULONG ldap_searchW(LDAP*,const PWCHAR,ULONG,const PWCHAR,PWCHAR[],ULONG);
WINLDAPAPI ULONG ldap_search_sA(LDAP*,const PCHAR,ULONG,const PCHAR,PCHAR[],ULONG,LDAPMessage**);
WINLDAPAPI ULONG ldap_search_sW(LDAP*,const PWCHAR,ULONG,const PWCHAR,PWCHAR[],ULONG,LDAPMessage**);
WINLDAPAPI ULONG ldap_search_stA(LDAP*,const PCHAR,ULONG,const PCHAR,PCHAR[],ULONG,struct l_timeval*,LDAPMessage**);
WINLDAPAPI ULONG ldap_search_stW(LDAP*,const PWCHAR,ULONG,const PWCHAR,PWCHAR[],ULONG,struct l_timeval*,LDAPMessage**);
WINLDAPAPI ULONG ldap_compare_extA(LDAP*,const PCHAR,const PCHAR,PCHAR,struct berval*,PLDAPControlA*,PLDAPControlA*,ULONG*);
WINLDAPAPI ULONG ldap_compare_extW(LDAP*,const PWCHAR,const PWCHAR,PWCHAR,struct berval*,PLDAPControlW*,PLDAPControlW*,ULONG*);
WINLDAPAPI ULONG ldap_compare_ext_sA(LDAP*,const PCHAR,const PCHAR,const PCHAR,struct berval*,PLDAPControlA*,PLDAPControlA*);
WINLDAPAPI ULONG ldap_compare_ext_sW(LDAP*,const PWCHAR,const PWCHAR,const PWCHAR,struct berval*,PLDAPControlW*,PLDAPControlW*);
WINLDAPAPI ULONG ldap_compareA(LDAP*,const PCHAR,const PCHAR,PCHAR);
WINLDAPAPI ULONG ldap_compareW(LDAP*,const PWCHAR,const PWCHAR,PWCHAR);
WINLDAPAPI ULONG ldap_compare_sA(LDAP*,const PCHAR,const PCHAR,PCHAR);
WINLDAPAPI ULONG ldap_compare_sW(LDAP*,const PWCHAR,const PWCHAR,PWCHAR);
WINLDAPAPI ULONG ldap_modify_extA(LDAP*,const PCHAR,LDAPModA*[],PLDAPControlA*,PLDAPControlA*,ULONG*);
WINLDAPAPI ULONG ldap_modify_extW(LDAP*,const PWCHAR,LDAPModW*[],PLDAPControlW*,PLDAPControlW*,ULONG*);
WINLDAPAPI ULONG ldap_modify_ext_sA(LDAP*,const PCHAR,LDAPModA*[],PLDAPControlA*,PLDAPControlA*);
WINLDAPAPI ULONG ldap_modify_ext_sW(LDAP*,const PWCHAR,LDAPModW*[],PLDAPControlW*,PLDAPControlW*);
WINLDAPAPI ULONG ldap_modifyA(LDAP*,const PCHAR,LDAPModA*[]);
WINLDAPAPI ULONG ldap_modifyW(LDAP*,const PWCHAR,LDAPModW*[]);
WINLDAPAPI ULONG ldap_modify_sA(LDAP*,const PCHAR,LDAPModA*[]);
WINLDAPAPI ULONG ldap_modify_sW(LDAP*,const PWCHAR,LDAPModW*[]);
WINLDAPAPI ULONG ldap_rename_extA(LDAP*,const PCHAR,const PCHAR,const PCHAR,INT,PLDAPControlA*,PLDAPControlA*,ULONG*);
WINLDAPAPI ULONG ldap_rename_extW(LDAP*,const PWCHAR,const PWCHAR,const PWCHAR,INT,PLDAPControlW*,PLDAPControlW*,ULONG*);
WINLDAPAPI ULONG ldap_rename_ext_sA(LDAP*,const PCHAR,const PCHAR,const PCHAR,INT,PLDAPControlA*,PLDAPControlA*);
WINLDAPAPI ULONG ldap_rename_ext_sW(LDAP*,const PWCHAR,const PWCHAR,const PWCHAR,INT,PLDAPControlW*,PLDAPControlW*);
WINLDAPAPI ULONG ldap_modrdnA(LDAP*,PCHAR,PCHAR) _WINLDAP_DEPRECATED;
WINLDAPAPI ULONG ldap_modrdnW(LDAP*,PWCHAR,PWCHAR) _WINLDAP_DEPRECATED;
WINLDAPAPI ULONG ldap_modrdn_sA(LDAP*,PCHAR,PCHAR) _WINLDAP_DEPRECATED;
WINLDAPAPI ULONG ldap_modrdn_sW(LDAP*,PWCHAR,PWCHAR) _WINLDAP_DEPRECATED;
WINLDAPAPI ULONG ldap_modrdn2A(LDAP*,PCHAR,PCHAR,INT) _WINLDAP_DEPRECATED;
WINLDAPAPI ULONG ldap_modrdn2W(LDAP*,PWCHAR,PWCHAR,INT) _WINLDAP_DEPRECATED;
WINLDAPAPI ULONG ldap_modrdn2_sA(LDAP*,PCHAR,PCHAR,INT) _WINLDAP_DEPRECATED;
WINLDAPAPI ULONG ldap_modrdn2_sW(LDAP*,PWCHAR,PWCHAR,INT) _WINLDAP_DEPRECATED;
WINLDAPAPI ULONG ldap_add_extA(LDAP*,const PCHAR,LDAPModA*[],PLDAPControlA*,PLDAPControlA*,ULONG*);
WINLDAPAPI ULONG ldap_add_extW(LDAP*,const PWCHAR,LDAPModW*[],PLDAPControlW*,PLDAPControlW*,ULONG*);
WINLDAPAPI ULONG ldap_add_ext_sA(LDAP*,const PCHAR,LDAPModA*[],PLDAPControlA*,PLDAPControlA*);
WINLDAPAPI ULONG ldap_add_ext_sW(LDAP*,const PWCHAR,LDAPModW*[],PLDAPControlW*,PLDAPControlW*);
WINLDAPAPI ULONG ldap_addA(LDAP*,const PCHAR,LDAPModA*[]);
WINLDAPAPI ULONG ldap_addW(LDAP*,const PWCHAR,LDAPModW*[]);
WINLDAPAPI ULONG ldap_add_sA(LDAP*,const PCHAR,LDAPModA*[]);
WINLDAPAPI ULONG ldap_add_sW(LDAP*,const PWCHAR,LDAPModW*[]);
WINLDAPAPI ULONG ldap_delete_extA(LDAP*,const PCHAR,PLDAPControlA*,PLDAPControlA*,ULONG*);
WINLDAPAPI ULONG ldap_delete_extW(LDAP*,const PWCHAR,PLDAPControlW*,PLDAPControlW*,ULONG*);
WINLDAPAPI ULONG ldap_delete_ext_sA(LDAP*,const PCHAR,PLDAPControlA*,PLDAPControlA*);
WINLDAPAPI ULONG ldap_delete_ext_sW(LDAP*,const PWCHAR,PLDAPControlW*,PLDAPControlW*);
WINLDAPAPI ULONG ldap_deleteA(LDAP*,const PCHAR);
WINLDAPAPI ULONG ldap_deleteW(LDAP*,const PWCHAR);
WINLDAPAPI ULONG ldap_delete_sA(LDAP*,const PCHAR);
WINLDAPAPI ULONG ldap_delete_sW(LDAP*,const PWCHAR);
WINLDAPAPI ULONG ldap_extended_operationA(LDAP*,const PCHAR,const struct berval*,PLDAPControlA*,PLDAPControlA*,ULONG*);
WINLDAPAPI ULONG ldap_extended_operationW(LDAP*,const PWCHAR,const struct berval*,PLDAPControlW*,PLDAPControlW*,ULONG*);
WINLDAPAPI ULONG ldap_extended_operation_sA(LDAP*,const PCHAR,const struct berval*,PLDAPControlA*,PLDAPControlA*,PCHAR*,struct berval**);
WINLDAPAPI ULONG ldap_extended_operation_sW(LDAP*,const PWCHAR,const struct berval*,PLDAPControlW*,PLDAPControlW*,PWCHAR*,struct berval**);
WINLDAPAPI ULONG ldap_close_extended_op(LDAP*,ULONG);
WINLDAPAPI ULONG ldap_abandon(LDAP*,ULONG);
WINLDAPAPI ULONG ldap_result(LDAP*,ULONG,ULONG,struct l_timeval*,LDAPMessage**);
WINLDAPAPI ULONG ldap_msgfree(LDAPMessage*);
WINLDAPAPI ULONG ldap_parse_resultA(LDAP*,LDAPMessage*,ULONG*,PCHAR*,PCHAR*,PCHAR**,PLDAPControlA**,BOOLEAN);
WINLDAPAPI ULONG ldap_parse_resultW(LDAP*,LDAPMessage*,ULONG*,PWCHAR*,PWCHAR*,PWCHAR**,PLDAPControlW**,BOOLEAN);
WINLDAPAPI ULONG ldap_parse_extended_resultA(LDAP,LDAPMessage*,PCHAR*,struct berval**,BOOLEAN);
WINLDAPAPI ULONG ldap_parse_extended_resultW(LDAP,LDAPMessage*,PWCHAR*,struct berval**,BOOLEAN);
WINLDAPAPI PCHAR ldap_err2stringA(ULONG);
WINLDAPAPI PWCHAR ldap_err2stringW(ULONG);
WINLDAPAPI ULONG LdapGetLastError(void);
WINLDAPAPI ULONG LdapMapErrorToWin32(ULONG);
WINLDAPAPI ULONG ldap_result2error(LDAP*,LDAPMessage*,ULONG);
WINLDAPAPI PLDAPMessage ldap_first_entry(LDAP*,LDAPMessage*);
WINLDAPAPI PLDAPMessage ldap_next_entry(LDAP*,LDAPMessage*);
WINLDAPAPI PLDAPMessage ldap_first_reference(LDAP*,LDAPMessage*);
WINLDAPAPI PLDAPMessage ldap_next_reference(LDAP*,LDAPMessage*);
WINLDAPAPI ULONG ldap_count_entries(LDAP*,LDAPMessage*);
WINLDAPAPI ULONG ldap_count_references(LDAP*,LDAPMessage*);
WINLDAPAPI PCHAR ldap_first_attributeA(LDAP*,LDAPMessage*,BerElement**);
WINLDAPAPI PWCHAR ldap_first_attributeW(LDAP*,LDAPMessage*,BerElement**);
WINLDAPAPI PCHAR ldap_next_attributeA(LDAP*,LDAPMessage*,BerElement*);
WINLDAPAPI PWCHAR ldap_next_attributeW(LDAP*,LDAPMessage*,BerElement*);
WINLDAPAPI VOID ldap_memfreeA(PCHAR);
WINLDAPAPI VOID ldap_memfreeW(PWCHAR);
WINLDAPAPI PCHAR* ldap_get_valuesA(LDAP*,LDAPMessage*,const PCHAR);
WINLDAPAPI PWCHAR* ldap_get_valuesW(LDAP*,LDAPMessage*,const PWCHAR);
WINLDAPAPI BerValue** ldap_get_values_lenA(LDAP*,LDAPMessage*,const PCHAR);
WINLDAPAPI BerValue** ldap_get_values_lenW(LDAP*,LDAPMessage*,const PWCHAR);
WINLDAPAPI ULONG ldap_count_valuesA(PCHAR*);
WINLDAPAPI ULONG ldap_count_valuesW(PWCHAR*);
WINLDAPAPI ULONG ldap_count_values_len(struct berval**);
WINLDAPAPI ULONG ldap_value_freeA(PCHAR*);
WINLDAPAPI ULONG ldap_value_freeW(PWCHAR*);
WINLDAPAPI ULONG ldap_value_free_len(struct berval**);
WINLDAPAPI PCHAR ldap_get_dnA(LDAP*,LDAPMessage*);
WINLDAPAPI PWCHAR ldap_get_dnW(LDAP*,LDAPMessage*);
WINLDAPAPI PCHAR ldap_explode_dnA(PCHAR,ULONG);
WINLDAPAPI PWCHAR ldap_explode_dnW(PWCHAR,ULONG);
WINLDAPAPI PCHAR ldap_dn2ufnA(PCHAR);
WINLDAPAPI PWCHAR ldap_dn2ufnW(PWCHAR);
WINLDAPAPI ULONG ldap_ufn2dnA(const PCHAR,PCHAR*);
WINLDAPAPI ULONG ldap_ufn2dnW(const PWCHAR,PWCHAR*);
WINLDAPAPI ULONG ldap_parse_referenceA(LDAP*,LDAPMessage*,PCHAR**);
WINLDAPAPI ULONG ldap_parse_referenceW(LDAP*,LDAPMessage*,PWCHAR**);
WINLDAPAPI ULONG ldap_check_filterA(LDAP*,PCHAR);
WINLDAPAPI ULONG ldap_check_filterW(LDAP*,PWCHAR);
WINLDAPAPI ULONG ldap_create_page_controlA(PLDAP,ULONG,struct berval*,UCHAR,PLDAPControlA*);
WINLDAPAPI ULONG ldap_create_page_controlW(PLDAP,ULONG,struct berval*,UCHAR,PLDAPControlW*);
WINLDAPAPI ULONG ldap_create_sort_controlA(PLDAP,PLDAPSortKeyA*,UCHAR,PLDAPControlA*);
WINLDAPAPI ULONG ldap_create_sort_controlW(PLDAP,PLDAPSortKeyW*,UCHAR,PLDAPControlW*);
WINLDAPAPI INT ldap_create_vlv_controlA(LDAP*,LDAPVLVInfo*,UCHAR,LDAPControlA**);
WINLDAPAPI INT ldap_create_vlv_controlW(LDAP*,LDAPVLVInfo*,UCHAR,LDAPControlW**);
WINLDAPAPI ULONG ldap_encode_sort_controlA(PLDAP,PLDAPSortKeyA*,PLDAPControlA,BOOLEAN);
WINLDAPAPI ULONG ldap_encode_sort_controlW(PLDAP,PLDAPSortKeyW*,PLDAPControlW,BOOLEAN);
WINLDAPAPI ULONG ldap_escape_filter_elementA(PCHAR,ULONG,PCHAR,ULONG);
WINLDAPAPI ULONG ldap_escape_filter_elementW(PWCHAR,ULONG,PWCHAR,ULONG);
WINLDAPAPI ULONG ldap_get_next_page(PLDAP,PLDAPSearch,ULONG,ULONG*);
WINLDAPAPI ULONG ldap_get_next_page_s(PLDAP,PLDAPSearch,struct l_timeval*,ULONG,ULONG*,LDAPMessage**);
WINLDAPAPI ULONG ldap_get_paged_count(PLDAP,PLDAPSearch,ULONG*,PLDAPMessage);
WINLDAPAPI ULONG ldap_parse_page_controlA(PLDAP,PLDAPControlA*,ULONG*,struct berval**);
WINLDAPAPI ULONG ldap_parse_page_controlW(PLDAP,PLDAPControlW*,ULONG*,struct berval**);
WINLDAPAPI ULONG ldap_parse_sort_controlA(PLDAP,PLDAPControlA*,ULONG*,PCHAR*);
WINLDAPAPI ULONG ldap_parse_sort_controlW(PLDAP,PLDAPControlW*,ULONG*,PWCHAR*);
WINLDAPAPI INT ldap_parse_vlv_controlA(LDAP*,LDAPControlA**,unsigned long*,unsigned long*,struct berval**,int*);
WINLDAPAPI INT ldap_parse_vlv_controlW(LDAP*,LDAPControlW**,unsigned long*,unsigned long*,struct berval**,int*);
WINLDAPAPI PLDAPSearch ldap_search_init_pageA(PLDAP,const PCHAR,ULONG,const PCHAR,PCHAR[],ULONG,PLDAPControlA*,PLDAPControlA*,ULONG,ULONG,PLDAPSortKeyA*);
WINLDAPAPI PLDAPSearch ldap_search_init_pageW(PLDAP,const PWCHAR,ULONG,const PWCHAR,PWCHAR[],ULONG,PLDAPControlW*,PLDAPControlW*,ULONG,ULONG,PLDAPSortKeyW*);
WINLDAPAPI ULONG ldap_search_abandon_page(PLDAP,PLDAPSearch);
WINLDAPAPI LDAP ldap_conn_from_msg(LDAP*,LDAPMessage*);
WINLDAPAPI INT LdapUnicodeToUTF8(LPCWSTR,int,LPSTR,int);
WINLDAPAPI INT LdapUTF8ToUnicode(LPCSTR,int,LPWSTR,int);

#if UNICODE
#define LDAPControl LDAPControlW
#define PLDAPControl PLDAPControlW
#define LDAPMod LDAPModW
#define PLDAPMod LDAPModW
#define LDAPSortKey LDAPSortKeyW
#define PLDAPSortKey PLDAPSortKeyW
#define LDAPAPIInfo LDAPAPIInfoW
#define PLDAPAPIInfo PLDAPAPIInfoW
#define LDAPAPIFeatureInfo LDAPAPIFeatureInfoW
#define PLDAPAPIFeatureInfo PLDAPAPIFeatureInfoW
#define cldap_open cldap_openW
#define ldap_open ldap_openW
#define ldap_bind ldap_bindW
#define ldap_bind_s ldap_bind_sW
#define ldap_simple_bind ldap_simple_bindW
#define ldap_simple_bind_s ldap_simple_bind_sW
#define ldap_sasl_bind ldap_sasl_bindW
#define ldap_sasl_bind_s ldap_sasl_bind_sW
#define ldap_init ldap_initW
#define ldap_sslinit ldap_sslinitW
#define ldap_get_option ldap_get_optionW
#define ldap_set_option ldap_set_optionW
#define ldap_start_tls_s ldap_start_tls_sW
#define ldap_add ldap_addW
#define ldap_add_ext ldap_add_extW
#define ldap_add_s ldap_add_sW
#define ldap_add_ext_s ldap_add_ext_sW
#define ldap_compare ldap_compareW
#define ldap_compare_ext ldap_compare_extW
#define ldap_compare_s ldap_compare_sW
#define ldap_compare_ext_s ldap_compare_ext_sW
#define ldap_delete ldap_deleteW
#define ldap_delete_ext ldap_delete_extW
#define ldap_delete_s ldap_delete_sW
#define ldap_delete_ext_s ldap_delete_ext_sW
#define ldap_extended_operation_s ldap_extended_operation_sW
#define ldap_extended_operation ldap_extended_operationW
#define ldap_modify ldap_modifyW
#define ldap_modify_ext ldap_modify_extW
#define ldap_modify_s ldap_modify_sW
#define ldap_modify_ext_s ldap_modify_ext_sW
#define ldap_check_filter ldap_check_filterW
#define ldap_count_values ldap_count_valuesW
#define ldap_create_page_control ldap_create_page_controlW
#define ldap_create_sort_control ldap_create_sort_controlW
#define ldap_create_vlv_control ldap_create_vlv_controlW
#define ldap_encode_sort_control ldap_encode_sort_controlW
#define ldap_escape_filter_element ldap_escape_filter_elementW
#define ldap_first_attribute ldap_first_attributeW
#define ldap_next_attribute ldap_next_attributeW
#define ldap_get_values ldap_get_valuesW
#define ldap_get_values_len ldap_get_values_lenW
#define ldap_parse_extended_result ldap_parse_extended_resultW
#define ldap_parse_page_control ldap_parse_page_controlW
#define ldap_parse_reference ldap_parse_referenceW
#define ldap_parse_result ldap_parse_resultW
#define ldap_parse_sort_control ldap_parse_sort_controlW
#define ldap_parse_vlv_control ldap_parse_vlv_controlW
#define ldap_search ldap_searchW
#define ldap_search_s ldap_search_sW
#define ldap_search_st ldap_search_stW
#define ldap_search_ext ldap_search_extW
#define ldap_search_ext_s ldap_search_ext_sW
#define ldap_search_init_page ldap_search_init_pageW
#define ldap_err2string ldap_err2stringW
#define ldap_control_free ldap_control_freeW
#define ldap_controls_free ldap_controls_freeW
#define ldap_free_controls ldap_free_controlsW
#define ldap_memfree ldap_memfreeW
#define ldap_value_free ldap_value_freeW
#define ldap_dn2ufn ldap_dn2ufnW
#define ldap_ufn2dn ldap_ufn2dnW
#define ldap_explode_dn ldap_explode_dnW
#define ldap_get_dn ldap_get_dnW
#define ldap_modrdn ldap_modrdnW
#define ldap_modrdn_s ldap_modrdn_sW
#define ldap_modrdn2 ldap_modrdn2W
#define ldap_modrdn2_s ldap_modrdn2_sW
#define ldap_rename ldap_rename_extW
#define ldap_rename_s ldap_rename_ext_sW
#define ldap_rename_ext ldap_rename_extW
#define ldap_rename_ext_s ldap_rename_ext_sW
#else
#define LDAPControl LDAPControlA
#define PLDAPControl PLDAPControlA
#define LDAPMod LDAPModA
#define PLDAPMod LDAPModA
#define LDAPSortKey LDAPSortKeyA
#define PLDAPSortKey PLDAPSortKeyA
#define LDAPAPIInfo LDAPAPIInfoA
#define PLDAPAPIInfo PLDAPAPIInfoA
#define LDAPAPIFeatureInfo LDAPAPIFeatureInfoA
#define PLDAPAPIFeatureInfo PLDAPAPIFeatureInfoA
#define cldap_open cldap_openA
#define ldap_open ldap_openA
#define ldap_bind ldap_bindA
#define ldap_bind_s ldap_bind_sA
#define ldap_simple_bind ldap_simple_bindA
#define ldap_simple_bind_s ldap_simple_bind_sA
#define ldap_sasl_bind ldap_sasl_bindA
#define ldap_sasl_bind_s ldap_sasl_bind_sA
#define ldap_init ldap_initA
#define ldap_sslinit ldap_sslinitA
#define ldap_get_option ldap_get_optionA
#define ldap_set_option ldap_set_optionA
#define ldap_start_tls_s ldap_start_tls_sA
#define ldap_add ldap_addA
#define ldap_add_ext ldap_add_extA
#define ldap_add_s ldap_add_sA
#define ldap_add_ext_s ldap_add_ext_sA
#define ldap_compare ldap_compareA
#define ldap_compare_ext ldap_compare_extA
#define ldap_compare_s ldap_compare_sA
#define ldap_compare_ext_s ldap_compare_ext_sA
#define ldap_delete ldap_deleteA
#define ldap_delete_ext ldap_delete_extA
#define ldap_delete_s ldap_delete_sA
#define ldap_delete_ext_s ldap_delete_ext_sA
#define ldap_extended_operation_s ldap_extended_operation_sA
#define ldap_extended_operation ldap_extended_operationA
#define ldap_modify ldap_modifyA
#define ldap_modify_ext ldap_modify_extA
#define ldap_modify_s ldap_modify_sA
#define ldap_modify_ext_s ldap_modify_ext_sA
#define ldap_check_filter ldap_check_filterA
#define ldap_count_values ldap_count_valuesA
#define ldap_create_page_control ldap_create_page_controlA
#define ldap_create_sort_control ldap_create_sort_controlA
#define ldap_create_vlv_control ldap_create_vlv_controlA
#define ldap_encode_sort_control ldap_encode_sort_controlA
#define ldap_escape_filter_element ldap_escape_filter_elementA
#define ldap_first_attribute ldap_first_attributeA
#define ldap_next_attribute ldap_next_attributeA
#define ldap_get_values ldap_get_valuesA
#define ldap_get_values_len ldap_get_values_lenA
#define ldap_parse_extended_result ldap_parse_extended_resultA
#define ldap_parse_page_control ldap_parse_page_controlA
#define ldap_parse_reference ldap_parse_referenceA
#define ldap_parse_result ldap_parse_resultA
#define ldap_parse_sort_control ldap_parse_sort_controlA
#define ldap_parse_vlv_control ldap_parse_vlv_controlA
#define ldap_search ldap_searchA
#define ldap_search_s ldap_search_sA
#define ldap_search_st ldap_search_stA
#define ldap_search_ext ldap_search_extA
#define ldap_search_ext_s ldap_search_ext_sA
#define ldap_search_init_page ldap_search_init_pageA
#define ldap_err2string ldap_err2stringA
#define ldap_control_free ldap_control_freeA
#define ldap_controls_free ldap_controls_freeA
#define ldap_free_controls ldap_free_controlsA
#define ldap_memfree ldap_memfreeA
#define ldap_value_free ldap_value_freeA
#define ldap_dn2ufn ldap_dn2ufnA
#define ldap_ufn2dn ldap_ufn2dnA
#define ldap_explode_dn ldap_explode_dnA
#define ldap_get_dn ldap_get_dnA
#define ldap_modrdn ldap_modrdnA
#define ldap_modrdn_s ldap_modrdn_sA
#define ldap_modrdn2 ldap_modrdn2A
#define ldap_modrdn2_s ldap_modrdn2_sA
#define ldap_rename ldap_rename_extA
#define ldap_rename_s ldap_rename_ext_sA
#define ldap_rename_ext ldap_rename_extA
#define ldap_rename_ext_s ldap_rename_ext_sA
#endif

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* _WINLDAP_H */
