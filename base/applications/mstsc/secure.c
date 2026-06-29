/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - RDP encryption and licensing
   Copyright (C) Matthew Chapman <matthewc.unsw.edu.au> 1999-2008
   Copyright 2005-2011 Peter Astrand <astrand@cendio.se> for Cendio AB

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "precomp.h"

void *
rdssl_sha1_info_create(void);
void
rdssl_sha1_info_delete(void * sha1_info);
void
rdssl_sha1_clear(void * sha1_info);
void
rdssl_sha1_transform(void * sha1_info, char * data, int len);
void
rdssl_sha1_complete(void * sha1_info, char * data);
void *
rdssl_md5_info_create(void);
void
rdssl_md5_info_delete(void * md5_info);
void *
rdssl_md5_info_create(void);
void
rdssl_md5_info_delete(void * md5_info);
void
rdssl_md5_clear(void * md5_info);
void
rdssl_md5_transform(void * md5_info, char * data, int len);
void
rdssl_md5_complete(void * md5_info, char * data);
void *
rdssl_rc4_info_create(void);
void
rdssl_rc4_info_delete(void * rc4_info);
void
rdssl_rc4_set_key(void * rc4_info, char * key, int len);
void
rdssl_rc4_crypt(void * rc4_info, char * in_data, char * out_data, int len);
int
rdssl_mod_exp(char* out, int out_len, char* in, int in_len,
              char* mod, int mod_len, char* exp, int exp_len);
int
rdssl_sign_ok(char* e_data, int e_len, char* n_data, int n_len,
              char* sign_data, int sign_len, char* sign_data2, int sign_len2, char* testkey);
PCCERT_CONTEXT
rdssl_cert_read(uint8 * data, uint32 len);
void
rdssl_cert_free(PCCERT_CONTEXT context);
uint8 *
rdssl_cert_to_rkey(PCCERT_CONTEXT cert, uint32 * key_len);
RD_BOOL
rdssl_certs_ok(PCCERT_CONTEXT server_cert, PCCERT_CONTEXT cacert);
int
rdssl_rkey_get_exp_mod(uint8 * rkey, uint8 * exponent, uint32 max_exp_len, uint8 * modulus,
    uint32 max_mod_len);
void
rdssl_rkey_free(uint8 * rkey);

extern char g_hostname[16];
extern int g_width;
extern int g_height;
extern unsigned int g_keylayout;
extern int g_keyboard_type;
extern int g_keyboard_subtype;
extern int g_keyboard_functionkeys;
extern RD_BOOL g_encryption;
extern RD_BOOL g_licence_issued;
extern RD_BOOL g_licence_error_result;
extern RDP_VERSION g_rdp_version;
extern RD_BOOL g_console_session;
extern uint32 g_redirect_session_id;
extern int g_server_depth;
extern VCHANNEL g_channels[];
extern unsigned int g_num_channels;
extern uint8 g_client_random[SEC_RANDOM_SIZE];

static int g_rc4_key_len;
static void * g_rc4_decrypt_key;
static void * g_rc4_encrypt_key;
static uint32 g_server_public_key_len;

static uint8 g_sec_sign_key[16];
static uint8 g_sec_decrypt_key[16];
static uint8 g_sec_encrypt_key[16];
static uint8 g_sec_decrypt_update_key[16];
static uint8 g_sec_encrypt_update_key[16];
static uint8 g_sec_crypted_random[SEC_MAX_MODULUS_SIZE];

uint16 g_server_rdp_version = 0;

/* These values must be available to reset state - Session Directory */
static int g_sec_encrypt_use_count = 0;
static int g_sec_decrypt_use_count = 0;

#define SEC_MODULUS_SIZE 64

