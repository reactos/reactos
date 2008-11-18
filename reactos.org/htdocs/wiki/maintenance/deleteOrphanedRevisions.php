<?php

/**
 * Maintenance script to delete revisions which refer to a nonexisting page
 * Sometimes manual deletion done in a rush leaves crap in the database
 *
 * @file
 * @ingroup Maintenance
 * @author Rob Church <robchur@gmail.com>
 * @todo More efficient cleanup of text records
 */
 
$options = array( 'report', 'help' );
require_once( 'commandLine.inc' );
require_once( 'deleteOrphanedRevisions.inc.php' );
echo( "Delete Orphaned Revisions\n" );

if( isset( $options['help'] ) )
	showUsage();

$report = isset( $options['report'] );

$dbw = wfGetDB( DB_MASTER );
$dbw->immediateBegin();
extract( $dbw->tableNames( 'page', 'revision' ) );

# Find all the orphaned revisions
echo( "Checking for orphaned revisions..." );
$sql = "SELECT rev_id FROM {$revision} LEFT JOIN {$page} ON rev_page = page_id WHERE page_namespace IS NULL";
$res = $dbw->query( $sql, 'deleteOrphanedRevisions' );

# Stash 'em all up for deletion (if needed)
while( $row = $dbw->fetchObject( $res ) )
	$revisions[] = $row->rev_id;
$dbw->freeResult( $res );
$count = count( $revisions );
echo( "found {$count}.\n" );

# Nothing to do?
if( $report || $count == 0 ) {
	$dbw->immediateCommit();
	exit();
}

# Delete each revision
echo( "Deleting..." );
deleteRevisions( $revisions, $dbw );
echo( "done.\n" );

# Close the transaction and call the script to purge unused text records
$dbw->immediateCommit();
require_once( 'purgeOldText.inc' );
PurgeRedundantText( true );

