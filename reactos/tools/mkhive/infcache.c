/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: infcache.c,v 1.1 2003/04/14 17:18:48 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/infcache.c
 * PURPOSE:         INF file parser that caches contents of INF file in memory
 * PROGRAMMER:      Royce Mitchell III
 *                  Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "mkhive.h"
#include "infcache.h"


#define CONTROL_Z  '\x1a'
#define MAX_SECTION_NAME_LEN  255
#define MAX_FIELD_LEN         511  /* larger fields get silently truncated */
/* actual string limit is MAX_INF_STRING_LENGTH+1 (plus terminating null) under Windows */
#define MAX_STRING_LEN        (MAX_INF_STRING_LENGTH+1)


typedef struct _INFCACHEFIELD
{
  struct _INFCACHEFIELD *Next;
  struct _INFCACHEFIELD *Prev;

  CHAR Data[1];
} INFCACHEFIELD, *PINFCACHEFIELD;


typedef struct _INFCACHELINE
{
  struct _INFCACHELINE *Next;
  struct _INFCACHELINE *Prev;

  LONG FieldCount;

  PCHAR Key;

  PINFCACHEFIELD FirstField;
  PINFCACHEFIELD LastField;

} INFCACHELINE, *PINFCACHELINE;


typedef struct _INFCACHESECTION
{
  struct _INFCACHESECTION *Next;
  struct _INFCACHESECTION *Prev;

  PINFCACHELINE FirstLine;
  PINFCACHELINE LastLine;

  LONG LineCount;

  CHAR Name[1];
} INFCACHESECTION, *PINFCACHESECTION;


typedef struct _INFCACHE
{
  PINFCACHESECTION FirstSection;
  PINFCACHESECTION LastSection;

  PINFCACHESECTION StringsSection;
} INFCACHE, *PINFCACHE;


/* parser definitions */

enum parser_state
{
  LINE_START,      /* at beginning of a line */
  SECTION_NAME,    /* parsing a section name */
  KEY_NAME,        /* parsing a key name */
  VALUE_NAME,      /* parsing a value name */
  EOL_BACKSLASH,   /* backslash at end of line */
  QUOTES,          /* inside quotes */
  LEADING_SPACES,  /* leading spaces */
  TRAILING_SPACES, /* trailing spaces */
  COMMENT,         /* inside a comment */
  NB_PARSER_STATES
};

struct parser
{
  const CHAR        *start;       /* start position of item being parsed */
  const CHAR        *end;         /* end of buffer */
  PINFCACHE         file;         /* file being built */
  enum parser_state state;        /* current parser state */
  enum parser_state stack[4];     /* state stack */
  int               stack_pos;    /* current pos in stack */

  PINFCACHESECTION cur_section;   /* pointer to the section being parsed*/
  PINFCACHELINE    line;          /* current line */
  unsigned int     line_pos;      /* current line position in file */
  unsigned int     error;         /* error code */
  unsigned int     token_len;     /* current token len */
  CHAR token[MAX_FIELD_LEN+1];   /* current token */
};

typedef const CHAR * (*parser_state_func)( struct parser *parser, const CHAR *pos );

/* parser state machine functions */
static const CHAR *line_start_state( struct parser *parser, const CHAR *pos );
static const CHAR *section_name_state( struct parser *parser, const CHAR *pos );
static const CHAR *key_name_state( struct parser *parser, const CHAR *pos );
static const CHAR *value_name_state( struct parser *parser, const CHAR *pos );
static const CHAR *eol_backslash_state( struct parser *parser, const CHAR *pos );
static const CHAR *quotes_state( struct parser *parser, const CHAR *pos );
static const CHAR *leading_spaces_state( struct parser *parser, const CHAR *pos );
static const CHAR *trailing_spaces_state( struct parser *parser, const CHAR *pos );
static const CHAR *comment_state( struct parser *parser, const CHAR *pos );

static const parser_state_func parser_funcs[NB_PARSER_STATES] =
{
  line_start_state,      /* LINE_START */
  section_name_state,    /* SECTION_NAME */
  key_name_state,        /* KEY_NAME */
  value_name_state,      /* VALUE_NAME */
  eol_backslash_state,   /* EOL_BACKSLASH */
  quotes_state,          /* QUOTES */
  leading_spaces_state,  /* LEADING_SPACES */
  trailing_spaces_state, /* TRAILING_SPACES */
  comment_state          /* COMMENT */
};


/* PRIVATE FUNCTIONS ********************************************************/

