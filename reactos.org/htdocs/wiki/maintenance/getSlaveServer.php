<?php
/**
 * This script reports the hostname of a slave server.
 *
 * @file
 * @ingroup Maintenance
 */

require_once( dirname(__FILE__).'/commandLine.inc' );

if ( $wgAllDBsAreLocalhost ) {
	# Can't fool the backup script
	print "localhost\n";
	exit;
}

if( isset( $options['group'] ) ) {
	$db = wfGetDB( DB_SLAVE, $options['group'] );
	$host = $db->getServer();
} else {
	$lb = wfGetLB();
	$i = $lb->getReaderIndex();
	$host = $lb->getServerName( $i );
}

print "$host\n";