static uint8 g_testkey[176] =
{
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x5c, 0x00,
    0x52, 0x53, 0x41, 0x31, 0x48, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x79, 0x6f, 0xb4, 0xdf,
    0xa6, 0x95, 0xb9, 0xa9, 0x61, 0xe3, 0xc4, 0x5e,
    0xff, 0x6b, 0xd8, 0x81, 0x8a, 0x12, 0x4a, 0x93,
    0x42, 0x97, 0x18, 0x93, 0xac, 0xd1, 0x3a, 0x38,
    0x3c, 0x68, 0x50, 0x19, 0x31, 0xb6, 0x84, 0x51,
    0x79, 0xfb, 0x1c, 0xe7, 0xe3, 0x99, 0x20, 0xc7,
    0x84, 0xdf, 0xd1, 0xaa, 0xb5, 0x15, 0xef, 0x47,
    0x7e, 0xfc, 0x88, 0xeb, 0x29, 0xc3, 0x27, 0x5a,
    0x35, 0xf8, 0xfd, 0xaa, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
                            0x08, 0x00, 0x48, 0x00,
    0x32, 0x3b, 0xde, 0x6f, 0x18, 0x97, 0x1e, 0xc3,
    0x6b, 0x2b, 0x2d, 0xe4, 0xfc, 0x2d, 0xa2, 0x8e,
    0x32, 0x3c, 0xf3, 0x1b, 0x24, 0x90, 0x57, 0x4d,
    0x8e, 0xe4, 0x69, 0xfc, 0x16, 0x8d, 0x41, 0x92,
    0x78, 0xc7, 0x9c, 0xb4, 0x26, 0xff, 0xe8, 0x3e,
    0xa1, 0x8a, 0xf5, 0x57, 0xc0, 0x7f, 0x3e, 0x21,
    0x17, 0x32, 0x30, 0x6f, 0x79, 0xe1, 0x36, 0xcd,
    0xb6, 0x8e, 0xbe, 0x57, 0x57, 0xd2, 0xa9, 0x36
};

/*
 * I believe this is based on SSLv3 with the following differences:
 *  MAC algorithm (5.2.3.1) uses only 32-bit length in place of seq_num/type/length fields
 *  MAC algorithm uses SHA1 and MD5 for the two hash functions instead of one or other
 *  key_block algorithm (6.2.2) uses 'X', 'YY', 'ZZZ' instead of 'A', 'BB', 'CCC'
 *  key_block partitioning is different (16 bytes each: MAC secret, decrypt key, encrypt key)
 *  encryption/decryption keys updated every 4096 packets
 * See http://wp.netscape.com/eng/ssl3/draft302.txt
 */

/*
 * 48-byte transformation used to generate master secret (6.1) and key material (6.2.2).
 * Both SHA1 and MD5 algorithms are used.
 */
void
sec_hash_48(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2, uint8 salt)
{
	uint8 shasig[20];
	uint8 pad[4];
	void * sha;
	void * md5;
	int i;

	for (i = 0; i < 3; i++)
	{
		memset(pad, salt + i, i + 1);
		sha = rdssl_sha1_info_create();
		rdssl_sha1_clear(sha);
		rdssl_sha1_transform(sha, (char *)pad, i + 1);
		rdssl_sha1_transform(sha, (char *)in, 48);
		rdssl_sha1_transform(sha, (char *)salt1, 32);
		rdssl_sha1_transform(sha, (char *)salt2, 32);
		rdssl_sha1_complete(sha, (char *)shasig);
		rdssl_sha1_info_delete(sha);
		md5 = rdssl_md5_info_create();
		rdssl_md5_clear(md5);
        rdssl_md5_transform(md5, (char *)in, 48);
        rdssl_md5_transform(md5, (char *)shasig, 20);
		rdssl_md5_complete(md5, (char *)out + i * 16);
		rdssl_md5_info_delete(md5);
	}
}

/*
 * 16-byte transformation used to generate export keys (6.2.2).
 */
void
sec_hash_16(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2)
{
	void * md5;
	
	md5 = rdssl_md5_info_create();
	rdssl_md5_clear(md5);
	rdssl_md5_transform(md5, (char *)in, 16);
	rdssl_md5_transform(md5, (char *)salt1, 32);
	rdssl_md5_transform(md5, (char *)salt2, 32);
    rdssl_md5_complete(md5, (char *)out);
	rdssl_md5_info_delete(md5);
}

/*
 * 16-byte sha1 hash
 */
void
sec_hash_sha1_16(uint8 * out, uint8 * in, uint8 * salt1)
{
	void * sha;
	sha = rdssl_sha1_info_create();
	rdssl_sha1_clear(sha);
	rdssl_sha1_transform(&sha, (char *)in, 16);
	rdssl_sha1_transform(&sha, (char *)salt1, 16);
	rdssl_sha1_complete(&sha, (char *)out);
	rdssl_sha1_info_delete(sha);
}

/* create string from hash */
void
sec_hash_to_string(char *out, int out_size, uint8 * in, int in_size)
{
	int k;
	memset(out, 0, out_size);
	for (k = 0; k < in_size; k++, out += 2)
	{
		sprintf(out, "%.2x", in[k]);
	}
}

/* Reduce key entropy from 64 to 40 bits */
static void
sec_make_40bit(uint8 * key)
{
	key[0] = 0xd1;
	key[1] = 0x26;
	key[2] = 0x9e;
}

