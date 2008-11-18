<?php

/**
 * Support functions for the removeUnusedAccounts maintenance script
 *
 * @file
 * @ingroup Maintenance
 * @author Rob Church <robchur@gmail.com>
 */

/**
 * Could the specified user account be deemed inactive?
 * (No edits, no deleted edits, no log entries, no current/old uploads)
 *
 * @param $id User's ID
 * @param $master Perform checking on the master
 * @return bool
 */
function isInactiveAccount( $id, $master = false ) {
	$dbo = wfGetDB( $master ? DB_MASTER : DB_SLAVE );
	$fname = 'isInactiveAccount';
	$checks = array( 'revision' => 'rev', 'archive' => 'ar', 'logging' => 'log',
					 'image' => 'img', 'oldimage' => 'oi' );
	$count = 0;

	$dbo->immediateBegin();
	foreach( $checks as $table => $fprefix ) {
		$conds = array( $fprefix . '_user' => $id );
		$count += (int)$dbo->selectField( $table, 'COUNT(*)', $conds, $fname );
	}
	$dbo->immediateCommit();

	return $count == 0;
}

/**
 * Show help for the maintenance script
 */
function showHelp() {
	echo( "Delete unused user accounts from the database.\n\n" );
	echo( "USAGE: php removeUnusedAccounts.php [--delete]\n\n" );
	echo( "  --delete : Delete accounts which are discovered to be inactive\n" );
	echo( "\n" );
}
