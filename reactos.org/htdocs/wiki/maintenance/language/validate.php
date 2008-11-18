<?php
/**
 * @file
 * @ingroup MaintenanceLanguage
 */

if ( !isset( $argv[1] ) ) {
	print "Usage: php {$argv[0]} <filename>\n";
	exit( 1 );
}
array_shift( $argv );

define( 'MEDIAWIKI', 1 );
define( 'NOT_REALLY_MEDIAWIKI', 1 );

$IP = dirname( __FILE__ ) . '/../..';

require_once( "$IP/includes/Defines.php" );
require_once( "$IP/languages/Language.php" );

$files = array();
foreach ( $argv as $arg ) {
	$files = array_merge( $files, glob( $arg ) );
}

foreach ( $files as $filename ) {
	print "$filename...";
	$vars = getVars( $filename );
	$keys = array_keys( $vars );
	$diff = array_diff( $keys, Language::$mLocalisationKeys );
	if ( $diff ) {
		print "\nWarning: unrecognised variable(s): " . implode( ', ', $diff ) ."\n";
	} else {
		print " ok\n";
	}
}

function getVars( $filename ) {
	require( $filename );
	$vars = get_defined_vars();
	unset( $vars['filename'] );
	return $vars;
}

