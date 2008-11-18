<?php
/**
 * Script for re-attributing edits
 * Use reassingEdits.php
 *
 * @file
 * @ingroup Maintenance
 */

/** */
require_once( "commandLine.inc" );

# Parameters
if ( count( $args ) < 2 ) {
	print "Not enough parameters\n";
	if ( $wgWikiFarm ) {
		print "Usage: php attribute.php <language> <site> <source> <destination>\n";
	} else {
		print "Usage: php attribute.php <source> <destination>\n";
	}
	exit;
}

$source = $args[0];
$dest = $args[1];

$dbr = wfGetDB( DB_SLAVE );
extract( $dbr->tableNames( 'page', 'revision','user' ));
$eSource = $dbr->strencode( $source );
$eDest = $dbr->strencode( $dest );

# Get user id
$res = $dbr->query( "SELECT user_id FROM $user WHERE user_name='$eDest'" );
$row = $dbr->fetchObject( $res );
if ( !$row ) {
	print "Warning: the target name \"$dest\" does not exist";
	$uid = 0;
} else {
	$uid = $row->user_id;
}

# Initialise files
$logfile = fopen( "attribute.log", "a" );
$sqlfile = fopen( "attribute.sql", "a" );

fwrite( $logfile, "* $source &rarr; $dest\n" );

fwrite( $sqlfile,
"-- Changing attribution SQL file
-- Generated with attribute.php
-- $source -> $dest ($uid)
");

$omitTitle = "Wikipedia:Changing_attribution_for_an_edit";

# Get revisions
print "\nPage revisions\n\n";

$res = $dbr->query( "SELECT page_namespace, page_title, rev_id, rev_timestamp
FROM $revision,$page
WHERE rev_user_text='$eSource' and rev_page=page_id" );
$row = $dbr->fetchObject( $res );

if ( $row ) {
/*
	if ( $row->old_title=='Votes_for_deletion' && $row->old_namespace == 4 ) {
		# We don't have that long
		break;
	}
*/
	fwrite( $logfile, "**Revision IDs: " );
	fwrite( $sqlfile, "UPDATE $revision SET rev_user=$uid, rev_user_text='$eDest' WHERE rev_id IN (\n" );

	for ( $first=true; $row; $row = $dbr->fetchObject( $res ) ) {
		$title = Title::makeTitle( $row->page_namespace, $row->page_title );
		$fullTitle = $title->getPrefixedDbKey();
		if ( $fullTitle == $omitTitle ) {
			continue;
		}

		print "$fullTitle\n";
		$url = $title->getFullUrl( "oldid={$row->rev_id}" );

		# Output
		fwrite( $sqlfile, "      " );
		if ( $first ) {
			$first = false;
		} else {
			fwrite( $sqlfile, ", " );
			fwrite( $logfile, ", " );
		}

		fwrite( $sqlfile, "{$row->rev_id} -- $url\n" );
		fwrite( $logfile, "[$url {$row->rev_id}]" );

	}
	fwrite( $sqlfile, ");\n" );
	fwrite( $logfile, "\n" );
}

print "\n";

fclose( $sqlfile );
fclose( $logfile );