static PINFCACHELINE
InfpCacheFreeLine (PINFCACHELINE Line)
{
  PINFCACHELINE Next;
  PINFCACHEFIELD Field;

  if (Line == NULL)
    {
      return NULL;
    }

  Next = Line->Next;
  if (Line->Key != NULL)
    {
      free (Line->Key);
      Line->Key = NULL;
    }

  /* Remove data fields */
  while (Line->FirstField != NULL)
    {
      Field = Line->FirstField->Next;
      free (Line->FirstField);
      Line->FirstField = Field;
    }
  Line->LastField = NULL;

  free (Line);

  return Next;
}


static PINFCACHESECTION
InfpCacheFreeSection (PINFCACHESECTION Section)
{
  PINFCACHESECTION Next;

  if (Section == NULL)
    {
      return NULL;
    }

  /* Release all keys */
  Next = Section->Next;
  while (Section->FirstLine != NULL)
    {
      Section->FirstLine = InfpCacheFreeLine (Section->FirstLine);
    }
  Section->LastLine = NULL;

  free (Section);

  return Next;
}


static PINFCACHESECTION
InfpCacheFindSection (PINFCACHE Cache,
		      PCHAR Name)
{
  PINFCACHESECTION Section = NULL;

  if (Cache == NULL || Name == NULL)
    {
      return NULL;
    }

  /* iterate through list of sections */
  Section = Cache->FirstSection;
  while (Section != NULL)
    {
      if (stricmp (Section->Name, Name) == 0)
	{
	  return Section;
	}

      /* get the next section*/
      Section = Section->Next;
    }

  return NULL;
}


static PINFCACHESECTION
InfpCacheAddSection (PINFCACHE Cache,
		     PCHAR Name)
{
  PINFCACHESECTION Section = NULL;
  ULONG Size;

  if (Cache == NULL || Name == NULL)
    {
      DPRINT ("Invalid parameter\n");
      return NULL;
    }

  /* Allocate and initialize the new section */
  Size = sizeof(INFCACHESECTION) + strlen (Name);
  Section = (PINFCACHESECTION)malloc (Size);
  if (Section == NULL)
    {
      DPRINT ("RtlAllocateHeap() failed\n");
      return NULL;
    }
  memset (Section, 0, Size);

  /* Copy section name */
  strcpy (Section->Name, Name);

  /* Append section */
  if (Cache->FirstSection == NULL)
    {
      Cache->FirstSection = Section;
      Cache->LastSection = Section;
    }
  else
    {
      Cache->LastSection->Next = Section;
      Section->Prev = Cache->LastSection;
      Cache->LastSection = Section;
    }

  return Section;
}


static PINFCACHELINE
InfpCacheAddLine (PINFCACHESECTION Section)
{
  PINFCACHELINE Line;

  if (Section == NULL)
    {
      DPRINT("Invalid parameter\n");
      return NULL;
    }

  Line = (PINFCACHELINE)malloc (sizeof(INFCACHELINE));
  if (Line == NULL)
    {
      DPRINT("RtlAllocateHeap() failed\n");
      return NULL;
    }
  memset (Line, 0, sizeof(INFCACHELINE));

  /* Append line */
  if (Section->FirstLine == NULL)
    {
      Section->FirstLine = Line;
      Section->LastLine = Line;
    }
  else
    {
      Section->LastLine->Next = Line;
      Line->Prev = Section->LastLine;
      Section->LastLine = Line;
    }
  Section->LineCount++;

  return Line;
}


static PVOID
InfpAddKeyToLine (PINFCACHELINE Line,
		  PCHAR Key)
{
  if (Line == NULL)
    return NULL;

  if (Line->Key != NULL)
    return NULL;

  Line->Key = (PCHAR)malloc (strlen (Key) + 1);
  if (Line->Key == NULL)
    return NULL;

  strcpy (Line->Key, Key);

  return (PVOID)Line->Key;
}


static PVOID
InfpAddFieldToLine (PINFCACHELINE Line,
		    PCHAR Data)
{
  PINFCACHEFIELD Field;
  ULONG Size;

  Size = sizeof(INFCACHEFIELD) + strlen(Data);
  Field = (PINFCACHEFIELD)malloc (Size);
  if (Field == NULL)
    {
      return NULL;
    }
  memset (Field, 0, Size);
  strcpy (Field->Data, Data);

  /* Append key */
  if (Line->FirstField == NULL)
    {
      Line->FirstField = Field;
      Line->LastField = Field;
    }
  else
    {
      Line->LastField->Next = Field;
      Field->Prev = Line->LastField;
      Line->LastField = Field;
    }
  Line->FieldCount++;

  return (PVOID)Field;
}


