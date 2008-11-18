<?php

/**
 * Support functions for the deleteOrphanedRevisions maintenance script
 *
 * @file
 * @ingroup Maintenance
 * @author Rob Church <robchur@gmail.com>
 */

/**
 * Delete one or more revisions from the database
 * Do this inside a transaction
 *
 * @param $id Array of revision id values
 * @param $db Database class (needs to be a master)
 */
function deleteRevisions( $id, &$dbw ) {
	if( !is_array( $id ) )
		$id = array( $id );
	$dbw->delete( 'revision', array( 'rev_id' => $id ), 'deleteRevision' );
}

/**
 * Spit out script usage information and exit
 */
function showUsage() {
	echo( "Finds revisions which refer to nonexisting pages and deletes them from the database\n" );
	echo( "USAGE: php deleteOrphanedRevisions.php [--report]\n\n" );
	echo( " --report : Prints out a count of affected revisions but doesn't delete them\n\n" );
}

