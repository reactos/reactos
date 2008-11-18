<?php
/**
 * Fsck for MediaWiki
 *
 * @file
 * @ingroup Maintenance ExternalStorage
 */

define( 'CONCAT_HEADER', 'O:27:"concatenatedgziphistoryblob"' );

if ( !defined( 'MEDIAWIKI' ) ) {
	require_once( dirname(__FILE__) . '/../commandLine.inc' );

	$cs = new CheckStorage;
	$fix = isset( $options['fix'] );
	if ( isset( $args[0] ) ) {
		$xml = $args[0];
	} else {
		$xml = false;
	}
	$cs->check( $fix, $xml );
}


//----------------------------------------------------------------------------------

/**
 * @ingroup Maintenance ExternalStorage
 */
class CheckStorage {
	var $oldIdMap, $errors;
	var $dbStore = null;

	var $errorDescriptions = array(
		'restore text' => 'Damaged text, need to be restored from a backup',
		'restore revision' => 'Damaged revision row, need to be restored from a backup',
		'unfixable' => 'Unexpected errors with no automated fixing method',
		'fixed' => 'Errors already fixed',
		'fixable' => 'Errors which would already be fixed if --fix was specified',
	);		

	function check( $fix = false, $xml = '' ) {
		$fname = 'checkStorage';
		$dbr = wfGetDB( DB_SLAVE );
		if ( $fix ) {
			$dbw = wfGetDB( DB_MASTER );
			print "Checking, will fix errors if possible...\n";
		} else {
			print "Checking...\n";
		}
		$maxRevId = $dbr->selectField( 'revision', 'MAX(rev_id)', false, $fname );
		$chunkSize = 1000;
		$flagStats = array();
		$objectStats = array();
		$knownFlags = array( 'external', 'gzip', 'object', 'utf-8' );
		$this->errors = array(
			'restore text' => array(),
			'restore revision' => array(),
			'unfixable' => array(),
			'fixed' => array(),
			'fixable' => array(),
		);

		for ( $chunkStart = 1 ; $chunkStart < $maxRevId; $chunkStart += $chunkSize ) {
			$chunkEnd = $chunkStart + $chunkSize - 1;
			//print "$chunkStart of $maxRevId\n";

			// Fetch revision rows
			$this->oldIdMap = array();
			$dbr->ping();		
			$res = $dbr->select( 'revision', array( 'rev_id', 'rev_text_id' ), 
				array( "rev_id BETWEEN $chunkStart AND $chunkEnd" ), $fname );
			while ( $row = $dbr->fetchObject( $res ) ) {
				$this->oldIdMap[$row->rev_id] = $row->rev_text_id;
			}
			$dbr->freeResult( $res );

			if ( !count( $this->oldIdMap ) ) {
				continue;
			}

			// Fetch old_flags
			$missingTextRows = array_flip( $this->oldIdMap );
			$externalRevs = array();
			$objectRevs = array();
			$res = $dbr->select( 'text', array( 'old_id', 'old_flags' ), 
				'old_id IN (' . implode( ',', $this->oldIdMap ) . ')', $fname );
			while ( $row = $dbr->fetchObject( $res ) ) {
				$flags = $row->old_flags;
				$id = $row->old_id;

				// Create flagStats row if it doesn't exist
				$flagStats = $flagStats + array( $flags => 0 );
				// Increment counter
				$flagStats[$flags]++;

				// Not missing
				unset( $missingTextRows[$row->old_id] );

				// Check for external or object
				if ( $flags == '' ) {
					$flagArray = array();
				} else {
					$flagArray = explode( ',', $flags );
				}
				if ( in_array( 'external', $flagArray ) ) {
					$externalRevs[] = $id;
				} elseif ( in_array( 'object', $flagArray ) ) {
					$objectRevs[] = $id;
				}

				// Check for unrecognised flags
				if ( $flags == '0' ) {
					// This is a known bug from 2004
					// It's safe to just erase the old_flags field
					if ( $fix ) {
						$this->error( 'fixed', "Warning: old_flags set to 0", $id );
						$dbw->ping();
						$dbw->update( 'text', array( 'old_flags' => '' ), 
							array( 'old_id' => $id ), $fname );
						echo "Fixed\n";
					} else {
						$this->error( 'fixable', "Warning: old_flags set to 0", $id );
					}
				} elseif ( count( array_diff( $flagArray, $knownFlags ) ) ) {
					$this->error( 'unfixable', "Error: invalid flags field \"$flags\"", $id );
				}
			}
			$dbr->freeResult( $res );

			// Output errors for any missing text rows
			foreach ( $missingTextRows as $oldId => $revId ) {
				$this->error( 'restore revision', "Error: missing text row", $oldId );
			}

			// Verify external revisions
			$externalConcatBlobs = array();
			$externalNormalBlobs = array();
			if ( count( $externalRevs ) ) {
				$res = $dbr->select( 'text', array( 'old_id', 'old_flags', 'old_text' ), 
					array( 'old_id IN (' . implode( ',', $externalRevs ) . ')' ), $fname );
				while ( $row = $dbr->fetchObject( $res ) ) {
					$urlParts = explode( '://', $row->old_text, 2 );
					if ( count( $urlParts ) !== 2 || $urlParts[1] == '' ) {
						$this->error( 'restore text', "Error: invalid URL \"{$row->old_text}\"", $row->old_id );
						continue;
					}
					list( $proto, $path ) = $urlParts;
					if ( $proto != 'DB' ) {
						$this->error( 'restore text', "Error: invalid external protocol \"$proto\"", $row->old_id );
						continue;
					}
					$path = explode( '/', $row->old_text );
					$cluster = $path[2];
					$id = $path[3];
					if ( isset( $path[4] ) ) {
						$externalConcatBlobs[$cluster][$id][] = $row->old_id;
					} else {
						$externalNormalBlobs[$cluster][$id][] = $row->old_id;
					}
				}
				$dbr->freeResult( $res );
			}

			// Check external concat blobs for the right header
			$this->checkExternalConcatBlobs( $externalConcatBlobs );
			
			// Check external normal blobs for existence
			if ( count( $externalNormalBlobs ) ) {
				if ( is_null( $this->dbStore ) ) {
					$this->dbStore = new ExternalStoreDB;
				}
				foreach ( $externalConcatBlobs as $cluster => $xBlobIds ) {
					$blobIds = array_keys( $xBlobIds );
					$extDb =& $this->dbStore->getSlave( $cluster );
					$blobsTable = $this->dbStore->getTable( $extDb );
					$res = $extDb->select( $blobsTable, 
						array( 'blob_id' ), 
						array( 'blob_id IN( ' . implode( ',', $blobIds ) . ')' ), $fname );
					while ( $row = $extDb->fetchObject( $res ) ) {
						unset( $xBlobIds[$row->blob_id] );
					}
					$extDb->freeResult( $res );
					// Print errors for missing blobs rows
					foreach ( $xBlobIds as $blobId => $oldId ) {
						$this->error( 'restore text', "Error: missing target $blobId for one-part ES URL", $oldId );
					}
				}
			}

			// Check local objects
			$dbr->ping();
			$concatBlobs = array();
			$curIds = array();
			if ( count( $objectRevs ) ) {
				$headerLength = 300;
				$res = $dbr->select( 'text', array( 'old_id', 'old_flags', "LEFT(old_text, $headerLength) AS header" ), 
					array( 'old_id IN (' . implode( ',', $objectRevs ) . ')' ), $fname );
				while ( $row = $dbr->fetchObject( $res ) ) {
					$oldId = $row->old_id;
					$matches = array();
					if ( !preg_match( '/^O:(\d+):"(\w+)"/', $row->header, $matches ) ) {
						$this->error( 'restore text', "Error: invalid object header", $oldId );
						continue;
					}

					$className = strtolower( $matches[2] );
					if ( strlen( $className ) != $matches[1] ) {
						$this->error( 'restore text', "Error: invalid object header, wrong class name length", $oldId );
						continue;
					}

					$objectStats = $objectStats + array( $className => 0 );
					$objectStats[$className]++;

					switch ( $className ) {
						case 'concatenatedgziphistoryblob':
							// Good
							break;
						case 'historyblobstub':
						case 'historyblobcurstub':
							if ( strlen( $row->header ) == $headerLength ) {
								$this->error( 'unfixable', "Error: overlong stub header", $oldId );
								continue;
							}
							$stubObj = unserialize( $row->header );
							if ( !is_object( $stubObj ) ) {
								$this->error( 'restore text', "Error: unable to unserialize stub object", $oldId );
								continue;
							}
							if ( $className == 'historyblobstub' ) {
								$concatBlobs[$stubObj->mOldId][] = $oldId;
							} else {
								$curIds[$stubObj->mCurId][] = $oldId;
							}
							break;
						default:
							$this->error( 'unfixable', "Error: unrecognised object class \"$className\"", $oldId );
					}
				}
				$dbr->freeResult( $res );
			}

			// Check local concat blob validity
			$externalConcatBlobs = array();
			if ( count( $concatBlobs ) ) {
				$headerLength = 300;
				$res = $dbr->select( 'text', array( 'old_id', 'old_flags', "LEFT(old_text, $headerLength) AS header" ), 
					array( 'old_id IN (' . implode( ',', array_keys( $concatBlobs ) ) . ')' ), $fname );
				while ( $row = $dbr->fetchObject( $res ) ) {
					$flags = explode( ',', $row->old_flags );
					if ( in_array( 'external', $flags ) ) {
						// Concat blob is in external storage?
						if ( in_array( 'object', $flags ) ) {
							$urlParts = explode( '/', $row->header );
							if ( $urlParts[0] != 'DB:' ) {
								$this->error( 'unfixable', "Error: unrecognised external storage type \"{$urlParts[0]}", $row->old_id );
							} else {
								$cluster = $urlParts[2];
								$id = $urlParts[3];
								if ( !isset( $externalConcatBlobs[$cluster][$id] ) ) {
									$externalConcatBlobs[$cluster][$id] = array();
								}
								$externalConcatBlobs[$cluster][$id] = array_merge( 
									$externalConcatBlobs[$cluster][$id], $concatBlobs[$row->old_id]
								);
							}
						} else {
							$this->error( 'unfixable', "Error: invalid flags \"{$row->old_flags}\" on concat bulk row {$row->old_id}",
								$concatBlobs[$row->old_id] );
						}
					} elseif ( strcasecmp( substr( $row->header, 0, strlen( CONCAT_HEADER ) ), CONCAT_HEADER ) ) {
						$this->error( 'restore text', "Error: Incorrect object header for concat bulk row {$row->old_id}", 
							$concatBlobs[$row->old_id] );
					} # else good

					unset( $concatBlobs[$row->old_id] );
				}
				$dbr->freeResult( $res );
			}

			// Check targets of unresolved stubs
			$this->checkExternalConcatBlobs( $externalConcatBlobs );

			// next chunk
		}

		print "\n\nErrors:\n";
		foreach( $this->errors as $name => $errors ) {
			if ( count( $errors ) ) {
				$description = $this->errorDescriptions[$name];
				echo "$description: " . implode( ',', array_keys( $errors ) ) . "\n";
			}
		}

		if ( count( $this->errors['restore text'] ) && $fix ) {
			if ( (string)$xml !== '' ) {
				$this->restoreText( array_keys( $this->errors['restore text'] ), $xml );
			} else {
				echo "Can't fix text, no XML backup specified\n";
			}
		}

		print "\nFlag statistics:\n";
		$total = array_sum( $flagStats );
		foreach ( $flagStats as $flag => $count ) {
			printf( "%-30s %10d %5.2f%%\n", $flag, $count, $count / $total * 100 );
		}
		print "\nLocal object statistics:\n";
		$total = array_sum( $objectStats );
		foreach ( $objectStats as $className => $count ) {
			printf( "%-30s %10d %5.2f%%\n", $className, $count, $count / $total * 100 );
		}
	}