static PINFCACHELINE
InfpCacheFindKeyLine (PINFCACHESECTION Section,
		      PCHAR Key)
{
  PINFCACHELINE Line;

  Line = Section->FirstLine;
  while (Line != NULL)
    {
      if (Line->Key != NULL && stricmp (Line->Key, Key) == 0)
	{
	  return Line;
	}

      Line = Line->Next;
    }

  return NULL;
}


/* push the current state on the parser stack */
inline static void push_state( struct parser *parser, enum parser_state state )
{
//  assert( parser->stack_pos < sizeof(parser->stack)/sizeof(parser->stack[0]) );
  parser->stack[parser->stack_pos++] = state;
}


/* pop the current state */
inline static void pop_state( struct parser *parser )
{
//  assert( parser->stack_pos );
  parser->state = parser->stack[--parser->stack_pos];
}


/* set the parser state and return the previous one */
inline static enum parser_state set_state( struct parser *parser, enum parser_state state )
{
  enum parser_state ret = parser->state;
  parser->state = state;
  return ret;
}


/* check if the pointer points to an end of file */
inline static int is_eof( struct parser *parser, const CHAR *ptr )
{
  return (ptr >= parser->end || *ptr == CONTROL_Z);
}


/* check if the pointer points to an end of line */
inline static int is_eol( struct parser *parser, const CHAR *ptr )
{
  return (ptr >= parser->end || *ptr == CONTROL_Z || *ptr == '\r' /*'\n'*/);
}


/* push data from current token start up to pos into the current token */
static int push_token( struct parser *parser, const CHAR *pos )
{
  int len = pos - parser->start;
  const CHAR *src = parser->start;
  CHAR *dst = parser->token + parser->token_len;

  if (len > MAX_FIELD_LEN - parser->token_len)
    len = MAX_FIELD_LEN - parser->token_len;

  parser->token_len += len;
  for ( ; len > 0; len--, dst++, src++)
    *dst = *src ? *src : ' ';
  *dst = 0;
  parser->start = pos;

  return 0;
}



/* add a section with the current token as name */
static PVOID add_section_from_token( struct parser *parser )
{
  PINFCACHESECTION Section;

  if (parser->token_len > MAX_SECTION_NAME_LEN)
    {
      parser->error = STATUS_SECTION_NAME_TOO_LONG;
      return NULL;
    }

  Section = InfpCacheFindSection (parser->file,
				  parser->token);
  if (Section == NULL)
    {
      /* need to create a new one */
      Section= InfpCacheAddSection (parser->file,
				    parser->token);
      if (Section == NULL)
	{
	  parser->error = STATUS_NOT_ENOUGH_MEMORY;
	  return NULL;
	}
    }

  parser->token_len = 0;
  parser->cur_section = Section;

  return (PVOID)Section;
}


/* add a field containing the current token to the current line */
static struct field *add_field_from_token( struct parser *parser, int is_key )
{
  PVOID field;
  CHAR *text;

  if (!parser->line)  /* need to start a new line */
    {
      if (parser->cur_section == NULL)  /* got a line before the first section */
	{
	  parser->error = STATUS_WRONG_INF_STYLE;
	  return NULL;
	}

      parser->line = InfpCacheAddLine (parser->cur_section);
      if (parser->line == NULL)
	goto error;
    }
  else
    {
//      assert(!is_key);
    }

  if (is_key)
    {
      field = InfpAddKeyToLine(parser->line, parser->token);
    }
  else
    {
      field = InfpAddFieldToLine(parser->line, parser->token);
    }

  if (field != NULL)
    {
      parser->token_len = 0;
      return field;
    }

error:
  parser->error = STATUS_NOT_ENOUGH_MEMORY;
  return NULL;
}


/* close the current line and prepare for parsing a new one */
static void close_current_line( struct parser *parser )
{
  parser->line = NULL;
}