/* Generate encryption keys given client and server randoms */
static void
sec_generate_keys(uint8 * client_random, uint8 * server_random, int rc4_key_size)
{
	uint8 pre_master_secret[48];
	uint8 master_secret[48];
	uint8 key_block[48];

	/* Construct pre-master secret */
	memcpy(pre_master_secret, client_random, 24);
	memcpy(pre_master_secret + 24, server_random, 24);

	/* Generate master secret and then key material */
	sec_hash_48(master_secret, pre_master_secret, client_random, server_random, 'A');
	sec_hash_48(key_block, master_secret, client_random, server_random, 'X');

	/* First 16 bytes of key material is MAC secret */
	memcpy(g_sec_sign_key, key_block, 16);

	/* Generate export keys from next two blocks of 16 bytes */
	sec_hash_16(g_sec_decrypt_key, &key_block[16], client_random, server_random);
	sec_hash_16(g_sec_encrypt_key, &key_block[32], client_random, server_random);

	if (rc4_key_size == 1)
	{
		DEBUG(("40-bit encryption enabled\n"));
		sec_make_40bit(g_sec_sign_key);
		sec_make_40bit(g_sec_decrypt_key);
		sec_make_40bit(g_sec_encrypt_key);
		g_rc4_key_len = 8;
	}
	else
	{
		DEBUG(("rc_4_key_size == %d, 128-bit encryption enabled\n", rc4_key_size));
		g_rc4_key_len = 16;
	}

	/* Save initial RC4 keys as update keys */
	memcpy(g_sec_decrypt_update_key, g_sec_decrypt_key, 16);
	memcpy(g_sec_encrypt_update_key, g_sec_encrypt_key, 16);

	/* Initialise RC4 state arrays */

    rdssl_rc4_info_delete(g_rc4_decrypt_key);
	g_rc4_decrypt_key = rdssl_rc4_info_create(); 
	rdssl_rc4_set_key(g_rc4_decrypt_key, (char *)g_sec_decrypt_key, g_rc4_key_len); 

    rdssl_rc4_info_delete(g_rc4_encrypt_key);
	g_rc4_encrypt_key = rdssl_rc4_info_create(); 
	rdssl_rc4_set_key(g_rc4_encrypt_key, (char *)g_sec_encrypt_key, g_rc4_key_len); 
}

static uint8 pad_54[40] = {
	54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
	54, 54, 54,
	54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
	54, 54, 54
};

static uint8 pad_92[48] = {
	92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
	92, 92, 92, 92, 92, 92, 92,
	92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
	92, 92, 92, 92, 92, 92, 92
};

/* Output a uint32 into a buffer (little-endian) */
void
buf_out_uint32(uint8 * buffer, uint32 value)
{
	buffer[0] = (value) & 0xff;
	buffer[1] = (value >> 8) & 0xff;
	buffer[2] = (value >> 16) & 0xff;
	buffer[3] = (value >> 24) & 0xff;
}

/* Generate a MAC hash (5.2.3.1), using a combination of SHA1 and MD5 */
void
sec_sign(uint8 * signature, int siglen, uint8 * session_key, int keylen, uint8 * data, int datalen)
{
	uint8 shasig[20];
	uint8 md5sig[16];
	uint8 lenhdr[4];
	void * sha;
	void * md5;

	buf_out_uint32(lenhdr, datalen);

	sha = rdssl_sha1_info_create();
	rdssl_sha1_clear(sha);
    rdssl_sha1_transform(sha, (char *)session_key, keylen);
	rdssl_sha1_transform(sha, (char *)pad_54, 40);
	rdssl_sha1_transform(sha, (char *)lenhdr, 4);
	rdssl_sha1_transform(sha, (char *)data, datalen);
	rdssl_sha1_complete(sha, (char *)shasig);
	rdssl_sha1_info_delete(sha);

	md5 = rdssl_md5_info_create();
	rdssl_md5_clear(md5);
    rdssl_md5_transform(md5, (char *)session_key, keylen);
	rdssl_md5_transform(md5, (char *)pad_92, 48);
	rdssl_md5_transform(md5, (char *)shasig, 20);
	rdssl_md5_complete(md5, (char *)md5sig);
	rdssl_md5_info_delete(md5);	

	memcpy(signature, md5sig, siglen);
}

