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
** A tokenizer for SQL
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
#include "query.h"
#include "sql.tab.h"

/*
** All the keywords of the SQL language are stored as in a hash
** table composed of instances of the following structure.
*/
struct keyword
{
  const WCHAR *name;             /* The keyword name */
  unsigned int len;
  int tokenType;           /* The token value for this keyword */
};

#define MAX_TOKEN_LEN 11

/*
** These are the keywords
** They MUST be in alphabetical order
*/
#define X(str)  str, ARRAY_SIZE(str) - 1
static const struct keyword aKeywordTable[] = {
  { X(L"ADD"),         TK_ADD },
  { X(L"ALTER"),       TK_ALTER },
  { X(L"AND"),         TK_AND },
  { X(L"BY"),          TK_BY },
  { X(L"CHAR"),        TK_CHAR },
  { X(L"CHARACTER"),   TK_CHAR },
  { X(L"CREATE"),      TK_CREATE },
  { X(L"DELETE"),      TK_DELETE },
  { X(L"DISTINCT"),    TK_DISTINCT },
  { X(L"DROP"),        TK_DROP },
  { X(L"FREE"),        TK_FREE },
  { X(L"FROM"),        TK_FROM },
  { X(L"HOLD"),        TK_HOLD },
  { X(L"INSERT"),      TK_INSERT },
  { X(L"INT"),         TK_INT },
  { X(L"INTEGER"),     TK_INT },
  { X(L"INTO"),        TK_INTO },
  { X(L"IS"),          TK_IS },
  { X(L"KEY"),         TK_KEY },
  { X(L"LIKE"),        TK_LIKE },
  { X(L"LOCALIZABLE"), TK_LOCALIZABLE },
  { X(L"LONG"),        TK_LONG },
  { X(L"LONGCHAR"),    TK_LONGCHAR },
  { X(L"NOT"),         TK_NOT },
  { X(L"NULL"),        TK_NULL },
  { X(L"OBJECT"),      TK_OBJECT },
  { X(L"OR"),          TK_OR },
  { X(L"ORDER"),       TK_ORDER },
  { X(L"PRIMARY"),     TK_PRIMARY },
  { X(L"SELECT"),      TK_SELECT },
  { X(L"SET"),         TK_SET },
  { X(L"SHORT"),       TK_SHORT },
  { X(L"TABLE"),       TK_TABLE },
  { X(L"TEMPORARY"),   TK_TEMPORARY },
  { X(L"UPDATE"),      TK_UPDATE },
  { X(L"VALUES"),      TK_VALUES },
  { X(L"WHERE"),       TK_WHERE },
};
#undef X

/*
** Comparison function for binary search.
*/
static int __cdecl compKeyword(const void *m1, const void *m2){
  const struct keyword *k1 = m1, *k2 = m2;
  int ret, len = min( k1->len, k2->len );

  if ((ret = wcsnicmp( k1->name, k2->name, len ))) return ret;
  if (k1->len < k2->len) return -1;
  else if (k1->len > k2->len) return 1;
  return 0;
}

/*
** This function looks up an identifier to determine if it is a
** keyword.  If it is a keyword, the token code of that keyword is 
** returned.  If the input is not a keyword, TK_ID is returned.
*/
static int sqliteKeywordCode(const WCHAR *z, int n){
  struct keyword key, *r;

  if( n>MAX_TOKEN_LEN )
    return TK_ID;

  key.tokenType = 0;
  key.name = z;
  key.len = n;
  r = bsearch( &key, aKeywordTable, ARRAY_SIZE(aKeywordTable), sizeof(struct keyword), compKeyword );
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
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 0x */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 1x */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,  /* 2x */
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
** WCHAR safe version of isdigit()
*/
static inline int isDigit(WCHAR c)
{
    return c >= '0' && c <= '9';
}

/*
** WCHAR safe version of isspace(), except '\r'
*/
static inline int isSpace(WCHAR c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\f';
}

/*
** Return the length of the token that begins at z[0].  Return
** -1 if the token is (or might be) incomplete.  Store the token
** type in *tokenType before returning.
*/
int sqliteGetToken(const WCHAR *z, int *tokenType, int *skip){
  int i;

  *skip = 0;
  switch( *z ){
    case ' ': case '\t': case '\n': case '\f':
      for(i=1; isSpace(z[i]); i++){}
      *tokenType = TK_SPACE;
      return i;
    case '-':
      if( z[1]==0 ) return -1;
      *tokenType = TK_MINUS;
      return 1;
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
    case '<':
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
    case '>':
      if( z[1]=='=' ){
        *tokenType = TK_GE;
        return 2;
      }else{
        *tokenType = TK_GT;
        return 1;
      }
    case '!':
      if( z[1]!='=' ){
        *tokenType = TK_ILLEGAL;
        return 2;
      }else{
        *tokenType = TK_NE;
        return 2;
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
        if( z[i]==delim )
          break;
      }
      if( z[i] ) i++;
      if( delim == '`' )
        *tokenType = TK_ID;
      else
        *tokenType = TK_STRING;
      return i;
    }
    case '.':
      if( !isDigit(z[1]) ){
        *tokenType = TK_DOT;
        return 1;
      }
      /* Fall through */
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      *tokenType = TK_INTEGER;
      for(i=1; isDigit(z[i]); i++){}
      return i;
    case '[':
      for(i=1; z[i] && z[i-1]!=']'; i++){}
      *tokenType = TK_ID;
      return i;
    default:
      if( !isIdChar[*z] ){
        break;
      }
      for(i=1; isIdChar[z[i]]; i++){}
      *tokenType = sqliteKeywordCode(z, i);
      if( *tokenType == TK_ID && z[i] == '`' ) *skip = 1;
      return i;
  }
  *tokenType = TK_ILLEGAL;
  return 1;
}
