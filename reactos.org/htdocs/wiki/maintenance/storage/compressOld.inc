<?php
/**
 * @file
 * @ingroup Maintenance ExternalStorage
 */

/** */
require_once( 'Revision.php' );
require_once( 'ExternalStoreDB.php' );

/** @todo document */
function compressOldPages( $start = 0, $extdb = '' ) {
	$fname = 'compressOldPages';

	$chunksize = 50;
	print "Starting from old_id $start...\n";
	$dbw = wfGetDB( DB_MASTER );
	do {
		$res = $dbw->select( 'text', array( 'old_id','old_flags','old_text' ),
			"old_id>=$start", $fname, array( 'ORDER BY' => 'old_id', 'LIMIT' => $chunksize, 'FOR UPDATE' ) );
		if( $dbw->numRows( $res ) == 0 ) {
			break;
		}
		$last = $start;
		while( $row = $dbw->fetchObject( $res ) ) {
			# print "  {$row->old_id} - {$row->old_namespace}:{$row->old_title}\n";
			compressPage( $row, $extdb );
			$last = $row->old_id;
		}
		$dbw->freeResult( $res );
		$start = $last + 1; # Deletion may leave long empty stretches
		print "$start...\n";
	} while( true );
}

/** @todo document */
function compressPage( $row, $extdb ) {
	$fname = 'compressPage';
	if ( false !== strpos( $row->old_flags, 'gzip' ) || false !== strpos( $row->old_flags, 'object' ) ) {
		#print "Already compressed row {$row->old_id}\n";
		return false;
	}
	$dbw = wfGetDB( DB_MASTER );
	$flags = $row->old_flags ? "{$row->old_flags},gzip" : "gzip";
	$compress = gzdeflate( $row->old_text );

	# Store in external storage if required
	if ( $extdb !== '' ) {
		$storeObj = new ExternalStoreDB;
		$compress = $storeObj->store( $extdb, $compress );
		if ( $compress === false ) {
			print "Unable to store object\n";
			return false;
		}
	}

	# Update text row
	$dbw->update( 'text',
		array( /* SET */
			'old_flags' => $flags,
			'old_text' => $compress
		), array( /* WHERE */
			'old_id' => $row->old_id
		), $fname, 'LIMIT 1'
	);
	return true;
}

define( 'LS_INDIVIDUAL', 0 );
define( 'LS_CHUNKED', 1 );