/* Update an encryption key */
static void
sec_update(uint8 * key, uint8 * update_key)
{
	uint8 shasig[20];
	void * sha;
	void * md5;
	void * update;

	sha = rdssl_sha1_info_create();
	rdssl_sha1_clear(sha);
	rdssl_sha1_transform(sha, (char *)update_key, g_rc4_key_len);
	rdssl_sha1_transform(sha, (char *)pad_54, 40);
	rdssl_sha1_transform(sha, (char *)key, g_rc4_key_len);
	rdssl_sha1_complete(sha, (char *)shasig);
	rdssl_sha1_info_delete(sha);

	md5 = rdssl_md5_info_create();
	rdssl_md5_clear(md5);
    rdssl_md5_transform(md5, (char *)update_key, g_rc4_key_len);
	rdssl_md5_transform(md5, (char *)pad_92, 48);
	rdssl_md5_transform(md5, (char *)shasig, 20);
	rdssl_md5_complete(md5, (char *)key);
	rdssl_md5_info_delete(md5);


	update = rdssl_rc4_info_create();
	rdssl_rc4_set_key(update, (char *)key, g_rc4_key_len);
	rdssl_rc4_crypt(update, (char *)key, (char *)key, g_rc4_key_len);
	rdssl_rc4_info_delete(update);

	if (g_rc4_key_len == 8)
		sec_make_40bit(key);
}

/* Encrypt data using RC4 */
static void
sec_encrypt(uint8 * data, int length)
{
	if (g_sec_encrypt_use_count == 4096)
	{
		sec_update(g_sec_encrypt_key, g_sec_encrypt_update_key);
		rdssl_rc4_set_key(g_rc4_encrypt_key, (char *)g_sec_encrypt_key, g_rc4_key_len);
		g_sec_encrypt_use_count = 0;
	}

	rdssl_rc4_crypt(g_rc4_encrypt_key, (char *)data, (char *)data, length);
	g_sec_encrypt_use_count++;
}

/* Decrypt data using RC4 */
void
sec_decrypt(uint8 * data, int length)
{
	if (g_sec_decrypt_use_count == 4096)
	{
		sec_update(g_sec_decrypt_key, g_sec_decrypt_update_key);
		rdssl_rc4_set_key(g_rc4_decrypt_key, (char *)g_sec_decrypt_key, g_rc4_key_len);
		g_sec_decrypt_use_count = 0;
	}

	rdssl_rc4_crypt(g_rc4_decrypt_key,(char *)data, (char *)data, length);
	g_sec_decrypt_use_count++;
}

/* Perform an RSA public key encryption operation */
static void
sec_rsa_encrypt(uint8 * out, uint8 * in, int len, uint32 modulus_size, uint8 * modulus,
		uint8 * exponent)
{
	rdssl_mod_exp((char *)out, 64, (char *)in, 32, (char *)modulus, 64, (char *)exponent, 4);
}

/* Initialise secure transport packet */
STREAM
sec_init(uint32 flags, int maxlen)
{
	int hdrlen;
	STREAM s;

	if (!g_licence_issued && !g_licence_error_result)
		hdrlen = (flags & SEC_ENCRYPT) ? 12 : 4;
	else
		hdrlen = (flags & SEC_ENCRYPT) ? 12 : 0;
	s = mcs_init(maxlen + hdrlen);
	s_push_layer(s, sec_hdr, hdrlen);

	return s;
}

/* Transmit secure transport packet over specified channel */
void
sec_send_to_channel(STREAM s, uint32 flags, uint16 channel)
{
	int datalen;

#ifdef WITH_SCARD
	scard_lock(SCARD_LOCK_SEC);
#endif

	s_pop_layer(s, sec_hdr);
	if ((!g_licence_issued && !g_licence_error_result) || (flags & SEC_ENCRYPT))
		out_uint32_le(s, flags);

	if (flags & SEC_ENCRYPT)
	{
		flags &= ~SEC_ENCRYPT;
		datalen = s->end - s->p - 8;

#ifdef WITH_DEBUG
		DEBUG(("Sending encrypted packet:\n"));
		hexdump(s->p + 8, datalen);
#endif

		sec_sign(s->p, 8, g_sec_sign_key, g_rc4_key_len, s->p + 8, datalen);
		sec_encrypt(s->p + 8, datalen);
	}

	mcs_send_to_channel(s, channel);

#ifdef WITH_SCARD
	scard_unlock(SCARD_LOCK_SEC);
#endif
}

/* Transmit secure transport packet */

void
sec_send(STREAM s, uint32 flags)
{
	sec_send_to_channel(s, flags, MCS_GLOBAL_CHANNEL);
}


/* Transfer the client random to the server */
static void
sec_establish_key(void)
{
	uint32 length = g_server_public_key_len + SEC_PADDING_SIZE;
	uint32 flags = SEC_EXCHANGE_PKT;
	STREAM s;

	s = sec_init(flags, length + 4);

	out_uint32_le(s, length);
	out_uint8p(s, g_sec_crypted_random, g_server_public_key_len);
	out_uint8s(s, SEC_PADDING_SIZE);

	s_mark_end(s);
	sec_send(s, flags);
}

