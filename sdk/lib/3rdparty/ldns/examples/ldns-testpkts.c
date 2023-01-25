/*
 * ldns-testpkts. Data file parse for test packets, and query matching.
 *
 * Data storage for specially crafted replies for testing purposes.
 *
 * (c) NLnet Labs, 2005, 2006, 2007, 2008
 * See the file LICENSE for the license
 */

/**
 * \file
 * This is a debugging aid. It is not efficient, especially
 * with a long config file, but it can give any reply to any query.
 * This can help the developer pre-script replies for queries.
 *
 * You can specify a packet RR by RR with header flags to return.
 *
 * Missing features:
 *		- matching content different from reply content.
 *		- find way to adjust mangled packets?
 */

#include "config.h"
struct sockaddr_storage;
#include <ldns/ldns.h>
#include <errno.h>
#include "ldns-testpkts.h"

/** max line length */
#define MAX_LINE   10240	
/** string to show in warnings and errors */
static const char* prog_name = "ldns-testpkts";

/** logging routine, provided by caller */
void verbose(int lvl, const char* msg, ...) ATTR_FORMAT(printf, 2, 3);

/** print error and exit */
static void error(const char* msg, ...) __attribute__((noreturn));
static void error(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "%s error: ", prog_name);
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	fflush(stderr);
	va_end(args);
	exit(EXIT_FAILURE);
}

/** return if string is empty or comment */
static bool isendline(char c)
{
	if(c == ';' || c == '#' 
		|| c == '\n' || c == 0)
		return true;
	return false;
}

/** true if the string starts with the keyword given. Moves the str ahead. 
 * @param str: before keyword, afterwards after keyword and spaces.
 * @param keyword: the keyword to match
 * @return: true if keyword present. False otherwise, and str unchanged.
*/
static bool str_keyword(char** str, const char* keyword)
{
	size_t len = strlen(keyword);
	assert(str && keyword);
	if(strncmp(*str, keyword, len) != 0)
		return false;
	*str += len;
	while(isspace((int)**str))
		(*str)++;
	return true;
}

/** Add reply packet to entry */
static struct reply_packet*
entry_add_reply(struct entry* entry) 
{
	struct reply_packet* pkt = (struct reply_packet*)malloc(
		sizeof(struct reply_packet));
	struct reply_packet ** p = &entry->reply_list;
	pkt->next = NULL;
	pkt->packet_sleep = 0;
	pkt->reply = ldns_pkt_new();
	pkt->reply_from_hex = NULL;
	/* link at end */
	while(*p)
		p = &((*p)->next);
	*p = pkt;
	return pkt;
}

/** parse MATCH line */
static void matchline(char* line, struct entry* e)
{
	char* parse = line;
	while(*parse) {
		if(isendline(*parse)) 
			return;
		if(str_keyword(&parse, "opcode")) {
			e->match_opcode = true;
		} else if(str_keyword(&parse, "qtype")) {
			e->match_qtype = true;
		} else if(str_keyword(&parse, "qname")) {
			e->match_qname = true;
		} else if(str_keyword(&parse, "subdomain")) {
			e->match_subdomain = true;
		} else if(str_keyword(&parse, "all")) {
			e->match_all = true;
		} else if(str_keyword(&parse, "ttl")) {
			e->match_ttl = true;
		} else if(str_keyword(&parse, "DO")) {
			e->match_do = true;
		} else if(str_keyword(&parse, "noedns")) {
			e->match_noedns = true;
		} else if(str_keyword(&parse, "ednsdata")) {
			e->match_ednsdata_raw = true;
		} else if(str_keyword(&parse, "UDP")) {
			e->match_transport = transport_udp;
		} else if(str_keyword(&parse, "TCP")) {
			e->match_transport = transport_tcp;
		} else if(str_keyword(&parse, "serial")) {
			e->match_serial = true;
			if(*parse != '=' && *parse != ':')
				error("expected = or : in MATCH: %s", line);
			parse++;
			e->ixfr_soa_serial = (uint32_t)strtol(parse, (char**)&parse, 10);
			while(isspace((int)*parse)) 
				parse++;
		} else {
			error("could not parse MATCH: '%s'", parse);
		}
	}
}

