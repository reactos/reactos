/*
	id3: ID3v2.3 and ID3v2.4 parsing (a relevant subset)

	copyright 2006-2020 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis

	WIP: Handling of multiple ID3 tags in a stream.

	1. With update flag: Add non-unique data, replace unique.
	- Only one TALB, TPE1, etc.
	- Only one TXXX with a certain description.
	- Only one COMM with certain language and description.
	- Only one APIC with certain type and description, generally only one
	  of type 1 and 2 each.
	2. Without update flag: wipe whole data and only store new stuff.

	BIG BAD BUT: How to properly handle seeks in a stream that make
	the parser encounter the same tags again in random order? Is
	there even a correct way to handle that without storing an
	ordered list of all tags? I could simplify the code here and just
	communicate that a frame should be an update to previous, and
	at which stream position the frame was encountered. But since
	libmpg123 is driven by MPEG frames, there could be multiple
	ID3v2 tags in direct succession treated by the parser without
	the library user being able to interfere.

	This is severely fucked. All that complexity also doesn't matter
	in practice, as streams use ICY and individual files have just one
	ID3v2 tag (relevant for libmpg123).  It's an academic problem. But
	for seekable files, I could implement some jumping logic to find
	and parse all ID3v2 for once and then set a flag that only jumps
	the frames on seeks. That covers all local disk playback. For
	streams, seeking is no issue (seeking back, at least), so the
	update/replace logic works.

	Look at the standard:

------
5.   Tag location

   The default location of an ID3v2 tag is prepended to the audio so
   that players can benefit from the information when the data is
   streamed. It is however possible to append the tag, or make a
   prepend/append combination. When deciding upon where an unembedded
   tag should be located, the following order of preference SHOULD be
   considered.
   
     1. Prepend the tag.

     2. Prepend a tag with all vital information and add a second tag at 
        the end of the file, before tags from other tagging systems. The
        first tag is required to have a SEEK frame.
      
     3. Add a tag at the end of the file, before tags from other tagging
        systems.
      
   In case 2 and 3 the tag can simply be appended if no other known tags
   are present. The suggested method to find ID3v2 tags are:
   
     1. Look for a prepended tag using the pattern found in section 3.1.

     2. If a SEEK frame was found, use its values to guide further
        searching.

     3. Look for a tag footer, scanning from the back of the file.

   For every new tag that is found, the old tag should be discarded
   unless the update flag in the extended header (section 3.2) is set.
------

	For seekable streams, I simply need to implement explicit ID3v2 search along
	that recommendation and keep the complete information. Streams that continue
	growing during playback will not recognize added ID3v2 tags. So be it.
	For non-seekable streams, a tag is always parsed when encountered, assuming
	the order of update tags always matches.

	First step for the 1.26 release shall be the implementaton of the update
	logic and glossing over the theoretical problem of re-parsing update
	frames in the wrong order by ignoring it. They are not that relevant.

	TODO: Cave in and add the missing frames from the spec. Not that far to go.
	But need another data structure to communicate those ...
*/

#include "mpg123lib_intern.h"
#include "id3.h"
#include "debug.h"

#ifndef NO_ID3V2 /* Only the main parsing routine will always be there. */

/* We know the usual text frames plus some specifics. */
#define KNOWN_FRAMES 5
static const char frame_type[KNOWN_FRAMES][5] = { "COMM", "TXXX", "RVA2", "USLT", "APIC" };
enum frame_types { unknown = -2, text = -1, comment, extra, rva2, uslt, picture };

/* UTF support definitions */

typedef void (*text_converter)(mpg123_string *sb, const unsigned char* source, size_t len, const int noquiet);

static void convert_latin1  (mpg123_string *sb, const unsigned char* source, size_t len, const int noquiet);
static void convert_utf16bom(mpg123_string *sb, const unsigned char* source, size_t len, const int noquiet);
static void convert_utf8    (mpg123_string *sb, const unsigned char* source, size_t len, const int noquiet);

static const text_converter text_converters[4] =
{
	convert_latin1,
	/* We always check for (multiple) BOM in 16bit unicode. Without BOM, UTF16 BE is the default.
	   Errors in encoding are detected anyway. */
	convert_utf16bom,
	convert_utf16bom,
	convert_utf8
};

static const unsigned int encoding_widths[4] = { 1, 2, 2, 1 };

/* the code starts here... */

static void null_id3_links(mpg123_handle *fr)
{
	fr->id3v2.title  = NULL;
	fr->id3v2.artist = NULL;
	fr->id3v2.album  = NULL;
	fr->id3v2.year   = NULL;
	fr->id3v2.genre  = NULL;
	fr->id3v2.comment = NULL;
}

void init_id3(mpg123_handle *fr)
{
	fr->id3v2.version = 0; /* nothing there */
	null_id3_links(fr);
	fr->id3v2.comments     = 0;
	fr->id3v2.comment_list = NULL;
	fr->id3v2.texts    = 0;
	fr->id3v2.text     = NULL;
	fr->id3v2.extras   = 0;
	fr->id3v2.extra    = NULL;
	fr->id3v2.pictures   = 0;
	fr->id3v2.picture    = NULL;
}

/* Managing of the text, comment and extra lists. */

/* Initialize one element. */
static void init_mpg123_text(mpg123_text *txt)
{
	mpg123_init_string(&txt->text);
	mpg123_init_string(&txt->description);
	txt->id[0] = 0;
	txt->id[1] = 0;
	txt->id[2] = 0;
	txt->id[3] = 0;
	txt->lang[0] = 0;
	txt->lang[1] = 0;
	txt->lang[2] = 0;
}

static void init_mpg123_picture(mpg123_picture *pic)
{
	mpg123_init_string(&pic->mime_type);
	mpg123_init_string(&pic->description);
	pic->type = 0;
	pic->size = 0;
	pic->data = NULL;
}

/* Free memory of one element. */
static void free_mpg123_text(mpg123_text *txt)
{
	mpg123_free_string(&txt->text);
	mpg123_free_string(&txt->description);
}

static void free_mpg123_picture(mpg123_picture * pic)
{
	mpg123_free_string(&pic->mime_type);
	mpg123_free_string(&pic->description);
	if (pic->data != NULL)
		free(pic->data);
}

/* Free memory of whole list. */
#define free_comment(mh) free_id3_text(&((mh)->id3v2.comment_list), &((mh)->id3v2.comments))
#define free_text(mh)    free_id3_text(&((mh)->id3v2.text),         &((mh)->id3v2.texts))
#define free_extra(mh)   free_id3_text(&((mh)->id3v2.extra),        &((mh)->id3v2.extras))
#define free_picture(mh) free_id3_picture(&((mh)->id3v2.picture),   &((mh)->id3v2.pictures))
static void free_id3_text(mpg123_text **list, size_t *size)
{
	size_t i;
	for(i=0; i<*size; ++i) free_mpg123_text(&((*list)[i]));

	free(*list);
	*list = NULL;
	*size = 0;
}
static void free_id3_picture(mpg123_picture **list, size_t *size)
{
	size_t i;
	for(i=0; i<*size; ++i) free_mpg123_picture(&((*list)[i]));

	free(*list);
	*list = NULL;
	*size = 0;
}

