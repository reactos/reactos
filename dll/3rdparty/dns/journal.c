/*
 * Copyright (C) 2004, 2005, 2007-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2002  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: journal.c,v 1.103.48.2 2009/01/18 23:47:37 tbox Exp $ */

#include <config.h>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <isc/file.h>
#include <isc/mem.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/compress.h>
#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/diff.h>
#include <dns/fixedname.h>
#include <dns/journal.h>
#include <dns/log.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/result.h>
#include <dns/soa.h>

/*! \file
 * \brief Journaling.
 *
 * A journal file consists of
 *
 *   \li A fixed-size header of type journal_rawheader_t.
 *
 *   \li The index.  This is an unordered array of index entries
 *     of type journal_rawpos_t giving the locations
 *     of some arbitrary subset of the journal's addressable
 *     transactions.  The index entries are used as hints to
 *     speed up the process of locating a transaction with a given
 *     serial number.  Unused index entries have an "offset"
 *     field of zero.  The size of the index can vary between
 *     journal files, but does not change during the lifetime
 *     of a file.  The size can be zero.
 *
 *   \li The journal data.  This  consists of one or more transactions.
 *     Each transaction begins with a transaction header of type
 *     journal_rawxhdr_t.  The transaction header is followed by a
 *     sequence of RRs, similar in structure to an IXFR difference
 *     sequence (RFC1995).  That is, the pre-transaction SOA,
 *     zero or more other deleted RRs, the post-transaction SOA,
 *     and zero or more other added RRs.  Unlike in IXFR, each RR
 *     is prefixed with a 32-bit length.
 *
 *     The journal data part grows as new transactions are
 *     appended to the file.  Only those transactions
 *     whose serial number is current-(2^31-1) to current
 *     are considered "addressable" and may be pointed
 *     to from the header or index.  They may be preceded
 *     by old transactions that are no longer addressable,
 *     and they may be followed by transactions that were
 *     appended to the journal but never committed by updating
 *     the "end" position in the header.  The latter will
 *     be overwritten when new transactions are added.
 */
/*%
 * When true, accept IXFR difference sequences where the
 * SOA serial number does not change (BIND 8 sends such
 * sequences).
 */
static isc_boolean_t bind8_compat = ISC_TRUE; /* XXX config */

/**************************************************************************/
/*
 * Miscellaneous utilities.
 */

#define JOURNAL_COMMON_LOGARGS \
	dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_JOURNAL

#define JOURNAL_DEBUG_LOGARGS(n) \
	JOURNAL_COMMON_LOGARGS, ISC_LOG_DEBUG(n)

/*%
 * It would be non-sensical (or at least obtuse) to use FAIL() with an
 * ISC_R_SUCCESS code, but the test is there to keep the Solaris compiler
 * from complaining about "end-of-loop code not reached".
 */
#define FAIL(code) \
	do { result = (code);					\
		if (result != ISC_R_SUCCESS) goto failure;	\
	} while (0)

#define CHECK(op) \
	do { result = (op); 					\
		if (result != ISC_R_SUCCESS) goto failure; 	\
	} while (0)

static isc_result_t index_to_disk(dns_journal_t *);

static inline isc_uint32_t
decode_uint32(unsigned char *p) {
	return ((p[0] << 24) +
		(p[1] << 16) +
		(p[2] <<  8) +
		(p[3] <<  0));
}

static inline void
encode_uint32(isc_uint32_t val, unsigned char *p) {
	p[0] = (isc_uint8_t)(val >> 24);
	p[1] = (isc_uint8_t)(val >> 16);
	p[2] = (isc_uint8_t)(val >>  8);
	p[3] = (isc_uint8_t)(val >>  0);
}

isc_result_t
dns_db_createsoatuple(dns_db_t *db, dns_dbversion_t *ver, isc_mem_t *mctx,
		      dns_diffop_t op, dns_difftuple_t **tp)
{
	isc_result_t result;
	dns_dbnode_t *node;
	dns_rdataset_t rdataset;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_name_t *zonename;

	zonename = dns_db_origin(db);

	node = NULL;
	result = dns_db_findnode(db, zonename, ISC_FALSE, &node);
	if (result != ISC_R_SUCCESS)
		goto nonode;

	dns_rdataset_init(&rdataset);
	result = dns_db_findrdataset(db, node, ver, dns_rdatatype_soa, 0,
				     (isc_stdtime_t)0, &rdataset, NULL);
	if (result != ISC_R_SUCCESS)
		goto freenode;

	result = dns_rdataset_first(&rdataset);
	if (result != ISC_R_SUCCESS)
		goto freenode;

	dns_rdataset_current(&rdataset, &rdata);

	result = dns_difftuple_create(mctx, op, zonename, rdataset.ttl,
				      &rdata, tp);

	dns_rdataset_disassociate(&rdataset);
	dns_db_detachnode(db, &node);
	return (ISC_R_SUCCESS);

 freenode:
	dns_db_detachnode(db, &node);
 nonode:
	UNEXPECTED_ERROR(__FILE__, __LINE__, "missing SOA");
	return (result);
}

/* Journaling */

/*%
 * On-disk representation of a "pointer" to a journal entry.
 * These are used in the journal header to locate the beginning
 * and end of the journal, and in the journal index to locate
 * other transactions.
 */
typedef struct {
	unsigned char	serial[4];  /*%< SOA serial before update. */
	/*
	 * XXXRTH  Should offset be 8 bytes?
	 * XXXDCL ... probably, since isc_offset_t is 8 bytes on many OSs.
	 * XXXAG  ... but we will not be able to seek >2G anyway on many
	 *            platforms as long as we are using fseek() rather
	 *            than lseek().
	 */
	unsigned char	offset[4];  /*%< Offset from beginning of file. */
} journal_rawpos_t;


/*%
 * The header is of a fixed size, with some spare room for future
 * extensions.
 */
#define JOURNAL_HEADER_SIZE 64 /* Bytes. */

/*%
 * The on-disk representation of the journal header.
 * All numbers are stored in big-endian order.
 */
typedef union {
	struct {
		/*% File format version ID. */
		unsigned char 		format[16];
		/*% Position of the first addressable transaction */
		journal_rawpos_t 	begin;
		/*% Position of the next (yet nonexistent) transaction. */
		journal_rawpos_t 	end;
		/*% Number of index entries following the header. */
		unsigned char 		index_size[4];
	} h;
	/* Pad the header to a fixed size. */
	unsigned char pad[JOURNAL_HEADER_SIZE];
} journal_rawheader_t;

/*%
 * The on-disk representation of the transaction header.
 * There is one of these at the beginning of each transaction.
 */
typedef struct {
	unsigned char	size[4]; 	/*%< In bytes, excluding header. */
	unsigned char	serial0[4];	/*%< SOA serial before update. */
	unsigned char	serial1[4];	/*%< SOA serial after update. */
} journal_rawxhdr_t;

/*%
 * The on-disk representation of the RR header.
 * There is one of these at the beginning of each RR.
 */
typedef struct {
	unsigned char	size[4]; 	/*%< In bytes, excluding header. */
} journal_rawrrhdr_t;

/*%
 * The in-core representation of the journal header.
 */
typedef struct {
	isc_uint32_t	serial;
	isc_offset_t	offset;
} journal_pos_t;

#define POS_VALID(pos) 		((pos).offset != 0)
#define POS_INVALIDATE(pos) 	((pos).offset = 0, (pos).serial = 0)

typedef struct {
	unsigned char 	format[16];
	journal_pos_t 	begin;
	journal_pos_t 	end;
	isc_uint32_t	index_size;
} journal_header_t;

/*%
 * The in-core representation of the transaction header.
 */

typedef struct {
	isc_uint32_t	size;
	isc_uint32_t	serial0;
	isc_uint32_t	serial1;
} journal_xhdr_t;

/*%
 * The in-core representation of the RR header.
 */
typedef struct {
	isc_uint32_t	size;
} journal_rrhdr_t;


/*%
 * Initial contents to store in the header of a newly created
 * journal file.
 *
 * The header starts with the magic string ";BIND LOG V9\n"
 * to identify the file as a BIND 9 journal file.  An ASCII
 * identification string is used rather than a binary magic
 * number to be consistent with BIND 8 (BIND 8 journal files
 * are ASCII text files).
 */

static journal_header_t
initial_journal_header = { ";BIND LOG V9\n", { 0, 0 }, { 0, 0 }, 0 };

#define JOURNAL_EMPTY(h) ((h)->begin.offset == (h)->end.offset)

typedef enum {
	JOURNAL_STATE_INVALID,
	JOURNAL_STATE_READ,
	JOURNAL_STATE_WRITE,
	JOURNAL_STATE_TRANSACTION
} journal_state_t;