/** parse REPLY line */
static void replyline(char* line, ldns_pkt *reply)
{
	char* parse = line;
	while(*parse) {
		if(isendline(*parse)) 
			return;
			/* opcodes */
		if(str_keyword(&parse, "QUERY")) {
			ldns_pkt_set_opcode(reply, LDNS_PACKET_QUERY);
		} else if(str_keyword(&parse, "IQUERY")) {
			ldns_pkt_set_opcode(reply, LDNS_PACKET_IQUERY);
		} else if(str_keyword(&parse, "STATUS")) {
			ldns_pkt_set_opcode(reply, LDNS_PACKET_STATUS);
		} else if(str_keyword(&parse, "NOTIFY")) {
			ldns_pkt_set_opcode(reply, LDNS_PACKET_NOTIFY);
		} else if(str_keyword(&parse, "UPDATE")) {
			ldns_pkt_set_opcode(reply, LDNS_PACKET_UPDATE);
			/* rcodes */
		} else if(str_keyword(&parse, "NOERROR")) {
			ldns_pkt_set_rcode(reply, LDNS_RCODE_NOERROR);
		} else if(str_keyword(&parse, "FORMERR")) {
			ldns_pkt_set_rcode(reply, LDNS_RCODE_FORMERR);
		} else if(str_keyword(&parse, "SERVFAIL")) {
			ldns_pkt_set_rcode(reply, LDNS_RCODE_SERVFAIL);
		} else if(str_keyword(&parse, "NXDOMAIN")) {
			ldns_pkt_set_rcode(reply, LDNS_RCODE_NXDOMAIN);
		} else if(str_keyword(&parse, "NOTIMPL")) {
			ldns_pkt_set_rcode(reply, LDNS_RCODE_NOTIMPL);
		} else if(str_keyword(&parse, "REFUSED")) {
			ldns_pkt_set_rcode(reply, LDNS_RCODE_REFUSED);
		} else if(str_keyword(&parse, "YXDOMAIN")) {
			ldns_pkt_set_rcode(reply, LDNS_RCODE_YXDOMAIN);
		} else if(str_keyword(&parse, "YXRRSET")) {
			ldns_pkt_set_rcode(reply, LDNS_RCODE_YXRRSET);
		} else if(str_keyword(&parse, "NXRRSET")) {
			ldns_pkt_set_rcode(reply, LDNS_RCODE_NXRRSET);
		} else if(str_keyword(&parse, "NOTAUTH")) {
			ldns_pkt_set_rcode(reply, LDNS_RCODE_NOTAUTH);
		} else if(str_keyword(&parse, "NOTZONE")) {
			ldns_pkt_set_rcode(reply, LDNS_RCODE_NOTZONE);
			/* flags */
		} else if(str_keyword(&parse, "QR")) {
			ldns_pkt_set_qr(reply, true);
		} else if(str_keyword(&parse, "AA")) {
			ldns_pkt_set_aa(reply, true);
		} else if(str_keyword(&parse, "TC")) {
			ldns_pkt_set_tc(reply, true);
		} else if(str_keyword(&parse, "RD")) {
			ldns_pkt_set_rd(reply, true);
		} else if(str_keyword(&parse, "CD")) {
			ldns_pkt_set_cd(reply, true);
		} else if(str_keyword(&parse, "RA")) {
			ldns_pkt_set_ra(reply, true);
		} else if(str_keyword(&parse, "AD")) {
			ldns_pkt_set_ad(reply, true);
		} else if(str_keyword(&parse, "DO")) {
			ldns_pkt_set_edns_udp_size(reply, 4096);
			ldns_pkt_set_edns_do(reply, true);
		} else {
			error("could not parse REPLY: '%s'", parse);
		}
	}
}

/** parse ADJUST line */
static void adjustline(char* line, struct entry* e, 
	struct reply_packet* pkt)
{
	char* parse = line;
	while(*parse) {
		if(isendline(*parse)) 
			return;
		if(str_keyword(&parse, "copy_id")) {
			e->copy_id = true;
		} else if(str_keyword(&parse, "copy_query")) {
			e->copy_query = true;
		} else if(str_keyword(&parse, "sleep=")) {
			e->sleeptime = (unsigned int) strtol(parse, (char**)&parse, 10);
			while(isspace((int)*parse)) 
				parse++;
		} else if(str_keyword(&parse, "packet_sleep=")) {
			pkt->packet_sleep = (unsigned int) strtol(parse, (char**)&parse, 10);
			while(isspace((int)*parse)) 
				parse++;
		} else {
			error("could not parse ADJUST: '%s'", parse);
		}
	}
}

/** create new entry */
static struct entry* new_entry(void)
{
	struct entry* e = LDNS_MALLOC(struct entry);
	memset(e, 0, sizeof(*e));
	e->match_opcode = false;
	e->match_qtype = false;
	e->match_qname = false;
	e->match_subdomain = false;
	e->match_all = false;
	e->match_ttl = false;
	e->match_do = false;
	e->match_noedns = false;
	e->match_serial = false;
	e->ixfr_soa_serial = 0;
	e->match_transport = transport_any;
	e->reply_list = NULL;
	e->copy_id = false;
	e->copy_query = false;
	e->sleeptime = 0;
	e->next = NULL;
	return e;
}

