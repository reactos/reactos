<?php
/*
 * Script to remove broken, unparseable titles in the Watchlist.
 *
 * Usage: php cleanupWatchlist.php [--fix]
 * Options:
 *   --fix  Actually remove entries; without will only report.
 *
 * Copyright (C) 2005,2006 Brion Vibber <brion@pobox.com>
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
 * @ingroup Maintenance
 */

$options = array( 'fix' );

require_once( 'commandLine.inc' );
require_once( 'FiveUpgrade.inc' );

/**
 * @ingroup Maintenance
 */
class WatchlistCleanup extends FiveUpgrade {
	function WatchlistCleanup( $dryrun = false ) {
		parent::FiveUpgrade();

		$this->maxLag = 10; # if slaves are lagged more than 10 secs, wait
		$this->dryrun = $dryrun;
	}

	function cleanup() {
		$this->runTable( 'watchlist',
			'',
			array( &$this, 'processEntry' ) );
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

		printf( "%s %s: %6.2f%% done on %s; ETA %s [%d/%d] %.2f/sec <%.2f%% updated>\n",
			wfWikiID(),
			wfTimestamp( TS_DB, intval( $now ) ),
			$portion * 100.0,
			$this->table,
			wfTimestamp( TS_DB, intval( $eta ) ),
			$this->processed,
			$this->count,
			$this->processed / $delta,
			$updateRate * 100.0 );
		flush();
	}

	function runTable( $table, $where, $callback ) {
		$fname = 'WatchlistCleanup::runTable';

		$count = $this->dbw->selectField( $table, 'count(*)', '', $fname );
		$this->init( $count, 'watchlist' );
		$this->log( "Processing $table..." );

		$tableName = $this->dbr->tableName( $table );
		$sql = "SELECT * FROM $tableName $where";
		$result = $this->dbr->query( $sql, $fname );

		while( $row = $this->dbr->fetchObject( $result ) ) {
			call_user_func( $callback, $row );
		}
		$this->log( "Finished $table... $this->updated of $this->processed rows updated" );
		$this->dbr->freeResult( $result );
	}

	function processEntry( $row ) {
		$current = Title::makeTitle( $row->wl_namespace, $row->wl_title );
		$display = $current->getPrefixedText();

		$verified = UtfNormal::cleanUp( $display );

		$title = Title::newFromText( $verified );

		if( $row->wl_user == 0 || is_null( $title ) || !$title->equals( $current ) ) {
			$this->log( "invalid watch by {$row->wl_user} for ({$row->wl_namespace}, \"{$row->wl_title}\")" );
			$this->removeWatch( $row );
			return $this->progress( 1 );
		}

		$this->progress( 0 );
	}
	
	function removeWatch( $row ) {
		if( !$this->dryrun) {
			$dbw = wfGetDB( DB_MASTER );
			$dbw->delete( 'watchlist', array(
				'wl_user'      => $row->wl_user,
				'wl_namespace' => $row->wl_namespace,
				'wl_title'     => $row->wl_title ),
			'WatchlistCleanup::removeWatch' );
			$this->log( '- removed' );
		}
	}
}

$wgUser->setName( 'Conversion script' );
$caps = new WatchlistCleanup( !isset( $options['fix'] ) );
$caps->cleanup();