/* Output connect initial data blob */
static void
sec_out_mcs_data(STREAM s, uint32 selected_protocol)
{
	int hostlen = 2 * strlen(g_hostname);
	int length = 162 + 76 + 12 + 4;
	unsigned int i;
	uint32 cluster_flags = 0;

	if (g_num_channels > 0)
		length += g_num_channels * 12 + 8;

	if (hostlen > 30)
		hostlen = 30;

	/* Generic Conference Control (T.124) ConferenceCreateRequest */
	out_uint16_be(s, 5);
	out_uint16_be(s, 0x14);
	out_uint8(s, 0x7c);
	out_uint16_be(s, 1);

	out_uint16_be(s, (length | 0x8000));	/* remaining length */

	out_uint16_be(s, 8);	/* length? */
	out_uint16_be(s, 16);
	out_uint8(s, 0);
	out_uint16_le(s, 0xc001);
	out_uint8(s, 0);

	out_uint32_le(s, 0x61637544);	/* OEM ID: "Duca", as in Ducati. */
	out_uint16_be(s, ((length - 14) | 0x8000));	/* remaining length */

	/* Client information */
	out_uint16_le(s, SEC_TAG_CLI_INFO);
	out_uint16_le(s, 216);	/* length */
	out_uint16_le(s, (g_rdp_version >= RDP_V5) ? 4 : 1);	/* RDP version. 1 == RDP4, 4 >= RDP5 to RDP8 */
	out_uint16_le(s, 8);
	out_uint16_le(s, g_width);
	out_uint16_le(s, g_height);
	out_uint16_le(s, 0xca01);
	out_uint16_le(s, 0xaa03);
	out_uint32_le(s, g_keylayout);
	out_uint32_le(s, 2600);	/* Client build. We are now 2600 compatible :-) */

	/* Unicode name of client, padded to 32 bytes */
	rdp_out_unistr(s, g_hostname, hostlen);
	out_uint8s(s, 30 - hostlen);

	/* See
	   http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wceddk40/html/cxtsksupportingremotedesktopprotocol.asp (DEAD_LINK) */
	out_uint32_le(s, g_keyboard_type);
	out_uint32_le(s, g_keyboard_subtype);
	out_uint32_le(s, g_keyboard_functionkeys);
	out_uint8s(s, 64);	/* reserved? 4 + 12 doublewords */
	out_uint16_le(s, 0xca01);	/* colour depth? */
	out_uint16_le(s, 1);

	out_uint32(s, 0);
	out_uint8(s, g_server_depth);
	out_uint16_le(s, 0x0700);
	out_uint8(s, 0);
	out_uint32_le(s, 1);
	out_uint8s(s, 64);
	out_uint32_le(s, selected_protocol);	/* End of client info */

	/* Write a Client Cluster Data (TS_UD_CS_CLUSTER) */
	out_uint16_le(s, SEC_TAG_CLI_CLUSTER);	/* header.type */
	out_uint16_le(s, 12);	/* length */

	cluster_flags |= SEC_CC_REDIRECTION_SUPPORTED;
	cluster_flags |= (SEC_CC_REDIRECT_VERSION_3 << 2);

	if (g_console_session || g_redirect_session_id != 0)
		cluster_flags |= SEC_CC_REDIRECT_SESSIONID_FIELD_VALID;

	out_uint32_le(s, cluster_flags);
	out_uint32(s, g_redirect_session_id);

	/* Client encryption settings */
	out_uint16_le(s, SEC_TAG_CLI_CRYPT);
	out_uint16_le(s, 12);	/* length */
	out_uint32_le(s, g_encryption ? 0x3 : 0);	/* encryption supported, 128-bit supported */
	out_uint32(s, 0);	/* Unknown */

	DEBUG_RDP5(("g_num_channels is %d\n", g_num_channels));
	if (g_num_channels > 0)
	{
		out_uint16_le(s, SEC_TAG_CLI_CHANNELS);
		out_uint16_le(s, g_num_channels * 12 + 8);	/* length */
		out_uint32_le(s, g_num_channels);	/* number of virtual channels */
		for (i = 0; i < g_num_channels; i++)
		{
			DEBUG_RDP5(("Requesting channel %s\n", g_channels[i].name));
			out_uint8a(s, g_channels[i].name, 8);
			out_uint32_be(s, g_channels[i].flags);
		}
	}

	s_mark_end(s);
}