/**
 * Converts a hex string to binary data
 * @param hexstr: string of hex.
 * @param len: is the length of the string
 * @param buf: is the buffer to store the result in
 * @param offset: is the starting position in the result buffer
 * @param buf_len: is the length of buf.
 * @return This function returns the length of the result
 */
static size_t
hexstr2bin(char *hexstr, int len, uint8_t *buf, size_t offset, size_t buf_len)
{
	char c;
	int i; 
	uint8_t int8 = 0;
	int sec = 0;
	size_t bufpos = 0;
	
	if (len % 2 != 0) {
		return 0;
	}

	for (i=0; i<len; i++) {
		c = hexstr[i];

		/* case insensitive, skip spaces */
		if (c != ' ') {
			if (c >= '0' && c <= '9') {
				int8 += c & 0x0f;  
			} else if (c >= 'a' && c <= 'z') {
				int8 += (c & 0x0f) + 9;   
			} else if (c >= 'A' && c <= 'Z') {
				int8 += (c & 0x0f) + 9;   
			} else {
				return 0;
			}
			 
			if (sec == 0) {
				int8 = int8 << 4;
				sec = 1;
			} else {
				if (bufpos + offset + 1 <= buf_len) {
					buf[bufpos+offset] = int8;
					int8 = 0;
					sec = 0; 
					bufpos++;
				} else {
					fprintf(stderr, "Buffer too small in hexstr2bin");
				}
			}
		}
        }
        return bufpos;
}

/** convert hex buffer to binary buffer */
static ldns_buffer *
data_buffer2wire(ldns_buffer *data_buffer)
{
	ldns_buffer *wire_buffer = NULL;
	int c;
	
	/* stat hack
	 * 0 = normal
	 * 1 = comment (skip to end of line)
	 * 2 = unprintable character found, read binary data directly
	 */
	size_t data_buf_pos = 0;
	int state = 0;
	uint8_t *hexbuf;
	int hexbufpos = 0;
	size_t wirelen;
	uint8_t *data_wire = (uint8_t *) ldns_buffer_begin(data_buffer);
	uint8_t *wire = LDNS_XMALLOC(uint8_t, LDNS_MAX_PACKETLEN);
	
	hexbuf = LDNS_XMALLOC(uint8_t, LDNS_MAX_PACKETLEN);
	for (data_buf_pos = 0; data_buf_pos < ldns_buffer_position(data_buffer); data_buf_pos++) {
		c = (int) data_wire[data_buf_pos];
		
		if (state < 2 && !isascii(c)) {
			/*verbose("non ascii character found in file: (%d) switching to raw mode\n", c);*/
			state = 2;
		}
		switch (state) {
			case 0:
				if (	(c >= '0' && c <= '9') ||
					(c >= 'a' && c <= 'f') ||
					(c >= 'A' && c <= 'F') )
				{
					if (hexbufpos >= LDNS_MAX_PACKETLEN) {
						error("buffer overflow");
						LDNS_FREE(hexbuf);
						return 0;

					}
					hexbuf[hexbufpos] = (uint8_t) c;
					hexbufpos++;
				} else if (c == ';') {
					state = 1;
				} else if (c == ' ' || c == '\t' || c == '\n') {
					/* skip whitespace */
				} 
				break;
			case 1:
				if (c == '\n' || c == EOF) {
					state = 0;
				}
				break;
			case 2:
				if (hexbufpos >= LDNS_MAX_PACKETLEN) {
					error("buffer overflow");
					LDNS_FREE(hexbuf);
					return 0;
				}
				hexbuf[hexbufpos] = (uint8_t) c;
				hexbufpos++;
				break;
		}
	}

	if (hexbufpos >= LDNS_MAX_PACKETLEN) {
		/*verbose("packet size reached\n");*/
	}
	
	/* lenient mode: length must be multiple of 2 */
	if (hexbufpos % 2 != 0) {
		if (hexbufpos >= LDNS_MAX_PACKETLEN) {
			error("buffer overflow");
			LDNS_FREE(hexbuf);
			return 0;
		}
		hexbuf[hexbufpos] = (uint8_t) '0';
		hexbufpos++;
	}

	if (state < 2) {
		wirelen = hexstr2bin((char *) hexbuf, hexbufpos, wire, 0, LDNS_MAX_PACKETLEN);
		wire_buffer = ldns_buffer_new(wirelen);
		ldns_buffer_new_frm_data(wire_buffer, wire, wirelen);
	} else {
		error("Incomplete hex data, not at byte boundary\n");
	}
	LDNS_FREE(wire);
	LDNS_FREE(hexbuf);
	return wire_buffer;
}	

