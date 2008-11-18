<?php
require_once 'counter.php';
/**
 * Rebuild the fulltext search indexes. This may take a while
 * depending on the database size and server configuration.
 *
 * Rebuilding is faster if you drop the index and recreate it,
 * but that will prevent searches from working while it runs.
 *
 * @file
 * @todo document
 * @ingroup Maintenance
 */

/** */
define( "RTI_CHUNK_SIZE", 500 );

function dropTextIndex( &$database )
{
	$searchindex = $database->tableName( 'searchindex' );
	if ( $database->indexExists( "searchindex", "si_title" ) ) {
		echo "Dropping index...\n";
		$sql = "ALTER TABLE $searchindex DROP INDEX si_title, DROP INDEX si_text";
		$database->query($sql, "dropTextIndex" );
	}
}

function createTextIndex( &$database )
{
	$searchindex = $database->tableName( 'searchindex' );
	echo "\nRebuild the index...\n";
	$sql = "ALTER TABLE $searchindex ADD FULLTEXT si_title (si_title), " .
	  "ADD FULLTEXT si_text (si_text)";
	$database->query($sql, "createTextIndex" );
}

function rebuildTextIndex( &$database )
{
	list ($page, $revision, $text, $searchindex) = $database->tableNamesN( 'page', 'revision', 'text', 'searchindex' );

	$sql = "SELECT MAX(page_id) AS count FROM $page";
	$res = $database->query($sql, "rebuildTextIndex" );
	$s = $database->fetchObject($res);
	$count = $s->count;
	echo "Rebuilding index fields for {$count} pages...\n";
	$n = 0;

	while ( $n < $count ) {
		print_c( $n - 1, $n);
		$end = $n + RTI_CHUNK_SIZE - 1;
		$sql = "SELECT page_id, page_namespace, page_title, old_flags, old_text
		          FROM $page, $revision, $text
		         WHERE page_id BETWEEN $n AND $end
		           AND page_latest=rev_id
		           AND rev_text_id=old_id";
		$res = $database->query($sql, "rebuildTextIndex" );

		while( $s = $database->fetchObject($res) ) {
			$revtext = Revision::getRevisionText( $s );
			$u = new SearchUpdate( $s->page_id, $s->page_title, $revtext );
			$u->doUpdate();
		}
		$database->freeResult( $res );
		$n += RTI_CHUNK_SIZE;
	}
}
