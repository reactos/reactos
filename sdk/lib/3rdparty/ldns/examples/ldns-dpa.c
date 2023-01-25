/*
 * ldns-dpa inspects the (udp) DNS packets found in a pcap file
 * and provides statistics about them
 * 
 * (C) NLnet Labs 2006 - 2008
 *
 * See the file LICENSE for the license
 */
#include "config.h"

#include <ldns/ldns.h>

#ifdef HAVE_PCAP_H
#ifdef HAVE_LIBPCAP
#include "ldns-dpa.h"

#ifdef HAVE_NETINET_IP6_H
#include <netinet/ip6.h>
#endif
#include <errno.h>

#ifndef IP_OFFMASK
#define IP_OFFMASK 0x1fff
#endif

int verbosity = 1;

#define ETHER_HEADER_LENGTH 14
#define UDP_HEADER_LENGTH 8
#define IP6_HEADER_LENGTH 40

/* some systems don't have this? */
#ifndef ETHERTYPE_IPV6
#define ETHERTYPE_IPV6 0x86dd
#endif

#define MAX_MATCHES 20
#define MAX_OPERATORS 7


/* global options */
bool show_filter_matches = false;
size_t total_nr_of_dns_packets = 0;
size_t total_nr_of_filtered_packets = 0;
size_t not_ip_packets = 0;
size_t bad_dns_packets = 0;
size_t arp_packets = 0;
size_t udp_packets = 0;
size_t tcp_packets = 0;
size_t fragmented_packets = 0;
size_t lost_packet_fragments = 0;
FILE *hexdumpfile = NULL;
pcap_dumper_t *dumper = NULL;
pcap_dumper_t *not_ip_dump = NULL;
pcap_dumper_t *bad_dns_dump = NULL;


struct
fragment_part {
	uint16_t ip_id;
	uint8_t data[65536];
	size_t cur_len;
};

struct fragment_part *fragment_p;

/* To add a match,
 * - add it to the enum
 * - add it to the table_matches const
 * - add a handler to value_matches
 * - tell in get_string_value() where in the packet the data lies
 * - add to parser?
 * - add to show_match_ function
 */
enum enum_match_ids {
	MATCH_ID,
	MATCH_OPCODE,
	MATCH_RCODE,
	MATCH_PACKETSIZE,
	MATCH_QR,
	MATCH_TC,
	MATCH_AD,
	MATCH_CD,
	MATCH_RD,
	MATCH_EDNS,
	MATCH_EDNS_PACKETSIZE,
	MATCH_DO,
	MATCH_QUESTION_SIZE,
	MATCH_ANSWER_SIZE,
	MATCH_AUTHORITY_SIZE,
	MATCH_ADDITIONAL_SIZE,
	MATCH_SRC_ADDRESS,
	MATCH_DST_ADDRESS,
	MATCH_TIMESTAMP,
	MATCH_QUERY,
	MATCH_QTYPE,
	MATCH_QNAME,
	MATCH_ANSWER,
	MATCH_AUTHORITY,
	MATCH_ADDITIONAL,
	MATCH_LAST
};
typedef enum enum_match_ids match_id;

enum enum_counter_types {
	TYPE_INT,
	TYPE_BOOL,
	TYPE_OPCODE,
	TYPE_RCODE,
	TYPE_STRING,
	TYPE_TIMESTAMP,
	TYPE_ADDRESS,
	TYPE_RR,
	TYPE_RR_TYPE,
	TYPE_LAST
};
typedef enum enum_counter_types counter_type;

const ldns_lookup_table lt_types[] = {
	{TYPE_INT, "int" },
	{TYPE_BOOL, "bool" },
	{TYPE_OPCODE, "opcode" },
	{TYPE_RCODE, "rcode" },
	{TYPE_STRING, "string" },
	{TYPE_TIMESTAMP, "timestamp" }, 
	{TYPE_ADDRESS, "address" }, 
	{TYPE_RR, "rr" },
	{ 0, NULL }
};

enum enum_type_operators {
	OP_EQUAL,
	OP_NOTEQUAL,
	OP_GREATER,
	OP_LESSER,
	OP_GREATEREQUAL,
	OP_LESSEREQUAL,
	OP_CONTAINS,
	OP_LAST
};
typedef enum enum_type_operators type_operator;

const ldns_lookup_table lt_operators[] = {
	{ OP_EQUAL, "=" },
	{ OP_NOTEQUAL, "!=" },
	{ OP_GREATER, ">" },
	{ OP_LESSER, "<" },
	{ OP_GREATEREQUAL, ">=" },
	{ OP_LESSEREQUAL, "<=" },
	{ OP_CONTAINS, "~=" },
	{ 0, NULL }
};

static const char *get_op_str(type_operator op) {
	const ldns_lookup_table *lt;
	lt = ldns_lookup_by_id((ldns_lookup_table *) lt_operators, op);
	if (lt) {
		return lt->name;
	} else {
		fprintf(stderr, "Unknown operator id: %u\n", op);
		exit(1);
	}
}

static type_operator
get_op_id(char *op_str)
{
	const ldns_lookup_table *lt;
	lt = ldns_lookup_by_name((ldns_lookup_table *) lt_operators, op_str);
	if (lt) {
		return (type_operator) lt->id;
	} else {
		fprintf(stderr, "Unknown operator: %s\n", op_str);
		exit(1);
	}
}

struct struct_type_operators {
	counter_type type;
	size_t operator_count;
	type_operator operators[10];
};
typedef struct struct_type_operators type_operators;

