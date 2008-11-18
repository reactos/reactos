<?php

/**
 * Delete archived (deleted from public) revisions from the database
 *
 * @file
 * @ingroup Maintenance
 * @author Aaron Schulz
 * Shamelessly stolen from deleteOldRevisions.php by Rob Church :)
 */

$options = array( 'delete', 'help' );
require_once( 'commandLine.inc' );
require_once( 'deleteArchivedRevisions.inc' );

echo( "Delete Archived Revisions\n\n" );

if( @$options['help'] ) {
	ShowUsage();
} else {
	DeleteArchivedRevisions( @$options['delete'] );
}

function ShowUsage() {
	echo( "Deletes all archived revisions.\n\n" );
	echo( "These revisions will no longer be restorable.\n\n" );
	echo( "Usage: php deleteArchivedRevisions.php [--delete|--help]\n\n" );
	echo( "delete : Performs the deletion\n" );
	echo( "  help : Show this usage information\n" );
}

