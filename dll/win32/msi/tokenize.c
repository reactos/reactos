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
#include "wine/unicode.h"
#include "query.h"
#include "sql.tab.h"

/*
** All the keywords of the SQL language are stored as in a hash
** table composed of instances of the following structure.
*/
typedef struct Keyword Keyword;
struct Keyword {
  const WCHAR *name;             /* The keyword name */
  unsigned int len;
  int tokenType;           /* The token value for this keyword */
};

#define MAX_TOKEN_LEN 11

static const WCHAR addW[] = {'A','D','D'};
static const WCHAR alterW[] = {'A','L','T','E','R'};
static const WCHAR andW[] = {'A','N','D'};
static const WCHAR byW[] = {'B','Y'};
static const WCHAR charW[] = {'C','H','A','R'};
static const WCHAR characterW[] = {'C','H','A','R','A','C','T','E','R'};
static const WCHAR createW[] = {'C','R','E','A','T','E'};
static const WCHAR deleteW[] = {'D','E','L','E','T','E'};
static const WCHAR distinctW[] = {'D','I','S','T','I','N','C','T'};
static const WCHAR dropW[] = {'D','R','O','P'};
static const WCHAR freeW[] = {'F','R','E','E'};
static const WCHAR fromW[] = {'F','R','O','M'};
static const WCHAR holdW[] = {'H','O','L','D'};
static const WCHAR insertW[] = {'I','N','S','E','R','T'};
static const WCHAR intW[] = {'I','N','T'};
static const WCHAR integerW[] = {'I','N','T','E','G','E','R'};
static const WCHAR intoW[] = {'I','N','T','O'};
static const WCHAR isW[] = {'I','S'};
static const WCHAR keyW[] = {'K','E','Y'};
static const WCHAR likeW[] = {'L','I','K','E'};
static const WCHAR localizableW[] = {'L','O','C','A','L','I','Z','A','B','L','E'};
static const WCHAR longW[] = {'L','O','N','G'};
static const WCHAR longcharW[] = {'L','O','N','G','C','H','A','R'};
static const WCHAR notW[] = {'N','O','T'};
static const WCHAR nullW[] = {'N','U','L','L'};
static const WCHAR objectW[] = {'O','B','J','E','C','T'};
static const WCHAR orW[] = {'O','R'};
static const WCHAR orderW[] = {'O','R','D','E','R'};
static const WCHAR primaryW[] = {'P','R','I','M','A','R','Y'};
static const WCHAR selectW[] = {'S','E','L','E','C','T'};
static const WCHAR setW[] = {'S','E','T'};
static const WCHAR shortW[] = {'S','H','O','R','T'};
static const WCHAR tableW[] = {'T','A','B','L','E'};
static const WCHAR temporaryW[] = {'T','E','M','P','O','R','A','R','Y'};
static const WCHAR updateW[] = {'U','P','D','A','T','E'};
static const WCHAR valuesW[] = {'V','A','L','U','E','S'};
static const WCHAR whereW[] = {'W','H','E','R','E'};

/*
** These are the keywords
** They MUST be in alphabetical order
*/
static const Keyword aKeywordTable[] = {
  { addW,         ARRAY_SIZE(addW),         TK_ADD },
  { alterW,       ARRAY_SIZE(alterW),       TK_ALTER },
  { andW,         ARRAY_SIZE(andW),         TK_AND },
  { byW,          ARRAY_SIZE(byW),          TK_BY },
  { charW,        ARRAY_SIZE(charW),        TK_CHAR },
  { characterW,   ARRAY_SIZE(characterW),   TK_CHAR },
  { createW,      ARRAY_SIZE(createW),      TK_CREATE },
  { deleteW,      ARRAY_SIZE(deleteW),      TK_DELETE },
  { distinctW,    ARRAY_SIZE(distinctW),    TK_DISTINCT },
  { dropW,        ARRAY_SIZE(dropW),        TK_DROP },
  { freeW,        ARRAY_SIZE(freeW),        TK_FREE },
  { fromW,        ARRAY_SIZE(fromW),        TK_FROM },
  { holdW,        ARRAY_SIZE(holdW),        TK_HOLD },
  { insertW,      ARRAY_SIZE(insertW),      TK_INSERT },
  { intW,         ARRAY_SIZE(intW),         TK_INT },
  { integerW,     ARRAY_SIZE(integerW),     TK_INT },
  { intoW,        ARRAY_SIZE(intoW),        TK_INTO },
  { isW,          ARRAY_SIZE(isW),          TK_IS },
  { keyW,         ARRAY_SIZE(keyW),         TK_KEY },
  { likeW,        ARRAY_SIZE(likeW),        TK_LIKE },
  { localizableW, ARRAY_SIZE(localizableW), TK_LOCALIZABLE },
  { longW,        ARRAY_SIZE(longW),        TK_LONG },
  { longcharW,    ARRAY_SIZE(longcharW),    TK_LONGCHAR },
  { notW,         ARRAY_SIZE(notW),         TK_NOT },
  { nullW,        ARRAY_SIZE(nullW),        TK_NULL },
  { objectW,      ARRAY_SIZE(objectW),      TK_OBJECT },
  { orW,          ARRAY_SIZE(orW),          TK_OR },
  { orderW,       ARRAY_SIZE(orderW),       TK_ORDER },
  { primaryW,     ARRAY_SIZE(primaryW),     TK_PRIMARY },
  { selectW,      ARRAY_SIZE(selectW),      TK_SELECT },
  { setW,         ARRAY_SIZE(setW),         TK_SET },
  { shortW,       ARRAY_SIZE(shortW),       TK_SHORT },
  { tableW,       ARRAY_SIZE(tableW),       TK_TABLE },
  { temporaryW,   ARRAY_SIZE(temporaryW),   TK_TEMPORARY },
  { updateW,      ARRAY_SIZE(updateW),      TK_UPDATE },
  { valuesW,      ARRAY_SIZE(valuesW),      TK_VALUES },
  { whereW,       ARRAY_SIZE(whereW),       TK_WHERE },
};

/*
** Comparison function for binary search.
*/
static int compKeyword(const void *m1, const void *m2){
  const Keyword *k1 = m1, *k2 = m2;
  int ret, len = min( k1->len, k2->len );

  if ((ret = memicmpW( k1->name, k2->name, len ))) return ret;
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
  Keyword key, *r;

  if( n>MAX_TOKEN_LEN )
    return TK_ID;

  key.tokenType = 0;
  key.name = z;
  key.len = n;
  r = bsearch( &key, aKeywordTable, ARRAY_SIZE(aKeywordTable), sizeof(Keyword), compKeyword );
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
