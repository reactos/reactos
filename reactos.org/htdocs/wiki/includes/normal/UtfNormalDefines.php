<?php

define( 'UNICODE_HANGUL_FIRST', 0xac00 );
define( 'UNICODE_HANGUL_LAST',  0xd7a3 );

define( 'UNICODE_HANGUL_LBASE', 0x1100 );
define( 'UNICODE_HANGUL_VBASE', 0x1161 );
define( 'UNICODE_HANGUL_TBASE', 0x11a7 );

define( 'UNICODE_HANGUL_LCOUNT', 19 );
define( 'UNICODE_HANGUL_VCOUNT', 21 );
define( 'UNICODE_HANGUL_TCOUNT', 28 );
define( 'UNICODE_HANGUL_NCOUNT', UNICODE_HANGUL_VCOUNT * UNICODE_HANGUL_TCOUNT );

define( 'UNICODE_HANGUL_LEND', UNICODE_HANGUL_LBASE + UNICODE_HANGUL_LCOUNT - 1 );
define( 'UNICODE_HANGUL_VEND', UNICODE_HANGUL_VBASE + UNICODE_HANGUL_VCOUNT - 1 );
define( 'UNICODE_HANGUL_TEND', UNICODE_HANGUL_TBASE + UNICODE_HANGUL_TCOUNT - 1 );

define( 'UNICODE_SURROGATE_FIRST', 0xd800 );
define( 'UNICODE_SURROGATE_LAST', 0xdfff );
define( 'UNICODE_MAX', 0x10ffff );
define( 'UNICODE_REPLACEMENT', 0xfffd );


define( 'UTF8_HANGUL_FIRST', "\xea\xb0\x80" /*codepointToUtf8( UNICODE_HANGUL_FIRST )*/ );
define( 'UTF8_HANGUL_LAST', "\xed\x9e\xa3" /*codepointToUtf8( UNICODE_HANGUL_LAST )*/ );

define( 'UTF8_HANGUL_LBASE', "\xe1\x84\x80" /*codepointToUtf8( UNICODE_HANGUL_LBASE )*/ );
define( 'UTF8_HANGUL_VBASE', "\xe1\x85\xa1" /*codepointToUtf8( UNICODE_HANGUL_VBASE )*/ );
define( 'UTF8_HANGUL_TBASE', "\xe1\x86\xa7" /*codepointToUtf8( UNICODE_HANGUL_TBASE )*/ );

define( 'UTF8_HANGUL_LEND', "\xe1\x84\x92" /*codepointToUtf8( UNICODE_HANGUL_LEND )*/ );
define( 'UTF8_HANGUL_VEND', "\xe1\x85\xb5" /*codepointToUtf8( UNICODE_HANGUL_VEND )*/ );
define( 'UTF8_HANGUL_TEND', "\xe1\x87\x82" /*codepointToUtf8( UNICODE_HANGUL_TEND )*/ );

define( 'UTF8_SURROGATE_FIRST', "\xed\xa0\x80" /*codepointToUtf8( UNICODE_SURROGATE_FIRST )*/ );
define( 'UTF8_SURROGATE_LAST', "\xed\xbf\xbf" /*codepointToUtf8( UNICODE_SURROGATE_LAST )*/ );
define( 'UTF8_MAX', "\xf4\x8f\xbf\xbf" /*codepointToUtf8( UNICODE_MAX )*/ );
define( 'UTF8_REPLACEMENT', "\xef\xbf\xbd" /*codepointToUtf8( UNICODE_REPLACEMENT )*/ );
#define( 'UTF8_REPLACEMENT', '!' );

define( 'UTF8_OVERLONG_A', "\xc1\xbf" );
define( 'UTF8_OVERLONG_B', "\xe0\x9f\xbf" );
define( 'UTF8_OVERLONG_C', "\xf0\x8f\xbf\xbf" );

# These two ranges are illegal
define( 'UTF8_FDD0', "\xef\xb7\x90" /*codepointToUtf8( 0xfdd0 )*/ );
define( 'UTF8_FDEF', "\xef\xb7\xaf" /*codepointToUtf8( 0xfdef )*/ );
define( 'UTF8_FFFE', "\xef\xbf\xbe" /*codepointToUtf8( 0xfffe )*/ );
define( 'UTF8_FFFF', "\xef\xbf\xbf" /*codepointToUtf8( 0xffff )*/ );

define( 'UTF8_HEAD', false );
define( 'UTF8_TAIL', true );
