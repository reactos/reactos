<?php
/**
 * @file
 * @ingroup Maintenance
 */

$wgUseRootUser = true;
require_once( 'commandLine.inc' );

//$wgDebugLogFile = '/dev/stdout';

$slaveIndexes = array();
for ( $i = 1; $i < count( $wgDBservers ); $i++ ) {
	if ( wfGetLB()->isNonZeroLoad( $i ) ) {
		$slaveIndexes[] = $i;
	}
}
/*
foreach ( wfGetLB()->mServers as $i => $server ) {
	wfGetLB()->mServers[$i]['flags'] |= DBO_DEBUG;
}*/
$reportingInterval = 1000;

if ( isset( $args[0] ) ) {
	desyncFixPage( $args[0] );
} else {
	$dbw = wfGetDB( DB_MASTER );
	$maxPage = $dbw->selectField( 'page', 'MAX(page_id)', false, 'fixDesync.php' );
	$corrupt = findPageLatestCorruption();
	foreach ( $corrupt as $id => $dummy ) {
		desyncFixPage( $id );
	}
		/*
	for ( $i=1; $i <= $maxPage; $i++ ) {
		desyncFixPage( $i );
		if ( !($i % $reportingInterval) ) {
			print "$i\n";
		}
	}*/
}

function findPageLatestCorruption() {
	$desync = array();
	$n = 0;
	$dbw = wfGetDB( DB_MASTER );
	$masterIDs = array();
	$res = $dbw->select( 'page', array( 'page_id', 'page_latest' ), array( 'page_id<6054123' ), __METHOD__ );
	print "Number of pages: " . $dbw->numRows( $res ) . "\n";
	while ( $row = $dbw->fetchObject( $res ) ) {
		$masterIDs[$row->page_id] = $row->page_latest;
		if ( !( ++$n % 10000 ) ) {
			print "$n\r";
		}
	}
	print "\n";
	$dbw->freeResult( $res );
	
	global $slaveIndexes;
	foreach ( $slaveIndexes as $i ) {
		$db = wfGetDB( $i );
		$res = $db->select( 'page', array( 'page_id', 'page_latest' ), array( 'page_id<6054123' ), __METHOD__ );
		while ( $row = $db->fetchObject( $res ) ) {
			if ( isset( $masterIDs[$row->page_id] ) && $masterIDs[$row->page_id] != $row->page_latest ) {
				$desync[$row->page_id] = true;
				print $row->page_id . "\t";
			}
		}
		$db->freeResult( $res );
	}
	print "\n";
	return $desync;
}

function desyncFixPage( $pageID ) {
	global $slaveIndexes;
	$fname = 'desyncFixPage';

	# Check for a corrupted page_latest
	$dbw = wfGetDB( DB_MASTER );
	$dbw->begin();
	$realLatest = $dbw->selectField( 'page', 'page_latest', array( 'page_id' => $pageID ), 
		$fname, 'FOR UPDATE' );
	#list( $masterFile, $masterPos ) = $dbw->getMasterPos();
	$found = false;
	foreach ( $slaveIndexes as $i ) {
		$db = wfGetDB( $i );
		/*
		if ( !$db->masterPosWait( $masterFile, $masterPos, 10 ) ) {
		       echo "Slave is too lagged, aborting\n";
		       $dbw->commit();
		       sleep(10);
		       return;
		}*/	       
		$latest = $db->selectField( 'page', 'page_latest', array( 'page_id' => $pageID ), $fname );
		$max = $db->selectField( 'revision', 'MAX(rev_id)', false, $fname );
		if ( $latest != $realLatest && $realLatest < $max ) {
			print "page_latest corrupted in page $pageID, server $i\n";
			$found = true;
			break;
		}
	}
	if ( !$found ) {
		print "page_id $pageID seems fine\n";
		$dbw->commit();
		return;
	}

	# Find the missing revisions
	$res = $dbw->select( 'revision', array( 'rev_id' ), array( 'rev_page' => $pageID ), 
		$fname, 'FOR UPDATE' );
	$masterIDs = array();
	while ( $row = $dbw->fetchObject( $res ) ) {
		$masterIDs[] = $row->rev_id;
	}
	$dbw->freeResult( $res );

	$res = $db->select( 'revision', array( 'rev_id' ), array( 'rev_page' => $pageID ), $fname );
	$slaveIDs = array();
	while ( $row = $db->fetchObject( $res ) ) {
		$slaveIDs[] = $row->rev_id;
	}
	$db->freeResult( $res );
	if ( count( $masterIDs ) < count( $slaveIDs ) ) {
		$missingIDs = array_diff( $slaveIDs, $masterIDs );
		if ( count( $missingIDs ) ) {
			print "Found " . count( $missingIDs ) . " lost in master, copying from slave... ";
			$dbFrom = $db;
			$found = true;
			$toMaster = true;
		} else {
			$found = false;
		}
	} else {
		$missingIDs = array_diff( $masterIDs, $slaveIDs );
		if ( count( $missingIDs ) ) {
			print "Found " . count( $missingIDs ) . " missing revision(s), copying from master... ";
			$dbFrom = $dbw;
			$found = true;
			$toMaster = false;
		} else {
			$found = false;
		}
	}

	if ( $found ) {
		foreach ( $missingIDs as $rid ) {
			print "$rid ";
			# Revision
			$row = $dbFrom->selectRow( 'revision', '*', array( 'rev_id' => $rid ), $fname );
			if ( $toMaster ) {
				$id = $dbw->selectField( 'revision', 'rev_id', array( 'rev_id' => $rid ), 
					$fname,	'FOR UPDATE' );
				if ( $id ) {
					echo "Revision already exists\n";
					$found = false;
					break;
				} else {
					$dbw->insert( 'revision', get_object_vars( $row ), $fname, 'IGNORE' );
				}
			} else {
				foreach ( $slaveIndexes as $i ) {
					$db = wfGetDB( $i );
					$db->insert( 'revision', get_object_vars( $row ), $fname, 'IGNORE' );
				}
			}

			# Text
			$row = $dbFrom->selectRow( 'text', '*', array( 'old_id' => $row->rev_text_id ), $fname );
			if ( $toMaster ) {
				$dbw->insert( 'text', get_object_vars( $row ), $fname, 'IGNORE' );
			} else {
				foreach ( $slaveIndexes as $i ) {
					$db = wfGetDB( $i );
					$db->insert( 'text', get_object_vars( $row ), $fname, 'IGNORE' );
				}
			}
		}
		print "done\n";
	}

	if ( $found ) {
		print "Fixing page_latest... ";
		if ( $toMaster ) {
			#$dbw->update( 'page', array( 'page_latest' => $realLatest ), array( 'page_id' => $pageID ), $fname );
		} else {
			foreach ( $slaveIndexes as $i ) {
				$db = wfGetDB( $i );
				$db->update( 'page', array( 'page_latest' => $realLatest ), array( 'page_id' => $pageID ), $fname );
			}
		}
		print "done\n";
	}
	$dbw->commit();
}


