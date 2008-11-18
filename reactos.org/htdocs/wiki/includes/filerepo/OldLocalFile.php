<?php

/**
 * Class to represent a file in the oldimage table
 *
 * @ingroup FileRepo
 */
class OldLocalFile extends LocalFile {
	var $requestedTime, $archive_name;

	const CACHE_VERSION = 1;
	const MAX_CACHE_ROWS = 20;

	static function newFromTitle( $title, $repo, $time = null ) {
		# The null default value is only here to avoid an E_STRICT
		if( $time === null )
			throw new MWException( __METHOD__.' got null for $time parameter' );
		return new self( $title, $repo, $time, null );
	}

	static function newFromArchiveName( $title, $repo, $archiveName ) {
		return new self( $title, $repo, null, $archiveName );
	}

	static function newFromRow( $row, $repo ) {
		$title = Title::makeTitle( NS_IMAGE, $row->oi_name );
		$file = new self( $title, $repo, null, $row->oi_archive_name );
		$file->loadFromRow( $row, 'oi_' );
		return $file;
	}
	
	static function newFromKey( $sha1, $repo, $timestamp = false ) {
		# Polymorphic function name to distinguish foreign and local fetches
		$fname = get_class( $this ) . '::' . __FUNCTION__;

		$conds = array( 'oi_sha1' => $sha1 );
		if( $timestamp ) {
			$conds['oi_timestamp'] = $timestamp;
		}
		$row = $dbr->selectRow( 'oldimage', $this->getCacheFields( 'oi_' ), $conds, $fname );
		if( $row ) {
			return self::newFromRow( $row, $repo );
		} else {
			return false;
		}
	}
	
	/**
	 * Fields in the oldimage table
	 */
	static function selectFields() {
		return array(
			'oi_name',
			'oi_archive_name',
			'oi_size',
			'oi_width',
			'oi_height',
			'oi_metadata',
			'oi_bits',
			'oi_media_type',
			'oi_major_mime',
			'oi_minor_mime',
			'oi_description',
			'oi_user',
			'oi_user_text',
			'oi_timestamp',
			'oi_sha1',
		);
	}

	/**
	 * @param Title $title
	 * @param FileRepo $repo
	 * @param string $time Timestamp or null to load by archive name
	 * @param string $archiveName Archive name or null to load by timestamp
	 */
	function __construct( $title, $repo, $time, $archiveName ) {
		parent::__construct( $title, $repo );
		$this->requestedTime = $time;
		$this->archive_name = $archiveName;
		if ( is_null( $time ) && is_null( $archiveName ) ) {
			throw new MWException( __METHOD__.': must specify at least one of $time or $archiveName' );
		}
	}

	function getCacheKey() {
		return false;
	}

	function getArchiveName() {
		if ( !isset( $this->archive_name ) ) {
			$this->load();
		}
		return $this->archive_name;
	}

	function isOld() {
		return true;
	}

	function isVisible() {
		return $this->exists() && !$this->isDeleted(File::DELETED_FILE);
	}

	function loadFromDB() {
		wfProfileIn( __METHOD__ );
		$this->dataLoaded = true;
		$dbr = $this->repo->getSlaveDB();
		$conds = array( 'oi_name' => $this->getName() );
		if ( is_null( $this->requestedTime ) ) {
			$conds['oi_archive_name'] = $this->archive_name;
		} else {
			$conds[] = 'oi_timestamp = ' . $dbr->addQuotes( $dbr->timestamp( $this->requestedTime ) );
		}
		$row = $dbr->selectRow( 'oldimage', $this->getCacheFields( 'oi_' ),
			$conds, __METHOD__, array( 'ORDER BY' => 'oi_timestamp DESC' ) );
		if ( $row ) {
			$this->loadFromRow( $row, 'oi_' );
		} else {
			$this->fileExists = false;
		}
		wfProfileOut( __METHOD__ );
	}

	function getCacheFields( $prefix = 'img_' ) {
		$fields = parent::getCacheFields( $prefix );
		$fields[] = $prefix . 'archive_name';
		$fields[] = $prefix . 'deleted';
		return $fields;
	}

	function getRel() {
		return 'archive/' . $this->getHashPath() . $this->getArchiveName();
	}

	function getUrlRel() {
		return 'archive/' . $this->getHashPath() . urlencode( $this->getArchiveName() );
	}

	function upgradeRow() {
		wfProfileIn( __METHOD__ );
		$this->loadFromFile();

		# Don't destroy file info of missing files
		if ( !$this->fileExists ) {
			wfDebug( __METHOD__.": file does not exist, aborting\n" );
			wfProfileOut( __METHOD__ );
			return;
		}

		$dbw = $this->repo->getMasterDB();
		list( $major, $minor ) = self::splitMime( $this->mime );

		wfDebug(__METHOD__.': upgrading '.$this->archive_name." to the current schema\n");
		$dbw->update( 'oldimage',
			array(
				'oi_width' => $this->width,
				'oi_height' => $this->height,
				'oi_bits' => $this->bits,
				'oi_media_type' => $this->media_type,
				'oi_major_mime' => $major,
				'oi_minor_mime' => $minor,
				'oi_metadata' => $this->metadata,
				'oi_sha1' => $this->sha1,
			), array(
				'oi_name' => $this->getName(),
				'oi_archive_name' => $this->archive_name ),
			__METHOD__
		);
		wfProfileOut( __METHOD__ );
	}

	/**
	 * int $field one of DELETED_* bitfield constants
	 * for file or revision rows
	 * @return bool
	 */
	function isDeleted( $field ) {
		return ($this->deleted & $field) == $field;
	}

	/**
	 * Determine if the current user is allowed to view a particular
	 * field of this FileStore image file, if it's marked as deleted.
	 * @param int $field
	 * @return bool
	 */
	function userCan( $field ) {
		if( isset($this->deleted) && ($this->deleted & $field) == $field ) {
			global $wgUser;
			$permission = ( $this->deleted & File::DELETED_RESTRICTED ) == File::DELETED_RESTRICTED
				? 'suppressrevision'
				: 'deleterevision';
			wfDebug( "Checking for $permission due to $field match on $this->mDeleted\n" );
			return $wgUser->isAllowed( $permission );
		} else {
			return true;
		}
	}
}