const type_operators const_type_operators[] = {
	{ TYPE_INT, 6, { OP_EQUAL, OP_NOTEQUAL, OP_GREATER, OP_LESSER, OP_GREATEREQUAL, OP_LESSEREQUAL, 0, 0, 0, 0 } },
	{ TYPE_BOOL, 2, { OP_EQUAL, OP_NOTEQUAL, 0, 0, 0, 0, 0, 0, 0, 0} },
	{ TYPE_OPCODE, 2, { OP_EQUAL, OP_NOTEQUAL, 0, 0, 0, 0, 0, 0, 0, 0} },
	{ TYPE_RCODE, 2, { OP_EQUAL, OP_NOTEQUAL, 0, 0, 0, 0, 0, 0, 0, 0} },
	{ TYPE_STRING, 3, { OP_EQUAL, OP_NOTEQUAL, OP_CONTAINS, 0, 0, 0, 0, 0, 0, 0} },
	{ TYPE_TIMESTAMP, 6, { OP_EQUAL, OP_NOTEQUAL, OP_GREATER, OP_LESSER, OP_GREATEREQUAL, OP_LESSEREQUAL, 0, 0, 0, 0 } },
	{ TYPE_ADDRESS, 3, { OP_EQUAL, OP_NOTEQUAL, OP_CONTAINS, 0, 0, 0, 0, 0, 0, 0} },
	{ TYPE_RR, 3, { OP_EQUAL, OP_NOTEQUAL, OP_CONTAINS, 0, 0, 0, 0, 0, 0, 0} },
	{ TYPE_RR_TYPE, 6, { OP_EQUAL, OP_NOTEQUAL, OP_GREATER, OP_LESSER, OP_GREATEREQUAL, OP_LESSEREQUAL, 0, 0, 0, 0 } },
	{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
};

const type_operators *
get_type_operators(counter_type type) {
	const type_operators *to = const_type_operators;
	while (to) {
		if (to->type == type) {
			return to;
		}
		to++;
	}
	return NULL;
}

struct struct_match_table {
	match_id id;
	const char *name;
	const char *description;
	const counter_type type;
};
typedef struct struct_match_table match_table;

/* order of entries has been changed after gprof analysis, and reasoning
 * about the uses of -u arguments
 */
const match_table matches[] = {
	{ MATCH_QUERY, "query", "String representation of the query RR", TYPE_RR },
	{ MATCH_QTYPE, "qtype", "RR Type of the question RR, if present", TYPE_RR_TYPE },
	{ MATCH_QNAME, "qname", "Owner name of the question RR, if present", TYPE_STRING },
	{ MATCH_SRC_ADDRESS, "srcaddress", "address the packet was sent from", TYPE_ADDRESS },
	{ MATCH_TIMESTAMP, "timestamp", "time the packet was sent", TYPE_TIMESTAMP },
	{ MATCH_DST_ADDRESS, "dstaddress", "address the packet was sent to", TYPE_ADDRESS },
	{ MATCH_EDNS_PACKETSIZE, "edns-packetsize", "packets size specified in edns rr", TYPE_INT },
	{ MATCH_ID, "id", "id of the packet", TYPE_INT },
	{ MATCH_OPCODE, "opcode", "opcode of packet (rfc1035)", TYPE_OPCODE },
	{ MATCH_RCODE, "rcode", "response code of packet", TYPE_RCODE },
	{ MATCH_PACKETSIZE, "packetsize", "size of packet in bytes", TYPE_INT },
	{ MATCH_QR, "qr", "value of qr bit", TYPE_BOOL },
	{ MATCH_TC, "tc", "value of tc bit", TYPE_BOOL },
	{ MATCH_AD, "ad", "value of ad bit", TYPE_BOOL },
	{ MATCH_CD, "cd", "value of cd bit", TYPE_BOOL },
	{ MATCH_RD, "rd", "value of rd bit", TYPE_BOOL },
	{ MATCH_EDNS, "edns", "existence of edns rr", TYPE_BOOL },
	{ MATCH_DO, "do", "value of do bit", TYPE_BOOL },
	{ MATCH_QUESTION_SIZE, "questionsize", "number of rrs in the question section", TYPE_INT },
	{ MATCH_ANSWER_SIZE, "answersize", "number of rrs in the answer section", TYPE_INT },
	{ MATCH_AUTHORITY_SIZE, "authoritysize", "number of rrs in the authority section", TYPE_INT },
	{ MATCH_ADDITIONAL_SIZE, "additionalsize", "number of rrs in the additional section", TYPE_INT },
	{ MATCH_ANSWER, "answer", "String representation of the answer RRs", TYPE_RR },
	{ MATCH_AUTHORITY, "authority", "String representation of the authority RRs", TYPE_RR },
	{ MATCH_ADDITIONAL, "additional", "String representation of the additional RRs", TYPE_RR },
	{ 0, NULL , NULL, TYPE_INT}
};

enum enum_match_expression_operators {
	MATCH_EXPR_OR,
	MATCH_EXPR_AND,
	MATCH_EXPR_LEAF
};
typedef enum enum_match_expression_operators match_expression_operator;

struct struct_match_operation {
	match_id id;
	type_operator operator;
	char *value;
};
typedef struct struct_match_operation match_operation;

typedef struct struct_match_expression match_expression;
struct struct_match_expression {
	/* and or or, or leaf (in which case there are no subtrees, but only a match_table */
	match_expression_operator op;
	match_expression *left;
	match_expression *right;
	match_operation *match;
	size_t count;
};

typedef struct struct_match_counters match_counters;
struct struct_match_counters {
/*
	match_expression **counter;
	size_t size;
*/
	match_expression *match;
	match_counters *left;
	match_counters *right;
};

match_table *
get_match_by_name(char *name) {
	match_table *mt = (match_table *) matches;
	if (name) {
		while (mt->name != NULL) {
			if (strcasecmp(name, mt->name) == 0) {
				return mt;
			}
			mt++;
		}
	}
	return NULL;
}

static match_table *
get_match_by_id(match_id id) {
	match_table *mt = (match_table *) matches;

	while (mt->name != NULL) {
		if (mt->id == id) {
			return mt;
		}
		mt++;
	}
	return NULL;
}

static const char *
get_match_name_str(match_id id) {
	match_table *mt = get_match_by_id(id);
	if (mt) {
		return mt->name;
	} else {
		fprintf(stderr, "Unknown match id: %u\n", id);
		exit(1);
		return "Unknown match id";
	}
}

static void
print_match_operation(FILE *output, match_operation *mc)
{
	match_table *mt = NULL;
	ldns_lookup_table *lt;
	struct timeval time;
	time_t time_tt;
	int value;
	size_t pos;
	char *tmp, *tmp2;

	if (mc) {
		mt = get_match_by_id(mc->id);

		if (mt) {
			fprintf(output, "%s %s ",mt->name, get_op_str(mc->operator));
			
			switch (mt->type) {
				case TYPE_INT:
				case TYPE_STRING:
				case TYPE_ADDRESS:
				case TYPE_RR:
					fprintf(output, "'%s'", mc->value);
					break;
				case TYPE_BOOL:
					if (strncmp(mc->value, "1", 2) == 0) {
						fprintf(output,"'true'");
					} else {
						fprintf(output,"'false'");
					}
					break;
				case TYPE_OPCODE:
					value = atoi(mc->value);
					lt = ldns_lookup_by_id(ldns_opcodes, value);
					if (lt) {
						fprintf(output, "%s", lt->name);
					} else {
						fprintf(output, "%s", mc->value);
					}
					break;
				case TYPE_RCODE:
					value = atoi(mc->value);
					lt = ldns_lookup_by_id(ldns_rcodes, value);
					if (lt) {
						fprintf(output, "%s", lt->name);
					} else {
						fprintf(output, "%s", mc->value);
					}
					break;
				case TYPE_TIMESTAMP:
#ifndef S_SPLINT_S
					time.tv_sec = (long int) atol(mc->value);
#endif
					time_tt = (time_t)time.tv_sec;
					tmp = ctime(&time_tt);
					tmp2 = malloc(strlen(tmp) + 1);
					for (pos = 0; pos < strlen(tmp); pos++) {
						if (tmp[pos] == '\n') {
							tmp2[pos] = '\0';
						} else {
							tmp2[pos] = tmp[pos];
						}
					}
					tmp2[pos] = '\0';
					fprintf(output, "%s", tmp2);
					free(tmp2);
					break;
				default:
				fprintf(output, "'%s'", mc->value);
			}

		} else {
			fprintf(output, "%u %s '%s'", mc->id, get_op_str(mc->operator), mc->value);
		}
	} else {
		fprintf(output, "(nil)");
	}
}

static void
print_match_expression(FILE *output, match_expression *expr)
{
	if (expr) {
		switch (expr->op) {
			case MATCH_EXPR_OR:
				fprintf(output, "(");
				print_match_expression(output, expr->left);
				fprintf(output, " | ");
				print_match_expression(output, expr->right);
				fprintf(output, ")");
				break;
			case MATCH_EXPR_AND:
				fprintf(output, "(");
				print_match_expression(output, expr->left);
				fprintf(output, " & ");
				print_match_expression(output, expr->right);
				fprintf(output, ")");
				break;
			case MATCH_EXPR_LEAF:
				print_match_operation(output, expr->match);
				break;
			default:
/*
				fprintf(output, "ERROR PRINTING MATCH: unknown op: %u\n", expr->op);
				exit(1);
*/
				fprintf(output, "(");
if (expr->left) {
	print_match_expression(output, expr->left);
}
				fprintf(output, " ? ");
if (expr->right) {
	print_match_expression(output, expr->right);
}
				fprintf(output, ") _");
if (expr->match) {
	print_match_operation(output, expr->match);				
}
fprintf(output, "_");
		}
	} else {
		printf("(nil)");
	}			
}

static void
print_counters(FILE *output, match_counters *counters, bool show_percentages, size_t total, int count_minimum)
{
	double percentage;

	if (!counters || !output) {
		return;
	}

	if (counters->left) {
		print_counters(output, counters->left, show_percentages, total, count_minimum);
	}
	if (counters->match) {
		if (count_minimum < (int) counters->match->count) {
			print_match_expression(output, counters->match);
			printf(": %u", (unsigned int) counters->match->count);
			if (show_percentages) {
				percentage = (double) counters->match->count / (double) total * 100.0;
				printf(" (%.2f%%)", percentage);
			}
			printf("\n");
		}
	}
	if (counters->right) {
		print_counters(output, counters->right, show_percentages, total, count_minimum);
	}
	
	return;	
}

static void
ldns_pkt2file_hex(FILE *fp, const ldns_pkt *pkt)
{
	uint8_t *wire;
	size_t size, i;
	ldns_status status;
	
	status = ldns_pkt2wire(&wire, pkt, &size);
	
	if (status != LDNS_STATUS_OK) {
		fprintf(stderr, "Unable to convert packet: error code %u", status);
		return;
	}
	
	fprintf(fp, "; 0");
	for (i = 1; i < 20; i++) {
		fprintf(fp, " %2u", (unsigned int) i);
	}
	fprintf(fp, "\n");
	fprintf(fp, ";--");
	for (i = 1; i < 20; i++) {
		fprintf(fp, " --");
	}
	fprintf(fp, "\n");
	for (i = 0; i < size; i++) {
		if (i % 20 == 0 && i > 0) {
			fprintf(fp, "\t; %4u-%4u\n", (unsigned int) i-19, (unsigned int) i);
		}
		fprintf(fp, " %02x", (unsigned int)wire[i]);
	}
	fprintf(fp, "\n\n");
}

/*
 * Calculate the total for all match operations with the same id as this one
 * (if they are 'under' this one in the tree, which should be the case in
 * the unique counter tree
 */
static size_t
calculate_total_value(match_counters *counters, match_operation *cur)
{
	size_t result = 0;
	
	if (!counters) {
		return 0;
	}
	
	if (counters->match->match->id == cur->id) {
		result = (size_t) atol(counters->match->match->value) * counters->match->count;
	}
	
	if (counters->left) {
		result += calculate_total_value(counters->left, cur);
	}
	if (counters->right) {
		result += calculate_total_value(counters->right, cur);
	}
	
	return result;
}

static size_t
calculate_total_count_matches(match_counters *counters, match_operation *cur)
{
	size_t result = 0;
	
	if (!counters) {
		return 0;
	}
	
	if (counters->match->match->id == cur->id) {
		result = 1;
	}
	
	if (counters->left) {
		/* In some cases, you don't want the number of actual
		   counted matches, for instance when calculating the
		   average number of queries per second. In this case
		   you want the number of seconds */
		if (cur->id == MATCH_TIMESTAMP) {
			result += (size_t) abs((int) (atol(counters->match->match->value) - atol(counters->left->match->match->value))) - 1;
		}
		result += calculate_total_count_matches(counters->left, cur);
	}
	if (counters->right) {
		if (cur->id == MATCH_TIMESTAMP) {
			result += (size_t) abs((int) (atol(counters->right->match->match->value) - atol(counters->match->match->value))) - 1;
		}
		result += calculate_total_count_matches(counters->right, cur);
	}
	
	return result;
}

/**
 * Returns true if there is a previous match operation with the given type
 * in the counters structure
 */
static bool
has_previous_match(match_counters *counters, match_operation *cur)
{
	if (!counters) {
		return false;
	}
	
	if (counters->left) {
		if (counters->left->match->match->id == cur->id) {
			return true;
		} else if (has_previous_match(counters->left, cur)) {
			return true;
		} else if (counters->left->right) {
			if (counters->left->right->match->match->id == cur->id) {
				return true;
			} else if (has_previous_match(counters->left->right, cur)) {
				return true;
			}
		}
	}
	return false;
}

/**
 * Returns true if there is a later match operation with the given type
 * in the counters structure
 */
static bool
has_next_match(match_counters *counters, match_operation *cur)
{
	if (!counters) {
		return false;
	}
	
	if (counters->right) {
		if (counters->right->match->match->id == cur->id) {
			return true;
		} else if (has_next_match(counters->right, cur)) {
			return true;
		} else if (counters->right->left) {
			if (counters->right->left->match->match->id == cur->id) {
				return true;
			} else if (has_next_match(counters->right->left, cur)) {
				return true;
			}
		}
	}
	return false;
}

/**
 * Returns the first match with the same type at *cur in
 * the counter list, or NULL if it is not found
 */
static match_expression *
get_first_match_expression(match_counters *counters, match_operation *cur)
{
	if (!counters) {
		return NULL;
	}
	
	if (has_previous_match(counters, cur)) {
		return get_first_match_expression(counters->left, cur);
	} else if (counters->match->match->id == cur->id) {
		return counters->match;
	} else if (counters->right) {
		return get_first_match_expression(counters->right, cur);
	} else {
		return NULL;
	}
}

/**
 * Returns the second match expression with the same type at *cur in
 * the counter list, or NULL if it is not found
 */
static match_expression *
get_second_match_expression(match_counters *counters, match_operation *cur)
{
	if (!counters) {
		return NULL;
	}
	
	if (has_previous_match(counters, cur)) {
		if (has_previous_match(counters->left, cur)) {
			return get_second_match_expression(counters->left, cur);
		} else {
			return counters->left->match;
		}
/*
	} else if (counters->match->match->id == cur->id) {
		return counters->match->match->value;
*/	} else if (counters->right) {
		return get_first_match_expression(counters->right, cur);
	} else {
		return NULL;
	}
}

/**
 * Returns the last match expression with the same type at *cur in
 * the counter list, or NULL if it is not found
 */
static match_expression *
get_last_match_expression(match_counters *counters, match_operation *cur)
{
	if (!counters) {
		return NULL;
	}
	
	if (has_next_match(counters, cur)) {
		return get_last_match_expression(counters->right, cur);
	} else if (counters->match->match->id == cur->id) {
		return counters->match;
	} else if (counters->left) {
		return get_last_match_expression(counters->left, cur);
	} else {
		return NULL;
	}
}

/**
 * Returns the last but one match expression with the same type at *cur in
 * the counter list, or NULL if it is not found
 */
static match_expression *
get_last_but_one_match_expression(match_counters *counters, match_operation *cur)
{
	if (!counters) {
		return NULL;
	}
	
	if (has_next_match(counters, cur)) {
		if (has_next_match(counters->right, cur)) {
			return get_last_but_one_match_expression(counters->right, cur);
		} else {
			return counters->match;
		}
/*
	} else if (counters->match->match->id == cur->id) {
		return counters->match->match->value;
*/	} else if (counters->left) {
		return get_last_match_expression(counters->right, cur);
	} else {
		return NULL;
	}
}

static size_t
get_first_count(match_counters *counters, match_operation *cur)
{
	match_expression *o = get_first_match_expression(counters, cur);
	if (o) {
		return o->count;
	} else {
		return 0;
	}
}

static size_t
get_last_count(match_counters *counters, match_operation *cur)
{
	match_expression *o = get_last_match_expression(counters, cur);
	if (o) {
		return o->count;
	} else {
		return 0;
	}
}


static size_t
calculate_total_count(match_counters *counters, match_operation *cur)
{
	size_t result = 0;
	
	if (!counters) {
		return 0;
	}
	
	if (counters->match->match->id == cur->id) {
		result = counters->match->count;
	}
	
	if (counters->left) {
		result += calculate_total_count(counters->left, cur);
	}
	if (counters->right) {
		result += calculate_total_count(counters->right, cur);
	}
	
	return result;
}

static void
print_counter_averages(FILE *output, match_counters *counters, match_operation *cur)
{
	size_t total_value;
	size_t total_count;
	match_table *mt;
	
	if (!counters || !output) {
		return;
	}
	
	if (!cur) {
		cur = counters->match->match;
		mt = get_match_by_id(cur->id);
		total_value = calculate_total_value(counters, cur);
		total_count = calculate_total_count(counters, cur);
		printf("Average for %s: (%u / %u) %.02f\n", mt->name, (unsigned int) total_value, (unsigned int) total_count, (float) total_value / (float) total_count);
		if (counters->left) {
			print_counter_averages(output, counters->left, cur);
		}
		if (counters->right) {
			print_counter_averages(output, counters->right, cur);
		}
	} else {
		if (counters->left) {
			if (counters->left->match->match->id != cur->id) {
				print_counter_averages(output, counters->left, NULL);
			}
		}
		if (counters->right) {
			if (counters->right->match->match->id != cur->id) {
				print_counter_averages(output, counters->right, NULL);
			}
		}
	}
	
	return;	
}

static void
print_counter_average_count(FILE *output, match_counters *counters, match_operation *cur, bool remove_first_last)
{
	size_t total_matches;
	size_t total_count;
	match_table *mt;
	
	if (!counters || !output) {
		return;
	}
	
	if (!cur) {
		cur = counters->match->match;
		mt = get_match_by_id(cur->id);
		total_matches = calculate_total_count_matches(counters, cur);
		total_count = calculate_total_count(counters, cur);
		/* Remove the first and last for instance for timestamp average counts (half seconds drag down the average) */
		if (remove_first_last) {
			total_count -= get_first_count(counters, cur);
			total_count -= get_last_count(counters, cur);	
			printf("Removing first count from average: %u\n", (unsigned int) get_first_count(counters,cur));
			printf("Removing last count from average: %u\n", (unsigned int) get_last_count(counters,cur));
			/* in the case where we count the differences between match values too
			 * (like with timestamps) we need to subtract from the match count too
			 */
			if (cur->id == MATCH_TIMESTAMP) {
				if (get_first_match_expression(counters, cur) && get_second_match_expression(counters, cur)) {
					total_matches -= atol(get_second_match_expression(counters, cur)->match->value) - atol(get_first_match_expression(counters, cur)->match->value);
				}
				if (get_last_match_expression(counters, cur) && get_last_but_one_match_expression(counters, cur)) {
					total_matches -= atol(get_last_match_expression(counters, cur)->match->value) - atol(get_last_but_one_match_expression(counters, cur)->match->value);
				}
			} else {
				total_matches -= 2;
			}
		}
		printf("Average count for %s: (%u / %u) %.02f\n", mt->name, (unsigned int) total_count, (unsigned int) total_matches, (float) total_count / (float) total_matches);
		if (counters->left) {
			print_counter_averages(output, counters->left, cur);
		}
		if (counters->right) {
			print_counter_averages(output, counters->right, cur);
		}
	} else {
		if (counters->left) {
			if (counters->left->match->match->id != cur->id) {
				print_counter_averages(output, counters->left, NULL);
			}
		}
		if (counters->right) {
			if (counters->right->match->match->id != cur->id) {
				print_counter_averages(output, counters->right, NULL);
			}
		}
	}
	
	return;	
}

static bool
match_int(type_operator operator,
          char *value,
	  char *mvalue)
{
	int a, b;

	if (!value || !mvalue) {
		return false;
	}

	a = atoi(value);
	b = atoi(mvalue);

	switch (operator) {
		case OP_EQUAL:
			return a == b;
			break;
		case OP_NOTEQUAL:
			return a != b;
			break;
		case OP_GREATER:
			return a > b;
			break;
		case OP_LESSER:
			return a < b;
			break;
		case OP_GREATEREQUAL:
			return a >= b;
			break;
		case OP_LESSEREQUAL:
			return a <= b;
			break;
		default:
			fprintf(stderr, "Unknown operator: %u\n", operator);
			exit(2);
	}
}

static bool
match_opcode(type_operator operator,
             char *value,
             char *mvalue)
{
	ldns_pkt_opcode a, b;
	int i;
	ldns_lookup_table *lt;

	/* try parse name first, then parse as int */
	lt = ldns_lookup_by_name(ldns_opcodes, value);
	if (lt) {
		a = lt->id;
	} else {
		i = atoi(value);
		if (i >= 0 && isdigit((unsigned char)value[0])) {
			lt = ldns_lookup_by_id(ldns_opcodes, i);
			if (lt) {
				a = lt->id;
			} else {
				fprintf(stderr, "Unknown opcode: %s\n", value);
				exit(1);
				return false;
			}
		} else {
			fprintf(stderr, "Unknown opcode: %s\n", value);
			exit(1);
			return false;
		}
	}

	lt = ldns_lookup_by_name(ldns_opcodes, mvalue);
	if (lt) {
		b = lt->id;
	} else {
		i = atoi(mvalue);
		if (i >= 0 && isdigit((unsigned char)mvalue[0])) {
			lt = ldns_lookup_by_id(ldns_opcodes, i);
			if (lt) {
				b = lt->id;
			} else {
				fprintf(stderr, "Unknown opcode: %s\n", mvalue);
				exit(1);
				return false;
			}
		} else {
			fprintf(stderr, "Unknown opcode: %s\n", mvalue);
			exit(1);
			return false;
		}
	}

	switch(operator) {
		case OP_EQUAL:
			return a == b;
			break;
		case OP_NOTEQUAL:
			return a != b;
			break;
		default:
			fprintf(stderr, "Error bad operator for opcode: %s\n", get_op_str(operator));
			return false;
			break;
	}
}

static bool
match_str(type_operator operator,
          char *value,
          char *mvalue)
{
	char *valuedup, *mvaluedup;
	size_t i;
	bool result;
	
	if (operator == OP_CONTAINS) {
		/* strcasestr is not C89 
		return strcasestr(value, mvalue) != 0;
		*/
		valuedup = _strdup(value);
		mvaluedup = _strdup(mvalue);
		for (i = 0; i < strlen(valuedup); i++) {
			valuedup[i] = tolower((unsigned char)valuedup[i]);
		}
		for (i = 0; i < strlen(mvaluedup); i++) {
			mvaluedup[i] = tolower((unsigned char)mvaluedup[i]);
		}
		result = strstr(valuedup, mvaluedup) != 0;
		free(valuedup);
		free(mvaluedup);
		return result;
	} else if (operator == OP_EQUAL) {
		return strcmp(value, mvalue) == 0;
	} else {
		return strcmp(value, mvalue) != 0;
	}	
}

static bool
match_rr_type(type_operator operator,
              char *value,
	      char *mvalue)
{
	ldns_rr_type a,b;
	
	a = ldns_get_rr_type_by_name(value);
	b = ldns_get_rr_type_by_name(mvalue);
	
	switch (operator) {
		case OP_EQUAL:
			return a == b;
			break;
		case OP_NOTEQUAL:
			return a != b;
			break;
		case OP_GREATER:
			return a > b;
			break;
		case OP_LESSER:
			return a < b;
			break;
		case OP_GREATEREQUAL:
			return a >= b;
			break;
		case OP_LESSEREQUAL:
			return a <= b;
			break;
		default:
			fprintf(stderr, "Unknown operator: %u\n", operator);
			exit(2);
	}
}

static bool
match_rcode(type_operator operator,
             char *value,
             char *mvalue)
{
	int a, b;
	int i;
	ldns_lookup_table *lt;

	/* try parse name first, then parse as int */
	lt = ldns_lookup_by_name(ldns_rcodes, value);
	if (lt) {
		a = lt->id;
	} else {
		i = atoi(value);
		if (i >= 0 && isdigit((unsigned char)value[0])) {
			lt = ldns_lookup_by_id(ldns_rcodes, i);
			if (lt) {
				a = lt->id;
			} else {
				fprintf(stderr, "Unknown rcode: %s\n", value);
				exit(1);
				return false;
			}
		} else {
			fprintf(stderr, "Unknown rcode: %s\n", value);
			exit(1);
			return false;
		}
	}

	lt = ldns_lookup_by_name(ldns_rcodes, mvalue);
	if (lt) {
		b = lt->id;
	} else {
		i = atoi(mvalue);
		if (i >= 0 && isdigit((unsigned char)mvalue[0])) {
			lt = ldns_lookup_by_id(ldns_rcodes, i);
			if (lt) {
				b = lt->id;
			} else {
				fprintf(stderr, "Unknown rcode: %s\n", mvalue);
				exit(1);
				return false;
			}
		} else {
			fprintf(stderr, "Unknown rcode: %s\n", mvalue);
			exit(1);
			return false;
		}
	}

	switch(operator) {
		case OP_EQUAL:
			return a == b;
			break;
		case OP_NOTEQUAL:
			return a != b;
			break;
		default:
			fprintf(stderr, "Error bad operator for rcode: %s\n", get_op_str(operator));
			return false;
			break;
	}
}

static bool
value_matches(match_id id,
        type_operator operator,
        char *value,
        char *mvalue)
{
	int result;

	if (verbosity >= 5) {
		printf("Match %s: %s %s %s: ", get_match_name_str(id), value, get_op_str(operator), mvalue);
	}
	switch(id) {
		case MATCH_OPCODE:
			result = match_opcode(operator, value, mvalue);
			break;
		case MATCH_RCODE:
			result = match_rcode(operator, value, mvalue);
			break;
		case MATCH_ID:
		case MATCH_QR:
		case MATCH_TC:
		case MATCH_AD:
		case MATCH_CD:
		case MATCH_RD:
		case MATCH_DO:
		case MATCH_PACKETSIZE:
		case MATCH_EDNS:
		case MATCH_EDNS_PACKETSIZE:
		case MATCH_QUESTION_SIZE:
		case MATCH_ANSWER_SIZE:
		case MATCH_AUTHORITY_SIZE:
		case MATCH_ADDITIONAL_SIZE:
		case MATCH_TIMESTAMP:
			result = match_int(operator, value, mvalue);
			break;
		case MATCH_QUERY:
		case MATCH_QNAME:
		case MATCH_ANSWER:
		case MATCH_AUTHORITY:
		case MATCH_ADDITIONAL:
			result = match_str(operator, value, mvalue);
			break;
		case MATCH_SRC_ADDRESS:
		case MATCH_DST_ADDRESS:
			result = match_str(operator, value, mvalue);
			break;
		case MATCH_QTYPE:
			result = match_rr_type(operator, value, mvalue);
			break;
		default:
			fprintf(stderr, "Error: value_matches() for operator %s not implemented yet.\n", get_op_str((type_operator) id));
			exit(3);
	}
	if (verbosity >= 5) {
		if (result) {
			printf("true\n");
		} else {
			printf("false\n");
		}
	}
	return result;
}

static char *
get_string_value(match_id id, ldns_pkt *pkt, ldns_rdf *src_addr, ldns_rdf *dst_addr)
{
	char *val;
	match_table *mt;
	size_t valsize = 100;

	val = malloc(valsize);
	memset(val, 0, valsize);

	switch(id) {
		case MATCH_QR:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_qr(pkt));
			break;
		case MATCH_ID:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_id(pkt));
			break;
		case MATCH_OPCODE:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_get_opcode(pkt));
			break;
		case MATCH_RCODE:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_get_rcode(pkt));
			break;
		case MATCH_PACKETSIZE:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_size(pkt));
			break;
		case MATCH_TC:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_tc(pkt));
			break;
		case MATCH_AD:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_ad(pkt));
			break;
		case MATCH_CD:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_cd(pkt));
			break;
		case MATCH_RD:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_rd(pkt));
			break;
		case MATCH_EDNS:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_edns(pkt));
			break;
		case MATCH_EDNS_PACKETSIZE:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_edns_udp_size(pkt));
			break;
		case MATCH_DO:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_edns_do(pkt));
			break;
		case MATCH_QUESTION_SIZE:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_qdcount(pkt));
			break;
		case MATCH_ANSWER_SIZE:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_ancount(pkt));
			break;
		case MATCH_AUTHORITY_SIZE:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_nscount(pkt));
			break;
		case MATCH_ADDITIONAL_SIZE:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_arcount(pkt));
			break;
		case MATCH_SRC_ADDRESS:
			free(val);
			val = ldns_rdf2str(src_addr);
			break;
		case MATCH_DST_ADDRESS:
			free(val);
			val = ldns_rdf2str(dst_addr);
			break;
		case MATCH_TIMESTAMP:
			snprintf(val, valsize, "%u", (unsigned int) ldns_pkt_timestamp(pkt).tv_sec);
			break;
		case MATCH_QUERY:
			if (ldns_pkt_qdcount(pkt) > 0) {
				free(val);
				val = ldns_rr2str(ldns_rr_list_rr(ldns_pkt_question(pkt), 0));
				/* replace \n for nicer printing later */
				if (strchr(val, '\n')) {
					*(strchr(val, '\n')) = '\0';
				}
			} else {
				val[0] = '\0';
			}
			break;
		case MATCH_QNAME:
			if (ldns_pkt_qdcount(pkt) > 0) {
				free(val);
				val = ldns_rdf2str(ldns_rr_owner(ldns_rr_list_rr(ldns_pkt_question(pkt), 0)));
				/* replace \n for nicer printing later */
				if (strchr(val, '\n')) {
					*(strchr(val, '\n')) = '\0';
				}
			} else {
				val[0] = '\0';
			}
			break;
		case MATCH_QTYPE:
			if (ldns_pkt_qdcount(pkt) > 0) {
				free(val);
				val = ldns_rr_type2str(ldns_rr_get_type(ldns_rr_list_rr(ldns_pkt_question(pkt), 0)));
			} else {
				val[0] = '\0';
			}
			break;
		case MATCH_ANSWER:
			if (ldns_pkt_ancount(pkt) > 0) {
				free(val);
				val = ldns_rr_list2str(ldns_pkt_answer(pkt));
			} else {
				val[0] = '\0';
			}
			break;
		case MATCH_AUTHORITY:
			if (ldns_pkt_nscount(pkt) > 0) {
				free(val);
				val = ldns_rr_list2str(ldns_pkt_authority(pkt));
			} else {
				val[0] = '\0';
			}
			break;
		case MATCH_ADDITIONAL:
			if (ldns_pkt_arcount(pkt) > 0) {
				free(val);
				val = ldns_rr_list2str(ldns_pkt_additional(pkt));
			} else {
				val[0] = '\0';
			}
			break;
		default:
			mt = get_match_by_id(id);
			if (!mt) {
				printf("ERROR UNKNOWN MATCH_TABLE ID %u\n", id);
				exit(1);
			}
			printf("Matcher for %s not implemented yet\n", mt->name);
			exit(1);
			return NULL;
	}

	return val;
}

