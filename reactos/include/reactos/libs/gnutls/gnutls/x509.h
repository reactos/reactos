/*
 * Copyright (C) 2003-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/* This file contains the types and prototypes for the X.509
 * certificate and CRL handling functions.
 */

#ifndef GNUTLS_X509_H
#define GNUTLS_X509_H

#include <gnutls/gnutls.h>

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

/* Some OIDs usually found in Distinguished names, or
 * in Subject Directory Attribute extensions.
 */
#define GNUTLS_OID_X520_COUNTRY_NAME		"2.5.4.6"
#define GNUTLS_OID_X520_ORGANIZATION_NAME	"2.5.4.10"
#define GNUTLS_OID_X520_ORGANIZATIONAL_UNIT_NAME "2.5.4.11"
#define GNUTLS_OID_X520_COMMON_NAME		"2.5.4.3"
#define GNUTLS_OID_X520_LOCALITY_NAME		"2.5.4.7"
#define GNUTLS_OID_X520_STATE_OR_PROVINCE_NAME	"2.5.4.8"

#define GNUTLS_OID_X520_INITIALS		"2.5.4.43"
#define GNUTLS_OID_X520_GENERATION_QUALIFIER	"2.5.4.44"
#define GNUTLS_OID_X520_SURNAME			"2.5.4.4"
#define GNUTLS_OID_X520_GIVEN_NAME		"2.5.4.42"
#define GNUTLS_OID_X520_TITLE			"2.5.4.12"
#define GNUTLS_OID_X520_DN_QUALIFIER		"2.5.4.46"
#define GNUTLS_OID_X520_PSEUDONYM		"2.5.4.65"
#define GNUTLS_OID_X520_POSTALCODE              "2.5.4.17"
#define GNUTLS_OID_X520_NAME                    "2.5.4.41"

#define GNUTLS_OID_LDAP_DC			"0.9.2342.19200300.100.1.25"
#define GNUTLS_OID_LDAP_UID			"0.9.2342.19200300.100.1.1"

/* The following should not be included in DN.
 */
#define GNUTLS_OID_PKCS9_EMAIL			"1.2.840.113549.1.9.1"

#define GNUTLS_OID_PKIX_DATE_OF_BIRTH		"1.3.6.1.5.5.7.9.1"
#define GNUTLS_OID_PKIX_PLACE_OF_BIRTH		"1.3.6.1.5.5.7.9.2"
#define GNUTLS_OID_PKIX_GENDER			"1.3.6.1.5.5.7.9.3"
#define GNUTLS_OID_PKIX_COUNTRY_OF_CITIZENSHIP	"1.3.6.1.5.5.7.9.4"
#define GNUTLS_OID_PKIX_COUNTRY_OF_RESIDENCE	"1.3.6.1.5.5.7.9.5"

/* Key purpose Object Identifiers.
 */
#define GNUTLS_KP_TLS_WWW_SERVER		"1.3.6.1.5.5.7.3.1"
#define GNUTLS_KP_TLS_WWW_CLIENT                "1.3.6.1.5.5.7.3.2"
#define GNUTLS_KP_CODE_SIGNING			"1.3.6.1.5.5.7.3.3"
#define GNUTLS_KP_MS_SMART_CARD_LOGON		"1.3.6.1.4.1.311.20.2.2"
#define GNUTLS_KP_EMAIL_PROTECTION		"1.3.6.1.5.5.7.3.4"
#define GNUTLS_KP_TIME_STAMPING			"1.3.6.1.5.5.7.3.8"
#define GNUTLS_KP_OCSP_SIGNING			"1.3.6.1.5.5.7.3.9"
#define GNUTLS_KP_IPSEC_IKE			"1.3.6.1.5.5.7.3.17"
#define GNUTLS_KP_ANY				"2.5.29.37.0"

#define GNUTLS_OID_AIA				"1.3.6.1.5.5.7.1.1"
#define GNUTLS_OID_AD_OCSP			"1.3.6.1.5.5.7.48.1"
#define GNUTLS_OID_AD_CAISSUERS			"1.3.6.1.5.5.7.48.2"

#define GNUTLS_FSAN_SET 0
#define GNUTLS_FSAN_APPEND 1

/* Certificate handling functions.
 */

/**
 * gnutls_certificate_import_flags:
 * @GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED: Fail if the
 *   certificates in the buffer are more than the space allocated for
 *   certificates. The error code will be %GNUTLS_E_SHORT_MEMORY_BUFFER.
 * @GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED: Fail if the certificates
 *   in the buffer are not ordered starting from subject to issuer.
 *   The error code will be %GNUTLS_E_CERTIFICATE_LIST_UNSORTED.
 *
 * Enumeration of different certificate import flags.
 */
typedef enum gnutls_certificate_import_flags {
	GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED = 1,
	GNUTLS_X509_CRT_LIST_FAIL_IF_UNSORTED = 2
} gnutls_certificate_import_flags;

int gnutls_x509_crt_init(gnutls_x509_crt_t * cert);
void gnutls_x509_crt_deinit(gnutls_x509_crt_t cert);
int gnutls_x509_crt_import(gnutls_x509_crt_t cert,
			   const gnutls_datum_t * data,
			   gnutls_x509_crt_fmt_t format);
int gnutls_x509_crt_list_import2(gnutls_x509_crt_t ** certs,
				 unsigned int *size,
				 const gnutls_datum_t * data,
				 gnutls_x509_crt_fmt_t format,
				 unsigned int flags);
int gnutls_x509_crt_list_import(gnutls_x509_crt_t * certs,
				unsigned int *cert_max,
				const gnutls_datum_t * data,
				gnutls_x509_crt_fmt_t format,
				unsigned int flags);
int gnutls_x509_crt_export(gnutls_x509_crt_t cert,
			   gnutls_x509_crt_fmt_t format,
			   void *output_data, size_t * output_data_size);
int gnutls_x509_crt_export2(gnutls_x509_crt_t cert,
			    gnutls_x509_crt_fmt_t format,
			    gnutls_datum_t * out);
int gnutls_x509_crt_get_private_key_usage_period(gnutls_x509_crt_t
						 cert,
						 time_t *
						 activation,
						 time_t *
						 expiration, unsigned int
						 *critical);

int gnutls_x509_crt_get_issuer_dn(gnutls_x509_crt_t cert,
				  char *buf, size_t * buf_size);
int gnutls_x509_crt_get_issuer_dn2(gnutls_x509_crt_t cert,
				   gnutls_datum_t * dn);
int gnutls_x509_crt_get_issuer_dn_oid(gnutls_x509_crt_t cert,
				      int indx, void *oid,
				      size_t * oid_size);
int gnutls_x509_crt_get_issuer_dn_by_oid(gnutls_x509_crt_t cert,
					 const char *oid, int indx,
					 unsigned int raw_flag,
					 void *buf, size_t * buf_size);
int gnutls_x509_crt_get_dn(gnutls_x509_crt_t cert, char *buf,
			   size_t * buf_size);
