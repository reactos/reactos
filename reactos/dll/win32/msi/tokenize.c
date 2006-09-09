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
#include "wine/debug.h"
#include "wine/unicode.h"
#include "query.h"
#include "sql.tab.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

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

static const WCHAR ABORT_W[] = { 'A','B','O','R','T',0 };
static const WCHAR AFTER_W[] = { 'A','F','T','E','R',0 };
static const WCHAR ALTER_W[] = { 'A','L','T','E','R',0 };
static const WCHAR ALL_W[] = { 'A','L','L',0 };
static const WCHAR AND_W[] = { 'A','N','D',0 };
static const WCHAR AS_W[] = { 'A','S',0 };
static const WCHAR ASC_W[] = { 'A','S','C',0 };
static const WCHAR BEFORE_W[] = { 'B','E','F','O','R','E',0 };
static const WCHAR BEGIN_W[] = { 'B','E','G','I','N','W',0 };
static const WCHAR BETWEEN_W[] = { 'B','E','T','W','E','E','N',0 };
static const WCHAR BY_W[] = { 'B','Y',0 };
static const WCHAR CASCADE_W[] = { 'C','A','S','C','A','D','E',0 };
static const WCHAR CASE_W[] = { 'C','A','S','E',0 };
static const WCHAR CHAR_W[] = { 'C','H','A','R',0 };
static const WCHAR CHARACTER_W[] = { 'C','H','A','R','A','C','T','E','R',0 };
static const WCHAR CHECK_W[] = { 'C','H','E','C','K',0 };
static const WCHAR CLUSTER_W[] = { 'C','L','U','S','T','E','R',0 };
static const WCHAR COLLATE_W[] = { 'C','O','L','L','A','T','E',0 };
static const WCHAR COMMIT_W[] = { 'C','O','M','M','I','T',0 };
static const WCHAR CONFLICT_W[] = { 'C','O','N','F','L','I','C','T',0 };
static const WCHAR CONSTRAINT_W[] = { 'C','O','N','S','T','R','A','I','N','T',0 };
static const WCHAR COPY_W[] = { 'C','O','P','Y',0 };
static const WCHAR CREATE_W[] = { 'C','R','E','A','T','E',0 };
static const WCHAR CROSS_W[] = { 'C','R','O','S','S',0 };
static const WCHAR DEFAULT_W[] = { 'D','E','F','A','U','L','T',0 };
static const WCHAR DEFERRED_W[] = { 'D','E','F','E','R','R','E','D',0 };
static const WCHAR DEFERRABLE_W[] = { 'D','E','F','E','R','R','A','B','L','E',0 };
static const WCHAR DELETE_W[] = { 'D','E','L','E','T','E',0 };
static const WCHAR DELIMITERS_W[] = { 'D','E','L','I','M','I','T','E','R','S',0 };
static const WCHAR DESC_W[] = { 'D','E','S','C',0 };
static const WCHAR DISTINCT_W[] = { 'D','I','S','T','I','N','C','T',0 };
static const WCHAR DROP_W[] = { 'D','R','O','P',0 };
static const WCHAR END_W[] = { 'E','N','D',0 };
static const WCHAR EACH_W[] = { 'E','A','C','H',0 };
static const WCHAR ELSE_W[] = { 'E','L','S','E',0 };
static const WCHAR EXCEPT_W[] = { 'E','X','C','E','P','T',0 };
static const WCHAR EXPLAIN_W[] = { 'E','X','P','L','A','I','N',0 };
static const WCHAR FAIL_W[] = { 'F','A','I','L',0 };
static const WCHAR FOR_W[] = { 'F','O','R',0 };
static const WCHAR FOREIGN_W[] = { 'F','O','R','E','I','G','N',0 };
static const WCHAR FREE_W[] = { 'F','R','E','E',0 };
static const WCHAR FROM_W[] = { 'F','R','O','M',0 };
static const WCHAR FULL_W[] = { 'F','U','L','L',0 };
static const WCHAR GLOB_W[] = { 'G','L','O','B',0 };
static const WCHAR GROUP_W[] = { 'G','R','O','U','P',0 };
static const WCHAR HAVING_W[] = { 'H','A','V','I','N','G',0 };
static const WCHAR HOLD_W[] = { 'H','O','L','D',0 };
static const WCHAR IGNORE_W[] = { 'I','G','N','O','R','E',0 };
static const WCHAR IMMEDIATE_W[] = { 'I','M','M','E','D','I','A','T','E',0 };
static const WCHAR IN_W[] = { 'I','N',0 };
static const WCHAR INDEX_W[] = { 'I','N','D','E','X',0 };
static const WCHAR INITIALLY_W[] = { 'I','N','I','T','I','A','L','L','Y',0 };
static const WCHAR INNER_W[] = { 'I','N','N','E','R',0 };
static const WCHAR INSERT_W[] = { 'I','N','S','E','R','T',0 };
static const WCHAR INSTEAD_W[] = { 'I','N','S','T','E','A','D',0 };
static const WCHAR INT_W[] = { 'I','N','T',0 };
static const WCHAR INTERSECT_W[] = { 'I','N','T','E','R','S','E','C','T',0 };
static const WCHAR INTO_W[] = { 'I','N','T','O',0 };
static const WCHAR IS_W[] = { 'I','S',0 };
static const WCHAR ISNULL_W[] = { 'I','S','N','U','L','L',0 };
static const WCHAR JOIN_W[] = { 'J','O','I','N',0 };
static const WCHAR KEY_W[] = { 'K','E','Y',0 };
static const WCHAR LEFT_W[] = { 'L','E','F','T',0 };
static const WCHAR LIKE_W[] = { 'L','I','K','E',0 };
static const WCHAR LIMIT_W[] = { 'L','I','M','I','T',0 };
static const WCHAR LOCALIZABLE_W[] = { 'L','O','C','A','L','I','Z','A','B','L','E',0 };
static const WCHAR LONG_W[] = { 'L','O','N','G',0 };
static const WCHAR LONGCHAR_W[] = { 'L','O','N','G','C','H','A','R',0 };
static const WCHAR MATCH_W[] = { 'M','A','T','C','H',0 };
static const WCHAR NATURAL_W[] = { 'N','A','T','U','R','A','L',0 };
static const WCHAR NOT_W[] = { 'N','O','T',0 };
static const WCHAR NOTNULL_W[] = { 'N','O','T','N','U','L','L',0 };
static const WCHAR NULL_W[] = { 'N','U','L','L',0 };
static const WCHAR OBJECT_W[] = { 'O','B','J','E','C','T',0 };
static const WCHAR OF_W[] = { 'O','F',0 };
static const WCHAR OFFSET_W[] = { 'O','F','F','S','E','T',0 };
static const WCHAR ON_W[] = { 'O','N',0 };
static const WCHAR OR_W[] = { 'O','R',0 };
static const WCHAR ORDER_W[] = { 'O','R','D','E','R',0 };
static const WCHAR OUTER_W[] = { 'O','U','T','E','R',0 };
static const WCHAR PRAGMA_W[] = { 'P','R','A','G','M','A',0 };
static const WCHAR PRIMARY_W[] = { 'P','R','I','M','A','R','Y',0 };
static const WCHAR RAISE_W[] = { 'R','A','I','S','E',0 };
static const WCHAR REFERENCES_W[] = { 'R','E','F','E','R','E','N','C','E','S',0 };
static const WCHAR REPLACE_W[] = { 'R','E','P','L','A','C','E',0 };
static const WCHAR RESTRICT_W[] = { 'R','E','S','T','R','I','C','T',0 };
static const WCHAR RIGHT_W[] = { 'R','I','G','H','T',0 };
static const WCHAR ROLLBACK_W[] = { 'R','O','L','L','B','A','C','K',0 };
static const WCHAR ROW_W[] = { 'R','O','W',0 };
static const WCHAR SELECT_W[] = { 'S','E','L','E','C','T',0 };
static const WCHAR SET_W[] = { 'S','E','T',0 };
static const WCHAR SHORT_W[] = { 'S','H','O','R','T',0 };
static const WCHAR STATEMENT_W[] = { 'S','T','A','T','E','M','E','N','T',0 };
static const WCHAR TABLE_W[] = { 'T','A','B','L','E',0 };
static const WCHAR TEMP_W[] = { 'T','E','M','P',0 };
static const WCHAR TEMPORARY_W[] = { 'T','E','M','P','O','R','A','R','Y',0 };
static const WCHAR THEN_W[] = { 'T','H','E','N',0 };
static const WCHAR TRANSACTION_W[] = { 'T','R','A','N','S','A','C','T','I','O','N',0 };
static const WCHAR TRIGGER_W[] = { 'T','R','I','G','G','E','R',0 };
static const WCHAR UNION_W[] = { 'U','N','I','O','N',0 };
static const WCHAR UNIQUE_W[] = { 'U','N','I','Q','U','E',0 };
static const WCHAR UPDATE_W[] = { 'U','P','D','A','T','E',0 };
static const WCHAR USING_W[] = { 'U','S','I','N','G',0 };
static const WCHAR VACUUM_W[] = { 'V','A','C','U','U','M',0 };
static const WCHAR VALUES_W[] = { 'V','A','L','U','E','S',0 };
static const WCHAR VIEW_W[] = { 'V','I','E','W',0 };
static const WCHAR WHEN_W[] = { 'W','H','E','N',0 };
static const WCHAR WHERE_W[] = { 'W','H','E','R','E',0 };

