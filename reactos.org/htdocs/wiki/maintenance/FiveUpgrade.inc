<?php
/**
 * @file
 * @ingroup Maintenance
 */

require_once( 'cleanupDupes.inc' );
require_once( 'userDupes.inc' );
require_once( 'updaters.inc' );

define( 'MW_UPGRADE_COPY',     false );
define( 'MW_UPGRADE_ENCODE',   true  );
define( 'MW_UPGRADE_NULL',     null  );
define( 'MW_UPGRADE_CALLBACK', null  ); // for self-documentation only

/**
 * @ingroup Maintenance
 */
class FiveUpgrade {
	function FiveUpgrade() {
		$this->conversionTables = $this->prepareWindows1252();

		$this->dbw =& $this->newConnection();
		$this->dbr =& $this->streamConnection();

		$this->cleanupSwaps = array();
		$this->emailAuth = false; # don't preauthenticate emails
		$this->maxLag    = 10; # if slaves are lagged more than 10 secs, wait
	}

	function doing( $step ) {
		return is_null( $this->step ) || $step == $this->step;
	}

	function upgrade( $step ) {
		$this->step = $step;

		$tables = array(
			'page',
			'links',
			'user',
			'image',
			'oldimage',
			'watchlist',
			'logging',
			'archive',
			'imagelinks',
			'categorylinks',
			'ipblocks',
			'recentchanges',
			'querycache' );
		foreach( $tables as $table ) {
			if( $this->doing( $table ) ) {
				$method = 'upgrade' . ucfirst( $table );
				$this->$method();
			}
		}

		if( $this->doing( 'cleanup' ) ) {
			$this->upgradeCleanup();
		}
	}


	/**
	 * Open a connection to the master server with the admin rights.
	 * @return Database
	 * @access private
	 */
	function &newConnection() {
		global $wgDBadminuser, $wgDBadminpassword, $wgDBtype;
		global $wgDBserver, $wgDBname;
		$dbclass = 'Database' . ucfirst( $wgDBtype ) ;
		$db = new $dbclass( $wgDBserver, $wgDBadminuser, $wgDBadminpassword, $wgDBname );
		return $db;
	}

	/**
	 * Open a second connection to the master server, with buffering off.
	 * This will let us stream large datasets in and write in chunks on the
	 * other end.
	 * @return Database
	 * @access private
	 */
	function &streamConnection() {
		global $wgDBtype;

		$timeout = 3600 * 24;
		$db =& $this->newConnection();
		$db->bufferResults( false );
        if ($wgDBtype == 'mysql') {
			$db->query( "SET net_read_timeout=$timeout" );
			$db->query( "SET net_write_timeout=$timeout" );
		}
		return $db;
	}

	/**
	 * Prepare a conversion array for converting Windows Code Page 1252 to
	 * UTF-8. This should provide proper conversion of text that was miscoded
	 * as Windows-1252 by naughty user-agents, and doesn't rely on an outside
	 * iconv library.
	 *
	 * @return array
	 * @access private
	 */
	function prepareWindows1252() {
		# Mappings from:
		# http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1252.TXT
		static $cp1252 = array(
			0x80 => 0x20AC,	#EURO SIGN
			0x81 => 0xFFFD, #REPLACEMENT CHARACTER (no mapping)
			0x82 => 0x201A,	#SINGLE LOW-9 QUOTATION MARK
			0x83 => 0x0192,	#LATIN SMALL LETTER F WITH HOOK
			0x84 => 0x201E,	#DOUBLE LOW-9 QUOTATION MARK
			0x85 => 0x2026,	#HORIZONTAL ELLIPSIS
			0x86 => 0x2020,	#DAGGER
			0x87 => 0x2021,	#DOUBLE DAGGER
			0x88 => 0x02C6,	#MODIFIER LETTER CIRCUMFLEX ACCENT
			0x89 => 0x2030,	#PER MILLE SIGN
			0x8A => 0x0160,	#LATIN CAPITAL LETTER S WITH CARON
			0x8B => 0x2039,	#SINGLE LEFT-POINTING ANGLE QUOTATION MARK
			0x8C => 0x0152,	#LATIN CAPITAL LIGATURE OE
			0x8D => 0xFFFD, #REPLACEMENT CHARACTER (no mapping)
			0x8E => 0x017D,	#LATIN CAPITAL LETTER Z WITH CARON
			0x8F => 0xFFFD, #REPLACEMENT CHARACTER (no mapping)
			0x90 => 0xFFFD, #REPLACEMENT CHARACTER (no mapping)
			0x91 => 0x2018,	#LEFT SINGLE QUOTATION MARK
			0x92 => 0x2019,	#RIGHT SINGLE QUOTATION MARK
			0x93 => 0x201C,	#LEFT DOUBLE QUOTATION MARK
			0x94 => 0x201D,	#RIGHT DOUBLE QUOTATION MARK
			0x95 => 0x2022,	#BULLET
			0x96 => 0x2013,	#EN DASH
			0x97 => 0x2014,	#EM DASH
			0x98 => 0x02DC,	#SMALL TILDE
			0x99 => 0x2122,	#TRADE MARK SIGN
			0x9A => 0x0161,	#LATIN SMALL LETTER S WITH CARON
			0x9B => 0x203A,	#SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
			0x9C => 0x0153,	#LATIN SMALL LIGATURE OE
			0x9D => 0xFFFD, #REPLACEMENT CHARACTER (no mapping)
			0x9E => 0x017E,	#LATIN SMALL LETTER Z WITH CARON
			0x9F => 0x0178,	#LATIN CAPITAL LETTER Y WITH DIAERESIS
			);
		$pairs = array();
		for( $i = 0; $i < 0x100; $i++ ) {
			$unicode = isset( $cp1252[$i] ) ? $cp1252[$i] : $i;
			$pairs[chr( $i )] = codepointToUtf8( $unicode );
		}
		return $pairs;
	}

