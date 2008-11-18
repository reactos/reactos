<?php
/**
 * Why yes, this *is* another special-purpose Wikimedia maintenance script!
 * Should be fixed up and generalized.
 *
 * @file
 * @ingroup Maintenance
 */

require_once( "commandLine.inc" );

if ( count( $args ) != 2 ) {
	wfDie( "Rename external storage dbs and leave a new one...\n" .
			"Usage: php renamewiki.php <olddb> <newdb>\n" );
}

list( $from, $to ) = $args;

echo "Renaming blob tables in ES from $from to $to...\n";
echo "Sleeping 5 seconds...";
sleep(5);
echo "\n";

$maintenance = "$IP/maintenance";

# Initialise external storage
if ( is_array( $wgDefaultExternalStore ) ) {
	$stores = $wgDefaultExternalStore;
} elseif ( $wgDefaultExternalStore ) {
	$stores = array( $wgDefaultExternalStore );
} else {
	$stores = array();
}
if ( count( $stores ) ) {
	require_once( 'ExternalStoreDB.php' );
	print "Initialising external storage $store...\n";
	global $wgDBuser, $wgDBpassword, $wgExternalServers;
	foreach ( $stores as $storeURL ) {
		$m = array();
		if ( !preg_match( '!^DB://(.*)$!', $storeURL, $m ) ) {
			continue;
		}
		
		$cluster = $m[1];
		
		# Hack
		$wgExternalServers[$cluster][0]['user'] = $wgDBuser;
		$wgExternalServers[$cluster][0]['password'] = $wgDBpassword;
		
		$store = new ExternalStoreDB;
		$extdb =& $store->getMaster( $cluster );
		$extdb->query( "SET table_type=InnoDB" );
		$extdb->query( "CREATE DATABASE {$to}" );
		$extdb->query( "ALTER TABLE {$from}.blobs RENAME TO {$to}.blobs" );
		$extdb->selectDB( $from );
		dbsource( "$maintenance/storage/blobs.sql", $extdb );
		$extdb->immediateCommit();
	}
}

echo "done.\n";

