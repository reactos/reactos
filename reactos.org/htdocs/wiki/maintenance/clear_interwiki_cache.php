<?php
/**
 * This script is used to clear the interwiki links for ALL languages in
 * memcached.
 *
 * @file
 * @ingroup Maintenance
 */

/** */
require_once('commandLine.inc');

$dbr = wfGetDB( DB_SLAVE );
$res = $dbr->select( 'interwiki', array( 'iw_prefix' ), false );
$prefixes = array();
while ( $row = $dbr->fetchObject( $res ) ) {
	$prefixes[] = $row->iw_prefix;
}

foreach ( $wgLocalDatabases as $db ) {
	print "$db ";
	foreach ( $prefixes as $prefix ) {
		$wgMemc->delete("$db:interwiki:$prefix");
	}
}
print "\n";