static bool
match_packet_to_operation(ldns_pkt *pkt, ldns_rdf *src_addr, ldns_rdf *dst_addr, match_operation *operation)
{
	bool result;
	char *val;

	if (!pkt || !operation) {
		return false;
	} else {
		val = get_string_value(operation->id, pkt, src_addr, dst_addr);
		if (!val) {
			return false;
		}
		result = value_matches(operation->id, operation->operator, val, operation->value);
		free(val);
		return result;
	}
}

static int
match_operation_compare(const void *a, const void *b)
{
	match_operation *moa, *mob;
	match_table *mt;
	long ia, ib;

	if (!a) {
		return 1;
	} else if (!b) {
		return -1;
	} else {
		moa = (match_operation *) a;
		mob = (match_operation *) b;

		if (moa->id < mob->id) {
			return -1;
		} else if (moa->id > mob->id) {
			return 1;
		} else {
			if (moa->operator < mob->operator) {
				return -1;
			} else if (moa->operator > mob->operator) {
				return 1;
			} else {
				mt = get_match_by_id(moa->id);
				if (mt) {
					switch (mt->type) {
						case TYPE_INT:
						case TYPE_TIMESTAMP:
						case TYPE_BOOL:
						case TYPE_OPCODE:
						case TYPE_RCODE:
							ia = atol(moa->value);
							ib = atol(mob->value);
							return ia - ib;
							break;
						case TYPE_STRING:
						case TYPE_ADDRESS:
						case TYPE_RR:
						default:
							return strcmp(moa->value, mob->value);
							break;
					}
				} else {
					return strcmp(moa->value, mob->value);
				}
			}
		}
	}
}

