<?php
/**
 * Copyright (C) 2005 Brion Vibber <brion@pobox.com>
 * http://www.mediawiki.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @file
 * @ingroup Maintenance
 */

$optionsWithArgs = array( 'report' );

require_once( 'commandLine.inc' );

/**
 * @ingroup Maintenance
 */
class BackupReader {
	var $reportingInterval = 100;
	var $reporting = true;
	var $pageCount = 0;
	var $revCount  = 0;
	var $dryRun    = false;
	var $debug     = false;
	var $uploads   = false;

	function BackupReader() {
		$this->stderr = fopen( "php://stderr", "wt" );
	}

	function reportPage( $page ) {
		$this->pageCount++;
	}

	function handleRevision( $rev ) {
		$title = $rev->getTitle();
		if (!$title) {
			$this->progress( "Got bogus revision with null title!" );
			return;
		}
		#$timestamp = $rev->getTimestamp();
		#$display = $title->getPrefixedText();
		#echo "$display $timestamp\n";

		$this->revCount++;
		$this->report();

		if( !$this->dryRun ) {
			call_user_func( $this->importCallback, $rev );
		}
	}
	
	function handleUpload( $revision ) {
		if( $this->uploads ) {
			$this->uploadCount++;
			//$this->report();
			$this->progress( "upload: " . $revision->getFilename() );
			
			if( !$this->dryRun ) {
				// bluuuh hack
				//call_user_func( $this->uploadCallback, $revision );
				$dbw = wfGetDB( DB_MASTER );
				return $dbw->deadlockLoop( array( $revision, 'importUpload' ) );
			}
		}
	}

	function report( $final = false ) {
		if( $final xor ( $this->pageCount % $this->reportingInterval == 0 ) ) {
			$this->showReport();
		}
	}

	function showReport() {
		if( $this->reporting ) {
			$delta = wfTime() - $this->startTime;
			if( $delta ) {
				$rate = sprintf("%.2f", $this->pageCount / $delta);
				$revrate = sprintf("%.2f", $this->revCount / $delta);
			} else {
				$rate = '-';
				$revrate = '-';
			}
			$this->progress( "$this->pageCount ($rate pages/sec $revrate revs/sec)" );
		}
		wfWaitForSlaves(5);
	}

	function progress( $string ) {
		fwrite( $this->stderr, $string . "\n" );
	}

	function importFromFile( $filename ) {
		if( preg_match( '/\.gz$/', $filename ) ) {
			$filename = 'compress.zlib://' . $filename;
		}
		$file = fopen( $filename, 'rt' );
		return $this->importFromHandle( $file );
	}

	function importFromStdin() {
		$file = fopen( 'php://stdin', 'rt' );
		return $this->importFromHandle( $file );
	}

	function importFromHandle( $handle ) {
		$this->startTime = wfTime();

		$source = new ImportStreamSource( $handle );
		$importer = new WikiImporter( $source );

		$importer->setDebug( $this->debug );
		$importer->setPageCallback( array( &$this, 'reportPage' ) );
		$this->importCallback =  $importer->setRevisionCallback(
			array( &$this, 'handleRevision' ) );
		$this->uploadCallback = $importer->setUploadCallback(
			array( &$this, 'handleUpload' ) );

		return $importer->doImport();
	}
}

if( wfReadOnly() ) {
	wfDie( "Wiki is in read-only mode; you'll need to disable it for import to work.\n" );
}

$reader = new BackupReader();
if( isset( $options['quiet'] ) ) {
	$reader->reporting = false;
}
if( isset( $options['report'] ) ) {
	$reader->reportingInterval = intval( $options['report'] );
}
if( isset( $options['dry-run'] ) ) {
	$reader->dryRun = true;
}
if( isset( $options['debug'] ) ) {
	$reader->debug = true;
}
if( isset( $options['uploads'] ) ) {
	$reader->uploads = true; // experimental!
}

if( isset( $args[0] ) ) {
	$result = $reader->importFromFile( $args[0] );
} else {
	$result = $reader->importFromStdin();
}

if( WikiError::isError( $result ) ) {
	echo $result->getMessage() . "\n";
} else {
	echo "Done!\n";
	echo "You might want to run rebuildrecentchanges.php to regenerate\n";
	echo "the recentchanges page.\n";
}