/** @todo document */
function compressWithConcat( $startId, $maxChunkSize, $maxChunkFactor, $factorThreshold, $beginDate,
	$endDate, $extdb="", $maxPageId = false )
{
	$fname = 'compressWithConcat';
	$loadStyle = LS_CHUNKED;

	$dbr = wfGetDB( DB_SLAVE );
	$dbw = wfGetDB( DB_MASTER );

	# Set up external storage
	if ( $extdb != '' ) {
		$storeObj = new ExternalStoreDB;
	}

	# Get all articles by page_id
	if ( !$maxPageId ) {
		$maxPageId = $dbr->selectField( 'page', 'max(page_id)', '', $fname );
	}
	print "Starting from $startId of $maxPageId\n";
	$pageConds = array();

	/*
	if ( $exclude_ns0 ) {
		print "Excluding main namespace\n";
		$pageConds[] = 'page_namespace<>0';
	}
	if ( $queryExtra ) {
                $pageConds[] = $queryExtra;
	}
	 */

	# For each article, get a list of revisions which fit the criteria
	
	# No recompression, use a condition on old_flags
	# Don't compress object type entities, because that might produce data loss when
	# overwriting bulk storage concat rows. Don't compress external references, because
	# the script doesn't yet delete rows from external storage.
	$conds = array(
		"old_flags NOT LIKE '%object%' AND old_flags NOT LIKE '%external%'");

	if ( $beginDate ) {
		if ( !preg_match( '/^\d{14}$/', $beginDate ) ) {
			print "Invalid begin date \"$beginDate\"\n";
			return false;
		}
		$conds[] = "rev_timestamp>'" . $beginDate . "'";
	}
	if ( $endDate )  {
		if ( !preg_match( '/^\d{14}$/', $endDate ) ) {
			print "Invalid end date \"$endDate\"\n";
			return false;
		}
		$conds[] = "rev_timestamp<'" . $endDate . "'";
	}
	if ( $loadStyle == LS_CHUNKED ) {
		$tables = array( 'revision', 'text' );
		$fields = array( 'rev_id', 'rev_text_id', 'old_flags', 'old_text' );
		$conds[] = 'rev_text_id=old_id';
		$revLoadOptions = 'FOR UPDATE';
	} else {
		$tables = array( 'revision' );
		$fields = array( 'rev_id', 'rev_text_id' );
		$revLoadOptions = array();
	}

	# Don't work with current revisions
	# Don't lock the page table for update either -- TS 2006-04-04
	#$tables[] = 'page';
	#$conds[] = 'page_id=rev_page AND rev_id != page_latest';

	for ( $pageId = $startId; $pageId <= $maxPageId; $pageId++ ) {
		wfWaitForSlaves( 5 );

		# Wake up
		$dbr->ping();		

		# Get the page row
		$pageRes = $dbr->select( 'page', 
			array('page_id', 'page_namespace', 'page_title','page_latest'),
			$pageConds + array('page_id' => $pageId), $fname );
		if ( $dbr->numRows( $pageRes ) == 0 ) {
			continue;
		}
		$pageRow = $dbr->fetchObject( $pageRes );

		# Display progress
		$titleObj = Title::makeTitle( $pageRow->page_namespace, $pageRow->page_title );
		print "$pageId\t" . $titleObj->getPrefixedDBkey() . " ";

		# Load revisions
		$revRes = $dbw->select( $tables, $fields,
			array_merge( array( 
				'rev_page' => $pageRow->page_id, 
				# Don't operate on the current revision
				# Use < instead of <> in case the current revision has changed 
				# since the page select, which wasn't locking
				'rev_id < ' . $pageRow->page_latest
			), $conds ),
			$fname,
			$revLoadOptions
		);
		$revs = array();
		while ( $revRow = $dbw->fetchObject( $revRes ) ) {
			$revs[] = $revRow;
		}

		if ( count( $revs ) < 2) {
			# No revisions matching, no further processing
			print "\n";
			continue;
		}

		# For each chunk
		$i = 0;
		while ( $i < count( $revs ) ) {
			if ( $i < count( $revs ) - $maxChunkSize ) {
				$thisChunkSize = $maxChunkSize;
			} else {
				$thisChunkSize = count( $revs ) - $i;
			}

			$chunk = new ConcatenatedGzipHistoryBlob();
			$stubs = array();
			$dbw->begin();
			$usedChunk = false;
			$primaryOldid = $revs[$i]->rev_text_id;

			# Get the text of each revision and add it to the object
			for ( $j = 0; $j < $thisChunkSize && $chunk->isHappy( $maxChunkFactor, $factorThreshold ); $j++ ) {
				$oldid = $revs[$i + $j]->rev_text_id;

				# Get text
				if ( $loadStyle == LS_INDIVIDUAL ) {
					$textRow = $dbw->selectRow( 'text',
						array( 'old_flags', 'old_text' ),
						array( 'old_id' => $oldid ),
						$fname,
						'FOR UPDATE'
					);
					$text = Revision::getRevisionText( $textRow );
				} else {
					$text = Revision::getRevisionText( $revs[$i + $j] );
				}

				if ( $text === false ) {
					print "\nError, unable to get text in old_id $oldid\n";
					#$dbw->delete( 'old', array( 'old_id' => $oldid ) );
				}

				if ( $extdb == "" && $j == 0 ) {
					$chunk->setText( $text );
					print '.';
				} else {
					# Don't make a stub if it's going to be longer than the article
					# Stubs are typically about 100 bytes
					if ( strlen( $text ) < 120 ) {
						$stub = false;
						print 'x';
					} else {
						$stub = $chunk->addItem( $text );
						$stub->setLocation( $primaryOldid );
						$stub->setReferrer( $oldid );
						print '.';
						$usedChunk = true;
					}
					$stubs[$j] = $stub;
				}
			}
			$thisChunkSize = $j;

			# If we couldn't actually use any stubs because the pages were too small, do nothing
			if ( $usedChunk ) {
				if ( $extdb != "" ) {
					# Move blob objects to External Storage
					$stored = $storeObj->store( $extdb, serialize( $chunk ));
					if ($stored === false) {
						print "Unable to store object\n";
						return false;
					}
					# Store External Storage URLs instead of Stub placeholders
					foreach ($stubs as $stub) {
						if ($stub===false)
							continue;
						# $stored should provide base path to a BLOB
						$url = $stored."/".$stub->getHash();
						$dbw->update( 'text',
							array( /* SET */
								'old_text' => $url,
								'old_flags' => 'external,utf-8',
							), array ( /* WHERE */
								'old_id' => $stub->getReferrer(),
							)
						);
					}
				} else {
					# Store the main object locally
					$dbw->update( 'text',
						array( /* SET */
							'old_text' => serialize( $chunk ),
							'old_flags' => 'object,utf-8',
						), array( /* WHERE */
							'old_id' => $primaryOldid
						)
					);

					# Store the stub objects
					for ( $j = 1; $j < $thisChunkSize; $j++ ) {
						# Skip if not compressing and don't overwrite the first revision
						if ( $stubs[$j] !== false && $revs[$i + $j]->rev_text_id != $primaryOldid ) {
							$dbw->update( 'text',
								array( /* SET */
									'old_text' => serialize($stubs[$j]),
									'old_flags' => 'object,utf-8',
								), array( /* WHERE */
									'old_id' => $revs[$i + $j]->rev_text_id
								)
							);
						}
					}
				}
			}
			# Done, next
			print "/";
			$dbw->commit();
			$i += $thisChunkSize;
			wfWaitForSlaves( 5 );
		}
		print "\n";
	}
	return true;
}
