<?php
/**
 * Move revision's text to external storage
 *
 * @file
 * @ingroup Maintenance ExternalStorage
 */

define( 'REPORTING_INTERVAL', 1 );

if ( !defined( 'MEDIAWIKI' ) ) {
	$optionsWithArgs = array( 'e', 's' );

	require_once( dirname(__FILE__) . '/../commandLine.inc' );
	require_once( 'ExternalStoreDB.php' );
	require_once( 'resolveStubs.php' );

	$fname = 'moveToExternal';

	if ( !isset( $args[0] ) ) {
		print "Usage: php moveToExternal.php [-s <startid>] [-e <endid>] <cluster>\n";
		exit;
	}

	$cluster = $args[0];
	$dbw = wfGetDB( DB_MASTER );

	if ( isset( $options['e'] ) ) {
		$maxID = $options['e'];
	} else {
		$maxID = $dbw->selectField( 'text', 'MAX(old_id)', false, $fname );
	}
	$minID = isset( $options['s'] ) ? $options['s'] : 1;

	moveToExternal( $cluster, $maxID, $minID );
}



function moveToExternal( $cluster, $maxID, $minID = 1 ) {
	$fname = 'moveToExternal';
	$dbw = wfGetDB( DB_MASTER );
	$dbr = wfGetDB( DB_SLAVE );

	$count = $maxID - $minID + 1;
	$blockSize = 1000;
	$numBlocks = ceil( $count / $blockSize );
	print "Moving text rows from $minID to $maxID to external storage\n";
	$ext = new ExternalStoreDB;
	$numMoved = 0;
	$numStubs = 0;
	
	for ( $block = 0; $block < $numBlocks; $block++ ) {
		$blockStart = $block * $blockSize + $minID;
		$blockEnd = $blockStart + $blockSize - 1;
		
		if ( !($block % REPORTING_INTERVAL) ) {
			print "oldid=$blockStart, moved=$numMoved\n";
			wfWaitForSlaves( 2 );
		}
		
		$res = $dbr->select( 'text', array( 'old_id', 'old_flags', 'old_text' ),
			array(
				"old_id BETWEEN $blockStart AND $blockEnd",
				"old_flags NOT LIKE '%external%'",
			), $fname );
		while ( $row = $dbr->fetchObject( $res ) ) {
			# Resolve stubs
			$text = $row->old_text;
			$id = $row->old_id;
			if ( $row->old_flags === '' ) {
				$flags = 'external';
			} else {
				$flags = "{$row->old_flags},external";
			}
			
			if ( strpos( $flags, 'object' ) !== false ) {
				$obj = unserialize( $text );
				$className = strtolower( get_class( $obj ) );
				if ( $className == 'historyblobstub' ) {
					#resolveStub( $id, $row->old_text, $row->old_flags );
					#$numStubs++;
					continue;
				} elseif ( $className == 'historyblobcurstub' ) {
					$text = gzdeflate( $obj->getText() );
					$flags = 'utf-8,gzip,external';
				} elseif ( $className == 'concatenatedgziphistoryblob' ) {
					// Do nothing
				} else {
					print "Warning: unrecognised object class \"$className\"\n";
					continue;
				}
			} else {
				$className = false;
			}

			if ( strlen( $text ) < 100 && $className === false ) {
				// Don't move tiny revisions
				continue;
			}

			#print "Storing "  . strlen( $text ) . " bytes to $url\n";
			#print "old_id=$id\n";

			$url = $ext->store( $cluster, $text );
			if ( !$url ) {
				print "Error writing to external storage\n";
				exit;
			}
			$dbw->update( 'text',
				array( 'old_flags' => $flags, 'old_text' => $url ),
				array( 'old_id' => $id ), $fname );
			$numMoved++;
		}
		$dbr->freeResult( $res );
	}
}


