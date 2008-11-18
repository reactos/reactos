<?php
/**
 * @defgroup JobQueue JobQueue
 */

if ( !defined( 'MEDIAWIKI' ) ) {
	die( "This file is part of MediaWiki, it is not a valid entry point\n" );
}

/**
 * Class to both describe a background job and handle jobs.
 *
 * @ingroup JobQueue
 */
abstract class Job {
	var $command,
		$title,
		$params,
		$id,
		$removeDuplicates,
		$error;

	/*-------------------------------------------------------------------------
	 * Abstract functions
	 *------------------------------------------------------------------------*/

	/**
	 * Run the job
	 * @return boolean success
	 */
	abstract function run();

	/*-------------------------------------------------------------------------
	 * Static functions
	 *------------------------------------------------------------------------*/

	/**
	 * @deprecated use LinksUpdate::queueRecursiveJobs()
	 */
	/**
	 * static function queueLinksJobs( $titles ) {}
	 */

	/**
	 * Pop a job of a certain type.  This tries less hard than pop() to
	 * actually find a job; it may be adversely affected by concurrent job
	 * runners.
	 */
	static function pop_type($type) {
		wfProfilein( __METHOD__ );

		$dbw = wfGetDB( DB_MASTER );


		$row = $dbw->selectRow( 'job', '*', array( 'job_cmd' => $type ), __METHOD__,
				array( 'LIMIT' => 1 ));

		if ($row === false) {
			wfProfileOut( __METHOD__ );
			return false;
		}

		/* Ensure we "own" this row */
		$dbw->delete( 'job', array( 'job_id' => $row->job_id ), __METHOD__ );
		$affected = $dbw->affectedRows();

		if ($affected == 0) {
			wfProfileOut( __METHOD__ );
			return false;
		}

		$namespace = $row->job_namespace;
		$dbkey = $row->job_title;
		$title = Title::makeTitleSafe( $namespace, $dbkey );
		$job = Job::factory( $row->job_cmd, $title, Job::extractBlob( $row->job_params ), $row->job_id );

		$dbw->delete( 'job', $job->insertFields(), __METHOD__ );
		$dbw->immediateCommit();

		wfProfileOut( __METHOD__ );
		return $job;
	}

	/**
	 * Pop a job off the front of the queue
	 *
	 * @param $offset Number of jobs to skip
	 * @return Job or false if there's no jobs
	 */
	static function pop($offset=0) {
		wfProfileIn( __METHOD__ );

		$dbr = wfGetDB( DB_SLAVE );

		/* Get a job from the slave, start with an offset,
			scan full set afterwards, avoid hitting purged rows

			NB: If random fetch previously was used, offset
				will always be ahead of few entries
		*/

		$row = $dbr->selectRow( 'job', '*', "job_id >= ${offset}", __METHOD__,
			array( 'ORDER BY' => 'job_id', 'LIMIT' => 1 ));

		// Refetching without offset is needed as some of job IDs could have had delayed commits
		// and have lower IDs than jobs already executed, blame concurrency :)
		//
		if ( $row === false) {
			if ($offset!=0)
				$row = $dbr->selectRow( 'job', '*', '', __METHOD__,
					array( 'ORDER BY' => 'job_id', 'LIMIT' => 1 ));

			if ($row === false ) {
				wfProfileOut( __METHOD__ );
				return false;
			}
		}
		$offset = $row->job_id;

		// Try to delete it from the master
		$dbw = wfGetDB( DB_MASTER );
		$dbw->delete( 'job', array( 'job_id' => $row->job_id ), __METHOD__ );
		$affected = $dbw->affectedRows();
		$dbw->immediateCommit();

		if ( !$affected ) {
			// Failed, someone else beat us to it
			// Try getting a random row
			$row = $dbw->selectRow( 'job', array( 'MIN(job_id) as minjob',
				'MAX(job_id) as maxjob' ), "job_id >= $offset", __METHOD__ );
			if ( $row === false || is_null( $row->minjob ) || is_null( $row->maxjob ) ) {
				// No jobs to get
				wfProfileOut( __METHOD__ );
				return false;
			}
			// Get the random row
			$row = $dbw->selectRow( 'job', '*',
				'job_id >= ' . mt_rand( $row->minjob, $row->maxjob ),	__METHOD__ );
			if ( $row === false ) {
				// Random job gone before we got the chance to select it
				// Give up
				wfProfileOut( __METHOD__ );
				return false;
			}
			// Delete the random row
			$dbw->delete( 'job', array( 'job_id' => $row->job_id ), __METHOD__ );
			$affected = $dbw->affectedRows();
			$dbw->immediateCommit();

			if ( !$affected ) {
				// Random job gone before we exclusively deleted it
				// Give up
				wfProfileOut( __METHOD__ );
				return false;
			}
		}

		// If execution got to here, there's a row in $row that has been deleted from the database
		// by this thread. Hence the concurrent pop was successful.
		$namespace = $row->job_namespace;
		$dbkey = $row->job_title;
		$title = Title::makeTitleSafe( $namespace, $dbkey );
		$job = Job::factory( $row->job_cmd, $title, Job::extractBlob( $row->job_params ), $row->job_id );

		// Remove any duplicates it may have later in the queue
		// Deadlock prone section
		$dbw->begin();
		$dbw->delete( 'job', $job->insertFields(), __METHOD__ );
		$dbw->commit();

		wfProfileOut( __METHOD__ );
		return $job;
	}