int gnutls_x509_crt_get_dn2(gnutls_x509_crt_t cert, gnutls_datum_t * dn);
int gnutls_x509_crt_get_dn_oid(gnutls_x509_crt_t cert, int indx,
			       void *oid, size_t * oid_size);
int gnutls_x509_crt_get_dn_by_oid(gnutls_x509_crt_t cert,
				  const char *oid, int indx,
				  unsigned int raw_flag, void *buf,
				  size_t * buf_size);
int gnutls_x509_crt_check_hostname(gnutls_x509_crt_t cert,
				   const char *hostname);

int gnutls_x509_crt_get_signature_algorithm(gnutls_x509_crt_t cert);
int gnutls_x509_crt_get_signature(gnutls_x509_crt_t cert,
				  char *sig, size_t * sizeof_sig);
int gnutls_x509_crt_get_version(gnutls_x509_crt_t cert);
int gnutls_x509_crt_get_key_id(gnutls_x509_crt_t crt,
			       unsigned int flags,
			       unsigned char *output_data,
			       size_t * output_data_size);

int gnutls_x509_crt_set_private_key_usage_period(gnutls_x509_crt_t
						 crt,
						 time_t activation,
						 time_t expiration);
int gnutls_x509_crt_set_authority_key_id(gnutls_x509_crt_t cert,
					 const void *id, size_t id_size);
int gnutls_x509_crt_get_authority_key_id(gnutls_x509_crt_t cert,
					 void *id,
					 size_t * id_size,
					 unsigned int *critical);
int gnutls_x509_crt_get_authority_key_gn_serial(gnutls_x509_crt_t
						cert,
						unsigned int seq,
						void *alt,
						size_t * alt_size,
						unsigned int
						*alt_type,
						void *serial,
						size_t *
						serial_size, unsigned int
						*critical);

int gnutls_x509_crt_get_subject_key_id(gnutls_x509_crt_t cert,
				       void *ret,
				       size_t * ret_size,
				       unsigned int *critical);

int gnutls_x509_crt_get_subject_unique_id(gnutls_x509_crt_t crt,
					  char *buf, size_t * buf_size);

int gnutls_x509_crt_get_issuer_unique_id(gnutls_x509_crt_t crt,
					 char *buf, size_t * buf_size);

void gnutls_x509_crt_set_pin_function(gnutls_x509_crt_t crt,
				      gnutls_pin_callback_t fn,
				      void *userdata);

  /**
   * gnutls_info_access_what_t:
   * @GNUTLS_IA_ACCESSMETHOD_OID: Get accessMethod OID.
   * @GNUTLS_IA_ACCESSLOCATION_GENERALNAME_TYPE: Get accessLocation name type.
   * @GNUTLS_IA_URI: Get accessLocation URI value.
   * @GNUTLS_IA_OCSP_URI: get accessLocation URI value for OCSP.
   * @GNUTLS_IA_CAISSUERS_URI: get accessLocation URI value for caIssuers.
   *
   * Enumeration of types for the @what parameter of
   * gnutls_x509_crt_get_authority_info_access().
   */
typedef enum gnutls_info_access_what_t {
	GNUTLS_IA_ACCESSMETHOD_OID = 1,
	GNUTLS_IA_ACCESSLOCATION_GENERALNAME_TYPE = 2,
	/* use 100-108 for the generalName types, populate as needed */
	GNUTLS_IA_URI = 106,
	/* quick-access variants that match both OID and name type. */
	GNUTLS_IA_OCSP_URI = 10006,
	GNUTLS_IA_CAISSUERS_URI = 10106
} gnutls_info_access_what_t;

int gnutls_x509_crt_get_authority_info_access(gnutls_x509_crt_t
					      crt,
					      unsigned int seq,
					      int what,
					      gnutls_datum_t *
					      data, unsigned int
					      *critical);

#define GNUTLS_CRL_REASON_SUPERSEEDED GNUTLS_CRL_REASON_SUPERSEDED,
  /**
   * gnutls_x509_crl_reason_flags_t:
   * @GNUTLS_CRL_REASON_PRIVILEGE_WITHDRAWN: The privileges were withdrawn from the owner.
   * @GNUTLS_CRL_REASON_CERTIFICATE_HOLD: The certificate is on hold.
   * @GNUTLS_CRL_REASON_CESSATION_OF_OPERATION: The end-entity is no longer operating.
   * @GNUTLS_CRL_REASON_SUPERSEDED: There is a newer certificate of the owner.
   * @GNUTLS_CRL_REASON_AFFILIATION_CHANGED: The end-entity affiliation has changed.
   * @GNUTLS_CRL_REASON_CA_COMPROMISE: The CA was compromised.
   * @GNUTLS_CRL_REASON_KEY_COMPROMISE: The certificate's key was compromised.
   * @GNUTLS_CRL_REASON_UNUSED: The key was never used.
   * @GNUTLS_CRL_REASON_AA_COMPROMISE: AA compromised.
   *
   * Enumeration of types for the CRL revocation reasons. 
   */
typedef enum gnutls_x509_crl_reason_flags_t {
	GNUTLS_CRL_REASON_UNSPECIFIED = 0,
	GNUTLS_CRL_REASON_PRIVILEGE_WITHDRAWN = 1,
	GNUTLS_CRL_REASON_CERTIFICATE_HOLD = 2,
	GNUTLS_CRL_REASON_CESSATION_OF_OPERATION = 4,
	GNUTLS_CRL_REASON_SUPERSEDED = 8,
	GNUTLS_CRL_REASON_AFFILIATION_CHANGED = 16,
	GNUTLS_CRL_REASON_CA_COMPROMISE = 32,
	GNUTLS_CRL_REASON_KEY_COMPROMISE = 64,
	GNUTLS_CRL_REASON_UNUSED = 128,
	GNUTLS_CRL_REASON_AA_COMPROMISE = 32768
} gnutls_x509_crl_reason_flags_t;

int gnutls_x509_crt_get_crl_dist_points(gnutls_x509_crt_t cert,
					unsigned int seq,
					void *ret,
					size_t * ret_size,
					unsigned int *reason_flags,
					unsigned int *critical);
int gnutls_x509_crt_set_crl_dist_points2(gnutls_x509_crt_t crt,
					 gnutls_x509_subject_alt_name_t
					 type, const void *data,
					 unsigned int data_size,
					 unsigned int reason_flags);
int gnutls_x509_crt_set_crl_dist_points(gnutls_x509_crt_t crt,
					gnutls_x509_subject_alt_name_t
					type,
					const void *data_string,
					unsigned int reason_flags);
int gnutls_x509_crt_cpy_crl_dist_points(gnutls_x509_crt_t dst,
					gnutls_x509_crt_t src);

int gnutls_x509_crl_sign2(gnutls_x509_crl_t crl,
			  gnutls_x509_crt_t issuer,
			  gnutls_x509_privkey_t issuer_key,
			  gnutls_digest_algorithm_t dig,
			  unsigned int flags);