/** parse ORIGIN */
static void 
get_origin(const char* name, int lineno, ldns_rdf** origin, char* parse)
{
	/* snip off rest of the text so as to make the parse work in ldns */
	char* end;
	char store;
	ldns_status status;

	ldns_rdf_free(*origin);
	*origin = NULL;

	end=parse;
	while(!isspace((int)*end) && !isendline(*end))
		end++;
	store = *end;
	*end = 0;
	verbose(3, "parsing '%s'\n", parse);
	status = ldns_str2rdf_dname(origin, parse);
	*end = store;
	if (status != LDNS_STATUS_OK)
		error("%s line %d:\n\t%s: %s", name, lineno,
		ldns_get_errorstr_by_id(status), parse);
}

/* Reads one entry from file. Returns entry or NULL on error. */
struct entry*
read_entry(FILE* in, const char* name, int *lineno, uint32_t* default_ttl, 
	ldns_rdf** origin, ldns_rdf** prev_rr, int skip_whitespace)
{
	struct entry* current = NULL;
	char line[MAX_LINE];
	char* parse;
	ldns_pkt_section add_section = LDNS_SECTION_QUESTION;
	struct reply_packet *cur_reply = NULL;
	bool reading_hex = false;
	bool reading_hex_ednsdata = false;
	ldns_buffer* hex_data_buffer = NULL;
	ldns_buffer* hex_ednsdata_buffer = NULL;

	while(fgets(line, (int)sizeof(line), in) != NULL) {
		line[MAX_LINE-1] = 0;
		parse = line;
		(*lineno) ++;
		
		while(isspace((int)*parse))
			parse++;
		/* test for keywords */
		if(isendline(*parse))
			continue; /* skip comment and empty lines */
		if(str_keyword(&parse, "ENTRY_BEGIN")) {
			if(current) {
				error("%s line %d: previous entry does not ENTRY_END", 
					name, *lineno);
			}
			current = new_entry();
			current->lineno = *lineno;
			cur_reply = entry_add_reply(current);
			continue;
		} else if(str_keyword(&parse, "$ORIGIN")) {
			get_origin(name, *lineno, origin, parse);
			continue;
		} else if(str_keyword(&parse, "$TTL")) {
			*default_ttl = (uint32_t)atoi(parse);
			continue;
		}

		/* working inside an entry */
		if(!current) {
			error("%s line %d: expected ENTRY_BEGIN but got %s", 
				name, *lineno, line);
		}
		if(str_keyword(&parse, "MATCH")) {
			matchline(parse, current);
		} else if(str_keyword(&parse, "REPLY")) {
			replyline(parse, cur_reply->reply);
		} else if(str_keyword(&parse, "ADJUST")) {
			adjustline(parse, current, cur_reply);
		} else if(str_keyword(&parse, "EXTRA_PACKET")) {
			cur_reply = entry_add_reply(current);
		} else if(str_keyword(&parse, "SECTION")) {
			if(str_keyword(&parse, "QUESTION"))
				add_section = LDNS_SECTION_QUESTION;
			else if(str_keyword(&parse, "ANSWER"))
				add_section = LDNS_SECTION_ANSWER;
			else if(str_keyword(&parse, "AUTHORITY"))
				add_section = LDNS_SECTION_AUTHORITY;
			else if(str_keyword(&parse, "ADDITIONAL"))
				add_section = LDNS_SECTION_ADDITIONAL;
			else error("%s line %d: bad section %s", name, *lineno, parse);
		} else if(str_keyword(&parse, "HEX_ANSWER_BEGIN")) {
			hex_data_buffer = ldns_buffer_new(LDNS_MAX_PACKETLEN);
			reading_hex = true;
		} else if(str_keyword(&parse, "HEX_ANSWER_END")) {
			if (!reading_hex) {
				error("%s line %d: HEX_ANSWER_END read but no HEX_ANSWER_BEGIN keyword seen", name, *lineno);
			}
			reading_hex = false;
			cur_reply->reply_from_hex = data_buffer2wire(hex_data_buffer);
			ldns_buffer_free(hex_data_buffer);
			hex_data_buffer = NULL;
		} else if(reading_hex) {
			ldns_buffer_printf(hex_data_buffer, line);
		} else if(str_keyword(&parse, "HEX_EDNSDATA_BEGIN")) {
			hex_ednsdata_buffer = ldns_buffer_new(LDNS_MAX_PACKETLEN);
			reading_hex_ednsdata = true;
		} else if(str_keyword(&parse, "HEX_EDNSDATA_END")) {
			if (!reading_hex_ednsdata) {
				error("%s line %d: HEX_EDNSDATA_END read but no"
					"HEX_EDNSDATA_BEGIN keyword seen", name, *lineno);
			}
			reading_hex_ednsdata = false;
			cur_reply->raw_ednsdata = data_buffer2wire(hex_ednsdata_buffer);
			ldns_buffer_free(hex_ednsdata_buffer);
			hex_ednsdata_buffer = NULL;
		} else if(reading_hex_ednsdata) {
			ldns_buffer_printf(hex_ednsdata_buffer, line);
		} else if(str_keyword(&parse, "ENTRY_END")) {
			if (hex_data_buffer)
				ldns_buffer_free(hex_data_buffer);
			return current;
		} else {
			/* it must be a RR, parse and add to packet. */
			ldns_rr* n = NULL;
			ldns_status status;
			char* rrstr = line;
			if (skip_whitespace)
				rrstr = parse;
			if(add_section == LDNS_SECTION_QUESTION)
				status = ldns_rr_new_question_frm_str(
					&n, rrstr, *origin, prev_rr);
			else status = ldns_rr_new_frm_str(&n, rrstr,
				*default_ttl, *origin, prev_rr);
			if(status != LDNS_STATUS_OK)
				error("%s line %d:\n\t%s: %s", name, *lineno,
					ldns_get_errorstr_by_id(status), rrstr);
			ldns_pkt_push_rr(cur_reply->reply, add_section, n);
		}

	}
	if (reading_hex) {
		error("%s: End of file reached while still reading hex, "
			"missing HEX_ANSWER_END\n", name);
	}
	if(current) {
		error("%s: End of file reached while reading entry. "
			"missing ENTRY_END\n", name);
	}
	return 0;
}

