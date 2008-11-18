<?php
/**
 * Undelete a page by fetching it from the archive table
 *
 * @file
 * @ingroup Maintenance
 */

$usage = <<<EOT
Undelete a page
Usage: php undelete.php [-u <user>] [-r <reason>] <pagename>

EOT;

$optionsWithArgs = array( 'u', 'r' );
require_once( 'commandLine.inc' );

$user = 'Command line script';
$reason = '';

if ( isset( $options['u'] ) ) {
	$user = $options['u'];
}
if ( isset( $options['r'] ) ) {
	$reason = $options['r'];
}
$pageName = @$args[0];
$title = Title::newFromText( $pageName );
if ( !$title ) {
	echo $usage;
	exit( 1 );
}
$wgUser = User::newFromName( $user );
$archive = new PageArchive( $title );
echo "Undeleting " . $title->getPrefixedDBkey() . '...';
$archive->undelete( array(), $reason );
echo "done\n";


