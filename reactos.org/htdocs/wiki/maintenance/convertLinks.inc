<?php
/**
 * @file
 * @todo document
 * @ingroup Maintenance
 */

/** */
function convertLinks() {
	global $wgDBtype;
	if( $wgDBtype == 'postgres' ) {
		print "Links table already ok on Postgres.\n";
		return;
	}

	print "Converting links table to ID-ID...\n";

	global $wgLang, $wgDBserver, $wgDBadminuser, $wgDBadminpassword, $wgDBname;
	global $noKeys, $logPerformance, $fh;

	$tuplesAdded = $numBadLinks = $curRowsRead = 0; #counters etc
	$totalTuplesInserted = 0; # total tuples INSERTed into links_temp

	$reportCurReadProgress = true; #whether or not to give progress reports while reading IDs from cur table
	$curReadReportInterval = 1000; #number of rows between progress reports

	$reportLinksConvProgress = true; #whether or not to give progress reports during conversion
	$linksConvInsertInterval = 1000; #number of rows per INSERT

	$initialRowOffset = 0;
	#$finalRowOffset = 0; # not used yet; highest row number from links table to process

	# Overwrite the old links table with the new one.  If this is set to false,
	# the new table will be left at links_temp.
	$overwriteLinksTable = true;

	# Don't create keys, and so allow duplicates in the new links table.
	# This gives a huge speed improvement for very large links tables which are MyISAM. (What about InnoDB?)
	$noKeys = false;


	$logPerformance = false; # output performance data to a file
	$perfLogFilename = "convLinksPerf.txt";
	#--------------------------------------------------------------------

	$dbw = wfGetDB( DB_MASTER );
	list ($cur, $links, $links_temp, $links_backup) = $dbw->tableNamesN( 'cur', 'links', 'links_temp', 'links_backup' );

	$res = $dbw->query( "SELECT l_from FROM $links LIMIT 1" );
	if ( $dbw->fieldType( $res, 0 ) == "int" ) {
		print "Schema already converted\n";
		return;
	}

	$res = $dbw->query( "SELECT COUNT(*) AS count FROM $links" );
	$row = $dbw->fetchObject($res);
	$numRows = $row->count;
	$dbw->freeResult( $res );

	if ( $numRows == 0 ) {
		print "Updating schema (no rows to convert)...\n";
		createTempTable();
	} else {
		if ( $logPerformance ) { $fh = fopen ( $perfLogFilename, "w" ); }
		$baseTime = $startTime = getMicroTime();
		# Create a title -> cur_id map
		print "Loading IDs from $cur table...\n";
		performanceLog ( "Reading $numRows rows from cur table...\n" );
		performanceLog ( "rows read vs seconds elapsed:\n" );

		$dbw->bufferResults( false );
		$res = $dbw->query( "SELECT cur_namespace,cur_title,cur_id FROM $cur" );
		$ids = array();

		while ( $row = $dbw->fetchObject( $res ) ) {
			$title = $row->cur_title;
			if ( $row->cur_namespace ) {
				$title = $wgLang->getNsText( $row->cur_namespace ) . ":$title";
			}
			$ids[$title] = $row->cur_id;
			$curRowsRead++;
			if ($reportCurReadProgress) {
				if (($curRowsRead % $curReadReportInterval) == 0) {
					performanceLog( $curRowsRead . " " . (getMicroTime() - $baseTime) . "\n" );
					print "\t$curRowsRead rows of $cur table read.\n";
				}
			}
		}
		$dbw->freeResult( $res );
		$dbw->bufferResults( true );
		print "Finished loading IDs.\n\n";
		performanceLog( "Took " . (getMicroTime() - $baseTime) . " seconds to load IDs.\n\n" );
	#--------------------------------------------------------------------

		# Now, step through the links table (in chunks of $linksConvInsertInterval rows),
		# convert, and write to the new table.
		createTempTable();
		performanceLog( "Resetting timer.\n\n" );
		$baseTime = getMicroTime();
		print "Processing $numRows rows from $links table...\n";
		performanceLog( "Processing $numRows rows from $links table...\n" );
		performanceLog( "rows inserted vs seconds elapsed:\n" );

		for ($rowOffset = $initialRowOffset; $rowOffset < $numRows; $rowOffset += $linksConvInsertInterval) {
			$sqlRead = "SELECT * FROM $links ";
			$sqlRead = $dbw->limitResult($sqlRead, $linksConvInsertInterval,$rowOffset);
			$res = $dbw->query($sqlRead);
			if ( $noKeys ) {
				$sqlWrite = array("INSERT INTO $links_temp (l_from,l_to) VALUES ");
			} else {
				$sqlWrite = array("INSERT IGNORE INTO $links_temp (l_from,l_to) VALUES ");
			}

			$tuplesAdded = 0; # no tuples added to INSERT yet
			while ( $row = $dbw->fetchObject($res) ) {
				$fromTitle = $row->l_from;
				if ( array_key_exists( $fromTitle, $ids ) ) { # valid title
					$from = $ids[$fromTitle];
					$to = $row->l_to;
					if ( $tuplesAdded != 0 ) {
						$sqlWrite[] = ",";
					}
					$sqlWrite[] = "($from,$to)";
					$tuplesAdded++;
				} else { # invalid title
					$numBadLinks++;
				}
			}
			$dbw->freeResult($res);
			#print "rowOffset: $rowOffset\ttuplesAdded: $tuplesAdded\tnumBadLinks: $numBadLinks\n";
			if ( $tuplesAdded != 0  ) {
				if ($reportLinksConvProgress) {
					print "Inserting $tuplesAdded tuples into $links_temp...";
				}
				$dbw->query( implode("",$sqlWrite) );
				$totalTuplesInserted += $tuplesAdded;
				if ($reportLinksConvProgress)
					print " done. Total $totalTuplesInserted tuples inserted.\n";
					performanceLog( $totalTuplesInserted . " " . (getMicroTime() - $baseTime) . "\n"  );
			}
		}
		print "$totalTuplesInserted valid titles and $numBadLinks invalid titles were processed.\n\n";
		performanceLog( "$totalTuplesInserted valid titles and $numBadLinks invalid titles were processed.\n" );
		performanceLog( "Total execution time: " . (getMicroTime() - $startTime) . " seconds.\n" );
		if ( $logPerformance ) { fclose ( $fh ); }
	}
	#--------------------------------------------------------------------

	if ( $overwriteLinksTable ) {
		$dbConn = Database::newFromParams( $wgDBserver, $wgDBadminuser, $wgDBadminpassword, $wgDBname );
		if (!($dbConn->isOpen())) {
			print "Opening connection to database failed.\n";
			return;
		}
		# Check for existing links_backup, and delete it if it exists.
		print "Dropping backup links table if it exists...";
		$dbConn->query( "DROP TABLE IF EXISTS $links_backup", DB_MASTER);
		print " done.\n";

		# Swap in the new table, and move old links table to links_backup
		print "Swapping tables '$links' to '$links_backup'; '$links_temp' to '$links'...";
		$dbConn->query( "RENAME TABLE links TO $links_backup, $links_temp TO $links", DB_MASTER );
		print " done.\n\n";

		$dbConn->close();
		print "Conversion complete. The old table remains at $links_backup;\n";
		print "delete at your leisure.\n";
	} else {
		print "Conversion complete.  The converted table is at $links_temp;\n";
		print "the original links table is unchanged.\n";
	}
}