	/**
	 * Create the appropriate object to handle a specific job
	 *
	 * @param $command String: Job command
	 * @param $title Title: Associated title
	 * @param $params Array: Job parameters
	 * @param $id Int: Job identifier
	 * @return Job
	 */
	static function factory( $command, $title, $params = false, $id = 0 ) {
		global $wgJobClasses;
		if( isset( $wgJobClasses[$command] ) ) {
			$class = $wgJobClasses[$command];
			return new $class( $title, $params, $id );
		}
		throw new MWException( "Invalid job command `{$command}`" );
	}

	static function makeBlob( $params ) {
		if ( $params !== false ) {
			return serialize( $params );
		} else {
			return '';
		}
	}

	static function extractBlob( $blob ) {
		if ( (string)$blob !== '' ) {
			return unserialize( $blob );
		} else {
			return false;
		}
	}

	/**
	 * Batch-insert a group of jobs into the queue.
	 * This will be wrapped in a transaction with a forced commit.
	 *
	 * This may add duplicate at insert time, but they will be
	 * removed later on, when the first one is popped.
	 *
	 * @param $jobs array of Job objects
	 */
	static function batchInsert( $jobs ) {
		if( !count( $jobs ) ) {
			return;
		}
		$dbw = wfGetDB( DB_MASTER );
		$rows = array();
		foreach( $jobs as $job ) {
			$rows[] = $job->insertFields();
			if ( count( $rows ) >= 50 ) {
				# Do a small transaction to avoid slave lag
				$dbw->begin();
				$dbw->insert( 'job', $rows, __METHOD__, 'IGNORE' );
				$dbw->commit();
				$rows = array();
			}
		}
		if ( $rows ) {
			$dbw->begin();
			$dbw->insert( 'job', $rows, __METHOD__, 'IGNORE' );
			$dbw->commit();
		}
	}

	/*-------------------------------------------------------------------------
	 * Non-static functions
	 *------------------------------------------------------------------------*/

	function __construct( $command, $title, $params = false, $id = 0 ) {
		$this->command = $command;
		$this->title = $title;
		$this->params = $params;
		$this->id = $id;

		// A bit of premature generalisation
		// Oh well, the whole class is premature generalisation really
		$this->removeDuplicates = true;
	}

	/**
	 * Insert a single job into the queue.
	 */
	function insert() {
		$fields = $this->insertFields();

		$dbw = wfGetDB( DB_MASTER );

		if ( $this->removeDuplicates ) {
			$res = $dbw->select( 'job', array( '1' ), $fields, __METHOD__ );
			if ( $dbw->numRows( $res ) ) {
				return;
			}
		}
		$fields['job_id'] = $dbw->nextSequenceValue( 'job_job_id_seq' );
		$dbw->insert( 'job', $fields, __METHOD__ );
	}

	protected function insertFields() {
		return array(
			'job_cmd' => $this->command,
			'job_namespace' => $this->title->getNamespace(),
			'job_title' => $this->title->getDBkey(),
			'job_params' => Job::makeBlob( $this->params )
		);
	}

	function toString() {
		$paramString = '';
		if ( $this->params ) {
			foreach ( $this->params as $key => $value ) {
				if ( $paramString != '' ) {
					$paramString .= ' ';
				}
				$paramString .= "$key=$value";
			}
		}

		if ( is_object( $this->title ) ) {
			$s = "{$this->command} " . $this->title->getPrefixedDBkey();
			if ( $paramString !== '' ) {
				$s .= ' ' . $paramString;
			}
			return $s;
		} else {
			return "{$this->command} $paramString";
		}
	}

	protected function setLastError( $error ) {
		$this->error = $error;
	}

	function getLastError() {
		return $this->error;
	}
}
