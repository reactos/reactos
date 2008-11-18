<?php
/**
 * @file
 * @ingroup Maintenance
 */

require_once( 'commandLine.inc' );
require_once( "$IP/includes/LinkFilter.php" );

function cleanupArticle( $id, $domain ) {
	$title = Title::newFromID( $id );
	if ( !$title ) {
		print "Internal error: no page for ID $id\n";
		return;
	}

	print $title->getPrefixedDBkey() . " ...";
	$rev = Revision::newFromTitle( $title );
	$revId = $rev->getId();
	$currentRevId = $revId;
	
	while ( $rev && LinkFilter::matchEntry( $rev->getText() , $domain ) ) {
		# Revision::getPrevious can't be used in this way before MW 1.6 (Revision.php 1.26)
		#$rev = $rev->getPrevious();
		$revId = $title->getPreviousRevisionID( $revId );
		if ( $revId ) {
			$rev = Revision::newFromTitle( $title, $revId );
		} else {
			$rev = false;
		}
	}
	if ( $revId == $currentRevId ) {
		// The regex didn't match the current article text
		// This happens e.g. when a link comes from a template rather than the page itself
		print "False match\n";
	} else {
		$dbw = wfGetDB( DB_MASTER );
		$dbw->immediateBegin();
		if ( !$rev ) {
			// Didn't find a non-spammy revision, blank the page
			print "blanking\n";
			$article = new Article( $title );
			$article->updateArticle( '', wfMsg( 'spam_blanking', $domain ),
				false, false );

		} else {
			// Revert to this revision
			print "reverting\n";
			$article = new Article( $title );
			$article->updateArticle( $rev->getText(), wfMsg( 'spam_reverting', $domain ), false, false );
		}
		$dbw->immediateCommit();
		wfDoUpdates();
	}
}
//------------------------------------------------------------------------------




$username = wfMsg( 'spambot_username' );
$fname = $username;
$wgUser = User::newFromName( $username );
// Create the user if necessary
if ( !$wgUser->getId() ) {
	$wgUser->addToDatabase();
}

if ( !isset( $args[0] ) ) {
	print "Usage: php cleanupSpam.php <hostname>\n";
	exit(1);
}
$spec = $args[0];
$like = LinkFilter::makeLike( $spec );
if ( !$like ) {
	print "Not a valid hostname specification: $spec\n";
	exit(1);
}

$dbr = wfGetDB( DB_SLAVE );

if ( isset($options['all']) ) {
	// Clean up spam on all wikis
	$dbr = wfGetDB( DB_SLAVE );
	print "Finding spam on " . count($wgLocalDatabases) . " wikis\n";
	$found = false;
	foreach ( $wgLocalDatabases as $db ) {
		$count = $dbr->selectField( "`$db`.externallinks", 'COUNT(*)', 
			array( 'el_index LIKE ' . $dbr->addQuotes( $like ) ), $fname );
		if ( $count ) {
			$found = true;
			passthru( "php cleanupSpam.php $db $spec | sed s/^/$db:  /" );
		}
	}
	if ( $found ) {
		print "All done\n";
	} else {
		print "None found\n";
	}
} else {
	// Clean up spam on this wiki
	$res = $dbr->select( 'externallinks', array( 'DISTINCT el_from' ), 
		array( 'el_index LIKE ' . $dbr->addQuotes( $like ) ), $fname );
	$count = $dbr->numRows( $res );
	print "Found $count articles containing $spec\n";
	while ( $row = $dbr->fetchObject( $res ) ) {
		cleanupArticle( $row->el_from, $spec );
	}
	if ( $count ) {
		print "Done\n";
	}
}