time_t gnutls_x509_crt_get_activation_time(gnutls_x509_crt_t cert);

#define GNUTLS_X509_NO_WELL_DEFINED_EXPIRATION ((time_t)4294197631)

time_t gnutls_x509_crt_get_expiration_time(gnutls_x509_crt_t cert);
int gnutls_x509_crt_get_serial(gnutls_x509_crt_t cert,
			       void *result, size_t * result_size);

int gnutls_x509_crt_get_pk_algorithm(gnutls_x509_crt_t cert,
				     unsigned int *bits);
int gnutls_x509_crt_get_pk_rsa_raw(gnutls_x509_crt_t crt,
				   gnutls_datum_t * m, gnutls_datum_t * e);
int gnutls_x509_crt_get_pk_dsa_raw(gnutls_x509_crt_t crt,
				   gnutls_datum_t * p,
				   gnutls_datum_t * q,
				   gnutls_datum_t * g, gnutls_datum_t * y);

int gnutls_x509_crt_get_subject_alt_name(gnutls_x509_crt_t cert,
					 unsigned int seq,
					 void *san,
					 size_t * san_size,
					 unsigned int *critical);
int gnutls_x509_crt_get_subject_alt_name2(gnutls_x509_crt_t cert,
					  unsigned int seq,
					  void *san,
					  size_t * san_size,
					  unsigned int *san_type,
					  unsigned int *critical);

int gnutls_x509_crt_get_subject_alt_othername_oid(gnutls_x509_crt_t
						  cert,
						  unsigned int seq,
						  void *oid,
						  size_t * oid_size);

int gnutls_x509_crt_get_issuer_alt_name(gnutls_x509_crt_t cert,
					unsigned int seq,
					void *ian,
					size_t * ian_size,
					unsigned int *critical);
int gnutls_x509_crt_get_issuer_alt_name2(gnutls_x509_crt_t cert,
					 unsigned int seq,
					 void *ian,
					 size_t * ian_size,
					 unsigned int *ian_type,
					 unsigned int *critical);

int gnutls_x509_crt_get_issuer_alt_othername_oid(gnutls_x509_crt_t
						 cert,
						 unsigned int seq,
						 void *ret,
						 size_t * ret_size);

int gnutls_x509_crt_get_ca_status(gnutls_x509_crt_t cert,
				  unsigned int *critical);
int gnutls_x509_crt_get_basic_constraints(gnutls_x509_crt_t cert,
					  unsigned int *critical,
					  unsigned int *ca, int *pathlen);

/* The key_usage flags are defined in gnutls.h. They are the
 * GNUTLS_KEY_* definitions.
 */
int gnutls_x509_crt_get_key_usage(gnutls_x509_crt_t cert,
				  unsigned int *key_usage,
				  unsigned int *critical);
int gnutls_x509_crt_set_key_usage(gnutls_x509_crt_t crt,
				  unsigned int usage);
int gnutls_x509_crt_set_authority_info_access(gnutls_x509_crt_t
					      crt, int what,
					      gnutls_datum_t * data);

int gnutls_x509_crt_get_proxy(gnutls_x509_crt_t cert,
			      unsigned int *critical,
			      int *pathlen,
			      char **policyLanguage,
			      char **policy, size_t * sizeof_policy);

#define GNUTLS_MAX_QUALIFIERS 8

  /**
   * gnutls_x509_qualifier_t:
   * @GNUTLS_X509_QUALIFIER_UNKNOWN: Unknown qualifier.
   * @GNUTLS_X509_QUALIFIER_URI: A URL
   * @GNUTLS_X509_QUALIFIER_NOICE: A text notice.
   *
   * Enumeration of types for the X.509 qualifiers, of the certificate policy extension. 
   */
typedef enum gnutls_x509_qualifier_t {
	GNUTLS_X509_QUALIFIER_UNKNOWN = 0, GNUTLS_X509_QUALIFIER_URI,
	GNUTLS_X509_QUALIFIER_NOTICE
} gnutls_x509_qualifier_t;

typedef struct gnutls_x509_policy_st {
	char *oid;
	unsigned int qualifiers;
	struct {
		gnutls_x509_qualifier_t type;
		char *data;
		unsigned int size;
	} qualifier[GNUTLS_MAX_QUALIFIERS];
} gnutls_x509_policy_st;

void gnutls_x509_policy_release(struct gnutls_x509_policy_st
				*policy);
int gnutls_x509_crt_get_policy(gnutls_x509_crt_t crt, int indx, struct gnutls_x509_policy_st
			       *policy, unsigned int *critical);
int gnutls_x509_crt_set_policy(gnutls_x509_crt_t crt, struct gnutls_x509_policy_st
			       *policy, unsigned int critical);

int gnutls_x509_dn_oid_known(const char *oid);

#define GNUTLS_X509_DN_OID_RETURN_OID 1
const char *gnutls_x509_dn_oid_name(const char *oid, unsigned int flags);

	/* Read extensions by OID. */
int gnutls_x509_crt_get_extension_oid(gnutls_x509_crt_t cert,
				      int indx, void *oid,
				      size_t * oid_size);
int gnutls_x509_crt_get_extension_by_oid(gnutls_x509_crt_t cert,
					 const char *oid, int indx,
					 void *buf,
					 size_t * buf_size,
					 unsigned int *critical);

	/* Read extensions by sequence number. */
int gnutls_x509_crt_get_extension_info(gnutls_x509_crt_t cert,
				       int indx, void *oid,
				       size_t * oid_size,
				       unsigned int *critical);
int gnutls_x509_crt_get_extension_data(gnutls_x509_crt_t cert,
				       int indx, void *data,
				       size_t * sizeof_data);

int gnutls_x509_crt_set_extension_by_oid(gnutls_x509_crt_t crt,
					 const char *oid,
					 const void *buf,
					 size_t sizeof_buf,
					 unsigned int critical);

/* X.509 Certificate writing.
 */
int gnutls_x509_crt_set_dn(gnutls_x509_crt_t crt, const char *dn,
			   const char **err);

int gnutls_x509_crt_set_dn_by_oid(gnutls_x509_crt_t crt,
				  const char *oid,
				  unsigned int raw_flag,
				  const void *name,
				  unsigned int sizeof_name);
int gnutls_x509_crt_set_issuer_dn_by_oid(gnutls_x509_crt_t crt,
					 const char *oid,
					 unsigned int raw_flag,
					 const void *name,
					 unsigned int sizeof_name);
int gnutls_x509_crt_set_issuer_dn(gnutls_x509_crt_t crt,
				  const char *dn, const char **err);

int gnutls_x509_crt_set_version(gnutls_x509_crt_t crt,
				unsigned int version);
int gnutls_x509_crt_set_key(gnutls_x509_crt_t crt,
			    gnutls_x509_privkey_t key);
int gnutls_x509_crt_set_ca_status(gnutls_x509_crt_t crt, unsigned int ca);
int gnutls_x509_crt_set_basic_constraints(gnutls_x509_crt_t crt,
					  unsigned int ca,
					  int pathLenConstraint);