/* Parse a public key structure */
static RD_BOOL
sec_parse_public_key(STREAM s, uint8 * modulus, uint8 * exponent)
{
	uint32 magic, modulus_len;

	in_uint32_le(s, magic);
	if (magic != SEC_RSA_MAGIC)
	{
		error("RSA magic 0x%x\n", magic);
		return False;
	}

	in_uint32_le(s, modulus_len);
	modulus_len -= SEC_PADDING_SIZE;
	if ((modulus_len < SEC_MODULUS_SIZE) || (modulus_len > SEC_MAX_MODULUS_SIZE))
	{
		error("Bad server public key size (%u bits)\n", modulus_len * 8);
		return False;
	}

	in_uint8s(s, 8);	/* modulus_bits, unknown */
	in_uint8a(s, exponent, SEC_EXPONENT_SIZE);
	in_uint8a(s, modulus, modulus_len);
	in_uint8s(s, SEC_PADDING_SIZE);
	g_server_public_key_len = modulus_len;

	return s_check(s);
}

/* Parse a public signature structure */
static RD_BOOL
sec_parse_public_sig(STREAM s, uint32 len, uint8 * modulus, uint8 * exponent)
{
	uint8 signature[SEC_MAX_MODULUS_SIZE];
	uint8 signature_[SEC_MAX_MODULUS_SIZE];
	uint32 sig_len;

	if (len != 72)
	{
		return True;
	}
	memset(signature, 0, sizeof(signature));
	sig_len = len - 8;
	in_uint8a(s, signature, sig_len);
    if(rdssl_sign_ok((char *)exponent, SEC_EXPONENT_SIZE, (char *)modulus, g_server_public_key_len,
                     (char *)signature_, SEC_MODULUS_SIZE, (char *)signature, sig_len, (char *)g_testkey))
    {
        DEBUG_RDP5(("key signature doesn't match test key\n"));
    }
	return s_check(s);
}