	function error( $type, $msg, $ids ) {
		if ( is_array( $ids ) && count( $ids ) == 1 ) {
			$ids = reset( $ids );
		}
		if ( is_array( $ids ) ) {
			$revIds = array();
			foreach ( $ids as $id ) {
				$revIds = array_merge( $revIds, array_keys( $this->oldIdMap, $id ) );
			}
			print "$msg in text rows " . implode( ', ', $ids ) . 
				", revisions " . implode( ', ', $revIds ) . "\n";
		} else {
			$id = $ids;
			$revIds = array_keys( $this->oldIdMap, $id );
			if ( count( $revIds ) == 1 ) {
				print "$msg in old_id $id, rev_id {$revIds[0]}\n";
			} else {
				print "$msg in old_id $id, revisions " . implode( ', ', $revIds ) . "\n";
			}
		}
		$this->errors[$type] = $this->errors[$type] + array_flip( $revIds );
	}

	function checkExternalConcatBlobs( $externalConcatBlobs ) {
		$fname = 'CheckStorage::checkExternalConcatBlobs';
		if ( !count( $externalConcatBlobs ) ) {
			return;
		}

		if ( is_null( $this->dbStore ) ) {
			$this->dbStore = new ExternalStoreDB;
		}
		
		foreach ( $externalConcatBlobs as $cluster => $oldIds ) {
			$blobIds = array_keys( $oldIds );
			$extDb =& $this->dbStore->getSlave( $cluster );
			$blobsTable = $this->dbStore->getTable( $extDb );
			$headerLength = strlen( CONCAT_HEADER );
			$res = $extDb->select( $blobsTable, 
				array( 'blob_id', "LEFT(blob_text, $headerLength) AS header" ), 
				array( 'blob_id IN( ' . implode( ',', $blobIds ) . ')' ), $fname );
			while ( $row = $extDb->fetchObject( $res ) ) {
				if ( strcasecmp( $row->header, CONCAT_HEADER ) ) {
					$this->error( 'restore text', "Error: invalid header on target $cluster/{$row->blob_id} of two-part ES URL", 
						$oldIds[$row->blob_id] );
				}
				unset( $oldIds[$row->blob_id] );

			}
			$extDb->freeResult( $res );

			// Print errors for missing blobs rows
			foreach ( $oldIds as $blobId => $oldIds ) {
				$this->error( 'restore text', "Error: missing target $cluster/$blobId for two-part ES URL", $oldIds );
			}
		}
	}

