/*
 * WLDAP32 - LDAP support for Wine
 *
 * Copyright 2005 Hans Leidekker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * This is an internal version of winldap.h where constants, types and
 * functions are prefixed with WLDAP32_ whenever they conflict with
 * native headers.
 */

typedef enum {
    WLDAP32_LDAP_SUCCESS                 =   0x00,
    WLDAP32_LDAP_UNWILLING_TO_PERFORM    =   0x35,
    WLDAP32_LDAP_SERVER_DOWN             =   0x51,
    WLDAP32_LDAP_LOCAL_ERROR             =   0x52,
    WLDAP32_LDAP_ENCODING_ERROR          =   0x53,
    WLDAP32_LDAP_DECODING_ERROR          =   0x54,
    WLDAP32_LDAP_TIMEOUT                 =   0x55,
    WLDAP32_LDAP_AUTH_UNKNOWN            =   0x56,
    WLDAP32_LDAP_FILTER_ERROR            =   0x57,
    WLDAP32_LDAP_USER_CANCELLED          =   0x58,
    WLDAP32_LDAP_PARAM_ERROR             =   0x59,
    WLDAP32_LDAP_NO_MEMORY               =   0x5a,
    WLDAP32_LDAP_CONNECT_ERROR           =   0x5b,
    WLDAP32_LDAP_NOT_SUPPORTED           =   0x5c,
    WLDAP32_LDAP_NO_RESULTS_RETURNED     =   0x5e,
    WLDAP32_LDAP_CONTROL_NOT_FOUND       =   0x5d,
    WLDAP32_LDAP_MORE_RESULTS_TO_RETURN  =   0x5f,

    WLDAP32_LDAP_CLIENT_LOOP             =   0x60,
    WLDAP32_LDAP_REFERRAL_LIMIT_EXCEEDED =   0x61
} LDAP_RETCODE;

typedef struct berelement
{
    PCHAR opaque;
} WLDAP32_BerElement;

#define WLDAP32_LDAP_OPT_API_INFO               0x00
#define WLDAP32_LDAP_OPT_DESC                   0x01
#define WLDAP32_LDAP_OPT_DEREF                  0x02
#define WLDAP32_LDAP_OPT_SIZELIMIT              0x03
#define WLDAP32_LDAP_OPT_TIMELIMIT              0x04
#define WLDAP32_LDAP_OPT_THREAD_FN_PTRS         0x05
#define WLDAP32_LDAP_OPT_REBIND_FN              0x06
#define WLDAP32_LDAP_OPT_REBIND_ARG             0x07
#define WLDAP32_LDAP_OPT_REFERRALS              0x08
#define WLDAP32_LDAP_OPT_RESTART                0x09
#define WLDAP32_LDAP_OPT_SSL                    0x0a
#define WLDAP32_LDAP_OPT_IO_FN_PTRS             0x0b
#define WLDAP32_LDAP_OPT_CACHE_FN_PTRS          0x0d
#define WLDAP32_LDAP_OPT_CACHE_STRATEGY         0x0e
#define WLDAP32_LDAP_OPT_CACHE_ENABLE           0x0f
#define WLDAP32_LDAP_OPT_REFERRAL_HOP_LIMIT     0x10
#define WLDAP32_LDAP_OPT_VERSION                0x11
#define WLDAP32_LDAP_OPT_PROTOCOL_VERSION       WLDAP32_LDAP_OPT_VERSION
#define WLDAP32_LDAP_OPT_SERVER_CONTROLS        0x12
#define WLDAP32_LDAP_OPT_API_FEATURE_INFO       0x15
#define WLDAP32_LDAP_OPT_HOST_NAME              0x30
#define WLDAP32_LDAP_OPT_ERROR_NUMBER           0x31
#define WLDAP32_LDAP_OPT_ERROR_STRING           0x32
#define WLDAP32_LDAP_OPT_SERVER_ERROR           0x33
#define WLDAP32_LDAP_OPT_SERVER_EXT_ERROR       0x34
#define WLDAP32_LDAP_OPT_PING_KEEP_ALIVE        0x36
#define WLDAP32_LDAP_OPT_PING_WAIT_TIME         0x37
#define WLDAP32_LDAP_OPT_PING_LIMIT             0x38
#define WLDAP32_LDAP_OPT_DNSDOMAIN_NAME         0x3b
#define WLDAP32_LDAP_OPT_GETDSNAME_FLAGS        0x3d
#define WLDAP32_LDAP_OPT_HOST_REACHABLE         0x3e
#define WLDAP32_LDAP_OPT_PROMPT_CREDENTIALS     0x3f
#define WLDAP32_LDAP_OPT_TCP_KEEPALIVE          0x40
#define WLDAP32_LDAP_OPT_FAST_CONCURRENT_BIND   0x41
#define WLDAP32_LDAP_OPT_SEND_TIMEOUT           0x42
#define WLDAP32_LDAP_OPT_REFERRAL_CALLBACK      0x70
#define WLDAP32_LDAP_OPT_CLIENT_CERTIFICATE     0x80
#define WLDAP32_LDAP_OPT_SERVER_CERTIFICATE     0x81
#define WLDAP32_LDAP_OPT_AUTO_RECONNECT         0x91
#define WLDAP32_LDAP_OPT_SSPI_FLAGS             0x92
#define WLDAP32_LDAP_OPT_SSL_INFO               0x93
#define WLDAP32_LDAP_OPT_REF_DEREF_CONN_PER_MSG 0x94
#define WLDAP32_LDAP_OPT_TLS                    WLDAP32_LDAP_OPT_SSL
#define WLDAP32_LDAP_OPT_TLS_INFO               WLDAP32_LDAP_OPT_SSL_INFO
#define WLDAP32_LDAP_OPT_SIGN                   0x95
#define WLDAP32_LDAP_OPT_ENCRYPT                0x96
#define WLDAP32_LDAP_OPT_SASL_METHOD            0x97
#define WLDAP32_LDAP_OPT_AREC_EXCLUSIVE         0x98
#define WLDAP32_LDAP_OPT_SECURITY_CONTEXT       0x99
#define WLDAP32_LDAP_OPT_ROOTDSE_CACHE          0x9a

