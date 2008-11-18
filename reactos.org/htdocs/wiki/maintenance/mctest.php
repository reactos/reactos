<?php
/**
 * This script makes several 'set', 'incr' and 'get' requests on every
 * memcached server and shows a report.
 *
 * $Id: mctest.php 37886 2008-07-21 17:06:35Z greg $
 * @file
 * @ingroup Maintenance
 */

$optionsWithArgs = array( 'i' );

require_once('commandLine.inc');

function microtime_float()
{
   list($usec, $sec) = explode(" ", microtime());
   return ((float)$usec + (float)$sec);
}


#$wgDebugLogFile = '/dev/stdout';

if ( isset( $args[0] ) ) {
	$wgMemCachedServers = array( $args[0] );
}
if ( isset( $options['i'] ) ) {
	$iterations = $options['i'];
} else {
	$iterations = 100;
}

foreach ( $wgMemCachedServers as $server ) {
        print "$server ";
	$mcc = new MemCachedClientforWiki( array('persistant' => true) );
	$mcc->set_servers( array( $server ) );
	$set = 0;
	$incr = 0;
	$get = 0;
        $time_start=microtime_float();
	for ( $i=1; $i<=$iterations; $i++ ) {
		if ( !is_null( $mcc->set( "test$i", $i ) ) ) {
			$set++;
		}
	}

	for ( $i=1; $i<=$iterations; $i++ ) {
		if ( !is_null( $mcc->incr( "test$i", $i ) ) ) {
			$incr++;
		}
	}

	for ( $i=1; $i<=$iterations; $i++ ) {
		$value = $mcc->get( "test$i" );
		if ( $value == $i*2 ) {
			$get++;
		}
	}
        $exectime=microtime_float()-$time_start;

	print "set: $set   incr: $incr   get: $get time: $exectime\n";
}