	function restoreText( $revIds, $xml ) {
		global $wgTmpDirectory, $wgDBname;

		if ( !count( $revIds ) ) {
			return;
		}

		print "Restoring text from XML backup...\n";

		$revFileName = "$wgTmpDirectory/broken-revlist-$wgDBname";
		$filteredXmlFileName = "$wgTmpDirectory/filtered-$wgDBname.xml";
		
		// Write revision list
		if ( !file_put_contents( $revFileName, implode( "\n", $revIds ) ) ) {
			echo "Error writing revision list, can't restore text\n";
			return;
		}

		// Run mwdumper
		echo "Filtering XML dump...\n";
		$exitStatus = 0;
		passthru( 'mwdumper ' . 
			wfEscapeShellArg( 
				"--output=file:$filteredXmlFileName",
				"--filter=revlist:$revFileName",
				$xml
			), $exitStatus
		);

		if ( $exitStatus ) {
			echo "mwdumper died with exit status $exitStatus\n";
			return;
		}

		$file = fopen( $filteredXmlFileName, 'r' );
		if ( !$file ) {
			echo "Unable to open filtered XML file\n";
			return;
		}

		$dbr = wfGetDB( DB_SLAVE );
		$dbw = wfGetDB( DB_MASTER );
		$dbr->ping();
		$dbw->ping();
		
		$source = new ImportStreamSource( $file );
		$importer = new WikiImporter( $source );
		$importer->setRevisionCallback( array( &$this, 'importRevision' ) );
		$importer->doImport();
	}