static int
match_expression_compare(const void *a, const void *b)
{
	match_expression *mea, *meb;
	
	if (!a) {
		return 1;
	} else if (!b) {
		return -1;
	} else {
		mea = (match_expression *) a;
		meb = (match_expression *) b;
		
		if (mea->op < meb->op) {
			return -1;
		} else if (mea->op > meb->op) {
			return 1;
		} else {
			switch(mea->op) {
				case MATCH_EXPR_AND:
				case MATCH_EXPR_OR:
					if (match_expression_compare(mea->left, meb->left) < 0) {
						return -1;
					} else if (match_expression_compare(mea->left, meb->left) > 0) {
						return 1;
					} else {
						return match_expression_compare(mea->right, meb->right);
					}
					break;
				case MATCH_EXPR_LEAF:
					return match_operation_compare(mea->match, meb->match);
					break;
				default:
					fprintf(stderr, "Unknown Match Expression logic operator: %u\n", mea->op);
					exit(1);
			}
		}
	}
}

/**
 * If count is true, and the counter is found, its count is increased by 1
 */
static int
add_match_counter(match_counters *counters,
		  match_expression *expr,
                  bool count)
{
	int cmp;
	match_counters *new;

	if (!counters || !expr) {
		return -1;
	} else {
		if (counters->match) {
			cmp = match_expression_compare(counters->match, 
			                               expr);
			if (cmp > 0) {
				if (counters->left) {
					return add_match_counter(counters->left,
					                         expr,
					                         count);
				} else {
					new = malloc(sizeof(match_counters));
					new->left = NULL;
					new->right = NULL;
					new->match = expr;
					counters->left = new;
					return 0;
				}
			} else if (cmp < 0) {
				if (counters->right) {
					return add_match_counter(counters->right,
					                         expr,
					                         count);
				} else {
					new = malloc(sizeof(match_counters));
					new->left = NULL;
					new->right = NULL;
					new->match = expr;
					counters->right = new;
					return 0;
				}
			} else  {
				/* already there? */
				if (count) {
					counters->match->count++;
				}
				return 1;
			}
		} else {
			/* shouldn't happen but anyway */
			counters->match = expr;
		}
	}
	return 0;
}

static bool
match_dns_packet_to_expr(ldns_pkt *pkt, ldns_rdf *src_addr, ldns_rdf *dst_addr, match_expression *expr)
{
	bool result;

	if (!pkt || !expr) {
		return false;
	}
	
	switch(expr->op) {
		case MATCH_EXPR_OR:
			result = (match_dns_packet_to_expr(pkt, src_addr, dst_addr, expr->left) ||
			       match_dns_packet_to_expr(pkt, src_addr, dst_addr, expr->right));
			break;
		case MATCH_EXPR_AND:
			result = (match_dns_packet_to_expr(pkt, src_addr, dst_addr, expr->left) &&
			       match_dns_packet_to_expr(pkt, src_addr, dst_addr, expr->right));
			break;
		case MATCH_EXPR_LEAF:
			result = match_packet_to_operation(pkt, src_addr, dst_addr, expr->match);
			break;
		default:
			fprintf(stderr, "Error, unknown expression operator %u\n", expr->op);
			fprintf(stderr, "full expression:\n");
			print_match_expression(stderr, expr);
			fprintf(stderr, "\n");
			exit(1);
	}

	if (result) {
		if (verbosity >= 5) {
			printf("Found Match:\n");
			print_match_expression(stdout, expr);
			printf("\nCount now %u\n", (unsigned int) expr->count);
		}
		expr->count++;
	}

	return result;
}

static void
free_match_operation(match_operation *operation)
{
	if (operation) {
		if (operation->value) {
			free(operation->value);
		}
		free(operation);
	}
}

static void
free_match_expression(match_expression *expr)
{
	if (expr) {
		switch(expr->op) {
			case MATCH_EXPR_OR:
			case MATCH_EXPR_AND:
				free_match_expression(expr->left);
				free_match_expression(expr->right);
				break;
			case MATCH_EXPR_LEAF:
				free_match_operation(expr->match);
				break;
		}
		free(expr);
	}
}