struct dns_journal {
	unsigned int		magic;		/*%< JOUR */
	isc_mem_t		*mctx;		/*%< Memory context */
	journal_state_t		state;
	const char 		*filename;	/*%< Journal file name */
	FILE *			fp;		/*%< File handle */
	isc_offset_t		offset;		/*%< Current file offset */
	journal_header_t 	header;		/*%< In-core journal header */
	unsigned char		*rawindex;	/*%< In-core buffer for journal index in on-disk format */
	journal_pos_t		*index;		/*%< In-core journal index */

	/*% Current transaction state (when writing). */
	struct {
		unsigned int	n_soa;		/*%< Number of SOAs seen */
		journal_pos_t	pos[2];		/*%< Begin/end position */
	} x;

	/*% Iteration state (when reading). */
	struct {
		/* These define the part of the journal we iterate over. */
		journal_pos_t bpos;		/*%< Position before first, */
		journal_pos_t epos;		/*%< and after last transaction */
		/* The rest is iterator state. */
		isc_uint32_t current_serial;	/*%< Current SOA serial */
		isc_buffer_t source;		/*%< Data from disk */
		isc_buffer_t target;		/*%< Data from _fromwire check */
		dns_decompress_t dctx;		/*%< Dummy decompression ctx */
		dns_name_t name;		/*%< Current domain name */
		dns_rdata_t rdata;		/*%< Current rdata */
		isc_uint32_t ttl;		/*%< Current TTL */
		unsigned int xsize;		/*%< Size of transaction data */
		unsigned int xpos;		/*%< Current position in it */
		isc_result_t result;		/*%< Result of last call */
	} it;
};

#define DNS_JOURNAL_MAGIC	ISC_MAGIC('J', 'O', 'U', 'R')
#define DNS_JOURNAL_VALID(t)	ISC_MAGIC_VALID(t, DNS_JOURNAL_MAGIC)

static void
journal_pos_decode(journal_rawpos_t *raw, journal_pos_t *cooked) {
	cooked->serial = decode_uint32(raw->serial);
	cooked->offset = decode_uint32(raw->offset);
}

static void
journal_pos_encode(journal_rawpos_t *raw, journal_pos_t *cooked) {
	encode_uint32(cooked->serial, raw->serial);
	encode_uint32(cooked->offset, raw->offset);
}

static void
journal_header_decode(journal_rawheader_t *raw, journal_header_t *cooked) {
	INSIST(sizeof(cooked->format) == sizeof(raw->h.format));
	memcpy(cooked->format, raw->h.format, sizeof(cooked->format));
	journal_pos_decode(&raw->h.begin, &cooked->begin);
	journal_pos_decode(&raw->h.end, &cooked->end);
	cooked->index_size = decode_uint32(raw->h.index_size);
}

static void
journal_header_encode(journal_header_t *cooked, journal_rawheader_t *raw) {
	INSIST(sizeof(cooked->format) == sizeof(raw->h.format));
	memset(raw->pad, 0, sizeof(raw->pad));
	memcpy(raw->h.format, cooked->format, sizeof(raw->h.format));
	journal_pos_encode(&raw->h.begin, &cooked->begin);
	journal_pos_encode(&raw->h.end, &cooked->end);
	encode_uint32(cooked->index_size, raw->h.index_size);
}

/*
 * Journal file I/O subroutines, with error checking and reporting.
 */
static isc_result_t
journal_seek(dns_journal_t *j, isc_uint32_t offset) {
	isc_result_t result;
	result = isc_stdio_seek(j->fp, (long)offset, SEEK_SET);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "%s: seek: %s", j->filename,
			      isc_result_totext(result));
		return (ISC_R_UNEXPECTED);
	}
	j->offset = offset;
	return (ISC_R_SUCCESS);
}

static isc_result_t
journal_read(dns_journal_t *j, void *mem, size_t nbytes) {
	isc_result_t result;

	result = isc_stdio_read(mem, 1, nbytes, j->fp, NULL);
	if (result != ISC_R_SUCCESS) {
		if (result == ISC_R_EOF)
			return (ISC_R_NOMORE);
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "%s: read: %s",
			      j->filename, isc_result_totext(result));
		return (ISC_R_UNEXPECTED);
	}
	j->offset += nbytes;
	return (ISC_R_SUCCESS);
}

static isc_result_t
journal_write(dns_journal_t *j, void *mem, size_t nbytes) {
	isc_result_t result;

	result = isc_stdio_write(mem, 1, nbytes, j->fp, NULL);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "%s: write: %s",
			      j->filename, isc_result_totext(result));
		return (ISC_R_UNEXPECTED);
	}
	j->offset += nbytes;
	return (ISC_R_SUCCESS);
}

static isc_result_t
journal_fsync(dns_journal_t *j) {
	isc_result_t result;
	result = isc_stdio_flush(j->fp);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "%s: flush: %s",
			      j->filename, isc_result_totext(result));
		return (ISC_R_UNEXPECTED);
	}
	result = isc_stdio_sync(j->fp);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "%s: fsync: %s",
			      j->filename, isc_result_totext(result));
		return (ISC_R_UNEXPECTED);
	}
	return (ISC_R_SUCCESS);
}

/*
 * Read/write a transaction header at the current file position.
 */

static isc_result_t
journal_read_xhdr(dns_journal_t *j, journal_xhdr_t *xhdr) {
	journal_rawxhdr_t raw;
	isc_result_t result;
	result = journal_read(j, &raw, sizeof(raw));
	if (result != ISC_R_SUCCESS)
		return (result);
	xhdr->size = decode_uint32(raw.size);
	xhdr->serial0 = decode_uint32(raw.serial0);
	xhdr->serial1 = decode_uint32(raw.serial1);
	return (ISC_R_SUCCESS);
}

static isc_result_t
journal_write_xhdr(dns_journal_t *j, isc_uint32_t size,
		   isc_uint32_t serial0, isc_uint32_t serial1)
{
	journal_rawxhdr_t raw;
	encode_uint32(size, raw.size);
	encode_uint32(serial0, raw.serial0);
	encode_uint32(serial1, raw.serial1);
	return (journal_write(j, &raw, sizeof(raw)));
}


/*
 * Read an RR header at the current file position.
 */

static isc_result_t
journal_read_rrhdr(dns_journal_t *j, journal_rrhdr_t *rrhdr) {
	journal_rawrrhdr_t raw;
	isc_result_t result;
	result = journal_read(j, &raw, sizeof(raw));
	if (result != ISC_R_SUCCESS)
		return (result);
	rrhdr->size = decode_uint32(raw.size);
	return (ISC_R_SUCCESS);
}

