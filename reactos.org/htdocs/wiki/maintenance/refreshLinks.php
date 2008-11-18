<?php
/**
 * @file
 * @ingroup Maintenance
 */

/** */
$optionsWithArgs = array( 'm', 'e' );

require_once( "commandLine.inc" );
require_once( "refreshLinks.inc" );

if( isset( $options['help'] ) ) {
	echo <<<TEXT
Usage:
    php refreshLinks.php --help
    php refreshLinks.php [<start>] [-e <end>] [-m <maxlag>] [--dfn-only]
                         [--new-only] [--redirects-only]
    php refreshLinks.php [<start>] [-e <end>] [-m <maxlag>] --old-redirects-only

    --help               : This help message
    --dfn-only           : Delete links from nonexistent articles only
    --new-only           : Only affect articles with just a single edit
    --redirects-only     : Only fix redirects, not all links
    --old-redirects-only : Only fix redirects with no redirect table entry
    -m <number>          : Maximum replication lag
    <start>              : First page id to refresh
    -e <number>          : Last page id to refresh

TEXT;
	exit(0);
}

error_reporting( E_ALL & (~E_NOTICE) );

if ( !$options['dfn-only'] ) {
	if ( isset( $args[0] ) ) {
		$start = (int)$args[0];
	} else {
		$start = 1;
	}

	refreshLinks( $start, $options['new-only'], $options['m'], $options['e'], $options['redirects-only'], $options['old-redirects-only'] );
}
// this bit's bad for replication: disabling temporarily
// --brion 2005-07-16
//deleteLinksFromNonexistent();

if ( $options['globals'] ) {
	print_r( $GLOBALS );
}