/* Add items to the list. */

#define add_comment(mh, l, d) add_id3_text(&((mh)->id3v2.comment_list), &((mh)->id3v2.comments), NULL,    l, d)
#define add_text(mh, id)      add_id3_text(&((mh)->id3v2.text),         &((mh)->id3v2.texts),      id, NULL, NULL)
#define add_uslt(mh, l, d)    add_id3_text(&((mh)->id3v2.text),         &((mh)->id3v2.texts),      id, l, d)
#define add_extra(mh, d)      add_id3_text(&((mh)->id3v2.extra),        &((mh)->id3v2.extras),   NULL, NULL, d)
#define add_picture(mh, t, d) add_id3_picture(&((mh)->id3v2.picture), &((mh)->id3v2.pictures), t, d)
static mpg123_text *add_id3_text( mpg123_text **list, size_t *size
,	char id[4], char lang[3], mpg123_string *description )
{
	mdebug( "add_id3_text id=%s lang=%s, desc=%s"
	,	id ? (char[5]) { id[0], id[1], id[2], id[3], 0 } : "(nil)"
	,	lang ? (char[4]) { lang[0], lang[1], lang[2], 0 }  : "(nil)"
	,	description ? (description->fill ? description->p : "(empty)") : "(nil)" );
	if(lang && !description)
		return NULL; // no lone language intended
	if(id || description)
	{
		// Look through list of existing texts and return an existing entry
		// if it should be overwritten.
		for(size_t i=0; i<*size; ++i)
		{
			mpg123_text *entry = *list+i;
			if(description)
			{ // Overwrite entry with same description and same ID and language.
				if( (!id || !memcmp(id, entry->id, 4))
					&& (!lang || !memcmp(entry->lang, lang, 3))
					&& mpg123_same_string(&(entry->description), description)
				)
				return entry;
			} else if(id && !memcmp(id, entry->id, 4))
				return entry; // Just ovewrite because of same ID.
			mdebug("add_id3_text: entry %zu was no match", i);
		}
	}
	mdebug("add_id3_text: append to list of %zu", *size);
	// Nothing found, add new one.
	mpg123_text *x = safe_realloc(*list, sizeof(mpg123_text)*(*size+1));
	if(x == NULL) return NULL; /* bad */

	*list  = x;
	*size += 1;
	init_mpg123_text(&((*list)[*size-1]));

	return &((*list)[*size-1]); /* Return pointer to the added text. */
}


static mpg123_picture *add_id3_picture(mpg123_picture **list, size_t *size, char type, mpg123_string *description)
{
	if(!description)
		return NULL;

	// Return entry to overwrite, if appropriate.
	for(size_t i=0; i<*size; ++i)
	{
		mpg123_picture *entry = *list+i;
		if(  type == entry->type
			&& ( type == 1 || type == 2 ||
				mpg123_same_string(&entry->description, description)
			)
		)
			return entry;
	}
	// Append a new one.
	mpg123_picture *x = safe_realloc(*list, sizeof(mpg123_picture)*(*size+1));
	if(x == NULL) return NULL; /* bad */

	*list  = x;
	*size += 1;
	init_mpg123_picture(&((*list)[*size-1]));

	return &((*list)[*size-1]); /* Return pointer to the added picture. */
}

/* OK, back to the higher level functions. */

void exit_id3(mpg123_handle *fr)
{
	free_picture(fr);
	free_comment(fr);
	free_extra(fr);
	free_text(fr);
}

void reset_id3(mpg123_handle *fr)
{
	exit_id3(fr);
	init_id3(fr);
}

/* Set the id3v2.artist id3v2.title ... links to elements of the array. */
void id3_link(mpg123_handle *fr)
{
	size_t i;
	mpg123_id3v2 *v2 = &fr->id3v2;
	debug("linking ID3v2");
	null_id3_links(fr);
	for(i=0; i<v2->texts; ++i)
	{
		mpg123_text *entry = &v2->text[i];
		if     (!strncmp("TIT2", entry->id, 4)) v2->title  = &entry->text;
		else if(!strncmp("TALB", entry->id, 4)) v2->album  = &entry->text;
		else if(!strncmp("TPE1", entry->id, 4)) v2->artist = &entry->text;
		else if(!strncmp("TYER", entry->id, 4)) v2->year   = &entry->text;
		else if(!strncmp("TCON", entry->id, 4)) v2->genre  = &entry->text;
	}
	for(i=0; i<v2->comments; ++i)
	{
		mpg123_text *entry = &v2->comment_list[i];
		if(entry->description.fill == 0 || entry->description.p[0] == 0)
		v2->comment = &entry->text;
	}
	/* When no generic comment found, use the last non-generic one. */
	if(v2->comment == NULL && v2->comments > 0)
	v2->comment = &v2->comment_list[v2->comments-1].text;
}

/*
	Store ID3 text data in an mpg123_string; either verbatim copy or
	everything translated to UTF-8 encoding.
	Preserve the zero string separator (I don't need strlen for the total size).

	Since we can overwrite strings with ID3 update frames, don't free
	memory, just grow strings.
*/
static void store_id3_text(mpg123_string *sb, unsigned char *source, size_t source_size, const int noquiet, const int notranslate)
{
	unsigned char encoding;
	if(sb) // Always overwrite, even with nothing.
		sb->fill = 0;
	if(!source_size)
	{
		debug("Empty id3 data!");
		return;
	}

	/* We shall just copy the data. Client wants to decode itself. */
	if(notranslate)
	{
		/* Future: Add a path for ID3 errors. */
		if(!mpg123_grow_string(sb, source_size))
		{
			if(noquiet) error("Cannot resize target string, out of memory?");
			return;
		}
		memcpy(sb->p, source, source_size);
		sb->fill = source_size;
		debug1("stored undecoded ID3 text of size %"SIZE_P, (size_p)source_size);
		return;
	}

	encoding = source[0];
	if(encoding > mpg123_id3_enc_max)
	{
		if(noquiet)
			error1("Unknown text encoding %u, I take no chances, sorry!", encoding);
		return;
	}
	id3_to_utf8(sb, encoding, source+1, source_size-1, noquiet);

	if(sb->fill) debug1("UTF-8 string (the first one): %s", sb->p);
	else if(noquiet) error("unable to convert string to UTF-8 (out of memory, junk input?)!");
}

