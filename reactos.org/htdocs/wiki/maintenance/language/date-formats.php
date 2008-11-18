<?php
/**
 * @file
 * @ingroup MaintenanceLanguage
 */

$ts = '20010115123456';

	
$IP = dirname( __FILE__ ) . '/../..';
require_once( "$IP/maintenance/commandLine.inc" );

foreach ( glob( "$IP/languages/messages/Messages*.php" ) as $filename ) {
	$base = basename( $filename );
	$m = array();
	if ( !preg_match( '/Messages(.*)\.php$/', $base, $m ) ) {
		continue;
	}
	$code = str_replace( '_', '-', strtolower( $m[1] ) );
	print "$code ";
	$lang = Language::factory( $code );
	$prefs = $lang->getDatePreferences();
	if ( !$prefs ) {
		$prefs = array( 'default' );
	}
	print "date: ";
	foreach ( $prefs as $index => $pref ) {
		if ( $index > 0 ) {
			print ' | ';
		}
		print $lang->date( $ts, false, $pref );
	}
	print "\n$code time: ";
	foreach ( $prefs as $index => $pref ) {
		if ( $index > 0 ) {
			print ' | ';
		}
		print $lang->time( $ts, false, $pref );
	}
	print "\n$code both: "; 
	foreach ( $prefs as $index => $pref ) {
		if ( $index > 0 ) {
			print ' | ';
		}
		print $lang->timeanddate( $ts, false, $pref );
	}
	print "\n\n";
}