typedef struct ldap
{
    struct
    {
        UINT_PTR sb_sd;
        UCHAR Reserved1[41];
        ULONG_PTR sb_naddr;
        UCHAR Reserved2[24];
    } ld_sb;

    PCHAR ld_host;
    ULONG ld_version;
    UCHAR ld_lberoptions;
    ULONG ld_deref;
    ULONG ld_timelimit;
    ULONG ld_sizelimit;
    ULONG ld_errno;
    PCHAR ld_matched;
    PCHAR ld_error;
    ULONG ld_msgid;
    UCHAR Reserved3[25];
    ULONG ld_cldaptries;
    ULONG ld_cldaptimeout;
    ULONG ld_refhoplimit;
    ULONG ld_options;
} WLDAP32_LDAP, *WLDAP32_PLDAP;

typedef struct ldapmodA {
    ULONG mod_op;
    PCHAR mod_type;
    union {
        PCHAR *modv_strvals;
        struct berval **modv_bvals;
    } mod_vals;
} LDAPModA, *PLDAPModA;

typedef struct ldapmodW {
    ULONG mod_op;
    PWCHAR mod_type;
    union {
        PWCHAR *modv_strvals;
        struct berval **modv_bvals;
    } mod_vals;
} LDAPModW, *PLDAPModW;

typedef struct l_timeval
{
    LONG tv_sec;
    LONG tv_usec;
} LDAP_TIMEVAL, *PLDAP_TIMEVAL;

typedef struct ldapmsg
{
    ULONG lm_msgid;
    ULONG lm_msgtype;

    PVOID lm_ber;

    struct ldapmsg *lm_chain;
    struct ldapmsg *lm_next;
    ULONG lm_time;

    WLDAP32_PLDAP Connection;
    PVOID Request;
    ULONG lm_returncode;
    USHORT lm_referral;
    BOOLEAN lm_chased;
    BOOLEAN lm_eom;
    BOOLEAN ConnectionReferenced;
} WLDAP32_LDAPMessage, *WLDAP32_PLDAPMessage;

#define LAPI_MAJOR_VER1     1
#define LAPI_MINOR_VER1     1

typedef struct ldap_version_info
{
    ULONG lv_size;
    ULONG lv_major;
    ULONG lv_minor;
} LDAP_VERSION_INFO, *PLDAP_VERSION_INFO;

typedef struct WLDAP32_berval
{
    ULONG bv_len;
    PCHAR bv_val;
} LDAP_BERVAL, *PLDAP_BERVAL, BERVAL, *PBERVAL, WLDAP32_BerValue;

