<?php

/**
 * Support functions for cleaning up redundant text records
 *
 * @file
 * @ingroup Maintenance
 * @author Rob Church <robchur@gmail.com>
 */

function PurgeRedundantText( $delete = false ) {
	
	# Data should come off the master, wrapped in a transaction
	$dbw = wfGetDB( DB_MASTER );
	$dbw->begin();
	
	$tbl_arc = $dbw->tableName( 'archive' );
	$tbl_rev = $dbw->tableName( 'revision' );
	$tbl_txt = $dbw->tableName( 'text' );
	
	# Get "active" text records from the revisions table
	echo( "Searching for active text records in revisions table..." );
	$res = $dbw->query( "SELECT DISTINCTROW rev_text_id FROM $tbl_rev" );
	while( $row = $dbw->fetchObject( $res ) ) {
		$cur[] = $row->rev_text_id;
	}
	echo( "done.\n" );
	
	# Get "active" text records from the archive table
	echo( "Searching for active text records in archive table..." );
	$res = $dbw->query( "SELECT DISTINCTROW ar_text_id FROM $tbl_arc" );
	while( $row = $dbw->fetchObject( $res ) ) {
		$cur[] = $row->ar_text_id;
	}
	echo( "done.\n" );
	
	# Get the IDs of all text records not in these sets
	echo( "Searching for inactive text records..." );
	$set = implode( ', ', $cur );
	$res = $dbw->query( "SELECT old_id FROM $tbl_txt WHERE old_id NOT IN ( $set )" );
	$old = array();
	while( $row = $dbw->fetchObject( $res ) ) {
		$old[] = $row->old_id;
	}
	echo( "done.\n" );
	
	# Inform the user of what we're going to do
	$count = count( $old );
	echo( "$count inactive items found.\n" );
	
	# Delete as appropriate
	if( $delete && $count ) {
		echo( "Deleting..." );
		$set = implode( ', ', $old );
		$dbw->query( "DELETE FROM $tbl_txt WHERE old_id IN ( $set )" );
		echo( "done.\n" );
	}
	
	# Done
	$dbw->commit();
	
}
