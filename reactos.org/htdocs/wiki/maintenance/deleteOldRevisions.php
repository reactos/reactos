<?php

/**
 * Delete old (non-current) revisions from the database
 *
 * @file
 * @ingroup Maintenance
 * @author Rob Church <robchur@gmail.com>
 */

$options = array( 'delete', 'help' );
require_once( 'commandLine.inc' );
require_once( 'deleteOldRevisions.inc' );

echo( "Delete Old Revisions\n\n" );

if( @$options['help'] ) {
	ShowUsage();
} else {
	DeleteOldRevisions( @$options['delete'], $args );
}

function ShowUsage() {
	echo( "Deletes non-current revisions from the database.\n\n" );
	echo( "Usage: php deleteOldRevisions.php [--delete|--help] [<page_id> ...]\n\n" );
	echo( "delete : Performs the deletion\n" );
	echo( "  help : Show this usage information\n" );
}

