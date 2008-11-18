<?php

$wgNoDBParam = true;
$optionsWithArgs = array( 'o' );
require_once( dirname(__FILE__).'/../maintenance/commandLine.inc' );
require_once( dirname(__FILE__).'/serialize.php' );

$stderr = fopen( 'php://stderr', 'w' );
if ( !isset( $args[0] ) ) {
	fwrite( $stderr, "No input file specified\n" );
	exit( 1 );
}
$file = $args[0];
$code = str_replace( 'Messages', '', basename( $file ) );
$code = str_replace( '.php', '', $code );
$code = strtolower( str_replace( '_', '-', $code ) );

$localisation = Language::getLocalisationArray( $code, true );
if ( wfIsWindows() ) {
	$localisation = unixLineEndings( $localisation );
}

if ( isset( $options['o'] ) ) {
	$out = fopen( $options['o'], 'wb' );
	if ( !$out ) {
		fwrite( $stderr, "Unable to open file \"{$options['o']}\" for output\n" );
		exit( 1 );
	}
} else {
	$out = fopen( 'php://stdout', 'wb' );
}

fwrite( $out, serialize( $localisation ) );