static isc_result_t
journal_file_create(isc_mem_t *mctx, const char *filename) {
	FILE *fp = NULL;
	isc_result_t result;
	journal_header_t header;
	journal_rawheader_t rawheader;
	int index_size = 56; /* XXX configurable */
	int size;
	void *mem; /* Memory for temporary index image. */

	INSIST(sizeof(journal_rawheader_t) == JOURNAL_HEADER_SIZE);

	result = isc_stdio_open(filename, "wb", &fp);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "%s: create: %s",
			      filename, isc_result_totext(result));
		return (ISC_R_UNEXPECTED);
	}

	header = initial_journal_header;
	header.index_size = index_size;
	journal_header_encode(&header, &rawheader);

	size = sizeof(journal_rawheader_t) +
		index_size * sizeof(journal_rawpos_t);

	mem = isc_mem_get(mctx, size);
	if (mem == NULL) {
		(void)isc_stdio_close(fp);
		(void)isc_file_remove(filename);
		return (ISC_R_NOMEMORY);
	}
	memset(mem, 0, size);
	memcpy(mem, &rawheader, sizeof(rawheader));

	result = isc_stdio_write(mem, 1, (size_t) size, fp, NULL);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
				 "%s: write: %s",
				 filename, isc_result_totext(result));
		(void)isc_stdio_close(fp);
		(void)isc_file_remove(filename);
		isc_mem_put(mctx, mem, size);
		return (ISC_R_UNEXPECTED);
	}
	isc_mem_put(mctx, mem, size);

	result = isc_stdio_close(fp);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
				 "%s: close: %s",
				 filename, isc_result_totext(result));
		(void)isc_file_remove(filename);
		return (ISC_R_UNEXPECTED);
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
journal_open(isc_mem_t *mctx, const char *filename, isc_boolean_t write,
	     isc_boolean_t create, dns_journal_t **journalp) {
	FILE *fp = NULL;
	isc_result_t result;
	journal_rawheader_t rawheader;
	dns_journal_t *j;

	INSIST(journalp != NULL && *journalp == NULL);
	j = isc_mem_get(mctx, sizeof(*j));
	if (j == NULL)
		return (ISC_R_NOMEMORY);

	j->mctx = mctx;
	j->state = JOURNAL_STATE_INVALID;
	j->fp = NULL;
	j->filename = filename;
	j->index = NULL;
	j->rawindex = NULL;

	result = isc_stdio_open(j->filename, write ? "rb+" : "rb", &fp);

	if (result == ISC_R_FILENOTFOUND) {
		if (create) {
			isc_log_write(JOURNAL_COMMON_LOGARGS,
				      ISC_LOG_INFO,
				      "journal file %s does not exist, "
				      "creating it",
				      j->filename);
			CHECK(journal_file_create(mctx, filename));
			/*
			 * Retry.
			 */
			result = isc_stdio_open(j->filename, "rb+", &fp);
		} else {
			FAIL(ISC_R_NOTFOUND);
		}
	}
	if (result != ISC_R_SUCCESS) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "%s: open: %s",
			      j->filename, isc_result_totext(result));
		FAIL(ISC_R_UNEXPECTED);
	}

	j->fp = fp;

	/*
	 * Set magic early so that seek/read can succeed.
	 */
	j->magic = DNS_JOURNAL_MAGIC;

	CHECK(journal_seek(j, 0));
	CHECK(journal_read(j, &rawheader, sizeof(rawheader)));

	if (memcmp(rawheader.h.format, initial_journal_header.format,
		   sizeof(initial_journal_header.format)) != 0) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
				 "%s: journal format not recognized",
				 j->filename);
		FAIL(ISC_R_UNEXPECTED);
	}
	journal_header_decode(&rawheader, &j->header);

	/*
	 * If there is an index, read the raw index into a dynamically
	 * allocated buffer and then convert it into a cooked index.
	 */
	if (j->header.index_size != 0) {
		unsigned int i;
		unsigned int rawbytes;
		unsigned char *p;

		rawbytes = j->header.index_size * sizeof(journal_rawpos_t);
		j->rawindex = isc_mem_get(mctx, rawbytes);
		if (j->rawindex == NULL)
			FAIL(ISC_R_NOMEMORY);

		CHECK(journal_read(j, j->rawindex, rawbytes));

		j->index = isc_mem_get(mctx, j->header.index_size *
				       sizeof(journal_pos_t));
		if (j->index == NULL)
			FAIL(ISC_R_NOMEMORY);

		p = j->rawindex;
		for (i = 0; i < j->header.index_size; i++) {
			j->index[i].serial = decode_uint32(p);
			p += 4;
			j->index[i].offset = decode_uint32(p);
			p += 4;
		}
		INSIST(p == j->rawindex + rawbytes);
	}
	j->offset = -1; /* Invalid, must seek explicitly. */

	/*
	 * Initialize the iterator.
	 */
	dns_name_init(&j->it.name, NULL);
	dns_rdata_init(&j->it.rdata);

	/*
	 * Set up empty initial buffers for unchecked and checked
	 * wire format RR data.  They will be reallocated
	 * later.
	 */
	isc_buffer_init(&j->it.source, NULL, 0);
	isc_buffer_init(&j->it.target, NULL, 0);
	dns_decompress_init(&j->it.dctx, -1, DNS_DECOMPRESS_NONE);

	j->state =
		write ? JOURNAL_STATE_WRITE : JOURNAL_STATE_READ;

	*journalp = j;
	return (ISC_R_SUCCESS);

 failure:
	j->magic = 0;
	if (j->index != NULL) {
		isc_mem_put(j->mctx, j->index, j->header.index_size *
			    sizeof(journal_rawpos_t));
		j->index = NULL;
	}
	if (j->fp != NULL)
		(void)isc_stdio_close(j->fp);
	isc_mem_put(j->mctx, j, sizeof(*j));
	return (result);
}

isc_result_t
dns_journal_open(isc_mem_t *mctx, const char *filename, isc_boolean_t write,
		 dns_journal_t **journalp) {
	isc_result_t result;
	int namelen;
	char backup[1024];

	result = journal_open(mctx, filename, write, write, journalp);
	if (result == ISC_R_NOTFOUND) {
		namelen = strlen(filename);
		if (namelen > 4 && strcmp(filename + namelen - 4, ".jnl") == 0)
			namelen -= 4;

		result = isc_string_printf(backup, sizeof(backup), "%.*s.jbk",
					   namelen, filename);
		if (result != ISC_R_SUCCESS)
			return (result);
		result = journal_open(mctx, backup, write, write, journalp);
	}
	return (result);
}

/*
 * A comparison function defining the sorting order for
 * entries in the IXFR-style journal file.
 *
 * The IXFR format requires that deletions are sorted before
 * additions, and within either one, SOA records are sorted
 * before others.
 *
 * Also sort the non-SOA records by type as a courtesy to the
 * server receiving the IXFR - it may help reduce the amount of
 * rdataset merging it has to do.
 */
static int
ixfr_order(const void *av, const void *bv) {
	dns_difftuple_t const * const *ap = av;
	dns_difftuple_t const * const *bp = bv;
	dns_difftuple_t const *a = *ap;
	dns_difftuple_t const *b = *bp;
	int r;
	int bop = 0, aop = 0;

	switch (a->op) {
	case DNS_DIFFOP_DEL:
	case DNS_DIFFOP_DELRESIGN:
		aop = 1;
		break;
	case DNS_DIFFOP_ADD:
	case DNS_DIFFOP_ADDRESIGN:
		aop = 0;
		break;
	default:
		INSIST(0);
	}

	switch (b->op) {
	case DNS_DIFFOP_DEL:
	case DNS_DIFFOP_DELRESIGN:
		bop = 1;
		break;
	case DNS_DIFFOP_ADD:
	case DNS_DIFFOP_ADDRESIGN:
		bop = 0;
		break;
	default:
		INSIST(0);
	}

	r = bop - aop;
	if (r != 0)
		return (r);

	r = (b->rdata.type == dns_rdatatype_soa) -
		(a->rdata.type == dns_rdatatype_soa);
	if (r != 0)
		return (r);

	r = (a->rdata.type - b->rdata.type);
	return (r);
}

/*
 * Advance '*pos' to the next journal transaction.
 *
 * Requires:
 *	*pos refers to a valid journal transaction.
 *
 * Ensures:
 *	When ISC_R_SUCCESS is returned,
 *	*pos refers to the next journal transaction.
 *
 * Returns one of:
 *
 *    ISC_R_SUCCESS
 *    ISC_R_NOMORE 	*pos pointed at the last transaction
 *    Other results due to file errors are possible.
 */
static isc_result_t
journal_next(dns_journal_t *j, journal_pos_t *pos) {
	isc_result_t result;
	journal_xhdr_t xhdr;
	REQUIRE(DNS_JOURNAL_VALID(j));

	result = journal_seek(j, pos->offset);
	if (result != ISC_R_SUCCESS)
		return (result);

	if (pos->serial == j->header.end.serial)
		return (ISC_R_NOMORE);
	/*
	 * Read the header of the current transaction.
	 * This will return ISC_R_NOMORE if we are at EOF.
	 */
	result = journal_read_xhdr(j, &xhdr);
	if (result != ISC_R_SUCCESS)
		return (result);

	/*
	 * Check serial number consistency.
	 */
	if (xhdr.serial0 != pos->serial) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "%s: journal file corrupt: "
			      "expected serial %u, got %u",
			      j->filename, pos->serial, xhdr.serial0);
		return (ISC_R_UNEXPECTED);
	}

	/*
	 * Check for offset wraparound.
	 */
	if ((isc_offset_t)(pos->offset + sizeof(journal_rawxhdr_t) + xhdr.size)
	    < pos->offset) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "%s: offset too large", j->filename);
		return (ISC_R_UNEXPECTED);
	}

	pos->offset += sizeof(journal_rawxhdr_t) + xhdr.size;
	pos->serial = xhdr.serial1;
	return (ISC_R_SUCCESS);
}

/*
 * If the index of the journal 'j' contains an entry "better"
 * than '*best_guess', replace '*best_guess' with it.
 *
 * "Better" means having a serial number closer to 'serial'
 * but not greater than 'serial'.
 */
static void
index_find(dns_journal_t *j, isc_uint32_t serial, journal_pos_t *best_guess) {
	unsigned int i;
	if (j->index == NULL)
		return;
	for (i = 0; i < j->header.index_size; i++) {
		if (POS_VALID(j->index[i]) &&
		    DNS_SERIAL_GE(serial, j->index[i].serial) &&
		    DNS_SERIAL_GT(j->index[i].serial, best_guess->serial))
			*best_guess = j->index[i];
	}
}

/*
 * Add a new index entry.  If there is no room, make room by removing
 * the odd-numbered entries and compacting the others into the first
 * half of the index.  This decimates old index entries exponentially
 * over time, so that the index always contains a much larger fraction
 * of recent serial numbers than of old ones.  This is deliberate -
 * most index searches are for outgoing IXFR, and IXFR tends to request
 * recent versions more often than old ones.
 */