	/**
	 * Convert from 8-bit Windows-1252 to UTF-8 if necessary.
	 * @param string $text
	 * @return string
	 * @access private
	 */
	function conv( $text ) {
		global $wgUseLatin1;
		return is_null( $text )
			? null
			: ( $wgUseLatin1
				? strtr( $text, $this->conversionTables )
				: $text );
	}

	/**
	 * Dump timestamp and message to output
	 * @param string $message
	 * @access private
	 */
	function log( $message ) {
		echo wfWikiID() . ' ' . wfTimestamp( TS_DB ) . ': ' . $message . "\n";
		flush();
	}

	/**
	 * Initialize the chunked-insert system.
	 * Rows will be inserted in chunks of the given number, rather
	 * than in a giant INSERT...SELECT query, to keep the serialized
	 * MySQL database replication from getting hung up. This way other
	 * things can be going on during conversion without waiting for
	 * slaves to catch up as badly.
	 *
	 * @param int $chunksize Number of rows to insert at once
	 * @param int $final Total expected number of rows / id of last row,
	 *                   used for progress reports.
	 * @param string $table to insert on
	 * @param string $fname function name to report in SQL
	 * @access private
	 */
	function setChunkScale( $chunksize, $final, $table, $fname ) {
		$this->chunkSize  = $chunksize;
		$this->chunkFinal = $final;
		$this->chunkCount = 0;
		$this->chunkStartTime = wfTime();
		$this->chunkOptions = array( 'IGNORE' );
		$this->chunkTable = $table;
		$this->chunkFunction = $fname;
	}

	/**
	 * Chunked inserts: perform an insert if we've reached the chunk limit.
	 * Prints a progress report with estimated completion time.
	 * @param array &$chunk -- This will be emptied if an insert is done.
	 * @param int $key A key identifier to use in progress estimation in
	 *                 place of the number of rows inserted. Use this if
	 *                 you provided a max key number instead of a count
	 *                 as the final chunk number in setChunkScale()
	 * @access private
	 */
	function addChunk( &$chunk, $key = null ) {
		if( count( $chunk ) >= $this->chunkSize ) {
			$this->insertChunk( $chunk );

			$this->chunkCount += count( $chunk );
			$now = wfTime();
			$delta = $now - $this->chunkStartTime;
			$rate = $this->chunkCount / $delta;

			if( is_null( $key ) ) {
				$completed = $this->chunkCount;
			} else {
				$completed = $key;
			}
			$portion = $completed / $this->chunkFinal;

			$estimatedTotalTime = $delta / $portion;
			$eta = $this->chunkStartTime + $estimatedTotalTime;

			printf( "%s: %6.2f%% done on %s; ETA %s [%d/%d] %.2f/sec\n",
				wfTimestamp( TS_DB, intval( $now ) ),
				$portion * 100.0,
				$this->chunkTable,
				wfTimestamp( TS_DB, intval( $eta ) ),
				$completed,
				$this->chunkFinal,
				$rate );
			flush();

			$chunk = array();
		}
	}

	/**
	 * Chunked inserts: perform an insert unconditionally, at the end, and log.
	 * @param array &$chunk -- This will be emptied if an insert is done.
	 * @access private
	 */
	function lastChunk( &$chunk ) {
		$n = count( $chunk );
		if( $n > 0 ) {
			$this->insertChunk( $chunk );
		}
		$this->log( "100.00% done on $this->chunkTable (last chunk $n rows)." );
	}

	/**
	 * Chunked inserts: perform an insert.
	 * @param array &$chunk -- This will be emptied if an insert is done.
	 * @access private
	 */
	function insertChunk( &$chunk ) {
		// Give slaves a chance to catch up
		wfWaitForSlaves( $this->maxLag );
		$this->dbw->insert( $this->chunkTable, $chunk, $this->chunkFunction, $this->chunkOptions );
	}


	/**
	 * Copy and transcode a table to table_temp.
	 * @param string $name Base name of the source table
	 * @param string $tabledef CREATE TABLE definition, w/ $1 for the name
	 * @param array $fields set of destination fields to these constants:
	 *              MW_UPGRADE_COPY   - straight copy
	 *              MW_UPGRADE_ENCODE - for old Latin1 wikis, conv to UTF-8
	 *              MW_UPGRADE_NULL   - just put NULL
	 * @param callable $callback An optional callback to modify the data
	 *                           or perform other processing. Func should be
	 *                           ( object $row, array $copy ) and return $copy
	 * @access private
	 */
	function copyTable( $name, $tabledef, $fields, $callback = null ) {
		$fname = 'FiveUpgrade::copyTable';

		$name_temp = $name . '_temp';
		$this->log( "Migrating $name table to $name_temp..." );

		$table_temp = $this->dbw->tableName( $name_temp );

		// Create temporary table; we're going to copy everything in there,
		// then at the end rename the final tables into place.
		$def = str_replace( '$1', $table_temp, $tabledef );
		$this->dbw->query( $def, $fname );

		$numRecords = $this->dbw->selectField( $name, 'COUNT(*)', '', $fname );
		$this->setChunkScale( 100, $numRecords, $name_temp, $fname );

		// Pull all records from the second, streaming database connection.
		$sourceFields = array_keys( array_filter( $fields,
			create_function( '$x', 'return $x !== MW_UPGRADE_NULL;' ) ) );
		$result = $this->dbr->select( $name,
			$sourceFields,
			'',
			$fname );

		$add = array();
		while( $row = $this->dbr->fetchObject( $result ) ) {
			$copy = array();
			foreach( $fields as $field => $source ) {
				if( $source === MW_UPGRADE_COPY ) {
					$copy[$field] = $row->$field;
				} elseif( $source === MW_UPGRADE_ENCODE ) {
					$copy[$field] = $this->conv( $row->$field );
				} elseif( $source === MW_UPGRADE_NULL ) {
					$copy[$field] = null;
				} else {
					$this->log( "Unknown field copy type: $field => $source" );
				}
			}
			if( is_callable( $callback ) ) {
				$copy = call_user_func( $callback, $row, $copy );
			}
			$add[] = $copy;
			$this->addChunk( $add );
		}
		$this->lastChunk( $add );
		$this->dbr->freeResult( $result );

		$this->log( "Done converting $name." );
		$this->cleanupSwaps[] = $name;
	}

