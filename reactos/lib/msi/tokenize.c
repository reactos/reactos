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
#include "winnls.h"
#include "query.h"
#include "sql.tab.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/*
** All the keywords of the SQL language are stored as in a hash
** table composed of instances of the following structure.
*/
typedef struct Keyword Keyword;
struct Keyword {
  const char *zName;             /* The keyword name */
  int tokenType;           /* The token value for this keyword */
};

/*
** These are the keywords
*/
static const Keyword aKeywordTable[] = {
  { "ABORT", TK_ABORT },
  { "AFTER", TK_AFTER },
  { "ALL", TK_ALL },
  { "AND", TK_AND },
  { "AS", TK_AS },
  { "ASC", TK_ASC },
  { "BEFORE", TK_BEFORE },
  { "BEGIN", TK_BEGIN },
  { "BETWEEN", TK_BETWEEN },
  { "BY", TK_BY },
  { "CASCADE", TK_CASCADE },
  { "CASE", TK_CASE },
  { "CHAR", TK_CHAR },
  { "CHARACTER", TK_CHAR },
  { "CHECK", TK_CHECK },
  { "CLUSTER", TK_CLUSTER },
  { "COLLATE", TK_COLLATE },
  { "COMMIT", TK_COMMIT },
  { "CONFLICT", TK_CONFLICT },
  { "CONSTRAINT", TK_CONSTRAINT },
  { "COPY", TK_COPY },
  { "CREATE", TK_CREATE },
  { "CROSS", TK_JOIN_KW },
  { "DEFAULT", TK_DEFAULT },
  { "DEFERRED", TK_DEFERRED },
  { "DEFERRABLE", TK_DEFERRABLE },
  { "DELETE", TK_DELETE },
  { "DELIMITERS", TK_DELIMITERS },
  { "DESC", TK_DESC },
  { "DISTINCT", TK_DISTINCT },
  { "DROP", TK_DROP },
  { "END", TK_END },
  { "EACH", TK_EACH },
  { "ELSE", TK_ELSE },
  { "EXCEPT", TK_EXCEPT },
  { "EXPLAIN", TK_EXPLAIN },
  { "FAIL", TK_FAIL },
  { "FOR", TK_FOR },
  { "FOREIGN", TK_FOREIGN },
  { "FROM", TK_FROM },
  { "FULL", TK_JOIN_KW },
  { "GLOB", TK_GLOB },
  { "GROUP", TK_GROUP },
  { "HAVING", TK_HAVING },
  { "HOLD", TK_HOLD },
  { "IGNORE", TK_IGNORE },
  { "IMMEDIATE", TK_IMMEDIATE },
  { "IN", TK_IN },
  { "INDEX", TK_INDEX },
  { "INITIALLY", TK_INITIALLY },
  { "INNER", TK_JOIN_KW },
  { "INSERT", TK_INSERT },
  { "INSTEAD", TK_INSTEAD },
  { "INT", TK_INT },
  { "INTERSECT", TK_INTERSECT },
  { "INTO", TK_INTO },
  { "IS", TK_IS },
  { "ISNULL", TK_ISNULL },
  { "JOIN", TK_JOIN },
  { "KEY", TK_KEY },
  { "LEFT", TK_JOIN_KW },
  { "LIKE", TK_LIKE },
  { "LIMIT", TK_LIMIT },
  { "LOCALIZABLE", TK_LOCALIZABLE },
  { "LONG", TK_LONG },
  { "LONGCHAR", TK_LONGCHAR },
  { "MATCH", TK_MATCH },
  { "NATURAL", TK_JOIN_KW },
  { "NOT", TK_NOT },
  { "NOTNULL", TK_NOTNULL },
  { "NULL", TK_NULL },
  { "OBJECT", TK_OBJECT },
  { "OF", TK_OF },
  { "OFFSET", TK_OFFSET },
  { "ON", TK_ON },
  { "OR", TK_OR },
  { "ORDER", TK_ORDER },
  { "OUTER", TK_JOIN_KW },
  { "PRAGMA", TK_PRAGMA },
  { "PRIMARY", TK_PRIMARY },
  { "RAISE", TK_RAISE },
  { "REFERENCES", TK_REFERENCES },
  { "REPLACE", TK_REPLACE },
  { "RESTRICT", TK_RESTRICT },
  { "RIGHT", TK_JOIN_KW },
  { "ROLLBACK", TK_ROLLBACK },
  { "ROW", TK_ROW },
  { "SELECT", TK_SELECT },
  { "SET", TK_SET },
  { "SHORT", TK_SHORT },
  { "STATEMENT", TK_STATEMENT },
  { "TABLE", TK_TABLE },
  { "TEMP", TK_TEMP },
  { "TEMPORARY", TK_TEMP },
  { "THEN", TK_THEN },
  { "TRANSACTION", TK_TRANSACTION },
  { "TRIGGER", TK_TRIGGER },
  { "UNION", TK_UNION },
  { "UNIQUE", TK_UNIQUE },
  { "UPDATE", TK_UPDATE },
  { "USING", TK_USING },
  { "VACUUM", TK_VACUUM },
  { "VALUES", TK_VALUES },
  { "VIEW", TK_VIEW },
  { "WHEN", TK_WHEN },
  { "WHERE", TK_WHERE },
};

#define KEYWORD_COUNT ( sizeof aKeywordTable/sizeof (Keyword) )

/*
** This function looks up an identifier to determine if it is a
** keyword.  If it is a keyword, the token code of that keyword is 
** returned.  If the input is not a keyword, TK_ID is returned.
*/
int sqliteKeywordCode(const WCHAR *z, int n){
  UINT i, len;
  char buffer[0x10];

  len = WideCharToMultiByte( CP_ACP, 0, z, n, buffer, sizeof buffer, NULL, NULL );
  for(i=0; i<len; i++)
      buffer[i] = toupper(buffer[i]);
  for(i=0; i<KEYWORD_COUNT; i++)
  {
      if(memcmp(buffer, aKeywordTable[i].zName, len))
          continue;
      if(strlen(aKeywordTable[i].zName) == len )
          return aKeywordTable[i].tokenType;
  }
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
