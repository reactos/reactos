/*
 * host2str.c
 *
 * conversion routines from the host format
 * to the presentation format (strings)
 *
 * a Net::DNS like library for C
 *
 * (c) NLnet Labs, 2004-2006
 *
 * See the file LICENSE for the license
 */
#include <ldns/config.h>

#include <ldns/ldns.h>

#include <limits.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include <time.h>
#include <sys/time.h>

#ifdef HAVE_SSL
#include <openssl/bn.h>
#include <openssl/rsa.h>
#ifdef USE_DSA
#include <openssl/dsa.h>
#endif
#endif

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

/* Internal helper function */
ldns_edns_option_list*
pkt_edns_data2edns_option_list(const ldns_rdf *edns_data);

/* lookup tables for standard DNS stuff  */

/* Taken from RFC 2535, section 7.  */
ldns_lookup_table ldns_algorithms[] = {
        { LDNS_RSAMD5, "RSAMD5" },
        { LDNS_DH, "DH" },
        { LDNS_DSA, "DSA" },
        { LDNS_ECC, "ECC" },
        { LDNS_RSASHA1, "RSASHA1" },
        { LDNS_DSA_NSEC3, "DSA-NSEC3-SHA1" },
        { LDNS_RSASHA1_NSEC3, "RSASHA1-NSEC3-SHA1" },
	{ LDNS_RSASHA256, "RSASHA256"},
	{ LDNS_RSASHA512, "RSASHA512"},
	{ LDNS_ECC_GOST, "ECC-GOST"},
        { LDNS_ECDSAP256SHA256, "ECDSAP256SHA256"},
        { LDNS_ECDSAP384SHA384, "ECDSAP384SHA384"},
	{ LDNS_ED25519, "ED25519"},
	{ LDNS_ED448, "ED448"},
        { LDNS_INDIRECT, "INDIRECT" },
        { LDNS_PRIVATEDNS, "PRIVATEDNS" },
        { LDNS_PRIVATEOID, "PRIVATEOID" },
        { 0, NULL }
};

/* Hashing algorithms used in the DS record */
ldns_lookup_table ldns_hashes[] = {
        {LDNS_SHA1     , "SHA1" },      /* RFC 4034 */
        {LDNS_SHA256   , "SHA256" },    /* RFC 4509 */
        {LDNS_HASH_GOST, "HASH-GOST" }, /* RFC 5933 */
        {LDNS_SHA384   , "SHA384" },    /* RFC 6605 */
        { 0, NULL }
};

/* Taken from RFC 4398  */
ldns_lookup_table ldns_cert_algorithms[] = {
        { LDNS_CERT_PKIX, "PKIX" },
        { LDNS_CERT_SPKI, "SPKI" },
        { LDNS_CERT_PGP, "PGP" },
        { LDNS_CERT_IPKIX, "IPKIX" },
        { LDNS_CERT_ISPKI, "ISPKI" },
        { LDNS_CERT_IPGP, "IPGP" },
        { LDNS_CERT_ACPKIX, "ACPKIX" },
        { LDNS_CERT_IACPKIX, "IACPKIX" },
        { LDNS_CERT_URI, "URI" },
        { LDNS_CERT_OID, "OID" },
        { 0, NULL }
};

/* classes  */
ldns_lookup_table ldns_rr_classes[] = {
        { LDNS_RR_CLASS_IN, "IN" },
        { LDNS_RR_CLASS_CH, "CH" },
        { LDNS_RR_CLASS_HS, "HS" },
        { LDNS_RR_CLASS_NONE, "NONE" },
        { LDNS_RR_CLASS_ANY, "ANY" },
        { 0, NULL }
};

/* if these are used elsewhere */
ldns_lookup_table ldns_rcodes[] = {
        { LDNS_RCODE_NOERROR, "NOERROR" },
        { LDNS_RCODE_FORMERR, "FORMERR" },
        { LDNS_RCODE_SERVFAIL, "SERVFAIL" },
        { LDNS_RCODE_NXDOMAIN, "NXDOMAIN" },
        { LDNS_RCODE_NOTIMPL, "NOTIMPL" },
        { LDNS_RCODE_REFUSED, "REFUSED" },
        { LDNS_RCODE_YXDOMAIN, "YXDOMAIN" },
        { LDNS_RCODE_YXRRSET, "YXRRSET" },
        { LDNS_RCODE_NXRRSET, "NXRRSET" },
        { LDNS_RCODE_NOTAUTH, "NOTAUTH" },
        { LDNS_RCODE_NOTZONE, "NOTZONE" },
        { 0, NULL }
};

ldns_lookup_table ldns_opcodes[] = {
        { LDNS_PACKET_QUERY, "QUERY" },
        { LDNS_PACKET_IQUERY, "IQUERY" },
        { LDNS_PACKET_STATUS, "STATUS" },
	{ LDNS_PACKET_NOTIFY, "NOTIFY" },
	{ LDNS_PACKET_UPDATE, "UPDATE" },
        { 0, NULL }
};

const ldns_output_format   ldns_output_format_nocomments_record = { 0, NULL };
const ldns_output_format  *ldns_output_format_nocomments 
			= &ldns_output_format_nocomments_record;
const ldns_output_format   ldns_output_format_onlykeyids_record = {
	LDNS_COMMENT_KEY, NULL
};
const ldns_output_format  *ldns_output_format_onlykeyids
			= &ldns_output_format_onlykeyids_record;
const ldns_output_format  *ldns_output_format_default
			= &ldns_output_format_onlykeyids_record;

const ldns_output_format   ldns_output_format_bubblebabble_record = { 
	LDNS_COMMENT_KEY | LDNS_COMMENT_BUBBLEBABBLE | LDNS_COMMENT_FLAGS, NULL
};
const ldns_output_format  *ldns_output_format_bubblebabble 
			= &ldns_output_format_bubblebabble_record;

static bool
ldns_output_format_covers_type(const ldns_output_format* fmt, ldns_rr_type t)
{
	return fmt && (fmt->flags & LDNS_FMT_RFC3597) &&
		((ldns_output_format_storage*)fmt)->bitmap &&
		ldns_nsec_bitmap_covers_type(
				((ldns_output_format_storage*)fmt)->bitmap, t);
}

ldns_status
ldns_output_format_set_type(ldns_output_format* fmt, ldns_rr_type t)
{
	ldns_output_format_storage* fmt_st = (ldns_output_format_storage*)fmt;
	ldns_status s;
	
	assert(fmt != NULL);
	
	if (!(fmt_st->flags & LDNS_FMT_RFC3597)) {
		ldns_output_format_set(fmt, LDNS_FMT_RFC3597);
	}
	if (! fmt_st->bitmap) {
		s = ldns_rdf_bitmap_known_rr_types_space(&fmt_st->bitmap);
		if (s != LDNS_STATUS_OK) {
			return s;
		}
	}
	return ldns_nsec_bitmap_set_type(fmt_st->bitmap, t);
}

ldns_status
ldns_output_format_clear_type(ldns_output_format* fmt, ldns_rr_type t)
{
	ldns_output_format_storage* fmt_st = (ldns_output_format_storage*)fmt;
	ldns_status s;
	
	assert(fmt != NULL);

	if (!(fmt_st->flags & LDNS_FMT_RFC3597)) {
		ldns_output_format_set(fmt, LDNS_FMT_RFC3597);
	}
	if (! fmt_st->bitmap) {
		s = ldns_rdf_bitmap_known_rr_types(&fmt_st->bitmap);
		if (s != LDNS_STATUS_OK) {
			return s;
		}
	}
	return ldns_nsec_bitmap_clear_type(fmt_st->bitmap, t);
}

ldns_status
ldns_pkt_opcode2buffer_str(ldns_buffer *output, ldns_pkt_opcode opcode)
{
	ldns_lookup_table *lt = ldns_lookup_by_id(ldns_opcodes, opcode);
	if (lt && lt->name) {
		ldns_buffer_printf(output, "%s", lt->name);
	} else {
		ldns_buffer_printf(output, "OPCODE%u", opcode);
	}
	return ldns_buffer_status(output);
}

ldns_status
ldns_pkt_rcode2buffer_str(ldns_buffer *output, ldns_pkt_rcode rcode)
{
	ldns_lookup_table *lt = ldns_lookup_by_id(ldns_rcodes, rcode);
	if (lt && lt->name) {
		ldns_buffer_printf(output, "%s", lt->name);
	} else {
		ldns_buffer_printf(output, "RCODE%u", rcode);
	}
	return ldns_buffer_status(output);
}

ldns_status
ldns_algorithm2buffer_str(ldns_buffer *output,
                          ldns_algorithm algorithm)
{
	ldns_lookup_table *lt = ldns_lookup_by_id(ldns_algorithms,
	                                          algorithm);
	if (lt && lt->name) {
		ldns_buffer_printf(output, "%s", lt->name);
	} else {
		ldns_buffer_printf(output, "ALG%u", algorithm);
	}
	return ldns_buffer_status(output);
}

ldns_status
ldns_cert_algorithm2buffer_str(ldns_buffer *output,
                               ldns_cert_algorithm cert_algorithm)
{
	ldns_lookup_table *lt = ldns_lookup_by_id(ldns_cert_algorithms,
	                                          cert_algorithm);
	if (lt && lt->name) {
		ldns_buffer_printf(output, "%s", lt->name);
	} else {
		ldns_buffer_printf(output, "CERT_ALG%u",
		                   cert_algorithm);
	}
	return ldns_buffer_status(output);
}

char *
ldns_pkt_opcode2str(ldns_pkt_opcode opcode)
{
	char *str;
	ldns_buffer *buf;

	buf = ldns_buffer_new(12);
	if (!buf) {
		return NULL;
	}

	str = NULL;
	if (ldns_pkt_opcode2buffer_str(buf, opcode) == LDNS_STATUS_OK) {
		str = ldns_buffer_export2str(buf);
	}

	ldns_buffer_free(buf);
	return str;
}

char *
ldns_pkt_rcode2str(ldns_pkt_rcode rcode)
{
	char *str;
	ldns_buffer *buf;

	buf = ldns_buffer_new(10);
	if (!buf) {
		return NULL;
	}

	str = NULL;
	if (ldns_pkt_rcode2buffer_str(buf, rcode) == LDNS_STATUS_OK) {
		str = ldns_buffer_export2str(buf);
	}

	ldns_buffer_free(buf);
	return str;
}

char *
ldns_pkt_algorithm2str(ldns_algorithm algorithm)
{
	char *str;
	ldns_buffer *buf;

	buf = ldns_buffer_new(10);
	if (!buf) {
		return NULL;
	}

	str = NULL;
	if (ldns_algorithm2buffer_str(buf, algorithm)
	    == LDNS_STATUS_OK) {
		str = ldns_buffer_export2str(buf);
	}

	ldns_buffer_free(buf);
	return str;
}

char *
ldns_pkt_cert_algorithm2str(ldns_cert_algorithm cert_algorithm)
{
	char *str;
	ldns_buffer *buf;

	buf = ldns_buffer_new(10);
	if (!buf) {
		return NULL;
	}

	str = NULL;
	if (ldns_cert_algorithm2buffer_str(buf, cert_algorithm)
	    == LDNS_STATUS_OK) {
		str = ldns_buffer_export2str(buf);
	}

	ldns_buffer_free(buf);
	return str;
}