int gnutls_x509_crt_set_subject_alternative_name(gnutls_x509_crt_t
						 crt,
						 gnutls_x509_subject_alt_name_t
						 type, const char
						 *data_string);
int gnutls_x509_crt_set_subject_alt_name(gnutls_x509_crt_t crt,
					 gnutls_x509_subject_alt_name_t
					 type, const void *data,
					 unsigned int data_size,
					 unsigned int flags);
int gnutls_x509_crt_sign(gnutls_x509_crt_t crt,
			 gnutls_x509_crt_t issuer,
			 gnutls_x509_privkey_t issuer_key);
int gnutls_x509_crt_sign2(gnutls_x509_crt_t crt,
			  gnutls_x509_crt_t issuer,
			  gnutls_x509_privkey_t issuer_key,
			  gnutls_digest_algorithm_t dig,
			  unsigned int flags);
int gnutls_x509_crt_set_activation_time(gnutls_x509_crt_t cert,
					time_t act_time);
int gnutls_x509_crt_set_expiration_time(gnutls_x509_crt_t cert,
					time_t exp_time);
int gnutls_x509_crt_set_serial(gnutls_x509_crt_t cert,
			       const void *serial, size_t serial_size);

int gnutls_x509_crt_set_subject_key_id(gnutls_x509_crt_t cert,
				       const void *id, size_t id_size);

int gnutls_x509_crt_set_proxy_dn(gnutls_x509_crt_t crt,
				 gnutls_x509_crt_t eecrt,
				 unsigned int raw_flag,
				 const void *name,
				 unsigned int sizeof_name);
int gnutls_x509_crt_set_proxy(gnutls_x509_crt_t crt,
			      int pathLenConstraint,
			      const char *policyLanguage,
			      const char *policy, size_t sizeof_policy);

int gnutls_x509_crt_print(gnutls_x509_crt_t cert,
			  gnutls_certificate_print_formats_t
			  format, gnutls_datum_t * out);
int gnutls_x509_crl_print(gnutls_x509_crl_t crl,
			  gnutls_certificate_print_formats_t
			  format, gnutls_datum_t * out);

	/* Access to internal Certificate fields.
	 */
int gnutls_x509_crt_get_raw_issuer_dn(gnutls_x509_crt_t cert,
				      gnutls_datum_t * start);
int gnutls_x509_crt_get_raw_dn(gnutls_x509_crt_t cert,
			       gnutls_datum_t * start);

/* RDN handling.
 */
int gnutls_x509_rdn_get(const gnutls_datum_t * idn,
			char *buf, size_t * sizeof_buf);
int gnutls_x509_rdn_get_oid(const gnutls_datum_t * idn,
			    int indx, void *buf, size_t * sizeof_buf);

int gnutls_x509_rdn_get_by_oid(const gnutls_datum_t * idn,
			       const char *oid, int indx,
			       unsigned int raw_flag, void *buf,
			       size_t * sizeof_buf);

typedef void *gnutls_x509_dn_t;

typedef struct gnutls_x509_ava_st {
	gnutls_datum_t oid;
	gnutls_datum_t value;
	unsigned long value_tag;
} gnutls_x509_ava_st;

int gnutls_x509_crt_get_subject(gnutls_x509_crt_t cert,
				gnutls_x509_dn_t * dn);
int gnutls_x509_crt_get_issuer(gnutls_x509_crt_t cert,
			       gnutls_x509_dn_t * dn);
int gnutls_x509_dn_get_rdn_ava(gnutls_x509_dn_t dn, int irdn,
			       int iava, gnutls_x509_ava_st * ava);

int gnutls_x509_dn_init(gnutls_x509_dn_t * dn);

int gnutls_x509_dn_import(gnutls_x509_dn_t dn,
			  const gnutls_datum_t * data);

int gnutls_x509_dn_export(gnutls_x509_dn_t dn,
			  gnutls_x509_crt_fmt_t format,
			  void *output_data, size_t * output_data_size);
int gnutls_x509_dn_export2(gnutls_x509_dn_t dn,
			   gnutls_x509_crt_fmt_t format,
			   gnutls_datum_t * out);

void gnutls_x509_dn_deinit(gnutls_x509_dn_t dn);


/* CRL handling functions.
 */
int gnutls_x509_crl_init(gnutls_x509_crl_t * crl);
void gnutls_x509_crl_deinit(gnutls_x509_crl_t crl);

int gnutls_x509_crl_import(gnutls_x509_crl_t crl,
			   const gnutls_datum_t * data,
			   gnutls_x509_crt_fmt_t format);
int gnutls_x509_crl_export(gnutls_x509_crl_t crl,
			   gnutls_x509_crt_fmt_t format,
			   void *output_data, size_t * output_data_size);
int gnutls_x509_crl_export2(gnutls_x509_crl_t crl,
			    gnutls_x509_crt_fmt_t format,
			    gnutls_datum_t * out);

int
gnutls_x509_crl_get_raw_issuer_dn(gnutls_x509_crl_t crl,
				  gnutls_datum_t * dn);

int gnutls_x509_crl_get_issuer_dn(gnutls_x509_crl_t crl,
				  char *buf, size_t * sizeof_buf);
int gnutls_x509_crl_get_issuer_dn2(gnutls_x509_crl_t crl,
				   gnutls_datum_t * dn);
int gnutls_x509_crl_get_issuer_dn_by_oid(gnutls_x509_crl_t crl,
					 const char *oid, int indx,
					 unsigned int raw_flag,
					 void *buf, size_t * sizeof_buf);
int gnutls_x509_crl_get_dn_oid(gnutls_x509_crl_t crl, int indx,
			       void *oid, size_t * sizeof_oid);

int gnutls_x509_crl_get_signature_algorithm(gnutls_x509_crl_t crl);
int gnutls_x509_crl_get_signature(gnutls_x509_crl_t crl,
				  char *sig, size_t * sizeof_sig);
int gnutls_x509_crl_get_version(gnutls_x509_crl_t crl);

time_t gnutls_x509_crl_get_this_update(gnutls_x509_crl_t crl);
time_t gnutls_x509_crl_get_next_update(gnutls_x509_crl_t crl);

int gnutls_x509_crl_get_crt_count(gnutls_x509_crl_t crl);
int gnutls_x509_crl_get_crt_serial(gnutls_x509_crl_t crl, int indx,
				   unsigned char *serial,
				   size_t * serial_size, time_t * t);
#define gnutls_x509_crl_get_certificate_count gnutls_x509_crl_get_crt_count
#define gnutls_x509_crl_get_certificate gnutls_x509_crl_get_crt_serial

int gnutls_x509_crl_check_issuer(gnutls_x509_crl_t crl,
				 gnutls_x509_crt_t issuer);

int gnutls_x509_crl_list_import2(gnutls_x509_crl_t ** crls,
				 unsigned int *size,
				 const gnutls_datum_t * data,
				 gnutls_x509_crt_fmt_t format,
				 unsigned int flags);