static void
free_counters(match_counters *counters)
{
	if (counters) {
		if (counters->left) {
			free_counters(counters->left);
		}
		if (counters->match) {
			free_match_expression(counters->match);
		}
		if (counters->right) {
			free_counters(counters->right);
		}
		free(counters);
	}
}

static void
match_pkt_counters(ldns_pkt *pkt, ldns_rdf *src_addr, ldns_rdf *dst_addr, match_counters *counts)
{
	if (counts->left) {
		match_pkt_counters(pkt, src_addr, dst_addr, counts->left);
	}
	if (counts->match) {
		if (match_dns_packet_to_expr(pkt, src_addr, dst_addr, counts->match)) {
/*
			counts->match->count++;
*/
		}
	}
	if (counts->right) {
		match_pkt_counters(pkt, src_addr, dst_addr, counts->right);
	}	
}

static void
match_pkt_uniques(ldns_pkt *pkt, ldns_rdf *src_addr, ldns_rdf *dst_addr, match_counters *uniques, match_id unique_ids[], size_t unique_id_count)
{
	match_expression *me;
	size_t i;
	match_operation *mo;
	int add_result;
	
	for (i = 0; i < unique_id_count; i++) {
		mo = malloc(sizeof(match_operation));
		mo->id = unique_ids[i];
		mo->operator = OP_EQUAL;
		mo->value = get_string_value(mo->id, pkt, src_addr, dst_addr);

		me = malloc(sizeof(match_expression));
		me->op = MATCH_EXPR_LEAF;
		me->left = NULL;
		me->right = NULL;
		me->match = mo;
		me->count = 1;

		add_result = add_match_counter(uniques, me, true);
		/* if result=1 it was already found, so delete new one */
		if (add_result == 1) {
			free_match_expression(me);
		}
	}

#if 0
	size_t i, j;
	bool found;
	match_expression *me;
	match_operation *mo;

	/* get the value, match uniques for that, if not match, add new */
	/* all unique values should be MATCH_EXPR_LEAF */
		found = false;
		for (j = 0; j < uniques->size; j++) {
			if (uniques->counter[j]->match->id == unique_ids[i]) {
				if (match_dns_packet_to_expr(pkt, src_addr, dst_addr, uniques->counter[j])) {
					found = true;
				}
			}
		}
		if (!found) {
			mo = malloc(sizeof(match_operation));
			mo->id = unique_ids[i];
			mo->operator = OP_EQUAL;
			mo->value = get_string_value(mo->id, pkt, src_addr, dst_addr);

			me = malloc(sizeof(match_expression));
			me->match = mo;
			me->op = MATCH_EXPR_LEAF;
			me->left = NULL;
			me->right = NULL;
			me->count = 1;

			add_counter(uniques, me);
		}
	}
#endif
}

static match_expression *
parse_match_expression(char *string)
{
	match_expression *expr;
	size_t i,j;
	size_t leftstart, leftend = 0;
	char *left_str, *op = NULL, *val;
	match_table *mt;
	match_operation *mo = NULL;
	const type_operators *tos;
	match_expression *result;
	ldns_lookup_table *lt = NULL;

	/* remove whitespace */
	char *str = calloc(1, strlen(string) + 1);

	j = 0;
	for (i = 0; i < strlen(string); i++) {
		if(!isspace((unsigned char)string[i])) {
			str[j] = string[i];
			j++;
		}
	}
	str[j] = '\0';
	
	expr = malloc(sizeof(match_expression));
	expr->left = NULL;
	expr->right = NULL;
	expr->match = NULL;
	expr->count = 0;
	leftstart = 0;
	for (i = 0; i < strlen(str); i++) {
		if (str[i] == '&') {
			expr->op = MATCH_EXPR_AND;
			if (!expr->left) {
				left_str = malloc(leftend - leftstart + 2);
				strncpy(left_str, &str[leftstart], leftend-leftstart+1);
				left_str[leftend - leftstart + 1] = '\0';
				expr->left = parse_match_expression(left_str);
				free(left_str);
			}
			expr->right = parse_match_expression(&str[i+1]);
			if (expr->left && expr->right) {
				result = expr;
				goto done;
			} else {
				result = NULL;
				goto done;
			}
		} else if (str[i] == '|') {
			expr->op = MATCH_EXPR_OR;
			if (!expr->left) {
				left_str = malloc(leftend - leftstart + 2);
				strncpy(left_str, &str[leftstart], leftend-leftstart+1);
				left_str[leftend - leftstart + 1] = '\0';
				expr->left = parse_match_expression(left_str);
				free(left_str);
			}
			expr->right = parse_match_expression(&str[i+1]);
			expr->count = 0;
			if (expr->left && expr->right) {
				result = expr;
				goto done;
			} else {
				result = NULL;
				goto done;
			}
		} else if (str[i] == '(') {
			leftstart = i + 1;
			j = 1;
			while (j > 0) {
				i++;
				if (i > strlen(str)) {
					printf("parse error: no closing bracket: %s\n", str);
					printf("                                 ");
					for (j = 0; j < leftstart - 1; j++) {
						printf(" ");	
					}
					printf("^\n");
					result = NULL;
					goto done;
				}
				if (str[i] == ')') {
					j--;
				} else if (str[i] == '(') {
					j++;
				} else {
				}
			}
			leftend = i-1;
			left_str = malloc(leftend - leftstart + 1);
			strncpy(left_str, &str[leftstart], leftend - leftstart + 1);
			expr->left = parse_match_expression(left_str);
			free(left_str);
			if (i >= strlen(str)-1) {
				result = expr->left;
				free_match_expression(expr);
				goto done;
			}
		} else if (str[i] == ')') {
			printf("parse error: ) without (\n");
			result = NULL;
			goto done;
		} else {
			leftend = i;
		}
	}
	
	/* no operators or hooks left, expr should be of the form
	   <name><operator><value> now */
	for (i = 0; i < strlen(str); i++) {
		if (str[i] == '=' ||
		    str[i] == '>' ||
		    str[i] == '<' ||
		    str[i] == '!' ||
		    str[i] == '~'
		   ) {
		 	leftend = i-1;
			op = malloc(3);
			j = 0;
			op[j] = str[i];
			i++;
			j++;
			
			if (i > strlen(str)) {
				printf("parse error no right hand side: %s\n", str);
				result = NULL;
				goto done;
			}
			if (str[i] == '=' ||
			    str[i] == '>' ||
			    str[i] == '<' ||
			    str[i] == '!' ||
			    str[i] == '~'
			   ) {
			   	op[j] = str[i];
				i++;
			   	j++;
				if (i > strlen(str)) {
					printf("parse error no right hand side: %s\n", str);
					result = NULL;
					if (op)
						free(op);
					goto done;
				}
			}
			op[j] = '\0';
			left_str = malloc(leftend - leftstart + 2);
			strncpy(left_str, &str[leftstart], leftend - leftstart + 1);
			left_str[leftend - leftstart + 1] = '\0';
			mt = get_match_by_name(left_str);
			if (!mt) {
				printf("parse error: unknown match name: %s\n", left_str);
				if (op)
					free(op);
				result = NULL;
				goto done;
			} else {
				/* check if operator is allowed */
				tos = get_type_operators(mt->type);
				for (j = 0; j < tos->operator_count; j++) {
					if (get_op_id(op) == tos->operators[j]) {
						if (mo)
							free(mo);
						mo = malloc(sizeof(match_operation));
						mo->id = mt->id;
						mo->operator = get_op_id(op);
						switch (mt->type) {
							case TYPE_BOOL:
								val = malloc(2);
								if (strncmp(&str[i], "true", 5) == 0 ||
								    strncmp(&str[i], "TRUE", 5) == 0 ||
								    strncmp(&str[i], "True", 5) == 0 ||
								    strncmp(&str[i], "1", 2) == 0
								) {
									val[0] = '1';
									val[1] = '\0';
								} else if (strncmp(&str[i], "false", 5) == 0 ||
								    strncmp(&str[i], "FALSE", 5) == 0 ||
								    strncmp(&str[i], "False", 5) == 0 ||
								    strncmp(&str[i], "0", 2) == 0
								) {

									val[0] = '0';
								} else {
									fprintf(stderr, "Bad value for bool: %s\n", &str[i]);
									exit(EXIT_FAILURE);
								}
								val[1] = '\0';
								break;
							case TYPE_RR:
								/* convert first so we have the same strings for the same rrs in match_ later */
								/*
								qrr = ldns_rr_new_frm_str(&str[i], LDNS_DEFAULT_TTL, NULL);
								if (!qrr) {
									fprintf(stderr, "Bad value for RR: %s\n", &str[i]);
									exit(EXIT_FAILURE);
								}
								val = ldns_rr2str(qrr);
								*/
								/* remove \n for readability */
								/*
								if (strchr(val, '\n')) {
									*(strchr(val, '\n')) = '\0';
								}
								ldns_rr_free(qrr);
								*/
								val = _strdup(&str[i]);
								break;
							case TYPE_OPCODE:
								lt = ldns_lookup_by_name(ldns_opcodes, &str[i]);
								if (lt) {
									val = malloc(4);
									snprintf(val, 3, "%u", (unsigned int) lt->id);
								} else {
									val = _strdup(&str[i]);
								}
								break;
							case TYPE_RCODE:
								lt = ldns_lookup_by_name(ldns_rcodes, &str[i]);
								if (lt) {
									val = malloc(4);
									snprintf(val, 3, "%u", (unsigned int) lt->id);
								} else {
									val = _strdup(&str[i]);
								}
								break;
							default:
								val = _strdup(&str[i]);
								break;
						}
						mo->value = val;
					}
				}
				if (!mo) {
					printf("parse error: operator %s not allowed for match %s\n", op, left_str);
					result = NULL;
					if (op)
						free(op);
					goto done;
				}
			}
			free(left_str);
			free(op);
			expr->match = mo;
			expr->op = MATCH_EXPR_LEAF;
			result = expr;
			goto done;
		}
	}

	result = NULL;
	
	done:
	free(str);
	if (!result) {
		free_match_expression(expr);
	}
	return result;
	
}
/* end of matches and counts */
void 
usage(FILE *output)
{
	fprintf(output, "Usage: ldns-dpa [OPTIONS] <pcap file>\n");
	fprintf(output, "Options:\n");
	fprintf(output, "\t-c <exprlist>:\tCount occurrences of matching expressions\n");
	fprintf(output, "\t-f <expression>:\tFilter occurrences of matching expressions\n");
	fprintf(output, "\t-h:\t\tshow this help\n");
	fprintf(output, "\t-p:\t\tshow percentage of -u and -c values (of the total of\n\t\t\tmatching on the -f filter. if no filter is given,\n\t\t\tpercentages are on all correct dns packets)\n");
	fprintf(output, "\t-of <file>:\tWrite pcap packets that match the -f flag to file\n");
	fprintf(output, "\t-ofh <file>:\tWrite pcap packets that match the -f flag to file\n\t\tin a hexadecimal format readable by drill\n");
	fprintf(output, "\t-s:\t\tshow possible match names\n");
	fprintf(output, "\t-s <matchname>:\tshow possible match operators and values for <name>\n");
	fprintf(output, "\t-sf:\t\tPrint packet that match -f. If no -f is given, print\n\t\t\tall dns packets\n");
	fprintf(output, "\t-u <matchnamelist>:\tCount all occurrences of matchname\n");
	fprintf(output, "\t-ua:\t\tShow average value of every -u matchname\n");
	fprintf(output, "\t-uac:\t\tShow average count of every -u matchname\n");
	fprintf(output, "\t-um <number>:\tOnly show -u results that occurred more than number times\n");
	fprintf(output, "\t-v <level>:\tbe more verbose\n");
	fprintf(output, "\t-notip <file>:\tDump pcap packets that were not recognized as\n\t\t\tIP packets to file\n");
	fprintf(output, "\t-baddns <file>:\tDump mangled dns packets to file\n");
	fprintf(output, "\t-version:\tShow the version and exit\n");
	fprintf(output, "\n");
	fprintf(output, "The filename '-' stands for stdin or stdout, so you can use \"-of -\" if you want to pipe the output to another process\n");
	fprintf(output, "\n");
	fprintf(output, "A <list> is a comma separated list of items\n");
	fprintf(output, "\n");
	fprintf(output, "An expression has the following form:\n");
	fprintf(output, "<expr>:\t(<expr>)\n");
	fprintf(output, "\t<expr> | <expr>\n");
	fprintf(output, "\t<expr> & <expr>\n");
	fprintf(output, "\t<match>\n");
	fprintf(output, "\n");
	fprintf(output, "<match>:\t<matchname> <operator> <value>\n");
	fprintf(output, "\n");
	fprintf(output, "See the -s option for possible matchnames, operators and values.\n");
}