static void
index_add(dns_journal_t *j, journal_pos_t *pos) {
	unsigned int i;
	if (j->index == NULL)
		return;
	/*
	 * Search for a vacant position.
	 */
	for (i = 0; i < j->header.index_size; i++) {
		if (! POS_VALID(j->index[i]))
			break;
	}
	if (i == j->header.index_size) {
		unsigned int k = 0;
		/*
		 * Found no vacant position.  Make some room.
		 */
		for (i = 0; i < j->header.index_size; i += 2) {
			j->index[k++] = j->index[i];
		}
		i = k; /* 'i' identifies the first vacant position. */
		while (k < j->header.index_size) {
			POS_INVALIDATE(j->index[k]);
			k++;
		}
	}
	INSIST(i < j->header.index_size);
	INSIST(! POS_VALID(j->index[i]));

	/*
	 * Store the new index entry.
	 */
	j->index[i] = *pos;
}

/*
 * Invalidate any existing index entries that could become
 * ambiguous when a new transaction with number 'serial' is added.
 */
static void
index_invalidate(dns_journal_t *j, isc_uint32_t serial) {
	unsigned int i;
	if (j->index == NULL)
		return;
	for (i = 0; i < j->header.index_size; i++) {
		if (! DNS_SERIAL_GT(serial, j->index[i].serial))
			POS_INVALIDATE(j->index[i]);
	}
}

/*
 * Try to find a transaction with initial serial number 'serial'
 * in the journal 'j'.
 *
 * If found, store its position at '*pos' and return ISC_R_SUCCESS.
 *
 * If 'serial' is current (= the ending serial number of the
 * last transaction in the journal), set '*pos' to
 * the position immediately following the last transaction and
 * return ISC_R_SUCCESS.
 *
 * If 'serial' is within the range of addressable serial numbers
 * covered by the journal but that particular serial number is missing
 * (from the journal, not just from the index), return ISC_R_NOTFOUND.
 *
 * If 'serial' is outside the range of addressable serial numbers
 * covered by the journal, return ISC_R_RANGE.
 *
 */
static isc_result_t
journal_find(dns_journal_t *j, isc_uint32_t serial, journal_pos_t *pos) {
	isc_result_t result;
	journal_pos_t current_pos;
	REQUIRE(DNS_JOURNAL_VALID(j));

	if (DNS_SERIAL_GT(j->header.begin.serial, serial))
		return (ISC_R_RANGE);
	if (DNS_SERIAL_GT(serial, j->header.end.serial))
		return (ISC_R_RANGE);
	if (serial == j->header.end.serial) {
		*pos = j->header.end;
		return (ISC_R_SUCCESS);
	}

	current_pos = j->header.begin;
	index_find(j, serial, &current_pos);

	while (current_pos.serial != serial) {
		if (DNS_SERIAL_GT(current_pos.serial, serial))
			return (ISC_R_NOTFOUND);
		result = journal_next(j, &current_pos);
		if (result != ISC_R_SUCCESS)
			return (result);
	}
	*pos = current_pos;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_journal_begin_transaction(dns_journal_t *j) {
	isc_uint32_t offset;
	isc_result_t result;
	journal_rawxhdr_t hdr;

	REQUIRE(DNS_JOURNAL_VALID(j));
	REQUIRE(j->state == JOURNAL_STATE_WRITE);

	/*
	 * Find the file offset where the new transaction should
	 * be written, and seek there.
	 */
	if (JOURNAL_EMPTY(&j->header)) {
		offset = sizeof(journal_rawheader_t) +
			j->header.index_size * sizeof(journal_rawpos_t);
	} else {
		offset = j->header.end.offset;
	}
	j->x.pos[0].offset = offset;
	j->x.pos[1].offset = offset; /* Initial value, will be incremented. */
	j->x.n_soa = 0;

	CHECK(journal_seek(j, offset));

	/*
	 * Write a dummy transaction header of all zeroes to reserve
	 * space.  It will be filled in when the transaction is
	 * finished.
	 */
	memset(&hdr, 0, sizeof(hdr));
	CHECK(journal_write(j, &hdr, sizeof(hdr)));
	j->x.pos[1].offset = j->offset;

	j->state = JOURNAL_STATE_TRANSACTION;
	result = ISC_R_SUCCESS;
 failure:
	return (result);
}

isc_result_t
dns_journal_writediff(dns_journal_t *j, dns_diff_t *diff) {
	dns_difftuple_t *t;
	isc_buffer_t buffer;
	void *mem = NULL;
	unsigned int size;
	isc_result_t result;
	isc_region_t used;

	REQUIRE(DNS_DIFF_VALID(diff));
	REQUIRE(j->state == JOURNAL_STATE_TRANSACTION);

	isc_log_write(JOURNAL_DEBUG_LOGARGS(3), "writing to journal");
	(void)dns_diff_print(diff, NULL);

	/*
	 * Pass 1: determine the buffer size needed, and
	 * keep track of SOA serial numbers.
	 */
	size = 0;
	for (t = ISC_LIST_HEAD(diff->tuples); t != NULL;
	     t = ISC_LIST_NEXT(t, link))
	{
		if (t->rdata.type == dns_rdatatype_soa) {
			if (j->x.n_soa < 2)
				j->x.pos[j->x.n_soa].serial =
					dns_soa_getserial(&t->rdata);
			j->x.n_soa++;
		}
		size += sizeof(journal_rawrrhdr_t);
		size += t->name.length; /* XXX should have access macro? */
		size += 10;
		size += t->rdata.length;
	}

	mem = isc_mem_get(j->mctx, size);
	if (mem == NULL)
		return (ISC_R_NOMEMORY);

	isc_buffer_init(&buffer, mem, size);

	/*
	 * Pass 2.  Write RRs to buffer.
	 */
	for (t = ISC_LIST_HEAD(diff->tuples); t != NULL;
	     t = ISC_LIST_NEXT(t, link))
	{
		/*
		 * Write the RR header.
		 */
		isc_buffer_putuint32(&buffer, t->name.length + 10 +
				     t->rdata.length);
		/*
		 * Write the owner name, RR header, and RR data.
		 */
		isc_buffer_putmem(&buffer, t->name.ndata, t->name.length);
		isc_buffer_putuint16(&buffer, t->rdata.type);
		isc_buffer_putuint16(&buffer, t->rdata.rdclass);
		isc_buffer_putuint32(&buffer, t->ttl);
		INSIST(t->rdata.length < 65536);
		isc_buffer_putuint16(&buffer, (isc_uint16_t)t->rdata.length);
		INSIST(isc_buffer_availablelength(&buffer) >= t->rdata.length);
		isc_buffer_putmem(&buffer, t->rdata.data, t->rdata.length);
	}

	isc_buffer_usedregion(&buffer, &used);
	INSIST(used.length == size);

	j->x.pos[1].offset += used.length;

	/*
	 * Write the buffer contents to the journal file.
	 */
	CHECK(journal_write(j, used.base, used.length));

	result = ISC_R_SUCCESS;

 failure:
	if (mem != NULL)
		isc_mem_put(j->mctx, mem, size);
	return (result);

}

isc_result_t
dns_journal_commit(dns_journal_t *j) {
	isc_result_t result;
	journal_rawheader_t rawheader;

	REQUIRE(DNS_JOURNAL_VALID(j));
	REQUIRE(j->state == JOURNAL_STATE_TRANSACTION);

	/*
	 * Perform some basic consistency checks.
	 */
	if (j->x.n_soa != 2) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "%s: malformed transaction: %d SOAs",
			      j->filename, j->x.n_soa);
		return (ISC_R_UNEXPECTED);
	}
	if (! (DNS_SERIAL_GT(j->x.pos[1].serial, j->x.pos[0].serial) ||
	       (bind8_compat &&
		j->x.pos[1].serial == j->x.pos[0].serial)))
	{
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "%s: malformed transaction: serial number "
			      "would decrease", j->filename);
		return (ISC_R_UNEXPECTED);
	}
	if (! JOURNAL_EMPTY(&j->header)) {
		if (j->x.pos[0].serial != j->header.end.serial) {
			isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
					 "malformed transaction: "
					 "%s last serial %u != "
					 "transaction first serial %u",
					 j->filename,
					 j->header.end.serial,
					 j->x.pos[0].serial);
			return (ISC_R_UNEXPECTED);
		}
	}

	/*
	 * Some old journal entries may become non-addressable
	 * when we increment the current serial number.  Purge them
	 * by stepping header.begin forward to the first addressable
	 * transaction.  Also purge them from the index.
	 */
	if (! JOURNAL_EMPTY(&j->header)) {
		while (! DNS_SERIAL_GT(j->x.pos[1].serial,
				       j->header.begin.serial)) {
			CHECK(journal_next(j, &j->header.begin));
		}
		index_invalidate(j, j->x.pos[1].serial);
	}
