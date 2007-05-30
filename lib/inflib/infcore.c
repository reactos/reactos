/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * PROGRAMMER: Royce Mitchell III
 *             Eric Kohl
 *             Ge van Geldorp <gvg@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "inflib.h"

#define NDEBUG
#include <debug.h>

#define CONTROL_Z  '\x1a'
#define MAX_SECTION_NAME_LEN  255
#define MAX_FIELD_LEN         511  /* larger fields get silently truncated */
/* actual string limit is MAX_INF_STRING_LENGTH+1 (plus terminating null) under Windows */
#define MAX_STRING_LEN        (MAX_INF_STRING_LENGTH+1)


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
  TCHAR token[MAX_FIELD_LEN+1];   /* current token */
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
InfpFreeLine (PINFCACHELINE Line)
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
      FREE (Line->Key);
      Line->Key = NULL;
    }

  /* Remove data fields */
  while (Line->FirstField != NULL)
    {
      Field = Line->FirstField->Next;
      FREE (Line->FirstField);
      Line->FirstField = Field;
    }
  Line->LastField = NULL;

  FREE (Line);

  return Next;
}


PINFCACHESECTION
InfpFreeSection (PINFCACHESECTION Section)
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
      Section->FirstLine = InfpFreeLine (Section->FirstLine);
    }
  Section->LastLine = NULL;

  FREE (Section);

  return Next;
}


PINFCACHESECTION
InfpFindSection(PINFCACHE Cache,
                PCTSTR Name)
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
      if (_tcsicmp (Section->Name, Name) == 0)
        {
          return Section;
        }

      /* get the next section*/
      Section = Section->Next;
    }

  return NULL;
}


PINFCACHESECTION
InfpAddSection(PINFCACHE Cache,
               PCTSTR Name)
{
  PINFCACHESECTION Section = NULL;
  ULONG Size;

  if (Cache == NULL || Name == NULL)
    {
      DPRINT("Invalid parameter\n");
      return NULL;
    }

  /* Allocate and initialize the new section */
  Size = sizeof(INFCACHESECTION) + (_tcslen (Name) * sizeof(TCHAR));
  Section = (PINFCACHESECTION)MALLOC (Size);
  if (Section == NULL)
    {
      DPRINT("MALLOC() failed\n");
      return NULL;
    }
  ZEROMEMORY (Section,
              Size);

  /* Copy section name */
  _tcscpy (Section->Name, Name);

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


PINFCACHELINE
InfpAddLine(PINFCACHESECTION Section)
{
  PINFCACHELINE Line;

  if (Section == NULL)
    {
      DPRINT("Invalid parameter\n");
      return NULL;
    }

  Line = (PINFCACHELINE)MALLOC (sizeof(INFCACHELINE));
  if (Line == NULL)
    {
      DPRINT("MALLOC() failed\n");
      return NULL;
    }
  ZEROMEMORY(Line,
             sizeof(INFCACHELINE));

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


PVOID
InfpAddKeyToLine(PINFCACHELINE Line,
                 PCTSTR Key)
{
  if (Line == NULL)
    {
      DPRINT1("Invalid Line\n");
      return NULL;
    }

  if (Line->Key != NULL)
    {
      DPRINT1("Line already has a key\n");
      return NULL;
    }

  Line->Key = (PTCHAR)MALLOC((_tcslen(Key) + 1) * sizeof(TCHAR));
  if (Line->Key == NULL)
    {
      DPRINT1("MALLOC() failed\n");
      return NULL;
    }

  _tcscpy(Line->Key, Key);

  return (PVOID)Line->Key;
}


PVOID
InfpAddFieldToLine(PINFCACHELINE Line,
                   PCTSTR Data)
{
  PINFCACHEFIELD Field;
  ULONG Size;

  Size = sizeof(INFCACHEFIELD) + (_tcslen(Data) * sizeof(TCHAR));
  Field = (PINFCACHEFIELD)MALLOC(Size);
  if (Field == NULL)
    {
      DPRINT1("MALLOC() failed\n");
      return NULL;
    }
  ZEROMEMORY (Field,
              Size);
  _tcscpy (Field->Data, Data);

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


PINFCACHELINE
InfpFindKeyLine(PINFCACHESECTION Section,
                PCTSTR Key)
{
  PINFCACHELINE Line;

  Line = Section->FirstLine;
  while (Line != NULL)
    {
      if (Line->Key != NULL && _tcsicmp (Line->Key, Key) == 0)
        {
          return Line;
        }

      Line = Line->Next;
    }

  return NULL;
}


/* push the current state on the parser stack */
__inline static void push_state( struct parser *parser, enum parser_state state )
{
//  assert( parser->stack_pos < sizeof(parser->stack)/sizeof(parser->stack[0]) );
  parser->stack[parser->stack_pos++] = state;
}


/* pop the current state */
__inline static void pop_state( struct parser *parser )
{
//  assert( parser->stack_pos );
  parser->state = parser->stack[--parser->stack_pos];
}


/* set the parser state and return the previous one */
__inline static enum parser_state set_state( struct parser *parser, enum parser_state state )
{
  enum parser_state ret = parser->state;
  parser->state = state;
  return ret;
}


/* check if the pointer points to an end of file */
__inline static int is_eof( struct parser *parser, const CHAR *ptr )
{
  return (ptr >= parser->end || *ptr == CONTROL_Z);
}


/* check if the pointer points to an end of line */
__inline static int is_eol( struct parser *parser, const CHAR *ptr )
{
  return (ptr >= parser->end ||
          *ptr == CONTROL_Z ||
          *ptr == '\n' ||
          (*ptr == '\r' && *(ptr + 1) == '\n'));
}


/* push data from current token start up to pos into the current token */
static int push_token( struct parser *parser, const CHAR *pos )
{
  unsigned int len = pos - parser->start;
  const CHAR *src = parser->start;
  TCHAR *dst = parser->token + parser->token_len;

  if (len > MAX_FIELD_LEN - parser->token_len)
    len = MAX_FIELD_LEN - parser->token_len;

  parser->token_len += len;
  for ( ; len > 0; len--, dst++, src++)
    *dst = *src ? (TCHAR)*src : L' ';
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
      parser->error = INF_STATUS_SECTION_NAME_TOO_LONG;
      return NULL;
    }

  Section = InfpFindSection(parser->file,
                            parser->token);
  if (Section == NULL)
    {
      /* need to create a new one */
      Section= InfpAddSection(parser->file,
                              parser->token);
      if (Section == NULL)
        {
          parser->error = INF_STATUS_NOT_ENOUGH_MEMORY;
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

  if (!parser->line)  /* need to start a new line */
    {
      if (parser->cur_section == NULL)  /* got a line before the first section */
        {
          parser->error = INF_STATUS_WRONG_INF_STYLE;
          return NULL;
        }

      parser->line = InfpAddLine(parser->cur_section);
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
  parser->error = INF_STATUS_NOT_ENOUGH_MEMORY;
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
          case '\r':
            continue;

          case '\n':
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
  parser->error = INF_STATUS_BAD_SECTION_NAME_LINE; /* unfinished section name */
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
          case '\r':
            continue;

          case '\n':
            parser->line_pos++;
            parser->start = p + 1;
            set_state( parser, LEADING_SPACES );
            return p + 1;

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
INFSTATUS
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
  file->StringsSection = InfpFindSection(file,
                                         _T("Strings"));

  return INF_STATUS_SUCCESS;
}

/* EOF */