int gnutls_x509_crl_list_import(gnutls_x509_crl_t * crls,
				unsigned int *crl_max,
				const gnutls_datum_t * data,
				gnutls_x509_crt_fmt_t format,
				unsigned int flags);
/* CRL writing.
 */
int gnutls_x509_crl_set_version(gnutls_x509_crl_t crl,
				unsigned int version);
int gnutls_x509_crl_set_this_update(gnutls_x509_crl_t crl,
				    time_t act_time);
int gnutls_x509_crl_set_next_update(gnutls_x509_crl_t crl,
				    time_t exp_time);
int gnutls_x509_crl_set_crt_serial(gnutls_x509_crl_t crl,
				   const void *serial,
				   size_t serial_size,
				   time_t revocation_time);
int gnutls_x509_crl_set_crt(gnutls_x509_crl_t crl,
			    gnutls_x509_crt_t crt, time_t revocation_time);

int gnutls_x509_crl_get_authority_key_id(gnutls_x509_crl_t crl,
					 void *id,
					 size_t * id_size,
					 unsigned int *critical);
int gnutls_x509_crl_get_authority_key_gn_serial(gnutls_x509_crl_t
						crl,
						unsigned int seq,
						void *alt,
						size_t * alt_size,
						unsigned int
						*alt_type,
						void *serial,
						size_t *
						serial_size, unsigned int
						*critical);

int gnutls_x509_crl_get_number(gnutls_x509_crl_t crl, void *ret,
			       size_t * ret_size, unsigned int *critical);

int gnutls_x509_crl_get_extension_oid(gnutls_x509_crl_t crl,
				      int indx, void *oid,
				      size_t * sizeof_oid);

int gnutls_x509_crl_get_extension_info(gnutls_x509_crl_t crl,
				       int indx, void *oid,
				       size_t * sizeof_oid,
				       unsigned int *critical);

int gnutls_x509_crl_get_extension_data(gnutls_x509_crl_t crl,
				       int indx, void *data,
				       size_t * sizeof_data);

int gnutls_x509_crl_set_authority_key_id(gnutls_x509_crl_t crl,
					 const void *id, size_t id_size);

int gnutls_x509_crl_set_number(gnutls_x509_crl_t crl,
			       const void *nr, size_t nr_size);


/* PKCS7 structures handling
 */
struct gnutls_pkcs7_int;
typedef struct gnutls_pkcs7_int *gnutls_pkcs7_t;

int gnutls_pkcs7_init(gnutls_pkcs7_t * pkcs7);
void gnutls_pkcs7_deinit(gnutls_pkcs7_t pkcs7);
int gnutls_pkcs7_import(gnutls_pkcs7_t pkcs7,
			const gnutls_datum_t * data,
			gnutls_x509_crt_fmt_t format);
int gnutls_pkcs7_export(gnutls_pkcs7_t pkcs7,
			gnutls_x509_crt_fmt_t format,
			void *output_data, size_t * output_data_size);
int gnutls_pkcs7_export2(gnutls_pkcs7_t pkcs7,
			 gnutls_x509_crt_fmt_t format,
			 gnutls_datum_t * out);

int gnutls_pkcs7_get_crt_count(gnutls_pkcs7_t pkcs7);
int gnutls_pkcs7_get_crt_raw(gnutls_pkcs7_t pkcs7, int indx,
			     void *certificate, size_t * certificate_size);

int gnutls_pkcs7_set_crt_raw(gnutls_pkcs7_t pkcs7,
			     const gnutls_datum_t * crt);
int gnutls_pkcs7_set_crt(gnutls_pkcs7_t pkcs7, gnutls_x509_crt_t crt);
int gnutls_pkcs7_delete_crt(gnutls_pkcs7_t pkcs7, int indx);

int gnutls_pkcs7_get_crl_raw(gnutls_pkcs7_t pkcs7,
			     int indx, void *crl, size_t * crl_size);
int gnutls_pkcs7_get_crl_count(gnutls_pkcs7_t pkcs7);

int gnutls_pkcs7_set_crl_raw(gnutls_pkcs7_t pkcs7,
			     const gnutls_datum_t * crl);
int gnutls_pkcs7_set_crl(gnutls_pkcs7_t pkcs7, gnutls_x509_crl_t crl);
int gnutls_pkcs7_delete_crl(gnutls_pkcs7_t pkcs7, int indx);

/* X.509 Certificate verification functions.
 */

/**
 * gnutls_certificate_verify_flags:
 * @GNUTLS_VERIFY_DISABLE_CA_SIGN: If set a signer does not have to be
 *   a certificate authority. This flag should normally be disabled,
 *   unless you know what this means.
 * @GNUTLS_VERIFY_DISABLE_TRUSTED_TIME_CHECKS: If set a signer in the trusted
 *   list is never checked for expiration or activation.
 * @GNUTLS_VERIFY_ALLOW_X509_V1_CA_CRT: Allow trusted CA certificates
 *   with version 1. This is safer than %GNUTLS_VERIFY_ALLOW_ANY_X509_V1_CA_CRT,
 *   and should be used instead. That way only signers in your trusted list
 *   will be allowed to have certificates of version 1. This is the default.
 * @GNUTLS_VERIFY_DO_NOT_ALLOW_X509_V1_CA_CRT: Do not allow trusted CA
 *   certificates that have version 1.  This option is to be used
 *   to deprecate all certificates of version 1.
 * @GNUTLS_VERIFY_DO_NOT_ALLOW_SAME: If a certificate is not signed by
 *   anyone trusted but exists in the trusted CA list do not treat it
 *   as trusted.
 * @GNUTLS_VERIFY_ALLOW_UNSORTED_CHAIN: A certificate chain is tolerated
 *   if unsorted (the case with many TLS servers out there). This is the
 *   default since GnuTLS 3.1.4.
 * @GNUTLS_VERIFY_DO_NOT_ALLOW_UNSORTED_CHAIN: Do not tolerate an unsorted
 *   certificate chain.
 * @GNUTLS_VERIFY_ALLOW_ANY_X509_V1_CA_CRT: Allow CA certificates that
 *   have version 1 (both root and intermediate). This might be
 *   dangerous since those haven't the basicConstraints
 *   extension. Must be used in combination with
 *   %GNUTLS_VERIFY_ALLOW_X509_V1_CA_CRT.
 * @GNUTLS_VERIFY_ALLOW_SIGN_RSA_MD2: Allow certificates to be signed
 *   using the broken MD2 algorithm.
 * @GNUTLS_VERIFY_ALLOW_SIGN_RSA_MD5: Allow certificates to be signed
 *   using the broken MD5 algorithm.
 * @GNUTLS_VERIFY_DISABLE_TIME_CHECKS: Disable checking of activation
 *   and expiration validity periods of certificate chains. Don't set
 *   this unless you understand the security implications.
 * @GNUTLS_VERIFY_DISABLE_CRL_CHECKS: Disable checking for validity
 *   using certificate revocation lists or the available OCSP data.
 *
 * Enumeration of different certificate verify flags.
 */
