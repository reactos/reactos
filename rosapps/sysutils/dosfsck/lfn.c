/* lfn.c  -  Functions for handling VFAT long filenames */

/* Written 1998 by Roman Hodek */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "common.h"
#include "io.h"
#include "dosfsck.h"
#include "lfn.h"
#include "file.h"

typedef struct {
	__u8    id;		/* sequence number for slot */
	__u8    name0_4[10];	/* first 5 characters in name */
	__u8    attr;		/* attribute byte */
	__u8    reserved;	/* always 0 */
	__u8    alias_checksum;	/* checksum for 8.3 alias */
	__u8    name5_10[12];	/* 6 more characters in name */
	__u16   start;		/* starting cluster number, 0 in long slots */
	__u8    name11_12[4];	/* last 2 characters in name */
} LFN_ENT;

#define LFN_ID_START	0x40
#define LFN_ID_SLOTMASK	0x1f

#define CHARS_PER_LFN	13

/* These modul-global vars represent the state of the LFN parser */
unsigned char *lfn_unicode = NULL;
unsigned char lfn_checksum;
int lfn_slot = -1;
loff_t *lfn_offsets = NULL;
int lfn_parts = 0;

static unsigned char fat_uni2esc[64] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z', '+', '-'
};

/* This defines which unicode chars are directly convertable to ISO-8859-1 */
#define UNICODE_CONVERTABLE(cl,ch)	(ch == 0 && (cl < 0x80 || cl >= 0xa0))

/* for maxlen param */
#define UNTIL_0		INT_MAX

/* Convert name part in 'lfn' from unicode to ASCII */
#define CNV_THIS_PART(lfn)				\
    ({							\
	char __part_uni[CHARS_PER_LFN*2];		\
	copy_lfn_part( __part_uni, lfn );		\
	cnv_unicode( __part_uni, CHARS_PER_LFN, 0 );	\
    })
    
/* Convert name parts collected so far (from previous slots) from unicode to
 * ASCII */
#define CNV_PARTS_SO_FAR()					\
	(cnv_unicode( lfn_unicode+(lfn_slot*CHARS_PER_LFN*2),	\
		      lfn_parts*CHARS_PER_LFN, 0 ))

/* This function converts an unicode string to a normal ASCII string, assuming
 * ISO-8859-1 charset. Characters not in 8859-1 are converted to the same
 * escape notation as used by the kernel, i.e. the uuencode-like ":xxx" */
static char *cnv_unicode( const unsigned char *uni, int maxlen, int use_q )
{
    const unsigned char *up;
    unsigned char *out, *cp;
    int len, val;
    
    for( len = 0, up = uni; (up-uni)/2 < maxlen && (up[0] || up[1]); up += 2 ){
	if (UNICODE_CONVERTABLE(up[0],up[1]))
	    ++len;
	else
	    len += 4;
    }
    cp = out = use_q ? qalloc( &mem_queue, len+1 ) : alloc( len+1 );

    for( up = uni; (up-uni)/2 < maxlen && (up[0] || up[1]); up += 2 ) {
	if (UNICODE_CONVERTABLE(up[0],up[1]))
	    *cp++ = up[0];
	else {
	    /* here the same escape notation is used as in the Linux kernel */
	    *cp++ = ':';
	    val = (up[1] << 8) + up[0];
	    cp[2] = fat_uni2esc[val & 0x3f];
	    val >>= 6;
	    cp[1] = fat_uni2esc[val & 0x3f];
	    val >>= 6;
	    cp[0] = fat_uni2esc[val & 0x3f];
	    cp += 3;
	}
    }
    *cp = 0;

    return( out );
}


static void copy_lfn_part( char *dst, LFN_ENT *lfn )
{
    memcpy( dst,    lfn->name0_4,   10 );
    memcpy( dst+10, lfn->name5_10,  12 );
    memcpy( dst+22, lfn->name11_12, 4 );
}


static void clear_lfn_slots( int start, int end )
{
    int i;
    LFN_ENT empty;

    /* New dir entry is zeroed except first byte, which is set to 0xe5.
     * This is to avoid that some FAT-reading OSes (not Linux! ;) stop reading
     * a directory at the first zero entry...
     */
    memset( &empty, 0, sizeof(empty) );
    empty.id = DELETED_FLAG;
    
    for( i = start; i <= end; ++i ) {
	fs_write( lfn_offsets[i], sizeof(LFN_ENT), &empty );
    }
}

