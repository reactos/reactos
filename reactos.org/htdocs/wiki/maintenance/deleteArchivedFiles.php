<?php

/**
 * Delete archived (non-current) files from the database
 *
 * @file
 * @ingroup Maintenance
 * @author Aaron Schulz
 * Based on deleteOldRevisions.php by Rob Church
 */

$options = array( 'delete', 'help' );
require_once( 'commandLine.inc' );
require_once( 'deleteArchivedFiles.inc' );

echo( "Delete Archived Images\n\n" );

if( @$options['help'] ) {
	ShowUsage();
} else {
	DeleteArchivedFiles( @$options['delete'] );
}

function ShowUsage() {
	echo( "Deletes all archived images.\n\n" );
	echo( "These images will no longer be restorable.\n\n" );
	echo( "Usage: php deleteArchivedRevisions.php [--delete|--help]\n\n" );
	echo( "delete : Performs the deletion\n" );
	echo( "  help : Show this usage information\n" );
}