#define LDAP_PAGED_RESULT_OID_STRING "1.2.840.113556.1.4.319"
#define LDAP_PAGED_RESULT_OID_STRING_W (const WCHAR []){'1','.','2','.', \
        '8','4','0','.','1','1','3','5','5','6','.','1','.','4','.','3','1','9',0}

#define LDAP_SERVER_RESP_SORT_OID "1.2.840.113556.1.4.474"
#define LDAP_SERVER_RESP_SORT_OID_W (const WCHAR []){'1','.','2','.', \
        '8','4','0','.','1','1','3','5','5','6','.','1','.','4','.','4','7','4',0}

#define LDAP_CONTROL_VLVRESPONSE "2.16.840.1.113730.3.4.10"
#define LDAP_CONTROL_VLVRESPONSE_W (const WCHAR []){'2','.','1','6','.', \
        '8','4','0','.','1','.','1','1','3','7','3','0','.','3','.','4','.','1','0',0}

typedef struct ldapcontrolA
{
    PCHAR ldctl_oid;
    struct WLDAP32_berval ldctl_value;
    BOOLEAN ldctl_iscritical;
} LDAPControlA, *PLDAPControlA;

typedef struct ldapcontrolW
{
    PWCHAR ldctl_oid;
    struct WLDAP32_berval ldctl_value;
    BOOLEAN ldctl_iscritical;
} LDAPControlW, *PLDAPControlW;

typedef struct WLDAP32_ldapvlvinfo
{
    int ldvlv_version;
    ULONG ldvlv_before_count;
    ULONG ldvlv_after_count;
    ULONG ldvlv_offset;
    ULONG ldvlv_count;
    PBERVAL ldvlv_attrvalue;
    PBERVAL ldvlv_context;
    VOID *ldvlv_extradata;
} WLDAP32_LDAPVLVInfo, *WLDAP32_PLDAPVLVInfo;

typedef struct ldapsearch LDAPSearch, *PLDAPSearch;

typedef struct ldapsortkeyA
{
    PCHAR sk_attrtype;
    PCHAR sk_matchruleoid;
    BOOLEAN sk_reverseorder;
} LDAPSortKeyA, *PLDAPSortKeyA;

typedef struct ldapsortkeyW
{
    PWCHAR sk_attrtype;
    PWCHAR sk_matchruleoid;
    BOOLEAN sk_reverseorder;
} LDAPSortKeyW, *PLDAPSortKeyW;

typedef struct ldapapiinfoA
{
    int ldapai_info_version;
    int ldapai_api_version;
    int ldapai_protocol_version;
    char **ldapai_extensions;
    char *ldapai_vendor_name;
    int ldapai_vendor_version;
} LDAPAPIInfoA;

typedef struct ldapapiinfoW
{
    int ldapai_info_version;
    int ldapai_api_version;
    int ldapai_protocol_version;
    PWCHAR *ldapai_extensions;
    PWCHAR ldapai_vendor_name;
    int ldapai_vendor_version;
} LDAPAPIInfoW;

typedef struct ldap_apifeature_infoA
{
    int ldapaif_info_version;
    char *ldapaif_name;
    int ldapaif_version;
} LDAPAPIFeatureInfoA;

typedef struct ldap_apifeature_infoW
{
    int ldapaif_info_version;
    PWCHAR ldapaif_name;
    int ldapaif_version;
} LDAPAPIFeatureInfoW;