void lfn_reset( void )
{
    if (lfn_unicode)
	free( lfn_unicode );
    lfn_unicode = NULL;
    if (lfn_offsets)
	free( lfn_offsets );
    lfn_offsets = NULL;
    lfn_slot = -1;
}


/* This function is only called with de->attr == VFAT_LN_ATTR. It stores part
 * of the long name. */
void lfn_add_slot( DIR_ENT *de, loff_t dir_offset )
{
    LFN_ENT *lfn = (LFN_ENT *)de;
    unsigned offset;

    if (de->attr != VFAT_LN_ATTR)
	die("lfn_add_slot called with non-LFN directory entry");

    if (lfn->id & LFN_ID_START) {
	if (lfn_slot != -1) {
	    int can_clear = 0;
	    /* There is already a LFN "in progess", so it is an error that a
	     * new start entry is here. */
	    /* Causes: 1) if slot# == expected: start bit set mysteriously, 2)
	     *         old LFN overwritten by new one */
	    /* Fixes: 1) delete previous LFN 2) if slot# == expected and
	     *        checksum ok: clear start bit */
	    /* XXX: Should delay that until next LFN known (then can better
	     * display the name) */
	    printf( "A new long file name starts within an old one.\n" );
	    if ((lfn->id & LFN_ID_SLOTMASK) == lfn_slot &&
		lfn->alias_checksum == lfn_checksum) {
		char *part1 = CNV_THIS_PART(lfn);
		char *part2 = CNV_PARTS_SO_FAR();
		printf( "  It could be that the LFN start bit is wrong here\n"
			"  if \"%s\" seems to match \"%s\".\n", part1, part2 );
		free( part1 );
		free( part2 );
		can_clear = 1;
	    }
	    if (interactive) {
		printf( "1: Delete previous LFN\n2: Leave it as it is.\n" );
		if (can_clear)
		    printf( "3: Clear start bit and concatenate LFNs\n" );
	    }
	    else printf( "  Not auto-correcting this.\n" );
	    if (interactive) {
		switch( get_key( can_clear ? "123" : "12", "?" )) {
		  case '1':
		    clear_lfn_slots( 0, lfn_parts-1 );
		    lfn_reset();
		    break;
		  case '2':
		    break;
		  case '3':
		    lfn->id &= ~LFN_ID_START;
		    fs_write( dir_offset+offsetof(LFN_ENT,id),
			      sizeof(lfn->id), &lfn->id );
		    break;
		}
	    }
	}
	lfn_slot = lfn->id & LFN_ID_SLOTMASK;
	lfn_checksum = lfn->alias_checksum;
	lfn_unicode = alloc( (lfn_slot*CHARS_PER_LFN+1)*2 );
	lfn_offsets = alloc( lfn_slot*sizeof(loff_t) );
	lfn_parts = 0;
    }
    else if (lfn_slot == -1) {
	/* No LFN in progress, but slot found; start bit missing */
	/* Causes: 1) start bit got lost, 2) Previous slot with start bit got
	 *         lost */
	/* Fixes: 1) delete LFN, 2) set start bit */
	char *part = CNV_THIS_PART(lfn);
	printf( "Long filename fragment \"%s\" found outside a LFN "
		"sequence.\n  (Maybe the start bit is missing on the "
		"last fragment)\n", part );
	if (interactive) {
	    printf( "1: Delete fragment\n2: Leave it as it is.\n"
		    "3: Set start bit\n" );
	}
	else printf( "  Not auto-correcting this.\n" );
	if (interactive) {
	    switch( get_key( "123", "?" )) {
	      case '1':
		if (!lfn_offsets)
		    lfn_offsets = alloc( sizeof(loff_t) );
		lfn_offsets[0] = dir_offset;
		clear_lfn_slots( 0, 0 );
		lfn_reset();
		return;
	      case '2':
		lfn_reset();
		return;
	      case '3':
		lfn->id |= LFN_ID_START;
		fs_write( dir_offset+offsetof(LFN_ENT,id),
			  sizeof(lfn->id), &lfn->id );
		lfn_slot = lfn->id & LFN_ID_SLOTMASK;
		lfn_checksum = lfn->alias_checksum;
		lfn_unicode = alloc( (lfn_slot*CHARS_PER_LFN+1)*2 );
		lfn_offsets = alloc( lfn_slot*sizeof(loff_t) );
		lfn_parts = 0;
		break;
	    }
	}
    }
    else if ((lfn->id & LFN_ID_SLOTMASK) != lfn_slot) {
	/* wrong sequence number */
	/* Causes: 1) seq-no destroyed */
	/* Fixes: 1) delete LFN, 2) fix number (maybe only if following parts
	 *        are ok?, maybe only if checksum is ok?) (Attention: space
	 *        for name was allocated before!) */
	int can_fix = 0;
	printf( "Unexpected long filename sequence number "
		"(%d vs. expected %d).\n",
		(lfn->id & LFN_ID_SLOTMASK), lfn_slot );
	if (lfn->alias_checksum == lfn_checksum) {
	    char *part1 = CNV_THIS_PART(lfn);
	    char *part2 = CNV_PARTS_SO_FAR();
	    printf( "  It could be that just the number is wrong\n"
		    "  if \"%s\" seems to match \"%s\".\n", part1, part2 );
	    free( part1 );
	    free( part2 );
	    can_fix = 1;
	}
	if (interactive) {
	    printf( "1: Delete LFN\n2: Leave it as it is (and ignore LFN so far)\n" );
	    if (can_fix)
		printf( "3: Correct sequence number\n" );
	}
	else printf( "  Not auto-correcting this.\n" );
	if (interactive) {
	    switch( get_key( can_fix ? "123" : "12", "?" )) {
	      case '1':
		lfn_offsets[lfn_parts++] = dir_offset;
		clear_lfn_slots( 0, lfn_parts-1 );
		lfn_reset();
		return;
	      case '2':
		lfn_reset();
		return;
	      case '3':
		lfn->id = (lfn->id & ~LFN_ID_SLOTMASK) | lfn_slot;
		fs_write( dir_offset+offsetof(LFN_ENT,id),
			  sizeof(lfn->id), &lfn->id );
		break;
	    }
	}
    }

    if (lfn->alias_checksum != lfn_checksum) {
	/* checksum mismatch */
	/* Causes: 1) checksum field here destroyed */
	/* Fixes: 1) delete LFN, 2) fix checksum */
	printf( "Checksum in long filename part wrong "
		"(%02x vs. expected %02x).\n",
		lfn->alias_checksum, lfn_checksum );
	if (interactive) {
	    printf( "1: Delete LFN\n2: Leave it as it is.\n"
		    "3: Correct checksum\n" );
	}
	else printf( "  Not auto-correcting this.\n" );
	if (interactive) {
	    switch( get_key( "123", "?" )) {
	      case '1':
		lfn_offsets[lfn_parts++] = dir_offset;
		clear_lfn_slots( 0, lfn_parts-1 );
		lfn_reset();
		return;
	      case '2':
		break;
	      case '3':
		lfn->alias_checksum = lfn_checksum;
		fs_write( dir_offset+offsetof(LFN_ENT,alias_checksum),
			  sizeof(lfn->alias_checksum), &lfn->alias_checksum );
		break;
	    }
	}
    }

    if (lfn_slot != -1) {
	lfn_slot--;
	offset = lfn_slot * CHARS_PER_LFN*2;
	copy_lfn_part( lfn_unicode+offset, lfn );
	if (lfn->id & LFN_ID_START)
	    lfn_unicode[offset+26] = lfn_unicode[offset+27] = 0;
	lfn_offsets[lfn_parts++] = dir_offset;
    }

    if (lfn->reserved != 0) {
	printf( "Reserved field in VFAT long filename slot is not 0 "
		"(but 0x%02x).\n", lfn->reserved );
	if (interactive)
	    printf( "1: Fix.\n2: Leave it.\n" );
	else printf( "Auto-setting to 0.\n" );
	if (!interactive || get_key("12","?") == '1') {
	    lfn->reserved = 0;
	    fs_write( dir_offset+offsetof(LFN_ENT,reserved),
		      sizeof(lfn->reserved), &lfn->reserved );
	}
    }
    if (lfn->start != CT_LE_W(0)) {
	printf( "Start cluster field in VFAT long filename slot is not 0 "
		"(but 0x%04x).\n", lfn->start );
	if (interactive)
	    printf( "1: Fix.\n2: Leave it.\n" );
	else printf( "Auto-setting to 0.\n" );
	if (!interactive || get_key("12","?") == '1') {
	    lfn->start = CT_LE_W(0);
	    fs_write( dir_offset+offsetof(LFN_ENT,start),
		      sizeof(lfn->start),&lfn->start );
	}
    }
}