/* On error, sb->size is 0. */
/* Also, encoding has been checked already! */
void id3_to_utf8(mpg123_string *sb, unsigned char encoding, const unsigned char *source, size_t source_size, int noquiet)
{
	unsigned int bwidth;
	if(sb)
		sb->fill = 0;
	debug1("encoding: %u", encoding);
	/* A note: ID3v2.3 uses UCS-2 non-variable 16bit encoding, v2.4 uses UTF16.
	   UTF-16 uses a reserved/private range in UCS-2 to add the magic, so we just always treat it as UTF. */
	bwidth = encoding_widths[encoding];
	/* Hack! I've seen a stray zero byte before BOM. Is that supposed to happen? */
	if(encoding != mpg123_id3_utf16be) /* UTF16be _can_ beging with a null byte! */
	while(source_size > bwidth && source[0] == 0)
	{
		--source_size;
		++source;
		debug("skipped leading zero");
	}
	if(source_size % bwidth)
	{
		/* When we need two bytes for a character, it's strange to have an uneven bytestream length. */
		if(noquiet) warning2("Weird tag size %d for encoding %u - I will probably trim too early or something but I think the MP3 is broken.", (int)source_size, encoding);
		source_size -= source_size % bwidth;
	}
	text_converters[encoding](sb, source, source_size, noquiet);
}

/* You have checked encoding to be in the range already. */
static unsigned char *next_text(unsigned char* prev, unsigned char encoding, size_t limit)
{
	unsigned char *text = prev;
	size_t width = encoding_widths[encoding];

	/* So I go lengths to find zero or double zero...
	   Remember bug 2834636: Only check for aligned NULLs! */
	while(text-prev < (ssize_t)limit)
	{
		if(text[0] == 0)
		{
			if(width <= limit-(text-prev))
			{
				size_t i = 1;
				for(; i<width; ++i) if(text[i] != 0) break;

				if(i == width) /* found a null wide enough! */
				{
					text += width;
					break;
				}
			}
			else return NULL; /* No full character left? This text is broken */
		}

		text += width;
	}
	if((size_t)(text-prev) >= limit) text = NULL;

	return text;
}

static const char *enc_name(unsigned char enc)
{
	switch(enc)
	{
		case 0:  return "Latin 1";
		case 1:  return "UTF-16 BOM";
		case 2:  return "UTF-16 BE";
		case 3:  return "UTF-8";
		default: return "unknown!";
	}
}

static void process_text(mpg123_handle *fr, unsigned char *realdata, size_t realsize, char *id)
{
	/* Text encoding          $xx */
	/* The text (encoded) ... */
	mpg123_text *t = add_text(fr, id);
	if(VERBOSE4) fprintf(stderr, "Note: Storing text from %s encoding\n", enc_name(realdata[0]));
	if(t == NULL)
	{
		if(NOQUIET) error("Unable to attach new text!");
		return;
	}
	mdebug("process_text: (over)writing entry with ID %s", t->id
	?	(char[5]) { t->id[0], t->id[1], t->id[2], t->id[3], 0 }
	:	"(nil)" );
	memcpy(t->id, id, 4);
	store_id3_text(&t->text, realdata, realsize, NOQUIET, fr->p.flags & MPG123_PLAIN_ID3TEXT);
	if(VERBOSE4) // Do not print unsanitized text to terminals!
		fprintf(stderr, "Note: ID3v2 %c%c%c%c text frame stored\n", id[0], id[1], id[2], id[3]);
}

static void process_picture(mpg123_handle *fr, unsigned char *realdata, size_t realsize)
{
	unsigned char encoding;
	mpg123_picture *i = NULL;
	unsigned char* workpoint = NULL;
	mpg123_string mime; mpg123_init_string(&mime);
	unsigned char image_type = 0;
	mpg123_string description;	mpg123_init_string(&description);
	unsigned char *image_data = NULL;
	if(realsize < 1)
	{
		debug("Empty id3 data!");
		return;
	}
	encoding = realdata[0];
	realdata++; realsize--;
	if(encoding > mpg123_id3_enc_max)
	{
		if(NOQUIET)
			error1("Unknown text encoding %u, I take no chances, sorry!", encoding);
		return;
	}
	if(VERBOSE4) fprintf(stderr, "Note: Storing picture from APIC frame.\n");

	/* get mime type (encoding is always latin-1) */
	workpoint = next_text(realdata, 0, realsize);
	if(!workpoint)
	{
		if(NOQUIET)
			error("Unable to get mime type for picture; skipping picture.");
		return;
	}
	id3_to_utf8(&mime, 0, realdata, workpoint - realdata, NOQUIET);
	realsize -= workpoint - realdata;
	realdata = workpoint;
	/* get picture type */
	image_type = realdata[0];
	realdata++; realsize--;
	/* get description (encoding is encoding) */
	workpoint = next_text(realdata, encoding, realsize);
	if(!workpoint)
	{
		if(NOQUIET)
			error("Unable to get description for picture; skipping picture.");
		mpg123_free_string(&mime);
		return;
	}
	id3_to_utf8(&description, encoding, realdata, workpoint - realdata, NOQUIET);
	realsize -= workpoint - realdata;
	if(realsize)
		image_data = (unsigned char*)malloc(realsize);
	if(!realsize || !image_data) {
		if(NOQUIET)
			error("No picture data or malloc failure; skipping picture.");
		mpg123_free_string(&description);
		mpg123_free_string(&mime);
		return;
	}
	memcpy(image_data, workpoint, realsize);

	// All data ready now, append to/replace in list.
	i = add_picture(fr, image_type, &description);
	if(!i)
	{
		if(NOQUIET)
			error("Unable to attach new picture!");
		free(image_data);
		mpg123_free_string(&description);
		mpg123_free_string(&mime);
		return;
	}

	// Either this is a fresh image, or one to be replaced.
	// We hand over memory, so old storage needs to be freed.
	free_mpg123_picture(i);
	i->type = image_type;
	i->size = realsize;
	i->data = image_data;
	mpg123_move_string(&mime, &i->mime_type);
	mpg123_move_string(&description, &i->description);
	if(VERBOSE4)
		fprintf(stderr, "Note: ID3v2 APIC picture frame of type: %d\n", i->type);
}

/* Store a new comment that perhaps is a RVA / RVA_ALBUM/AUDIOPHILE / RVA_MIX/RADIO one
   Special gimmik: It also stores USLT to the texts. Stucture is the same as for comments. */
