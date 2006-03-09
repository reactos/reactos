#include <stdio.h>

int main( int argc, char **argv ) {
    char buf[8];
    int rlen, i;
    FILE *fin = NULL, *fout = NULL;

    if( argc < 3 ) return 1;

    fin = fopen( argv[1], "rb" );

    if( fin ) 
        fout = fopen( argv[2], "wb" );
    if( !fout ) return 1;

    do { 
        rlen = fread( buf, 1, 8, fin );
        for( i = 7; rlen > 0 && i >= 0; i-- ) fputc( buf[i], fout );
    }
    while( rlen == 8 );

    fclose( fout );

    return 0;
}
