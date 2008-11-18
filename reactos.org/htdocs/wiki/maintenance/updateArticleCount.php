<?php
/**
 * Maintenance script to provide a better count of the number of articles
 * and update the site statistics table, if desired
 *
 * @file
 * @ingroup Maintenance
 * @author Rob Church <robchur@gmail.com>
 */

$options = array( 'update', 'help' );
require_once( 'commandLine.inc' );
require_once( 'updateArticleCount.inc.php' );
echo( "Update Article Count\n\n" );

if( isset( $options['help'] ) && $options['help'] ) {
	echo( "Usage: php updateArticleCount.php [--update]\n\n" );
	echo( "--update : Update site statistics table\n" );
	exit( 0 );
}

echo( "Counting articles..." );
$counter = new ArticleCounter();
$result = $counter->count();

if( $result !== false ) {
	echo( "found {$result}.\n" );
	if( isset( $options['update'] ) && $options['update'] ) {
		echo( "Updating site statistics table... " );
		$dbw = wfGetDB( DB_MASTER );
		$dbw->update( 'site_stats', array( 'ss_good_articles' => $result ), array( 'ss_row_id' => 1 ), __METHOD__ );
		echo( "done.\n" );
	} else {
		echo( "To update the site statistics table, run the script with the --update option.\n" );
	}
} else {
	echo( "failed.\n" );
}
echo( "\n" );

