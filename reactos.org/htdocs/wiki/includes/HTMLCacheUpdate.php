<?php

/**
 * Class to invalidate the HTML cache of all the pages linking to a given title.
 * Small numbers of links will be done immediately, large numbers are pushed onto
 * the job queue.
 *
 * This class is designed to work efficiently with small numbers of links, and
 * to work reasonably well with up to ~10^5 links. Above ~10^6 links, the memory
 * and time requirements of loading all backlinked IDs in doUpdate() might become
 * prohibitive. The requirements measured at Wikimedia are approximately:
 *
 *   memory: 48 bytes per row
 *   time: 16us per row for the query plus processing
 *
 * The reason this query is done is to support partitioning of the job
 * by backlinked ID. The memory issue could be allieviated by doing this query in
 * batches, but of course LIMIT with an offset is inefficient on the DB side.
 *
 * The class is nevertheless a vast improvement on the previous method of using
 * Image::getLinksTo() and Title::touchArray(), which uses about 2KB of memory per
 * link.
 *
 * @ingroup Cache
 */
class HTMLCacheUpdate
{
	public $mTitle, $mTable, $mPrefix;
	public $mRowsPerJob, $mRowsPerQuery;

	function __construct( $titleTo, $table ) {
		global $wgUpdateRowsPerJob, $wgUpdateRowsPerQuery;

		$this->mTitle = $titleTo;
		$this->mTable = $table;
		$this->mRowsPerJob = $wgUpdateRowsPerJob;
		$this->mRowsPerQuery = $wgUpdateRowsPerQuery;
	}

	function doUpdate() {
		# Fetch the IDs
		$cond = $this->getToCondition();
		$dbr = wfGetDB( DB_SLAVE );
		$res = $dbr->select( $this->mTable, $this->getFromField(), $cond, __METHOD__ );

		if ( $dbr->numRows( $res ) != 0 ) {
			if ( $dbr->numRows( $res ) > $this->mRowsPerJob ) {
				$this->insertJobs( $res );
			} else {
				$this->invalidateIDs( $res );
			}
		}
	}

	function insertJobs( ResultWrapper $res ) {
		$numRows = $res->numRows();
		$numBatches = ceil( $numRows / $this->mRowsPerJob );
		$realBatchSize = $numRows / $numBatches;
		$start = false;
		$jobs = array();
		do {
			for ( $i = 0; $i < $realBatchSize - 1; $i++ ) {
				$row = $res->fetchRow();
				if ( $row ) {
					$id = $row[0];
				} else {
					$id = false;
					break;
				}
			}

			$params = array(
				'table' => $this->mTable,
				'start' => $start,
				'end' => ( $id !== false ? $id - 1 : false ),
			);
			$jobs[] = new HTMLCacheUpdateJob( $this->mTitle, $params );

			$start = $id;
		} while ( $start );

		Job::batchInsert( $jobs );
	}

	function getPrefix() {
		static $prefixes = array(
			'pagelinks' => 'pl',
			'imagelinks' => 'il',
			'categorylinks' => 'cl',
			'templatelinks' => 'tl',
			'redirect' => 'rd',

			# Not needed
			# 'externallinks' => 'el',
			# 'langlinks' => 'll'
		);

		if ( is_null( $this->mPrefix ) ) {
			$this->mPrefix = $prefixes[$this->mTable];
			if ( is_null( $this->mPrefix ) ) {
				throw new MWException( "Invalid table type \"{$this->mTable}\" in " . __CLASS__ );
			}
		}
		return $this->mPrefix;
	}

	function getFromField() {
		return $this->getPrefix() . '_from';
	}

	function getToCondition() {
		$prefix = $this->getPrefix();
		switch ( $this->mTable ) {
			case 'pagelinks':
			case 'templatelinks':
			case 'redirect':
				return array(
					"{$prefix}_namespace" => $this->mTitle->getNamespace(),
					"{$prefix}_title" => $this->mTitle->getDBkey()
				);
			case 'imagelinks':
				return array( 'il_to' => $this->mTitle->getDBkey() );
			case 'categorylinks':
				return array( 'cl_to' => $this->mTitle->getDBkey() );
		}
		throw new MWException( 'Invalid table type in ' . __CLASS__ );
	}

	/**
	 * Invalidate a set of IDs, right now
	 */
	function invalidateIDs( ResultWrapper $res ) {
		global $wgUseFileCache, $wgUseSquid;

		if ( $res->numRows() == 0 ) {
			return;
		}

		$dbw = wfGetDB( DB_MASTER );
		$timestamp = $dbw->timestamp();
		$done = false;

		while ( !$done ) {
			# Get all IDs in this query into an array
			$ids = array();
			for ( $i = 0; $i < $this->mRowsPerQuery; $i++ ) {
				$row = $res->fetchRow();
				if ( $row ) {
					$ids[] = $row[0];
				} else {
					$done = true;
					break;
				}
			}

			if ( !count( $ids ) ) {
				break;
			}

			# Update page_touched
			$dbw->update( 'page',
				array( 'page_touched' => $timestamp ),
				array( 'page_id IN (' . $dbw->makeList( $ids ) . ')' ),
				__METHOD__
			);

			# Update squid
			if ( $wgUseSquid || $wgUseFileCache ) {
				$titles = Title::newFromIDs( $ids );
				if ( $wgUseSquid ) {
					$u = SquidUpdate::newFromTitles( $titles );
					$u->doUpdate();
				}

				# Update file cache
				if  ( $wgUseFileCache ) {
					foreach ( $titles as $title ) {
						$cm = new HTMLFileCache($title);
						@unlink($cm->fileCacheName());
					}
				}
			}
		}
	}
}

/**
 * @todo document (e.g. one-sentence top-level class description).
 * @ingroup JobQueue
 */
class HTMLCacheUpdateJob extends Job {
	var $table, $start, $end;

	/**
	 * Construct a job
	 * @param Title $title The title linked to
	 * @param array $params Job parameters (table, start and end page_ids)
	 * @param integer $id job_id
	 */
	function __construct( $title, $params, $id = 0 ) {
		parent::__construct( 'htmlCacheUpdate', $title, $params, $id );
		$this->table = $params['table'];
		$this->start = $params['start'];
		$this->end = $params['end'];
	}

	function run() {
		$update = new HTMLCacheUpdate( $this->title, $this->table );

		$fromField = $update->getFromField();
		$conds = $update->getToCondition();
		if ( $this->start ) {
			$conds[] = "$fromField >= {$this->start}";
		}
		if ( $this->end ) {
			$conds[] = "$fromField <= {$this->end}";
		}

		$dbr = wfGetDB( DB_SLAVE );
		$res = $dbr->select( $this->table, $fromField, $conds, __METHOD__ );
		$update->invalidateIDs( $res );

		return true;
	}
}