void
show_match_names(char *name)
{
	size_t j;
	match_table *mt;
	ldns_lookup_table *lt;
	const type_operators *tos;
	char *str;
	size_t i;
	
	if (name) {
		mt = get_match_by_name(name);
		if (mt) {
			printf("%s:\n", mt->name);
			printf("\t%s.\n", mt->description);
			printf("\toperators: ");
			printf("\t");
			tos = get_type_operators(mt->type);
			if (tos)  {
				for (j = 0; j < tos->operator_count; j++) {
					printf("%s ", get_op_str(tos->operators[j]));
/*
					lt = ldns_lookup_by_id((ldns_lookup_table *) lt_operators, tos->operators[j]);
					if (lt) {
						printf("%s ", lt->name);
					} else {
						printf("? ");
					}
*/
				}
			} else {
				printf("unknown type");
			}
			
			printf("\n");
			printf("\tValues:\n");
			switch (mt->type) {
				case TYPE_INT:
					printf("\t\t<Integer>\n");
					break;
				case TYPE_BOOL:
					printf("\t\t0\n");
					printf("\t\t1\n");
					printf("\t\ttrue\n");
					printf("\t\tfalse\n");
					break;
				case TYPE_OPCODE:
					printf("\t\t<Integer>\n");
					lt = ldns_opcodes;
					while (lt->name != NULL) {
						printf("\t\t%s\n", lt->name);
						lt++;
					}
					break;
				case TYPE_RCODE:
					printf("\t\t<Integer>\n");
					lt = ldns_rcodes;
					while (lt->name != NULL) {
						printf("\t\t%s\n", lt->name);
						lt++;
					}
					break;
				case TYPE_STRING:
					printf("\t\t<String>\n");
					break;
				case TYPE_TIMESTAMP:
					printf("\t\t<Integer> (seconds since epoch)\n");
					break;
				case TYPE_ADDRESS:
					printf("\t\t<IP address>\n");
					break;
				case TYPE_RR:
					printf("\t\t<Resource Record>\n");
					break;
				default:
					break;
			}
		} else {
			printf("Unknown match name: %s\n", name);
		}
	} else {
		mt = (match_table *) matches;
		while (mt->name != NULL) {
			str = (char *) mt->name;
			printf("%s:", str);
			i = strlen(str) + 1;
			while (i < 24) {
				printf(" ");
				i++;
			}
			printf("%s\n", mt->description);
			mt++;
		}
	}
}