	function upgradePage() {
		$fname = "FiveUpgrade::upgradePage";
		$chunksize = 100;

		if( $this->dbw->tableExists( 'page' ) ) {
			$this->log( 'Page table already exists; aborting.' );
			die( -1 );
		}

		$this->log( "Checking cur table for unique title index and applying if necessary" );
		checkDupes( true );

		$this->log( "...converting from cur/old to page/revision/text DB structure." );

		list ($cur, $old, $page, $revision, $text) = $this->dbw->tableNamesN( 'cur', 'old', 'page', 'revision', 'text' );

		$this->log( "Creating page and revision tables..." );
		$this->dbw->query("CREATE TABLE $page (
  			page_id int(8) unsigned NOT NULL auto_increment,
  			page_namespace int NOT NULL,
  			page_title varchar(255) binary NOT NULL,
  			page_restrictions tinyblob NOT NULL default '',
  			page_counter bigint(20) unsigned NOT NULL default '0',
  			page_is_redirect tinyint(1) unsigned NOT NULL default '0',
  			page_is_new tinyint(1) unsigned NOT NULL default '0',
  			page_random real unsigned NOT NULL,
  			page_touched char(14) binary NOT NULL default '',
  			page_latest int(8) unsigned NOT NULL,
  			page_len int(8) unsigned NOT NULL,

  			PRIMARY KEY page_id (page_id),
  			UNIQUE INDEX name_title (page_namespace,page_title),
  			INDEX (page_random),
  			INDEX (page_len)
			) TYPE=InnoDB", $fname );
		$this->dbw->query("CREATE TABLE $revision (
  			rev_id int(8) unsigned NOT NULL auto_increment,
  			rev_page int(8) unsigned NOT NULL,
  			rev_text_id int(8) unsigned NOT NULL,
  			rev_comment tinyblob NOT NULL default '',
  			rev_user int(5) unsigned NOT NULL default '0',
  			rev_user_text varchar(255) binary NOT NULL default '',
  			rev_timestamp char(14) binary NOT NULL default '',
  			rev_minor_edit tinyint(1) unsigned NOT NULL default '0',
			rev_deleted tinyint(1) unsigned NOT NULL default '0',

  			PRIMARY KEY rev_page_id (rev_page, rev_id),
  			UNIQUE INDEX rev_id (rev_id),
  			INDEX rev_timestamp (rev_timestamp),
  			INDEX page_timestamp (rev_page,rev_timestamp),
  			INDEX user_timestamp (rev_user,rev_timestamp),
  			INDEX usertext_timestamp (rev_user_text,rev_timestamp)
			) TYPE=InnoDB", $fname );

		$maxold = intval( $this->dbw->selectField( 'old', 'max(old_id)', '', $fname ) );
		$this->log( "Last old record is {$maxold}" );

		global $wgLegacySchemaConversion;
		if( $wgLegacySchemaConversion ) {
			// Create HistoryBlobCurStub entries.
			// Text will be pulled from the leftover 'cur' table at runtime.
			echo "......Moving metadata from cur; using blob references to text in cur table.\n";
			$cur_text = "concat('O:18:\"historyblobcurstub\":1:{s:6:\"mCurId\";i:',cur_id,';}')";
			$cur_flags = "'object'";
		} else {
			// Copy all cur text in immediately: this may take longer but avoids
			// having to keep an extra table around.
			echo "......Moving text from cur.\n";
			$cur_text = 'cur_text';
			$cur_flags = "''";
		}

		$maxcur = $this->dbw->selectField( 'cur', 'max(cur_id)', '', $fname );
		$this->log( "Last cur entry is $maxcur" );

		/**
		 * Copy placeholder records for each page's current version into old
		 * Don't do any conversion here; text records are converted at runtime
		 * based on the flags (and may be originally binary!) while the meta
		 * fields will be converted in the old -> rev and cur -> page steps.
		 */
		$this->setChunkScale( $chunksize, $maxcur, 'old', $fname );
		$result = $this->dbr->query(
			"SELECT cur_id, cur_namespace, cur_title, $cur_text AS text, cur_comment,
			cur_user, cur_user_text, cur_timestamp, cur_minor_edit, $cur_flags AS flags
  			FROM $cur
  			ORDER BY cur_id", $fname );
  		$add = array();
		while( $row = $this->dbr->fetchObject( $result ) ) {
			$add[] = array(
				'old_namespace'  => $row->cur_namespace,
				'old_title'      => $row->cur_title,
				'old_text'       => $row->text,
				'old_comment'    => $row->cur_comment,
				'old_user'       => $row->cur_user,
				'old_user_text'  => $row->cur_user_text,
				'old_timestamp'  => $row->cur_timestamp,
				'old_minor_edit' => $row->cur_minor_edit,
				'old_flags'      => $row->flags );
			$this->addChunk( $add, $row->cur_id );
		}
		$this->lastChunk( $add );
		$this->dbr->freeResult( $result );

		/**
		 * Copy revision metadata from old into revision.
		 * We'll also do UTF-8 conversion of usernames and comments.
		 */
		#$newmaxold = $this->dbw->selectField( 'old', 'max(old_id)', '', $fname );
		#$this->setChunkScale( $chunksize, $newmaxold, 'revision', $fname );
		#$countold = $this->dbw->selectField( 'old', 'count(old_id)', '', $fname );
		$countold = $this->dbw->selectField( 'old', 'max(old_id)', '', $fname );
		$this->setChunkScale( $chunksize, $countold, 'revision', $fname );

		$this->log( "......Setting up revision table." );
		$result = $this->dbr->query(
			"SELECT old_id, cur_id, old_comment, old_user, old_user_text,
			old_timestamp, old_minor_edit
			FROM $old,$cur WHERE old_namespace=cur_namespace AND old_title=cur_title",
			$fname );

		$add = array();
		while( $row = $this->dbr->fetchObject( $result ) ) {
			$add[] = array(
				'rev_id'         =>              $row->old_id,
				'rev_page'       =>              $row->cur_id,
				'rev_text_id'    =>              $row->old_id,
				'rev_comment'    => $this->conv( $row->old_comment ),
				'rev_user'       =>              $row->old_user,
				'rev_user_text'  => $this->conv( $row->old_user_text ),
				'rev_timestamp'  =>              $row->old_timestamp,
				'rev_minor_edit' =>              $row->old_minor_edit );
			$this->addChunk( $add );
		}
		$this->lastChunk( $add );
		$this->dbr->freeResult( $result );


		/**
		 * Copy page metadata from cur into page.
		 * We'll also do UTF-8 conversion of titles.
		 */
		$this->log( "......Setting up page table." );
		$this->setChunkScale( $chunksize, $maxcur, 'page', $fname );
		$result = $this->dbr->query( "
			SELECT cur_id, cur_namespace, cur_title, cur_restrictions, cur_counter, cur_is_redirect, cur_is_new,
    				cur_random, cur_touched, rev_id, LENGTH(cur_text) AS len
  			FROM $cur,$revision
  			WHERE cur_id=rev_page AND rev_timestamp=cur_timestamp AND rev_id > {$maxold}
  			ORDER BY cur_id", $fname );
		$add = array();
		while( $row = $this->dbr->fetchObject( $result ) ) {
			$add[] = array(
				'page_id'           =>              $row->cur_id,
				'page_namespace'    =>              $row->cur_namespace,
				'page_title'        => $this->conv( $row->cur_title ),
				'page_restrictions' =>              $row->cur_restrictions,
				'page_counter'      =>              $row->cur_counter,
				'page_is_redirect'  =>              $row->cur_is_redirect,
				'page_is_new'       =>              $row->cur_is_new,
				'page_random'       =>              $row->cur_random,
				'page_touched'      =>              $this->dbw->timestamp(),
				'page_latest'       =>              $row->rev_id,
				'page_len'          =>              $row->len );
			#$this->addChunk( $add, $row->cur_id );
			$this->addChunk( $add );
		}
		$this->lastChunk( $add );
		$this->dbr->freeResult( $result );

		$this->log( "...done with cur/old -> page/revision." );
	}

	function upgradeLinks() {
		$fname = 'FiveUpgrade::upgradeLinks';
		$chunksize = 200;
		list ($links, $brokenlinks, $pagelinks, $cur) = $this->dbw->tableNamesN( 'links', 'brokenlinks', 'pagelinks', 'cur' );

		$this->log( 'Checking for interwiki table change in case of bogus items...' );
		if( $this->dbw->fieldExists( 'interwiki', 'iw_trans' ) ) {
			$this->log( 'interwiki has iw_trans.' );
		} else {
			global $IP;
			$this->log( 'adding iw_trans...' );
			dbsource( $IP . '/maintenance/archives/patch-interwiki-trans.sql', $this->dbw );
			$this->log( 'added iw_trans.' );
		}

		$this->log( 'Creating pagelinks table...' );
		$this->dbw->query( "
CREATE TABLE $pagelinks (
  -- Key to the page_id of the page containing the link.
  pl_from int(8) unsigned NOT NULL default '0',

  -- Key to page_namespace/page_title of the target page.
  -- The target page may or may not exist, and due to renames
  -- and deletions may refer to different page records as time
  -- goes by.
  pl_namespace int NOT NULL default '0',
  pl_title varchar(255) binary NOT NULL default '',

  UNIQUE KEY pl_from(pl_from,pl_namespace,pl_title),
  KEY (pl_namespace,pl_title)

) TYPE=InnoDB" );

		$this->log( 'Importing live links -> pagelinks' );
		$nlinks = $this->dbw->selectField( 'links', 'count(*)', '', $fname );
		if( $nlinks ) {
			$this->setChunkScale( $chunksize, $nlinks, 'pagelinks', $fname );
			$result = $this->dbr->query( "
			  SELECT l_from,cur_namespace,cur_title
				FROM $links, $cur
				WHERE l_to=cur_id", $fname );
			$add = array();
			while( $row = $this->dbr->fetchObject( $result ) ) {
				$add[] = array(
					'pl_from'      =>              $row->l_from,
					'pl_namespace' =>              $row->cur_namespace,
					'pl_title'     => $this->conv( $row->cur_title ) );
				$this->addChunk( $add );
			}
			$this->lastChunk( $add );
		} else {
			$this->log( 'no links!' );
		}

		$this->log( 'Importing brokenlinks -> pagelinks' );
		$nbrokenlinks = $this->dbw->selectField( 'brokenlinks', 'count(*)', '', $fname );
		if( $nbrokenlinks ) {
			$this->setChunkScale( $chunksize, $nbrokenlinks, 'pagelinks', $fname );
			$result = $this->dbr->query(
				"SELECT bl_from, bl_to FROM $brokenlinks",
				$fname );
			$add = array();
			while( $row = $this->dbr->fetchObject( $result ) ) {
				$pagename = $this->conv( $row->bl_to );
				$title = Title::newFromText( $pagename );
				if( is_null( $title ) ) {
					$this->log( "** invalid brokenlink: $row->bl_from -> '$pagename' (converted from '$row->bl_to')" );
				} else {
					$add[] = array(
						'pl_from'      => $row->bl_from,
						'pl_namespace' => $title->getNamespace(),
						'pl_title'     => $title->getDBkey() );
					$this->addChunk( $add );
				}
			}
			$this->lastChunk( $add );
		} else {
			$this->log( 'no brokenlinks!' );
		}

		$this->log( 'Done with links.' );
	}

	function upgradeUser() {
		// Apply unique index, if necessary:
		$duper = new UserDupes( $this->dbw );
		if( $duper->hasUniqueIndex() ) {
			$this->log( "Already have unique user_name index." );
		} else {
			$this->log( "Clearing user duplicates..." );
			if( !$duper->clearDupes() ) {
				$this->log( "WARNING: Duplicate user accounts, may explode!" );
			}
		}

		$tabledef = <<<END
CREATE TABLE $1 (
  user_id int(5) unsigned NOT NULL auto_increment,
  user_name varchar(255) binary NOT NULL default '',
  user_real_name varchar(255) binary NOT NULL default '',
  user_password tinyblob NOT NULL default '',
  user_newpassword tinyblob NOT NULL default '',
  user_email tinytext NOT NULL default '',
  user_options blob NOT NULL default '',
  user_touched char(14) binary NOT NULL default '',
  user_token char(32) binary NOT NULL default '',
  user_email_authenticated CHAR(14) BINARY,
  user_email_token CHAR(32) BINARY,
  user_email_token_expires CHAR(14) BINARY,

  PRIMARY KEY user_id (user_id),
  UNIQUE INDEX user_name (user_name),
  INDEX (user_email_token)

) TYPE=InnoDB
END;
		$fields = array(
			'user_id'                  => MW_UPGRADE_COPY,
			'user_name'                => MW_UPGRADE_ENCODE,
			'user_real_name'           => MW_UPGRADE_ENCODE,
			'user_password'            => MW_UPGRADE_COPY,
			'user_newpassword'         => MW_UPGRADE_COPY,
			'user_email'               => MW_UPGRADE_ENCODE,
			'user_options'             => MW_UPGRADE_ENCODE,
			'user_touched'             => MW_UPGRADE_CALLBACK,
			'user_token'               => MW_UPGRADE_COPY,
			'user_email_authenticated' => MW_UPGRADE_CALLBACK,
			'user_email_token'         => MW_UPGRADE_NULL,
			'user_email_token_expires' => MW_UPGRADE_NULL );
		$this->copyTable( 'user', $tabledef, $fields,
			array( &$this, 'userCallback' ) );
	}

	function userCallback( $row, $copy ) {
		$now = $this->dbw->timestamp();
		$copy['user_touched'] = $now;
		$copy['user_email_authenticated'] = $this->emailAuth ? $now : null;
		return $copy;
	}

	function upgradeImage() {
		$tabledef = <<<END
CREATE TABLE $1 (
  img_name varchar(255) binary NOT NULL default '',
  img_size int(8) unsigned NOT NULL default '0',
  img_width int(5)  NOT NULL default '0',
  img_height int(5)  NOT NULL default '0',
  img_metadata mediumblob NOT NULL,
  img_bits int(3)  NOT NULL default '0',
  img_media_type ENUM("UNKNOWN", "BITMAP", "DRAWING", "AUDIO", "VIDEO", "MULTIMEDIA", "OFFICE", "TEXT", "EXECUTABLE", "ARCHIVE") default NULL,
  img_major_mime ENUM("unknown", "application", "audio", "image", "text", "video", "message", "model", "multipart") NOT NULL default "unknown",
  img_minor_mime varchar(32) NOT NULL default "unknown",
  img_description tinyblob NOT NULL default '',
  img_user int(5) unsigned NOT NULL default '0',
  img_user_text varchar(255) binary NOT NULL default '',
  img_timestamp char(14) binary NOT NULL default '',

  PRIMARY KEY img_name (img_name),
  INDEX img_size (img_size),
  INDEX img_timestamp (img_timestamp)
) TYPE=InnoDB
END;
		$fields = array(
			'img_name'        => MW_UPGRADE_ENCODE,
			'img_size'        => MW_UPGRADE_COPY,
			'img_width'       => MW_UPGRADE_CALLBACK,
			'img_height'      => MW_UPGRADE_CALLBACK,
			'img_metadata'    => MW_UPGRADE_CALLBACK,
			'img_bits'        => MW_UPGRADE_CALLBACK,
			'img_media_type'  => MW_UPGRADE_CALLBACK,
			'img_major_mime'  => MW_UPGRADE_CALLBACK,
			'img_minor_mime'  => MW_UPGRADE_CALLBACK,
			'img_description' => MW_UPGRADE_ENCODE,
			'img_user'        => MW_UPGRADE_COPY,
			'img_user_text'   => MW_UPGRADE_ENCODE,
			'img_timestamp'   => MW_UPGRADE_COPY );
		$this->copyTable( 'image', $tabledef, $fields,
			array( &$this, 'imageCallback' ) );
	}

	function imageCallback( $row, $copy ) {
		global $options;
		if( !isset( $options['noimage'] ) ) {
			// Fill in the new image info fields
			$info = $this->imageInfo( $row->img_name );

			$copy['img_width'     ] = $info['width'];
			$copy['img_height'    ] = $info['height'];
			$copy['img_metadata'  ] = ""; // loaded on-demand
			$copy['img_bits'      ] = $info['bits'];
			$copy['img_media_type'] = $info['media'];
			$copy['img_major_mime'] = $info['major'];
			$copy['img_minor_mime'] = $info['minor'];
		}

		// If doing UTF8 conversion the file must be renamed
		$this->renameFile( $row->img_name, 'wfImageDir' );

		return $copy;
	}

	function imageInfo( $filename ) {
		$info = array(
			'width'  => 0,
			'height' => 0,
			'bits'   => 0,
			'media'  => '',
			'major'  => '',
			'minor'  => '' );

		$magic = MimeMagic::singleton();
		$mime = $magic->guessMimeType( $filename, true );
		list( $info['major'], $info['minor'] ) = explode( '/', $mime );

		$info['media'] = $magic->getMediaType( $filename, $mime );

		$image = UnregisteredLocalFile::newFromPath( $filename, $mime );

		$info['width'] = $image->getWidth();
		$info['height'] = $image->getHeight();

		$gis = $image->getImageSize();
		if ( isset( $gis['bits'] ) ) {
			$info['bits'] = $gis['bits'];
		}

		return $info;
	}


	/**
	 * Truncate a table.
	 * @param string $table The table name to be truncated
	 */
	function clearTable( $table ) {
		print "Clearing $table...\n";
		$tableName = $this->db->tableName( $table );
		$this->db->query( "TRUNCATE $tableName" );
	}

	/**
	 * Rename a given image or archived image file to the converted filename,
	 * leaving a symlink for URL compatibility.
	 *
	 * @param string $oldname pre-conversion filename
	 * @param string $basename pre-conversion base filename for dir hashing, if an archive
	 * @access private
	 */
	function renameFile( $oldname, $subdirCallback='wfImageDir', $basename=null ) {
		$newname = $this->conv( $oldname );
		if( $newname == $oldname ) {
			// No need to rename; another field triggered this row.
			return false;
		}

		if( is_null( $basename ) ) $basename = $oldname;
		$ubasename = $this->conv( $basename );
		$oldpath = call_user_func( $subdirCallback, $basename ) . '/' . $oldname;
		$newpath = call_user_func( $subdirCallback, $ubasename ) . '/' . $newname;

		$this->log( "$oldpath -> $newpath" );
		if( rename( $oldpath, $newpath ) ) {
			$relpath = wfRelativePath( $newpath, dirname( $oldpath ) );
			if( !symlink( $relpath, $oldpath ) ) {
				$this->log( "... symlink failed!" );
			}
			return $newname;
		} else {
			$this->log( "... rename failed!" );
			return false;
		}
	}

	function upgradeOldImage() {
		$tabledef = <<<END
CREATE TABLE $1 (
  -- Base filename: key to image.img_name
  oi_name varchar(255) binary NOT NULL default '',

  -- Filename of the archived file.
  -- This is generally a timestamp and '!' prepended to the base name.
  oi_archive_name varchar(255) binary NOT NULL default '',

  -- Other fields as in image...
  oi_size int(8) unsigned NOT NULL default 0,
  oi_width int(5) NOT NULL default 0,
  oi_height int(5) NOT NULL default 0,
  oi_bits int(3) NOT NULL default 0,
  oi_description tinyblob NOT NULL default '',
  oi_user int(5) unsigned NOT NULL default '0',
  oi_user_text varchar(255) binary NOT NULL default '',
  oi_timestamp char(14) binary NOT NULL default '',

  INDEX oi_name (oi_name(10))

) TYPE=InnoDB;
END;
		$fields = array(
			'oi_name'         => MW_UPGRADE_ENCODE,
			'oi_archive_name' => MW_UPGRADE_ENCODE,
			'oi_size'         => MW_UPGRADE_COPY,
			'oi_width'        => MW_UPGRADE_CALLBACK,
			'oi_height'       => MW_UPGRADE_CALLBACK,
			'oi_bits'         => MW_UPGRADE_CALLBACK,
			'oi_description'  => MW_UPGRADE_ENCODE,
			'oi_user'         => MW_UPGRADE_COPY,
			'oi_user_text'    => MW_UPGRADE_ENCODE,
			'oi_timestamp'    => MW_UPGRADE_COPY );
		$this->copyTable( 'oldimage', $tabledef, $fields,
			array( &$this, 'oldimageCallback' ) );
	}

	function oldimageCallback( $row, $copy ) {
		global $options;
		if( !isset( $options['noimage'] ) ) {
			// Fill in the new image info fields
			$info = $this->imageInfo( $row->oi_archive_name, 'wfImageArchiveDir', $row->oi_name );
			$copy['oi_width' ] = $info['width' ];
			$copy['oi_height'] = $info['height'];
			$copy['oi_bits'  ] = $info['bits'  ];
		}

		// If doing UTF8 conversion the file must be renamed
		$this->renameFile( $row->oi_archive_name, 'wfImageArchiveDir', $row->oi_name );

		return $copy;
	}


	function upgradeWatchlist() {
		$fname = 'FiveUpgrade::upgradeWatchlist';
		$chunksize = 100;

		list ($watchlist, $watchlist_temp) = $this->dbw->tableNamesN( 'watchlist', 'watchlist_temp' );

		$this->log( 'Migrating watchlist table to watchlist_temp...' );
		$this->dbw->query(
"CREATE TABLE $watchlist_temp (
  -- Key to user_id
  wl_user int(5) unsigned NOT NULL,

  -- Key to page_namespace/page_title
  -- Note that users may watch patches which do not exist yet,
  -- or existed in the past but have been deleted.
  wl_namespace int NOT NULL default '0',
  wl_title varchar(255) binary NOT NULL default '',

  -- Timestamp when user was last sent a notification e-mail;
  -- cleared when the user visits the page.
  -- FIXME: add proper null support etc
  wl_notificationtimestamp varchar(14) binary NOT NULL default '0',

  UNIQUE KEY (wl_user, wl_namespace, wl_title),
  KEY namespace_title (wl_namespace,wl_title)

) TYPE=InnoDB;", $fname );

		// Fix encoding for Latin-1 upgrades, add some fields,
		// and double article to article+talk pairs
		$numwatched = $this->dbw->selectField( 'watchlist', 'count(*)', '', $fname );

		$this->setChunkScale( $chunksize, $numwatched * 2, 'watchlist_temp', $fname );
		$result = $this->dbr->select( 'watchlist',
			array(
				'wl_user',
				'wl_namespace',
				'wl_title' ),
			'',
			$fname );

		$add = array();
		while( $row = $this->dbr->fetchObject( $result ) ) {
			$add[] = array(
				'wl_user'      =>                        $row->wl_user,
				'wl_namespace' => Namespace::getSubject( $row->wl_namespace ),
				'wl_title'     =>           $this->conv( $row->wl_title ),
				'wl_notificationtimestamp' =>            '0' );
			$this->addChunk( $add );

			$add[] = array(
				'wl_user'      =>                        $row->wl_user,
				'wl_namespace' =>    Namespace::getTalk( $row->wl_namespace ),
				'wl_title'     =>           $this->conv( $row->wl_title ),
				'wl_notificationtimestamp' =>            '0' );
			$this->addChunk( $add );
		}
		$this->lastChunk( $add );
		$this->dbr->freeResult( $result );

		$this->log( 'Done converting watchlist.' );
		$this->cleanupSwaps[] = 'watchlist';
	}

	function upgradeLogging() {
		$tabledef = <<<ENDS
CREATE TABLE $1 (
  -- Symbolic keys for the general log type and the action type
  -- within the log. The output format will be controlled by the
  -- action field, but only the type controls categorization.
  log_type char(10) NOT NULL default '',
  log_action char(10) NOT NULL default '',

  -- Timestamp. Duh.
  log_timestamp char(14) NOT NULL default '19700101000000',

  -- The user who performed this action; key to user_id
  log_user int unsigned NOT NULL default 0,

  -- Key to the page affected. Where a user is the target,
  -- this will point to the user page.
  log_namespace int NOT NULL default 0,
  log_title varchar(255) binary NOT NULL default '',

  -- Freeform text. Interpreted as edit history comments.
  log_comment varchar(255) NOT NULL default '',

  -- LF separated list of miscellaneous parameters
  log_params blob NOT NULL default '',

  KEY type_time (log_type, log_timestamp),
  KEY user_time (log_user, log_timestamp),
  KEY page_time (log_namespace, log_title, log_timestamp)

) TYPE=InnoDB
ENDS;
		$fields = array(
			'log_type'      => MW_UPGRADE_COPY,
			'log_action'    => MW_UPGRADE_COPY,
			'log_timestamp' => MW_UPGRADE_COPY,
			'log_user'      => MW_UPGRADE_COPY,
			'log_namespace' => MW_UPGRADE_COPY,
			'log_title'     => MW_UPGRADE_ENCODE,
			'log_comment'   => MW_UPGRADE_ENCODE,
			'log_params'    => MW_UPGRADE_ENCODE );
		$this->copyTable( 'logging', $tabledef, $fields );
	}

	function upgradeArchive() {
		$tabledef = <<<ENDS
CREATE TABLE $1 (
  ar_namespace int NOT NULL default '0',
  ar_title varchar(255) binary NOT NULL default '',
  ar_text mediumblob NOT NULL default '',

  ar_comment tinyblob NOT NULL default '',
  ar_user int(5) unsigned NOT NULL default '0',
  ar_user_text varchar(255) binary NOT NULL,
  ar_timestamp char(14) binary NOT NULL default '',
  ar_minor_edit tinyint(1) NOT NULL default '0',

  ar_flags tinyblob NOT NULL default '',

  ar_rev_id int(8) unsigned,
  ar_text_id int(8) unsigned,

  KEY name_title_timestamp (ar_namespace,ar_title,ar_timestamp)

) TYPE=InnoDB
ENDS;
		$fields = array(
			'ar_namespace'  => MW_UPGRADE_COPY,
			'ar_title'      => MW_UPGRADE_ENCODE,
			'ar_text'       => MW_UPGRADE_COPY,
			'ar_comment'    => MW_UPGRADE_ENCODE,
			'ar_user'       => MW_UPGRADE_COPY,
			'ar_user_text'  => MW_UPGRADE_ENCODE,
			'ar_timestamp'  => MW_UPGRADE_COPY,
			'ar_minor_edit' => MW_UPGRADE_COPY,
			'ar_flags'      => MW_UPGRADE_COPY,
			'ar_rev_id'     => MW_UPGRADE_NULL,
			'ar_text_id'    => MW_UPGRADE_NULL );
		$this->copyTable( 'archive', $tabledef, $fields );
	}

	function upgradeImagelinks() {
		global $wgUseLatin1;
		if( $wgUseLatin1 ) {
			$tabledef = <<<ENDS
CREATE TABLE $1 (
  -- Key to page_id of the page containing the image / media link.
  il_from int(8) unsigned NOT NULL default '0',

  -- Filename of target image.
  -- This is also the page_title of the file's description page;
  -- all such pages are in namespace 6 (NS_IMAGE).
  il_to varchar(255) binary NOT NULL default '',

  UNIQUE KEY il_from(il_from,il_to),
  KEY (il_to)

) TYPE=InnoDB
ENDS;
			$fields = array(
				'il_from' => MW_UPGRADE_COPY,
				'il_to'   => MW_UPGRADE_ENCODE );
			$this->copyTable( 'imagelinks', $tabledef, $fields );
		}
	}

	function upgradeCategorylinks() {
		global $wgUseLatin1;
		if( $wgUseLatin1 ) {
			$tabledef = <<<ENDS
CREATE TABLE $1 (
  cl_from int(8) unsigned NOT NULL default '0',
  cl_to varchar(255) binary NOT NULL default '',
  cl_sortkey varchar(86) binary NOT NULL default '',
  cl_timestamp timestamp NOT NULL,

  UNIQUE KEY cl_from(cl_from,cl_to),
  KEY cl_sortkey(cl_to,cl_sortkey),
  KEY cl_timestamp(cl_to,cl_timestamp)
) TYPE=InnoDB
ENDS;
			$fields = array(
				'cl_from'      => MW_UPGRADE_COPY,
				'cl_to'        => MW_UPGRADE_ENCODE,
				'cl_sortkey'   => MW_UPGRADE_ENCODE,
				'cl_timestamp' => MW_UPGRADE_COPY );
			$this->copyTable( 'categorylinks', $tabledef, $fields );
		}
	}

	function upgradeIpblocks() {
		global $wgUseLatin1;
		if( $wgUseLatin1 ) {
			$tabledef = <<<ENDS
CREATE TABLE $1 (
  ipb_id int(8) NOT NULL auto_increment,
  ipb_address varchar(40) binary NOT NULL default '',
  ipb_user int(8) unsigned NOT NULL default '0',
  ipb_by int(8) unsigned NOT NULL default '0',
  ipb_reason tinyblob NOT NULL default '',
  ipb_timestamp char(14) binary NOT NULL default '',
  ipb_auto tinyint(1) NOT NULL default '0',
  ipb_expiry char(14) binary NOT NULL default '',

  PRIMARY KEY ipb_id (ipb_id),
  INDEX ipb_address (ipb_address),
  INDEX ipb_user (ipb_user)

) TYPE=InnoDB
ENDS;
			$fields = array(
				'ipb_id'        => MW_UPGRADE_COPY,
				'ipb_address'   => MW_UPGRADE_COPY,
				'ipb_user'      => MW_UPGRADE_COPY,
				'ipb_by'        => MW_UPGRADE_COPY,
				'ipb_reason'    => MW_UPGRADE_ENCODE,
				'ipb_timestamp' => MW_UPGRADE_COPY,
				'ipb_auto'      => MW_UPGRADE_COPY,
				'ipb_expiry'    => MW_UPGRADE_COPY );
			$this->copyTable( 'ipblocks', $tabledef, $fields );
		}
	}

	function upgradeRecentchanges() {
		// There's a format change in the namespace field
		$tabledef = <<<ENDS
CREATE TABLE $1 (
  rc_id int(8) NOT NULL auto_increment,
  rc_timestamp varchar(14) binary NOT NULL default '',
  rc_cur_time varchar(14) binary NOT NULL default '',

  rc_user int(10) unsigned NOT NULL default '0',
  rc_user_text varchar(255) binary NOT NULL default '',

  rc_namespace int NOT NULL default '0',
  rc_title varchar(255) binary NOT NULL default '',

  rc_comment varchar(255) binary NOT NULL default '',
  rc_minor tinyint(3) unsigned NOT NULL default '0',

  rc_bot tinyint(3) unsigned NOT NULL default '0',
  rc_new tinyint(3) unsigned NOT NULL default '0',

  rc_cur_id int(10) unsigned NOT NULL default '0',
  rc_this_oldid int(10) unsigned NOT NULL default '0',
  rc_last_oldid int(10) unsigned NOT NULL default '0',

  rc_type tinyint(3) unsigned NOT NULL default '0',
  rc_moved_to_ns tinyint(3) unsigned NOT NULL default '0',
  rc_moved_to_title varchar(255) binary NOT NULL default '',

  rc_patrolled tinyint(3) unsigned NOT NULL default '0',

  rc_ip char(15) NOT NULL default '',

  PRIMARY KEY rc_id (rc_id),
  INDEX rc_timestamp (rc_timestamp),
  INDEX rc_namespace_title (rc_namespace, rc_title),
  INDEX rc_cur_id (rc_cur_id),
  INDEX new_name_timestamp(rc_new,rc_namespace,rc_timestamp),
  INDEX rc_ip (rc_ip)

) TYPE=InnoDB
ENDS;
		$fields = array(
			'rc_id'             => MW_UPGRADE_COPY,
			'rc_timestamp'      => MW_UPGRADE_COPY,
			'rc_cur_time'       => MW_UPGRADE_COPY,
			'rc_user'           => MW_UPGRADE_COPY,
			'rc_user_text'      => MW_UPGRADE_ENCODE,
			'rc_namespace'      => MW_UPGRADE_COPY,
			'rc_title'          => MW_UPGRADE_ENCODE,
			'rc_comment'        => MW_UPGRADE_ENCODE,
			'rc_minor'          => MW_UPGRADE_COPY,
			'rc_bot'            => MW_UPGRADE_COPY,
			'rc_new'            => MW_UPGRADE_COPY,
			'rc_cur_id'         => MW_UPGRADE_COPY,
			'rc_this_oldid'     => MW_UPGRADE_COPY,
			'rc_last_oldid'     => MW_UPGRADE_COPY,
			'rc_type'           => MW_UPGRADE_COPY,
			'rc_moved_to_ns'    => MW_UPGRADE_COPY,
			'rc_moved_to_title' => MW_UPGRADE_ENCODE,
			'rc_patrolled'      => MW_UPGRADE_COPY,
			'rc_ip'             => MW_UPGRADE_COPY );
		$this->copyTable( 'recentchanges', $tabledef, $fields );
	}

	function upgradeQuerycache() {
		// There's a format change in the namespace field
		$tabledef = <<<ENDS
CREATE TABLE $1 (
  -- A key name, generally the base name of of the special page.
  qc_type char(32) NOT NULL,

  -- Some sort of stored value. Sizes, counts...
  qc_value int(5) unsigned NOT NULL default '0',

  -- Target namespace+title
  qc_namespace int NOT NULL default '0',
  qc_title char(255) binary NOT NULL default '',

  KEY (qc_type,qc_value)

) TYPE=InnoDB
ENDS;
		$fields = array(
			'qc_type'      => MW_UPGRADE_COPY,
			'qc_value'     => MW_UPGRADE_COPY,
			'qc_namespace' => MW_UPGRADE_COPY,
			'qc_title'     => MW_UPGRADE_ENCODE );
		$this->copyTable( 'querycache', $tabledef, $fields );
	}

	/**
	 * Rename all our temporary tables into final place.
	 * We've left things in place so a read-only wiki can continue running
	 * on the old code during all this.
	 */
	function upgradeCleanup() {
		$this->renameTable( 'old', 'text' );

		foreach( $this->cleanupSwaps as $table ) {
			$this->swap( $table );
		}
	}

	function renameTable( $from, $to ) {
		$this->log( "Renaming $from to $to..." );

		$fromtable = $this->dbw->tableName( $from );
		$totable   = $this->dbw->tableName( $to );
		$this->dbw->query( "ALTER TABLE $fromtable RENAME TO $totable" );
	}

	function swap( $base ) {
		$this->renameTable( $base, "{$base}_old" );
		$this->renameTable( "{$base}_temp", $base );
	}

}
