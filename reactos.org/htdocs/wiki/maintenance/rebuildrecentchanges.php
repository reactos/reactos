<?php
/**
 * Rebuild link tracking tables from scratch.  This takes several
 * hours, depending on the database size and server configuration.
 *
 * @file
 * @todo document
 * @ingroup Maintenance
 */

/** */
require_once( "commandLine.inc" );
require_once( "rebuildrecentchanges.inc" );
$wgTitle = Title::newFromText( "Rebuild recent changes script" );

$wgDBuser			= $wgDBadminuser;
$wgDBpassword		= $wgDBadminpassword;

rebuildRecentChangesTable();

print "Done.\n";
exit();