/* handler for parser LINE_START state */
static const CHAR *line_start_state( struct parser *parser, const CHAR *pos )
{
  const CHAR *p;

  for (p = pos; !is_eof( parser, p ); p++)
    {
      switch(*p)
	{
//	  case '\n':
	  case '\r':
	    p++;
	    parser->line_pos++;
	    close_current_line( parser );
	    break;
	  case ';':
	    push_state( parser, LINE_START );
	    set_state( parser, COMMENT );
	    return p + 1;
	  case '[':
	    parser->start = p + 1;
	    set_state( parser, SECTION_NAME );
	    return p + 1;
	  default:
	    if (!isspace(*p))
	      {
		parser->start = p;
		set_state( parser, KEY_NAME );
		return p;
	      }
	    break;
	}
    }
  close_current_line( parser );
  return NULL;
}


/* handler for parser SECTION_NAME state */
static const CHAR *section_name_state( struct parser *parser, const CHAR *pos )
{
  const CHAR *p;

  for (p = pos; !is_eol( parser, p ); p++)
    {
      if (*p == ']')
	{
	  push_token( parser, p );
	  if (add_section_from_token( parser ) == NULL)
	    return NULL;
	  push_state( parser, LINE_START );
	  set_state( parser, COMMENT );  /* ignore everything else on the line */
	  return p + 1;
	}
    }
  parser->error = STATUS_BAD_SECTION_NAME_LINE; /* unfinished section name */
  return NULL;
}


/* handler for parser KEY_NAME state */
static const CHAR *key_name_state( struct parser *parser, const CHAR *pos )
{
    const CHAR *p, *token_end = parser->start;

    for (p = pos; !is_eol( parser, p ); p++)
    {
        if (*p == ',') break;
        switch(*p)
        {

         case '=':
            push_token( parser, token_end );
            if (!add_field_from_token( parser, 1 )) return NULL;
            parser->start = p + 1;
            push_state( parser, VALUE_NAME );
            set_state( parser, LEADING_SPACES );
            return p + 1;
        case ';':
            push_token( parser, token_end );
            if (!add_field_from_token( parser, 0 )) return NULL;
            push_state( parser, LINE_START );
            set_state( parser, COMMENT );
            return p + 1;
        case '"':
            push_token( parser, token_end );
            parser->start = p + 1;
            push_state( parser, KEY_NAME );
            set_state( parser, QUOTES );
            return p + 1;
        case '\\':
            push_token( parser, token_end );
            parser->start = p;
            push_state( parser, KEY_NAME );
            set_state( parser, EOL_BACKSLASH );
            return p;
        default:
            if (!isspace(*p)) token_end = p + 1;
            else
            {
                push_token( parser, p );
                push_state( parser, KEY_NAME );
                set_state( parser, TRAILING_SPACES );
                return p;
            }
            break;
        }
    }
    push_token( parser, token_end );
    set_state( parser, VALUE_NAME );
    return p;
}


/* handler for parser VALUE_NAME state */
static const CHAR *value_name_state( struct parser *parser, const CHAR *pos )
{
    const CHAR *p, *token_end = parser->start;

    for (p = pos; !is_eol( parser, p ); p++)
    {
        switch(*p)
        {
        case ';':
            push_token( parser, token_end );
            if (!add_field_from_token( parser, 0 )) return NULL;
            push_state( parser, LINE_START );
            set_state( parser, COMMENT );
            return p + 1;
        case ',':
            push_token( parser, token_end );
            if (!add_field_from_token( parser, 0 )) return NULL;
            parser->start = p + 1;
            push_state( parser, VALUE_NAME );
            set_state( parser, LEADING_SPACES );
            return p + 1;
        case '"':
            push_token( parser, token_end );
            parser->start = p + 1;
            push_state( parser, VALUE_NAME );
            set_state( parser, QUOTES );
            return p + 1;
        case '\\':
            push_token( parser, token_end );
            parser->start = p;
            push_state( parser, VALUE_NAME );
            set_state( parser, EOL_BACKSLASH );
            return p;
        default:
            if (!isspace(*p)) token_end = p + 1;
            else
            {
                push_token( parser, p );
                push_state( parser, VALUE_NAME );
                set_state( parser, TRAILING_SPACES );
                return p;
            }
            break;
        }
    }
    push_token( parser, token_end );
    if (!add_field_from_token( parser, 0 )) return NULL;
    set_state( parser, LINE_START );
    return p;
}


