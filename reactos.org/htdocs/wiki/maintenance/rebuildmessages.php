<?php
/**
 * This script purges all language messages from memcached
 * @file
 * @ingroup Maintenance
 */

require_once( 'commandLine.inc' );

if( $wgLocalDatabases ) {
	$databases = $wgLocalDatabases;
} else {
	$databases = array( $wgDBname );
}

foreach( $databases as $db ) {
	echo "Deleting message cache for {$db}... ";
	$messageMemc->delete( "{$db}:messages" );
	if( $wgEnableSidebarCache )
		$messageMemc->delete( "{$db}:sidebar" );
	echo "Deleted\n";
}