static void process_comment(mpg123_handle *fr, enum frame_types tt, unsigned char *realdata, size_t realsize, int rva_level, char *id)
{
	/* Text encoding          $xx */
	/* Language               $xx xx xx */
	/* Short description (encoded!)      <text> $00 (00) */
	/* Then the comment text (encoded) ... */
	unsigned char  encoding = realdata[0];
	char lang[3]; // realdata + 1
	unsigned char *descr    = realdata+4;
	unsigned char *text     = NULL;
	mpg123_text *xcom = NULL;
	mpg123_text localcom; // UTF-8 variant for local processing, remember to clean up!
	init_mpg123_text(&localcom);

	if(realsize < (size_t)(descr-realdata))
	{
		if(NOQUIET) error1("Invalid frame size of %"SIZE_P" (too small for anything).", (size_p)realsize);
		return;
	}
	if(encoding > mpg123_id3_enc_max)
	{
		if(NOQUIET)
			error1("Unknown text encoding %u, I take no chances, sorry!", encoding);
		return;
	}
	memcpy(lang, realdata+1, 3);
	/* Now I can abuse a byte from lang for the encoding. */
	descr[-1] = encoding;
	/* Be careful with finding the end of description, I have to honor encoding here. */
	text = next_text(descr, encoding, realsize-(descr-realdata));
	if(text == NULL)
	{
		if(NOQUIET)
			error("No comment text / valid description?");
		return;
	}
	{ // just vor variable scope
		mpg123_string description;
		mpg123_init_string(&description);
		// Store the text, with desired encoding, but for comments always a local copy in UTF-8.
		store_id3_text( &description, descr-1, text-descr+1
		,	NOQUIET, fr->p.flags & MPG123_PLAIN_ID3TEXT );
		if(tt == comment)
			store_id3_text( &localcom.description, descr-1, text-descr+1
			,	NOQUIET, 0 );
		if(VERBOSE4)
			fprintf( stderr, "Note: Storing comment from %s encoding\n"
			,	enc_name(realdata[0]) );
		xcom = tt == uslt
		?	add_uslt(fr, lang, &description)
		:	add_comment(fr, lang, &description);
		if(xcom == NULL)
		{
			if(NOQUIET)
				error("Unable to attach new comment!");
			mpg123_free_string(&description);
			free_mpg123_text(&localcom);
			return;
		}
		memcpy(xcom->id, id, 4);
		memcpy(xcom->lang, lang, 3);
		// That takes over the description allocation.
		mpg123_move_string(&description, &xcom->description);
	}

	text[-1] = encoding; /* Byte abusal for encoding... */
	store_id3_text(&xcom->text, text-1, realsize+1-(text-realdata), NOQUIET, fr->p.flags & MPG123_PLAIN_ID3TEXT);
	/* Remember: I will probably decode the above (again) for rva comment checking. So no messing around, please. */

	if(VERBOSE4) /* Do _not_ print the verbatim text: The encoding might be funny! */
	{
		fprintf(stderr, "Note: ID3 comm/uslt desc of length %"SIZE_P".\n", (size_p)xcom->description.fill);
		fprintf(stderr, "Note: ID3 comm/uslt text of length %"SIZE_P".\n", (size_p)xcom->text.fill);
	}
	/* Look out for RVA info only when we really deal with a straight comment. */
	if(tt == comment && localcom.description.fill > 0)
	{
		int rva_mode = -1; /* mix / album */
		if(    !strcasecmp(localcom.description.p, "rva")
			 || !strcasecmp(localcom.description.p, "rva_mix")
			 || !strcasecmp(localcom.description.p, "rva_track")
			 || !strcasecmp(localcom.description.p, "rva_radio") )
		rva_mode = 0;
		else if(    !strcasecmp(localcom.description.p, "rva_album")
		         || !strcasecmp(localcom.description.p, "rva_audiophile")
		         || !strcasecmp(localcom.description.p, "rva_user") )
		rva_mode = 1;
		if((rva_mode > -1) && (fr->rva.level[rva_mode] <= rva_level))
		{
			/* Only translate the contents in here where we really need them. */
			store_id3_text(&localcom.text, text-1, realsize+1-(text-realdata), NOQUIET, 0);
			if(localcom.text.fill > 0)
			{
				fr->rva.gain[rva_mode] = (float) atof(localcom.text.p);
				if(VERBOSE3) fprintf(stderr, "Note: RVA value %fdB\n", fr->rva.gain[rva_mode]);
				fr->rva.peak[rva_mode] = 0;
				fr->rva.level[rva_mode] = rva_level;
			}
		}
	}
	/* Make sure to free the local memory... */
	free_mpg123_text(&localcom);
}

static void process_extra(mpg123_handle *fr, unsigned char* realdata, size_t realsize, int rva_level, char *id)
{
	/* Text encoding          $xx */
	/* Description        ... $00 (00) */
	/* Text ... */
	unsigned char encoding = realdata[0];
	unsigned char *descr   = realdata+1; /* remember, the encoding is descr[-1] */
	unsigned char *text;
	mpg123_text *xex;
	mpg123_text localex;

	if((int)realsize < descr-realdata)
	{
		if(NOQUIET) error1("Invalid frame size of %lu (too small for anything).", (unsigned long)realsize);
		return;
	}
	if(encoding > mpg123_id3_enc_max)
	{
		if(NOQUIET)
			error1("Unknown text encoding %u, I take no chances, sorry!", encoding);
		return;
	}
	text = next_text(descr, encoding, realsize-(descr-realdata));
	if(VERBOSE4) fprintf(stderr, "Note: Storing extra from %s encoding\n", enc_name(realdata[0]));
	if(text == NULL)
	{
		if(NOQUIET) error("No extra frame text / valid description?");
		return;
	}
	{ // just vor variable scope
		mpg123_string description;
		mpg123_init_string(&description);
		/* The outside storage gets reencoded to UTF-8 only if not requested otherwise.
		   Remember that we really need the -1 here to hand in the encoding byte!*/
		store_id3_text( &description, descr-1, text-descr+1
		,	NOQUIET, fr->p.flags & MPG123_PLAIN_ID3TEXT );
		xex = add_extra(fr, &description);
		if(xex)
			mpg123_move_string(&description, &xex->description);
		else
			mpg123_free_string(&description);
	}
	if(xex == NULL)
	{
		if(NOQUIET) error("Unable to attach new extra text!");
		return;
	}
	memcpy(xex->id, id, 4);
	init_mpg123_text(&localex); /* For our local copy. */

	/* Our local copy is always stored in UTF-8! */
	store_id3_text(&localex.description, descr-1, text-descr+1, NOQUIET, 0);
	/* At first, only store the outside copy of the payload. We may not need the local copy. */
	text[-1] = encoding;
	store_id3_text(&xex->text, text-1, realsize-(text-realdata)+1, NOQUIET, fr->p.flags & MPG123_PLAIN_ID3TEXT);

	/* Now check if we would like to interpret this extra info for RVA. */
	if(localex.description.fill > 0)
	{
		int is_peak = 0;
		int rva_mode = -1; /* mix / album */

		if(!strncasecmp(localex.description.p, "replaygain_track_",17))
		{
			if(VERBOSE3) fprintf(stderr, "Note: RVA ReplayGain track gain/peak\n");

			rva_mode = 0;
			if(!strcasecmp(localex.description.p, "replaygain_track_peak")) is_peak = 1;
			else if(strcasecmp(localex.description.p, "replaygain_track_gain")) rva_mode = -1;
		}
		else
		if(!strncasecmp(localex.description.p, "replaygain_album_",17))
		{
			if(VERBOSE3) fprintf(stderr, "Note: RVA ReplayGain album gain/peak\n");

			rva_mode = 1;
			if(!strcasecmp(localex.description.p, "replaygain_album_peak")) is_peak = 1;
			else if(strcasecmp(localex.description.p, "replaygain_album_gain")) rva_mode = -1;
		}
		if((rva_mode > -1) && (fr->rva.level[rva_mode] <= rva_level))
		{
			/* Now we need the translated copy of the data. */
			store_id3_text(&localex.text, text-1, realsize-(text-realdata)+1, NOQUIET, 0);
			if(localex.text.fill > 0)
			{
				if(is_peak)
				{
					fr->rva.peak[rva_mode] = (float) atof(localex.text.p);
					if(VERBOSE3) fprintf(stderr, "Note: RVA peak %f\n", fr->rva.peak[rva_mode]);
				}
				else
				{
					fr->rva.gain[rva_mode] = (float) atof(localex.text.p);
					if(VERBOSE3) fprintf(stderr, "Note: RVA gain %fdB\n", fr->rva.gain[rva_mode]);
				}
				fr->rva.level[rva_mode] = rva_level;
			}
		}
	}

	free_mpg123_text(&localex);
}