/* reads the canned reply file and returns a list of structs */
struct entry* 
read_datafile(const char* name, int skip_whitespace)
{
	struct entry* list = NULL;
	struct entry* last = NULL;
	struct entry* current = NULL;
	FILE *in;
	int lineno = 0;
	uint32_t default_ttl = 0;
	ldns_rdf* origin = NULL;
	ldns_rdf* prev_rr = NULL;
	int entry_num = 0;

	if((in=fopen(name, "r")) == NULL) {
		error("could not open file %s: %s", name, strerror(errno));
	}

	while((current = read_entry(in, name, &lineno, &default_ttl, 
		&origin, &prev_rr, skip_whitespace)))
	{
		if(last)
			last->next = current;
		else	list = current;
		last = current;
		entry_num ++;
	}
	verbose(1, "%s: Read %d entries\n", prog_name, entry_num);

	fclose(in);
	ldns_rdf_deep_free(origin);
	ldns_rdf_deep_free(prev_rr);
	return list;
}

/** get qtype from rr */
static ldns_rr_type get_qtype(ldns_pkt* p)
{
	if(!ldns_rr_list_rr(ldns_pkt_question(p), 0))
		return 0;
	return ldns_rr_get_type(ldns_rr_list_rr(ldns_pkt_question(p), 0));
}

/** returns owner from rr */
static ldns_rdf* get_owner(ldns_pkt* p)
{
	if(!ldns_rr_list_rr(ldns_pkt_question(p), 0))
		return NULL;
	return ldns_rr_owner(ldns_rr_list_rr(ldns_pkt_question(p), 0));
}

/** get authority section SOA serial value */
static uint32_t get_serial(ldns_pkt* p)
{
	ldns_rr *rr = ldns_rr_list_rr(ldns_pkt_authority(p), 0);
	ldns_rdf *rdf;
	uint32_t val;
	if(!rr) return 0;
	rdf = ldns_rr_rdf(rr, 2);
	if(!rdf) return 0;
	val = ldns_rdf2native_int32(rdf);
	verbose(3, "found serial %u in msg. ", (int)val);
	return val;
}

/** match two rr lists */
static int
match_list(ldns_rr_list* q, ldns_rr_list *p, bool mttl)
{
	size_t i;
	if(ldns_rr_list_rr_count(q) != ldns_rr_list_rr_count(p))
		return 0;
	for(i=0; i<ldns_rr_list_rr_count(q); i++)
	{
		if(ldns_rr_compare(ldns_rr_list_rr(q, i), 
			ldns_rr_list_rr(p, i)) != 0) {
			verbose(3, "rr %d different", (int)i);
			return 0;
		}
		if(mttl && ldns_rr_ttl(ldns_rr_list_rr(q, i)) !=
			ldns_rr_ttl(ldns_rr_list_rr(p, i))) {
			verbose(3, "rr %d ttl different", (int)i);
			return 0;
		}
	}
	return 1;
}

