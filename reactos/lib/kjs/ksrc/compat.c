#include "ddk/ntddk.h"
#include "ddk/kefuncs.h"
#include "ctype.h"
#include "jsconfig.h"

void __kernel_abort() {
  KEBUGCHECK(0);
}

void _assert( const char *expr, const char *file, int line ) {
  DbgPrint("%s -- %s:%d\n", expr,file,line );
  __kernel_abort();
}

int isalnum( int x ) { return isalpha(x) || isdigit(x); }
int iscntrl( int x ) { return 32 > x; }
int ispunct( int x ) { return !isspace(x) && !isalnum(x) && !iscntrl(x) && !isspace(x); }

static int belongs_to_base( int x, int base ) {
  if( x >= '0' && '9' >= x ) {
    if( base > x - '0' ) return x - '0';
  }
  if( x >= 'a' && 'z' >= x ) {
    if( base > x - 'a' + 10 ) return x - 'a' + 10;
  }
  if( x >= 'A' && 'A' >= x ) {
    if( base > x - 'A' + 10 ) return x - 'A' + 10;
  }
  return -1;
}

long strtol( const char *data, char **endptr, int base ) {
    long out_number = 0;

    if( base == 0 ) {
        if( data[0] == '0' && (data[1] == 'x' || data[1] == 'X') ) {
            base = 16;
            data += 2;
        } else if( data[0] == '0' ) {
            base = 8;
            data++;
        } else {
            base = 10;
        }
    }

    while( data && *data && belongs_to_base( *data, base ) != -1 ) {
        out_number *= base;
        out_number += belongs_to_base( *data, base );
	data++;
    }
    if( endptr ) *endptr = (char *)data;
    return out_number;
} 

double strtod( const char *data, char **endptr ) {
    *endptr = (char *)data;
    return 0.0;
}

int memcmp( const void *a, const void *b, size_t s ) {
    char *aa = (char *)a, *bb = (char *)b;
    while(s && *aa == *bb) {
        aa++; bb++; s--;
    }
    return s ? *bb - *aa : 0;
}