/* Make a ID3v2.3+ 4-byte ID from a ID3v2.2 3-byte ID
   Note that not all frames survived to 2.4; the mapping goes to 2.3 .
   A notable miss is the old RVA frame, which is very unspecific anyway.
   This function returns -1 when a not known 3 char ID was encountered, 0 otherwise. */
static int promote_framename(mpg123_handle *fr, char *id) /* fr because of VERBOSE macros */
{
	size_t i;
	char *old[] =
	{
		"COM",  "TAL",  "TBP",  "TCM",  "TCO",  "TCR",  "TDA",  "TDY",  "TEN",  "TFT",
		"TIM",  "TKE",  "TLA",  "TLE",  "TMT",  "TOA",  "TOF",  "TOL",  "TOR",  "TOT",
		"TP1",  "TP2",  "TP3",  "TP4",  "TPA",  "TPB",  "TRC",  "TDA",  "TRK",  "TSI",
		"TSS",  "TT1",  "TT2",  "TT3",  "TXT",  "TXX",  "TYE"
	};
	char *new[] =
	{
		"COMM", "TALB", "TBPM", "TCOM", "TCON", "TCOP", "TDAT", "TDLY", "TENC", "TFLT",
		"TIME", "TKEY", "TLAN", "TLEN", "TMED", "TOPE", "TOFN", "TOLY", "TORY", "TOAL",
		"TPE1", "TPE2", "TPE3", "TPE4", "TPOS", "TPUB", "TSRC", "TRDA", "TRCK", "TSIZ",
		"TSSE", "TIT1", "TIT2", "TIT3", "TEXT", "TXXX", "TYER"
	};
	for(i=0; i<sizeof(old)/sizeof(char*); ++i)
	{
		if(!strncmp(id, old[i], 3))
		{
			memcpy(id, new[i], 4);
			if(VERBOSE3) fprintf(stderr, "Translated ID3v2.2 frame %s to %s\n", old[i], new[i]);
			return 0;
		}
	}
	if(VERBOSE3) fprintf(stderr, "Ignoring untranslated ID3v2.2 frame %c%c%c\n", id[0], id[1], id[2]);
	return -1;
}

#endif /* NO_ID3V2 */

int store_id3v2( mpg123_handle *fr
,	unsigned long first4bytes, unsigned char buf[6], unsigned long length )
{
	int ret = 1;
	off_t ret2;
	unsigned long fullen = 10+length;
	if(fr->id3v2_raw)
		free(fr->id3v2_raw);
	fr->id3v2_size = 0;
	/* Allocate one byte more for a closing zero as safety catch for strlen(). */
	fr->id3v2_raw = malloc(fullen+1);
	if(!fr->id3v2_raw)
	{
		fr->err = MPG123_OUT_OF_MEM;
		if(NOQUIET)
			error1("ID3v2: Arrg! Unable to allocate %lu bytes"
				" for ID3v2 data - trying to skip instead.", length+1);
		if((ret2=fr->rd->skip_bytes(fr,length)) < 0)
			ret = ret2;
		else
			ret = 0;
	}
	else
	{
		fr->id3v2_raw[0] = (first4bytes>>24) & 0xff;
		fr->id3v2_raw[1] = (first4bytes>>16) & 0xff;
		fr->id3v2_raw[2] = (first4bytes>>8)  & 0xff;
		fr->id3v2_raw[3] =  first4bytes      & 0xff;
		memcpy(fr->id3v2_raw+4, buf, 6);
		if((ret2=fr->rd->read_frame_body(fr, fr->id3v2_raw+10, length)) < 0)
		{
			ret=ret2;
			free(fr->id3v2_raw);
			fr->id3v2_raw = NULL;
		}
		else
		{ /* Closing with a zero for paranoia. */
			fr->id3v2_raw[fullen] = 0;
			fr->id3v2_size = fullen;
		}
	}
	return ret;
}