#ifdef notyet
	if (DNS_SERIAL_GT(last_dumped_serial, j->x.pos[1].serial)) {
		force_dump(...);
	}
#endif

	/*
	 * Commit the transaction data to stable storage.
	 */
	CHECK(journal_fsync(j));

	/*
	 * Update the transaction header.
	 */
	CHECK(journal_seek(j, j->x.pos[0].offset));
	CHECK(journal_write_xhdr(j, (j->x.pos[1].offset - j->x.pos[0].offset) -
				 sizeof(journal_rawxhdr_t),
				 j->x.pos[0].serial, j->x.pos[1].serial));

	/*
	 * Update the journal header.
	 */
	if (JOURNAL_EMPTY(&j->header)) {
		j->header.begin = j->x.pos[0];
	}
	j->header.end = j->x.pos[1];
	journal_header_encode(&j->header, &rawheader);
	CHECK(journal_seek(j, 0));
	CHECK(journal_write(j, &rawheader, sizeof(rawheader)));

	/*
	 * Update the index.
	 */
	index_add(j, &j->x.pos[0]);

	/*
	 * Convert the index into on-disk format and write
	 * it to disk.
	 */
	CHECK(index_to_disk(j));

	/*
	 * Commit the header to stable storage.
	 */
	CHECK(journal_fsync(j));

	/*
	 * We no longer have a transaction open.
	 */
	j->state = JOURNAL_STATE_WRITE;

	result = ISC_R_SUCCESS;

 failure:
	return (result);
}

isc_result_t
dns_journal_write_transaction(dns_journal_t *j, dns_diff_t *diff) {
	isc_result_t result;
	CHECK(dns_diff_sort(diff, ixfr_order));
	CHECK(dns_journal_begin_transaction(j));
	CHECK(dns_journal_writediff(j, diff));
	CHECK(dns_journal_commit(j));
	result = ISC_R_SUCCESS;
 failure:
	return (result);
}

void
dns_journal_destroy(dns_journal_t **journalp) {
	dns_journal_t *j = *journalp;
	REQUIRE(DNS_JOURNAL_VALID(j));

	j->it.result = ISC_R_FAILURE;
	dns_name_invalidate(&j->it.name);
	dns_decompress_invalidate(&j->it.dctx);
	if (j->rawindex != NULL)
		isc_mem_put(j->mctx, j->rawindex, j->header.index_size *
			    sizeof(journal_rawpos_t));
	if (j->index != NULL)
		isc_mem_put(j->mctx, j->index, j->header.index_size *
			    sizeof(journal_pos_t));
	if (j->it.target.base != NULL)
		isc_mem_put(j->mctx, j->it.target.base, j->it.target.length);
	if (j->it.source.base != NULL)
		isc_mem_put(j->mctx, j->it.source.base, j->it.source.length);

	if (j->fp != NULL)
		(void)isc_stdio_close(j->fp);
	j->magic = 0;
	isc_mem_put(j->mctx, j, sizeof(*j));
	*journalp = NULL;
}

/*
 * Roll the open journal 'j' into the database 'db'.
 * A new database version will be created.
 */

/* XXX Share code with incoming IXFR? */

static isc_result_t
roll_forward(dns_journal_t *j, dns_db_t *db, unsigned int options) {
	isc_buffer_t source;		/* Transaction data from disk */
	isc_buffer_t target;		/* Ditto after _fromwire check */
	isc_uint32_t db_serial;		/* Database SOA serial */
	isc_uint32_t end_serial;	/* Last journal SOA serial */
	isc_result_t result;
	dns_dbversion_t *ver = NULL;
	journal_pos_t pos;
	dns_diff_t diff;
	unsigned int n_soa = 0;
	unsigned int n_put = 0;
	dns_diffop_t op;

	REQUIRE(DNS_JOURNAL_VALID(j));
	REQUIRE(DNS_DB_VALID(db));

	dns_diff_init(j->mctx, &diff);

	/*
	 * Set up empty initial buffers for unchecked and checked
	 * wire format transaction data.  They will be reallocated
	 * later.
	 */
	isc_buffer_init(&source, NULL, 0);
	isc_buffer_init(&target, NULL, 0);

	/*
	 * Create the new database version.
	 */
	CHECK(dns_db_newversion(db, &ver));

	/*
	 * Get the current database SOA serial number.
	 */
	CHECK(dns_db_getsoaserial(db, ver, &db_serial));

	/*
	 * Locate a journal entry for the current database serial.
	 */
	CHECK(journal_find(j, db_serial, &pos));
	/*
	 * XXX do more drastic things, like marking zone stale,
	 * if this fails?
	 */
	/*
	 * XXXRTH  The zone code should probably mark the zone as bad and
	 *         scream loudly into the log if this is a dynamic update
	 *	   log reply that failed.
	 */

	end_serial = dns_journal_last_serial(j);
	if (db_serial == end_serial)
		CHECK(DNS_R_UPTODATE);

	CHECK(dns_journal_iter_init(j, db_serial, end_serial));

	for (result = dns_journal_first_rr(j);
	     result == ISC_R_SUCCESS;
	     result = dns_journal_next_rr(j))
	{
		dns_name_t *name;
		isc_uint32_t ttl;
		dns_rdata_t *rdata;
		dns_difftuple_t *tuple = NULL;

		name = NULL;
		rdata = NULL;
		dns_journal_current_rr(j, &name, &ttl, &rdata);

		if (rdata->type == dns_rdatatype_soa) {
			n_soa++;
			if (n_soa == 2)
				db_serial = j->it.current_serial;
		}

		if (n_soa == 3)
			n_soa = 1;
		if (n_soa == 0) {
			isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
					 "%s: journal file corrupt: missing "
					 "initial SOA", j->filename);
			FAIL(ISC_R_UNEXPECTED);
		}
		if ((options & DNS_JOURNALOPT_RESIGN) != 0)
			op = (n_soa == 1) ? DNS_DIFFOP_DELRESIGN :
					    DNS_DIFFOP_ADDRESIGN;
		else
			op = (n_soa == 1) ? DNS_DIFFOP_DEL : DNS_DIFFOP_ADD;

		CHECK(dns_difftuple_create(diff.mctx, op, name, ttl, rdata,
					   &tuple));
		dns_diff_append(&diff, &tuple);

		if (++n_put > 100)  {
			isc_log_write(JOURNAL_DEBUG_LOGARGS(3),
				      "%s: applying diff to database (%u)",
				      j->filename, db_serial);
			(void)dns_diff_print(&diff, NULL);
			CHECK(dns_diff_apply(&diff, db, ver));
			dns_diff_clear(&diff);
			n_put = 0;
		}
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;
	CHECK(result);

	if (n_put != 0) {
		isc_log_write(JOURNAL_DEBUG_LOGARGS(3),
			      "%s: applying final diff to database (%u)",
			      j->filename, db_serial);
		(void)dns_diff_print(&diff, NULL);
		CHECK(dns_diff_apply(&diff, db, ver));
		dns_diff_clear(&diff);
	}

 failure:
	if (ver != NULL)
		dns_db_closeversion(db, &ver, result == ISC_R_SUCCESS ?
				    ISC_TRUE : ISC_FALSE);

	if (source.base != NULL)
		isc_mem_put(j->mctx, source.base, source.length);
	if (target.base != NULL)
		isc_mem_put(j->mctx, target.base, target.length);

	dns_diff_clear(&diff);

	return (result);
}

isc_result_t
dns_journal_rollforward(isc_mem_t *mctx, dns_db_t *db,
			unsigned int options, const char *filename)
{
	dns_journal_t *j;
	isc_result_t result;

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(filename != NULL);

	j = NULL;
	result = dns_journal_open(mctx, filename, ISC_FALSE, &j);
	if (result == ISC_R_NOTFOUND) {
		isc_log_write(JOURNAL_DEBUG_LOGARGS(3),
			      "no journal file, but that's OK");
		return (DNS_R_NOJOURNAL);
	}
	if (result != ISC_R_SUCCESS)
		return (result);
	if (JOURNAL_EMPTY(&j->header))
		result = DNS_R_UPTODATE;
	else
		result = roll_forward(j, db, options);

	dns_journal_destroy(&j);

	return (result);
}

