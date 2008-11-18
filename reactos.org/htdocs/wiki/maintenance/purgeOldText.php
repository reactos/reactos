<?php

/**
 * Purge old text records from the database
 *
 * @file
 * @ingroup Maintenance
 * @author Rob Church <robchur@gmail.com>
 */

$options = array( 'purge', 'help' );
require_once( 'commandLine.inc' );
require_once( 'purgeOldText.inc' );

echo( "Purge Old Text\n\n" );

if( @$options['help'] ) {
	ShowUsage();
} else {
	PurgeRedundantText( @$options['purge'] );
}

function ShowUsage() {
	echo( "Prunes unused text records from the database.\n\n" );
	echo( "Usage: php purgeOldText.php [--purge]\n\n" );
	echo( "purge : Performs the deletion\n" );
	echo( " help : Show this usage information\n" );
}