/** compare two booleans */
static int
cmp_bool(int x, int y)
{
	if(!x && !y) return 0;
	if(x && y) return 0;
	if(!x) return -1;
	return 1;
}

/** match all of the packet */
static int
match_all(ldns_pkt* q, ldns_pkt* p, bool mttl)
{
	if(ldns_pkt_get_opcode(q) != ldns_pkt_get_opcode(p)) 
	{ verbose(3, "allmatch: opcode different"); return 0;}
	if(ldns_pkt_get_rcode(q) != ldns_pkt_get_rcode(p))
	{ verbose(3, "allmatch: rcode different"); return 0;}
	if(ldns_pkt_id(q) != ldns_pkt_id(p))
	{ verbose(3, "allmatch: id different"); return 0;}
	if(cmp_bool(ldns_pkt_qr(q), ldns_pkt_qr(p)) != 0)
	{ verbose(3, "allmatch: qr different"); return 0;}
	if(cmp_bool(ldns_pkt_aa(q), ldns_pkt_aa(p)) != 0)
	{ verbose(3, "allmatch: aa different"); return 0;}
	if(cmp_bool(ldns_pkt_tc(q), ldns_pkt_tc(p)) != 0)
	{ verbose(3, "allmatch: tc different"); return 0;}
	if(cmp_bool(ldns_pkt_rd(q), ldns_pkt_rd(p)) != 0)
	{ verbose(3, "allmatch: rd different"); return 0;}
	if(cmp_bool(ldns_pkt_cd(q), ldns_pkt_cd(p)) != 0)
	{ verbose(3, "allmatch: cd different"); return 0;}
	if(cmp_bool(ldns_pkt_ra(q), ldns_pkt_ra(p)) != 0)
	{ verbose(3, "allmatch: ra different"); return 0;}
	if(cmp_bool(ldns_pkt_ad(q), ldns_pkt_ad(p)) != 0)
	{ verbose(3, "allmatch: ad different"); return 0;}
	if(ldns_pkt_qdcount(q) != ldns_pkt_qdcount(p))
	{ verbose(3, "allmatch: qdcount different"); return 0;}
	if(ldns_pkt_ancount(q) != ldns_pkt_ancount(p))
	{ verbose(3, "allmatch: ancount different"); return 0;}
	if(ldns_pkt_nscount(q) != ldns_pkt_nscount(p))
	{ verbose(3, "allmatch: nscount different"); return 0;}
	if(ldns_pkt_arcount(q) != ldns_pkt_arcount(p))
	{ verbose(3, "allmatch: arcount different"); return 0;}
	if(!match_list(ldns_pkt_question(q), ldns_pkt_question(p), 0))
	{ verbose(3, "allmatch: qd section different"); return 0;}
	if(!match_list(ldns_pkt_answer(q), ldns_pkt_answer(p), mttl))
	{ verbose(3, "allmatch: an section different"); return 0;}
	if(!match_list(ldns_pkt_authority(q), ldns_pkt_authority(p), mttl))
	{ verbose(3, "allmatch: ar section different"); return 0;}
	if(!match_list(ldns_pkt_additional(q), ldns_pkt_additional(p), mttl)) 
	{ verbose(3, "allmatch: ns section different"); return 0;}
	return 1;
}

/** Convert to hexstring and call verbose(), prepend with header */
static void
verbose_hex(int lvl, uint8_t *data, size_t datalen, const char *header)
{
	verbose(lvl, "%s", header);
	while (datalen-- > 0) {
		verbose(lvl, " %02x", (unsigned int)*data++);
	}
	verbose(lvl, "\n");
}

/** Match q edns data to p raw edns data */
static int
match_ednsdata(ldns_pkt* q, struct reply_packet* p)
{
	size_t qdlen, pdlen;
	uint8_t *qd, *pd;
	if(!ldns_pkt_edns(q) || !ldns_pkt_edns_data(q)) {
		verbose(3, "No EDNS data\n");
		return 0;
	}
	qdlen = ldns_rdf_size(ldns_pkt_edns_data(q));
	pdlen = ldns_buffer_limit(p->raw_ednsdata);
	qd = ldns_rdf_data(ldns_pkt_edns_data(q));
	pd = ldns_buffer_begin(p->raw_ednsdata);
	if( qdlen == pdlen && 0 == memcmp(qd, pd, qdlen) ) return 1;
	verbose(3, "EDNS data does not match.\n");
	verbose_hex(3, qd, qdlen, "q:");
	verbose_hex(3, pd, pdlen, "p:");
	return 0;
}