typedef enum gnutls_certificate_verify_flags {
	GNUTLS_VERIFY_DISABLE_CA_SIGN = 1 << 0,
	GNUTLS_VERIFY_ALLOW_X509_V1_CA_CRT = 1 << 1,
	GNUTLS_VERIFY_DO_NOT_ALLOW_SAME = 1 << 2,
	GNUTLS_VERIFY_ALLOW_ANY_X509_V1_CA_CRT = 1 << 3,
	GNUTLS_VERIFY_ALLOW_SIGN_RSA_MD2 = 1 << 4,
	GNUTLS_VERIFY_ALLOW_SIGN_RSA_MD5 = 1 << 5,
	GNUTLS_VERIFY_DISABLE_TIME_CHECKS = 1 << 6,
	GNUTLS_VERIFY_DISABLE_TRUSTED_TIME_CHECKS = 1 << 7,
	GNUTLS_VERIFY_DO_NOT_ALLOW_X509_V1_CA_CRT = 1 << 8,
	GNUTLS_VERIFY_DISABLE_CRL_CHECKS = 1 << 9,
	GNUTLS_VERIFY_ALLOW_UNSORTED_CHAIN = 1 << 10,
	GNUTLS_VERIFY_DO_NOT_ALLOW_UNSORTED_CHAIN = 1 << 11,
} gnutls_certificate_verify_flags;

int gnutls_x509_crt_check_issuer(gnutls_x509_crt_t cert,
				 gnutls_x509_crt_t issuer);

int gnutls_x509_crt_list_verify(const gnutls_x509_crt_t *
				cert_list, int cert_list_length,
				const gnutls_x509_crt_t * CA_list,
				int CA_list_length,
				const gnutls_x509_crl_t * CRL_list,
				int CRL_list_length,
				unsigned int flags, unsigned int *verify);

int gnutls_x509_crt_verify(gnutls_x509_crt_t cert,
			   const gnutls_x509_crt_t * CA_list,
			   int CA_list_length, unsigned int flags,
			   unsigned int *verify);
int gnutls_x509_crl_verify(gnutls_x509_crl_t crl,
			   const gnutls_x509_crt_t * CA_list,
			   int CA_list_length, unsigned int flags,
			   unsigned int *verify);

int gnutls_x509_crt_check_revocation(gnutls_x509_crt_t cert,
				     const gnutls_x509_crl_t *
				     crl_list, int crl_list_length);

int gnutls_x509_crt_get_fingerprint(gnutls_x509_crt_t cert,
				    gnutls_digest_algorithm_t algo,
				    void *buf, size_t * buf_size);

int gnutls_x509_crt_get_key_purpose_oid(gnutls_x509_crt_t cert,
					int indx, void *oid,
					size_t * oid_size,
					unsigned int *critical);
int gnutls_x509_crt_set_key_purpose_oid(gnutls_x509_crt_t cert,
					const void *oid,
					unsigned int critical);

/* Private key handling.
 */

/* Flags for the gnutls_x509_privkey_export_pkcs8() function.
 */

#define GNUTLS_PKCS8_PLAIN GNUTLS_PKCS_PLAIN
#define GNUTLS_PKCS8_USE_PKCS12_3DES GNUTLS_PKCS_USE_PKCS12_3DES
#define GNUTLS_PKCS8_USE_PKCS12_ARCFOUR GNUTLS_PKCS_USE_PKCS12_ARCFOUR
#define GNUTLS_PKCS8_USE_PKCS12_RC2_40 GNUTLS_PKCS_USE_PKCS12_RC2_40

/**
 * gnutls_pkcs_encrypt_flags_t:
 * @GNUTLS_PKCS_PLAIN: Unencrypted private key.
 * @GNUTLS_PKCS_NULL_PASSWORD: Some schemas distinguish between an empty and a NULL password.
 * @GNUTLS_PKCS_USE_PKCS12_3DES: PKCS-12 3DES.
 * @GNUTLS_PKCS_USE_PKCS12_ARCFOUR: PKCS-12 ARCFOUR.
 * @GNUTLS_PKCS_USE_PKCS12_RC2_40: PKCS-12 RC2-40.
 * @GNUTLS_PKCS_USE_PBES2_3DES: PBES2 3DES.
 * @GNUTLS_PKCS_USE_PBES2_AES_128: PBES2 AES-128.
 * @GNUTLS_PKCS_USE_PBES2_AES_192: PBES2 AES-192.
 * @GNUTLS_PKCS_USE_PBES2_AES_256: PBES2 AES-256.
 *
 * Enumeration of different PKCS encryption flags.
 */
typedef enum gnutls_pkcs_encrypt_flags_t {
	GNUTLS_PKCS_PLAIN = 1,
	GNUTLS_PKCS_USE_PKCS12_3DES = 2,
	GNUTLS_PKCS_USE_PKCS12_ARCFOUR = 4,
	GNUTLS_PKCS_USE_PKCS12_RC2_40 = 8,
	GNUTLS_PKCS_USE_PBES2_3DES = 16,
	GNUTLS_PKCS_USE_PBES2_AES_128 = 32,
	GNUTLS_PKCS_USE_PBES2_AES_192 = 64,
	GNUTLS_PKCS_USE_PBES2_AES_256 = 128,
	GNUTLS_PKCS_NULL_PASSWORD = 256
} gnutls_pkcs_encrypt_flags_t;

int gnutls_x509_privkey_init(gnutls_x509_privkey_t * key);
void gnutls_x509_privkey_deinit(gnutls_x509_privkey_t key);
gnutls_sec_param_t
gnutls_x509_privkey_sec_param(gnutls_x509_privkey_t key);
int gnutls_x509_privkey_cpy(gnutls_x509_privkey_t dst,
			    gnutls_x509_privkey_t src);
int gnutls_x509_privkey_import(gnutls_x509_privkey_t key,
			       const gnutls_datum_t * data,
			       gnutls_x509_crt_fmt_t format);
int gnutls_x509_privkey_import_pkcs8(gnutls_x509_privkey_t key,
				     const gnutls_datum_t * data,
				     gnutls_x509_crt_fmt_t format,
				     const char *password,
				     unsigned int flags);
int gnutls_x509_privkey_import_openssl(gnutls_x509_privkey_t key,
				       const gnutls_datum_t * data,
				       const char *password);

int gnutls_x509_privkey_import2(gnutls_x509_privkey_t key,
				const gnutls_datum_t * data,
				gnutls_x509_crt_fmt_t format,
				const char *password, unsigned int flags);

int gnutls_x509_privkey_import_rsa_raw(gnutls_x509_privkey_t key,
				       const gnutls_datum_t * m,
				       const gnutls_datum_t * e,
				       const gnutls_datum_t * d,
				       const gnutls_datum_t * p,
				       const gnutls_datum_t * q,
				       const gnutls_datum_t * u);
