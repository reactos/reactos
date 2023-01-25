/*
 * edns.h
 *
 *
 * a Net::DNS like library for C
 *
 * (c) NLnet Labs, 2004-2022
 *
 * See the file LICENSE for the license
 */

#ifndef LDNS_EDNS_H
#define LDNS_EDNS_H

#include <ldns/common.h>

#ifdef __cplusplus
extern "C" {
#endif



/**
 * EDNS option codes
 */
enum ldns_enum_edns_option
{
	LDNS_EDNS_LLQ = 1, /* RFC8764 */
	LDNS_EDNS_UL = 2, /* http://files.dns-sd.org/draft-sekar-dns-ul.txt */
	LDNS_EDNS_NSID = 3, /* RFC5001 */
	/* 4 draft-cheshire-edns0-owner-option */
	LDNS_EDNS_DAU = 5, /* RFC6975 */
	LDNS_EDNS_DHU = 6, /* RFC6975 */
	LDNS_EDNS_N3U = 7, /* RFC6975 */
	LDNS_EDNS_CLIENT_SUBNET = 8, /* RFC7871 */
	LDNS_EDNS_EXPIRE = 9, /* RFC7314 */
	LDNS_EDNS_COOKIE = 10, /* RFC7873 */
	LDNS_EDNS_KEEPALIVE = 11, /* RFC7828*/
	LDNS_EDNS_PADDING = 12, /* RFC7830 */
	LDNS_EDNS_CHAIN = 13, /* RFC7901 */
	LDNS_EDNS_KEY_TAG = 14, /* RFC8145 */
	LDNS_EDNS_EDE = 15, /* RFC8914 */
	LDNS_EDNS_CLIENT_TAG = 16, /* draft-bellis-dnsop-edns-tags-01 */
	LDNS_EDNS_SERVER_TAG = 17 /* draft-bellis-dnsop-edns-tags-01 */
};
typedef enum ldns_enum_edns_option ldns_edns_option_code;

/**
 * Extended DNS Error (RFC 8914) codes
 */
enum ldns_edns_enum_ede_code
{
	LDNS_EDE_OTHER = 0,
	LDNS_EDE_UNSUPPORTED_DNSKEY_ALG = 1,
	LDNS_EDE_UNSUPPORTED_DS_DIGEST = 2,
	LDNS_EDE_STALE_ANSWER = 3,
	LDNS_EDE_FORGED_ANSWER = 4,
	LDNS_EDE_DNSSEC_INDETERMINATE = 5,
	LDNS_EDE_DNSSEC_BOGUS = 6,
	LDNS_EDE_SIGNATURE_EXPIRED = 7,
	LDNS_EDE_SIGNATURE_NOT_YET_VALID = 8,
	LDNS_EDE_DNSKEY_MISSING = 9,
	LDNS_EDE_RRSIGS_MISSING = 10,
	LDNS_EDE_NO_ZONE_KEY_BIT_SET = 11,
	LDNS_EDE_NSEC_MISSING = 12,
	LDNS_EDE_CACHED_ERROR = 13,
	LDNS_EDE_NOT_READY = 14,
	LDNS_EDE_BLOCKED = 15,
	LDNS_EDE_CENSORED = 16,
	LDNS_EDE_FILTERED = 17,
	LDNS_EDE_PROHIBITED = 18,
	LDNS_EDE_STALE_NXDOMAIN_ANSWER = 19,
	LDNS_EDE_NOT_AUTHORITATIVE = 20,
	LDNS_EDE_NOT_SUPPORTED = 21,
	LDNS_EDE_NO_REACHABLE_AUTHORITY = 22,
	LDNS_EDE_NETWORK_ERROR = 23,
	LDNS_EDE_INVALID_DATA = 24,
	LDNS_EDE_SIGNATURE_EXPIRED_BEFORE_VALID = 25,
	LDNS_EDE_TOO_EARLY = 26
};
typedef enum ldns_edns_enum_ede_code ldns_edns_ede_code;

/**
 * The struct that stores an ordered EDNS option.
 * An EDNS option is structed as follows:
  +0 (MSB)                            +1 (LSB)
     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  0: |                          OPTION-CODE                          |
     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  2: |                         OPTION-LENGTH                         |
     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  4: |                                                               |
     /                          OPTION-DATA                          /
     /                                                               /
     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 */
struct ldns_struct_edns_option {
	ldns_edns_option_code _code;
	size_t                _size;
	void                 *_data;
};
typedef struct ldns_struct_edns_option ldns_edns_option;


/* 
 * Array structure to store multiple EDNS options
 */
struct ldns_struct_edns_option_list
{
	size_t _option_count; /* the number of EDNS options in the list */
   size_t _option_capacity; /* the amount of options that fit into the list */
	size_t _options_size; /* the total size of the options serialized */
	ldns_edns_option **_options;
};
typedef struct ldns_struct_edns_option_list ldns_edns_option_list;

/*
 * Access functions 
 * do this as functions to get type checking
 */

/**
 * returns the size of the EDNS data.
 * \param[in] *edns the EDNS struct to read from
 * \return uint16_t with the size
 */
size_t ldns_edns_get_size(const ldns_edns_option *edns);

/**
 * returns the option code of the EDNS data.
 * \param[in] *edns the EDNS struct to read from
 * \return uint16_t with the size
 */
ldns_edns_option_code ldns_edns_get_code(const ldns_edns_option *edns);

/**
 * returns the EDNS option data.
 * \param[in] *edns the EDNS option to read from
 * \return uint8_t* pointer to the EDNS option's data
 */
uint8_t *ldns_edns_get_data(const ldns_edns_option *edns);


/**
 * serialise the EDNS option into wireformat.
 * \param[in] *edns the EDNS option to read from
 * \return ldns_buffer* the buffer containing the data
 */
ldns_buffer *ldns_edns_get_wireformat_buffer(const ldns_edns_option *edns);

/* Constructors and destructors*/

/**
 * allocates a new EDNS structure and fills it. This function *DOES NOT* copy
 * the contents from the data parameter.
 * \param[in] code the EDNS code
 * \param[in] size size of the buffer
 * \param[in] data pointer to the buffer to be assigned
 * \return the new EDNS structure or NULL on failure
 */
ldns_edns_option *ldns_edns_new(ldns_edns_option_code code, size_t size, void *data);

/**
 * allocates a new EDNS structure and fills it. This function *DOES* copy
 * the contents from the data parameter.
 * \param[in] code the EDNS code
 * \param[in] size size of the buffer
 * \param[in] data pointer to the buffer to be assigned
 * \return the new EDNS structure or NULL on failure
 */
ldns_edns_option *ldns_edns_new_from_data(ldns_edns_option_code code, size_t size, const void *data);

/**
 * clone an EDNS option
 * \param[in] edns the EDNS option
 * \return the new EDNS structure
 */
ldns_edns_option *ldns_edns_clone(ldns_edns_option *edns);

/**
 * free the EDNS option. Use deep_free if the _data member is allocated.
 * \param[in] edns the EDNS option to free
 */
void ldns_edns_deep_free(ldns_edns_option *edns);
void ldns_edns_free(ldns_edns_option *edns);

/**
 * allocates space for a new list of EDNS options
 * \return the new EDNS option list or NULL on failure
 */
ldns_edns_option_list* ldns_edns_option_list_new(void);

/**
 * clone the EDNS options list and it's contents
 * \param[in] options_list  the EDNS options_list to read from
 * \return the new EDNS option list
 */
ldns_edns_option_list *ldns_edns_option_list_clone(ldns_edns_option_list *options_list);

/**
 * free the EDNS option list. Use deep_free to free the options options
 * in the list as well.
 * \param[in] options_list the EDNS option to free
 */
void ldns_edns_option_list_free(ldns_edns_option_list *options_list);
void ldns_edns_option_list_deep_free(ldns_edns_option_list *options_list);

/* edns_option_list functions */

/**
 * returns the number of options in the EDNS options list.
 * \param[in] options_list  the EDNS options_list to read from
 * \return the number of EDNS options
 */
size_t ldns_edns_option_list_get_count(const ldns_edns_option_list *options_list);

/**
 * returns the EDNS option as the specified index in the list of EDNS options.
 * \param[in] options_list  the EDNS options_list to read from
 * \param[in] index         the location of the EDNS option to get in the list
 * \return the EDNS option located at the index or NULL on failure
 */
ldns_edns_option* ldns_edns_option_list_get_option(const ldns_edns_option_list *options_list,
	size_t index);


/**
 * returns the total size of all the individual EDNS options in the EDNS list.
 * \param[in] options_list  the EDNS options_list to read from
 * \return the total size of the options combined
 */
size_t ldns_edns_option_list_get_options_size(const ldns_edns_option_list *options_list);

/**
 * adds an EDNS option to the list of options at the specified index. Also
 * returns the option that was previously at that index.
 * \param[in] options_list  the EDNS options_list to add to
 * \param[in] option        the EDNS option to add to the list
 * \param[in] index         the index in the list where to set the option
 * \return the EDNS option previously located at the index
 */
ldns_edns_option *ldns_edns_option_list_set_option(ldns_edns_option_list *options_list,
	ldns_edns_option *option, size_t index);

/**
 * adds an EDNS option at the end of the list of options.
 * \param[in] options_list  the EDNS options_list to add to
 * \param[in] option        the (non-NULL) EDNS option to add to the list
 * \return true on success and false of failure
 */
bool ldns_edns_option_list_push(ldns_edns_option_list *options_list,
	ldns_edns_option *option);

/**
 * removes and returns the EDNS option at the end of the list of options.
 * \param[in] options_list  the EDNS options_list to add to
 * \return the EDNS option at the end of the list, or NULL on failure
 */
ldns_edns_option* ldns_edns_option_list_pop(ldns_edns_option_list *options_list);

/**
 * serializes all the EDNS options into a single wireformat buffer
 * \param[in] option_list  the EDNS options_list to combine into one wireformat
 * \return the filled buffer or NULL on failure
 */
ldns_buffer *ldns_edns_option_list2wireformat_buffer(const ldns_edns_option_list *option_list);

#ifdef __cplusplus
}
#endif

#endif /* LDNS_EDNS_H */