int
handle_ether_packet(const u_char *data, struct pcap_pkthdr cur_hdr, match_counters *count, match_expression *match_expr, match_counters *uniques, match_id unique_ids[], size_t unique_id_count)
{
	struct ether_header *eptr;
	struct ip *iptr;
	struct ip6_hdr *ip6_hdr;
	int ip_hdr_size;
	uint8_t protocol;
	size_t data_offset = 0;
	ldns_rdf *src_addr = NULL, *dst_addr = NULL;
	uint8_t *ap;
	char *astr;
	bpf_u_int32 len = cur_hdr.caplen;
	struct timeval timestamp;
	uint16_t ip_flags;
	uint16_t ip_len;
	uint16_t ip_id;
	uint16_t ip_f_offset;
	const u_char *newdata = NULL;
/*
printf("timeval: %u ; %u\n", cur_hdr.ts.tv_sec, cur_hdr.ts.tv_usec);
*/
	
	uint8_t *dnspkt;
	
	ldns_pkt *pkt;
	ldns_status status;
	
	/* lets start with the ether header... */
	eptr = (struct ether_header *) data;
	/* Do a couple of checks to see what packet type we have..*/
	if (ntohs (eptr->ether_type) == ETHERTYPE_IP)
	{
		if (verbosity >= 5) {
			printf("Ethernet type hex:%x dec:%u is an IP packet\n",
				(unsigned int) ntohs(eptr->ether_type),
				(unsigned int) ntohs(eptr->ether_type));
		}

		data_offset = ETHER_HEADER_LENGTH;
		iptr = (struct ip *) (data + data_offset);
		/*
		printf("IP_OFF: %u (%04x) %04x %04x (%d) (%d)\n", iptr->ip_off, iptr->ip_off, IP_MF, IP_DF, iptr->ip_off & 0x4000, iptr->ip_off & 0x2000);
		*/
		ip_flags = ldns_read_uint16(&(iptr->ip_off));
		ip_id = ldns_read_uint16(&(iptr->ip_id));
		ip_len = ldns_read_uint16(&(iptr->ip_len));
		ip_f_offset = (ip_flags & IP_OFFMASK)*8;
		if (ip_flags & IP_MF && ip_f_offset == 0) {
			/*printf("First Frag id %u len\n", ip_id, ip_len);*/
			fragment_p->ip_id = ip_id;
			memset(fragment_p->data, 0, 65535);
			memcpy(fragment_p->data, iptr, ip_len);
			fragment_p->cur_len = ip_len + 20;
/*
				for (ip_len = 0; ip_len < fragment_p->cur_len; ip_len++) {
					if (ip_len > 0 && ip_len % 20 == 0) {
						printf("\t; %u - %u\n", ip_len - 19, ip_len);
					}
					printf("%02x ", fragment_p->data[ip_len]);
				}
				printf("\t; ??? - %u\n", ip_len);
*/
			return 0;
		} else
		if (ip_flags & IP_MF && ip_f_offset != 0) {
			/*printf("Next frag\n");*/
			if (ip_id == fragment_p->ip_id) {
				/*printf("add fragment to current id %u len %u offset %u\n", ip_id, ip_len, ip_f_offset);*/
				memcpy(fragment_p->data + (ip_f_offset) + 20, data+data_offset+20, ip_len - (iptr->ip_hl)*4);
				/*printf("COPIED %u\n", ip_len);*/
				fragment_p->cur_len = fragment_p->cur_len + ip_len - 20;
				/*printf("cur len now %u\n", fragment_p->cur_len);*/
/*
				for (ip_len = 0; ip_len < fragment_p->cur_len; ip_len++) {
					if (ip_len > 0 && ip_len % 20 == 0) {
						printf("\t; %u - %u\n", ip_len - 19, ip_len);
					}
					printf("%02x ", fragment_p->data[ip_len]);
				}
				printf("\t; ??? - %u\n", ip_len);
*/
				return 0;
			} else {
				/*printf("Lost fragment %u\n", iptr->ip_id);*/
				lost_packet_fragments++;
				return 1;
			}
		} else
		if (!(ip_flags & IP_MF) && ip_f_offset != 0) {
			/*printf("Last frag\n");*/
			if (ip_id == fragment_p->ip_id) {
				/*printf("add fragment to current id %u len %u offset %u\n", ip_id, ip_len, ip_f_offset);*/
				memcpy(fragment_p->data + ip_f_offset + 20, data+data_offset+20, ip_len - 20);
				fragment_p->cur_len = fragment_p->cur_len + ip_len - 20;
				iptr = (struct ip *) fragment_p->data;
				newdata = malloc(fragment_p->cur_len + data_offset);
				if (!newdata) {
					printf("Malloc failed, out of mem?\n");
					exit(4);
				}
				memcpy((char *) newdata, data, data_offset);
				memcpy((char *) newdata+data_offset, fragment_p->data, fragment_p->cur_len);
				iptr->ip_len = (u_short) ldns_read_uint16(&(fragment_p->cur_len));
				iptr->ip_off = 0;
				len = (bpf_u_int32) fragment_p->cur_len;
				cur_hdr.caplen = len;
				fragment_p->ip_id = 0;
				fragmented_packets++;
/*
				for (ip_len = 0; ip_len < fragment_p->cur_len; ip_len++) {
					if (ip_len > 0 && ip_len % 20 == 0) {
						printf("\t; %u - %u\n", ip_len - 19, ip_len);
					}
					printf("%02x ", fragment_p->data[ip_len]);
				}
				printf("\t; ??? - %u\n", ip_len);
*/
			} else {
				/*printf("Lost fragment %u\n", iptr->ip_id);*/
				lost_packet_fragments++;
				return 1;
			}
		} else {
			newdata = data;
		}
/*
		if (iptr->ip_off & 0x0040) {
			printf("Don't fragment\n");
		}
*/

			/* in_addr portability woes, going manual for now */
			/* ipv4 */
			ap = (uint8_t *) &(iptr->ip_src);
			astr = malloc(INET_ADDRSTRLEN);
			if (inet_ntop(AF_INET, ap, astr, INET_ADDRSTRLEN)) {
				if (ldns_str2rdf_a(&src_addr, astr) == LDNS_STATUS_OK) {
					
				}
			}
			free(astr);
			ap = (uint8_t *) &(iptr->ip_dst);
			astr = malloc(INET_ADDRSTRLEN);
			if (inet_ntop(AF_INET, ap, astr, INET_ADDRSTRLEN)) {
				if (ldns_str2rdf_a(&dst_addr, astr) == LDNS_STATUS_OK) {
					
				}
			}
			free(astr);

			ip_hdr_size = (int) iptr->ip_hl * 4;
			protocol = (uint8_t) iptr->ip_p;
			
			data_offset += ip_hdr_size;

			if (protocol == IPPROTO_UDP) {
				udp_packets++;
				data_offset += UDP_HEADER_LENGTH;
				
				dnspkt = (uint8_t *) (newdata + data_offset);

				/*printf("packet starts at byte %u\n", data_offset);*/

				/*printf("Len: %u\n", len);*/

				status = ldns_wire2pkt(&pkt, dnspkt, len - data_offset);

				if (status != LDNS_STATUS_OK) {
					if (verbosity >= 3) {
						printf("No dns packet: %s\n", ldns_get_errorstr_by_id(status));
					}
					if (verbosity >= 5) {
						for (ip_len = 0; ip_len < len - data_offset; ip_len++) {
							if (ip_len > 0 && ip_len % 20 == 0) {
								printf("\t; %u - %u\n", (unsigned int) ip_len - 19, (unsigned int) ip_len);
							}
							printf("%02x ", (unsigned int) dnspkt[ip_len]);
						}
						printf("\t; ??? - %u\n", (unsigned int) ip_len);
						
					}
					bad_dns_packets++;
					if (bad_dns_dump) {
						pcap_dump((u_char *)bad_dns_dump, &cur_hdr, newdata);
					}
				} else {
					timestamp.tv_sec = cur_hdr.ts.tv_sec;
					timestamp.tv_usec = cur_hdr.ts.tv_usec;
					ldns_pkt_set_timestamp(pkt, timestamp);
				
					if (verbosity >= 4) {
						printf("DNS packet\n");
						ldns_pkt_print(stdout, pkt);
						printf("\n\n");
					}

					total_nr_of_dns_packets++;

					if (match_expr) {
						if (match_dns_packet_to_expr(pkt, src_addr, dst_addr, match_expr)) {
							/* if outputfile write */
							if (dumper) {
								pcap_dump((u_char *)dumper, &cur_hdr, data);
							}
							if (hexdumpfile) {
								fprintf(hexdumpfile, ";; %u\n", (unsigned int) total_nr_of_dns_packets);
								ldns_pkt2file_hex(hexdumpfile, pkt);
							}
							if (show_filter_matches) {
								printf(";; From: ");
								ldns_rdf_print(stdout, src_addr);
								printf("\n");
								printf(";; To:   ");
								ldns_rdf_print(stdout, dst_addr);
								printf("\n");
								ldns_pkt_print(stdout, pkt);
								printf("------------------------------------------------------------\n\n");
							}
						} else {
							ldns_pkt_free(pkt);
							ldns_rdf_deep_free(src_addr);
							ldns_rdf_deep_free(dst_addr);
							if (newdata && newdata != data)
								free((void *)newdata);
							return 0;
						}
					} else {
						if (dumper) {
							pcap_dump((u_char *)dumper, &cur_hdr, data);
						}
						if (hexdumpfile) {
							fprintf(hexdumpfile, ";; %u\n", (unsigned int) total_nr_of_dns_packets);
							ldns_pkt2file_hex(hexdumpfile, pkt);
						}
						if (show_filter_matches) {
							printf(";; From: ");
							ldns_rdf_print(stdout, src_addr);
							printf("\n");
							printf(";; To:   ");
							ldns_rdf_print(stdout, dst_addr);
							printf("\n");
							ldns_pkt_print(stdout, pkt);
							printf("------------------------------------------------------------\n\n");
						}
					}

					/* General counters here */
					total_nr_of_filtered_packets++;

					match_pkt_counters(pkt, src_addr, dst_addr, count);
					match_pkt_uniques(pkt, src_addr, dst_addr, uniques, unique_ids, unique_id_count);

					ldns_pkt_free(pkt);
					pkt = NULL;
				}
				ldns_rdf_deep_free(src_addr);
				ldns_rdf_deep_free(dst_addr);

			} else if (protocol == IPPROTO_TCP) {
				/* tcp packets are skipped */
				tcp_packets++;
			}
			if (newdata && newdata != data) {
				free((void *)newdata);
				newdata = NULL;
			}
	/* don't have a define for ethertype ipv6 */
	} else 	if (ntohs (eptr->ether_type) == ETHERTYPE_IPV6) {
		/*printf("IPv6!\n");*/


		/* copied from ipv4, move this to function? */

		data_offset = ETHER_HEADER_LENGTH;
		ip6_hdr = (struct ip6_hdr *) (data + data_offset);

		newdata = data;
		
		/* in_addr portability woes, going manual for now */
		/* ipv6 */
		ap = (uint8_t *) &(ip6_hdr->ip6_src);
		astr = malloc(INET6_ADDRSTRLEN);
		if (inet_ntop(AF_INET6, ap, astr, INET6_ADDRSTRLEN)) {
			if (ldns_str2rdf_aaaa(&src_addr, astr) == LDNS_STATUS_OK) {
				
			}
		}
		free(astr);
		ap = (uint8_t *) &(ip6_hdr->ip6_dst);
		astr = malloc(INET6_ADDRSTRLEN);
		if (inet_ntop(AF_INET6, ap, astr, INET6_ADDRSTRLEN)) {
			if (ldns_str2rdf_aaaa(&dst_addr, astr) == LDNS_STATUS_OK) {
				
			}
		}
		free(astr);

		ip_hdr_size = IP6_HEADER_LENGTH;
		protocol = (uint8_t) ip6_hdr->ip6_ctlun.ip6_un1.ip6_un1_nxt;
		
		data_offset += ip_hdr_size;

		if (protocol == IPPROTO_UDP) {
			udp_packets++;
			/*printf("V6 UDP!\n");*/
			data_offset += UDP_HEADER_LENGTH;
			
			dnspkt = (uint8_t *) (newdata + data_offset);

			/*printf("Len: %u\n", len);*/

			status = ldns_wire2pkt(&pkt, dnspkt, len - data_offset);

			if (status != LDNS_STATUS_OK) {
				if (verbosity >= 3) {
					printf("No dns packet: %s\n", ldns_get_errorstr_by_id(status));
				}
				bad_dns_packets++;
				if (bad_dns_dump) {
					pcap_dump((u_char *)bad_dns_dump, &cur_hdr, newdata);
				}
			} else {
				timestamp.tv_sec = cur_hdr.ts.tv_sec;
				timestamp.tv_usec = cur_hdr.ts.tv_usec;
				ldns_pkt_set_timestamp(pkt, timestamp);
			
				if (verbosity >= 4) {
					printf("DNS packet\n");
					ldns_pkt_print(stdout, pkt);
					printf("\n\n");
				}

				total_nr_of_dns_packets++;

				if (match_expr) {
					if (match_dns_packet_to_expr(pkt, src_addr, dst_addr, match_expr)) {
						/* if outputfile write */
						if (dumper) {
							pcap_dump((u_char *)dumper, &cur_hdr, data);
						}
						if (show_filter_matches) {
							printf(";; From: ");
							ldns_rdf_print(stdout, src_addr);
							printf("\n");
							printf(";; To:   ");
							ldns_rdf_print(stdout, dst_addr);
							printf("\n");
							ldns_pkt_print(stdout, pkt);
							printf("------------------------------------------------------------\n\n");
						}
					} else {
						ldns_pkt_free(pkt);
						ldns_rdf_deep_free(src_addr);
						ldns_rdf_deep_free(dst_addr);
						return 0;
					}
				} else {
					if (show_filter_matches) {
						printf(";; From: ");
						ldns_rdf_print(stdout, src_addr);
						printf("\n");
						printf(";; To:   ");
						ldns_rdf_print(stdout, dst_addr);
						printf("\n");
						ldns_pkt_print(stdout, pkt);
						printf("------------------------------------------------------------\n\n");
					}
				}

				/* General counters here */
				total_nr_of_filtered_packets++;

				match_pkt_counters(pkt, src_addr, dst_addr, count);
				match_pkt_uniques(pkt, src_addr, dst_addr, uniques, unique_ids, unique_id_count);

				ldns_pkt_free(pkt);
				pkt = NULL;
			}
			ldns_rdf_deep_free(src_addr);
			ldns_rdf_deep_free(dst_addr);

		} else if (protocol == IPPROTO_TCP) {
			/* tcp packets are skipped */
			tcp_packets++;
		} else {
			printf("ipv6 unknown next header type: %u\n", (unsigned int) protocol);
		}



	} else  if (ntohs (eptr->ether_type) == ETHERTYPE_ARP) {
		if (verbosity >= 5) {
			printf("Ethernet type hex:%x dec:%u is an ARP packet\n",
				(unsigned int) ntohs(eptr->ether_type),
				(unsigned int) ntohs(eptr->ether_type));
		}
		arp_packets++;
	} else {
		printf("Ethernet type %x not IP\n", (unsigned int) ntohs(eptr->ether_type));
		if (verbosity >= 5) {
			printf("Ethernet type %x not IP\n", (unsigned int) ntohs(eptr->ether_type));
		}
		not_ip_packets++;
		if (not_ip_dump) {
			pcap_dump((u_char *)not_ip_dump, &cur_hdr, data);
		}
	}

	return 0;
}

bool
parse_match_list(match_counters *counters, char *string)
{
	size_t i;
	match_expression *expr;
/*	match_counter *mc;*/
	size_t lastpos = 0;
	char *substring;

	/*printf("Parsing match list: '%s'\n", string);*/

	for (i = 0; i < strlen(string); i++) {
		if (string[i] == ',') {
			if (i<2) {
				fprintf(stderr, "Matchlist cannot start with ,\n");
				return false;
			} else {
				substring = malloc(strlen(string)+1);
				strncpy(substring, &string[lastpos], i - lastpos + 1);
				substring[i - lastpos] = '\0';
				expr = parse_match_expression(substring);
				free(substring);
				if (!expr) {
					return false;
				}
				/*
				if (expr->op != MATCH_EXPR_LEAF) {
					fprintf(stderr, "Matchlist can only contain <match>, not a logic expression\n");
					return false;
				}
				*/
				add_match_counter(counters, expr, false);
				lastpos = i+1;
			}
		}
	}
	substring = malloc(strlen(string) + 1);
	strncpy(substring, &string[lastpos], i - lastpos + 1);
	substring[i - lastpos] = '\0';
	expr = parse_match_expression(substring);

	if (!expr) {
		fprintf(stderr, "Bad match: %s\n", substring);
		free(substring);
		return false;
	}
	free(substring);
	/*
	if (expr->op != MATCH_EXPR_LEAF) {
		fprintf(stderr, "Matchlist can only contain <match>, not a logic expression\n");
		return false;
	}
	*/
	add_match_counter(counters, expr, false);
	return true;
}

