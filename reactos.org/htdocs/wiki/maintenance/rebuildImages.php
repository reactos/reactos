<?php
/*
 * Script to update image metadata records
 *
 * Usage: php rebuildImages.php [--missing] [--dry-run]
 * Options:
 *   --missing  Crawl the uploads dir for images without records, and
 *              add them only.
 *
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
 * @author Brion Vibber <brion at pobox.com>
 * @ingrouo maintenance
 */

$options = array( 'missing', 'dry-run' );

require_once( 'commandLine.inc' );
require_once( 'FiveUpgrade.inc' );

class ImageBuilder extends FiveUpgrade {
	function ImageBuilder( $dryrun = false ) {
		parent::FiveUpgrade();

		$this->maxLag = 10; # if slaves are lagged more than 10 secs, wait
		$this->dryrun = $dryrun;
		if ( $dryrun ) {
			$GLOBALS['wgReadOnly'] = 'Dry run mode, image upgrades are suppressed';
		}
	}

	function getRepo() {
		if ( !isset( $this->repo ) ) {
			$this->repo = RepoGroup::singleton()->getLocalRepo();
		}
		return $this->repo;
	}

	function build() {
		$this->buildImage();
		$this->buildOldImage();
	}

	function init( $count, $table ) {
		$this->processed = 0;
		$this->updated = 0;
		$this->count = $count;
		$this->startTime = wfTime();
		$this->table = $table;
	}

	function progress( $updated ) {
		$this->updated += $updated;
		$this->processed++;
		if( $this->processed % 100 != 0 ) {
			return;
		}
		$portion = $this->processed / $this->count;
		$updateRate = $this->updated / $this->processed;

		$now = wfTime();
		$delta = $now - $this->startTime;
		$estimatedTotalTime = $delta / $portion;
		$eta = $this->startTime + $estimatedTotalTime;

		printf( "%s: %6.2f%% done on %s; ETA %s [%d/%d] %.2f/sec <%.2f%% updated>\n",
			wfTimestamp( TS_DB, intval( $now ) ),
			$portion * 100.0,
			$this->table,
			wfTimestamp( TS_DB, intval( $eta ) ),
			$completed,   // $completed does not appear to be defined.
			$this->count,
			$rate,        // $rate does not appear to be defined.
			$updateRate * 100.0 );
		flush();
	}

	function buildTable( $table, $key, $callback ) {
		$fname = 'ImageBuilder::buildTable';

		$count = $this->dbw->selectField( $table, 'count(*)', '', $fname );
		$this->init( $count, $table );
		$this->log( "Processing $table..." );

		$tableName = $this->dbr->tableName( $table );
		$sql = "SELECT * FROM $tableName";
		$result = $this->dbr->query( $sql, $fname );

		while( $row = $this->dbr->fetchObject( $result ) ) {
			$update = call_user_func( $callback, $row );
			if( $update ) {
				$this->progress( 1 );
			} else {
				$this->progress( 0 );
			}
		}
		$this->log( "Finished $table... $this->updated of $this->processed rows updated" );
		$this->dbr->freeResult( $result );
	}

	function buildImage() {
		$callback = array( &$this, 'imageCallback' );
		$this->buildTable( 'image', 'img_name', $callback );
	}

	function imageCallback( $row ) {
		// Create a File object from the row
		// This will also upgrade it
		$file = $this->getRepo()->newFileFromRow( $row );
		return $file->getUpgraded();
	}

	function buildOldImage() {
		$this->buildTable( 'oldimage', 'oi_archive_name',
			array( &$this, 'oldimageCallback' ) );
	}

	function oldimageCallback( $row ) {
		// Create a File object from the row
		// This will also upgrade it
		if ( $row->oi_archive_name == '' ) {
			$this->log( "Empty oi_archive_name for oi_name={$row->oi_name}" );
			return false;
		}
		$file = $this->getRepo()->newFileFromRow( $row );
		return $file->getUpgraded();
	}

	function crawlMissing() {
		$repo = RepoGroup::singleton()->getLocalRepo();
		$repo->enumFilesInFS( array( $this, 'checkMissingImage' ) );
	}

	function checkMissingImage( $fullpath ) {
		$fname = 'ImageBuilder::checkMissingImage';
		$filename = wfBaseName( $fullpath );
		if( is_dir( $fullpath ) ) {
			return;
		}
		if( is_link( $fullpath ) ) {
			$this->log( "skipping symlink at $fullpath" );
			return;
		}
		$row = $this->dbw->selectRow( 'image',
			array( 'img_name' ),
			array( 'img_name' => $filename ),
			$fname );

		if( $row ) {
			// already known, move on
			return;
		} else {
			$this->addMissingImage( $filename, $fullpath );
		}
	}

	function addMissingImage( $filename, $fullpath ) {
		$fname = 'ImageBuilder::addMissingImage';

		$timestamp = $this->dbw->timestamp( filemtime( $fullpath ) );

		global $wgContLang;
		$altname = $wgContLang->checkTitleEncoding( $filename );
		if( $altname != $filename ) {
			if( $this->dryrun ) {
				$filename = $altname;
				$this->log( "Estimating transcoding... $altname" );
			} else {
				$filename = $this->renameFile( $filename );
			}
		}

		if( $filename == '' ) {
			$this->log( "Empty filename for $fullpath" );
			return;
		}
		if ( !$this->dryrun ) {
			$file = wfLocalFile( $filename );
			if ( !$file->recordUpload( '', '(recovered file, missing upload log entry)', '', '', '', 
				false, $timestamp ) )
			{
				$this->log( "Error uploading file $fullpath" );
				return;
			}
		}
		$this->log( $fullpath );
	}
}

$builder = new ImageBuilder( isset( $options['dry-run'] ) );
if( isset( $options['missing'] ) ) {
	$builder->crawlMissing();
} else {
	$builder->build();
}