WLDAP32_LDAP * CDECL cldap_openA(PCHAR,ULONG);
WLDAP32_LDAP * CDECL cldap_openW(PWCHAR,ULONG);
ULONG CDECL WLDAP32_ldap_abandon(WLDAP32_LDAP*,ULONG);
ULONG CDECL ldap_addA(WLDAP32_LDAP*,PCHAR,LDAPModA*[]);
ULONG CDECL ldap_addW(WLDAP32_LDAP*,PWCHAR,LDAPModW*[]);
ULONG CDECL ldap_add_extA(WLDAP32_LDAP*,PCHAR,LDAPModA*[],PLDAPControlA*,PLDAPControlA*,ULONG*);
ULONG CDECL ldap_add_extW(WLDAP32_LDAP*,PWCHAR,LDAPModW*[],PLDAPControlW*,PLDAPControlW*,ULONG*);
ULONG CDECL ldap_add_ext_sA(WLDAP32_LDAP*,PCHAR,LDAPModA*[],PLDAPControlA*,PLDAPControlA*);
ULONG CDECL ldap_add_ext_sW(WLDAP32_LDAP*,PWCHAR,LDAPModW*[],PLDAPControlW*,PLDAPControlW*);
ULONG CDECL ldap_add_sA(WLDAP32_LDAP*,PCHAR,LDAPModA*[]);
ULONG CDECL ldap_add_sW(WLDAP32_LDAP*,PWCHAR,LDAPModW*[]);
ULONG CDECL ldap_bindA(WLDAP32_LDAP*,PCHAR,PCHAR,ULONG);
ULONG CDECL ldap_bindW(WLDAP32_LDAP*,PWCHAR,PWCHAR,ULONG);
ULONG CDECL ldap_bind_sA(WLDAP32_LDAP*,PCHAR,PCHAR,ULONG);
ULONG CDECL ldap_bind_sW(WLDAP32_LDAP*,PWCHAR,PWCHAR,ULONG);
ULONG CDECL ldap_check_filterA(WLDAP32_LDAP*,PCHAR);
ULONG CDECL ldap_check_filterW(WLDAP32_LDAP*,PWCHAR);
ULONG CDECL ldap_cleanup(HANDLE);
ULONG CDECL ldap_close_extended_op(WLDAP32_LDAP*,ULONG);
ULONG CDECL ldap_compareA(WLDAP32_LDAP*,PCHAR,PCHAR,PCHAR);
ULONG CDECL ldap_compareW(WLDAP32_LDAP*,PWCHAR,PWCHAR,PWCHAR);
ULONG CDECL ldap_compare_extA(WLDAP32_LDAP*,PCHAR,PCHAR,PCHAR,struct WLDAP32_berval*,PLDAPControlA*,PLDAPControlA*,ULONG*);
ULONG CDECL ldap_compare_extW(WLDAP32_LDAP*,PWCHAR,PWCHAR,PWCHAR,struct WLDAP32_berval*,PLDAPControlW*,PLDAPControlW*,ULONG*);
ULONG CDECL ldap_compare_ext_sA(WLDAP32_LDAP*,PCHAR,PCHAR,PCHAR,struct WLDAP32_berval*,PLDAPControlA*,PLDAPControlA*);
ULONG CDECL ldap_compare_ext_sW(WLDAP32_LDAP*,PWCHAR,PWCHAR,PWCHAR,struct WLDAP32_berval*,PLDAPControlW*,PLDAPControlW*);
ULONG CDECL ldap_compare_sA(WLDAP32_LDAP*,PCHAR,PCHAR,PCHAR);
ULONG CDECL ldap_compare_sW(WLDAP32_LDAP*,PWCHAR,PWCHAR,PWCHAR);
ULONG CDECL ldap_connect(WLDAP32_LDAP*,LDAP_TIMEVAL*);
WLDAP32_LDAP * CDECL ldap_conn_from_msg(WLDAP32_LDAP*,WLDAP32_LDAPMessage*);
ULONG CDECL ldap_control_freeA(LDAPControlA*);
ULONG CDECL ldap_control_freeW(LDAPControlW*);
ULONG CDECL ldap_controls_freeA(LDAPControlA**);
ULONG CDECL ldap_controls_freeW(LDAPControlW**);
ULONG CDECL WLDAP32_ldap_count_entries(WLDAP32_LDAP*,WLDAP32_LDAPMessage*);
ULONG CDECL WLDAP32_ldap_count_references(WLDAP32_LDAP*,WLDAP32_LDAPMessage*);
ULONG CDECL ldap_count_valuesA(PCHAR*);
ULONG CDECL ldap_count_valuesW(PWCHAR*);
ULONG CDECL WLDAP32_ldap_count_values_len(PBERVAL*);
ULONG CDECL ldap_create_page_controlA(WLDAP32_PLDAP,ULONG,struct WLDAP32_berval*,UCHAR,PLDAPControlA*);
ULONG CDECL ldap_create_page_controlW(WLDAP32_PLDAP,ULONG,struct WLDAP32_berval*,UCHAR,PLDAPControlW*);
ULONG CDECL ldap_create_sort_controlA(WLDAP32_PLDAP,PLDAPSortKeyA*,UCHAR,PLDAPControlA*);
ULONG CDECL ldap_create_sort_controlW(WLDAP32_PLDAP,PLDAPSortKeyW*,UCHAR,PLDAPControlW*);
INT CDECL ldap_create_vlv_controlA(WLDAP32_LDAP*,WLDAP32_LDAPVLVInfo*,UCHAR,LDAPControlA**);
INT CDECL ldap_create_vlv_controlW(WLDAP32_LDAP*,WLDAP32_LDAPVLVInfo*,UCHAR,LDAPControlW**);
ULONG CDECL ldap_deleteA(WLDAP32_LDAP*,PCHAR);
ULONG CDECL ldap_deleteW(WLDAP32_LDAP*,PWCHAR);
ULONG CDECL ldap_delete_extA(WLDAP32_LDAP*,PCHAR,PLDAPControlA*,PLDAPControlA*,ULONG*);
ULONG CDECL ldap_delete_extW(WLDAP32_LDAP*,PWCHAR,PLDAPControlW*,PLDAPControlW*,ULONG*);
ULONG CDECL ldap_delete_ext_sA(WLDAP32_LDAP*,PCHAR,PLDAPControlA*,PLDAPControlA*);
ULONG CDECL ldap_delete_ext_sW(WLDAP32_LDAP*,PWCHAR,PLDAPControlW*,PLDAPControlW*);
ULONG CDECL ldap_delete_sA(WLDAP32_LDAP*,PCHAR);
ULONG CDECL ldap_delete_sW(WLDAP32_LDAP*,PWCHAR);
PCHAR CDECL ldap_dn2ufnA(PCHAR);
PWCHAR CDECL ldap_dn2ufnW(PWCHAR);
ULONG CDECL ldap_encode_sort_controlA(WLDAP32_PLDAP,PLDAPSortKeyA*,PLDAPControlA,BOOLEAN);
ULONG CDECL ldap_encode_sort_controlW(WLDAP32_PLDAP,PLDAPSortKeyW*,PLDAPControlW,BOOLEAN);
PCHAR CDECL ldap_err2stringA(ULONG);
PWCHAR CDECL ldap_err2stringW(ULONG);
ULONG CDECL ldap_escape_filter_elementA(PCHAR,ULONG,PCHAR,ULONG);
ULONG CDECL ldap_escape_filter_elementW(PCHAR,ULONG,PWCHAR,ULONG);
PCHAR * CDECL ldap_explode_dnA(PCHAR,ULONG);
PWCHAR * CDECL ldap_explode_dnW(PWCHAR,ULONG);
ULONG CDECL ldap_extended_operationA(WLDAP32_LDAP*,PCHAR,struct WLDAP32_berval*,PLDAPControlA*,PLDAPControlA*,ULONG*);
ULONG CDECL ldap_extended_operationW(WLDAP32_LDAP*,PWCHAR,struct WLDAP32_berval*,PLDAPControlW*,PLDAPControlW*,ULONG*);
ULONG CDECL ldap_extended_operation_sA(WLDAP32_LDAP*,PCHAR,struct WLDAP32_berval*,PLDAPControlA*, PLDAPControlA*,
    PCHAR*,struct WLDAP32_berval**);