isc_result_t
dns_journal_print(isc_mem_t *mctx, const char *filename, FILE *file) {
	dns_journal_t *j;
	isc_buffer_t source;		/* Transaction data from disk */
	isc_buffer_t target;		/* Ditto after _fromwire check */
	isc_uint32_t start_serial;		/* Database SOA serial */
	isc_uint32_t end_serial;	/* Last journal SOA serial */
	isc_result_t result;
	dns_diff_t diff;
	unsigned int n_soa = 0;
	unsigned int n_put = 0;

	REQUIRE(filename != NULL);

	j = NULL;
	result = dns_journal_open(mctx, filename, ISC_FALSE, &j);
	if (result == ISC_R_NOTFOUND) {
		isc_log_write(JOURNAL_DEBUG_LOGARGS(3), "no journal file");
		return (DNS_R_NOJOURNAL);
	}

	if (result != ISC_R_SUCCESS) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "journal open failure: %s: %s",
			      isc_result_totext(result), filename);
		return (result);
	}

	dns_diff_init(j->mctx, &diff);

	/*
	 * Set up empty initial buffers for unchecked and checked
	 * wire format transaction data.  They will be reallocated
	 * later.
	 */
	isc_buffer_init(&source, NULL, 0);
	isc_buffer_init(&target, NULL, 0);

	start_serial = dns_journal_first_serial(j);
	end_serial = dns_journal_last_serial(j);

	CHECK(dns_journal_iter_init(j, start_serial, end_serial));

	for (result = dns_journal_first_rr(j);
	     result == ISC_R_SUCCESS;
	     result = dns_journal_next_rr(j))
	{
		dns_name_t *name;
		isc_uint32_t ttl;
		dns_rdata_t *rdata;
		dns_difftuple_t *tuple = NULL;

		name = NULL;
		rdata = NULL;
		dns_journal_current_rr(j, &name, &ttl, &rdata);

		if (rdata->type == dns_rdatatype_soa)
			n_soa++;

		if (n_soa == 3)
			n_soa = 1;
		if (n_soa == 0) {
			isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
				      "%s: journal file corrupt: missing "
				      "initial SOA", j->filename);
			FAIL(ISC_R_UNEXPECTED);
		}
		CHECK(dns_difftuple_create(diff.mctx, n_soa == 1 ?
					   DNS_DIFFOP_DEL : DNS_DIFFOP_ADD,
					   name, ttl, rdata, &tuple));
		dns_diff_append(&diff, &tuple);

		if (++n_put > 100)  {
			result = dns_diff_print(&diff, file);
			dns_diff_clear(&diff);
			n_put = 0;
			if (result != ISC_R_SUCCESS)
				break;
		}
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;
	CHECK(result);

	if (n_put != 0) {
		result = dns_diff_print(&diff, file);
		dns_diff_clear(&diff);
	}
	goto cleanup;

 failure:
	isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
		      "%s: cannot print: journal file corrupt", j->filename);

 cleanup:
	if (source.base != NULL)
		isc_mem_put(j->mctx, source.base, source.length);
	if (target.base != NULL)
		isc_mem_put(j->mctx, target.base, target.length);

	dns_diff_clear(&diff);
	dns_journal_destroy(&j);

	return (result);
}

/**************************************************************************/
/*
 * Miscellaneous accessors.
 */
isc_uint32_t dns_journal_first_serial(dns_journal_t *j) {
	return (j->header.begin.serial);
}

isc_uint32_t dns_journal_last_serial(dns_journal_t *j) {
	return (j->header.end.serial);
}

/**************************************************************************/
/*
 * Iteration support.
 *
 * When serving an outgoing IXFR, we transmit a part the journal starting
 * at the serial number in the IXFR request and ending at the serial
 * number that is current when the IXFR request arrives.  The ending
 * serial number is not necessarily at the end of the journal:
 * the journal may grow while the IXFR is in progress, but we stop
 * when we reach the serial number that was current when the IXFR started.
 */

static isc_result_t read_one_rr(dns_journal_t *j);

/*
 * Make sure the buffer 'b' is has at least 'size' bytes
 * allocated, and clear it.
 *
 * Requires:
 *	Either b->base is NULL, or it points to b->length bytes of memory
 *	previously allocated by isc_mem_get().
 */

static isc_result_t
size_buffer(isc_mem_t *mctx, isc_buffer_t *b, unsigned size) {
	if (b->length < size) {
		void *mem = isc_mem_get(mctx, size);
		if (mem == NULL)
			return (ISC_R_NOMEMORY);
		if (b->base != NULL)
			isc_mem_put(mctx, b->base, b->length);
		b->base = mem;
		b->length = size;
	}
	isc_buffer_clear(b);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_journal_iter_init(dns_journal_t *j,
		      isc_uint32_t begin_serial, isc_uint32_t end_serial)
{
	isc_result_t result;

	CHECK(journal_find(j, begin_serial, &j->it.bpos));
	INSIST(j->it.bpos.serial == begin_serial);

	CHECK(journal_find(j, end_serial, &j->it.epos));
	INSIST(j->it.epos.serial == end_serial);

	result = ISC_R_SUCCESS;
 failure:
	j->it.result = result;
	return (j->it.result);
}


isc_result_t
dns_journal_first_rr(dns_journal_t *j) {
	isc_result_t result;

	/*
	 * Seek to the beginning of the first transaction we are
	 * interested in.
	 */
	CHECK(journal_seek(j, j->it.bpos.offset));
	j->it.current_serial = j->it.bpos.serial;

	j->it.xsize = 0;  /* We have no transaction data yet... */
	j->it.xpos = 0;	  /* ...and haven't used any of it. */

	return (read_one_rr(j));

 failure:
	return (result);
}

static isc_result_t
read_one_rr(dns_journal_t *j) {
	isc_result_t result;

	dns_rdatatype_t rdtype;
	dns_rdataclass_t rdclass;
	unsigned int rdlen;
	isc_uint32_t ttl;
	journal_xhdr_t xhdr;
	journal_rrhdr_t rrhdr;

	INSIST(j->offset <= j->it.epos.offset);
	if (j->offset == j->it.epos.offset)
		return (ISC_R_NOMORE);
	if (j->it.xpos == j->it.xsize) {
		/*
		 * We are at a transaction boundary.
		 * Read another transaction header.
		 */
		CHECK(journal_read_xhdr(j, &xhdr));
		if (xhdr.size == 0) {
			isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
				      "%s: journal corrupt: empty transaction",
				      j->filename);
			FAIL(ISC_R_UNEXPECTED);
		}
		if (xhdr.serial0 != j->it.current_serial) {
			isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
					 "%s: journal file corrupt: "
					 "expected serial %u, got %u",
					 j->filename,
					 j->it.current_serial, xhdr.serial0);
			FAIL(ISC_R_UNEXPECTED);
		}
		j->it.xsize = xhdr.size;
		j->it.xpos = 0;
	}
	/*
	 * Read an RR.
	 */
	CHECK(journal_read_rrhdr(j, &rrhdr));
	/*
	 * Perform a sanity check on the journal RR size.
	 * The smallest possible RR has a 1-byte owner name
	 * and a 10-byte header.  The largest possible
	 * RR has 65535 bytes of data, a header, and a maximum-
	 * size owner name, well below 70 k total.
	 */
	if (rrhdr.size < 1+10 || rrhdr.size > 70000) {
		isc_log_write(JOURNAL_COMMON_LOGARGS, ISC_LOG_ERROR,
				 "%s: journal corrupt: impossible RR size "
				 "(%d bytes)", j->filename, rrhdr.size);
		FAIL(ISC_R_UNEXPECTED);
	}

	CHECK(size_buffer(j->mctx, &j->it.source, rrhdr.size));
	CHECK(journal_read(j, j->it.source.base, rrhdr.size));
	isc_buffer_add(&j->it.source, rrhdr.size);

	/*
	 * The target buffer is made the same size
	 * as the source buffer, with the assumption that when
	 * no compression in present, the output of dns_*_fromwire()
	 * is no larger than the input.
	 */
	CHECK(size_buffer(j->mctx, &j->it.target, rrhdr.size));

	/*
	 * Parse the owner name.  We don't know where it
	 * ends yet, so we make the entire "remaining"
	 * part of the buffer "active".
	 */
	isc_buffer_setactive(&j->it.source,
			     j->it.source.used - j->it.source.current);
	CHECK(dns_name_fromwire(&j->it.name, &j->it.source,
				&j->it.dctx, 0, &j->it.target));

	/*
	 * Check that the RR header is there, and parse it.
	 */
	if (isc_buffer_remaininglength(&j->it.source) < 10)
		FAIL(DNS_R_FORMERR);

	rdtype = isc_buffer_getuint16(&j->it.source);
	rdclass = isc_buffer_getuint16(&j->it.source);
	ttl = isc_buffer_getuint32(&j->it.source);
	rdlen = isc_buffer_getuint16(&j->it.source);

	/*
	 * Parse the rdata.
	 */
	if (isc_buffer_remaininglength(&j->it.source) != rdlen)
		FAIL(DNS_R_FORMERR);
	isc_buffer_setactive(&j->it.source, rdlen);
	dns_rdata_reset(&j->it.rdata);
	CHECK(dns_rdata_fromwire(&j->it.rdata, rdclass,
				 rdtype, &j->it.source, &j->it.dctx,
				 0, &j->it.target));
	j->it.ttl = ttl;

	j->it.xpos += sizeof(journal_rawrrhdr_t) + rrhdr.size;
	if (rdtype == dns_rdatatype_soa) {
		/* XXX could do additional consistency checks here */
		j->it.current_serial = dns_soa_getserial(&j->it.rdata);
	}

	result = ISC_R_SUCCESS;

 failure:
	j->it.result = result;
	return (result);
}