/* handler for parser EOL_BACKSLASH state */
static const CHAR *eol_backslash_state( struct parser *parser, const CHAR *pos )
{
  const CHAR *p;

  for (p = pos; !is_eof( parser, p ); p++)
    {
      switch(*p)
	{
//	  case '\n':
	  case '\r':
	    parser->line_pos++;
//	    parser->start = p + 1;
	    parser->start = p + 2;
	    set_state( parser, LEADING_SPACES );
//	    return p + 1;
	    return p + 2;
	  case '\\':
	    continue;
	  case ';':
	    push_state( parser, EOL_BACKSLASH );
	    set_state( parser, COMMENT );
	    return p + 1;
	  default:
	    if (isspace(*p))
	      continue;
	    push_token( parser, p );
	    pop_state( parser );
	    return p;
	}
    }
  parser->start = p;
  pop_state( parser );

  return p;
}


/* handler for parser QUOTES state */
static const CHAR *quotes_state( struct parser *parser, const CHAR *pos )
{
  const CHAR *p, *token_end = parser->start;

  for (p = pos; !is_eol( parser, p ); p++)
    {
      if (*p == '"')
	{
	  if (p+1 < parser->end && p[1] == '"')  /* double quotes */
	    {
	      push_token( parser, p + 1 );
	      parser->start = token_end = p + 2;
	      p++;
	    }
	  else  /* end of quotes */
	    {
	      push_token( parser, p );
	      parser->start = p + 1;
	      pop_state( parser );
	      return p + 1;
	    }
	}
    }
  push_token( parser, p );
  pop_state( parser );
  return p;
}


/* handler for parser LEADING_SPACES state */
static const CHAR *leading_spaces_state( struct parser *parser, const CHAR *pos )
{
  const CHAR *p;

  for (p = pos; !is_eol( parser, p ); p++)
    {
      if (*p == '\\')
	{
	  parser->start = p;
	  set_state( parser, EOL_BACKSLASH );
	  return p;
	}
      if (!isspace(*p))
	break;
    }
  parser->start = p;
  pop_state( parser );
  return p;
}


/* handler for parser TRAILING_SPACES state */
static const CHAR *trailing_spaces_state( struct parser *parser, const CHAR *pos )
{
  const CHAR *p;

  for (p = pos; !is_eol( parser, p ); p++)
    {
      if (*p == '\\')
	{
	  set_state( parser, EOL_BACKSLASH );
	  return p;
	}
      if (!isspace(*p))
	break;
    }
  pop_state( parser );
  return p;
}


/* handler for parser COMMENT state */
static const CHAR *comment_state( struct parser *parser, const CHAR *pos )
{
  const CHAR *p = pos;

  while (!is_eol( parser, p ))
     p++;
  pop_state( parser );
  return p;
}


/* parse a complete buffer */
static BOOL
InfpParseBuffer (PINFCACHE file,
		 const CHAR *buffer,
		 const CHAR *end,
		 PULONG error_line)
{
  struct parser parser;
  const CHAR *pos = buffer;

  parser.start       = buffer;
  parser.end         = end;
  parser.file        = file;
  parser.line        = NULL;
  parser.state       = LINE_START;
  parser.stack_pos   = 0;
  parser.cur_section = NULL;
  parser.line_pos    = 1;
  parser.error       = 0;
  parser.token_len   = 0;

  /* parser main loop */
  while (pos)
    pos = (parser_funcs[parser.state])(&parser, pos);

  if (parser.error)
    {
      if (error_line)
	*error_line = parser.line_pos;
      return parser.error;
    }

  /* find the [strings] section */
  file->StringsSection = InfpCacheFindSection (file,
					       "Strings");

  return TRUE;
}



/* PUBLIC FUNCTIONS *********************************************************/

BOOL
InfOpenFile(PHINF InfHandle,
	    PCHAR FileName,
	    PULONG ErrorLine)
{
  FILE *File;
  PCHAR FileBuffer;
  ULONG FileLength;
  PINFCACHE Cache;

  *InfHandle = NULL;
  *ErrorLine = (ULONG)-1;

  /* Open the inf file */
  File = fopen (FileName, "rb");
  if (File == NULL)
    {
      DPRINT("fopen() failed\n");
      return FALSE;
    }

  DPRINT("fopen() successful\n");

  /* Query file size */
  fseek (File, 0, SEEK_END);
  FileLength = ftell (File);
  fseek (File, 0, SEEK_SET);

  DPRINT("File size: %lu\n", FileLength);

  /* Allocate file buffer */
  FileBuffer = malloc (FileLength + 1);
  if (FileBuffer == NULL)
    {
      DPRINT1("malloc() failed\n");
      fclose (File);
      return FALSE;
    }

  /* Read file */
  if (fread (FileBuffer, FileLength, 1, File) < 0)
    {
      DPRINT ("fread() failed\n");
      fclose (File);
      free (FileBuffer);
      return FALSE;
    }

  fclose (File);

  /* Append string terminator */
  FileBuffer[FileLength] = 0;

  /* Allocate infcache header */
  Cache = (PINFCACHE)malloc (sizeof(INFCACHE));
  if (Cache == NULL)
    {
      DPRINT("malloc() failed\n");
      free (FileBuffer);
      return FALSE;
    }

  /* Initialize inicache header */
  memset (Cache, 0, sizeof(INFCACHE));

  /* Parse the inf buffer */
  if (!InfpParseBuffer (Cache,
			FileBuffer,
			FileBuffer + FileLength,
			ErrorLine))
    {
      free (Cache);
      free (FileBuffer);
      return FALSE;
    }

  /* Free file buffer */
  free (FileBuffer);

  *InfHandle = (HINF)Cache;

  return TRUE;
}


