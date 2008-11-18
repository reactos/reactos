<?php
/**
 * @file
 * @ingroup Maintenance
 */

/** */
function updateSearchIndex( $start, $end, $maxLockTime, $quiet ) {
	global $wgQuiet;
	global $wgDisableSearchUpdate;

	$fname = "updateSearchIndex";

	$wgQuiet = $quiet;
	$wgDisableSearchUpdate = false;

	$dbw = wfGetDB( DB_MASTER );
	$recentchanges = $dbw->tableName( 'recentchanges' );

	output( "Updating searchindex between $start and $end\n" );

	# Select entries from recentchanges which are on top and between the specified times
	$start = $dbw->strencode( $start );
	$end = $dbw->strencode( $end );

	$page = $dbw->tableName( 'page' );
	$sql = "SELECT rc_cur_id,rc_type,rc_moved_to_ns,rc_moved_to_title FROM $recentchanges
	  JOIN $page ON rc_cur_id=page_id AND rc_this_oldid=page_latest
	  WHERE rc_timestamp BETWEEN '$start' AND '$end'
	  ";
	$res = $dbw->query( $sql, $fname );
	

	# Lock searchindex
	if ( $maxLockTime ) {
		output( "   --- Waiting for lock ---" );
		lockSearchindex( $dbw );
		$lockTime = time();
		output( "\n" );
	}

	# Loop through the results and do a search update
	while ( $row = $dbw->fetchObject( $res ) ) {
		# Allow reads to be processed
		if ( $maxLockTime && time() > $lockTime + $maxLockTime ) {
			output( "    --- Relocking ---" );
			relockSearchindex( $dbw );
			$lockTime = time();
			output( "\n" );
		}
		if ( $row->rc_type == RC_LOG ) {
			continue;
		} elseif ( $row->rc_type == RC_MOVE || $row->rc_type == RC_MOVE_OVER_REDIRECT ) {
			# Rename searchindex entry
			$titleObj = Title::makeTitle( $row->rc_moved_to_ns, $row->rc_moved_to_title );
			$title = $titleObj->getPrefixedDBkey();
			output( "$title..." );
			$u = new SearchUpdate( $row->rc_cur_id, $title, false );
			output( "\n" );
		} else {
			// Get current revision
			$rev = Revision::loadFromPageId( $dbw, $row->rc_cur_id );
			if( $rev ) {
				$titleObj = $rev->getTitle();
				$title = $titleObj->getPrefixedDBkey();
				output( $title );
				# Update searchindex
				$u = new SearchUpdate( $row->rc_cur_id, $titleObj->getText(), $rev->getText() );
				$u->doUpdate();
				output( "\n" );
			}
		}
	}

	# Unlock searchindex
	if ( $maxLockTime ) {
		output( "    --- Unlocking --" );
		unlockSearchindex( $dbw );
		output( "\n" );
	}
	output( "Done\n" );
}

function lockSearchindex( &$db ) {
	$write = array( 'searchindex' );
	$read = array( 'page', 'revision', 'text', 'interwiki' );
	$items = array();
	
	foreach( $write as $table ) {
		$items[] = $db->tableName( $table ) . ' LOW_PRIORITY WRITE';
	}
	foreach( $read as $table ) {
		$items[] = $db->tableName( $table ) . ' READ';
	}
	$sql = "LOCK TABLES " . implode( ',', $items );
	$db->query( $sql, 'updateSearchIndex.inc ' . __METHOD__ );
}

function unlockSearchindex( &$db ) {
	$db->query( "UNLOCK TABLES", 'updateSearchIndex.inc ' . __METHOD__ );
}

# Unlock and lock again
# Since the lock is low-priority, queued reads will be able to complete
function relockSearchindex( &$db ) {
	unlockSearchindex( $db );
	lockSearchindex( $db );
}

function output( $text ) {
	global $wgQuiet;
	if ( !$wgQuiet ) {
		print $text;
	}
}