int gnutls_x509_privkey_import_rsa_raw2(gnutls_x509_privkey_t key,
					const gnutls_datum_t * m,
					const gnutls_datum_t * e,
					const gnutls_datum_t * d,
					const gnutls_datum_t * p,
					const gnutls_datum_t * q,
					const gnutls_datum_t * u,
					const gnutls_datum_t * e1,
					const gnutls_datum_t * e2);
int gnutls_x509_privkey_import_ecc_raw(gnutls_x509_privkey_t key,
				       gnutls_ecc_curve_t curve,
				       const gnutls_datum_t * x,
				       const gnutls_datum_t * y,
				       const gnutls_datum_t * k);

int gnutls_x509_privkey_fix(gnutls_x509_privkey_t key);

int gnutls_x509_privkey_export_dsa_raw(gnutls_x509_privkey_t key,
				       gnutls_datum_t * p,
				       gnutls_datum_t * q,
				       gnutls_datum_t * g,
				       gnutls_datum_t * y,
				       gnutls_datum_t * x);
int gnutls_x509_privkey_import_dsa_raw(gnutls_x509_privkey_t key,
				       const gnutls_datum_t * p,
				       const gnutls_datum_t * q,
				       const gnutls_datum_t * g,
				       const gnutls_datum_t * y,
				       const gnutls_datum_t * x);

int gnutls_x509_privkey_get_pk_algorithm(gnutls_x509_privkey_t key);
int gnutls_x509_privkey_get_pk_algorithm2(gnutls_x509_privkey_t
					  key, unsigned int *bits);
int gnutls_x509_privkey_get_key_id(gnutls_x509_privkey_t key,
				   unsigned int flags,
				   unsigned char *output_data,
				   size_t * output_data_size);

int gnutls_x509_privkey_generate(gnutls_x509_privkey_t key,
				 gnutls_pk_algorithm_t algo,
				 unsigned int bits, unsigned int flags);
int gnutls_x509_privkey_verify_params(gnutls_x509_privkey_t key);

int gnutls_x509_privkey_export(gnutls_x509_privkey_t key,
			       gnutls_x509_crt_fmt_t format,
			       void *output_data,
			       size_t * output_data_size);
int gnutls_x509_privkey_export2(gnutls_x509_privkey_t key,
				gnutls_x509_crt_fmt_t format,
				gnutls_datum_t * out);
int gnutls_x509_privkey_export_pkcs8(gnutls_x509_privkey_t key,
				     gnutls_x509_crt_fmt_t format,
				     const char *password,
				     unsigned int flags,
				     void *output_data,
				     size_t * output_data_size);
int gnutls_x509_privkey_export2_pkcs8(gnutls_x509_privkey_t key,
				      gnutls_x509_crt_fmt_t format,
				      const char *password,
				      unsigned int flags,
				      gnutls_datum_t * out);
int gnutls_x509_privkey_export_rsa_raw2(gnutls_x509_privkey_t key,
					gnutls_datum_t * m,
					gnutls_datum_t * e,
					gnutls_datum_t * d,
					gnutls_datum_t * p,
					gnutls_datum_t * q,
					gnutls_datum_t * u,
					gnutls_datum_t * e1,
					gnutls_datum_t * e2);
int gnutls_x509_privkey_export_rsa_raw(gnutls_x509_privkey_t key,
				       gnutls_datum_t * m,
				       gnutls_datum_t * e,
				       gnutls_datum_t * d,
				       gnutls_datum_t * p,
				       gnutls_datum_t * q,
				       gnutls_datum_t * u);
int gnutls_x509_privkey_export_ecc_raw(gnutls_x509_privkey_t key,
				       gnutls_ecc_curve_t * curve,
				       gnutls_datum_t * x,
				       gnutls_datum_t * y,
				       gnutls_datum_t * k);
/* Certificate request stuff.
 */

int gnutls_x509_crq_sign2(gnutls_x509_crq_t crq,
			  gnutls_x509_privkey_t key,
			  gnutls_digest_algorithm_t dig,
			  unsigned int flags);

int gnutls_x509_crq_print(gnutls_x509_crq_t crq,
			  gnutls_certificate_print_formats_t
			  format, gnutls_datum_t * out);

int gnutls_x509_crq_verify(gnutls_x509_crq_t crq, unsigned int flags);

int gnutls_x509_crq_init(gnutls_x509_crq_t * crq);
void gnutls_x509_crq_deinit(gnutls_x509_crq_t crq);
int gnutls_x509_crq_import(gnutls_x509_crq_t crq,
			   const gnutls_datum_t * data,
			   gnutls_x509_crt_fmt_t format);

int gnutls_x509_crq_get_private_key_usage_period(gnutls_x509_crq_t
						 cert,
						 time_t *
						 activation,
						 time_t *
						 expiration, unsigned int
						 *critical);

int gnutls_x509_crq_get_dn(gnutls_x509_crq_t crq, char *buf,
			   size_t * sizeof_buf);
int gnutls_x509_crq_get_dn2(gnutls_x509_crq_t crq, gnutls_datum_t * dn);
int gnutls_x509_crq_get_dn_oid(gnutls_x509_crq_t crq, int indx,
			       void *oid, size_t * sizeof_oid);
int gnutls_x509_crq_get_dn_by_oid(gnutls_x509_crq_t crq,
				  const char *oid, int indx,
				  unsigned int raw_flag, void *buf,
				  size_t * sizeof_buf);
int gnutls_x509_crq_set_dn(gnutls_x509_crq_t crq, const char *dn,
			   const char **err);
int gnutls_x509_crq_set_dn_by_oid(gnutls_x509_crq_t crq,
				  const char *oid,
				  unsigned int raw_flag,
				  const void *data,
				  unsigned int sizeof_data);
int gnutls_x509_crq_set_version(gnutls_x509_crq_t crq,
				unsigned int version);
int gnutls_x509_crq_get_version(gnutls_x509_crq_t crq);
int gnutls_x509_crq_set_key(gnutls_x509_crq_t crq,
			    gnutls_x509_privkey_t key);

int gnutls_x509_crq_set_challenge_password(gnutls_x509_crq_t crq,
					   const char *pass);
int gnutls_x509_crq_get_challenge_password(gnutls_x509_crq_t crq,
					   char *pass,
					   size_t * sizeof_pass);

int gnutls_x509_crq_set_attribute_by_oid(gnutls_x509_crq_t crq,
					 const char *oid,
					 void *buf, size_t sizeof_buf);
int gnutls_x509_crq_get_attribute_by_oid(gnutls_x509_crq_t crq,
					 const char *oid, int indx,
					 void *buf, size_t * sizeof_buf);

int gnutls_x509_crq_export(gnutls_x509_crq_t crq,
			   gnutls_x509_crt_fmt_t format,
			   void *output_data, size_t * output_data_size);
int gnutls_x509_crq_export2(gnutls_x509_crq_t crq,
			    gnutls_x509_crt_fmt_t format,
			    gnutls_datum_t * out);

int gnutls_x509_crt_set_crq(gnutls_x509_crt_t crt, gnutls_x509_crq_t crq);
int gnutls_x509_crt_set_crq_extensions(gnutls_x509_crt_t crt,
				       gnutls_x509_crq_t crq);