/* finds entry in list, or returns NULL */
struct entry* 
find_match(struct entry* entries, ldns_pkt* query_pkt,
	enum transport_type transport)
{
	struct entry* p = entries;
	ldns_pkt* reply = NULL;
	for(p=entries; p; p=p->next) {
		verbose(3, "comparepkt: ");
		reply = p->reply_list->reply;
		if(p->match_opcode && ldns_pkt_get_opcode(query_pkt) != 
			ldns_pkt_get_opcode(reply)) {
			verbose(3, "bad opcode\n");
			continue;
		}
		if(p->match_qtype && get_qtype(query_pkt) != get_qtype(reply)) {
			verbose(3, "bad qtype\n");
			continue;
		}
		if(p->match_qname) {
			if(!get_owner(query_pkt) || !get_owner(reply) ||
				ldns_dname_compare(
				get_owner(query_pkt), get_owner(reply)) != 0) {
				verbose(3, "bad qname\n");
				continue;
			}
		}
		if(p->match_subdomain) {
			if(!get_owner(query_pkt) || !get_owner(reply) ||
				(ldns_dname_compare(get_owner(query_pkt), 
				get_owner(reply)) != 0 &&
				!ldns_dname_is_subdomain(
				get_owner(query_pkt), get_owner(reply))))
			{
				verbose(3, "bad subdomain\n");
				continue;
			}
		}
		if(p->match_serial && get_serial(query_pkt) != p->ixfr_soa_serial) {
				verbose(3, "bad serial\n");
				continue;
		}
		if(p->match_do && !ldns_pkt_edns_do(query_pkt)) {
			verbose(3, "no DO bit set\n");
			continue;
		}
		if(p->match_noedns && ldns_pkt_edns(query_pkt)) {
			verbose(3, "bad; EDNS OPT present\n");
			continue;
		}
		if(p->match_ednsdata_raw && 
				!match_ednsdata(query_pkt, p->reply_list)) {
			verbose(3, "bad EDNS data match.\n");
			continue;
		}
		if(p->match_transport != transport_any && p->match_transport != transport) {
			verbose(3, "bad transport\n");
			continue;
		}
		if(p->match_all && !match_all(query_pkt, reply, p->match_ttl)) {
			verbose(3, "bad allmatch\n");
			continue;
		}
		verbose(3, "match!\n");
		return p;
	}
	return NULL;
}

void
adjust_packet(struct entry* match, ldns_pkt* answer_pkt, ldns_pkt* query_pkt)
{
	/* copy & adjust packet */
	if(match->copy_id)
		ldns_pkt_set_id(answer_pkt, ldns_pkt_id(query_pkt));
	if(match->copy_query) {
		ldns_rr_list* list = ldns_pkt_get_section_clone(query_pkt,
			LDNS_SECTION_QUESTION);
		ldns_rr_list_deep_free(ldns_pkt_question(answer_pkt));
		ldns_pkt_set_question(answer_pkt, list);
	}
	if(match->sleeptime > 0) {
		verbose(3, "sleeping for %d seconds\n", match->sleeptime);
#ifdef HAVE_SLEEP
		sleep(match->sleeptime);
#else
		Sleep(match->sleeptime * 1000);
#endif
	}
}

/*
 * Parses data buffer to a query, finds the correct answer 
 * and calls the given function for every packet to send.
 */