/*
	trying to parse ID3v2.3 and ID3v2.4 tags...

	returns:  0: bad or just unparseable tag
	          1: good, (possibly) new tag info
	         <0: reader error (may need more data feed, try again)
*/
int parse_new_id3(mpg123_handle *fr, unsigned long first4bytes)
{
	#define UNSYNC_FLAG 128
	#define EXTHEAD_FLAG 64  /* ID3v2.3+ */
	#define COMPRESS_FLAG 64 /* ID3v2.2 */
	#define EXP_FLAG 32
	#define FOOTER_FLAG 16
	#define EXT_UPDATE_FLAG 64 /* ID3v2.4 only: extended header update flag */
	#define UNKNOWN_FLAGS 15 /* 00001111*/
	unsigned char buf[6];
	unsigned long length=0;
	unsigned char flags = 0;
	int ret = 1;
	off_t ret2;
	int storetag = 0;
	unsigned int footlen = 0;
#ifndef NO_ID3V2
	int skiptag = 0;
#endif
	unsigned char major = first4bytes & 0xff;
	debug1("ID3v2: major tag version: %i", major);

	if(major == 0xff) return 0; /* Invalid... */
	if((ret2 = fr->rd->read_frame_body(fr, buf, 6)) < 0) /* read more header information */
	return ret2;

	if(buf[0] == 0xff) return 0; /* Revision, will never be 0xff. */

	if(fr->p.flags & MPG123_STORE_RAW_ID3)
		storetag = 1;
	/* second new byte are some nice flags, if these are invalid skip the whole thing */
	flags = buf[1];
	debug1("ID3v2: flags 0x%08x", flags);
	/* use 4 bytes from buf to construct 28bit uint value and return 1; return 0 if bytes are not synchsafe */
	#define synchsafe_to_long(buf,res) \
	( \
		(((buf)[0]|(buf)[1]|(buf)[2]|(buf)[3]) & 0x80) ? 0 : \
		(res =  (((unsigned long) (buf)[0]) << 21) \
		     | (((unsigned long) (buf)[1]) << 14) \
		     | (((unsigned long) (buf)[2]) << 7) \
		     |  ((unsigned long) (buf)[3]) \
		,1) \
	)
	/* id3v2.3 does not store synchsafe frame sizes, but synchsafe tag size - doh! */
	/* Remember: bytes_to_long() can yield ULONG_MAX on 32 bit platforms! */
	#define bytes_to_long(buf,res) \
	( \
		major == 3 ? \
		(res =  (((unsigned long) (buf)[0]) << 24) \
		     | (((unsigned long) (buf)[1]) << 16) \
		     | (((unsigned long) (buf)[2]) << 8) \
		     |  ((unsigned long) (buf)[3]) \
		,1) : synchsafe_to_long(buf,res) \
	)
	/* for id3v2.2 only */
	#define threebytes_to_long(buf,res) \
	( \
		res =  (((unsigned long) (buf)[0]) << 16) \
		     | (((unsigned long) (buf)[1]) << 8) \
		     |  ((unsigned long) (buf)[2]) \
	)

	/* length-10 or length-20 (footer present); 4 synchsafe integers == 28 bit number  */
	/* we have already read 10 bytes, so left are length or length+10 bytes belonging to tag */
	/* Note: This is an 28 bit value in 32 bit storage, plenty of space for */
	/* length+x for reasonable x. */
	if(!synchsafe_to_long(buf+2,length))
	{
		if(NOQUIET) error4("Bad tag length (not synchsafe): 0x%02x%02x%02x%02x; You got a bad ID3 tag here.", buf[2],buf[3],buf[4],buf[5]);
		return 0;
	}
	if(flags & FOOTER_FLAG)
		footlen = 10;
	debug1("ID3v2: tag data length %lu", length);
#ifndef NO_ID3V2
	if(VERBOSE2) fprintf(stderr,"Note: ID3v2.%i rev %i tag of %lu bytes\n", major, buf[0], length);
	/* skip if unknown version/scary flags, parse otherwise */
	if(fr->p.flags & MPG123_SKIP_ID3V2)
	{
		if(VERBOSE3)
			fprintf(stderr, "Note: Skipping ID3v2 tag per user request.\n");
		skiptag = 1;
	}
	if((flags & UNKNOWN_FLAGS) || (major > 4) || (major < 2))
	{
		if(NOQUIET)
			warning2( "ID3v2: Won't parse the ID3v2 tag with major version"
				" %u and flags 0x%xu - some extra code may be needed"
			,	major, flags );
		skiptag = 1;
	}
	// Standard says that compressed tags should be ignored as there isn't an agreed
	// compressoion scheme.
	if(major == 2 && flags & COMPRESS_FLAG)
	{
		if(NOQUIET)
			warning("ID3v2: ignoring compressed ID3v2.2 tag");
		skiptag = 1;
	}
	if(length < 10)
	{
		if(NOQUIET)
			warning1("ID3v2: unrealistic small tag lengh %lu, skipping", length);
		skiptag = 1;
	}
	if(!skiptag)
		storetag = 1;
#endif
	if(storetag)
	{
		/* Stores whole tag with footer and an additonal trailing zero. */
		if((ret2 = store_id3v2(fr, first4bytes, buf, length+footlen)) <= 0)
			return ret2;
	}
#ifndef NO_ID3V2
	if(skiptag)
	{
		if(VERBOSE3)
			fprintf(stderr, "Note: skipped tag clearing possibly existing ID3v2 data");
		reset_id3(fr); // Old data is invalid.
#endif
		if(!storetag && (ret2=fr->rd->skip_bytes(fr,length+footlen))<0)
			ret=ret2;
#ifndef NO_ID3V2
	}
	else
	{
		unsigned char* tagdata = fr->id3v2_raw+10;
		/* try to interpret that beast */
		debug("ID3v2: analysing frames...");
		if(length > 0)
		{
			unsigned char extflags = 0;
			unsigned long tagpos = 0;
			/* bytes of frame title and of framesize value */
			unsigned int head_part = major > 2 ? 4 : 3;
			unsigned int flag_part = major > 2 ? 2 : 0;
			/* The amount of bytes that are unconditionally read for each frame: */
			/* ID, size, flags. */
			unsigned int framebegin = head_part+head_part+flag_part;
			debug1("ID3v2: have read at all %lu bytes for the tag now", (unsigned long)length+6);
			if(flags & EXTHEAD_FLAG)
			{
				debug("ID3v2: extended header");
				if(!bytes_to_long(tagdata, tagpos) || tagpos >= length)
				{
					ret = 0;
					if(NOQUIET)
						error4( "Bad (non-synchsafe/too large) tag offset from extended header:"
							"0x%02x%02x%02x%02x"
						,	tagdata[0], tagdata[1], tagdata[2], tagdata[3] );
				} else if(tagpos < 6)
				{
					ret = 0;
					if(NOQUIET)
						merror("Extended header too small (%lu).", tagpos);
				}
				if(major == 3)
				{
					tagpos += 4; // The size itself is not included.
					if(tagpos >= length)
					{
						ret = 0;
						if(NOQUIET)
							error("Too much extended v2.3 header.");
					}
				} else if(ret) // v2.4 and at least got my 6 bytes of ext header
				{
					// Only v4 knows update frames, check for that.
					// Need to step back. Header is 4 bytes length, one byte flag size,
					// one byte flags. Flag size has to be 1!
					if(tagdata[4] == 1 && tagdata[5] & EXT_UPDATE_FLAG)
					{
						if(VERBOSE3)
							fprintf(stderr, "Note: ID3v2.4 update tag\n");
						extflags |= EXT_UPDATE_FLAG;
					}
				}
			}
			if(!(extflags & EXT_UPDATE_FLAG))
			{
				if(VERBOSE3)
					fprintf(stderr, "Note: non-update tag replacing existing ID3v2 data\n");
				reset_id3(fr);
			}
			if(ret > 0)
			{
				char id[5];
				unsigned long framesize;
				unsigned long fflags; /* need 16 bits, actually */
				id[4] = 0;
				fr->id3v2.version = major;
				/* Pos now advanced after ext head, now a frame has to follow. */
				/* Note: tagpos <= length, which is 28 bit integer, so both */
				/* far away from overflow for adding known small values. */
				/* I want to read at least one full header now. */
				while(length >= tagpos+framebegin)
				{
					int i = 0;
					unsigned long pos = tagpos;
					/* level 1,2,3 - 0 is info from lame/info tag! */
					/* rva tags with ascending significance, then general frames */
					enum frame_types tt = unknown;
					/* we may have entered the padding zone or any other strangeness: check if we have valid frame id characters */
					for(i=0; i< head_part; ++i)
					if( !( ((tagdata[tagpos+i] > 47) && (tagdata[tagpos+i] < 58))
						 || ((tagdata[tagpos+i] > 64) && (tagdata[tagpos+i] < 91)) ) )
					{
						debug5("ID3v2: real tag data apparently ended after %lu bytes with 0x%02x%02x%02x%02x", tagpos, tagdata[tagpos], tagdata[tagpos+1], tagdata[tagpos+2], tagdata[tagpos+3]);
						/* This is no hard error... let's just hope that we got something meaningful already (ret==1 in that case). */
						goto tagparse_cleanup; /* Need to escape two loops here. */
					}
					if(ret > 0)
					{
						/* 4 or 3 bytes id */
						strncpy(id, (char*) tagdata+pos, head_part);
						id[head_part] = 0; /* terminate for 3 or 4 bytes */
						pos += head_part;
						tagpos += head_part;
						/* size as 32 bits or 28 bits */
						if(fr->id3v2.version == 2) threebytes_to_long(tagdata+pos, framesize);
						else
						if(!bytes_to_long(tagdata+pos, framesize))
						{
							/* Just assume that up to now there was some good data. */
							if(NOQUIET) error1("ID3v2: non-syncsafe size of %s frame, skipping the remainder of tag", id);
							break;
						}
						if(VERBOSE3) fprintf(stderr, "Note: ID3v2 %s frame of size %lu\n", id, framesize);
						tagpos += head_part;
						pos += head_part;
						if(fr->id3v2.version > 2)
						{
							fflags  = (((unsigned long) tagdata[pos]) << 8) | ((unsigned long) tagdata[pos+1]);
							pos    += 2;
							tagpos += 2;
						}
						else fflags = 0;

						if(length - tagpos < framesize)
						{
							if(NOQUIET) error("Whoa! ID3v2 frame claims to be larger than the whole rest of the tag.");
							break;
						}
						tagpos += framesize; /* the important advancement in whole tag */
						/* for sanity, after full parsing tagpos should be == pos */
						/* debug4("ID3v2: found %s frame, size %lu (as bytes: 0x%08lx), flags 0x%016lx", id, framesize, framesize, fflags); */
						/* %0abc0000 %0h00kmnp */
						#define BAD_FFLAGS (unsigned long) 36784
						#define PRES_TAG_FFLAG 16384
						#define PRES_FILE_FFLAG 8192
						#define READ_ONLY_FFLAG 4096
						#define GROUP_FFLAG 64
						#define COMPR_FFLAG 8
						#define ENCR_FFLAG 4
						#define UNSYNC_FFLAG 2
						#define DATLEN_FFLAG 1
						if(head_part < 4 && promote_framename(fr, id) != 0) continue;

						/* shall not or want not handle these */
						if(fflags & (BAD_FFLAGS | COMPR_FFLAG | ENCR_FFLAG))
						{
							if(NOQUIET) warning("ID3v2: skipping invalid/unsupported frame");
							continue;
						}

						for(i = 0; i < KNOWN_FRAMES; ++i)
						if(!strncmp(frame_type[i], id, 4)){ tt = i; break; }

						if(id[0] == 'T' && tt != extra) tt = text;

						if(tt != unknown)
						{
							int rva_mode = -1; /* mix / album */
							unsigned long realsize = framesize;
							unsigned char* realdata = tagdata+pos;
							unsigned char* unsyncbuffer = NULL;
							if(((flags & UNSYNC_FLAG) || (fflags & UNSYNC_FFLAG)) && framesize > 0)
							{
								unsigned long ipos = 0;
								unsigned long opos = 0;
								debug("Id3v2: going to de-unsync the frame data");
								/* de-unsync: FF00 -> FF; real FF00 is simply represented as FF0000 ... */
								/* damn, that means I have to delete bytes from withing the data block... thus need temporal storage */
								/* standard mandates that de-unsync should always be safe if flag is set */
								realdata = unsyncbuffer = malloc(framesize+1); /* will need <= bytes, plus a safety zero */
								if(realdata == NULL)
								{
									if(NOQUIET) error("ID3v2: unable to allocate working buffer for de-unsync");
									continue;
								}
								/* now going byte per byte through the data... */
								realdata[0] = tagdata[pos];
								opos = 1;
								for(ipos = pos+1; ipos < pos+framesize; ++ipos)
								{
									if(!((tagdata[ipos] == 0) && (tagdata[ipos-1] == 0xff)))
									{
										realdata[opos++] = tagdata[ipos];
									}
								}
								realsize = opos;
								/* Append a zero to keep strlen() safe. */
								realdata[realsize] = 0;
								debug2("ID3v2: de-unsync made %lu out of %lu bytes", realsize, framesize);
							}
							pos = 0; /* now at the beginning again... */
							/* Avoid reading over boundary, even if there is a */
							/* zero byte of padding for safety. */
							if(realsize) switch(tt)
							{
								case comment:
								case uslt:
									process_comment(fr, tt, realdata, realsize, comment+1, id);
								break;
								case extra: /* perhaps foobar2000's work */
									process_extra(fr, realdata, realsize, extra+1, id);
								break;
								case rva2: /* "the" RVA tag */
								{
									/* starts with null-terminated identification */
									if(VERBOSE3) fprintf(stderr, "Note: RVA2 identification \"%s\"\n", realdata);
									/* default: some individual value, mix mode */
									rva_mode = 0;
									if( !strncasecmp((char*)realdata, "album", 5)
										 || !strncasecmp((char*)realdata, "audiophile", 10)
										 || !strncasecmp((char*)realdata, "user", 4))
									rva_mode = 1;
									if(fr->rva.level[rva_mode] <= rva2+1)
									{
										pos += strlen((char*) realdata) + 1;
										debug2("got my pos: %zu - %zu", realsize, pos);
										// channel and two bytes for RVA value
										// pos possibly just past the safety zero, so one more than realsize
										if(pos > realsize || realsize-pos < 3)
										{
											if(NOQUIET)
												error("bad RVA2 tag (truncated?)");
										}
										else if(realdata[pos] == 1)
										{
											++pos;
											/* only handle master channel */
											debug("ID3v2: it is for the master channel");
											/* two bytes adjustment, one byte for bits representing peak - n bytes, eh bits, for peak */
											/* 16 bit signed integer = dB * 512. Do not shift signed integers! Multiply instead.
											   Also no implementation-defined casting. Reinterpret the pointer to signed char, then do
											   proper casting. */
											fr->rva.gain[rva_mode] = (float) (
												((short)((signed char*)realdata)[pos]) * 256 + (short)realdata[pos+1] ) / 512;
											pos += 2;
											if(VERBOSE3) fprintf(stderr, "Note: RVA value %fdB\n", fr->rva.gain[rva_mode]);
											/* heh, the peak value is represented by a number of bits - but in what manner? Skipping that part */
											fr->rva.peak[rva_mode] = 0;
											fr->rva.level[rva_mode] = rva2+1;
										}
									}
								}
								break;
								/* non-rva metainfo, simply store... */
								case text:
									process_text(fr, realdata, realsize, id);
								break;
								case picture:
									if (fr->p.flags & MPG123_PICTURE)
									process_picture(fr, realdata, realsize);

									break;
								default: if(NOQUIET) error1("ID3v2: unknown frame type %i", tt);
							}
							if(unsyncbuffer)
								free(unsyncbuffer);
						}
						#undef BAD_FFLAGS
						#undef PRES_TAG_FFLAG
						#undef PRES_FILE_FFLAG
						#undef READ_ONLY_FFLAG
						#undef GROUP_FFLAG
						#undef COMPR_FFLAG
						#undef ENCR_FFLAG
						#undef UNSYNC_FFLAG
						#undef DATLEN_FFLAG
					}
					else break;
					#undef KNOWN_FRAMES
				}
			} else
			{
				if(VERBOSE3)
					fprintf(stderr, "Note: faulty ID3v2 tag still clearing old data\n");
				reset_id3(fr);
			}
		} else // No new data, but still there was a tag that invalidates old data.
		{
			if(VERBOSE3)
				fprintf(stderr, "Note: empty ID3v2 clearing old data\n");
			reset_id3(fr);
		}
tagparse_cleanup:
		/* Get rid of stored raw data that should not be kept. */
		if(!(fr->p.flags & MPG123_STORE_RAW_ID3))
		{
			free(fr->id3v2_raw);
			fr->id3v2_raw = NULL;
			fr->id3v2_size = 0;
		}
	}
#endif /* NO_ID3V2 */
	return ret;
	#undef UNSYNC_FLAG
	#undef EXTHEAD_FLAG
	#undef COMPRESS_FLAG
	#undef EXP_FLAG
	#undef FOOTER_FLAG
	#undef EXT_UPDATE_FLAG
	#undef UNKOWN_FLAGS
}