VOID
InfCloseFile(HINF InfHandle)
{
  PINFCACHE Cache;

  Cache = (PINFCACHE)InfHandle;

  if (Cache == NULL)
    {
      return;
    }

  while (Cache->FirstSection != NULL)
    {
      Cache->FirstSection = InfpCacheFreeSection(Cache->FirstSection);
    }
  Cache->LastSection = NULL;

  free (Cache);
}


BOOL
InfFindFirstLine (HINF InfHandle,
		  PCHAR Section,
		  PCHAR Key,
		  PINFCONTEXT Context)
{
  PINFCACHE Cache;
  PINFCACHESECTION CacheSection;
  PINFCACHELINE CacheLine;

  if (InfHandle == NULL || Section == NULL || Context == NULL)
    {
      DPRINT("Invalid parameter\n");
      return FALSE;
    }

  Cache = (PINFCACHE)InfHandle;

  /* Iterate through list of sections */
  CacheSection = Cache->FirstSection;
  while (Section != NULL)
    {
      DPRINT("Comparing '%s' and '%s'\n", CacheSection->Name, Section);

      /* Are the section names the same? */
      if (stricmp(CacheSection->Name, Section) == 0)
	{
	  if (Key != NULL)
	    {
	      CacheLine = InfpCacheFindKeyLine (CacheSection, (PCHAR)Key);
	    }
	  else
	    {
	      CacheLine = CacheSection->FirstLine;
	    }

	  if (CacheLine == NULL)
	    return FALSE;

	  Context->Inf = (PVOID)Cache;
	  Context->Section = (PVOID)CacheSection;
	  Context->Line = (PVOID)CacheLine;

	  return TRUE;
	}

      /* Get the next section */
      CacheSection = CacheSection->Next;
    }

  DPRINT("Section not found\n");

  return FALSE;
}


BOOL
InfFindNextLine (PINFCONTEXT ContextIn,
		 PINFCONTEXT ContextOut)
{
  PINFCACHELINE CacheLine;

  if (ContextIn == NULL || ContextOut == NULL)
    return FALSE;

  if (ContextIn->Line == NULL)
    return FALSE;

  CacheLine = (PINFCACHELINE)ContextIn->Line;
  if (CacheLine->Next == NULL)
    return FALSE;

  if (ContextIn != ContextOut)
    {
      ContextOut->Inf = ContextIn->Inf;
      ContextOut->Section = ContextIn->Section;
    }
  ContextOut->Line = (PVOID)(CacheLine->Next);

  return TRUE;
}


BOOL
InfFindFirstMatchLine (PINFCONTEXT ContextIn,
		       PCHAR Key,
		       PINFCONTEXT ContextOut)
{
  PINFCACHELINE CacheLine;

  if (ContextIn == NULL || ContextOut == NULL || Key == NULL || *Key == 0)
    return FALSE;

  if (ContextIn->Inf == NULL || ContextIn->Section == NULL)
    return FALSE;

  CacheLine = ((PINFCACHESECTION)(ContextIn->Section))->FirstLine;
  while (CacheLine != NULL)
    {
      if (CacheLine->Key != NULL && stricmp (CacheLine->Key, Key) == 0)
	{

	  if (ContextIn != ContextOut)
	    {
	      ContextOut->Inf = ContextIn->Inf;
	      ContextOut->Section = ContextIn->Section;
	    }
	  ContextOut->Line = (PVOID)CacheLine;

	  return TRUE;
	}

      CacheLine = CacheLine->Next;
    }

  return FALSE;
}


