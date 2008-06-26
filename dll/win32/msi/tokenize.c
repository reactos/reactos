/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** An tokenizer for SQL
**
** This file contains C code that splits an SQL input string up into
** individual tokens and sends those tokens one-by-one over to the
** parser for analysis.
*/

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wine/unicode.h"
#include "query.h"
#include "sql.tab.h"

/*
** All the keywords of the SQL language are stored as in a hash
** table composed of instances of the following structure.
*/
typedef struct Keyword Keyword;
struct Keyword {
  const WCHAR *zName;             /* The keyword name */
  int tokenType;           /* The token value for this keyword */
};

#define MAX_TOKEN_LEN 11

static const WCHAR ADD_W[] = { 'A','D','D',0 };
static const WCHAR ALTER_W[] = { 'A','L','T','E','R',0 };
static const WCHAR AND_W[] = { 'A','N','D',0 };
static const WCHAR BY_W[] = { 'B','Y',0 };
static const WCHAR CHAR_W[] = { 'C','H','A','R',0 };
static const WCHAR CHARACTER_W[] = { 'C','H','A','R','A','C','T','E','R',0 };
static const WCHAR CREATE_W[] = { 'C','R','E','A','T','E',0 };
static const WCHAR DELETE_W[] = { 'D','E','L','E','T','E',0 };
static const WCHAR DISTINCT_W[] = { 'D','I','S','T','I','N','C','T',0 };
static const WCHAR FREE_W[] = { 'F','R','E','E',0 };
static const WCHAR FROM_W[] = { 'F','R','O','M',0 };
static const WCHAR HOLD_W[] = { 'H','O','L','D',0 };
static const WCHAR INSERT_W[] = { 'I','N','S','E','R','T',0 };
static const WCHAR INT_W[] = { 'I','N','T',0 };
static const WCHAR INTEGER_W[] = { 'I','N','T','E','G','E','R',0 };
static const WCHAR INTO_W[] = { 'I','N','T','O',0 };
static const WCHAR IS_W[] = { 'I','S',0 };
static const WCHAR KEY_W[] = { 'K','E','Y',0 };
static const WCHAR LIKE_W[] = { 'L','I','K','E',0 };
static const WCHAR LOCALIZABLE_W[] = { 'L','O','C','A','L','I','Z','A','B','L','E',0 };
static const WCHAR LONG_W[] = { 'L','O','N','G',0 };
static const WCHAR LONGCHAR_W[] = { 'L','O','N','G','C','H','A','R',0 };
static const WCHAR NOT_W[] = { 'N','O','T',0 };
static const WCHAR NULL_W[] = { 'N','U','L','L',0 };
static const WCHAR OBJECT_W[] = { 'O','B','J','E','C','T',0 };
static const WCHAR OR_W[] = { 'O','R',0 };
static const WCHAR ORDER_W[] = { 'O','R','D','E','R',0 };
static const WCHAR PRIMARY_W[] = { 'P','R','I','M','A','R','Y',0 };
static const WCHAR SELECT_W[] = { 'S','E','L','E','C','T',0 };
static const WCHAR SET_W[] = { 'S','E','T',0 };
static const WCHAR SHORT_W[] = { 'S','H','O','R','T',0 };
static const WCHAR TABLE_W[] = { 'T','A','B','L','E',0 };
static const WCHAR TEMPORARY_W[] = { 'T','E','M','P','O','R','A','R','Y',0 };
static const WCHAR UPDATE_W[] = { 'U','P','D','A','T','E',0 };
static const WCHAR VALUES_W[] = { 'V','A','L','U','E','S',0 };
static const WCHAR WHERE_W[] = { 'W','H','E','R','E',0 };

/*
** These are the keywords
** They MUST be in alphabetical order
*/
static const Keyword aKeywordTable[] = {
  { ADD_W, TK_ADD },
  { ALTER_W, TK_ALTER },
  { AND_W, TK_AND },
  { BY_W, TK_BY },
  { CHAR_W, TK_CHAR },
  { CHARACTER_W, TK_CHAR },
  { CREATE_W, TK_CREATE },
  { DELETE_W, TK_DELETE },
  { DISTINCT_W, TK_DISTINCT },
  { FREE_W, TK_FREE },
  { FROM_W, TK_FROM },
  { HOLD_W, TK_HOLD },
  { INSERT_W, TK_INSERT },
  { INT_W, TK_INT },
  { INTEGER_W, TK_INT },
  { INTO_W, TK_INTO },
  { IS_W, TK_IS },
  { KEY_W, TK_KEY },
  { LIKE_W, TK_LIKE },
  { LOCALIZABLE_W, TK_LOCALIZABLE },
  { LONG_W, TK_LONG },
  { LONGCHAR_W, TK_LONGCHAR },
  { NOT_W, TK_NOT },
  { NULL_W, TK_NULL },
  { OBJECT_W, TK_OBJECT },
  { OR_W, TK_OR },
  { ORDER_W, TK_ORDER },
  { PRIMARY_W, TK_PRIMARY },
  { SELECT_W, TK_SELECT },
  { SET_W, TK_SET },
  { SHORT_W, TK_SHORT },
  { TABLE_W, TK_TABLE },
  { TEMPORARY_W, TK_TEMPORARY },
  { UPDATE_W, TK_UPDATE },
  { VALUES_W, TK_VALUES },
  { WHERE_W, TK_WHERE },
};