bool
parse_uniques(match_id ids[], size_t *count, char *string)
{
	size_t i, j, lastpos;
	char *str, *strpart;
	match_table *mt;

	/*printf("Parsing unique counts: '%s'\n", string);*/
	str = calloc(1, strlen(string) + 1);
	j = 0;
	for (i = 0; i < strlen(string); i++) {
		if (!isspace((unsigned char)string[i])) {
			str[j] = string[i];
			j++;
		}
	}
	str[j] = '\0';

	lastpos = 0;
	for (i = 0; i <= strlen(str); i++) {
		if (str[i] == ',' || i >= strlen(str)) {
			if (!(strpart = malloc(i - lastpos + 1))) {
				free(str);
				return false;
			}
			strncpy(strpart, &str[lastpos], i - lastpos);
			strpart[i - lastpos] = '\0';
			if ((mt = get_match_by_name(strpart))) {
				ids[*count] = mt->id;
				*count = *count + 1;
			} else {
				printf("Error parsing match list; unknown match name: %s\n", strpart);
				free(strpart);
				free(str);
				return false;
			}
			free(strpart);
			lastpos = i + 1;
		}
	}
	if (i > lastpos) {
		strpart = malloc(i - lastpos + 1);
		strncpy(strpart, &str[lastpos], i - lastpos);
		strpart[i - lastpos] = '\0';
		if ((mt = get_match_by_name(strpart))) {
			ids[*count] = mt->id;
			*count = *count + 1;
		} else {
			printf("Error parsing match list; unknown match name: %s\n", strpart);
			return false;
		}
		free(strpart);
	}
	free(str);
	return true;
}

int main(int argc, char *argv[]) {

	int i;
	int status = EXIT_SUCCESS;
	match_counters *count = malloc(sizeof(match_counters));
	const char *inputfile = NULL;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *pc = NULL;
	const u_char *cur;
	struct pcap_pkthdr cur_hdr;
	match_expression *expr = NULL;
	match_id unique_ids[MAX_MATCHES];
	size_t unique_id_count = 0; /* number of unique counters */
	match_counters *uniques = malloc(sizeof(match_counters));
	char *dumpfile = NULL;
	char *hexdumpfilename = NULL;
	char *not_ip_dumpfile = NULL;
	char *bad_dns_dumpfile = NULL;

	bool show_percentages = false;
	bool show_averages = false;
	bool show_average_count = false;
	int unique_minimum = 0;

	count->left = NULL;
	count->match = NULL;
	count->right = NULL;
	uniques->left = NULL;
	uniques->match = NULL;
	uniques->right = NULL;

	fragment_p = malloc(sizeof(struct fragment_part));
	fragment_p->ip_id = 0;
	fragment_p->cur_len = 0;

	for (i = 1; i < argc; i++) {

		if (strncmp(argv[i], "-baddns", 8) == 0) {
			if (i + 1 < argc) {
				bad_dns_dumpfile = argv[i + 1];
				i++;
			} else {
				usage(stderr);
				status = EXIT_FAILURE;
				goto exit;
			}
		} else if (strncmp(argv[i], "-notip", 7) == 0) {
			if (i + 1 < argc) {
				not_ip_dumpfile = argv[i + 1];
				i++;
			} else {
				usage(stderr);
				status = EXIT_FAILURE;
				goto exit;
			}
		} else if (strncmp(argv[i], "-c", 3) == 0) {
			if (i + 1 < argc) {
				if (!parse_match_list(count, argv[i + 1])) {
					status = EXIT_FAILURE;
					goto exit;
				}
				i++;
			} else {
				usage(stderr);
				status = EXIT_FAILURE;
				goto exit;
			}
		} else 	if (strncmp(argv[i], "-f", 3) == 0) {
			if (i + 1 < argc) {
				if (expr || strchr(argv[i+1], ',')) {
					fprintf(stderr, "You can only specify 1 filter expression.\n");
					status = EXIT_FAILURE;
					goto exit;
				}
				expr = parse_match_expression(argv[i + 1]);
				i++;
			} else {
				usage(stderr);
				status = EXIT_FAILURE;
				goto exit;
			}
		} else if (strncmp(argv[i], "-h", 3) == 0) {
			usage(stdout);
			status = EXIT_SUCCESS;
			goto exit;
		} else if (strncmp(argv[i], "-p", 3) == 0) {
			show_percentages = true;
		} else if (strncmp(argv[i], "-of", 4) == 0) {
			if (i + 1 < argc) {
				dumpfile = argv[i + 1];
				i++;
			} else {
				usage(stderr);
				status = EXIT_FAILURE;
				goto exit;
			}
		} else if (strncmp(argv[i], "-ofh", 5) == 0) {
			if (i + 1 < argc) {
				hexdumpfilename = argv[i + 1];
				i++;
			} else {
				usage(stderr);
				status = EXIT_FAILURE;
				goto exit;
			}
		} else if (strncmp(argv[i], "-s", 3) == 0) {
			if (i + 1 < argc) {
				show_match_names(argv[i + 1]);
			} else {
				show_match_names(NULL);
			}
			status = EXIT_SUCCESS;
			goto exit;
		} else if (strncmp(argv[i], "-sf", 4) == 0) {
			show_filter_matches = true;
		} else if (strncmp(argv[i], "-u", 3) == 0) {
			if (i + 1 < argc) {
				if (!parse_uniques(unique_ids, &unique_id_count, argv[i + 1])) {
					status = EXIT_FAILURE;
					goto exit;
				}
				i++;
			} else {
				usage(stderr);
				status = EXIT_FAILURE;
				goto exit;
			}
		} else if (strcmp("-ua", argv[i]) == 0) {
			show_averages = true;
		} else if (strcmp("-uac", argv[i]) == 0) {
			show_average_count = true;
		} else if (strcmp("-um", argv[i]) == 0) {
			if (i + 1 < argc) {
				unique_minimum = atoi(argv[i+1]);
				i++;
			} else {
				fprintf(stderr, "-um requires an argument");
				usage(stderr);
				status = EXIT_FAILURE;
				goto exit;
			}
		} else if (strcmp("-v", argv[i]) == 0) {
			i++;
			if (i < argc) {
				verbosity = atoi(argv[i]);
			}
		} else if (strcmp("-version", argv[i]) == 0) {
			printf("dns packet analyzer, version %s (ldns version %s)\n", LDNS_VERSION, ldns_version());
			goto exit;
		} else {
			if (inputfile) {
				fprintf(stderr, "You can only specify 1 input file\n");
				exit(1);
			}
			inputfile = argv[i];
		}
	}

	if (!inputfile) {
		inputfile = "-";
	}

	if (verbosity >= 5) {
		printf("Filter:\n");
		print_match_expression(stdout, expr);
		printf("\n\n");
	}

	pc = pcap_open_offline(inputfile, errbuf);
	
	if (!pc) {
		if (errno != 0) {
			printf("Error opening pcap file %s: %s\n", inputfile, errbuf);
			exit(1);
		} else {
			goto showresult;
		}
	}

	if (dumpfile) {
	        dumper = pcap_dump_open(pc, dumpfile);

		if (!dumper) {
			printf("Error opening pcap dump file %s: %s\n", dumpfile, errbuf);
			exit(1);
		}
	}

	if (hexdumpfilename) {
		if (strncmp(hexdumpfilename, "-", 2) != 0) {
			printf("hexdump is file\n");
		        hexdumpfile = fopen(hexdumpfilename, "w");
		} else {
			printf("hexdump is stdout\n");
			hexdumpfile = stdout;
		}

		if (!hexdumpfile) {
			printf("Error opening hex dump file %s: %s\n", hexdumpfilename, strerror(errno));
			exit(1);
		}
	}

	if (not_ip_dumpfile) {
		not_ip_dump = pcap_dump_open(pc, not_ip_dumpfile);
		if (!not_ip_dump) {
			printf("Error opening pcap dump file NOT_IP: %s\n",  errbuf);
		}
	}
	if (bad_dns_dumpfile) {
		bad_dns_dump = pcap_dump_open(pc, bad_dns_dumpfile);
		if (!bad_dns_dump) {
			printf("Error opening pcap dump file NOT_IP: %s\n",  errbuf);
		}
	}
	
	while ((cur = pcap_next(pc, &cur_hdr))) {
		if (verbosity >= 5) {
			printf("\n\n\n[PKT_HDR] caplen: %u \tlen: %u\n", (unsigned int)cur_hdr.caplen, (unsigned int)cur_hdr.len);
		}
		handle_ether_packet(cur, cur_hdr, count, expr, uniques, unique_ids, unique_id_count);
	}

	if (not_ip_dump) {
		pcap_dump_close(not_ip_dump);
	}

	if (bad_dns_dump) {
		pcap_dump_close(bad_dns_dump);
	}

	if (dumper) {
		pcap_dump_close(dumper);
	}
	
	if (hexdumpfile && hexdumpfile != stdout) {
		fclose(hexdumpfile);
	}

	pcap_close(pc);
	
	showresult:
	if (show_percentages) {
		fprintf(stdout, "Packets that are not IP: %u\n", (unsigned int) not_ip_packets);
		fprintf(stdout, "bad dns packets: %u\n", (unsigned int) bad_dns_packets);
		fprintf(stdout, "arp packets: %u\n", (unsigned int) arp_packets);
		fprintf(stdout, "udp packets: %u\n", (unsigned int) udp_packets);
		fprintf(stdout, "tcp packets (skipped): %u\n", (unsigned int) tcp_packets);
		fprintf(stdout, "reassembled fragmented packets: %u\n", (unsigned int) fragmented_packets);
		fprintf(stdout, "packet fragments lost: %u\n", (unsigned int) lost_packet_fragments);
		fprintf(stdout, "Total number of DNS packets: %u\n", (unsigned int) total_nr_of_dns_packets);
		fprintf(stdout, "Total number of DNS packets after filter: %u\n", (unsigned int) total_nr_of_filtered_packets);
	}
	if (count->match) {
		print_counters(stdout, count, show_percentages, total_nr_of_filtered_packets, 0);
	}
	if (uniques->match) {
		print_counters(stdout, uniques, show_percentages, total_nr_of_filtered_packets, unique_minimum);
		if (show_averages) {
			print_counter_averages(stdout, uniques, NULL);
		}
		if (show_average_count) {
			print_counter_average_count(stdout, uniques, NULL, true);
		}
	}

	exit:

	free_match_expression(expr);
	free_counters(count);
	free_counters(uniques);

	return status;
}

#else
int main(void) {
	fprintf(stderr, "ldns-dpa was not built because there is no pcap library on this system, or there was no pcap header file at compilation time. Please install pcap and rebuild.\n");
	return 1;
}
#endif
#else
int main(void) {
	fprintf(stderr, "ldns-dpa was not built because there is no pcap library on this system, or there was no pcap header file at compilation time. Please install pcap and rebuild.\n");
	return 1;
}
#endif