BOOL
InfFindNextMatchLine (PINFCONTEXT ContextIn,
		      PCHAR Key,
		      PINFCONTEXT ContextOut)
{
  PINFCACHELINE CacheLine;

  if (ContextIn == NULL || ContextOut == NULL || Key == NULL || *Key == 0)
    return FALSE;

  if (ContextIn->Inf == NULL || ContextIn->Section == NULL || ContextIn->Line == NULL)
    return FALSE;

  CacheLine = (PINFCACHELINE)ContextIn->Line;
  while (CacheLine != NULL)
    {
      if (CacheLine->Key != NULL && stricmp (CacheLine->Key, Key) == 0)
	{

	  if (ContextIn != ContextOut)
	    {
	      ContextOut->Inf = ContextIn->Inf;
	      ContextOut->Section = ContextIn->Section;
	    }
	  ContextOut->Line = (PVOID)CacheLine;

	  return TRUE;
	}

      CacheLine = CacheLine->Next;
    }

  return FALSE;
}


LONG
InfGetLineCount(HINF InfHandle,
		PCHAR Section)
{
  PINFCACHE Cache;
  PINFCACHESECTION CacheSection;

  if (InfHandle == NULL || Section == NULL)
    {
      DPRINT("Invalid parameter\n");
      return -1;
    }

  Cache = (PINFCACHE)InfHandle;

  /* Iterate through list of sections */
  CacheSection = Cache->FirstSection;
  while (Section != NULL)
    {
      DPRINT("Comparing '%s' and '%s'\n", CacheSection->Name, Section);

      /* Are the section names the same? */
      if (stricmp(CacheSection->Name, Section) == 0)
	{
	  return CacheSection->LineCount;
	}

      /* Get the next section */
      CacheSection = CacheSection->Next;
    }

  DPRINT("Section not found\n");

  return -1;
}


/* InfGetLineText */


LONG
InfGetFieldCount(PINFCONTEXT Context)
{
  if (Context == NULL || Context->Line == NULL)
    return 0;

  return ((PINFCACHELINE)Context->Line)->FieldCount;
}


BOOL
InfGetBinaryField (PINFCONTEXT Context,
		   ULONG FieldIndex,
		   PUCHAR ReturnBuffer,
		   ULONG ReturnBufferSize,
		   PULONG RequiredSize)
{
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  ULONG Index;
  ULONG Size;
  PUCHAR Ptr;

  if (Context == NULL || Context->Line == NULL || FieldIndex == 0)
    {
      DPRINT("Invalid parameter\n");
      return FALSE;
    }

  if (RequiredSize != NULL)
    *RequiredSize = 0;

  CacheLine = (PINFCACHELINE)Context->Line;

  if (FieldIndex > CacheLine->FieldCount)
    return FALSE;

  CacheField = CacheLine->FirstField;
  for (Index = 1; Index < FieldIndex; Index++)
    CacheField = CacheField->Next;

  Size = CacheLine->FieldCount - FieldIndex + 1;

  if (RequiredSize != NULL)
    *RequiredSize = Size;

  if (ReturnBuffer != NULL)
    {
      if (ReturnBufferSize < Size)
	return FALSE;

      /* Copy binary data */
      Ptr = ReturnBuffer;
      while (CacheField != NULL)
	{
	  *Ptr = (UCHAR)strtoul (CacheField->Data, NULL, 16);

	  Ptr++;
	  CacheField = CacheField->Next;
	}
    }

  return TRUE;
}


BOOL
InfGetIntField (PINFCONTEXT Context,
		ULONG FieldIndex,
		PLONG IntegerValue)
{
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  ULONG Index;
  PCHAR Ptr;

  if (Context == NULL || Context->Line == NULL || IntegerValue == NULL)
    {
      DPRINT("Invalid parameter\n");
      return FALSE;
    }

  CacheLine = (PINFCACHELINE)Context->Line;

  if (FieldIndex > CacheLine->FieldCount)
    {
      DPRINT("Invalid parameter\n");
      return FALSE;
    }

  if (FieldIndex == 0)
    {
      Ptr = CacheLine->Key;
    }
  else
    {
      CacheField = CacheLine->FirstField;
      for (Index = 1; Index < FieldIndex; Index++)
	CacheField = CacheField->Next;

      Ptr = CacheField->Data;
    }

  *IntegerValue = strtol (Ptr, NULL, 0);

  return TRUE;
}


