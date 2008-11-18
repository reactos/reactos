<?php
/**
 * @file
 * @ingroup Maintenance
 */

require 'commandLine.inc';

$lb = wfGetLB();

if( $lb->getServerCount() == 1 ) {
	echo "This script dumps replication lag times, but you don't seem to have\n";
	echo "a multi-host db server configuration.\n";
} else {
	$lags = $lb->getLagTimes();
	foreach( $lags as $n => $lag ) {
		$host = $lb->getServerName( $n );
		if( IP::isValid( $host ) ) {
			$ip = $host;
			$host = gethostbyaddr( $host );
		} else {
			$ip = gethostbyname( $host );
		}
		$starLen = min( intval( $lag ), 40 );
		$stars = str_repeat( '*', $starLen );
		printf( "%10s %20s %3d %s\n", $ip, $host, $lag, $stars );
	}
}

