<?php

/**
 * Static accessor class for site_stats and related things
 */
class SiteStats {
	static $row, $loaded = false;
	static $admins, $jobs;
	static $pageCount = array();

	static function recache() {
		self::load( true );
	}

	static function load( $recache = false ) {
		if ( self::$loaded && !$recache ) {
			return;
		}

		self::$row = self::loadAndLazyInit();

		# This code is somewhat schema-agnostic, because I'm changing it in a minor release -- TS
		if ( !isset( self::$row->ss_total_pages ) && self::$row->ss_total_pages == -1 ) {
			# Update schema
			$u = new SiteStatsUpdate( 0, 0, 0 );
			$u->doUpdate();
			$dbr = wfGetDB( DB_SLAVE );
			self::$row = $dbr->selectRow( 'site_stats', '*', false, __METHOD__ );
		}

		self::$loaded = true;
	}

	static function loadAndLazyInit() {
		wfDebug( __METHOD__ . ": reading site_stats from slave\n" );
		$row = self::doLoad( wfGetDB( DB_SLAVE ) );

		if( !self::isSane( $row ) ) {
			// Might have just been initialized during this request? Underflow?
			wfDebug( __METHOD__ . ": site_stats damaged or missing on slave\n" );
			$row = self::doLoad( wfGetDB( DB_MASTER ) );
		}

		if( !self::isSane( $row ) ) {
			// Normally the site_stats table is initialized at install time.
			// Some manual construction scenarios may leave the table empty or
			// broken, however, for instance when importing from a dump into a
			// clean schema with mwdumper.
			wfDebug( __METHOD__ . ": initializing damaged or missing site_stats\n" );

			global $IP;
			require_once "$IP/maintenance/initStats.inc";

			ob_start();
			wfInitStats();
			ob_end_clean();

			$row = self::doLoad( wfGetDB( DB_MASTER ) );
		}

		if( !self::isSane( $row ) ) {
			wfDebug( __METHOD__ . ": site_stats persistently nonsensical o_O\n" );
		}
		return $row;
	}

	static function doLoad( $db ) {
		return $db->selectRow( 'site_stats', '*', false, __METHOD__ );
	}

	static function views() {
		self::load();
		return self::$row->ss_total_views;
	}

	static function edits() {
		self::load();
		return self::$row->ss_total_edits;
	}

	static function articles() {
		self::load();
		return self::$row->ss_good_articles;
	}

	static function pages() {
		self::load();
		return self::$row->ss_total_pages;
	}

	static function users() {
		self::load();
		return self::$row->ss_users;
	}

	static function images() {
		self::load();
		return self::$row->ss_images;
	}

	static function admins() {
		if ( !isset( self::$admins ) ) {
			$dbr = wfGetDB( DB_SLAVE );
			self::$admins = $dbr->selectField( 'user_groups', 'COUNT(*)', array( 'ug_group' => 'sysop' ), __METHOD__ );
		}
		return self::$admins;
	}

	static function jobs() {
		if ( !isset( self::$jobs ) ) {
			$dbr = wfGetDB( DB_SLAVE );
			self::$jobs = $dbr->estimateRowCount('job');
			/* Zero rows still do single row read for row that doesn't exist, but people are annoyed by that */
			if (self::$jobs == 1) {
				self::$jobs = 0;
			}
		}
		return self::$jobs;
	}

	static function pagesInNs( $ns ) {
		wfProfileIn( __METHOD__ );
		if( !isset( self::$pageCount[$ns] ) ) {
			$dbr = wfGetDB( DB_SLAVE );
			$pageCount[$ns] = (int)$dbr->selectField( 'page', 'COUNT(*)', array( 'page_namespace' => $ns ), __METHOD__ );
		}
		wfProfileOut( __METHOD__ );
		return $pageCount[$ns];
	}

	/** Is the provided row of site stats sane, or should it be regenerated? */
	private static function isSane( $row ) {
		if(
			$row === false
			or $row->ss_total_pages < $row->ss_good_articles
			or $row->ss_total_edits < $row->ss_total_pages
			or $row->ss_users       < $row->ss_admins
		) {
			return false;
		}
		// Now check for underflow/overflow
		foreach( array( 'total_views', 'total_edits', 'good_articles',
		'total_pages', 'users', 'admins', 'images' ) as $member ) {
			if(
				   $row->{"ss_$member"} > 2000000000
				or $row->{"ss_$member"} < 0
			) {
				return false;
			}
		}
		return true;
	}
}


/**
 *
 */
class SiteStatsUpdate {

	var $mViews, $mEdits, $mGood, $mPages, $mUsers;

	function __construct( $views, $edits, $good, $pages = 0, $users = 0 ) {
		$this->mViews = $views;
		$this->mEdits = $edits;
		$this->mGood = $good;
		$this->mPages = $pages;
		$this->mUsers = $users;
	}

	function appendUpdate( &$sql, $field, $delta ) {
		if ( $delta ) {
			if ( $sql ) {
				$sql .= ',';
			}
			if ( $delta < 0 ) {
				$sql .= "$field=$field-1";
			} else {
				$sql .= "$field=$field+1";
			}
		}
	}

	function doUpdate() {
		$fname = 'SiteStatsUpdate::doUpdate';
		$dbw = wfGetDB( DB_MASTER );

		# First retrieve the row just to find out which schema we're in
		$row = $dbw->selectRow( 'site_stats', '*', false, $fname );

		$updates = '';

		$this->appendUpdate( $updates, 'ss_total_views', $this->mViews );
		$this->appendUpdate( $updates, 'ss_total_edits', $this->mEdits );
		$this->appendUpdate( $updates, 'ss_good_articles', $this->mGood );

		if ( isset( $row->ss_total_pages ) ) {
			# Update schema if required
			if ( $row->ss_total_pages == -1 && !$this->mViews ) {
				$dbr = wfGetDB( DB_SLAVE, array( 'SpecialStatistics', 'vslow') );
				list( $page, $user ) = $dbr->tableNamesN( 'page', 'user' );

				$sql = "SELECT COUNT(page_namespace) AS total FROM $page";
				$res = $dbr->query( $sql, $fname );
				$pageRow = $dbr->fetchObject( $res );
				$pages = $pageRow->total + $this->mPages;

				$sql = "SELECT COUNT(user_id) AS total FROM $user";
				$res = $dbr->query( $sql, $fname );
				$userRow = $dbr->fetchObject( $res );
				$users = $userRow->total + $this->mUsers;

				if ( $updates ) {
					$updates .= ',';
				}
				$updates .= "ss_total_pages=$pages, ss_users=$users";
			} else {
				$this->appendUpdate( $updates, 'ss_total_pages', $this->mPages );
				$this->appendUpdate( $updates, 'ss_users', $this->mUsers );
			}
		}
		if ( $updates ) {
			$site_stats = $dbw->tableName( 'site_stats' );
			$sql = $dbw->limitResultForUpdate("UPDATE $site_stats SET $updates", 1);
			$dbw->begin();
			$dbw->query( $sql, $fname );
			$dbw->commit();
		}

		/*
		global $wgDBname, $wgTitle;
		if ( $this->mGood && $wgDBname == 'enwiki' ) {
			$good = $dbw->selectField( 'site_stats', 'ss_good_articles', '', $fname );
			error_log( $good . ' ' . $wgTitle->getPrefixedDBkey() . "\n", 3, '/home/wikipedia/logs/million.log' );
		}
		*/
	}
}