BOOL
InfGetMultiSzField (PINFCONTEXT Context,
		    ULONG FieldIndex,
		    PCHAR ReturnBuffer,
		    ULONG ReturnBufferSize,
		    PULONG RequiredSize)
{
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  PINFCACHEFIELD FieldPtr;
  ULONG Index;
  ULONG Size;
  PCHAR Ptr;

  if (Context == NULL || Context->Line == NULL || FieldIndex == 0)
    {
      DPRINT("Invalid parameter\n");
      return FALSE;
    }

  if (RequiredSize != NULL)
    *RequiredSize = 0;

  CacheLine = (PINFCACHELINE)Context->Line;

  if (FieldIndex > CacheLine->FieldCount)
    return FALSE;

  CacheField = CacheLine->FirstField;
  for (Index = 1; Index < FieldIndex; Index++)
    CacheField = CacheField->Next;

  /* Calculate the required buffer size */
  FieldPtr = CacheField;
  Size = 0;
  while (FieldPtr != NULL)
    {
      Size += (strlen (FieldPtr->Data) + 1);
      FieldPtr = FieldPtr->Next;
    }
  Size++;

  if (RequiredSize != NULL)
    *RequiredSize = Size;

  if (ReturnBuffer != NULL)
    {
      if (ReturnBufferSize < Size)
	return FALSE;

      /* Copy multi-sz string */
      Ptr = ReturnBuffer;
      FieldPtr = CacheField;
      while (FieldPtr != NULL)
	{
	  Size = strlen (FieldPtr->Data) + 1;

	  strcpy (Ptr, FieldPtr->Data);

	  Ptr = Ptr + Size;
	  FieldPtr = FieldPtr->Next;
	}
      *Ptr = 0;
    }

  return TRUE;
}


BOOL
InfGetStringField (PINFCONTEXT Context,
		   ULONG FieldIndex,
		   PCHAR ReturnBuffer,
		   ULONG ReturnBufferSize,
		   PULONG RequiredSize)
{
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  ULONG Index;
  PCHAR Ptr;
  ULONG Size;

  if (Context == NULL || Context->Line == NULL || FieldIndex == 0)
    {
      DPRINT("Invalid parameter\n");
      return FALSE;
    }

  if (RequiredSize != NULL)
    *RequiredSize = 0;

  CacheLine = (PINFCACHELINE)Context->Line;

  if (FieldIndex > CacheLine->FieldCount)
    return FALSE;

  if (FieldIndex == 0)
    {
      Ptr = CacheLine->Key;
    }
  else
    {
      CacheField = CacheLine->FirstField;
      for (Index = 1; Index < FieldIndex; Index++)
	CacheField = CacheField->Next;

      Ptr = CacheField->Data;
    }

  Size = wcslen (Ptr) + 1;

  if (RequiredSize != NULL)
    *RequiredSize = Size;

  if (ReturnBuffer != NULL)
    {
      if (ReturnBufferSize < Size)
	return FALSE;

      strcpy (ReturnBuffer, Ptr);
    }

  return TRUE;
}




BOOL
InfGetData (PINFCONTEXT Context,
	    PCHAR *Key,
	    PCHAR *Data)
{
  PINFCACHELINE CacheKey;

  if (Context == NULL || Context->Line == NULL || Data == NULL)
    {
      DPRINT("Invalid parameter\n");
      return FALSE;
    }

  CacheKey = (PINFCACHELINE)Context->Line;
  if (Key != NULL)
    *Key = CacheKey->Key;

  if (Data != NULL)
    {
      if (CacheKey->FirstField == NULL)
	{
	  *Data = NULL;
	}
      else
	{
	  *Data = CacheKey->FirstField->Data;
	}
    }

  return TRUE;
}


BOOL
InfGetDataField (PINFCONTEXT Context,
		 ULONG FieldIndex,
		 PCHAR *Data)
{
  PINFCACHELINE CacheLine;
  PINFCACHEFIELD CacheField;
  ULONG Index;

  if (Context == NULL || Context->Line == NULL || Data == NULL)
    {
      DPRINT("Invalid parameter\n");
      return FALSE;
    }

  CacheLine = (PINFCACHELINE)Context->Line;

  if (FieldIndex > CacheLine->FieldCount)
    return FALSE;

  if (FieldIndex == 0)
    {
      *Data = CacheLine->Key;
    }
  else
    {
      CacheField = CacheLine->FirstField;
      for (Index = 1; Index < FieldIndex; Index++)
	CacheField = CacheField->Next;

      *Data = CacheField->Data;
    }

  return TRUE;
}


/* EOF */