int gnutls_x509_crq_set_private_key_usage_period(gnutls_x509_crq_t
						 crq,
						 time_t activation,
						 time_t expiration);
int gnutls_x509_crq_set_key_rsa_raw(gnutls_x509_crq_t crq,
				    const gnutls_datum_t * m,
				    const gnutls_datum_t * e);
int gnutls_x509_crq_set_subject_alt_name(gnutls_x509_crq_t crq,
					 gnutls_x509_subject_alt_name_t
					 nt, const void *data,
					 unsigned int data_size,
					 unsigned int flags);

int gnutls_x509_crq_set_key_usage(gnutls_x509_crq_t crq,
				  unsigned int usage);
int gnutls_x509_crq_set_basic_constraints(gnutls_x509_crq_t crq,
					  unsigned int ca,
					  int pathLenConstraint);
int gnutls_x509_crq_set_key_purpose_oid(gnutls_x509_crq_t crq,
					const void *oid,
					unsigned int critical);
int gnutls_x509_crq_get_key_purpose_oid(gnutls_x509_crq_t crq,
					int indx, void *oid,
					size_t * sizeof_oid,
					unsigned int *critical);

int gnutls_x509_crq_get_extension_data(gnutls_x509_crq_t crq,
				       int indx, void *data,
				       size_t * sizeof_data);
int gnutls_x509_crq_get_extension_info(gnutls_x509_crq_t crq,
				       int indx, void *oid,
				       size_t * sizeof_oid,
				       unsigned int *critical);
int gnutls_x509_crq_get_attribute_data(gnutls_x509_crq_t crq,
				       int indx, void *data,
				       size_t * sizeof_data);
int gnutls_x509_crq_get_attribute_info(gnutls_x509_crq_t crq,
				       int indx, void *oid,
				       size_t * sizeof_oid);
int gnutls_x509_crq_get_pk_algorithm(gnutls_x509_crq_t crq,
				     unsigned int *bits);

int gnutls_x509_crq_get_key_id(gnutls_x509_crq_t crq,
			       unsigned int flags,
			       unsigned char *output_data,
			       size_t * output_data_size);
int gnutls_x509_crq_get_key_rsa_raw(gnutls_x509_crq_t crq,
				    gnutls_datum_t * m,
				    gnutls_datum_t * e);

int gnutls_x509_crq_get_key_usage(gnutls_x509_crq_t crq,
				  unsigned int *key_usage,
				  unsigned int *critical);
int gnutls_x509_crq_get_basic_constraints(gnutls_x509_crq_t crq,
					  unsigned int *critical,
					  unsigned int *ca, int *pathlen);
int gnutls_x509_crq_get_subject_alt_name(gnutls_x509_crq_t crq,
					 unsigned int seq,
					 void *ret,
					 size_t * ret_size,
					 unsigned int *ret_type,
					 unsigned int *critical);
int gnutls_x509_crq_get_subject_alt_othername_oid(gnutls_x509_crq_t
						  crq,
						  unsigned int seq,
						  void *ret,
						  size_t * ret_size);

int gnutls_x509_crq_get_extension_by_oid(gnutls_x509_crq_t crq,
					 const char *oid, int indx,
					 void *buf,
					 size_t * sizeof_buf,
					 unsigned int *critical);

typedef struct gnutls_x509_trust_list_st *gnutls_x509_trust_list_t;

int
gnutls_x509_trust_list_init(gnutls_x509_trust_list_t * list,
			    unsigned int size);

void
gnutls_x509_trust_list_deinit(gnutls_x509_trust_list_t list,
			      unsigned int all);

int gnutls_x509_trust_list_get_issuer(gnutls_x509_trust_list_t
				      list, gnutls_x509_crt_t cert,
				      gnutls_x509_crt_t * issuer,
				      unsigned int flags);

int
gnutls_x509_trust_list_add_cas(gnutls_x509_trust_list_t list,
			       const gnutls_x509_crt_t * clist,
			       int clist_size, unsigned int flags);
int gnutls_x509_trust_list_remove_cas(gnutls_x509_trust_list_t
				      list,
				      const gnutls_x509_crt_t *
				      clist, int clist_size);

int gnutls_x509_trust_list_add_named_crt(gnutls_x509_trust_list_t
					 list,
					 gnutls_x509_crt_t cert,
					 const void *name,
					 size_t name_size,
					 unsigned int flags);

#define GNUTLS_TL_VERIFY_CRL 1
int
gnutls_x509_trust_list_add_crls(gnutls_x509_trust_list_t list,
				const gnutls_x509_crl_t *
				crl_list, int crl_size,
				unsigned int flags,
				unsigned int verification_flags);

typedef int gnutls_verify_output_function(gnutls_x509_crt_t cert, gnutls_x509_crt_t issuer,	/* The issuer if verification failed 
												 * because of him. might be null.
												 */
					  gnutls_x509_crl_t crl,	/* The CRL that caused verification failure 
									 * if any. Might be null. 
									 */
					  unsigned int
					  verification_output);

int gnutls_x509_trust_list_verify_named_crt
    (gnutls_x509_trust_list_t list, gnutls_x509_crt_t cert,
     const void *name, size_t name_size, unsigned int flags,
     unsigned int *verify, gnutls_verify_output_function func);

int
gnutls_x509_trust_list_verify_crt(gnutls_x509_trust_list_t list,
				  gnutls_x509_crt_t * cert_list,
				  unsigned int cert_list_size,
				  unsigned int flags,
				  unsigned int *verify,
				  gnutls_verify_output_function func);

	/* trust list convenience functions */
int
gnutls_x509_trust_list_add_trust_mem(gnutls_x509_trust_list_t
				     list,
				     const gnutls_datum_t * cas,
				     const gnutls_datum_t * crls,
				     gnutls_x509_crt_fmt_t type,
				     unsigned int tl_flags,
				     unsigned int tl_vflags);

int
gnutls_x509_trust_list_add_trust_file(gnutls_x509_trust_list_t
				      list, const char *ca_file,
				      const char *crl_file,
				      gnutls_x509_crt_fmt_t type,
				      unsigned int tl_flags,
				      unsigned int tl_vflags);

int
gnutls_x509_trust_list_remove_trust_file(gnutls_x509_trust_list_t
					 list,
					 const char *ca_file,
					 gnutls_x509_crt_fmt_t type);

int
gnutls_x509_trust_list_remove_trust_mem(gnutls_x509_trust_list_t
					list,
					const gnutls_datum_t *
					cas, gnutls_x509_crt_fmt_t type);

int
gnutls_x509_trust_list_add_system_trust(gnutls_x509_trust_list_t
					list,
					unsigned int tl_flags,
					unsigned int tl_vflags);

void gnutls_certificate_set_trust_list
    (gnutls_certificate_credentials_t res,
     gnutls_x509_trust_list_t tlist, unsigned flags);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */
#endif				/* GNUTLS_X509_H */