/*
** These are the keywords
*/
static const Keyword aKeywordTable[] = {
  { ABORT_W, TK_ABORT },
  { AFTER_W, TK_AFTER },
  /*{ ALTER_W, TK_ALTER },*/
  { ALL_W, TK_ALL },
  { AND_W, TK_AND },
  { AS_W, TK_AS },
  { ASC_W, TK_ASC },
  { BEFORE_W, TK_BEFORE },
  { BEGIN_W, TK_BEGIN },
  { BETWEEN_W, TK_BETWEEN },
  { BY_W, TK_BY },
  { CASCADE_W, TK_CASCADE },
  { CASE_W, TK_CASE },
  { CHAR_W, TK_CHAR },
  { CHARACTER_W, TK_CHAR },
  { CHECK_W, TK_CHECK },
  { CLUSTER_W, TK_CLUSTER },
  { COLLATE_W, TK_COLLATE },
  { COMMIT_W, TK_COMMIT },
  { CONFLICT_W, TK_CONFLICT },
  { CONSTRAINT_W, TK_CONSTRAINT },
  { COPY_W, TK_COPY },
  { CREATE_W, TK_CREATE },
  { CROSS_W, TK_JOIN_KW },
  { DEFAULT_W, TK_DEFAULT },
  { DEFERRED_W, TK_DEFERRED },
  { DEFERRABLE_W, TK_DEFERRABLE },
  { DELETE_W, TK_DELETE },
  { DELIMITERS_W, TK_DELIMITERS },
  { DESC_W, TK_DESC },
  { DISTINCT_W, TK_DISTINCT },
  { DROP_W, TK_DROP },
  { END_W, TK_END },
  { EACH_W, TK_EACH },
  { ELSE_W, TK_ELSE },
  { EXCEPT_W, TK_EXCEPT },
  { EXPLAIN_W, TK_EXPLAIN },
  { FAIL_W, TK_FAIL },
  { FOR_W, TK_FOR },
  { FOREIGN_W, TK_FOREIGN },
  { FROM_W, TK_FROM },
  { FULL_W, TK_JOIN_KW },
  { GLOB_W, TK_GLOB },
  { GROUP_W, TK_GROUP },
  { HAVING_W, TK_HAVING },
  { HOLD_W, TK_HOLD },
  { IGNORE_W, TK_IGNORE },
  { IMMEDIATE_W, TK_IMMEDIATE },
  { IN_W, TK_IN },
  { INDEX_W, TK_INDEX },
  { INITIALLY_W, TK_INITIALLY },
  { INNER_W, TK_JOIN_KW },
  { INSERT_W, TK_INSERT },
  { INSTEAD_W, TK_INSTEAD },
  { INT_W, TK_INT },
  { INTERSECT_W, TK_INTERSECT },
  { INTO_W, TK_INTO },
  { IS_W, TK_IS },
  { ISNULL_W, TK_ISNULL },
  { JOIN_W, TK_JOIN },
  { KEY_W, TK_KEY },
  { LEFT_W, TK_JOIN_KW },
  { LIKE_W, TK_LIKE },
  { LIMIT_W, TK_LIMIT },
  { LOCALIZABLE_W, TK_LOCALIZABLE },
  { LONG_W, TK_LONG },
  { LONGCHAR_W, TK_LONGCHAR },
  { MATCH_W, TK_MATCH },
  { NATURAL_W, TK_JOIN_KW },
  { NOT_W, TK_NOT },
  { NOTNULL_W, TK_NOTNULL },
  { NULL_W, TK_NULL },
  { OBJECT_W, TK_OBJECT },
  { OF_W, TK_OF },
  { OFFSET_W, TK_OFFSET },
  { ON_W, TK_ON },
  { OR_W, TK_OR },
  { ORDER_W, TK_ORDER },
  { OUTER_W, TK_JOIN_KW },
  { PRAGMA_W, TK_PRAGMA },
  { PRIMARY_W, TK_PRIMARY },
  { RAISE_W, TK_RAISE },
  { REFERENCES_W, TK_REFERENCES },
  { REPLACE_W, TK_REPLACE },
  { RESTRICT_W, TK_RESTRICT },
  { RIGHT_W, TK_JOIN_KW },
  { ROLLBACK_W, TK_ROLLBACK },
  { ROW_W, TK_ROW },
  { SELECT_W, TK_SELECT },
  { SET_W, TK_SET },
  { SHORT_W, TK_SHORT },
  { STATEMENT_W, TK_STATEMENT },
  { TABLE_W, TK_TABLE },
  /*{ TEMPORARY_W, TK_TEMPORARY },*/
  { THEN_W, TK_THEN },
  { TRANSACTION_W, TK_TRANSACTION },
  { TRIGGER_W, TK_TRIGGER },
  { UNION_W, TK_UNION },
  { UNIQUE_W, TK_UNIQUE },
  { UPDATE_W, TK_UPDATE },
  { USING_W, TK_USING },
  { VACUUM_W, TK_VACUUM },
  { VALUES_W, TK_VALUES },
  { VIEW_W, TK_VIEW },
  { WHEN_W, TK_WHEN },
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
      if( z[1]=='-' ){
        for(i=2; z[i] && z[i]!='\n'; i++){}
        *tokenType = TK_COMMENT;
        return i;
      }
      *tokenType = TK_MINUS;
      return 1;
    }
    case '(': {
      if( z[1]=='+' && z[2]==')' ){
        *tokenType = TK_ORACLE_OUTER_JOIN;
        return 3;
      }else{
        *tokenType = TK_LP;
        return 1;
      }
    }
    case ')': {
      *tokenType = TK_RP;
      return 1;
    }
    case ';': {
      *tokenType = TK_SEMI;
      return 1;
    }
    case '+': {
      *tokenType = TK_PLUS;
      return 1;
    }
    case '*': {
      *tokenType = TK_STAR;
      return 1;
    }
    case '/': {
      if( z[1]!='*' || z[2]==0 ){
        *tokenType = TK_SLASH;
        return 1;
      }
      for(i=3; z[i] && (z[i]!='/' || z[i-1]!='*'); i++){}
      if( z[i] ) i++;
      *tokenType = TK_COMMENT;
      return i;
    }
    case '%': {
      *tokenType = TK_REM;
      return 1;
    }
    case '=': {
      *tokenType = TK_EQ;
      return 1 + (z[1]=='=');
    }
    case '<': {
      if( z[1]=='=' ){
        *tokenType = TK_LE;
        return 2;
      }else if( z[1]=='>' ){
        *tokenType = TK_NE;
        return 2;
      }else if( z[1]=='<' ){
        *tokenType = TK_LSHIFT;
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
      }else if( z[1]=='>' ){
        *tokenType = TK_RSHIFT;
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
    case '|': {
      if( z[1]!='|' ){
        *tokenType = TK_BITOR;
        return 1;
      }else{
        *tokenType = TK_CONCAT;
        return 2;
      }
    }
    case '?': {
      *tokenType = TK_WILDCARD;
      return 1;
    }
    case ',': {
      *tokenType = TK_COMMA;
      return 1;
    }
    case '&': {
      *tokenType = TK_BITAND;
      return 1;
    }
    case '~': {
      *tokenType = TK_BITNOT;
      return 1;
    }
    case '`': case '\'': case '"': {
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
      if( z[i]=='.' ){
        i++;
        while( isdigit(z[i]) ){ i++; }
        *tokenType = TK_FLOAT;
      }
      if( (z[i]=='e' || z[i]=='E') &&
           ( isdigit(z[i+1]) 
            || ((z[i+1]=='+' || z[i+1]=='-') && isdigit(z[i+2]))
           )
      ){
        i += 2;
        while( isdigit(z[i]) ){ i++; }
        *tokenType = TK_FLOAT;
      }else if( z[0]=='.' ){
        *tokenType = TK_FLOAT;
      }
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