ULONG CDECL ldap_extended_operation_sW(WLDAP32_LDAP*,PWCHAR,struct WLDAP32_berval*,PLDAPControlW*, PLDAPControlW*,
    PWCHAR*,struct WLDAP32_berval**);
PCHAR CDECL ldap_first_attributeA(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,WLDAP32_BerElement**);
PWCHAR CDECL ldap_first_attributeW(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,WLDAP32_BerElement**);
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_first_entry(WLDAP32_LDAP*,WLDAP32_LDAPMessage*);
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_first_reference(WLDAP32_LDAP*,WLDAP32_LDAPMessage*);
ULONG CDECL ldap_free_controlsA(LDAPControlA**);
ULONG CDECL ldap_free_controlsW(LDAPControlW**);
PCHAR CDECL ldap_get_dnA(WLDAP32_LDAP*,WLDAP32_LDAPMessage*);
PWCHAR CDECL ldap_get_dnW(WLDAP32_LDAP*,WLDAP32_LDAPMessage*);
ULONG CDECL ldap_get_next_page(WLDAP32_LDAP*,PLDAPSearch,ULONG,ULONG*);
ULONG CDECL ldap_get_next_page_s(WLDAP32_LDAP*,PLDAPSearch,struct l_timeval*,ULONG,ULONG*,WLDAP32_LDAPMessage**);
ULONG CDECL ldap_get_optionA(WLDAP32_LDAP*,int,void*);
ULONG CDECL ldap_get_optionW(WLDAP32_LDAP*,int,void*);
ULONG CDECL ldap_get_paged_count(WLDAP32_LDAP*,PLDAPSearch,ULONG*,WLDAP32_LDAPMessage*);
PCHAR * CDECL ldap_get_valuesA(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,PCHAR);
PWCHAR * CDECL ldap_get_valuesW(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,PWCHAR);
PBERVAL * CDECL ldap_get_values_lenA(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,PCHAR);
PBERVAL * CDECL ldap_get_values_lenW(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,PWCHAR);
WLDAP32_LDAP * CDECL ldap_initA(const PCHAR,ULONG);
WLDAP32_LDAP * CDECL ldap_initW(const PWCHAR,ULONG);
void CDECL ldap_memfreeA(PCHAR);
void CDECL ldap_memfreeW(PWCHAR);
ULONG CDECL ldap_modifyA(WLDAP32_LDAP*,PCHAR,LDAPModA*[]);
ULONG CDECL ldap_modifyW(WLDAP32_LDAP*,PWCHAR,LDAPModW*[]);
ULONG CDECL ldap_modify_extA(WLDAP32_LDAP*,PCHAR,LDAPModA*[],PLDAPControlA*,PLDAPControlA*,ULONG*);
ULONG CDECL ldap_modify_extW(WLDAP32_LDAP*,PWCHAR,LDAPModW*[],PLDAPControlW*,PLDAPControlW*,ULONG*);
ULONG CDECL ldap_modify_ext_sA(WLDAP32_LDAP*,PCHAR,LDAPModA*[],PLDAPControlA*,PLDAPControlA*);
ULONG CDECL ldap_modify_ext_sW(WLDAP32_LDAP*,PWCHAR,LDAPModW*[],PLDAPControlW*,PLDAPControlW*);
ULONG CDECL ldap_modify_sA(WLDAP32_LDAP*,PCHAR,LDAPModA*[]);
ULONG CDECL ldap_modify_sW(WLDAP32_LDAP*,PWCHAR,LDAPModW*[]);
ULONG CDECL ldap_modrdnA(WLDAP32_LDAP*,PCHAR,PCHAR);
ULONG CDECL ldap_modrdnW(WLDAP32_LDAP*,PWCHAR,PWCHAR);
ULONG CDECL ldap_modrdn2A(WLDAP32_LDAP*,PCHAR,PCHAR,INT);
ULONG CDECL ldap_modrdn2W(WLDAP32_LDAP*,PWCHAR,PWCHAR,INT);
ULONG CDECL ldap_modrdn2_sA(WLDAP32_LDAP*,PCHAR,PCHAR,INT);
ULONG CDECL ldap_modrdn2_sW(WLDAP32_LDAP*,PWCHAR,PWCHAR,INT);
ULONG CDECL ldap_modrdn_sA(WLDAP32_LDAP*,PCHAR,PCHAR);
ULONG CDECL ldap_modrdn_sW(WLDAP32_LDAP*,PWCHAR,PWCHAR);
ULONG CDECL WLDAP32_ldap_msgfree(WLDAP32_LDAPMessage*);
PCHAR CDECL ldap_next_attributeA(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,WLDAP32_BerElement*);
PWCHAR CDECL ldap_next_attributeW(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,WLDAP32_BerElement*);
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_next_entry(WLDAP32_LDAP*,WLDAP32_LDAPMessage*);
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_next_reference(WLDAP32_LDAP*,WLDAP32_LDAPMessage*);
WLDAP32_LDAP * CDECL ldap_openA(PCHAR,ULONG);
WLDAP32_LDAP * CDECL ldap_openW(PWCHAR,ULONG);
ULONG CDECL ldap_parse_extended_resultA(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,PCHAR*,struct WLDAP32_berval**,BOOLEAN);
ULONG CDECL ldap_parse_extended_resultW(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,PWCHAR*,struct WLDAP32_berval**,BOOLEAN);
ULONG CDECL ldap_parse_page_controlA(WLDAP32_LDAP*,PLDAPControlA*,ULONG*,struct WLDAP32_berval**);
ULONG CDECL ldap_parse_page_controlW(WLDAP32_LDAP*,PLDAPControlW*,ULONG*,struct WLDAP32_berval**);
ULONG CDECL ldap_parse_referenceA(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,PCHAR**);
ULONG CDECL ldap_parse_referenceW(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,PWCHAR**);
ULONG CDECL ldap_parse_resultA(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,ULONG*,PCHAR*,PCHAR*,PCHAR**,PLDAPControlA**,BOOLEAN);
ULONG CDECL ldap_parse_resultW(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,ULONG*,PWCHAR*,PWCHAR*,PWCHAR**,PLDAPControlW**,BOOLEAN);
ULONG CDECL ldap_parse_sort_controlA(WLDAP32_LDAP*,PLDAPControlA*,ULONG*,PCHAR*);
ULONG CDECL ldap_parse_sort_controlW(WLDAP32_LDAP*,PLDAPControlW*,ULONG*,PWCHAR*);
int CDECL ldap_parse_vlv_controlA(WLDAP32_LDAP*,LDAPControlA**,ULONG*,ULONG*,struct WLDAP32_berval**,INT*);
int CDECL ldap_parse_vlv_controlW(WLDAP32_LDAP*,LDAPControlW**,ULONG*,ULONG*,struct WLDAP32_berval**,INT*);
void CDECL WLDAP32_ldap_perror(WLDAP32_LDAP*,const PCHAR);
ULONG CDECL ldap_rename_extA(WLDAP32_LDAP*,PCHAR,PCHAR,PCHAR,INT,PLDAPControlA*,PLDAPControlA*,ULONG*);
ULONG CDECL ldap_rename_extW(WLDAP32_LDAP*,PWCHAR,PWCHAR,PWCHAR,INT,PLDAPControlW*,PLDAPControlW*,ULONG*);
ULONG CDECL ldap_rename_ext_sA(WLDAP32_LDAP*,PCHAR,PCHAR,PCHAR,INT,PLDAPControlA*,PLDAPControlA*);
ULONG CDECL ldap_rename_ext_sW(WLDAP32_LDAP*,PWCHAR,PWCHAR,PWCHAR,INT,PLDAPControlW*,PLDAPControlW*);
ULONG CDECL WLDAP32_ldap_result(WLDAP32_LDAP*,ULONG,ULONG,struct l_timeval*,WLDAP32_LDAPMessage**);
ULONG CDECL WLDAP32_ldap_result2error(WLDAP32_LDAP*,WLDAP32_LDAPMessage*,ULONG);
ULONG CDECL ldap_sasl_bindA(WLDAP32_LDAP*,const PCHAR,const PCHAR,const BERVAL*,PLDAPControlA*,PLDAPControlA*,int*);
ULONG CDECL ldap_sasl_bindW(WLDAP32_LDAP*,const PWCHAR,const PWCHAR,const BERVAL*,PLDAPControlW*,PLDAPControlW*,int*);
ULONG CDECL ldap_sasl_bind_sA(WLDAP32_LDAP*,const PCHAR,const PCHAR,const BERVAL*,PLDAPControlA*,PLDAPControlA*,PBERVAL*);
ULONG CDECL ldap_sasl_bind_sW(WLDAP32_LDAP*,const PWCHAR,const PWCHAR,const BERVAL*,PLDAPControlW*,PLDAPControlW*,PBERVAL*);
ULONG CDECL ldap_search_abandon_page(WLDAP32_PLDAP,PLDAPSearch);
ULONG CDECL ldap_searchA(WLDAP32_LDAP*,PCHAR,ULONG,PCHAR,PCHAR[],ULONG);
ULONG CDECL ldap_searchW(WLDAP32_LDAP*,PWCHAR,ULONG,PWCHAR,PWCHAR[],ULONG);
ULONG CDECL ldap_search_extA(WLDAP32_LDAP*,PCHAR,ULONG,PCHAR,PCHAR[],ULONG,PLDAPControlA*,
    PLDAPControlA*,ULONG,ULONG,ULONG*);