#ifndef NO_ID3V2 /* Disabling all the rest... */

static void convert_latin1(mpg123_string *sb, const unsigned char* s, size_t l, const int noquiet)
{
	size_t length = l;
	size_t i;
	unsigned char *p;
	/* determine real length, a latin1 character can at most take 2  in UTF8 */
	for(i=0; i<l; ++i)
	if(s[i] >= 0x80) ++length;

	debug1("UTF-8 length: %lu", (unsigned long)length);
	/* one extra zero byte for paranoia */
	if(!mpg123_grow_string(sb, length+1))
		return;

	p = (unsigned char*) sb->p; /* Signedness doesn't matter but it shows I thought about the non-issue */
	for(i=0; i<l; ++i)
	if(s[i] < 0x80){ *p = s[i]; ++p; }
	else /* two-byte encoding */
	{
		*p     = 0xc0 | (s[i]>>6);
		*(p+1) = 0x80 | (s[i] & 0x3f);
		p+=2;
	}

	sb->p[length] = 0;
	sb->fill = length+1;
}

/*
	Check if we have a byte oder mark(s) there, return:
	-1: little endian
	 0: no BOM
	 1: big endian

	This modifies source and len to indicate the data _after_ the BOM(s).
	Note on nasty data: The last encountered BOM determines the endianness.
	I have seen data with multiple BOMS, namely from "the" id3v2 program.
	Not nice, but what should I do?
*/
static int check_bom(const unsigned char** source, size_t *len)
{
	int this_bom    = 0;
	int further_bom = 0;

	if(*len < 2) return 0;

	if((*source)[0] == 0xff && (*source)[1] == 0xfe)
	this_bom = -1;

	if((*source)[0] == 0xfe && (*source)[1] == 0xff)
	this_bom = 1;

	/* Skip the detected BOM. */
	if(this_bom != 0)
	{
		*source += 2;
		*len    -= 2;
		/* Check for following BOMs. The last one wins! */
		further_bom = check_bom(source, len);
		if(further_bom == 0) return this_bom; /* End of the recursion. */
		else                 return further_bom;
	}
	else return 0;
}