/* Parse a crypto information structure */
static RD_BOOL
sec_parse_crypt_info(STREAM s, uint32 * rc4_key_size,
		     uint8 ** server_random, uint8 * modulus, uint8 * exponent)
{
	uint32 crypt_level, random_len, rsa_info_len;
	uint32 cacert_len, cert_len, flags;
    PCCERT_CONTEXT cacert, server_cert;
	BYTE *server_public_key;
	uint16 tag, length;
	uint8 *next_tag, *end;

	in_uint32_le(s, *rc4_key_size);	/* 1 = 40-bit, 2 = 128-bit */
	in_uint32_le(s, crypt_level);	/* 1 = low, 2 = medium, 3 = high */
	if (crypt_level == 0)
	{
		/* no encryption */
		return False;
	}

	in_uint32_le(s, random_len);
	in_uint32_le(s, rsa_info_len);

	if (random_len != SEC_RANDOM_SIZE)
	{
		error("random len %d, expected %d\n", random_len, SEC_RANDOM_SIZE);
		return False;
	}

	in_uint8p(s, *server_random, random_len);

	/* RSA info */
	end = s->p + rsa_info_len;
	if (end > s->end)
		return False;

	in_uint32_le(s, flags);	/* 1 = RDP4-style, 0x80000002 = X.509 */
	if (flags & 1)
	{
		DEBUG_RDP5(("We're going for the RDP4-style encryption\n"));
		in_uint8s(s, 8);	/* unknown */

		while (s->p < end)
		{
			in_uint16_le(s, tag);
			in_uint16_le(s, length);

			next_tag = s->p + length;

			switch (tag)
			{
				case SEC_TAG_PUBKEY:
					if (!sec_parse_public_key(s, modulus, exponent))
						return False;
					DEBUG_RDP5(("Got Public key, RDP4-style\n"));

					break;

				case SEC_TAG_KEYSIG:
					if (!sec_parse_public_sig(s, length, modulus, exponent))
						return False;
					break;

				default:
					unimpl("crypt tag 0x%x\n", tag);
			}

			s->p = next_tag;
		}
	}
	else
	{
		uint32 certcount;

		DEBUG_RDP5(("We're going for the RDP5-style encryption\n"));
		in_uint32_le(s, certcount);	/* Number of certificates */
		if (certcount < 2)
		{
			error("Server didn't send enough X509 certificates\n");
			return False;
		}
		for (; certcount > 2; certcount--)
		{		/* ignore all the certificates between the root and the signing CA */
			uint32 ignorelen;
            PCCERT_CONTEXT ignorecert;

			DEBUG_RDP5(("Ignored certs left: %d\n", certcount));
			in_uint32_le(s, ignorelen);
			DEBUG_RDP5(("Ignored Certificate length is %d\n", ignorelen));
			ignorecert = rdssl_cert_read(s->p, ignorelen);
			in_uint8s(s, ignorelen);
			if (ignorecert == NULL)
			{	/* XXX: error out? */
				DEBUG_RDP5(("got a bad cert: this will probably screw up the rest of the communication\n"));
			}

#ifdef WITH_DEBUG_RDP5
			DEBUG_RDP5(("cert #%d (ignored):\n", certcount));
			rdssl_cert_print_fp(stdout, ignorecert);
#endif
		}
		/* Do da funky X.509 stuffy

		   "How did I find out about this?  I looked up and saw a
		   bright light and when I came to I had a scar on my forehead
		   and knew about X.500"
		   - Peter Gutman in a early version of 
		   http://www.cs.auckland.ac.nz/~pgut001/pubs/x509guide.txt
		 */
		in_uint32_le(s, cacert_len);
		DEBUG_RDP5(("CA Certificate length is %d\n", cacert_len));
		cacert = rdssl_cert_read(s->p, cacert_len);
		in_uint8s(s, cacert_len);
		if (NULL == cacert)
		{
			error("Couldn't load CA Certificate from server\n");
			return False;
		}
		in_uint32_le(s, cert_len);
		DEBUG_RDP5(("Certificate length is %d\n", cert_len));
		server_cert = rdssl_cert_read(s->p, cert_len);
		in_uint8s(s, cert_len);
		if (NULL == server_cert)
		{
			rdssl_cert_free(cacert);
			error("Couldn't load Certificate from server\n");
			return False;
		}
		if (!rdssl_certs_ok(server_cert, cacert))
		{
			rdssl_cert_free(server_cert);
			rdssl_cert_free(cacert);
			error("Security error CA Certificate invalid\n");
			return False;
		}
		rdssl_cert_free(cacert);
		in_uint8s(s, 16);	/* Padding */
		server_public_key = rdssl_cert_to_rkey(server_cert, &g_server_public_key_len);
		if (NULL == server_public_key)
		{
			DEBUG_RDP5(("Didn't parse X509 correctly\n"));
			rdssl_cert_free(server_cert);
			return False;
		}
		rdssl_cert_free(server_cert);
		if ((g_server_public_key_len < SEC_MODULUS_SIZE) ||
		    (g_server_public_key_len > SEC_MAX_MODULUS_SIZE))
		{
			error("Bad server public key size (%u bits)\n",
			      g_server_public_key_len * 8);
			rdssl_rkey_free(server_public_key);
			return False;
		}
		if (rdssl_rkey_get_exp_mod(server_public_key, exponent, SEC_EXPONENT_SIZE,
					   modulus, SEC_MAX_MODULUS_SIZE) != 0)
		{
			error("Problem extracting RSA exponent, modulus");
			rdssl_rkey_free(server_public_key);
			return False;
		}
		rdssl_rkey_free(server_public_key);
		return True;	/* There's some garbage here we don't care about */
	}
	return s_check_end(s);
}

/* Process crypto information blob */
static void
sec_process_crypt_info(STREAM s)
{
	uint8 *server_random = NULL;
	uint8 modulus[SEC_MAX_MODULUS_SIZE];
	uint8 exponent[SEC_EXPONENT_SIZE];
	uint32 rc4_key_size;

	memset(modulus, 0, sizeof(modulus));
	memset(exponent, 0, sizeof(exponent));
	if (!sec_parse_crypt_info(s, &rc4_key_size, &server_random, modulus, exponent))
	{
		DEBUG(("Failed to parse crypt info\n"));
		return;
	}
	DEBUG(("Generating client random\n"));
	generate_random(g_client_random);
	sec_rsa_encrypt(g_sec_crypted_random, g_client_random, SEC_RANDOM_SIZE,
			g_server_public_key_len, modulus, exponent);
	sec_generate_keys(g_client_random, server_random, rc4_key_size);
}


/* Process SRV_INFO, find RDP version supported by server */
static void
sec_process_srv_info(STREAM s)
{
	in_uint16_le(s, g_server_rdp_version);
	DEBUG_RDP5(("Server RDP version is %d\n", g_server_rdp_version));
	if (1 == g_server_rdp_version)
	{
		g_rdp_version = RDP_V4;
		g_server_depth = 8;
	}
}