ULONG CDECL ldap_search_extW(WLDAP32_LDAP*,PWCHAR,ULONG,PWCHAR,PWCHAR[],ULONG,PLDAPControlW*,
    PLDAPControlW*,ULONG,ULONG,ULONG*);
ULONG CDECL ldap_search_ext_sA(WLDAP32_LDAP*,PCHAR,ULONG,PCHAR,PCHAR[],ULONG,PLDAPControlA*,
    PLDAPControlA*,struct l_timeval*,ULONG,WLDAP32_LDAPMessage**);
ULONG CDECL ldap_search_ext_sW(WLDAP32_LDAP*,PWCHAR,ULONG,PWCHAR,PWCHAR[],ULONG,PLDAPControlW*,
    PLDAPControlW*,struct l_timeval*,ULONG,WLDAP32_LDAPMessage**);
PLDAPSearch CDECL ldap_search_init_pageA(WLDAP32_PLDAP,PCHAR,ULONG,PCHAR,PCHAR[],ULONG,PLDAPControlA*,
    PLDAPControlA*,ULONG,ULONG,PLDAPSortKeyA*);
PLDAPSearch CDECL ldap_search_init_pageW(WLDAP32_PLDAP,PWCHAR,ULONG,PWCHAR,PWCHAR[],ULONG,PLDAPControlW*,
    PLDAPControlW*,ULONG,ULONG, PLDAPSortKeyW*);
