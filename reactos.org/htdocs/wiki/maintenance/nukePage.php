<?php

/**
 * Erase a page record from the database
 * Irreversible (can't use standard undelete) and does not update link tables
 *
 * @file
 * @ingroup Maintenance
 * @author Rob Church <robchur@gmail.com>
 */

require_once( 'commandLine.inc' );
require_once( 'nukePage.inc' );

echo( "Erase Page Record\n\n" );

if( isset( $args[0] ) ) {
	NukePage( $args[0], true );
} else {
	ShowUsage();
}

/** Show script usage information */
function ShowUsage() {
	echo( "Remove a page record from the database.\n\n" );
	echo( "Usage: php nukePage.php <title>\n\n" );
	echo( "	<title> : Page title; spaces escaped with underscores\n\n" );
}