/* Process connect response data blob */
void
sec_process_mcs_data(STREAM s)
{
	uint16 tag, length;
	uint8 *next_tag;
	uint8 len;

	in_uint8s(s, 21);	/* header (T.124 ConferenceCreateResponse) */
	in_uint8(s, len);
	if (len & 0x80)
		in_uint8(s, len);

	while (s->p < s->end)
	{
		in_uint16_le(s, tag);
		in_uint16_le(s, length);

		if (length <= 4)
			return;

		next_tag = s->p + length - 4;

		switch (tag)
		{
			case SEC_TAG_SRV_INFO:
				sec_process_srv_info(s);
				break;

			case SEC_TAG_SRV_CRYPT:
				sec_process_crypt_info(s);
				break;

			case SEC_TAG_SRV_CHANNELS:
				/* FIXME: We should parse this information and
				   use it to map RDP5 channels to MCS 
				   channels */
				break;

			default:
				unimpl("response tag 0x%x\n", tag);
		}

		s->p = next_tag;
	}
}

/* Receive secure transport packet */
STREAM
sec_recv(uint8 * rdpver)
{
	uint16 sec_flags;
	/* uint16 sec_flags_hi; */
	uint16 channel;
	STREAM s;

	while ((s = mcs_recv(&channel, rdpver)) != NULL)
	{
		if (rdpver != NULL)
		{
			if (*rdpver != 3)
			{
				if (*rdpver & 0x80)
				{
					in_uint8s(s, 8);	/* signature */
					sec_decrypt(s->p, s->end - s->p);
				}
				return s;
			}
		}
		if (g_encryption || (!g_licence_issued && !g_licence_error_result))
		{
            /* TS_SECURITY_HEADER */
            in_uint16_le(s, sec_flags);
            in_uint8s(s, 2); /* sec_flags_hi */

			if (g_encryption)
			{
				if (sec_flags & SEC_ENCRYPT)
				{
					in_uint8s(s, 8);	/* signature */
					sec_decrypt(s->p, s->end - s->p);
				}

				if (sec_flags & SEC_LICENSE_PKT)
				{
					licence_process(s);
					continue;
				}

				if (sec_flags & SEC_REDIRECTION_PKT)	/* SEC_REDIRECT_ENCRYPT */
				{
					uint8 swapbyte;

					in_uint8s(s, 8);	/* signature */
					sec_decrypt(s->p, s->end - s->p);

					/* Check for a redirect packet, starts with 00 04 */
					if (s->p[0] == 0 && s->p[1] == 4)
					{
						/* for some reason the PDU and the length seem to be swapped.
						   This isn't good, but we're going to do a byte for byte
						   swap.  So the first four values appear as: 00 04 XX YY,
						   where XX YY is the little endian length. We're going to
						   use 04 00 as the PDU type, so after our swap this will look
						   like: XX YY 04 00 */
						swapbyte = s->p[0];
						s->p[0] = s->p[2];
						s->p[2] = swapbyte;

						swapbyte = s->p[1];
						s->p[1] = s->p[3];
						s->p[3] = swapbyte;

						swapbyte = s->p[2];
						s->p[2] = s->p[3];
						s->p[3] = swapbyte;
					}
#ifdef WITH_DEBUG
					/* warning!  this debug statement will show passwords in the clear! */
					hexdump(s->p, s->end - s->p);
#endif
				}
			}
			else
			{
                if (sec_flags & SEC_LICENSE_PKT)
                {
					licence_process(s);
					continue;
				}
				s->p -= 4;
			}
		}

		if (channel != MCS_GLOBAL_CHANNEL)
		{
			channel_process(s, channel);
			if (rdpver != NULL)
				*rdpver = 0xff;
			return s;
		}

		return s;
	}

	return NULL;
}

/* Establish a secure connection */
RD_BOOL
sec_connect(char *server, char *username, char *domain, char *password, RD_BOOL reconnect)
{
	uint32 selected_proto;
	struct stream mcs_data;

	/* Start a MCS connect sequence */
	if (!mcs_connect_start(server, username, domain, password, reconnect, &selected_proto))
		return False;

	/* We exchange some RDP data during the MCS-Connect */
	mcs_data.size = 512;
	mcs_data.p = mcs_data.data = (uint8 *) xmalloc(mcs_data.size);
	sec_out_mcs_data(&mcs_data, selected_proto);

	/* finalize the MCS connect sequence */
	if (!mcs_connect_finalize(&mcs_data))
		return False;

	/* sec_process_mcs_data(&mcs_data); */
	if (g_encryption)
		sec_establish_key();
	xfree(mcs_data.data);
	return True;
}

/* Disconnect a connection */
void
sec_disconnect(void)
{
	mcs_disconnect();
}

/* reset the state of the sec layer */
void
sec_reset_state(void)
{
	g_server_rdp_version = 0;
	g_sec_encrypt_use_count = 0;
	g_sec_decrypt_use_count = 0;
	g_licence_issued = 0;
	g_licence_error_result = 0;
	mcs_reset_state();
}
