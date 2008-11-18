<?php

require_once( 'FiveUpgrade.inc' );

/**
 * @ingroup Maintenance
 */
abstract class TableCleanup extends FiveUpgrade {
	function __construct( $table, $dryrun = false ) {
		parent::__construct();

		$this->targetTable = $table;
		$this->maxLag = 10; # if slaves are lagged more than 10 secs, wait
		$this->dryrun = $dryrun;
	}

	function cleanup() {
		if( $this->dryrun ) {
			echo "Checking for bad titles...\n";
		} else {
			echo "Checking and fixing bad titles...\n";
		}
		$this->runTable( $this->targetTable,
			'', //'WHERE page_namespace=0',
			array( $this, 'processPage' ) );
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
		$fname = 'CapsCleanup::buildTable';

		$count = $this->dbw->selectField( $table, 'count(*)', '', $fname );
		$this->init( $count, $table );
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

	function hexChar( $matches ) {
		return sprintf( "\\x%02x", ord( $matches[1] ) );
	}
	
	abstract function processPage( $row );
	
}
