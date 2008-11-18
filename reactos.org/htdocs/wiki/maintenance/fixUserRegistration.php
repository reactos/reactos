<?php
/**
 * Fix the user_registration field.
 * In particular, for values which are NULL, set them to the date of the first edit
 *
 * @file
 * @ingroup Maintenance
 */

require_once( 'commandLine.inc' );

$fname = 'fixUserRegistration.php';

$dbr = wfGetDB( DB_SLAVE );
$dbw = wfGetDB( DB_MASTER );

// Get user IDs which need fixing
$res = $dbr->select( 'user', 'user_id', 'user_registration IS NULL', $fname );

while ( $row = $dbr->fetchObject( $res ) ) {
	$id = $row->user_id;
	// Get first edit time
	$timestamp = $dbr->selectField( 'revision', 'MIN(rev_timestamp)', array( 'rev_user' => $id ), $fname );
	// Update
	if ( !empty( $timestamp ) ) {
		$dbw->update( 'user', array( 'user_registration' => $timestamp ), array( 'user_id' => $id ), $fname );
		print "$id $timestamp\n";
	} else {
		print "$id NULL\n";
	}
}
print "\n";