#--------------------------------------------------------------------

function createTempTable() {
	global $wgDBserver, $wgDBadminuser, $wgDBadminpassword, $wgDBname;
	global $noKeys;
	$dbConn = Database::newFromParams( $wgDBserver, $wgDBadminuser, $wgDBadminpassword, $wgDBname );

	if (!($dbConn->isOpen())) {
		print "Opening connection to database failed.\n";
		return;
	}
	$links_temp = $dbConn->tableName( 'links_temp' );

	print "Dropping temporary links table if it exists...";
	$dbConn->query( "DROP TABLE IF EXISTS $links_temp");
	print " done.\n";

	print "Creating temporary links table...";
	if ( $noKeys ) {
		$dbConn->query( "CREATE TABLE $links_temp ( " .
		"l_from int(8) unsigned NOT NULL default '0', " .
		"l_to int(8) unsigned NOT NULL default '0')");
	} else {
		$dbConn->query( "CREATE TABLE $links_temp ( " .
		"l_from int(8) unsigned NOT NULL default '0', " .
		"l_to int(8) unsigned NOT NULL default '0', " .
		"UNIQUE KEY l_from(l_from,l_to), " .
		"KEY (l_to))");
	}
	print " done.\n\n";
}

function performanceLog( $text ) {
	global $logPerformance, $fh;
	if ( $logPerformance ) {
		fwrite( $fh, $text );
	}
}

function getMicroTime() { # return time in seconds, with microsecond accuracy
	list($usec, $sec) = explode(" ", microtime());
	return ((float)$usec + (float)$sec);
}