isc_result_t
dns_journal_next_rr(dns_journal_t *j) {
	j->it.result = read_one_rr(j);
	return (j->it.result);
}

void
dns_journal_current_rr(dns_journal_t *j, dns_name_t **name, isc_uint32_t *ttl,
		   dns_rdata_t **rdata)
{
	REQUIRE(j->it.result == ISC_R_SUCCESS);
	*name = &j->it.name;
	*ttl = j->it.ttl;
	*rdata = &j->it.rdata;
}

/**************************************************************************/
/*
 * Generating diffs from databases
 */

/*
 * Construct a diff containing all the RRs at the current name of the
 * database iterator 'dbit' in database 'db', version 'ver'.
 * Set '*name' to the current name, and append the diff to 'diff'.
 * All new tuples will have the operation 'op'.
 *
 * Requires: 'name' must have buffer large enough to hold the name.
 * Typically, a dns_fixedname_t would be used.
 */
static isc_result_t
get_name_diff(dns_db_t *db, dns_dbversion_t *ver, isc_stdtime_t now,
	      dns_dbiterator_t *dbit, dns_name_t *name, dns_diffop_t op,
	      dns_diff_t *diff)
{
	isc_result_t result;
	dns_dbnode_t *node = NULL;
	dns_rdatasetiter_t *rdsiter = NULL;
	dns_difftuple_t *tuple = NULL;

	result = dns_dbiterator_current(dbit, &node, name);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_db_allrdatasets(db, node, ver, now, &rdsiter);
	if (result != ISC_R_SUCCESS)
		goto cleanup_node;

	for (result = dns_rdatasetiter_first(rdsiter);
	     result == ISC_R_SUCCESS;
	     result = dns_rdatasetiter_next(rdsiter))
	{
		dns_rdataset_t rdataset;

		dns_rdataset_init(&rdataset);
		dns_rdatasetiter_current(rdsiter, &rdataset);

		for (result = dns_rdataset_first(&rdataset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(&rdataset))
		{
			dns_rdata_t rdata = DNS_RDATA_INIT;
			dns_rdataset_current(&rdataset, &rdata);
			result = dns_difftuple_create(diff->mctx, op, name,
						      rdataset.ttl, &rdata,
						      &tuple);
			if (result != ISC_R_SUCCESS) {
				dns_rdataset_disassociate(&rdataset);
				goto cleanup_iterator;
			}
			dns_diff_append(diff, &tuple);
		}
		dns_rdataset_disassociate(&rdataset);
		if (result != ISC_R_NOMORE)
			goto cleanup_iterator;
	}
	if (result != ISC_R_NOMORE)
		goto cleanup_iterator;

	result = ISC_R_SUCCESS;

 cleanup_iterator:
	dns_rdatasetiter_destroy(&rdsiter);

 cleanup_node:
	dns_db_detachnode(db, &node);

	return (result);
}

/*
 * Comparison function for use by dns_diff_subtract when sorting
 * the diffs to be subtracted.  The sort keys are the rdata type
 * and the rdata itself.  The owner name is ignored, because
 * it is known to be the same for all tuples.
 */
static int
rdata_order(const void *av, const void *bv) {
	dns_difftuple_t const * const *ap = av;
	dns_difftuple_t const * const *bp = bv;
	dns_difftuple_t const *a = *ap;
	dns_difftuple_t const *b = *bp;
	int r;
	r = (b->rdata.type - a->rdata.type);
	if (r != 0)
		return (r);
	r = dns_rdata_compare(&a->rdata, &b->rdata);
	return (r);
}

static isc_result_t
dns_diff_subtract(dns_diff_t diff[2], dns_diff_t *r) {
	isc_result_t result;
	dns_difftuple_t *p[2];
	int i, t;
	isc_boolean_t append;

	CHECK(dns_diff_sort(&diff[0], rdata_order));
	CHECK(dns_diff_sort(&diff[1], rdata_order));

	for (;;) {
		p[0] = ISC_LIST_HEAD(diff[0].tuples);
		p[1] = ISC_LIST_HEAD(diff[1].tuples);
		if (p[0] == NULL && p[1] == NULL)
			break;

		for (i = 0; i < 2; i++)
			if (p[!i] == NULL) {
				ISC_LIST_UNLINK(diff[i].tuples, p[i], link);
				ISC_LIST_APPEND(r->tuples, p[i], link);
				goto next;
			}
		t = rdata_order(&p[0], &p[1]);
		if (t < 0) {
			ISC_LIST_UNLINK(diff[0].tuples, p[0], link);
			ISC_LIST_APPEND(r->tuples, p[0], link);
			goto next;
		}
		if (t > 0) {
			ISC_LIST_UNLINK(diff[1].tuples, p[1], link);
			ISC_LIST_APPEND(r->tuples, p[1], link);
			goto next;
		}
		INSIST(t == 0);
		/*
		 * Identical RRs in both databases; skip them both
		 * if the ttl differs.
		 */
		append = ISC_TF(p[0]->ttl != p[1]->ttl);
		for (i = 0; i < 2; i++) {
			ISC_LIST_UNLINK(diff[i].tuples, p[i], link);
			if (append) {
				ISC_LIST_APPEND(r->tuples, p[i], link);
			} else {
				dns_difftuple_free(&p[i]);
			}
		}
	next: ;
	}
	result = ISC_R_SUCCESS;
 failure:
	return (result);
}

/*
 * Compare the databases 'dba' and 'dbb' and generate a journal
 * entry containing the changes to make 'dba' from 'dbb' (note
 * the order).  This journal entry will consist of a single,
 * possibly very large transaction.
 */

isc_result_t
dns_db_diff(isc_mem_t *mctx,
	    dns_db_t *dba, dns_dbversion_t *dbvera,
	    dns_db_t *dbb, dns_dbversion_t *dbverb,
	    const char *journal_filename)
{
	dns_db_t *db[2];
	dns_dbversion_t *ver[2];
	dns_dbiterator_t *dbit[2] = { NULL, NULL };
	isc_boolean_t have[2] = { ISC_FALSE, ISC_FALSE };
	dns_fixedname_t fixname[2];
	isc_result_t result, itresult[2];
	dns_diff_t diff[2], resultdiff;
	int i, t;
	dns_journal_t *journal = NULL;

	db[0] = dba, db[1] = dbb;
	ver[0] = dbvera, ver[1] = dbverb;

	dns_diff_init(mctx, &diff[0]);
	dns_diff_init(mctx, &diff[1]);
	dns_diff_init(mctx, &resultdiff);

	dns_fixedname_init(&fixname[0]);
	dns_fixedname_init(&fixname[1]);

	result = dns_journal_open(mctx, journal_filename, ISC_TRUE, &journal);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_db_createiterator(db[0], 0, &dbit[0]);
	if (result != ISC_R_SUCCESS)
		goto cleanup_journal;
	result = dns_db_createiterator(db[1], 0, &dbit[1]);
	if (result != ISC_R_SUCCESS)
		goto cleanup_interator0;

	itresult[0] = dns_dbiterator_first(dbit[0]);
	itresult[1] = dns_dbiterator_first(dbit[1]);

	for (;;) {
		for (i = 0; i < 2; i++) {
			if (! have[i] && itresult[i] == ISC_R_SUCCESS) {
				CHECK(get_name_diff(db[i], ver[i], 0, dbit[i],
					    dns_fixedname_name(&fixname[i]),
					    i == 0 ?
					    DNS_DIFFOP_ADD :
					    DNS_DIFFOP_DEL,
					    &diff[i]));
				itresult[i] = dns_dbiterator_next(dbit[i]);
				have[i] = ISC_TRUE;
			}
		}

		if (! have[0] && ! have[1]) {
			INSIST(ISC_LIST_EMPTY(diff[0].tuples));
			INSIST(ISC_LIST_EMPTY(diff[1].tuples));
			break;
		}

		for (i = 0; i < 2; i++) {
			if (! have[!i]) {
				ISC_LIST_APPENDLIST(resultdiff.tuples,
						    diff[i].tuples, link);
				INSIST(ISC_LIST_EMPTY(diff[i].tuples));
				have[i] = ISC_FALSE;
				goto next;
			}
		}

		t = dns_name_compare(dns_fixedname_name(&fixname[0]),
				     dns_fixedname_name(&fixname[1]));
		if (t < 0) {
			ISC_LIST_APPENDLIST(resultdiff.tuples,
					    diff[0].tuples, link);
			INSIST(ISC_LIST_EMPTY(diff[0].tuples));
			have[0] = ISC_FALSE;
			continue;
		}
		if (t > 0) {
			ISC_LIST_APPENDLIST(resultdiff.tuples,
					    diff[1].tuples, link);
			INSIST(ISC_LIST_EMPTY(diff[1].tuples));
			have[1] = ISC_FALSE;
			continue;
		}
		INSIST(t == 0);
		CHECK(dns_diff_subtract(diff, &resultdiff));
		INSIST(ISC_LIST_EMPTY(diff[0].tuples));
		INSIST(ISC_LIST_EMPTY(diff[1].tuples));
		have[0] = have[1] = ISC_FALSE;
	next: ;
	}
	if (itresult[0] != ISC_R_NOMORE)
		FAIL(itresult[0]);
	if (itresult[1] != ISC_R_NOMORE)
		FAIL(itresult[1]);

	if (ISC_LIST_EMPTY(resultdiff.tuples)) {
		isc_log_write(JOURNAL_DEBUG_LOGARGS(3), "no changes");
	} else {
		CHECK(dns_journal_write_transaction(journal, &resultdiff));
	}
	INSIST(ISC_LIST_EMPTY(diff[0].tuples));
	INSIST(ISC_LIST_EMPTY(diff[1].tuples));

 failure:
	dns_diff_clear(&resultdiff);
	dns_dbiterator_destroy(&dbit[1]);
 cleanup_interator0:
	dns_dbiterator_destroy(&dbit[0]);
 cleanup_journal:
	dns_journal_destroy(&journal);
	return (result);
}

isc_result_t
dns_journal_compact(isc_mem_t *mctx, char *filename, isc_uint32_t serial,
		    isc_uint32_t target_size)
{
	unsigned int i;
	journal_pos_t best_guess;
	journal_pos_t current_pos;
	dns_journal_t *j = NULL;
	dns_journal_t *new = NULL;
	journal_rawheader_t rawheader;
	unsigned int copy_length;
	int namelen;
	char *buf = NULL;
	unsigned int size = 0;
	isc_result_t result;
	unsigned int indexend;
	char newname[1024];
	char backup[1024];
	isc_boolean_t is_backup = ISC_FALSE;

	namelen = strlen(filename);
	if (namelen > 4 && strcmp(filename + namelen - 4, ".jnl") == 0)
		namelen -= 4;

	result = isc_string_printf(newname, sizeof(newname), "%.*s.jnw",
				   namelen, filename);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = isc_string_printf(backup, sizeof(backup), "%.*s.jbk",
				   namelen, filename);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = journal_open(mctx, filename, ISC_FALSE, ISC_FALSE, &j);
	if (result == ISC_R_NOTFOUND) {
		is_backup = ISC_TRUE;
		result = journal_open(mctx, backup, ISC_FALSE, ISC_FALSE, &j);
	}
	if (result != ISC_R_SUCCESS)
		return (result);

	if (JOURNAL_EMPTY(&j->header)) {
		dns_journal_destroy(&j);
		return (ISC_R_SUCCESS);
	}

	if (DNS_SERIAL_GT(j->header.begin.serial, serial) ||
	    DNS_SERIAL_GT(serial, j->header.end.serial)) {
		dns_journal_destroy(&j);
		return (ISC_R_RANGE);
	}

	/*
	 * Cope with very small target sizes.
	 */
	indexend = sizeof(journal_rawheader_t) +
		   j->header.index_size * sizeof(journal_rawpos_t);
	if (target_size < indexend * 2)
		target_size = target_size/2 + indexend;

	/*
	 * See if there is any work to do.
	 */
	if ((isc_uint32_t) j->header.end.offset < target_size) {
		dns_journal_destroy(&j);
		return (ISC_R_SUCCESS);
	}

	CHECK(journal_open(mctx, newname, ISC_TRUE, ISC_TRUE, &new));

	/*
	 * Remove overhead so space test below can succeed.
	 */
	if (target_size >= indexend)
		target_size -= indexend;

	/*
	 * Find if we can create enough free space.
	 */
	best_guess = j->header.begin;
	for (i = 0; i < j->header.index_size; i++) {
		if (POS_VALID(j->index[i]) &&
		    DNS_SERIAL_GE(serial, j->index[i].serial) &&
		    ((isc_uint32_t)(j->header.end.offset - j->index[i].offset)
		     >= target_size / 2) &&
		    j->index[i].offset > best_guess.offset)
			best_guess = j->index[i];
	}

	current_pos = best_guess;
	while (current_pos.serial != serial) {
		CHECK(journal_next(j, &current_pos));
		if (current_pos.serial == j->header.end.serial)
			break;

		if (DNS_SERIAL_GE(serial, current_pos.serial) &&
		   ((isc_uint32_t)(j->header.end.offset - current_pos.offset)
		     >= (target_size / 2)) &&
		    current_pos.offset > best_guess.offset)
			best_guess = current_pos;
		else
			break;
	}

	INSIST(best_guess.serial != j->header.end.serial);
	if (best_guess.serial != serial)
		CHECK(journal_next(j, &best_guess));

	/*
	 * We should now be roughly half target_size provided
	 * we did not reach 'serial'.  If not we will just copy
	 * all uncommitted deltas regardless of the size.
	 */
	copy_length = j->header.end.offset - best_guess.offset;

	if (copy_length != 0) {
		/*
		 * Copy best_guess to end into space just freed.
		 */
		size = 64*1024;
		if (copy_length < size)
			size = copy_length;
		buf = isc_mem_get(mctx, size);
		if (buf == NULL) {
			result = ISC_R_NOMEMORY;
			goto failure;
		}

		CHECK(journal_seek(j, best_guess.offset));
		CHECK(journal_seek(new, indexend));
		for (i = 0; i < copy_length; i += size) {
			unsigned int len = (copy_length - i) > size ? size :
							 (copy_length - i);
			CHECK(journal_read(j, buf, len));
			CHECK(journal_write(new, buf, len));
		}

		CHECK(journal_fsync(new));

		/*
		 * Compute new header.
		 */
		new->header.begin.serial = best_guess.serial;
		new->header.begin.offset = indexend;
		new->header.end.serial = j->header.end.serial;
		new->header.end.offset = indexend + copy_length;

		/*
		 * Update the journal header.
		 */
		journal_header_encode(&new->header, &rawheader);
		CHECK(journal_seek(new, 0));
		CHECK(journal_write(new, &rawheader, sizeof(rawheader)));
		CHECK(journal_fsync(new));

		/*
		 * Build new index.
		 */
		current_pos = new->header.begin;
		while (current_pos.serial != new->header.end.serial) {
			index_add(new, &current_pos);
			CHECK(journal_next(new, &current_pos));
		}

		/*
		 * Write index.
		 */
		CHECK(index_to_disk(new));
		CHECK(journal_fsync(new));

		indexend = new->header.end.offset;
	}
	dns_journal_destroy(&new);

	/*
	 * With a UFS file system this should just succeed and be atomic.
	 * Any IXFR outs will just continue and the old journal will be
	 * removed on final close.
	 *
	 * With MSDOS / NTFS we need to do a two stage rename triggered
	 * bu EEXISTS.  Hopefully all IXFR's that were active at the last
	 * rename are now complete.
	 */
	if (rename(newname, filename) == -1) {
		if (errno == EACCES && !is_backup) {
			result = isc_file_remove(backup);
			if (result != ISC_R_SUCCESS &&
			    result != ISC_R_FILENOTFOUND)
				goto failure;
			if (rename(filename, backup) == -1)
				goto maperrno;
			if (rename(newname, filename) == -1)
				goto maperrno;
			(void)isc_file_remove(backup);
		} else {
 maperrno:
			result = ISC_R_FAILURE;
			goto failure;
		}
	}

	dns_journal_destroy(&j);
	result = ISC_R_SUCCESS;

 failure:
	(void)isc_file_remove(newname);
	if (buf != NULL)
		isc_mem_put(mctx, buf, size);
	if (j != NULL)
		dns_journal_destroy(&j);
	if (new != NULL)
		dns_journal_destroy(&new);
	return (result);
}

static isc_result_t
index_to_disk(dns_journal_t *j) {
	isc_result_t result = ISC_R_SUCCESS;

	if (j->header.index_size != 0) {
		unsigned int i;
		unsigned char *p;
		unsigned int rawbytes;

		rawbytes = j->header.index_size * sizeof(journal_rawpos_t);

		p = j->rawindex;
		for (i = 0; i < j->header.index_size; i++) {
			encode_uint32(j->index[i].serial, p);
			p += 4;
			encode_uint32(j->index[i].offset, p);
			p += 4;
		}
		INSIST(p == j->rawindex + rawbytes);

		CHECK(journal_seek(j, sizeof(journal_rawheader_t)));
		CHECK(journal_write(j, j->rawindex, rawbytes));
	}
failure:
	return (result);
}