/* do NOT pass compressed data here :p */
ldns_status
ldns_rdf2buffer_str_dname(ldns_buffer *output, const ldns_rdf *dname)
{
	/* can we do with 1 pos var? or without at all? */
	uint8_t src_pos = 0;
	uint8_t len;
	uint8_t *data;
	uint8_t i;
	unsigned char c;

	data = (uint8_t*)ldns_rdf_data(dname);
	len = data[src_pos];

	if (ldns_rdf_size(dname) > LDNS_MAX_DOMAINLEN) {
		/* too large, return */
		return LDNS_STATUS_DOMAINNAME_OVERFLOW;
	}

	/* special case: root label */
	if (1 == ldns_rdf_size(dname)) {
		ldns_buffer_printf(output, ".");
	} else {
		while ((len > 0) && src_pos < ldns_rdf_size(dname)) {
			src_pos++;
			for(i = 0; i < len; i++) {
				/* paranoia check for various 'strange'
				   characters in dnames
				*/
				c = (unsigned char) data[src_pos];
				if(c == '.' || c == ';' ||
				   c == '(' || c == ')' ||
				   c == '\\') {
					ldns_buffer_printf(output, "\\%c",
							data[src_pos]);
				} else if (!(isascii(c) && isgraph(c))) {
					ldns_buffer_printf(output, "\\%03u",
						        data[src_pos]);
				} else {
					ldns_buffer_printf(output, "%c", data[src_pos]);
				}
				src_pos++;
			}

			if (src_pos < ldns_rdf_size(dname)) {
				ldns_buffer_printf(output, ".");
			}
			len = data[src_pos];
		}
	}
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_int8(ldns_buffer *output, const ldns_rdf *rdf)
{
	uint8_t data = ldns_rdf_data(rdf)[0];
	ldns_buffer_printf(output, "%lu", (unsigned long) data);
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_int16(ldns_buffer *output, const ldns_rdf *rdf)
{
	uint16_t data = ldns_read_uint16(ldns_rdf_data(rdf));
	ldns_buffer_printf(output, "%lu", (unsigned long) data);
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_int32(ldns_buffer *output, const ldns_rdf *rdf)
{
	uint32_t data = ldns_read_uint32(ldns_rdf_data(rdf));
	ldns_buffer_printf(output, "%lu", (unsigned long) data);
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_time(ldns_buffer *output, const ldns_rdf *rdf)
{
	/* create a YYYYMMDDHHMMSS string if possible */
	struct tm tm;
	char date_buf[16];

	memset(&tm, 0, sizeof(tm));
	if (ldns_serial_arithmetics_gmtime_r(ldns_rdf2native_int32(rdf), time(NULL), &tm)
	    && strftime(date_buf, 15, "%Y%m%d%H%M%S", &tm)) {
		ldns_buffer_printf(output, "%s", date_buf);
	}
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_a(ldns_buffer *output, const ldns_rdf *rdf)
{
	char str[INET_ADDRSTRLEN];

	if (inet_ntop(AF_INET, ldns_rdf_data(rdf), str, INET_ADDRSTRLEN)) {
		ldns_buffer_printf(output, "%s", str);
	}
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_aaaa(ldns_buffer *output, const ldns_rdf *rdf)
{
	char str[INET6_ADDRSTRLEN];

	if (inet_ntop(AF_INET6, ldns_rdf_data(rdf), str, INET6_ADDRSTRLEN)) {
		ldns_buffer_printf(output, "%s", str);
	}

	return ldns_buffer_status(output);
}

static void 
ldns_characters2buffer_str(ldns_buffer* output,
		size_t amount, const uint8_t* characters)
{
	uint8_t ch;
	while (amount > 0) {
		ch = *characters++;
		if (isprint((int)ch) || ch == '\t') {
			if (ch == '\"' || ch == '\\')
				ldns_buffer_printf(output, "\\%c", ch);
			else
				ldns_buffer_printf(output, "%c", ch);
		} else {
			ldns_buffer_printf(output, "\\%03u",
                                (unsigned)(uint8_t) ch);
		}
		amount--;
	}
}

ldns_status
ldns_rdf2buffer_str_str(ldns_buffer *output, const ldns_rdf *rdf)
{
        if(ldns_rdf_size(rdf) < 1) {
                return LDNS_STATUS_WIRE_RDATA_ERR;
        }
        if((int)ldns_rdf_size(rdf) < (int)ldns_rdf_data(rdf)[0] + 1) {
                return LDNS_STATUS_WIRE_RDATA_ERR;
        }
	ldns_buffer_printf(output, "\"");
	ldns_characters2buffer_str(output, 
			ldns_rdf_data(rdf)[0], ldns_rdf_data(rdf) + 1);
	ldns_buffer_printf(output, "\"");
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_b64(ldns_buffer *output, const ldns_rdf *rdf)
{
	size_t size;
	char *b64;

	if (ldns_rdf_size(rdf) == 0) {
		ldns_buffer_printf(output, "0");
		return ldns_buffer_status(output);
	} else
		size = ldns_b64_ntop_calculate_size(ldns_rdf_size(rdf));

	if (!(b64 = LDNS_XMALLOC(char, size)))
		return LDNS_STATUS_MEM_ERR;

	if (ldns_b64_ntop(ldns_rdf_data(rdf), ldns_rdf_size(rdf), b64, size)) {
		ldns_buffer_printf(output, "%s", b64);
	}
	LDNS_FREE(b64);
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_b32_ext(ldns_buffer *output, const ldns_rdf *rdf)
{
	size_t size;
	char *b32;
	if(ldns_rdf_size(rdf) == 0)
		return LDNS_STATUS_OK;
        /* remove -1 for the b32-hash-len octet */
	size = ldns_b32_ntop_calculate_size(ldns_rdf_size(rdf) - 1);
        /* add one for the end nul for the string */
	b32 = LDNS_XMALLOC(char, size + 1);
	if(!b32) return LDNS_STATUS_MEM_ERR;
	size = (size_t) ldns_b32_ntop_extended_hex(ldns_rdf_data(rdf) + 1,
		ldns_rdf_size(rdf) - 1, b32, size+1);
	if (size > 0) {
		ldns_buffer_printf(output, "%s", b32);
	}
	LDNS_FREE(b32);
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_hex(ldns_buffer *output, const ldns_rdf *rdf)
{
	size_t i;
	for (i = 0; i < ldns_rdf_size(rdf); i++) {
		ldns_buffer_printf(output, "%02x", ldns_rdf_data(rdf)[i]);
	}

	return ldns_buffer_status(output);
}

static ldns_status
ldns_rdf2buffer_str_type_fmt(ldns_buffer *output,
		const ldns_output_format* fmt, const ldns_rdf *rdf)
{
        uint16_t data = ldns_read_uint16(ldns_rdf_data(rdf));

	if (! ldns_output_format_covers_type(fmt, data) &&
			ldns_rr_descript(data) &&
			ldns_rr_descript(data)->_name) {

		ldns_buffer_printf(output, "%s",ldns_rr_descript(data)->_name);
	} else {
		ldns_buffer_printf(output, "TYPE%u", data);
	}
	return  ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_type(ldns_buffer *output, const ldns_rdf *rdf)
{
	return ldns_rdf2buffer_str_type_fmt(output,
			ldns_output_format_default, rdf);
}

ldns_status
ldns_rdf2buffer_str_class(ldns_buffer *output, const ldns_rdf *rdf)
{
	uint16_t data = ldns_read_uint16(ldns_rdf_data(rdf));
	ldns_lookup_table *lt;

 	lt = ldns_lookup_by_id(ldns_rr_classes, (int) data);
	if (lt) {
		ldns_buffer_printf(output, "\t%s", lt->name);
	} else {
		ldns_buffer_printf(output, "\tCLASS%d", data);
	}
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_cert_alg(ldns_buffer *output, const ldns_rdf *rdf)
{
        uint16_t data = ldns_read_uint16(ldns_rdf_data(rdf));
	ldns_lookup_table *lt;
 	lt = ldns_lookup_by_id(ldns_cert_algorithms, (int) data);
	if (lt) {
		ldns_buffer_printf(output, "%s", lt->name);
	} else {
		ldns_buffer_printf(output, "%d", data);
	}
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_alg(ldns_buffer *output, const ldns_rdf *rdf)
{
	return ldns_rdf2buffer_str_int8(output, rdf);
}

static void
loc_cm_print(ldns_buffer *output, uint8_t mantissa, uint8_t exponent)
{
	uint8_t i;
	/* is it 0.<two digits> ? */
	if(exponent < 2) {
		if(exponent == 1)
			mantissa *= 10;
		ldns_buffer_printf(output, "0.%02ld", (long)mantissa);
		return;
	}
	/* always <digit><string of zeros> */
	ldns_buffer_printf(output, "%d", (int)mantissa);
	for(i=0; i<exponent-2; i++)
		ldns_buffer_printf(output, "0");
}

ldns_status
ldns_rr_type2buffer_str(ldns_buffer *output, const ldns_rr_type type)
{
	const ldns_rr_descriptor *descriptor;

	descriptor = ldns_rr_descript(type);

	switch (type) {
		case LDNS_RR_TYPE_IXFR:
			ldns_buffer_printf(output, "IXFR");
			break;
		case LDNS_RR_TYPE_AXFR:
			ldns_buffer_printf(output, "AXFR");
			break;
		case LDNS_RR_TYPE_MAILA:
			ldns_buffer_printf(output, "MAILA");
			break;
		case LDNS_RR_TYPE_MAILB:
			ldns_buffer_printf(output, "MAILB");
			break;
		case LDNS_RR_TYPE_ANY:
			ldns_buffer_printf(output, "ANY");
			break;
		default:
			if (descriptor && descriptor->_name) {
				ldns_buffer_printf(output, "%s", descriptor->_name);
			} else {
				ldns_buffer_printf(output, "TYPE%u", type);
			}
	}
	return ldns_buffer_status(output);
}

char *
ldns_rr_type2str(const ldns_rr_type type)
{
	char *str;
	ldns_buffer *buf;

	buf = ldns_buffer_new(10);
	if (!buf) {
		return NULL;
	}

	str = NULL;
	if (ldns_rr_type2buffer_str(buf, type) == LDNS_STATUS_OK) {
		str = ldns_buffer_export2str(buf);
	}

	ldns_buffer_free(buf);
	return str;
}


ldns_status
ldns_rr_class2buffer_str(ldns_buffer *output,
                         const ldns_rr_class klass)
{
	ldns_lookup_table *lt;

	lt = ldns_lookup_by_id(ldns_rr_classes, klass);
	if (lt) {
		ldns_buffer_printf(output, "%s", lt->name);
	} else {
		ldns_buffer_printf(output, "CLASS%d", klass);
	}
	return ldns_buffer_status(output);
}

char *
ldns_rr_class2str(const ldns_rr_class klass)
{
	ldns_buffer *buf;
	char *str;

	buf = ldns_buffer_new(10);
	if (!buf) {
		return NULL;
	}

	str = NULL;
	if (ldns_rr_class2buffer_str(buf, klass) == LDNS_STATUS_OK) {
		str = ldns_buffer_export2str(buf);
	}
	ldns_buffer_free(buf);
	return str;
}

ldns_status
ldns_rdf2buffer_str_loc(ldns_buffer *output, const ldns_rdf *rdf)
{
	/* we could do checking (ie degrees < 90 etc)? */
	uint8_t version;
	uint8_t size;
	uint8_t horizontal_precision;
	uint8_t vertical_precision;
	uint32_t longitude;
	uint32_t latitude;
	uint32_t altitude;
	char latitude_hemisphere;
	char longitude_hemisphere;
	uint32_t h;
	uint32_t m;
	double s;

	uint32_t equator = (uint32_t) ldns_power(2, 31);

        if(ldns_rdf_size(rdf) < 1) {
                return LDNS_STATUS_WIRE_RDATA_ERR;
        }
       	version = ldns_rdf_data(rdf)[0];
	if (version == 0) {
		if(ldns_rdf_size(rdf) < 16) {
			return LDNS_STATUS_WIRE_RDATA_ERR;
		}
		size = ldns_rdf_data(rdf)[1];
		horizontal_precision = ldns_rdf_data(rdf)[2];
		vertical_precision = ldns_rdf_data(rdf)[3];

		latitude = ldns_read_uint32(&ldns_rdf_data(rdf)[4]);
		longitude = ldns_read_uint32(&ldns_rdf_data(rdf)[8]);
		altitude = ldns_read_uint32(&ldns_rdf_data(rdf)[12]);

		if (latitude > equator) {
			latitude_hemisphere = 'N';
			latitude = latitude - equator;
		} else {
			latitude_hemisphere = 'S';
			latitude = equator - latitude;
		}
		h = latitude / (1000 * 60 * 60);
		latitude = latitude % (1000 * 60 * 60);
		m = latitude / (1000 * 60);
		latitude = latitude % (1000 * 60);
		s = (double) latitude / 1000.0;
		ldns_buffer_printf(output, "%02u %02u %0.3f %c ",
			h, m, s, latitude_hemisphere);

		if (longitude > equator) {
			longitude_hemisphere = 'E';
			longitude = longitude - equator;
		} else {
			longitude_hemisphere = 'W';
			longitude = equator - longitude;
		}
		h = longitude / (1000 * 60 * 60);
		longitude = longitude % (1000 * 60 * 60);
		m = longitude / (1000 * 60);
		longitude = longitude % (1000 * 60);
		s = (double) longitude / (1000.0);
		ldns_buffer_printf(output, "%02u %02u %0.3f %c ",
			h, m, s, longitude_hemisphere);

		s = ((double) altitude) / 100;
		s -= 100000;

		if(altitude%100 != 0)
			ldns_buffer_printf(output, "%.2f", s);
		else
			ldns_buffer_printf(output, "%.0f", s);

		ldns_buffer_printf(output, "m ");

		loc_cm_print(output, (size & 0xf0) >> 4, size & 0x0f);
		ldns_buffer_printf(output, "m ");

		loc_cm_print(output, (horizontal_precision & 0xf0) >> 4,
			horizontal_precision & 0x0f);
		ldns_buffer_printf(output, "m ");

		loc_cm_print(output, (vertical_precision & 0xf0) >> 4,
			vertical_precision & 0x0f);
		ldns_buffer_printf(output, "m");

		return ldns_buffer_status(output);
	} else {
		return ldns_rdf2buffer_str_hex(output, rdf);
	}
}

ldns_status
ldns_rdf2buffer_str_unknown(ldns_buffer *output, const ldns_rdf *rdf)
{
	ldns_buffer_printf(output, "\\# %u ", ldns_rdf_size(rdf));
	return ldns_rdf2buffer_str_hex(output, rdf);
}

ldns_status
ldns_rdf2buffer_str_nsap(ldns_buffer *output, const ldns_rdf *rdf)
{
	ldns_buffer_printf(output, "0x");
	return ldns_rdf2buffer_str_hex(output, rdf);
}

ldns_status
ldns_rdf2buffer_str_atma(ldns_buffer *output, const ldns_rdf *rdf)
{
	return ldns_rdf2buffer_str_hex(output, rdf);
}

ldns_status
ldns_rdf2buffer_str_wks(ldns_buffer *output, const ldns_rdf *rdf)
{
	/* protocol, followed by bitmap of services */
	struct protoent *protocol;
	char *proto_name = NULL;
	uint8_t protocol_nr;
	struct servent *service;
	uint16_t current_service;

        if(ldns_rdf_size(rdf) < 1) {
                return LDNS_STATUS_WIRE_RDATA_ERR;
        }
	protocol_nr = ldns_rdf_data(rdf)[0];
	protocol = getprotobynumber((int) protocol_nr);
	if (protocol && (protocol->p_name != NULL)) {
		proto_name = protocol->p_name;
		ldns_buffer_printf(output, "%s ", protocol->p_name);
	} else {
		ldns_buffer_printf(output, "%u ", protocol_nr);
	}

#ifdef HAVE_ENDPROTOENT
	endprotoent();
#endif

	for (current_service = 0;
	     current_service < (ldns_rdf_size(rdf)-1)*8; current_service++) {
		if (ldns_get_bit(&(ldns_rdf_data(rdf)[1]), current_service)) {
			service = getservbyport((int) htons(current_service),
			                        proto_name);
			if (service && service->s_name) {
				ldns_buffer_printf(output, "%s ", service->s_name);
			} else {
				ldns_buffer_printf(output, "%u ", current_service);
			}
#ifdef HAVE_ENDSERVENT
			endservent();
#endif
		}
		/* exit from loop before integer overflow */
		if(current_service == 65535) { break; }
	}
	return ldns_buffer_status(output);
}

static ldns_status
ldns_rdf2buffer_str_nsec_fmt(ldns_buffer *output,
		const ldns_output_format* fmt, const ldns_rdf *rdf)
{
	/* Note: this code is duplicated in higher.c in
	 * ldns_nsec_type_check() function
	 */
	uint8_t window_block_nr;
	uint8_t bitmap_length;
	uint16_t type;
	uint16_t pos = 0;
	uint16_t bit_pos;
	uint8_t *data = ldns_rdf_data(rdf);

	while((size_t)(pos + 2) < ldns_rdf_size(rdf)) {
		window_block_nr = data[pos];
		bitmap_length = data[pos + 1];
		pos += 2;
		if (ldns_rdf_size(rdf) < pos + bitmap_length) {
			return LDNS_STATUS_WIRE_RDATA_ERR;
		}
		for (bit_pos = 0; bit_pos < (bitmap_length) * 8; bit_pos++) {
			if (! ldns_get_bit(&data[pos], bit_pos)) {
				continue;
			}
			type = 256 * (uint16_t) window_block_nr + bit_pos;

			if (! ldns_output_format_covers_type(fmt, type) &&
					ldns_rr_descript(type) &&
					ldns_rr_descript(type)->_name){

				ldns_buffer_printf(output, "%s ",
						ldns_rr_descript(type)->_name);
			} else {
				ldns_buffer_printf(output, "TYPE%u ", type);
			}
		}
		pos += (uint16_t) bitmap_length;
	}
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_nsec(ldns_buffer *output, const ldns_rdf *rdf)
{
	return ldns_rdf2buffer_str_nsec_fmt(output,
			ldns_output_format_default, rdf);
}

ldns_status
ldns_rdf2buffer_str_nsec3_salt(ldns_buffer *output, const ldns_rdf *rdf)
{
	uint8_t salt_length;
	uint8_t salt_pos;

	uint8_t *data = ldns_rdf_data(rdf);

        if(ldns_rdf_size(rdf) < 1) {
                return LDNS_STATUS_WIRE_RDATA_ERR;
        }
	salt_length = data[0];
	/* from now there are variable length entries so remember pos */
	if (salt_length == 0 || ((size_t)salt_length)+1 > ldns_rdf_size(rdf)) {
		ldns_buffer_printf(output, "- ");
	} else {
		for (salt_pos = 0; salt_pos < salt_length; salt_pos++) {
			ldns_buffer_printf(output, "%02x", data[1 + salt_pos]);
		}
		ldns_buffer_printf(output, " ");
	}

	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_period(ldns_buffer *output, const ldns_rdf *rdf)
{
	/* period is the number of seconds */
	if (ldns_rdf_size(rdf) != 4) {
		return LDNS_STATUS_WIRE_RDATA_ERR;
	}
	ldns_buffer_printf(output, "%u", ldns_read_uint32(ldns_rdf_data(rdf)));
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_tsigtime(ldns_buffer *output,const  ldns_rdf *rdf)
{
	/* tsigtime is 48 bits network order unsigned integer */
	uint64_t tsigtime = 0;
	uint8_t *data = ldns_rdf_data(rdf);
	uint64_t d0, d1, d2, d3, d4, d5;

	if (ldns_rdf_size(rdf) < 6) {
		return LDNS_STATUS_WIRE_RDATA_ERR;
	}
	d0 = data[0]; /* cast to uint64 for shift operations */
	d1 = data[1];
	d2 = data[2];
	d3 = data[3];
	d4 = data[4];
	d5 = data[5];
	tsigtime = (d0<<40) | (d1<<32) | (d2<<24) | (d3<<16) | (d4<<8) | d5;

	ldns_buffer_printf(output, "%llu ", (long long)tsigtime);

	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_apl(ldns_buffer *output, const ldns_rdf *rdf)
{
	uint8_t *data = ldns_rdf_data(rdf);
	uint16_t address_family;
	uint8_t prefix;
	bool negation;
	uint8_t adf_length;
	size_t i;
	size_t pos = 0;

	while (pos < (unsigned int) ldns_rdf_size(rdf)) {
                if(pos + 3 >= (unsigned)ldns_rdf_size(rdf))
                        return LDNS_STATUS_WIRE_RDATA_ERR;
		address_family = ldns_read_uint16(&data[pos]);
		prefix = data[pos + 2];
		negation = data[pos + 3] & LDNS_APL_NEGATION;
		adf_length = data[pos + 3] & LDNS_APL_MASK;
		if (address_family == LDNS_APL_IP4) {
			/* check if prefix < 32? */
			if (negation) {
				ldns_buffer_printf(output, "!");
			}
			ldns_buffer_printf(output, "%u:", address_family);
			/* address is variable length 0 - 4 */
			for (i = 0; i < 4; i++) {
				if (i > 0) {
					ldns_buffer_printf(output, ".");
				}
				if (i < (unsigned short) adf_length) {
                                        if(pos+i+4 >= ldns_rdf_size(rdf))
					    return LDNS_STATUS_WIRE_RDATA_ERR;
					ldns_buffer_printf(output, "%d",
					                   data[pos + i + 4]);
				} else {
					ldns_buffer_printf(output, "0");
				}
			}
			ldns_buffer_printf(output, "/%u ", prefix);
		} else if (address_family == LDNS_APL_IP6) {
			/* check if prefix < 128? */
			if (negation) {
				ldns_buffer_printf(output, "!");
			}
			ldns_buffer_printf(output, "%u:", address_family);
			/* address is variable length 0 - 16 */
			for (i = 0; i < 16; i++) {
				if (i % 2 == 0 && i > 0) {
					ldns_buffer_printf(output, ":");
				}
				if (i < (unsigned short) adf_length) {
                                        if(pos+i+4 >= ldns_rdf_size(rdf))
					    return LDNS_STATUS_WIRE_RDATA_ERR;
					ldns_buffer_printf(output, "%02x",
					                   data[pos + i + 4]);
				} else {
					ldns_buffer_printf(output, "00");
				}
			}
			ldns_buffer_printf(output, "/%u ", prefix);

		} else {
			/* unknown address family */
			ldns_buffer_printf(output,
					"Unknown address family: %u data: ",
					address_family);
			for (i = 1; i < (unsigned short) (4 + adf_length); i++) {
                                if(pos+i >= ldns_rdf_size(rdf))
                                        return LDNS_STATUS_WIRE_RDATA_ERR;
				ldns_buffer_printf(output, "%02x", data[i]);
			}
		}
		pos += 4 + adf_length;
	}
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_int16_data(ldns_buffer *output, const ldns_rdf *rdf)
{
	size_t size;
	char *b64;
	if (ldns_rdf_size(rdf) < 2) {
		return LDNS_STATUS_WIRE_RDATA_ERR;
	}
	/* Subtract the size (2) of the number that specifies the length */
	size = ldns_b64_ntop_calculate_size(ldns_rdf_size(rdf) - 2);
	ldns_buffer_printf(output, "%u ", ldns_rdf_size(rdf) - 2);
	if (ldns_rdf_size(rdf) > 2) {
		b64 = LDNS_XMALLOC(char, size);
		if(!b64)
			return LDNS_STATUS_MEM_ERR;

		if (ldns_rdf_size(rdf) > 2 &&
		ldns_b64_ntop(ldns_rdf_data(rdf) + 2,
					ldns_rdf_size(rdf) - 2,
					b64, size)) {
			ldns_buffer_printf(output, "%s", b64);
		}
		LDNS_FREE(b64);
	}
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_ipseckey(ldns_buffer *output, const ldns_rdf *rdf)
{
	/* wire format from
	   http://www.ietf.org/internet-drafts/draft-ietf-ipseckey-rr-12.txt
	*/
	uint8_t *data = ldns_rdf_data(rdf);
	uint8_t precedence;
	uint8_t gateway_type;
	uint8_t algorithm;

	ldns_rdf *gateway = NULL;
	uint8_t *gateway_data;

	size_t public_key_size;
	uint8_t *public_key_data;
	ldns_rdf *public_key;

	size_t offset = 0;
	ldns_status status;

	if (ldns_rdf_size(rdf) < 3) {
		return LDNS_STATUS_WIRE_RDATA_ERR;
	}
	precedence = data[0];
	gateway_type = data[1];
	algorithm = data[2];
	offset = 3;

	switch (gateway_type) {
		case 0:
			/* no gateway */
			break;
		case 1:
			if (ldns_rdf_size(rdf) < offset + LDNS_IP4ADDRLEN) {
				return LDNS_STATUS_ERR;
			}
			gateway_data = LDNS_XMALLOC(uint8_t, LDNS_IP4ADDRLEN);
                        if(!gateway_data)
                                return LDNS_STATUS_MEM_ERR;
			memcpy(gateway_data, &data[offset], LDNS_IP4ADDRLEN);
			gateway = ldns_rdf_new(LDNS_RDF_TYPE_A,
					LDNS_IP4ADDRLEN , gateway_data);
			offset += LDNS_IP4ADDRLEN;
                        if(!gateway) {
                                LDNS_FREE(gateway_data);
                                return LDNS_STATUS_MEM_ERR;
                        }
			break;
		case 2:
			if (ldns_rdf_size(rdf) < offset + LDNS_IP6ADDRLEN) {
				return LDNS_STATUS_ERR;
			}
			gateway_data = LDNS_XMALLOC(uint8_t, LDNS_IP6ADDRLEN);
                        if(!gateway_data)
                                return LDNS_STATUS_MEM_ERR;
			memcpy(gateway_data, &data[offset], LDNS_IP6ADDRLEN);
			offset += LDNS_IP6ADDRLEN;
			gateway =
				ldns_rdf_new(LDNS_RDF_TYPE_AAAA,
						LDNS_IP6ADDRLEN, gateway_data);
                        if(!gateway) {
                                LDNS_FREE(gateway_data);
                                return LDNS_STATUS_MEM_ERR;
                        }
			break;
		case 3:
			status = ldns_wire2dname(&gateway, data,
					ldns_rdf_size(rdf), &offset);
                        if(status != LDNS_STATUS_OK)
                                return status;
			break;
		default:
			/* error? */
			break;
	}

	if (ldns_rdf_size(rdf) <= offset) {
                ldns_rdf_deep_free(gateway);
		return LDNS_STATUS_ERR;
	}
	public_key_size = ldns_rdf_size(rdf) - offset;
	public_key_data = LDNS_XMALLOC(uint8_t, public_key_size);
        if(!public_key_data) {
                ldns_rdf_deep_free(gateway);
                return LDNS_STATUS_MEM_ERR;
        }
	memcpy(public_key_data, &data[offset], public_key_size);
	public_key = ldns_rdf_new(LDNS_RDF_TYPE_B64,
			public_key_size, public_key_data);
        if(!public_key) {
                LDNS_FREE(public_key_data);
                ldns_rdf_deep_free(gateway);
                return LDNS_STATUS_MEM_ERR;
        }

	ldns_buffer_printf(output, "%u %u %u ", precedence, gateway_type, algorithm);
	if (gateway)
	  	(void) ldns_rdf2buffer_str(output, gateway);
	else
		ldns_buffer_printf(output, ".");
	ldns_buffer_printf(output, " ");
	(void) ldns_rdf2buffer_str(output, public_key);

	ldns_rdf_deep_free(gateway);
	ldns_rdf_deep_free(public_key);

	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_ilnp64(ldns_buffer *output, const ldns_rdf *rdf)
{
	if (ldns_rdf_size(rdf) != 8) {
		return LDNS_STATUS_WIRE_RDATA_ERR;
	}
	ldns_buffer_printf(output,"%.4x:%.4x:%.4x:%.4x",
				ldns_read_uint16(ldns_rdf_data(rdf)),
				ldns_read_uint16(ldns_rdf_data(rdf)+2),
				ldns_read_uint16(ldns_rdf_data(rdf)+4),
				ldns_read_uint16(ldns_rdf_data(rdf)+6));
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_eui48(ldns_buffer *output, const ldns_rdf *rdf)
{
	if (ldns_rdf_size(rdf) != 6) {
		return LDNS_STATUS_WIRE_RDATA_ERR;
	}
	ldns_buffer_printf(output,"%.2x-%.2x-%.2x-%.2x-%.2x-%.2x",
				ldns_rdf_data(rdf)[0], ldns_rdf_data(rdf)[1],
				ldns_rdf_data(rdf)[2], ldns_rdf_data(rdf)[3],
				ldns_rdf_data(rdf)[4], ldns_rdf_data(rdf)[5]);
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_eui64(ldns_buffer *output, const ldns_rdf *rdf)
{
	if (ldns_rdf_size(rdf) != 8) {
		return LDNS_STATUS_WIRE_RDATA_ERR;
	}
	ldns_buffer_printf(output,"%.2x-%.2x-%.2x-%.2x-%.2x-%.2x-%.2x-%.2x",
				ldns_rdf_data(rdf)[0], ldns_rdf_data(rdf)[1],
				ldns_rdf_data(rdf)[2], ldns_rdf_data(rdf)[3],
				ldns_rdf_data(rdf)[4], ldns_rdf_data(rdf)[5],
				ldns_rdf_data(rdf)[6], ldns_rdf_data(rdf)[7]);
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_tag(ldns_buffer *output, const ldns_rdf *rdf)
{
	size_t nchars;
	const uint8_t* chars;
	char ch;
	if (ldns_rdf_size(rdf) < 2) {
		return LDNS_STATUS_WIRE_RDATA_ERR;
	}
	nchars = ldns_rdf_data(rdf)[0];
	if (nchars >= ldns_rdf_size(rdf) || /* should be rdf_size - 1 */
			nchars < 1) {
		return LDNS_STATUS_WIRE_RDATA_ERR;
	}
	chars = ldns_rdf_data(rdf) + 1;
	while (nchars > 0) {
		ch = (char)*chars++;
		if (! isalnum((unsigned char)ch)) {
			return LDNS_STATUS_WIRE_RDATA_ERR;
		}
		ldns_buffer_printf(output, "%c", ch);
		nchars--;
	}
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_long_str(ldns_buffer *output, const ldns_rdf *rdf)
{

	ldns_buffer_printf(output, "\"");
	ldns_characters2buffer_str(output,
			ldns_rdf_size(rdf), ldns_rdf_data(rdf));
	ldns_buffer_printf(output, "\"");
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_hip(ldns_buffer *output, const ldns_rdf *rdf)
{
	uint8_t *data = ldns_rdf_data(rdf);
	size_t rdf_size = ldns_rdf_size(rdf);
	uint8_t hit_size;
	uint16_t pk_size;
	int written;
	
	if (rdf_size < 6) {
		return LDNS_STATUS_WIRE_RDATA_ERR;
	}
	if ((hit_size = data[0]) == 0 ||
			(pk_size = ldns_read_uint16(data + 2)) == 0 ||
			rdf_size < (size_t) hit_size + pk_size + 4) {

		return LDNS_STATUS_WIRE_RDATA_ERR;
	}

	ldns_buffer_printf(output, "%d ", (int) data[1]);

	for (data += 4; hit_size > 0; hit_size--, data++) {

		ldns_buffer_printf(output, "%02x", (int) *data);
	}
	ldns_buffer_write_char(output, (uint8_t) ' ');

	if (ldns_buffer_reserve(output,
				ldns_b64_ntop_calculate_size(pk_size))) {

		written = ldns_b64_ntop(data, pk_size,
				(char *) ldns_buffer_current(output),
				ldns_buffer_remaining(output));

		if (written > 0 &&
				written < (int) ldns_buffer_remaining(output)) {

			output->_position += written;
		}
	}
	return ldns_buffer_status(output);
}

/* implementation mimicked from ldns_rdf2buffer_str_ipseckey */
ldns_status
ldns_rdf2buffer_str_amtrelay(ldns_buffer *output, const ldns_rdf *rdf)
{
	/* wire format from
	 * draft-ietf-mboned-driad-amt-discovery Section 4.2  
	 */
	uint8_t *data = ldns_rdf_data(rdf);
	uint8_t precedence;
	uint8_t discovery_optional;
	uint8_t relay_type;

	ldns_rdf *relay = NULL;
	uint8_t *relay_data;

	size_t offset = 0;
	ldns_status status;

	if (ldns_rdf_size(rdf) < 2) {
		return LDNS_STATUS_WIRE_RDATA_ERR;
	}
	precedence = data[0];
	discovery_optional = ((data[1] & 0x80) >> 7);
	relay_type = data[1] & 0x7F;
	offset = 2;

	switch (relay_type) {
		case 0:
			/* no relay */
			break;
		case 1:
			if (ldns_rdf_size(rdf) < offset + LDNS_IP4ADDRLEN) {
				return LDNS_STATUS_ERR;
			}
			relay_data = LDNS_XMALLOC(uint8_t, LDNS_IP4ADDRLEN);
                        if(!relay_data)
                                return LDNS_STATUS_MEM_ERR;
			memcpy(relay_data, &data[offset], LDNS_IP4ADDRLEN);
			relay = ldns_rdf_new(LDNS_RDF_TYPE_A,
					LDNS_IP4ADDRLEN , relay_data);
			offset += LDNS_IP4ADDRLEN;
                        if(!relay) {
                                LDNS_FREE(relay_data);
                                return LDNS_STATUS_MEM_ERR;
                        }
			break;
		case 2:
			if (ldns_rdf_size(rdf) < offset + LDNS_IP6ADDRLEN) {
				return LDNS_STATUS_ERR;
			}
			relay_data = LDNS_XMALLOC(uint8_t, LDNS_IP6ADDRLEN);
                        if(!relay_data)
                                return LDNS_STATUS_MEM_ERR;
			memcpy(relay_data, &data[offset], LDNS_IP6ADDRLEN);
			offset += LDNS_IP6ADDRLEN;
			relay =
				ldns_rdf_new(LDNS_RDF_TYPE_AAAA,
						LDNS_IP6ADDRLEN, relay_data);
                        if(!relay) {
                                LDNS_FREE(relay_data);
                                return LDNS_STATUS_MEM_ERR;
                        }
			break;
		case 3:
			status = ldns_wire2dname(&relay, data,
					ldns_rdf_size(rdf), &offset);
                        if(status != LDNS_STATUS_OK)
                                return status;
			break;
		default:
			/* error? */
			break;
	}

	if (ldns_rdf_size(rdf) != offset) {
                ldns_rdf_deep_free(relay);
		return LDNS_STATUS_ERR;
	}
	ldns_buffer_printf(output, "%u %u %u ",
			precedence, discovery_optional, relay_type);
	if (relay)
	  	(void) ldns_rdf2buffer_str(output, relay);

	ldns_rdf_deep_free(relay);
	return ldns_buffer_status(output);
}

#ifdef RRTYPE_SVCB_HTTPS
ldns_status svcparam_key2buffer_str(ldns_buffer *output, uint16_t key);

static ldns_status
svcparam_mandatory2buffer_str(ldns_buffer *output, size_t sz, uint8_t *data)
{
	if (sz % 2)
		return LDNS_STATUS_INVALID_SVCPARAM_VALUE;

	svcparam_key2buffer_str(output, ldns_read_uint16(data));
	for (data += 2, sz -= 2; sz; data += 2, sz -= 2) {
		ldns_buffer_write_char(output, ',');
		svcparam_key2buffer_str(output, ldns_read_uint16(data));
	}
	return ldns_buffer_status(output);
}

static ldns_status
svcparam_alpn2buffer_str(ldns_buffer *output, size_t sz, uint8_t *data)
{
	uint8_t *eod = data + sz, *dp;
	bool quote = false;
	size_t i;

	for (dp = data; dp < eod && !quote; dp += 1 + *dp) {
		if (dp + 1 + *dp > eod)
			return LDNS_STATUS_INVALID_SVCPARAM_VALUE;

		for (i = 0; i < *dp; i++)
			if (isspace(dp[i + 1]))
				break;
		quote = i < *dp;
	}
	if (quote)
		ldns_buffer_write_char(output, '"');
	while (data < eod) {
		uint8_t *eot = data + 1 + *data;

		if (eot > eod)
			return LDNS_STATUS_INVALID_SVCPARAM_VALUE;

		if (eod - data < (int)sz)
			ldns_buffer_write_char(output, ',');

		for (data += 1; data < eot; data += 1) {
			uint8_t ch = *data;

			if (isprint(ch) || ch == '\t') {
				if (ch == '"' ||  ch == ',' || ch == '\\')
					ldns_buffer_write_char(output, '\\');
				ldns_buffer_write_char(output, ch);
			} else
				ldns_buffer_printf(output, "\\%03u"
				                         , (unsigned)ch);
		}
	}
	if (quote)
		ldns_buffer_write_char(output, '"');
	return ldns_buffer_status(output);
}

static ldns_status
svcparam_port2buffer_str(ldns_buffer *output, size_t sz, uint8_t *data)
{
	if (sz != 2)
		return LDNS_STATUS_INVALID_SVCPARAM_VALUE;
	ldns_buffer_printf(output, "%d", (int)ldns_read_uint16(data));
	return ldns_buffer_status(output);
}

static ldns_status
svcparam_ipv4hint2buffer_str(ldns_buffer *output, size_t sz, uint8_t *data)
{
	char str[INET_ADDRSTRLEN];

	if (sz % 4 || !inet_ntop(AF_INET, data, str, INET_ADDRSTRLEN))
		return LDNS_STATUS_INVALID_SVCPARAM_VALUE;

	ldns_buffer_write_chars(output, str);

	for (data += 4, sz -= 4; sz ; data += 4, sz -= 4 ) {
		ldns_buffer_write_char(output, ',');
		if (!inet_ntop(AF_INET, data, str, INET_ADDRSTRLEN))
			return LDNS_STATUS_INVALID_SVCPARAM_VALUE;

		ldns_buffer_write_chars(output, str);
	}
	return ldns_buffer_status(output);
}

static ldns_status
svcparam_ech2buffer_str(ldns_buffer *output, size_t sz, uint8_t *data)
{
	size_t str_sz = ldns_b64_ntop_calculate_size(sz);
	int written;

	if (!ldns_buffer_reserve(output, str_sz))
		return LDNS_STATUS_MEM_ERR;

	written = ldns_b64_ntop( data, sz
	                       , (char *)ldns_buffer_current(output), str_sz);
	if (written > 0)
		ldns_buffer_skip(output, written);
	else
		return LDNS_STATUS_INVALID_SVCPARAM_VALUE;

	return ldns_buffer_status(output);
}

static ldns_status
svcparam_ipv6hint2buffer_str(ldns_buffer *output, size_t sz, uint8_t *data)
{
	char str[INET6_ADDRSTRLEN];

	if (sz % 16 || !inet_ntop(AF_INET6, data, str, INET6_ADDRSTRLEN))
		return LDNS_STATUS_INVALID_SVCPARAM_VALUE;

	ldns_buffer_write_chars(output, str);

	for (data += 16, sz -= 16; sz ; data += 16, sz -= 16) {
		ldns_buffer_write_char(output, ',');
		if (!inet_ntop(AF_INET6, data, str, INET6_ADDRSTRLEN))
			return LDNS_STATUS_INVALID_SVCPARAM_VALUE;

		ldns_buffer_write_chars(output, str);
	}
	return ldns_buffer_status(output);
}

static ldns_status
svcparam_value2buffer_str(ldns_buffer *output, size_t sz, uint8_t *data)
{
	uint8_t *eod = data + sz, *dp;
	bool quote = false;

	for (dp = data; dp < eod && !isspace(*dp); dp++)
		; /* pass */

	if ((quote = dp < eod))
		ldns_buffer_write_char(output, '"');

	for (dp = data; dp < eod; dp++) {
		uint8_t ch = *dp;

		if (isprint(ch) || ch == '\t') {
			if (ch == '"' ||  ch == '\\')
				ldns_buffer_write_char(output, '\\');
			ldns_buffer_write_char(output, ch);
		} else
			ldns_buffer_printf(output, "\\%03u", (unsigned)ch);
	}
	if (quote)
		ldns_buffer_write_char(output, '"');
	return ldns_buffer_status(output);
}

ldns_status
ldns_rdf2buffer_str_svcparams(ldns_buffer *output, const ldns_rdf *rdf)
{
	uint8_t    *data, *dp, *next_dp = NULL;
	size_t      sz;
	ldns_status st;

	if (!output)
		return LDNS_STATUS_NULL;

	if (!rdf || !(data = ldns_rdf_data(rdf)) || !(sz = ldns_rdf_size(rdf)))
		/* No svcparams is just fine. Just nothing to print. */
		return LDNS_STATUS_OK;
	
	for (dp = data; dp + 4 <= data + sz; dp = next_dp) {
		ldns_svcparam_key key    = ldns_read_uint16(dp);
		uint16_t          val_sz = ldns_read_uint16(dp + 2);

		if ((next_dp = dp + 4 + val_sz) > data + sz)
			return LDNS_STATUS_RDATA_OVERFLOW;

		if (dp > data)
			ldns_buffer_write_char(output, ' ');

		if ((st = svcparam_key2buffer_str(output, key)))
			return st;

		if (val_sz == 0)
			continue;
		dp += 4;
		ldns_buffer_write_char(output, '=');
		switch (key) {
		case LDNS_SVCPARAM_KEY_MANDATORY:
			st = svcparam_mandatory2buffer_str(output, val_sz, dp);
			break;
		case LDNS_SVCPARAM_KEY_ALPN:
			st = svcparam_alpn2buffer_str(output, val_sz, dp);
			break;
		case LDNS_SVCPARAM_KEY_NO_DEFAULT_ALPN:
			return LDNS_STATUS_NO_SVCPARAM_VALUE_EXPECTED;
		case LDNS_SVCPARAM_KEY_PORT:
			st = svcparam_port2buffer_str(output, val_sz, dp);
			break;
		case LDNS_SVCPARAM_KEY_IPV4HINT:
			st = svcparam_ipv4hint2buffer_str(output, val_sz, dp);
			break;
		case LDNS_SVCPARAM_KEY_ECH:
			st = svcparam_ech2buffer_str(output, val_sz, dp);
			break;
		case LDNS_SVCPARAM_KEY_IPV6HINT:
			st = svcparam_ipv6hint2buffer_str(output, val_sz, dp);
			break;
		default:
			st = svcparam_value2buffer_str(output, val_sz, dp);
			break;
		}
		if (st)
			return st;
	}
	return ldns_buffer_status(output);
}
#else	/* #ifdef RRTYPE_SVCB_HTTPS */
ldns_status
ldns_rdf2buffer_str_svcparams(ldns_buffer *output, const ldns_rdf *rdf)
{
	(void)output; (void)rdf;
	return LDNS_STATUS_NOT_IMPL;
}
#endif	/* #ifdef RRTYPE_SVCB_HTTPS */

static ldns_status
ldns_rdf2buffer_str_fmt(ldns_buffer *buffer,
		const ldns_output_format* fmt, const ldns_rdf *rdf)
{
	ldns_status res = LDNS_STATUS_OK;

	/*ldns_buffer_printf(buffer, "%u:", ldns_rdf_get_type(rdf));*/
	if (rdf) {
		switch(ldns_rdf_get_type(rdf)) {
		case LDNS_RDF_TYPE_NONE:
			break;
		case LDNS_RDF_TYPE_DNAME:
			res = ldns_rdf2buffer_str_dname(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_INT8: /* Don't output mnemonics for these */
		case LDNS_RDF_TYPE_ALG:
		case LDNS_RDF_TYPE_CERTIFICATE_USAGE:
		case LDNS_RDF_TYPE_SELECTOR:
		case LDNS_RDF_TYPE_MATCHING_TYPE:
			res = ldns_rdf2buffer_str_int8(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_INT16:
			res = ldns_rdf2buffer_str_int16(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_INT32:
			res = ldns_rdf2buffer_str_int32(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_PERIOD:
			res = ldns_rdf2buffer_str_period(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_TSIGTIME:
			res = ldns_rdf2buffer_str_tsigtime(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_A:
			res = ldns_rdf2buffer_str_a(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_AAAA:
			res = ldns_rdf2buffer_str_aaaa(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_STR:
			res = ldns_rdf2buffer_str_str(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_APL:
			res = ldns_rdf2buffer_str_apl(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_B32_EXT:
			res = ldns_rdf2buffer_str_b32_ext(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_B64:
			res = ldns_rdf2buffer_str_b64(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_HEX:
			res = ldns_rdf2buffer_str_hex(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_NSEC:
			res = ldns_rdf2buffer_str_nsec_fmt(buffer, fmt, rdf);
			break;
		case LDNS_RDF_TYPE_NSEC3_SALT:
			res = ldns_rdf2buffer_str_nsec3_salt(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_TYPE:
			res = ldns_rdf2buffer_str_type_fmt(buffer, fmt, rdf);
			break;
		case LDNS_RDF_TYPE_CLASS:
			res = ldns_rdf2buffer_str_class(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_CERT_ALG:
			res = ldns_rdf2buffer_str_cert_alg(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_UNKNOWN:
			res = ldns_rdf2buffer_str_unknown(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_TIME:
			res = ldns_rdf2buffer_str_time(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_HIP:
			res = ldns_rdf2buffer_str_hip(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_LOC:
			res = ldns_rdf2buffer_str_loc(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_WKS:
		case LDNS_RDF_TYPE_SERVICE:
			res = ldns_rdf2buffer_str_wks(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_NSAP:
			res = ldns_rdf2buffer_str_nsap(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_ATMA:
			res = ldns_rdf2buffer_str_atma(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_IPSECKEY:
			res = ldns_rdf2buffer_str_ipseckey(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_INT16_DATA:
			res = ldns_rdf2buffer_str_int16_data(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_NSEC3_NEXT_OWNER:
			res = ldns_rdf2buffer_str_b32_ext(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_ILNP64:
			res = ldns_rdf2buffer_str_ilnp64(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_EUI48:
			res = ldns_rdf2buffer_str_eui48(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_EUI64:
			res = ldns_rdf2buffer_str_eui64(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_TAG:
			res = ldns_rdf2buffer_str_tag(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_LONG_STR:
			res = ldns_rdf2buffer_str_long_str(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_AMTRELAY:
			res = ldns_rdf2buffer_str_amtrelay(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_SVCPARAMS:
			res = ldns_rdf2buffer_str_svcparams(buffer, rdf);
			break;
		}
	} else {
		/** This will write mangled RRs */
		ldns_buffer_printf(buffer, "(null) ");
		res = LDNS_STATUS_ERR;
	}
	return res;
}

ldns_status
ldns_rdf2buffer_str(ldns_buffer *buffer, const ldns_rdf *rdf)
{
	return ldns_rdf2buffer_str_fmt(buffer,ldns_output_format_default,rdf);
}

static ldns_rdf *
ldns_b32_ext2dname(const ldns_rdf *rdf)
{
	size_t size;
	char *b32;
	ldns_rdf *out;
	if(ldns_rdf_size(rdf) == 0)
		return NULL;
        /* remove -1 for the b32-hash-len octet */
	size = ldns_b32_ntop_calculate_size(ldns_rdf_size(rdf) - 1);
        /* add one for the end nul for the string */
	b32 = LDNS_XMALLOC(char, size + 2);
	if (b32) {
		if (ldns_b32_ntop_extended_hex(ldns_rdf_data(rdf) + 1, 
				ldns_rdf_size(rdf) - 1, b32, size+1) > 0) {
			b32[size] = '.';
			b32[size+1] = '\0';
			if (ldns_str2rdf_dname(&out, b32) == LDNS_STATUS_OK) {
				LDNS_FREE(b32);
				return out;
			}
		}
		LDNS_FREE(b32);
	}
	return NULL;
}

static ldns_status
ldns_rr2buffer_str_rfc3597(ldns_buffer *output, const ldns_rr *rr)
{
	size_t total_rdfsize = 0;
	size_t i, j;

	ldns_buffer_printf(output, "TYPE%u\t", ldns_rr_get_type(rr));
	for (i = 0; i < ldns_rr_rd_count(rr); i++) {
		total_rdfsize += ldns_rdf_size(ldns_rr_rdf(rr, i));
	}
	if (total_rdfsize == 0) {
		ldns_buffer_printf(output, "\\# 0\n");
		return ldns_buffer_status(output);
	}
	ldns_buffer_printf(output, "\\# %d ", total_rdfsize);
	for (i = 0; i < ldns_rr_rd_count(rr); i++) {
		for (j = 0; j < ldns_rdf_size(ldns_rr_rdf(rr, i)); j++) {
			ldns_buffer_printf(output, "%.2x",
					ldns_rdf_data(ldns_rr_rdf(rr, i))[j]);
		}
	}
	ldns_buffer_printf(output, "\n");
	return ldns_buffer_status(output);
}

ldns_status
ldns_rr2buffer_str_fmt(ldns_buffer *output, 
		const ldns_output_format *fmt, const ldns_rr *rr)
{
	uint16_t i, flags;
	ldns_status status = LDNS_STATUS_OK;
	ldns_output_format_storage* fmt_st = (ldns_output_format_storage*)fmt;

	if (fmt_st == NULL) {
		fmt_st = (ldns_output_format_storage*)
			  ldns_output_format_default;
	}
	if (!(fmt_st->flags & LDNS_FMT_SHORT)) {
		if (!rr) {
			if (LDNS_COMMENT_NULLS & fmt_st->flags) {
				ldns_buffer_printf(output, "; (null)\n");
			}
			return ldns_buffer_status(output);
		}
		if (ldns_rr_owner(rr)) {
			status = ldns_rdf2buffer_str_dname(output, ldns_rr_owner(rr));
		}
		if (status != LDNS_STATUS_OK) {
			return status;
		}

		/* TTL should NOT be printed if it is a question */
		if (!ldns_rr_is_question(rr)) {
			ldns_buffer_printf(output, "\t%u", (unsigned)ldns_rr_ttl(rr));
		}

		ldns_buffer_printf(output, "\t");
		status = ldns_rr_class2buffer_str(output, ldns_rr_get_class(rr));
		if (status != LDNS_STATUS_OK) {
			return status;
		}
		ldns_buffer_printf(output, "\t");

		if (ldns_output_format_covers_type(fmt, ldns_rr_get_type(rr))) {
			return ldns_rr2buffer_str_rfc3597(output, rr);
		}
		status = ldns_rr_type2buffer_str(output, ldns_rr_get_type(rr));
		if (status != LDNS_STATUS_OK) {
			return status;
		}

		if (ldns_rr_rd_count(rr) > 0) {
			ldns_buffer_printf(output, "\t");
		} else if (!ldns_rr_is_question(rr)) {
			ldns_buffer_printf(output, "\t\\# 0");
		}
	} else if (ldns_rr_rd_count(rr) == 0) {
		/* assert(fmt_st->flags & LDNS_FMT_SHORT); */

		ldns_buffer_printf(output, "# 0");
	}
	for (i = 0; i < ldns_rr_rd_count(rr); i++) {
		/* ldns_rdf2buffer_str handles NULL input fine! */
		if ((fmt_st->flags & LDNS_FMT_ZEROIZE_RRSIGS) &&
				(ldns_rr_get_type(rr) == LDNS_RR_TYPE_RRSIG) &&
				((/* inception  */ i == 4 &&
				  ldns_rdf_get_type(ldns_rr_rdf(rr, 4)) == 
							LDNS_RDF_TYPE_TIME) ||
				  (/* expiration */ i == 5 &&
				   ldns_rdf_get_type(ldns_rr_rdf(rr, 5)) ==
				   			LDNS_RDF_TYPE_TIME) ||
				  (/* signature  */ i == 8 &&
				   ldns_rdf_get_type(ldns_rr_rdf(rr, 8)) ==
				   			LDNS_RDF_TYPE_B64))) {

			ldns_buffer_printf(output, "(null)");
			status = ldns_buffer_status(output);
		} else if ((fmt_st->flags & LDNS_FMT_PAD_SOA_SERIAL) &&
				(ldns_rr_get_type(rr) == LDNS_RR_TYPE_SOA) &&
				/* serial */ i == 2 &&
			 	ldns_rdf_get_type(ldns_rr_rdf(rr, 2)) ==
			 				LDNS_RDF_TYPE_INT32) {
			ldns_buffer_printf(output, "%10lu",
				(unsigned long) ldns_read_uint32(
					ldns_rdf_data(ldns_rr_rdf(rr, 2))));
			status = ldns_buffer_status(output);
		} else {
			status = ldns_rdf2buffer_str_fmt(output,
					fmt, ldns_rr_rdf(rr, i));
		}
		if(status != LDNS_STATUS_OK)
			return status;
		if (i < ldns_rr_rd_count(rr) - 1) {
			ldns_buffer_printf(output, " ");
		}
	}
	/* per RR special comments - handy for DNSSEC types */
	/* check to prevent question sec. rr from
	 * getting here */
	if (ldns_rr_rd_count(rr) > 0) {
		switch (ldns_rr_get_type(rr)) {
		case LDNS_RR_TYPE_DNSKEY:
			/* if ldns_rr_rd_count(rr) > 0
				then ldns_rr_rdf(rr, 0) exists! */
			if (! (fmt_st->flags & LDNS_COMMENT_KEY)) {
				break;
			}
			flags = ldns_rdf2native_int16(ldns_rr_rdf(rr, 0));
			ldns_buffer_printf(output, " ;{");
			if (fmt_st->flags & LDNS_COMMENT_KEY_ID) {
				ldns_buffer_printf(output, "id = %u",
					(unsigned int) ldns_calc_keytag(rr));
			}
			if ((fmt_st->flags & LDNS_COMMENT_KEY_TYPE) &&
					(flags & LDNS_KEY_ZONE_KEY)){

				if (flags & LDNS_KEY_SEP_KEY) {
					ldns_buffer_printf(output, " (ksk)");
				} else {
					ldns_buffer_printf(output, " (zsk)");
				}
				if (fmt_st->flags & LDNS_COMMENT_KEY_SIZE){
					ldns_buffer_printf(output, ", ");
				}
			} else if (fmt_st->flags
					& (LDNS_COMMENT_KEY_ID
						|LDNS_COMMENT_KEY_SIZE)) {
				ldns_buffer_printf( output, ", ");
			}
			if (fmt_st->flags & LDNS_COMMENT_KEY_SIZE) {
				ldns_buffer_printf(output, "size = %db",
					ldns_rr_dnskey_key_size(rr));
			}
			ldns_buffer_printf(output, "}");
			break;
		case LDNS_RR_TYPE_RRSIG:
			if ((fmt_st->flags & LDNS_COMMENT_KEY)
					&& (fmt_st->flags& LDNS_COMMENT_RRSIGS)
					&& ldns_rr_rdf(rr, 6) != NULL) {
				ldns_buffer_printf(output, " ;{id = %d}",
						ldns_rdf2native_int16(
							ldns_rr_rdf(rr, 6)));
			}
			break;
		case LDNS_RR_TYPE_DS:
			if ((fmt_st->flags & LDNS_COMMENT_BUBBLEBABBLE) &&
					ldns_rr_rdf(rr, 3) != NULL) {

				uint8_t *data = ldns_rdf_data(
						ldns_rr_rdf(rr, 3));
				size_t len = ldns_rdf_size(ldns_rr_rdf(rr, 3));
				char *babble = ldns_bubblebabble(data, len);
				if(babble) {
					ldns_buffer_printf(output,
							" ;{%s}", babble);
				}
				LDNS_FREE(babble);
			}
			break;
		case LDNS_RR_TYPE_NSEC3:
			if (! (fmt_st->flags & LDNS_COMMENT_FLAGS) &&
				! (fmt_st->flags & LDNS_COMMENT_NSEC3_CHAIN)) {
				break;
			}
			ldns_buffer_printf(output, " ;{");
			if ((fmt_st->flags & LDNS_COMMENT_FLAGS)) {
				if (ldns_nsec3_optout(rr)) {
					ldns_buffer_printf(output,
						" flags: optout");
				} else {
					ldns_buffer_printf(output," flags: -");
				}
				if (fmt_st->flags & LDNS_COMMENT_NSEC3_CHAIN &&
						fmt_st->hashmap != NULL) {
					ldns_buffer_printf(output, ", ");
				}
			}
			if (fmt_st->flags & LDNS_COMMENT_NSEC3_CHAIN &&
					fmt_st->hashmap != NULL) {
				ldns_rbnode_t *node;
				ldns_rdf *key = ldns_dname_label(
						ldns_rr_owner(rr), 0);
				if (key) {
					node = ldns_rbtree_search(
						fmt_st->hashmap,
						(void *) key);
					if (node->data) {
						ldns_buffer_printf(output,
							"from: ");
						(void) ldns_rdf2buffer_str(
							output,
							ldns_dnssec_name_name(
							   (ldns_dnssec_name*)
							   node->data
							));
					}
					ldns_rdf_deep_free(key);
				}
				key = ldns_b32_ext2dname(
						ldns_nsec3_next_owner(rr));
				if (key) {
					node = ldns_rbtree_search(
						fmt_st->hashmap,
						(void *) key);
					if (node->data) {
						ldns_buffer_printf(output,
							" to: ");
						(void) ldns_rdf2buffer_str(
							output,
							ldns_dnssec_name_name(
							   (ldns_dnssec_name*)
							   node->data
							));
					}
					ldns_rdf_deep_free(key);
				}
			}
			ldns_buffer_printf(output, "}");
			break;
		default:
			break;

		}
	}
	/* last */
	ldns_buffer_printf(output, "\n");
	return ldns_buffer_status(output);
}

ldns_status
ldns_rr2buffer_str(ldns_buffer *output, const ldns_rr *rr)
{
	return ldns_rr2buffer_str_fmt(output, ldns_output_format_default, rr);
}

ldns_status
ldns_rr_list2buffer_str_fmt(ldns_buffer *output, 
		const ldns_output_format *fmt, const ldns_rr_list *list)
{
	uint16_t i;

	for(i = 0; i < ldns_rr_list_rr_count(list); i++) {
		(void) ldns_rr2buffer_str_fmt(output, fmt, 
				ldns_rr_list_rr(list, i));
	}
	return ldns_buffer_status(output);
}

ldns_status
ldns_rr_list2buffer_str(ldns_buffer *output, const ldns_rr_list *list)
{
	return ldns_rr_list2buffer_str_fmt(
			output, ldns_output_format_default, list);
}

ldns_status
ldns_pktheader2buffer_str(ldns_buffer *output, const ldns_pkt *pkt)
{
	ldns_lookup_table *opcode = ldns_lookup_by_id(ldns_opcodes,
			                    (int) ldns_pkt_get_opcode(pkt));
	ldns_lookup_table *rcode = ldns_lookup_by_id(ldns_rcodes,
			                    (int) ldns_pkt_get_rcode(pkt));

	ldns_buffer_printf(output, ";; ->>HEADER<<- ");
	if (opcode) {
		ldns_buffer_printf(output, "opcode: %s, ", opcode->name);
	} else {
		ldns_buffer_printf(output, "opcode: ?? (%u), ",
				ldns_pkt_get_opcode(pkt));
	}
	if (rcode) {
		ldns_buffer_printf(output, "rcode: %s, ", rcode->name);
	} else {
		ldns_buffer_printf(output, "rcode: ?? (%u), ", ldns_pkt_get_rcode(pkt));
	}
	ldns_buffer_printf(output, "id: %d\n", ldns_pkt_id(pkt));
	ldns_buffer_printf(output, ";; flags: ");

	if (ldns_pkt_qr(pkt)) {
		ldns_buffer_printf(output, "qr ");
	}
	if (ldns_pkt_aa(pkt)) {
		ldns_buffer_printf(output, "aa ");
	}
	if (ldns_pkt_tc(pkt)) {
		ldns_buffer_printf(output, "tc ");
	}
	if (ldns_pkt_rd(pkt)) {
		ldns_buffer_printf(output, "rd ");
	}
	if (ldns_pkt_cd(pkt)) {
		ldns_buffer_printf(output, "cd ");
	}
	if (ldns_pkt_ra(pkt)) {
		ldns_buffer_printf(output, "ra ");
	}
	if (ldns_pkt_ad(pkt)) {
		ldns_buffer_printf(output, "ad ");
	}
	ldns_buffer_printf(output, "; ");
	ldns_buffer_printf(output, "QUERY: %u, ", ldns_pkt_qdcount(pkt));
	ldns_buffer_printf(output, "ANSWER: %u, ", ldns_pkt_ancount(pkt));
	ldns_buffer_printf(output, "AUTHORITY: %u, ", ldns_pkt_nscount(pkt));
	ldns_buffer_printf(output, "ADDITIONAL: %u ", ldns_pkt_arcount(pkt));
	return ldns_buffer_status(output);
}


/* print EDNS option data in the Dig format: 76 61 6c 69 ... */
static void
ldns_edns_hex_data2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	size_t j;
	for (j = 0; j < len; j++) {
		ldns_buffer_printf(output, " %02x", data[j]);
	}
}

static ldns_status
ldns_edns_llq2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	/* LLQ constants */
	const char* llq_errors[] = {"NO-ERROR", "SERV-FULL", "STATIC",
		"FORMAT-ERR", "NO-SUCH-LLQ", "BAD-VERS", "UNKNOWN_ERR"};
	const unsigned int llq_errors_num = 7;
	const char* llq_opcodes[] = {"LLQ-SETUP", "LLQ-REFRESH", "LLQ-EVENT"};
	const unsigned int llq_opcodes_num = 3;

	uint16_t version, llq_opcode, error_code;
	uint64_t llq_id;
	uint32_t lease_life; /* Requested or granted life of LLQ, in seconds */

	ldns_buffer_printf(output, "; Long-Lived Query:");

	/* read the record */
	if(len != 18) {
		ldns_buffer_printf(output, " malformed LLQ ");
		ldns_edns_hex_data2buffer_str(output, data, len);

		return ldns_buffer_status(output);
	}
	version = ldns_read_uint16(data);
	llq_opcode = ldns_read_uint16(data+2);
	error_code = ldns_read_uint16(data+4);
	memmove(&llq_id, data+6, sizeof(uint64_t));
	lease_life = ldns_read_uint32(data+14);

	/* print option field entires */
	ldns_buffer_printf(output, "v%d ", (int)version);

	if(llq_opcode < llq_opcodes_num) {
		ldns_buffer_printf(output, "%s", llq_opcodes[llq_opcode]);
	} else {
		ldns_buffer_printf(output, "opcode %d", (int)llq_opcode);
	}

	if(error_code < llq_errors_num)
		ldns_buffer_printf(output, " %s", llq_errors[error_code]);
	else {
		ldns_buffer_printf(output, " error %d", (int)error_code);
	}

#ifndef USE_WINSOCK
	ldns_buffer_printf(output, " id %llx lease-life %lu",
		(unsigned long long)llq_id, (unsigned long)lease_life);
#else
	ldns_buffer_printf(output, " id %I64x lease-life %lu",
		(unsigned long long)llq_id, (unsigned long)lease_life);
#endif
	return ldns_buffer_status(output);
}


static ldns_status
ldns_edns_ul2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	uint32_t lease;

	ldns_buffer_printf(output, "; Update Lease:");

	if(len != 4) {
		ldns_buffer_printf(output, " malformed UL ");
		ldns_edns_hex_data2buffer_str(output, data, len);
		return ldns_buffer_status(output);
	}
	lease = ldns_read_uint32(data);
	ldns_buffer_printf(output, "lease %lu", (unsigned long)lease);

	return ldns_buffer_status(output);
}

static ldns_status
ldns_edns_nsid2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	size_t i, printed=0;

	ldns_buffer_printf(output, "; NSID:");
	ldns_edns_hex_data2buffer_str(output, data, len);

	/* print the human-readable text string */
	for(i = 0; i < len; i++) {
		if(isprint((unsigned char)data[i]) || data[i] == '\t') {
			if(!printed) {
				ldns_buffer_printf(output, " (");
				printed = 1;
			}
			ldns_buffer_printf(output, "%c", (char)data[i]);
		}
	}
	if(printed)
		ldns_buffer_printf(output, ")");
	return ldns_buffer_status(output);
}


static ldns_status
ldns_edns_dau2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	size_t i;
	ldns_lookup_table *lt;

	ldns_buffer_printf(output, "; DNSSEC Algorithm Understood (DAU):");

	for(i = 0; i <len; i++) {
		lt = ldns_lookup_by_id(ldns_algorithms, data[i]);
		if (lt && lt->name) {
			ldns_buffer_printf(output, " %s", lt->name);
		} else {
			ldns_buffer_printf(output, " ALG%u", data[i]);
		}
	}
	return ldns_buffer_status(output);
}

static ldns_status
ldns_edns_dhu2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	size_t i;
	ldns_lookup_table *lt;

	ldns_buffer_printf(output, "; DS Hash Understood (DHU):");

	for(i = 0; i < len; i++) {
		lt = ldns_lookup_by_id(ldns_hashes, data[i]);
		if (lt && lt->name) {
			ldns_buffer_printf(output, " %s", lt->name);
		} else {
			ldns_buffer_printf(output, " ALG%u", data[i]);
		}
	}
	return ldns_buffer_status(output);
}

static ldns_status
ldns_edns_d3u2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	size_t i;

	ldns_buffer_printf(output, "; NSEC3 Hash Understood (N3U):");

	for(i=0; i<len; i++) {
		if(data[i] == 1) {
			ldns_buffer_printf(output, " SHA1");
		} else {
			ldns_buffer_printf(output, " %d", (int)data[i]);
		}
	}
	return ldns_buffer_status(output);
}

static ldns_status
ldns_edns_subnet2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	uint16_t family;
	uint8_t source, scope;
	if(len < 4) {
		ldns_buffer_printf(output, "malformed subnet ");
		ldns_edns_hex_data2buffer_str(output, data, len);
		return ldns_buffer_status(output);
	}
	family = ldns_read_uint16(data);
	source = data[2];
	scope = data[3];
	if(family == 1) {
		/* IPv4 */
		char buf[64];
		uint8_t ip4[4];
		memset(ip4, 0, sizeof(ip4));
		if(len-4 > 4) {
			ldns_buffer_printf(output, "trailingdata:");
			ldns_edns_hex_data2buffer_str(output, data+4+4, len-4-4);
			ldns_buffer_printf(output, " ");
			len = 4+4;
		}
		memmove(ip4, data+4, len-4);
		if(!inet_ntop(AF_INET, ip4, buf, (socklen_t) sizeof(buf))) {
			ldns_buffer_printf(output, "ip4ntoperror ");
			ldns_edns_hex_data2buffer_str(output, data+4+4, len-4-4);
		} else {
			ldns_buffer_printf(output, "%s", buf);
		}
	} else if(family == 2) {
		/* IPv6 */
		char buf[64];
		uint8_t ip6[16];
		memset(ip6, 0, sizeof(ip6));
		if(len-4 > 16) {
			ldns_buffer_printf(output, "trailingdata:");
			ldns_edns_hex_data2buffer_str(output, data+4+16, len-4-16);
			ldns_buffer_printf(output, " ");
			len = 4+16;
		}
		memmove(ip6, data+4, len-4);
#ifdef AF_INET6
		if(!inet_ntop(AF_INET6, ip6, buf, (socklen_t) sizeof(buf))) {
			ldns_buffer_printf(output, "ip6ntoperror ");
			ldns_edns_hex_data2buffer_str(output, data+4+4, len-4-4);
		} else {
			ldns_buffer_printf(output, "%s", buf);
		}
#else
		ldns_edns_hex_data2buffer_str(output,  data+4+4, len-4-4);
#endif
	} else {
		/* unknown */
		ldns_buffer_printf(output, "family %d ", (int)family);
		ldns_edns_hex_data2buffer_str(output, data, len);
	}
	ldns_buffer_printf(output, "/%d scope /%d", (int)source, (int)scope);

	return ldns_buffer_status(output);
}

static ldns_status
ldns_edns_expire2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{

	ldns_buffer_printf(output, "; EXPIRE:");

	if (!(len == 0) || len == 4) {
		ldns_buffer_printf(output, "malformed expire ");
		ldns_edns_hex_data2buffer_str(output, data, len);

		return ldns_buffer_status(output);
	}

	// TODO can this output be more accurate?
	ldns_edns_hex_data2buffer_str(output, data, len);

	return ldns_buffer_status(output);
}


static ldns_status
ldns_edns_cookie2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	ldns_buffer_printf(output, "; COOKIE:");

	/* the size of an EDNS cookie is restricted by RFC 7873 */
	if (!(len == 8 || (len >= 16 && len < 40))) {
		ldns_buffer_printf(output, "malformed cookie ");
		ldns_edns_hex_data2buffer_str(output, data, len);
	}
	ldns_edns_hex_data2buffer_str(output, data, len);

	return ldns_buffer_status(output);
}

static ldns_status
ldns_edns_keepalive2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	uint16_t timeout;

	ldns_buffer_printf(output, "; KEEPALIVE:");

	if(!(len == 0 || len == 2)) {
		ldns_buffer_printf(output, "malformed keepalive ");
		ldns_edns_hex_data2buffer_str(output, data, len);

		return ldns_buffer_status(output);
	}

	if(len == 0) {
		ldns_buffer_printf(output, "no timeout value (only valid for client option)");
	} else {
		timeout = ldns_read_uint16(data);
		ldns_buffer_printf(output, "timeout value in units of 100ms %u", (int)timeout);
	}
	return ldns_buffer_status(output);
}

static ldns_status
ldns_edns_padding2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	ldns_buffer_printf(output, "; PADDING: ");
	ldns_edns_hex_data2buffer_str(output, data, len);

	return ldns_buffer_status(output);
}

static ldns_status
ldns_edns_chain2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	ldns_rdf** temp = NULL;

	ldns_buffer_printf(output, "; CHAIN: ");

	if (ldns_str2rdf_dname(temp, (char*) data) != LDNS_STATUS_OK) {
		ldns_buffer_printf(output, "malformed chain ");
		ldns_edns_hex_data2buffer_str(output, data, len);

		return ldns_buffer_status(output);
	}

	ldns_characters2buffer_str(output, len, data);

	return ldns_buffer_status(output);
}

static ldns_status
ldns_edns_key_tag2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	size_t i;

	ldns_buffer_printf(output, "; KEY TAG: ");

	if(len < 2 || len % 2 != 0) {
		ldns_buffer_printf(output, "malformed key tag ");
		ldns_edns_hex_data2buffer_str(output, data, len);

		return ldns_buffer_status(output);
	}

	for (i = 0; i < len; i += 2) {
		uint16_t tag = ldns_read_uint16(data);

		ldns_buffer_printf(output, " %hu", tag);
	}

	return ldns_buffer_status(output);
}

static ldns_status
ldns_edns_ede2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	size_t i;
	uint16_t ede;
	ldns_buffer_printf(output, "; EDE:");

	if(len < 2) {
		ldns_buffer_printf(output, "malformed ede ");
		ldns_edns_hex_data2buffer_str(output, data, len);

		return ldns_buffer_status(output);
	}

	ede = ldns_read_uint16(data);

	switch (ede) {
	case LDNS_EDE_OTHER:
		ldns_buffer_printf(output, " 0 (Other): ");
		break;
	case LDNS_EDE_UNSUPPORTED_DNSKEY_ALG:
		ldns_buffer_printf(output, " 1 (Unsupported DNSKEY Algorithm)");
		break;
	case LDNS_EDE_UNSUPPORTED_DS_DIGEST:
		ldns_buffer_printf(output, " 2 (Unsupported DS Digest type)");
		break;
	case LDNS_EDE_STALE_ANSWER:
		ldns_buffer_printf(output, " 3 (Stale Answer)");
		break;
	case LDNS_EDE_FORGED_ANSWER:
		ldns_buffer_printf(output, " 4 (Forged Answer)");
		break;
	case LDNS_EDE_DNSSEC_INDETERMINATE:
		ldns_buffer_printf(output, " 5 (DNSSEC Indeterminate)");
		break;
	case LDNS_EDE_DNSSEC_BOGUS:
		ldns_buffer_printf(output, " 6 (DNSSEC Bogus)");
		break;
	case LDNS_EDE_SIGNATURE_EXPIRED:
		ldns_buffer_printf(output, " 7 (Signature Expired)");
		break;
	case LDNS_EDE_SIGNATURE_NOT_YET_VALID:
		ldns_buffer_printf(output, " 8 (Signature Not Yet Valid)");
		break;
	case LDNS_EDE_DNSKEY_MISSING:
		ldns_buffer_printf(output, " 9 (DNSKEY Missing)");
		break;
	case LDNS_EDE_RRSIGS_MISSING:
		ldns_buffer_printf(output, " 10 (RRSIGs Missing)");
		break;
	case LDNS_EDE_NO_ZONE_KEY_BIT_SET:
		ldns_buffer_printf(output, " 11 (No Zone Key Bit Set)");
		break;
	case LDNS_EDE_NSEC_MISSING:
		ldns_buffer_printf(output, " 12 (NSEC Missing)");
		break;
	case LDNS_EDE_CACHED_ERROR:
		ldns_buffer_printf(output, " 13 (Cached Error)");
		break;
	case LDNS_EDE_NOT_READY:
		ldns_buffer_printf(output, " 14 (Not Ready)");
		break;
	case LDNS_EDE_BLOCKED:
		ldns_buffer_printf(output, " 15 (Blocked)");
		break;
	case LDNS_EDE_CENSORED:
		ldns_buffer_printf(output, " 16 (Censored)");
		break;
	case LDNS_EDE_FILTERED:
		ldns_buffer_printf(output, " 17 (Filtered)");
		break;
	case LDNS_EDE_PROHIBITED:
		ldns_buffer_printf(output, " 18 (Prohibited)");
		break;
	case LDNS_EDE_STALE_NXDOMAIN_ANSWER:
		ldns_buffer_printf(output, " 19 (NXDOMAIN Answer)");
		break;
	case LDNS_EDE_NOT_AUTHORITATIVE:
		ldns_buffer_printf(output, " 20 (Not Authoritative)");
		break;
	case LDNS_EDE_NOT_SUPPORTED:
		ldns_buffer_printf(output, " 21 (Not Supported)");
		break;
	case LDNS_EDE_NO_REACHABLE_AUTHORITY:
		ldns_buffer_printf(output, " 22 (No Reachable Authority)");
		break;
	case LDNS_EDE_NETWORK_ERROR:
		ldns_buffer_printf(output, " 23 (Network Error)");
		break;
	case LDNS_EDE_INVALID_DATA:
		ldns_buffer_printf(output, " 24 (Invalid Data)");
		break;
	case LDNS_EDE_SIGNATURE_EXPIRED_BEFORE_VALID:
		ldns_buffer_printf(output, " 25 (Signature Expired Before Valid)");
		break;
	case LDNS_EDE_TOO_EARLY:
		ldns_buffer_printf(output, " 26 (Too Early)");
		break;
	default:
		ldns_buffer_printf(output, " %02x", data[0]);
		ldns_buffer_printf(output, " %02x", data[1]);
		break;
	}

	/* skip the EDE code in the output */
	data += 2;
	len -= 2;

	if (len > 2) {
		/* format the hex bytes */
		ldns_buffer_printf(output, ":");
		for (i = 0; i < len; i++) {
			ldns_buffer_printf(output, " %02x", data[i]);
		}

		/* format the human-readable string */
		ldns_buffer_printf(output, " (");
		ldns_characters2buffer_str(output, len, data);
		ldns_buffer_printf(output, ")");
	}

	return ldns_buffer_status(output);
}

static ldns_status
ldns_edns_client_tag2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	ldns_buffer_printf(output, "; CLIENT-TAG:");

	if (len > 2) {
		ldns_buffer_printf(output, "malformed client-tag ");
		ldns_edns_hex_data2buffer_str(output, data, len);

		return ldns_buffer_status(output);
	}

	ldns_edns_hex_data2buffer_str(output, data, len);

	return ldns_buffer_status(output);
}

static ldns_status
ldns_edns_server_tag2buffer_str(ldns_buffer* output, uint8_t* data, size_t len)
{
	ldns_buffer_printf(output, "; SERVER-TAG:");

	if (len > 2) {
		ldns_buffer_printf(output, "malformed server-tag ");
		ldns_edns_hex_data2buffer_str(output, data, len);

		return ldns_buffer_status(output);
	}

	ldns_edns_hex_data2buffer_str(output, data, len);

	return ldns_buffer_status(output);
}

ldns_status
ldns_edns_option_list2buffer_str(ldns_buffer *output, ldns_edns_option_list* edns_list)
{
	size_t count = ldns_edns_option_list_get_count(edns_list);
	size_t i, size;
	uint8_t* data;

	for (i = 0; i < count; i++) {
		ldns_edns_option_code code;
		ldns_edns_option* edns = ldns_edns_option_list_get_option(edns_list, i);

		if (!edns) {
			break;
		}

		code = ldns_edns_get_code(edns);
		size = ldns_edns_get_size(edns);
		data = ldns_edns_get_data(edns);

		switch(code) {
		case LDNS_EDNS_LLQ:
			ldns_edns_llq2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_UL:
			ldns_edns_ul2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_NSID:
			ldns_edns_nsid2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_DAU:
			ldns_edns_dau2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_DHU:
			ldns_edns_dhu2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_N3U:
			ldns_edns_d3u2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_CLIENT_SUBNET:
			ldns_edns_subnet2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_EXPIRE:
			ldns_edns_expire2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_COOKIE:
			ldns_edns_cookie2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_KEEPALIVE:
			ldns_edns_keepalive2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_PADDING:
			ldns_edns_padding2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_CHAIN:
			ldns_edns_chain2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_KEY_TAG:
			ldns_edns_key_tag2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_EDE:
			ldns_edns_ede2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_CLIENT_TAG:
			ldns_edns_client_tag2buffer_str(output, data, size);
			break;
		case LDNS_EDNS_SERVER_TAG:
			ldns_edns_server_tag2buffer_str(output, data, size);
			break;
		default:
			ldns_buffer_printf(output, "; OPT=%d:", code);
			ldns_edns_hex_data2buffer_str(output, data, size);
			break;
		}
		ldns_buffer_printf(output, "\n");
	}

	return ldns_buffer_status(output);
}


ldns_status
ldns_pkt2buffer_str_fmt(ldns_buffer *output, 
		const ldns_output_format *fmt, const ldns_pkt *pkt)
{
	uint16_t i;
	ldns_status status = LDNS_STATUS_OK;
	char *tmp;
	struct timeval time;
	time_t time_tt;
	int short_fmt = fmt && (fmt->flags & LDNS_FMT_SHORT);

	if (!pkt) {
		ldns_buffer_printf(output, "null");
		return LDNS_STATUS_OK;
	}

	if (!ldns_buffer_status_ok(output)) {
		return ldns_buffer_status(output);
	}

	if (!short_fmt) {
		status = ldns_pktheader2buffer_str(output, pkt);
		if (status != LDNS_STATUS_OK) {
			return status;
		}

		ldns_buffer_printf(output, "\n");

		ldns_buffer_printf(output, ";; QUESTION SECTION:\n;; ");


		for (i = 0; i < ldns_pkt_qdcount(pkt); i++) {
			status = ldns_rr2buffer_str_fmt(output, fmt,
				       ldns_rr_list_rr(
					       ldns_pkt_question(pkt), i));
			if (status != LDNS_STATUS_OK) {
				return status;
			}
		}
		ldns_buffer_printf(output, "\n");

		ldns_buffer_printf(output, ";; ANSWER SECTION:\n");
	}
	for (i = 0; i < ldns_pkt_ancount(pkt); i++) {
		status = ldns_rr2buffer_str_fmt(output, fmt,
			       ldns_rr_list_rr(
				       ldns_pkt_answer(pkt), i));
		if (status != LDNS_STATUS_OK) {
			return status;
		}
	}
	if (!short_fmt) {
		ldns_buffer_printf(output, "\n");

		ldns_buffer_printf(output, ";; AUTHORITY SECTION:\n");

		for (i = 0; i < ldns_pkt_nscount(pkt); i++) {
			status = ldns_rr2buffer_str_fmt(output, fmt,
				       ldns_rr_list_rr(
					       ldns_pkt_authority(pkt), i));
			if (status != LDNS_STATUS_OK) {
				return status;
			}
		}
		ldns_buffer_printf(output, "\n");

		ldns_buffer_printf(output, ";; ADDITIONAL SECTION:\n");
		for (i = 0; i < ldns_pkt_arcount(pkt); i++) {
			status = ldns_rr2buffer_str_fmt(output, fmt,
				       ldns_rr_list_rr(
					       ldns_pkt_additional(pkt), i));
			if (status != LDNS_STATUS_OK) {
				return status;
			}

		}
		ldns_buffer_printf(output, "\n");
		/* add some further fields */
		ldns_buffer_printf(output, ";; Query time: %d msec\n",
				ldns_pkt_querytime(pkt));
		if (ldns_pkt_edns(pkt)) {
			ldns_buffer_printf(output,
				   ";; EDNS: version %u; flags:",
				   ldns_pkt_edns_version(pkt));
			if (ldns_pkt_edns_do(pkt)) {
				ldns_buffer_printf(output, " do");
			}
			/* the extended rcode is the value set, shifted four bits,
			 * and or'd with the original rcode */
			if (ldns_pkt_edns_extended_rcode(pkt)) {
				ldns_buffer_printf(output, " ; ext-rcode: %d",
					(ldns_pkt_edns_extended_rcode(pkt) << 4 | ldns_pkt_get_rcode(pkt)));
			}
			ldns_buffer_printf(output, " ; udp: %u\n",
					   ldns_pkt_edns_udp_size(pkt));

			if (pkt->_edns_list)
				ldns_edns_option_list2buffer_str(output, pkt->_edns_list);

			else if (ldns_pkt_edns_data(pkt)) {
				ldns_edns_option_list* edns_list;
				/* parse the EDNS data into separate EDNS options
				 * and add them to the list */
				if ((edns_list = pkt_edns_data2edns_option_list(ldns_pkt_edns_data(pkt)))) {
					ldns_edns_option_list2buffer_str(output, edns_list);
					ldns_edns_option_list_deep_free(edns_list);
				} else {
					ldns_buffer_printf(output, ";; Data: ");
					(void)ldns_rdf2buffer_str(output, ldns_pkt_edns_data(pkt));
				}
			}
		}
		if (ldns_pkt_tsig(pkt)) {
			ldns_buffer_printf(output, ";; TSIG:\n;; ");
			(void) ldns_rr2buffer_str_fmt(
					output, fmt, ldns_pkt_tsig(pkt));
			ldns_buffer_printf(output, "\n");
		}
		if (ldns_pkt_answerfrom(pkt)) {
			tmp = ldns_rdf2str(ldns_pkt_answerfrom(pkt));
			ldns_buffer_printf(output, ";; SERVER: %s\n", tmp);
			LDNS_FREE(tmp);
		}
		time = ldns_pkt_timestamp(pkt);
		time_tt = (time_t)time.tv_sec;
		ldns_buffer_printf(output, ";; WHEN: %s",
				(char*)ctime(&time_tt));

		ldns_buffer_printf(output, ";; MSG SIZE  rcvd: %d\n",
				(int)ldns_pkt_size(pkt));
	}
	return status;
}

ldns_status
ldns_pkt2buffer_str(ldns_buffer *output, const ldns_pkt *pkt)
{
	return ldns_pkt2buffer_str_fmt(output, ldns_output_format_default, pkt);
}


#ifdef HAVE_SSL
static ldns_status
ldns_hmac_key2buffer_str(ldns_buffer *output, const ldns_key *k)
{
	ldns_status status;
	size_t i;
	ldns_rdf *b64_bignum;

	ldns_buffer_printf(output, "Key: ");

 	i = ldns_key_hmac_size(k);
	b64_bignum =  ldns_rdf_new_frm_data(LDNS_RDF_TYPE_B64, i, ldns_key_hmac_key(k));
	status = ldns_rdf2buffer_str(output, b64_bignum);
	ldns_rdf_deep_free(b64_bignum);
	ldns_buffer_printf(output, "\n");
	return status;
}
#endif

#if defined(HAVE_SSL) && defined(USE_GOST)
static ldns_status
ldns_gost_key2buffer_str(ldns_buffer *output, EVP_PKEY *p)
{
	unsigned char* pp = NULL;
	int ret;
	ldns_rdf *b64_bignum;
	ldns_status status;

	ldns_buffer_printf(output, "GostAsn1: ");

	ret = i2d_PrivateKey(p, &pp);
	b64_bignum = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_B64, (size_t)ret, pp);
	status = ldns_rdf2buffer_str(output, b64_bignum);

	ldns_rdf_deep_free(b64_bignum);
	OPENSSL_free(pp);
	ldns_buffer_printf(output, "\n");
	return status;
}
#endif

#if defined(HAVE_SSL) && defined(USE_ED25519)
static ldns_status
ldns_ed25519_key2buffer_str(ldns_buffer *output, EVP_PKEY *p)
{
	unsigned char* pp = NULL;
	int ret;
	ldns_rdf *b64_bignum;
	ldns_status status;

	ldns_buffer_printf(output, "PrivateKey: ");

	ret = i2d_PrivateKey(p, &pp);
	/* 16 byte asn (302e020100300506032b657004220420) + 32byte key */
	if(ret != 16 + 32) {
		OPENSSL_free(pp);
		return LDNS_STATUS_ERR;
	}
	b64_bignum = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_B64,
		(size_t)ret-16, pp+16);
	status = ldns_rdf2buffer_str(output, b64_bignum);

	ldns_rdf_deep_free(b64_bignum);
	OPENSSL_free(pp);
	ldns_buffer_printf(output, "\n");
	return status;
}
#endif

#if defined(HAVE_SSL) && defined(USE_ED448)
static ldns_status
ldns_ed448_key2buffer_str(ldns_buffer *output, EVP_PKEY *p)
{
	unsigned char* pp = NULL;
	int ret;
	ldns_rdf *b64_bignum;
	ldns_status status;

	ldns_buffer_printf(output, "PrivateKey: ");

	ret = i2d_PrivateKey(p, &pp);
	/* some-ASN + 57byte key */
	if(ret != 16 + 57) {
		OPENSSL_free(pp);
		return LDNS_STATUS_ERR;
	}
	b64_bignum = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_B64,
		(size_t)ret-16, pp+16);
	status = ldns_rdf2buffer_str(output, b64_bignum);

	ldns_rdf_deep_free(b64_bignum);
	OPENSSL_free(pp);
	ldns_buffer_printf(output, "\n");
	return status;
}
#endif

#if defined(HAVE_SSL)
/** print one b64 encoded bignum to a line in the keybuffer */
static int
ldns_print_bignum_b64_line(ldns_buffer* output, const char* label, const BIGNUM* num)
{
	unsigned char  *bignumbuf = LDNS_XMALLOC(unsigned char, LDNS_MAX_KEYLEN);
	if(!bignumbuf) return 0;

	ldns_buffer_printf(output, "%s: ", label);
	if(num) {
		ldns_rdf *b64_bignum = NULL;
		int i = BN_bn2bin(num, bignumbuf);
		if (i > LDNS_MAX_KEYLEN) {
			LDNS_FREE(bignumbuf);
			return 0;
		}
		b64_bignum =  ldns_rdf_new_frm_data(LDNS_RDF_TYPE_B64, (size_t)i, bignumbuf);
		if (ldns_rdf2buffer_str(output, b64_bignum) != LDNS_STATUS_OK) {
			ldns_rdf_deep_free(b64_bignum);
			LDNS_FREE(bignumbuf);
			return 0;
		}
		ldns_rdf_deep_free(b64_bignum);
		ldns_buffer_printf(output, "\n");
	} else {
		ldns_buffer_printf(output, "(Not available)\n");
	}
	LDNS_FREE(bignumbuf);
	return 1;
}
#endif

ldns_status
ldns_key2buffer_str(ldns_buffer *output, const ldns_key *k)
{
	ldns_status status = LDNS_STATUS_OK;
	unsigned char  *bignum;
#ifdef HAVE_SSL
	RSA *rsa;
#ifdef USE_DSA
	DSA *dsa;
#endif /* USE_DSA */
#endif /* HAVE_SSL */

	if (!k) {
		return LDNS_STATUS_ERR;
	}

	bignum = LDNS_XMALLOC(unsigned char, LDNS_MAX_KEYLEN);
	if (!bignum) {
		return LDNS_STATUS_ERR;
	}

	if (ldns_buffer_status_ok(output)) {
#ifdef HAVE_SSL
		switch(ldns_key_algorithm(k)) {
			case LDNS_SIGN_RSASHA1:
			case LDNS_SIGN_RSASHA1_NSEC3:
			case LDNS_SIGN_RSASHA256:
			case LDNS_SIGN_RSASHA512:
			case LDNS_SIGN_RSAMD5:
				/* copied by looking at dnssec-keygen output */
				/* header */
				rsa = ldns_key_rsa_key(k);

				ldns_buffer_printf(output,"Private-key-format: v1.2\n");
				switch(ldns_key_algorithm(k)) {
				case LDNS_SIGN_RSAMD5:
					ldns_buffer_printf(output,
								    "Algorithm: %u (RSA)\n",
								    LDNS_RSAMD5);
					break;
				case LDNS_SIGN_RSASHA1:
					ldns_buffer_printf(output,
								    "Algorithm: %u (RSASHA1)\n",
								    LDNS_RSASHA1);
					break;
				case LDNS_SIGN_RSASHA1_NSEC3:
					ldns_buffer_printf(output,
								    "Algorithm: %u (RSASHA1_NSEC3)\n",
								    LDNS_RSASHA1_NSEC3);
					break;
#ifdef USE_SHA2
				case LDNS_SIGN_RSASHA256:
					ldns_buffer_printf(output,
								    "Algorithm: %u (RSASHA256)\n",
								    LDNS_RSASHA256);
					break;
				case LDNS_SIGN_RSASHA512:
					ldns_buffer_printf(output,
								    "Algorithm: %u (RSASHA512)\n",
								    LDNS_RSASHA512);
					break;
#endif
				default:
#ifdef STDERR_MSGS
					fprintf(stderr, "Warning: unknown signature ");
					fprintf(stderr,
						   "algorithm type %u\n",
						   ldns_key_algorithm(k));
#endif
					ldns_buffer_printf(output,
								    "Algorithm: %u (Unknown)\n",
								    ldns_key_algorithm(k));
					break;
				}

				/* print to buf, convert to bin, convert to b64,
				 * print to buf */

#ifndef S_SPLINT_S
				if(1) {
					const BIGNUM *n=NULL, *e=NULL, *d=NULL,
						*p=NULL, *q=NULL, *dmp1=NULL,
						*dmq1=NULL, *iqmp=NULL;
#if OPENSSL_VERSION_NUMBER < 0x10100000 || (defined(HAVE_LIBRESSL) && LIBRESSL_VERSION_NUMBER < 0x20700000)
					n = rsa->n;
					e = rsa->e;
					d = rsa->d;
					p = rsa->p;
					q = rsa->q;
					dmp1 = rsa->dmp1;
					dmq1 = rsa->dmq1;
					iqmp = rsa->iqmp;
#else
					RSA_get0_key(rsa, &n, &e, &d);
					RSA_get0_factors(rsa, &p, &q);
					RSA_get0_crt_params(rsa, &dmp1,
						&dmq1, &iqmp);
#endif
					if(!ldns_print_bignum_b64_line(output, "Modulus", n))
						goto error;
					if(!ldns_print_bignum_b64_line(output, "PublicExponent", e))
						goto error;
					if(!ldns_print_bignum_b64_line(output, "PrivateExponent", d))
						goto error;
					if(!ldns_print_bignum_b64_line(output, "Prime1", p))
						goto error;
					if(!ldns_print_bignum_b64_line(output, "Prime2", q))
						goto error;
					if(!ldns_print_bignum_b64_line(output, "Exponent1", dmp1))
						goto error;
					if(!ldns_print_bignum_b64_line(output, "Exponent2", dmq1))
						goto error;
					if(!ldns_print_bignum_b64_line(output, "Coefficient", iqmp))
						goto error;
				}
#endif /* splint */

				RSA_free(rsa);
				break;
#ifdef USE_DSA
			case LDNS_SIGN_DSA:
			case LDNS_SIGN_DSA_NSEC3:
				dsa = ldns_key_dsa_key(k);

				ldns_buffer_printf(output,"Private-key-format: v1.2\n");
				if (ldns_key_algorithm(k) == LDNS_SIGN_DSA) {
					ldns_buffer_printf(output,"Algorithm: 3 (DSA)\n");
				} else if (ldns_key_algorithm(k) == LDNS_SIGN_DSA_NSEC3) {
					ldns_buffer_printf(output,"Algorithm: 6 (DSA_NSEC3)\n");
				}

				/* print to buf, convert to bin, convert to b64,
				 * print to buf */
				if(1) {
					const BIGNUM *p=NULL, *q=NULL, *g=NULL,
						*priv_key=NULL, *pub_key=NULL;
#if OPENSSL_VERSION_NUMBER < 0x10100000 || (defined(HAVE_LIBRESSL) && LIBRESSL_VERSION_NUMBER < 0x20700000)
#ifndef S_SPLINT_S
					p = dsa->p;
					q = dsa->q;
					g = dsa->g;
					priv_key = dsa->priv_key;
					pub_key = dsa->pub_key;
#endif /* splint */
#else
					DSA_get0_pqg(dsa, &p, &q, &g);
					DSA_get0_key(dsa, &pub_key, &priv_key);
#endif
					if(!ldns_print_bignum_b64_line(output, "Prime(p)", p))
						goto error;
					if(!ldns_print_bignum_b64_line(output, "Subprime(q)", q))
						goto error;
					if(!ldns_print_bignum_b64_line(output, "Base(g)", g))
						goto error;
					if(!ldns_print_bignum_b64_line(output, "Private_value(x)", priv_key))
						goto error;
					if(!ldns_print_bignum_b64_line(output, "Public_value(y)", pub_key))
						goto error;
				}
				break;
#endif /* USE_DSA */
			case LDNS_SIGN_ECC_GOST:
				/* no format defined, use blob */
#if defined(HAVE_SSL) && defined(USE_GOST)
				ldns_buffer_printf(output, "Private-key-format: v1.2\n");
				ldns_buffer_printf(output, "Algorithm: %d (ECC-GOST)\n", LDNS_SIGN_ECC_GOST);
				status = ldns_gost_key2buffer_str(output, 
#ifndef S_SPLINT_S
					k->_key.key
#else
					NULL
#endif
				);
#else
				goto error;
#endif /* GOST */
				break;
			case LDNS_SIGN_ECDSAP256SHA256:
			case LDNS_SIGN_ECDSAP384SHA384:
#ifdef USE_ECDSA
                                ldns_buffer_printf(output, "Private-key-format: v1.2\n");
				ldns_buffer_printf(output, "Algorithm: %d (", ldns_key_algorithm(k));
                                status=ldns_algorithm2buffer_str(output, (ldns_algorithm)ldns_key_algorithm(k));
#ifndef S_SPLINT_S
				ldns_buffer_printf(output, ")\n");
                                if(k->_key.key) {
                                        EC_KEY* ec = EVP_PKEY_get1_EC_KEY(k->_key.key);
                                        const BIGNUM* b = EC_KEY_get0_private_key(ec);
					if(!ldns_print_bignum_b64_line(output, "PrivateKey", b))
						goto error;
                                        /* down reference count in EC_KEY
                                         * its still assigned to the PKEY */
                                        EC_KEY_free(ec);
                                }
#endif /* splint */
#else
				goto error;
#endif /* ECDSA */
                                break;
#ifdef USE_ED25519
			case LDNS_SIGN_ED25519:
                                ldns_buffer_printf(output, "Private-key-format: v1.2\n");
				ldns_buffer_printf(output, "Algorithm: %d (", ldns_key_algorithm(k));
                                status=ldns_algorithm2buffer_str(output, (ldns_algorithm)ldns_key_algorithm(k));
				ldns_buffer_printf(output, ")\n");
				if (status) break;
				status = ldns_ed25519_key2buffer_str(output,
					k->_key.key);
				break;
#endif /* USE_ED25519 */
#ifdef USE_ED448
			case LDNS_SIGN_ED448:
                                ldns_buffer_printf(output, "Private-key-format: v1.2\n");
				ldns_buffer_printf(output, "Algorithm: %d (", ldns_key_algorithm(k));
                                status=ldns_algorithm2buffer_str(output, (ldns_algorithm)ldns_key_algorithm(k));
				ldns_buffer_printf(output, ")\n");
				if (status) break;
				status = ldns_ed448_key2buffer_str(output,
					k->_key.key);
				break;
#endif /* USE_ED448 */
			case LDNS_SIGN_HMACMD5:
				/* there's not much of a format defined for TSIG */
				/* It's just a binary blob, Same for all algorithms */
                ldns_buffer_printf(output, "Private-key-format: v1.2\n");
                ldns_buffer_printf(output, "Algorithm: 157 (HMAC_MD5)\n");
				status = ldns_hmac_key2buffer_str(output, k);
				break;
			case LDNS_SIGN_HMACSHA1:
		        ldns_buffer_printf(output, "Private-key-format: v1.2\n");
		        ldns_buffer_printf(output, "Algorithm: 158 (HMAC_SHA1)\n");
				status = ldns_hmac_key2buffer_str(output, k);
				break;
			case LDNS_SIGN_HMACSHA224:
		        ldns_buffer_printf(output, "Private-key-format: v1.2\n");
		        ldns_buffer_printf(output, "Algorithm: 162 (HMAC_SHA224)\n");
				status = ldns_hmac_key2buffer_str(output, k);
				break;
			case LDNS_SIGN_HMACSHA256:
		        ldns_buffer_printf(output, "Private-key-format: v1.2\n");
		        ldns_buffer_printf(output, "Algorithm: 159 (HMAC_SHA256)\n");
				status = ldns_hmac_key2buffer_str(output, k);
				break;
			case LDNS_SIGN_HMACSHA384:
		        ldns_buffer_printf(output, "Private-key-format: v1.2\n");
		        ldns_buffer_printf(output, "Algorithm: 164 (HMAC_SHA384)\n");
				status = ldns_hmac_key2buffer_str(output, k);
				break;
			case LDNS_SIGN_HMACSHA512:
		        ldns_buffer_printf(output, "Private-key-format: v1.2\n");
		        ldns_buffer_printf(output, "Algorithm: 165 (HMAC_SHA512)\n");
				status = ldns_hmac_key2buffer_str(output, k);
				break;
		}
#endif /* HAVE_SSL */
	} else {
		LDNS_FREE(bignum);
		return ldns_buffer_status(output);
	}
	LDNS_FREE(bignum);
	return status;

#ifdef HAVE_SSL
	/* compiles warn the label isn't used */
error:
	LDNS_FREE(bignum);
	return LDNS_STATUS_ERR;
#endif /* HAVE_SSL */

}

/*
 * Zero terminate the buffer and copy data.
 */
char *
ldns_buffer2str(ldns_buffer *buffer)
{
	char *str;

	/* check if buffer ends with \0, if not, and
	   if there is space, add it */
	if (*(ldns_buffer_at(buffer, ldns_buffer_position(buffer))) != 0) {
		if (!ldns_buffer_reserve(buffer, 1)) {
			return NULL;
		}
		ldns_buffer_write_char(buffer, (uint8_t) '\0');
		if (!ldns_buffer_set_capacity(buffer, ldns_buffer_position(buffer))) {
			return NULL;
		}
	}

	str = _strdup((const char *)ldns_buffer_begin(buffer));
        if(!str) {
                return NULL;
        }
	return str;
}

/*
 * Zero terminate the buffer and export data.
 */
char *
ldns_buffer_export2str(ldns_buffer *buffer)
{
	/* Append '\0' as string terminator */
	if (! ldns_buffer_reserve(buffer, 1)) {
		return NULL;
	}
	ldns_buffer_write_char(buffer, 0);

	/* reallocate memory to the size of the string and export */
	ldns_buffer_set_capacity(buffer, ldns_buffer_position(buffer));
	return ldns_buffer_export(buffer);
}

char *
ldns_rdf2str(const ldns_rdf *rdf)
{
	char *result = NULL;
	ldns_buffer *tmp_buffer = ldns_buffer_new(LDNS_MAX_PACKETLEN);

	if (!tmp_buffer) {
		return NULL;
	}
	if (ldns_rdf2buffer_str(tmp_buffer, rdf) == LDNS_STATUS_OK) {
		/* export and return string, destroy rest */
		result = ldns_buffer_export2str(tmp_buffer);
	}
	ldns_buffer_free(tmp_buffer);
	return result;
}

char *
ldns_rr2str_fmt(const ldns_output_format *fmt, const ldns_rr *rr)
{
	char *result = NULL;
	ldns_buffer *tmp_buffer = ldns_buffer_new(LDNS_MAX_PACKETLEN);

	if (!tmp_buffer) {
		return NULL;
	}
	if (ldns_rr2buffer_str_fmt(tmp_buffer, fmt, rr)
		       	== LDNS_STATUS_OK) {
		/* export and return string, destroy rest */
		result = ldns_buffer_export2str(tmp_buffer);
	}
	ldns_buffer_free(tmp_buffer);
	return result;
}

char *
ldns_rr2str(const ldns_rr *rr)
{
	return ldns_rr2str_fmt(ldns_output_format_default, rr);
}

char *
ldns_pkt2str_fmt(const ldns_output_format *fmt, const ldns_pkt *pkt)
{
	char *result = NULL;
	ldns_buffer *tmp_buffer = ldns_buffer_new(LDNS_MAX_PACKETLEN);

	if (!tmp_buffer) {
		return NULL;
	}
	if (ldns_pkt2buffer_str_fmt(tmp_buffer, fmt, pkt)
		       	== LDNS_STATUS_OK) {
		/* export and return string, destroy rest */
		result = ldns_buffer_export2str(tmp_buffer);
	}

	ldns_buffer_free(tmp_buffer);
	return result;
}

char *
ldns_pkt2str(const ldns_pkt *pkt)
{
	return ldns_pkt2str_fmt(ldns_output_format_default, pkt);
}

char *
ldns_key2str(const ldns_key *k)
{
	char *result = NULL;
	ldns_buffer *tmp_buffer = ldns_buffer_new(LDNS_MAX_PACKETLEN);

	if (!tmp_buffer) {
		return NULL;
	}
	if (ldns_key2buffer_str(tmp_buffer, k) == LDNS_STATUS_OK) {
		/* export and return string, destroy rest */
		result = ldns_buffer_export2str(tmp_buffer);
	}
	ldns_buffer_free(tmp_buffer);
	return result;
}

char *
ldns_rr_list2str_fmt(const ldns_output_format *fmt, const ldns_rr_list *list)
{
	char *result = NULL;
	ldns_buffer *tmp_buffer = ldns_buffer_new(LDNS_MAX_PACKETLEN);

	if (!tmp_buffer) {
		return NULL;
	}
	if (list) {
		if (ldns_rr_list2buffer_str_fmt(
				   tmp_buffer, fmt, list)
			       	== LDNS_STATUS_OK) {
		}
	} else {
		if (fmt == NULL) {
			fmt = ldns_output_format_default;
		}
		if (fmt->flags & LDNS_COMMENT_NULLS) {
			ldns_buffer_printf(tmp_buffer, "; (null)\n");
		}
	}

	/* export and return string, destroy rest */
	result = ldns_buffer_export2str(tmp_buffer);
	ldns_buffer_free(tmp_buffer);
	return result;
}

char *
ldns_rr_list2str(const ldns_rr_list *list)
{
	return ldns_rr_list2str_fmt(ldns_output_format_default, list);
}

void
ldns_rdf_print(FILE *output, const ldns_rdf *rdf)
{
	char *str = ldns_rdf2str(rdf);
	if (str) {
		fprintf(output, "%s", str);
	} else {
		fprintf(output, ";Unable to convert rdf to string\n");
	}
	LDNS_FREE(str);
}

void
ldns_rr_print_fmt(FILE *output,
		const ldns_output_format *fmt, const ldns_rr *rr)
{
	char *str = ldns_rr2str_fmt(fmt, rr);
	if (str) {
		fprintf(output, "%s", str);
	} else {
		fprintf(output, ";Unable to convert rr to string\n");
	}
	LDNS_FREE(str);
}

void
ldns_rr_print(FILE *output, const ldns_rr *rr)
{
	ldns_rr_print_fmt(output, ldns_output_format_default, rr);
}

void
ldns_pkt_print_fmt(FILE *output, 
		const ldns_output_format *fmt, const ldns_pkt *pkt)
{
	char *str = ldns_pkt2str_fmt(fmt, pkt);
	if (str) {
		fprintf(output, "%s", str);
	} else {
		fprintf(output, ";Unable to convert packet to string\n");
	}
	LDNS_FREE(str);
}

void
ldns_pkt_print(FILE *output, const ldns_pkt *pkt)
{
	ldns_pkt_print_fmt(output, ldns_output_format_default, pkt);
}

void
ldns_rr_list_print_fmt(FILE *output, 
		const ldns_output_format *fmt, const ldns_rr_list *lst)
{
	size_t i;
	for (i = 0; i < ldns_rr_list_rr_count(lst); i++) {
		ldns_rr_print_fmt(output, fmt, ldns_rr_list_rr(lst, i));
	}
}

void
ldns_rr_list_print(FILE *output, const ldns_rr_list *lst)
{
	ldns_rr_list_print_fmt(output, ldns_output_format_default, lst);
}

void
ldns_resolver_print_fmt(FILE *output, 
		const ldns_output_format *fmt, const ldns_resolver *r)
{
	uint16_t i;
	ldns_rdf **n;
	ldns_rdf **s;
	size_t *rtt;
	if (!r) {
		return;
	}
	n = ldns_resolver_nameservers(r);
	s = ldns_resolver_searchlist(r);
	rtt = ldns_resolver_rtt(r);

	fprintf(output, "port: %d\n", (int)ldns_resolver_port(r));
	fprintf(output, "edns0 size: %d\n", (int)ldns_resolver_edns_udp_size(r));
	fprintf(output, "use ip6: %d\n", (int)ldns_resolver_ip6(r));

	fprintf(output, "recursive: %d\n", ldns_resolver_recursive(r));
	fprintf(output, "usevc: %d\n", ldns_resolver_usevc(r));
	fprintf(output, "igntc: %d\n", ldns_resolver_igntc(r));
	fprintf(output, "fail: %d\n", ldns_resolver_fail(r));
	fprintf(output, "retry: %d\n", (int)ldns_resolver_retry(r));
	fprintf(output, "retrans: %d\n", (int)ldns_resolver_retrans(r));
	fprintf(output, "fallback: %d\n", ldns_resolver_fallback(r));
	fprintf(output, "random: %d\n", ldns_resolver_random(r));
	fprintf(output, "timeout: %d\n", (int)ldns_resolver_timeout(r).tv_sec);
	fprintf(output, "dnssec: %d\n", ldns_resolver_dnssec(r));
	fprintf(output, "dnssec cd: %d\n", ldns_resolver_dnssec_cd(r));
	fprintf(output, "trust anchors (%d listed):\n",
		(int)ldns_rr_list_rr_count(ldns_resolver_dnssec_anchors(r)));
	ldns_rr_list_print_fmt(output, fmt, ldns_resolver_dnssec_anchors(r));
	fprintf(output, "tsig: %s %s\n",
                ldns_resolver_tsig_keyname(r)?ldns_resolver_tsig_keyname(r):"-",
                ldns_resolver_tsig_algorithm(r)?ldns_resolver_tsig_algorithm(r):"-");
	fprintf(output, "debug: %d\n", ldns_resolver_debug(r));

	fprintf(output, "default domain: ");
	ldns_rdf_print(output, ldns_resolver_domain(r));
	fprintf(output, "\n");
	fprintf(output, "apply default domain: %d\n", ldns_resolver_defnames(r));

	fprintf(output, "searchlist (%d listed):\n",  (int)ldns_resolver_searchlist_count(r));
	for (i = 0; i < ldns_resolver_searchlist_count(r); i++) {
		fprintf(output, "\t");
		ldns_rdf_print(output, s[i]);
		fprintf(output, "\n");
	}
	fprintf(output, "apply search list: %d\n", ldns_resolver_dnsrch(r));

	fprintf(output, "nameservers (%d listed):\n", (int)ldns_resolver_nameserver_count(r));
	for (i = 0; i < ldns_resolver_nameserver_count(r); i++) {
		fprintf(output, "\t");
		ldns_rdf_print(output, n[i]);

		switch ((int)rtt[i]) {
			case LDNS_RESOLV_RTT_MIN:
			fprintf(output, " - reachable\n");
			break;
			case LDNS_RESOLV_RTT_INF:
			fprintf(output, " - unreachable\n");
			break;
		}
	}
}

void
ldns_resolver_print(FILE *output, const ldns_resolver *r)
{
	ldns_resolver_print_fmt(output, ldns_output_format_default, r);
}

void
ldns_zone_print_fmt(FILE *output, 
		const ldns_output_format *fmt, const ldns_zone *z)
{
	if(ldns_zone_soa(z))
		ldns_rr_print_fmt(output, fmt, ldns_zone_soa(z));
	ldns_rr_list_print_fmt(output, fmt, ldns_zone_rrs(z));
}
void
ldns_zone_print(FILE *output, const ldns_zone *z)
{
	ldns_zone_print_fmt(output, ldns_output_format_default, z);
}