void
handle_query(uint8_t* inbuf, ptrdiff_t inlen, struct entry* entries, int* count,
	enum transport_type transport, void (*sendfunc)(uint8_t*, size_t, void*),
	void* userdata, FILE* verbose_out)
{
	ldns_status status;
	ldns_pkt *query_pkt = NULL;
	ldns_pkt *answer_pkt = NULL;
	struct reply_packet *p;
	ldns_rr *query_rr = NULL;
	uint8_t *outbuf = NULL;
	size_t answer_size = 0;
	struct entry* entry = NULL;
	ldns_rdf *stop_command = ldns_dname_new_frm_str("server.stop.");

	status = ldns_wire2pkt(&query_pkt, inbuf, (size_t)inlen);
	if (status != LDNS_STATUS_OK) {
		verbose(1, "Got bad packet: %s\n", ldns_get_errorstr_by_id(status));
		ldns_rdf_deep_free(stop_command);
		return;
	}
	
	query_rr = ldns_rr_list_rr(ldns_pkt_question(query_pkt), 0);
	verbose(1, "query %d: id %d: %s %d bytes: ", ++(*count), (int)ldns_pkt_id(query_pkt), 
		(transport==transport_tcp)?"TCP":"UDP", (int)inlen);
	if(verbose_out) ldns_rr_print(verbose_out, query_rr);
	if(verbose_out) ldns_pkt_print(verbose_out, query_pkt);

	if (ldns_rr_get_type(query_rr) == LDNS_RR_TYPE_TXT &&
	    ldns_rr_get_class(query_rr) == LDNS_RR_CLASS_CH &&
	    ldns_dname_compare(ldns_rr_owner(query_rr), stop_command) == 0) {
		exit(0);
        }
	
	/* fill up answer packet */
	entry = find_match(entries, query_pkt, transport);
	if(!entry || !entry->reply_list) {
		verbose(1, "no answer packet for this query, no reply.\n");
		ldns_pkt_free(query_pkt);
		ldns_rdf_deep_free(stop_command);
		return;
	}
	for(p = entry->reply_list; p; p = p->next)
	{
		verbose(3, "Answer pkt:\n");
		if (p->reply_from_hex) {
			/* try to parse the hex packet, if it can be
			 * parsed, we can use adjust rules. if not,
			 * send packet literally */
			status = ldns_buffer2pkt_wire(&answer_pkt, p->reply_from_hex);
			if (status == LDNS_STATUS_OK) {
				adjust_packet(entry, answer_pkt, query_pkt);
				if(verbose_out) ldns_pkt_print(verbose_out, answer_pkt);
				status = ldns_pkt2wire(&outbuf, answer_pkt, &answer_size);
				verbose(2, "Answer packet size: %u bytes.\n", (unsigned int)answer_size);
				if (status != LDNS_STATUS_OK) {
					verbose(1, "Error creating answer: %s\n", ldns_get_errorstr_by_id(status));
					ldns_pkt_free(query_pkt);
					ldns_rdf_deep_free(stop_command);
					return;
				}
				ldns_pkt_free(answer_pkt);
				answer_pkt = NULL;
			} else {
				verbose(3, "Could not parse hex data (%s), sending hex data directly.\n", ldns_get_errorstr_by_id(status));
				/* still try to adjust ID */
				answer_size = ldns_buffer_capacity(p->reply_from_hex);
				outbuf = LDNS_XMALLOC(uint8_t, answer_size);
				memcpy(outbuf, ldns_buffer_begin(p->reply_from_hex), answer_size);
				if(entry->copy_id) {
					ldns_write_uint16(outbuf, 
						ldns_pkt_id(query_pkt));
				}
			}
		} else {
			answer_pkt = ldns_pkt_clone(p->reply);
			adjust_packet(entry, answer_pkt, query_pkt);
			if(verbose_out) ldns_pkt_print(verbose_out, answer_pkt);
			status = ldns_pkt2wire(&outbuf, answer_pkt, &answer_size);
			verbose(1, "Answer packet size: %u bytes.\n", (unsigned int)answer_size);
			if (status != LDNS_STATUS_OK) {
				verbose(1, "Error creating answer: %s\n", ldns_get_errorstr_by_id(status));
				ldns_pkt_free(query_pkt);
				ldns_rdf_deep_free(stop_command);
				return;
			}
			ldns_pkt_free(answer_pkt);
			answer_pkt = NULL;
		}
		if(p->packet_sleep) {
			verbose(3, "sleeping for next packet %d secs\n", 
				p->packet_sleep);
#ifdef HAVE_SLEEP
			sleep(p->packet_sleep);
#else
			Sleep(p->packet_sleep * 1000);
#endif
			verbose(3, "wakeup for next packet "
				"(slept %d secs)\n", p->packet_sleep);
		}
		sendfunc(outbuf, answer_size, userdata);
		LDNS_FREE(outbuf);
		outbuf = NULL;
		answer_size = 0;
	}
	ldns_pkt_free(query_pkt);
	ldns_rdf_deep_free(stop_command);
}

/** delete the list of reply packets */
static void delete_replylist(struct reply_packet* replist)
{
	struct reply_packet *p=replist, *np;
	while(p) {
		np = p->next;
		ldns_pkt_free(p->reply);
		ldns_buffer_free(p->reply_from_hex);
		free(p);
		p=np;
	}
}

void delete_entry(struct entry* list)
{
	struct entry *p=list, *np;
	while(p) {
		np = p->next;
		delete_replylist(p->reply_list);
		free(p);
		p = np;
	}
}