#define FULLPOINT(f,s) ( (((f)&0x3ff)<<10) + ((s)&0x3ff) + 0x10000 )
/* Remember: There's a limit at 0x1ffff. */
#define UTF8LEN(x) ( (x)<0x80 ? 1 : ((x)<0x800 ? 2 : ((x)<0x10000 ? 3 : 4)))
static void convert_utf16bom(mpg123_string *sb, const unsigned char* s, size_t l, const int noquiet)
{
	size_t i;
	size_t n; /* number bytes that make up full pairs */
	unsigned char *p;
	size_t length = 0; /* the resulting UTF-8 length */
	/* Determine real length... extreme case can be more than utf-16 length. */
	size_t high = 0;
	size_t low  = 1;
	int bom_endian;

	debug1("convert_utf16 with length %lu", (unsigned long)l);

	bom_endian = check_bom(&s, &l);
	debug1("UTF16 endianness check: %i", bom_endian);

	if(bom_endian == -1) /* little-endian */
	{
		high = 1; /* The second byte is the high byte. */
		low  = 0; /* The first byte is the low byte. */
	}

	n = (l/2)*2; /* number bytes that make up full pairs */

	/* first: get length, check for errors -- stop at first one */
	for(i=0; i < n; i+=2)
	{
		unsigned long point = ((unsigned long) s[i+high]<<8) + s[i+low];
		if((point & 0xfc00) == 0xd800) /* lead surrogate */
		{
			unsigned short second = (i+3 < l) ? (s[i+2+high]<<8) + s[i+2+low] : 0;
			if((second & 0xfc00) == 0xdc00) /* good... */
			{
				point = FULLPOINT(point,second);
				length += UTF8LEN(point); /* possibly 4 bytes */
				i+=2; /* We overstepped one word. */
			}
			else /* if no valid pair, break here */
			{
				if(noquiet) error2("Invalid UTF16 surrogate pair at %li (0x%04lx).", (unsigned long)i, point);
				n = i; /* Forget the half pair, END! */
				break;
			}
		}
		else length += UTF8LEN(point); /* 1,2 or 3 bytes */
	}

	if(!mpg123_grow_string(sb, length+1))
		return;

	/* Now really convert, skip checks as these have been done just before. */
	p = (unsigned char*) sb->p; /* Signedness doesn't matter but it shows I thought about the non-issue */
	for(i=0; i < n; i+=2)
	{
		unsigned long codepoint = ((unsigned long) s[i+high]<<8) + s[i+low];
		if((codepoint & 0xfc00) == 0xd800) /* lead surrogate */
		{
			unsigned short second = (s[i+2+high]<<8) + s[i+2+low];
			codepoint = FULLPOINT(codepoint,second);
			i+=2; /* We overstepped one word. */
		}
		if(codepoint < 0x80) *p++ = (unsigned char) codepoint;
		else if(codepoint < 0x800)
		{
			*p++ = (unsigned char) (0xc0 | (codepoint>>6));
			*p++ = (unsigned char) (0x80 | (codepoint & 0x3f));
		}
		else if(codepoint < 0x10000)
		{
			*p++ = (unsigned char) (0xe0 | (codepoint>>12));
			*p++ = 0x80 | ((codepoint>>6) & 0x3f);
			*p++ = 0x80 | (codepoint & 0x3f);
		}
		else if (codepoint < 0x200000)
		{
			*p++ = (unsigned char) (0xf0 | codepoint>>18);
			*p++ = (unsigned char) (0x80 | ((codepoint>>12) & 0x3f));
			*p++ = (unsigned char) (0x80 | ((codepoint>>6) & 0x3f));
			*p++ = (unsigned char) (0x80 | (codepoint & 0x3f));
		} /* ignore bigger ones (that are not possible here anyway) */
	}
	sb->p[sb->size-1] = 0; /* paranoia... */
	sb->fill = sb->size;
}
#undef UTF8LEN
#undef FULLPOINT

static void convert_utf8(mpg123_string *sb, const unsigned char* source, size_t len, const int noquiet)
{
	if(mpg123_grow_string(sb, len+1))
	{
		memcpy(sb->p, source, len);
		sb->p[len] = 0;
		sb->fill = len+1;
	}
}

#endif