/* This function is always called when de->attr != VFAT_LN_ATTR is found, to
 * retrieve the previously constructed LFN. */
char *lfn_get( DIR_ENT *de )
{
    char *lfn;
    __u8 sum;
    int i;
    
    if (de->attr == VFAT_LN_ATTR)
	die("lfn_get called with LFN directory entry");

#if 0
    if (de->lcase)
	printf( "lcase=%02x\n",de->lcase );
#endif

    if (lfn_slot == -1)
	/* no long name for this file */
	return NULL;

    if (lfn_slot != 0) {
	/* The long name isn't finished yet. */
	/* Causes: 1) LFN slot overwritten by non-VFAT aware tool */
	/* Fixes: 1) delete LFN 2) move overwriting entry to somewhere else
	 * and let user enter missing part of LFN (hard to do :-()
	 * 3) renumber entries and truncate name */
	char *long_name = CNV_PARTS_SO_FAR();
	char *short_name = file_name(de->name);
	printf( "Unfinished long file name \"%s\".\n"
		"  (Start may have been overwritten by %s)\n",
		long_name, short_name );
	free( long_name );
	if (interactive) {
	    printf( "1: Delete LFN\n2: Leave it as it is.\n"
		    "3: Fix numbering (truncates long name and attaches "
		    "it to short name %s)\n", short_name );
	}
	else printf( "  Not auto-correcting this.\n" );
	if (interactive) {
	    switch( get_key( "123", "?" )) {
	      case '1':
		clear_lfn_slots( 0, lfn_parts-1 );
		lfn_reset();
		return NULL;
	      case '2':
		lfn_reset();
		return NULL;
	      case '3':
		for( i = 0; i < lfn_parts; ++i ) {
		    __u8 id = (lfn_parts-i) | (i==0 ? LFN_ID_START : 0);
		    fs_write( lfn_offsets[i]+offsetof(LFN_ENT,id),
			      sizeof(id), &id );
		}
		memmove( lfn_unicode, lfn_unicode+lfn_slot*CHARS_PER_LFN*2,
			 lfn_parts*CHARS_PER_LFN*2 );
		break;
	    }
	}
    }

    for (sum = 0, i = 0; i < 11; i++)
	sum = (((sum&1) << 7) | ((sum&0xfe) >> 1)) + de->name[i];
    if (sum != lfn_checksum) {
	/* checksum doesn't match, long name doesn't apply to this alias */
	/* Causes: 1) alias renamed */
	/* Fixes: 1) Fix checksum in LFN entries */
	char *long_name = CNV_PARTS_SO_FAR();
	char *short_name = file_name(de->name);
	printf( "Wrong checksum for long file name \"%s\".\n"
		"  (Short name %s may have changed without updating the long name)\n",
		long_name, short_name );
	free( long_name );
	if (interactive) {
	    printf( "1: Delete LFN\n2: Leave it as it is.\n"
		    "3: Fix checksum (attaches to short name %s)\n",
		    short_name );
	}
	else printf( "  Not auto-correcting this.\n" );
	if (interactive) {
	    switch( get_key( "123", "?" )) {
	      case '1':
		clear_lfn_slots( 0, lfn_parts-1 );
		lfn_reset();
		return NULL;
	      case '2':
		lfn_reset();
		return NULL;
	      case '3':
		for( i = 0; i < lfn_parts; ++i ) {
		    fs_write( lfn_offsets[i]+offsetof(LFN_ENT,alias_checksum),
			      sizeof(sum), &sum );
		}
		break;
	    }
	}
    }

    lfn = cnv_unicode( lfn_unicode, UNTIL_0, 1 );
    lfn_reset();
    return( lfn );
}

void lfn_check_orphaned(void)
{
    char *long_name;

    if (lfn_slot == -1)
	return;

    long_name = CNV_PARTS_SO_FAR();
    printf("Orphaned long file name part \"%s\"\n", long_name);
    if (interactive)
	printf( "1: Delete.\n2: Leave it.\n" );
    else printf( "  Auto-deleting.\n" );
    if (!interactive || get_key("12","?") == '1') {
	clear_lfn_slots(0, lfn_parts - 1);
    }
    lfn_reset();
}

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