#define KEYWORD_COUNT ( sizeof aKeywordTable/sizeof (Keyword) )

/*
** Comparison function for binary search.
*/
static int compKeyword(const void *m1, const void *m2){
  const Keyword *k1 = m1, *k2 = m2;

  return strcmpiW( k1->zName, k2->zName );
}

/*
** This function looks up an identifier to determine if it is a
** keyword.  If it is a keyword, the token code of that keyword is 
** returned.  If the input is not a keyword, TK_ID is returned.
*/
static int sqliteKeywordCode(const WCHAR *z, int n){
  WCHAR str[MAX_TOKEN_LEN+1];
  Keyword key, *r;

  if( n>MAX_TOKEN_LEN )
    return TK_ID;

  memcpy( str, z, n*sizeof (WCHAR) );
  str[n] = 0;
  key.tokenType = 0;
  key.zName = str;
  r = bsearch( &key, aKeywordTable, KEYWORD_COUNT, sizeof (Keyword), compKeyword );
  if( r )
    return r->tokenType;
  return TK_ID;
}


/*
** If X is a character that can be used in an identifier then
** isIdChar[X] will be 1.  Otherwise isIdChar[X] will be 0.
**
** In this implementation, an identifier can be a string of
** alphabetic characters, digits, and "_" plus any character
** with the high-order bit set.  The latter rule means that
** any sequence of UTF-8 characters or characters taken from
** an extended ISO8859 character set can form an identifier.
*/
static const char isIdChar[] = {
/* x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 0x */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 1x */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 2x */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  /* 3x */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 4x */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,  /* 5x */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 6x */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,  /* 7x */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 8x */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 9x */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* Ax */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* Bx */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* Cx */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* Dx */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* Ex */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* Fx */
};


/*
** Return the length of the token that begins at z[0].  Return
** -1 if the token is (or might be) incomplete.  Store the token
** type in *tokenType before returning.
*/
int sqliteGetToken(const WCHAR *z, int *tokenType){
  int i;
  switch( *z ){
    case ' ': case '\t': case '\n': case '\f': case '\r': {
      for(i=1; isspace(z[i]); i++){}
      *tokenType = TK_SPACE;
      return i;
    }
    case '-': {
      if( z[1]==0 ) return -1;
      *tokenType = TK_MINUS;
      return 1;
    }
    case '(':
      *tokenType = TK_LP;
      return 1;
    case ')':
      *tokenType = TK_RP;
      return 1;
    case '*':
      *tokenType = TK_STAR;
      return 1;
    case '=':
      *tokenType = TK_EQ;
      return 1;
    case '<': {
      if( z[1]=='=' ){
        *tokenType = TK_LE;
        return 2;
      }else if( z[1]=='>' ){
        *tokenType = TK_NE;
        return 2;
      }else{
        *tokenType = TK_LT;
        return 1;
      }
    }
    case '>': {
      if( z[1]=='=' ){
        *tokenType = TK_GE;
        return 2;
      }else{
        *tokenType = TK_GT;
        return 1;
      }
    }
    case '!': {
      if( z[1]!='=' ){
        *tokenType = TK_ILLEGAL;
        return 2;
      }else{
        *tokenType = TK_NE;
        return 2;
      }
    }
    case '?':
      *tokenType = TK_WILDCARD;
      return 1;
    case ',':
      *tokenType = TK_COMMA;
      return 1;
    case '`': case '\'': {
      int delim = z[0];
      for(i=1; z[i]; i++){
        if( z[i]==delim ){
          if( z[i+1]==delim ){
            i++;
          }else{
            break;
          }
        }
      }
      if( z[i] ) i++;
      if( delim == '`' )
        *tokenType = TK_ID;
      else
        *tokenType = TK_STRING;
      return i;
    }
    case '.': {
      if( !isdigit(z[1]) ){
        *tokenType = TK_DOT;
        return 1;
      }
      /* Fall thru into the next case */
    }
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': {
      *tokenType = TK_INTEGER;
      for(i=1; isdigit(z[i]); i++){}
      return i;
    }
    case '[': {
      for(i=1; z[i] && z[i-1]!=']'; i++){}
      *tokenType = TK_ID;
      return i;
    }
    default: {
      if( !isIdChar[*z] ){
        break;
      }
      for(i=1; isIdChar[z[i]]; i++){}
      *tokenType = sqliteKeywordCode(z, i);
      return i;
    }
  }
  *tokenType = TK_ILLEGAL;
  return 1;
}