ULONG CDECL ldap_search_sA(WLDAP32_LDAP*,PCHAR,ULONG,PCHAR,PCHAR[],ULONG,WLDAP32_LDAPMessage**);
ULONG CDECL ldap_search_sW(WLDAP32_LDAP*,PWCHAR,ULONG,PWCHAR,PWCHAR[],ULONG,WLDAP32_LDAPMessage**);
ULONG CDECL ldap_search_stA(WLDAP32_LDAP*,const PCHAR,ULONG,const PCHAR,PCHAR[],ULONG,
    struct l_timeval*,WLDAP32_LDAPMessage**);
ULONG CDECL ldap_search_stW(WLDAP32_LDAP*,const PWCHAR,ULONG,const PWCHAR,PWCHAR[],ULONG,
    struct l_timeval*,WLDAP32_LDAPMessage**);
ULONG CDECL ldap_set_optionA(WLDAP32_LDAP*,int,void*);
ULONG CDECL ldap_set_optionW(WLDAP32_LDAP*,int,void*);
ULONG CDECL ldap_simple_bindA(WLDAP32_LDAP*,PCHAR,PCHAR);
ULONG CDECL ldap_simple_bindW(WLDAP32_LDAP*,PWCHAR,PWCHAR);
ULONG CDECL ldap_simple_bind_sA(WLDAP32_LDAP*,PCHAR,PCHAR);
ULONG CDECL ldap_simple_bind_sW(WLDAP32_LDAP*,PWCHAR,PWCHAR);
WLDAP32_LDAP * CDECL ldap_sslinitA(PCHAR,ULONG,int);
WLDAP32_LDAP * CDECL ldap_sslinitW(PWCHAR,ULONG,int);
ULONG CDECL ldap_start_tls_sA(WLDAP32_PLDAP,PULONG,WLDAP32_LDAPMessage**,PLDAPControlA*,PLDAPControlA*);
ULONG CDECL ldap_start_tls_sW(WLDAP32_PLDAP,PULONG,WLDAP32_LDAPMessage**,PLDAPControlW*,PLDAPControlW*);
ULONG CDECL ldap_startup(PLDAP_VERSION_INFO,HANDLE*);
BOOLEAN CDECL ldap_stop_tls_s(WLDAP32_PLDAP);
ULONG CDECL ldap_ufn2dnA(PCHAR,PCHAR*);
ULONG CDECL ldap_ufn2dnW(PWCHAR,PWCHAR*);
ULONG CDECL WLDAP32_ldap_unbind(WLDAP32_LDAP*);
ULONG CDECL WLDAP32_ldap_unbind_s(WLDAP32_LDAP*);
ULONG CDECL ldap_value_freeA(PCHAR*);
ULONG CDECL ldap_value_freeW(PWCHAR*);
ULONG CDECL WLDAP32_ldap_value_free_len(struct WLDAP32_berval**);

ULONG CDECL LdapGetLastError(void);
ULONG CDECL LdapMapErrorToWin32(ULONG);
int CDECL LdapUnicodeToUTF8(LPCWSTR,int,LPSTR,int);
int CDECL LdapUTF8ToUnicode(LPCSTR,int,LPWSTR,int);