	function importRevision( &$revision, &$importer ) {
		$fname = 'CheckStorage::importRevision';

		$id = $revision->getID();
		$text = $revision->getText();
		if ( $text === '' ) {
			// This is what happens if the revision was broken at the time the 
			// dump was made. Unfortunately, it also happens if the revision was 
			// legitimately blank, so there's no way to tell the difference. To
			// be safe, we'll skip it and leave it broken
			$id = $id ? $id : '';
			echo "Revision $id is blank in the dump, may have been broken before export\n";
			return;
		}

		if ( !$id )  {
			// No ID, can't import
			echo "No id tag in revision, can't import\n";
			return;
		}

		// Find text row again
		$dbr = wfGetDB( DB_SLAVE );
		$oldId = $dbr->selectField( 'revision', 'rev_text_id', array( 'rev_id' => $id ), $fname );
		if ( !$oldId ) {
			echo "Missing revision row for rev_id $id\n";
			return;
		}

		// Compress the text
		$flags = Revision::compressRevisionText( $text );

		// Update the text row
		$dbw = wfGetDB( DB_MASTER );
		$dbw->update( 'text', 
			array( 'old_flags' => $flags, 'old_text' => $text ),
			array( 'old_id' => $oldId ),
			$fname, array( 'LIMIT' => 1 )
		);

		// Remove it from the unfixed list and add it to the fixed list
		unset( $this->errors['restore text'][$id] );
		$this->errors['fixed'][$id] = true;
	}
}

