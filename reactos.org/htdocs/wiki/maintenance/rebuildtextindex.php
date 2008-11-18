<?php
/**
 * Rebuild search index table from scratch.  This takes several
 * hours, depending on the database size and server configuration.
 *
 * This is only for MySQL (see bug 9905).
 * Postgres is trigger-based and should never need rebuilding.
 *
 * @file
 * @todo document
 * @ingroup Maintenance
 */

/** */
require_once( "commandLine.inc" );
require_once( "rebuildtextindex.inc" );

$database = wfGetDB( DB_MASTER );
if( !$database instanceof DatabaseMysql ) {
	print "This script is only for MySQL.\n";
	exit();
}

$wgTitle = Title::newFromText( "Rebuild text index script" );

dropTextIndex( $database );
rebuildTextIndex( $database );
createTextIndex( $database );

print "Done.\n";
exit();


