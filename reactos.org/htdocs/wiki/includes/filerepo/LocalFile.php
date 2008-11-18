<?php
/**
 */

/**
 * Bump this number when serialized cache records may be incompatible.
 */
define( 'MW_FILE_VERSION', 8 );

/**
 * Class to represent a local file in the wiki's own database
 *
 * Provides methods to retrieve paths (physical, logical, URL),
 * to generate image thumbnails or for uploading.
 *
 * Note that only the repo object knows what its file class is called. You should
 * never name a file class explictly outside of the repo class. Instead use the
 * repo's factory functions to generate file objects, for example:
 *
 * RepoGroup::singleton()->getLocalRepo()->newFile($title);
 *
 * The convenience functions wfLocalFile() and wfFindFile() should be sufficient
 * in most cases.
 *
 * @ingroup FileRepo
 */
class LocalFile extends File
{
	/**#@+
	 * @private
	 */
	var	$fileExists,       # does the file file exist on disk? (loadFromXxx)
		$historyLine, 	   # Number of line to return by nextHistoryLine() (constructor)
		$historyRes, 	   # result of the query for the file's history (nextHistoryLine)
		$width,            # \
		$height,           #  |
		$bits,             #   --- returned by getimagesize (loadFromXxx)
		$attr,             # /
		$media_type,       # MEDIATYPE_xxx (bitmap, drawing, audio...)
		$mime,             # MIME type, determined by MimeMagic::guessMimeType
		$major_mime,       # Major mime type
		$minor_mime,       # Minor mime type
		$size,             # Size in bytes (loadFromXxx)
		$metadata,         # Handler-specific metadata
		$timestamp,        # Upload timestamp
		$sha1,             # SHA-1 base 36 content hash
		$user, $user_text, # User, who uploaded the file
		$description,      # Description of current revision of the file
		$dataLoaded,       # Whether or not all this has been loaded from the database (loadFromXxx)
		$upgraded,         # Whether the row was upgraded on load
		$locked,           # True if the image row is locked
		$deleted;       # Bitfield akin to rev_deleted

	/**#@-*/

	/**
	 * Create a LocalFile from a title
	 * Do not call this except from inside a repo class.
	 *
	 * Note: $unused param is only here to avoid an E_STRICT
	 */
	static function newFromTitle( $title, $repo, $unused = null ) {
		return new self( $title, $repo );
	}

	/**
	 * Create a LocalFile from a title
	 * Do not call this except from inside a repo class.
	 */
	static function newFromRow( $row, $repo ) {
		$title = Title::makeTitle( NS_IMAGE, $row->img_name );
		$file = new self( $title, $repo );
		$file->loadFromRow( $row );
		return $file;
	}
	
	/**
	 * Create a LocalFile from a SHA-1 key
	 * Do not call this except from inside a repo class.
	 */
	static function newFromKey( $sha1, $repo, $timestamp = false ) {
		# Polymorphic function name to distinguish foreign and local fetches
		$fname = get_class( $this ) . '::' . __FUNCTION__;

		$conds = array( 'img_sha1' => $sha1 );
		if( $timestamp ) {
			$conds['img_timestamp'] = $timestamp;
		}
		$row = $dbr->selectRow( 'image', $this->getCacheFields( 'img_' ), $conds, $fname );
		if( $row ) {
			return self::newFromRow( $row, $repo );
		} else {
			return false;
		}
	}
	
	/**
	 * Fields in the image table
	 */
	static function selectFields() {
		return array(
			'img_name',
			'img_size',
			'img_width',
			'img_height',
			'img_metadata',
			'img_bits',
			'img_media_type',
			'img_major_mime',
			'img_minor_mime',
			'img_description',
			'img_user',
			'img_user_text',
			'img_timestamp',
			'img_sha1',
		);
	}

	/**
	 * Constructor.
	 * Do not call this except from inside a repo class.
	 */
	function __construct( $title, $repo ) {
		if( !is_object( $title ) ) {
			throw new MWException( __CLASS__.' constructor given bogus title.' );
		}
		parent::__construct( $title, $repo );
		$this->metadata = '';
		$this->historyLine = 0;
		$this->historyRes = null;
		$this->dataLoaded = false;
	}

	/**
	 * Get the memcached key
	 */
	function getCacheKey() {
		$hashedName = md5($this->getName());
		return wfMemcKey( 'file', $hashedName );
	}

	/**
	 * Try to load file metadata from memcached. Returns true on success.
	 */
	function loadFromCache() {
		global $wgMemc;
		wfProfileIn( __METHOD__ );
		$this->dataLoaded = false;
		$key = $this->getCacheKey();
		if ( !$key ) {
			return false;
		}
		$cachedValues = $wgMemc->get( $key );

		// Check if the key existed and belongs to this version of MediaWiki
		if ( isset($cachedValues['version']) && ( $cachedValues['version'] == MW_FILE_VERSION ) ) {
			wfDebug( "Pulling file metadata from cache key $key\n" );
			$this->fileExists = $cachedValues['fileExists'];
			if ( $this->fileExists ) {
				$this->setProps( $cachedValues );
			}
			$this->dataLoaded = true;
		}
		if ( $this->dataLoaded ) {
			wfIncrStats( 'image_cache_hit' );
		} else {
			wfIncrStats( 'image_cache_miss' );
		}

		wfProfileOut( __METHOD__ );
		return $this->dataLoaded;
	}

	/**
	 * Save the file metadata to memcached
	 */
	function saveToCache() {
		global $wgMemc;
		$this->load();
		$key = $this->getCacheKey();
		if ( !$key ) {
			return;
		}
		$fields = $this->getCacheFields( '' );
		$cache = array( 'version' => MW_FILE_VERSION );
		$cache['fileExists'] = $this->fileExists;
		if ( $this->fileExists ) {
			foreach ( $fields as $field ) {
				$cache[$field] = $this->$field;
			}
		}

		$wgMemc->set( $key, $cache, 60 * 60 * 24 * 7 ); // A week
	}

	/**
	 * Load metadata from the file itself
	 */
	function loadFromFile() {
		$this->setProps( self::getPropsFromPath( $this->getPath() ) );
	}

	function getCacheFields( $prefix = 'img_' ) {
		static $fields = array( 'size', 'width', 'height', 'bits', 'media_type',
			'major_mime', 'minor_mime', 'metadata', 'timestamp', 'sha1', 'user', 'user_text', 'description' );
		static $results = array();
		if ( $prefix == '' ) {
			return $fields;
		}
		if ( !isset( $results[$prefix] ) ) {
			$prefixedFields = array();
			foreach ( $fields as $field ) {
				$prefixedFields[] = $prefix . $field;
			}
			$results[$prefix] = $prefixedFields;
		}
		return $results[$prefix];
	}

	/**
	 * Load file metadata from the DB
	 */
	function loadFromDB() {
		# Polymorphic function name to distinguish foreign and local fetches
		$fname = get_class( $this ) . '::' . __FUNCTION__;
		wfProfileIn( $fname );

		# Unconditionally set loaded=true, we don't want the accessors constantly rechecking
		$this->dataLoaded = true;

		$dbr = $this->repo->getMasterDB();

		$row = $dbr->selectRow( 'image', $this->getCacheFields( 'img_' ),
			array( 'img_name' => $this->getName() ), $fname );
		if ( $row ) {
			$this->loadFromRow( $row );
		} else {
			$this->fileExists = false;
		}

		wfProfileOut( $fname );
	}

	/**
	 * Decode a row from the database (either object or array) to an array
	 * with timestamps and MIME types decoded, and the field prefix removed.
	 */
	function decodeRow( $row, $prefix = 'img_' ) {
		$array = (array)$row;
		$prefixLength = strlen( $prefix );
		// Sanity check prefix once
		if ( substr( key( $array ), 0, $prefixLength ) !== $prefix ) {
			throw new MWException( __METHOD__. ': incorrect $prefix parameter' );
		}
		$decoded = array();
		foreach ( $array as $name => $value ) {
			$decoded[substr( $name, $prefixLength )] = $value;
		}
		$decoded['timestamp'] = wfTimestamp( TS_MW, $decoded['timestamp'] );
		if ( empty( $decoded['major_mime'] ) ) {
			$decoded['mime'] = "unknown/unknown";
		} else {
			if (!$decoded['minor_mime']) {
				$decoded['minor_mime'] = "unknown";
			}
			$decoded['mime'] = $decoded['major_mime'].'/'.$decoded['minor_mime'];
		}
		# Trim zero padding from char/binary field
		$decoded['sha1'] = rtrim( $decoded['sha1'], "\0" );
		return $decoded;
	}

	/*
	 * Load file metadata from a DB result row
	 */
	function loadFromRow( $row, $prefix = 'img_' ) {
		$this->dataLoaded = true;
		$array = $this->decodeRow( $row, $prefix );
		foreach ( $array as $name => $value ) {
			$this->$name = $value;
		}
		$this->fileExists = true;
		$this->maybeUpgradeRow();
	}

	/**
	 * Load file metadata from cache or DB, unless already loaded
	 */
	function load() {
		if ( !$this->dataLoaded ) {
			if ( !$this->loadFromCache() ) {
				$this->loadFromDB();
				$this->saveToCache();
			}
			$this->dataLoaded = true;
		}
	}

	/**
	 * Upgrade a row if it needs it
	 */
	function maybeUpgradeRow() {
		if ( wfReadOnly() ) {
			return;
		}
		if ( is_null($this->media_type) ||
			$this->mime == 'image/svg'
		) {
			$this->upgradeRow();
			$this->upgraded = true;
		} else {
			$handler = $this->getHandler();
			if ( $handler && !$handler->isMetadataValid( $this, $this->metadata ) ) {
				$this->upgradeRow();
				$this->upgraded = true;
			}
		}
	}

	function getUpgraded() {
		return $this->upgraded;
	}

	/**
	 * Fix assorted version-related problems with the image row by reloading it from the file
	 */
	function upgradeRow() {
		wfProfileIn( __METHOD__ );

		$this->loadFromFile();

		# Don't destroy file info of missing files
		if ( !$this->fileExists ) {
			wfDebug( __METHOD__.": file does not exist, aborting\n" );
			return;
		}
		$dbw = $this->repo->getMasterDB();
		list( $major, $minor ) = self::splitMime( $this->mime );

		if ( wfReadOnly() ) {
			return;
		}
		wfDebug(__METHOD__.': upgrading '.$this->getName()." to the current schema\n");

		$dbw->update( 'image',
			array(
				'img_width' => $this->width,
				'img_height' => $this->height,
				'img_bits' => $this->bits,
				'img_media_type' => $this->media_type,
				'img_major_mime' => $major,
				'img_minor_mime' => $minor,
				'img_metadata' => $this->metadata,
				'img_sha1' => $this->sha1,
			), array( 'img_name' => $this->getName() ),
			__METHOD__
		);
		$this->saveToCache();
		wfProfileOut( __METHOD__ );
	}

	/**
	 * Set properties in this object to be equal to those given in the
	 * associative array $info. Only cacheable fields can be set.
	 *
	 * If 'mime' is given, it will be split into major_mime/minor_mime.
	 * If major_mime/minor_mime are given, $this->mime will also be set.
	 */
	function setProps( $info ) {
		$this->dataLoaded = true;
		$fields = $this->getCacheFields( '' );
		$fields[] = 'fileExists';
		foreach ( $fields as $field ) {
			if ( isset( $info[$field] ) ) {
				$this->$field = $info[$field];
			}
		}
		// Fix up mime fields
		if ( isset( $info['major_mime'] ) ) {
			$this->mime = "{$info['major_mime']}/{$info['minor_mime']}";
		} elseif ( isset( $info['mime'] ) ) {
			list( $this->major_mime, $this->minor_mime ) = self::splitMime( $this->mime );
		}
	}

	/** splitMime inherited */
	/** getName inherited */
	/** getTitle inherited */
	/** getURL inherited */
	/** getViewURL inherited */
	/** getPath inherited */
	/** isVisible inhereted */

	/**
	 * Return the width of the image
	 *
	 * Returns false on error
	 * @public
	 */
	function getWidth( $page = 1 ) {
		$this->load();
		if ( $this->isMultipage() ) {
			$dim = $this->getHandler()->getPageDimensions( $this, $page );
			if ( $dim ) {
				return $dim['width'];
			} else {
				return false;
			}
		} else {
			return $this->width;
		}
	}

	/**
	 * Return the height of the image
	 *
	 * Returns false on error
	 * @public
	 */
	function getHeight( $page = 1 ) {
		$this->load();
		if ( $this->isMultipage() ) {
			$dim = $this->getHandler()->getPageDimensions( $this, $page );
			if ( $dim ) {
				return $dim['height'];
			} else {
				return false;
			}
		} else {
			return $this->height;
		}
	}

	/**
	 * Returns ID or name of user who uploaded the file
	 *
	 * @param $type string 'text' or 'id'
	 */
	function getUser($type='text') {
		$this->load();
		if( $type == 'text' ) {
			return $this->user_text;
		} elseif( $type == 'id' ) {
			return $this->user;
		}
	}

	/**
	 * Get handler-specific metadata
	 */
	function getMetadata() {
		$this->load();
		return $this->metadata;
	}

	/**
	 * Return the size of the image file, in bytes
	 * @public
	 */
	function getSize() {
		$this->load();
		return $this->size;
	}

	/**
	 * Returns the mime type of the file.
	 */
	function getMimeType() {
		$this->load();
		return $this->mime;
	}

	/**
	 * Return the type of the media in the file.
	 * Use the value returned by this function with the MEDIATYPE_xxx constants.
	 */
	function getMediaType() {
		$this->load();
		return $this->media_type;
	}

	/** canRender inherited */
	/** mustRender inherited */
	/** allowInlineDisplay inherited */
	/** isSafeFile inherited */
	/** isTrustedFile inherited */

	/**
	 * Returns true if the file file exists on disk.
	 * @return boolean Whether file file exist on disk.
	 * @public
	 */
	function exists() {
		$this->load();
		return $this->fileExists;
	}

	/** getTransformScript inherited */
	/** getUnscaledThumb inherited */
	/** thumbName inherited */
	/** createThumb inherited */
	/** getThumbnail inherited */
	/** transform inherited */

	/**
	 * Fix thumbnail files from 1.4 or before, with extreme prejudice
	 */
	function migrateThumbFile( $thumbName ) {
		$thumbDir = $this->getThumbPath();
		$thumbPath = "$thumbDir/$thumbName";
		if ( is_dir( $thumbPath ) ) {
			// Directory where file should be
			// This happened occasionally due to broken migration code in 1.5
			// Rename to broken-*
			for ( $i = 0; $i < 100 ; $i++ ) {
				$broken = $this->repo->getZonePath('public') . "/broken-$i-$thumbName";
				if ( !file_exists( $broken ) ) {
					rename( $thumbPath, $broken );
					break;
				}
			}
			// Doesn't exist anymore
			clearstatcache();
		}
		if ( is_file( $thumbDir ) ) {
			// File where directory should be
			unlink( $thumbDir );
			// Doesn't exist anymore
			clearstatcache();
		}
	}

	/** getHandler inherited */
	/** iconThumb inherited */
	/** getLastError inherited */

	/**
	 * Get all thumbnail names previously generated for this file
	 */
	function getThumbnails() {
		$this->load();
		$files = array();
		$dir = $this->getThumbPath();

		if ( is_dir( $dir ) ) {
			$handle = opendir( $dir );

			if ( $handle ) {
				while ( false !== ( $file = readdir($handle) ) ) {
					if ( $file{0} != '.' ) {
						$files[] = $file;
					}
				}
				closedir( $handle );
			}
		}

		return $files;
	}

	/**
	 * Refresh metadata in memcached, but don't touch thumbnails or squid
	 */
	function purgeMetadataCache() {
		$this->loadFromDB();
		$this->saveToCache();
		$this->purgeHistory();
	}

	/**
	 * Purge the shared history (OldLocalFile) cache
	 */
	function purgeHistory() {
		global $wgMemc;
		$hashedName = md5($this->getName());
		$oldKey = wfMemcKey( 'oldfile', $hashedName );
		$wgMemc->delete( $oldKey );
	}

	/**
	 * Delete all previously generated thumbnails, refresh metadata in memcached and purge the squid
	 */
	function purgeCache() {
		// Refresh metadata cache
		$this->purgeMetadataCache();

		// Delete thumbnails
		$this->purgeThumbnails();

		// Purge squid cache for this file
		SquidUpdate::purge( array( $this->getURL() ) );
	}

	/**
	 * Delete cached transformed files
	 */
	function purgeThumbnails() {
		global $wgUseSquid;
		// Delete thumbnails
		$files = $this->getThumbnails();
		$dir = $this->getThumbPath();
		$urls = array();
		foreach ( $files as $file ) {
			# Check that the base file name is part of the thumb name
			# This is a basic sanity check to avoid erasing unrelated directories
			if ( strpos( $file, $this->getName() ) !== false ) {
				$url = $this->getThumbUrl( $file );
				$urls[] = $url;
				@unlink( "$dir/$file" );
			}
		}

		// Purge the squid
		if ( $wgUseSquid ) {
			SquidUpdate::purge( $urls );
		}
	}

	/** purgeDescription inherited */
	/** purgeEverything inherited */

	function getHistory($limit = null, $start = null, $end = null) {
		$dbr = $this->repo->getSlaveDB();
		$tables = array('oldimage');
		$join_conds = array();
		$fields = OldLocalFile::selectFields();
		$conds = $opts = array();
		$conds[] = "oi_name = " . $dbr->addQuotes( $this->title->getDBKey() );
		if( $start !== null ) {
			$conds[] = "oi_timestamp <= " . $dbr->addQuotes( $dbr->timestamp( $start ) );
		}
		if( $end !== null ) {
			$conds[] = "oi_timestamp >= " . $dbr->addQuotes( $dbr->timestamp( $end ) );
		}
		if( $limit ) {
			$opts['LIMIT'] = $limit;
		}
		$opts['ORDER BY'] = 'oi_timestamp DESC';
		
		wfRunHooks( 'LocalFile::getHistory', array( &$this, &$tables, &$fields, &$conds, &$opts, &$join_conds ) );
		
		$res = $dbr->select( $tables, $fields, $conds, __METHOD__, $opts, $join_conds );
		$r = array();
		while( $row = $dbr->fetchObject($res) ) {
			$r[] = OldLocalFile::newFromRow($row, $this->repo);
		}
		return $r;
	}

	/**
	 * Return the history of this file, line by line.
	 * starts with current version, then old versions.
	 * uses $this->historyLine to check which line to return:
	 *  0      return line for current version
	 *  1      query for old versions, return first one
	 *  2, ... return next old version from above query
	 *
	 * @public
	 */
	function nextHistoryLine() {
		# Polymorphic function name to distinguish foreign and local fetches
		$fname = get_class( $this ) . '::' . __FUNCTION__;

		$dbr = $this->repo->getSlaveDB();

		if ( $this->historyLine == 0 ) {// called for the first time, return line from cur
			$this->historyRes = $dbr->select( 'image',
				array(
					'*',
					"'' AS oi_archive_name",
					'0 as oi_deleted',
					'img_sha1'
				),
				array( 'img_name' => $this->title->getDBkey() ),
				$fname
			);
			if ( 0 == $dbr->numRows( $this->historyRes ) ) {
				$dbr->freeResult($this->historyRes);
				$this->historyRes = null;
				return FALSE;
			}
		} else if ( $this->historyLine == 1 ) {
			$dbr->freeResult($this->historyRes);
			$this->historyRes = $dbr->select( 'oldimage', '*',
				array( 'oi_name' => $this->title->getDBkey() ),
				$fname,
				array( 'ORDER BY' => 'oi_timestamp DESC' )
			);
		}
		$this->historyLine ++;

		return $dbr->fetchObject( $this->historyRes );
	}

	/**
	 * Reset the history pointer to the first element of the history
	 * @public
	 */
	function resetHistory() {
		$this->historyLine = 0;
		if (!is_null($this->historyRes)) {
			$this->repo->getSlaveDB()->freeResult($this->historyRes);
			$this->historyRes = null;
		}
	}

	/** getFullPath inherited */
	/** getHashPath inherited */
	/** getRel inherited */
	/** getUrlRel inherited */
	/** getArchiveRel inherited */
	/** getThumbRel inherited */
	/** getArchivePath inherited */
	/** getThumbPath inherited */
	/** getArchiveUrl inherited */
	/** getThumbUrl inherited */
	/** getArchiveVirtualUrl inherited */
	/** getThumbVirtualUrl inherited */
	/** isHashed inherited */

	/**
	 * Upload a file and record it in the DB
	 * @param string $srcPath Source path or virtual URL
	 * @param string $comment Upload description
	 * @param string $pageText Text to use for the new description page, if a new description page is created
	 * @param integer $flags Flags for publish()
	 * @param array $props File properties, if known. This can be used to reduce the
	 *                         upload time when uploading virtual URLs for which the file info
	 *                         is already known
	 * @param string $timestamp Timestamp for img_timestamp, or false to use the current time
	 *
	 * @return FileRepoStatus object. On success, the value member contains the
	 *     archive name, or an empty string if it was a new file.
	 */
	function upload( $srcPath, $comment, $pageText, $flags = 0, $props = false, $timestamp = false ) {
		$this->lock();
		$status = $this->publish( $srcPath, $flags );
		if ( $status->ok ) {
			if ( !$this->recordUpload2( $status->value, $comment, $pageText, $props, $timestamp ) ) {
				$status->fatal( 'filenotfound', $srcPath );
			}
		}
		$this->unlock();
		return $status;
	}

	/**
	 * Record a file upload in the upload log and the image table
	 * @deprecated use upload()
	 */
	function recordUpload( $oldver, $desc, $license = '', $copyStatus = '', $source = '',
		$watch = false, $timestamp = false )
	{
		$pageText = UploadForm::getInitialPageText( $desc, $license, $copyStatus, $source );
		if ( !$this->recordUpload2( $oldver, $desc, $pageText ) ) {
			return false;
		}
		if ( $watch ) {
			global $wgUser;
			$wgUser->addWatch( $this->getTitle() );
		}
		return true;

	}

	/**
	 * Record a file upload in the upload log and the image table
	 */
	function recordUpload2( $oldver, $comment, $pageText, $props = false, $timestamp = false )
	{
		global $wgUser;

		$dbw = $this->repo->getMasterDB();

		if ( !$props ) {
			$props = $this->repo->getFileProps( $this->getVirtualUrl() );
		}
		$props['description'] = $comment;
		$props['user'] = $wgUser->getId();
		$props['user_text'] = $wgUser->getName();
		$props['timestamp'] = wfTimestamp( TS_MW );
		$this->setProps( $props );

		// Delete thumbnails and refresh the metadata cache
		$this->purgeThumbnails();
		$this->saveToCache();
		SquidUpdate::purge( array( $this->getURL() ) );

		// Fail now if the file isn't there
		if ( !$this->fileExists ) {
			wfDebug( __METHOD__.": File ".$this->getPath()." went missing!\n" );
			return false;
		}

		$reupload = false;
		if ( $timestamp === false ) {
			$timestamp = $dbw->timestamp();
		}

		# Test to see if the row exists using INSERT IGNORE
		# This avoids race conditions by locking the row until the commit, and also
		# doesn't deadlock. SELECT FOR UPDATE causes a deadlock for every race condition.
		$dbw->insert( 'image',
			array(
				'img_name' => $this->getName(),
				'img_size'=> $this->size,
				'img_width' => intval( $this->width ),
				'img_height' => intval( $this->height ),
				'img_bits' => $this->bits,
				'img_media_type' => $this->media_type,
				'img_major_mime' => $this->major_mime,
				'img_minor_mime' => $this->minor_mime,
				'img_timestamp' => $timestamp,
				'img_description' => $comment,
				'img_user' => $wgUser->getId(),
				'img_user_text' => $wgUser->getName(),
				'img_metadata' => $this->metadata,
				'img_sha1' => $this->sha1
			),
			__METHOD__,
			'IGNORE'
		);

		if( $dbw->affectedRows() == 0 ) {
			$reupload = true;

			# Collision, this is an update of a file
			# Insert previous contents into oldimage
			$dbw->insertSelect( 'oldimage', 'image',
				array(
					'oi_name' => 'img_name',
					'oi_archive_name' => $dbw->addQuotes( $oldver ),
					'oi_size' => 'img_size',
					'oi_width' => 'img_width',
					'oi_height' => 'img_height',
					'oi_bits' => 'img_bits',
					'oi_timestamp' => 'img_timestamp',
					'oi_description' => 'img_description',
					'oi_user' => 'img_user',
					'oi_user_text' => 'img_user_text',
					'oi_metadata' => 'img_metadata',
					'oi_media_type' => 'img_media_type',
					'oi_major_mime' => 'img_major_mime',
					'oi_minor_mime' => 'img_minor_mime',
					'oi_sha1' => 'img_sha1'
				), array( 'img_name' => $this->getName() ), __METHOD__
			);

			# Update the current image row
			$dbw->update( 'image',
				array( /* SET */
					'img_size' => $this->size,
					'img_width' => intval( $this->width ),
					'img_height' => intval( $this->height ),
					'img_bits' => $this->bits,
					'img_media_type' => $this->media_type,
					'img_major_mime' => $this->major_mime,
					'img_minor_mime' => $this->minor_mime,
					'img_timestamp' => $timestamp,
					'img_description' => $comment,
					'img_user' => $wgUser->getId(),
					'img_user_text' => $wgUser->getName(),
					'img_metadata' => $this->metadata,
					'img_sha1' => $this->sha1
				), array( /* WHERE */
					'img_name' => $this->getName()
				), __METHOD__
			);
		} else {
			# This is a new file
			# Update the image count
			$site_stats = $dbw->tableName( 'site_stats' );
			$dbw->query( "UPDATE $site_stats SET ss_images=ss_images+1", __METHOD__ );
		}

		$descTitle = $this->getTitle();
		$article = new Article( $descTitle );

		# Add the log entry
		$log = new LogPage( 'upload' );
		$action = $reupload ? 'overwrite' : 'upload';
		$log->addEntry( $action, $descTitle, $comment );

		if( $descTitle->exists() ) {
			# Create a null revision
			$nullRevision = Revision::newNullRevision( $dbw, $descTitle->getArticleId(), $log->getRcComment(), false );
			$nullRevision->insertOn( $dbw );
			
			wfRunHooks( 'NewRevisionFromEditComplete', array($article, $nullRevision, false) );
			$article->updateRevisionOn( $dbw, $nullRevision );

			# Invalidate the cache for the description page
			$descTitle->invalidateCache();
			$descTitle->purgeSquid();
		} else {
			// New file; create the description page.
			// There's already a log entry, so don't make a second RC entry
			$article->doEdit( $pageText, $comment, EDIT_NEW | EDIT_SUPPRESS_RC );
		}

		# Hooks, hooks, the magic of hooks...
		wfRunHooks( 'FileUpload', array( $this ) );

		# Commit the transaction now, in case something goes wrong later
		# The most important thing is that files don't get lost, especially archives
		$dbw->immediateCommit();

		# Invalidate cache for all pages using this file
		$update = new HTMLCacheUpdate( $this->getTitle(), 'imagelinks' );
		$update->doUpdate();
		# Invalidate cache for all pages that redirects on this page
		$redirs = $this->getTitle()->getRedirectsHere();
		foreach( $redirs as $redir ) {
			$update = new HTMLCacheUpdate( $redir, 'imagelinks' );
			$update->doUpdate();
		}

		return true;
	}

	/**
	 * Move or copy a file to its public location. If a file exists at the
	 * destination, move it to an archive. Returns the archive name on success
	 * or an empty string if it was a new file, and a wikitext-formatted
	 * WikiError object on failure.
	 *
	 * The archive name should be passed through to recordUpload for database
	 * registration.
	 *
	 * @param string $sourcePath Local filesystem path to the source image
	 * @param integer $flags A bitwise combination of:
	 *     File::DELETE_SOURCE    Delete the source file, i.e. move
	 *         rather than copy
	 * @return FileRepoStatus object. On success, the value member contains the
	 *     archive name, or an empty string if it was a new file.
	 */
	function publish( $srcPath, $flags = 0 ) {
		$this->lock();
		$dstRel = $this->getRel();
		$archiveName = gmdate( 'YmdHis' ) . '!'. $this->getName();
		$archiveRel = 'archive/' . $this->getHashPath() . $archiveName;
		$flags = $flags & File::DELETE_SOURCE ? LocalRepo::DELETE_SOURCE : 0;
		$status = $this->repo->publish( $srcPath, $dstRel, $archiveRel, $flags );
		if ( $status->value == 'new' ) {
			$status->value = '';
		} else {
			$status->value = $archiveName;
		}
		$this->unlock();
		return $status;
	}

	/** getLinksTo inherited */
	/** getExifData inherited */
	/** isLocal inherited */
	/** wasDeleted inherited */

	/**
	 * Move file to the new title
	 *
	 * Move current, old version and all thumbnails
	 * to the new filename. Old file is deleted.
	 *
	 * Cache purging is done; checks for validity
	 * and logging are caller's responsibility
	 *
	 * @param $target Title New file name
	 * @return FileRepoStatus object.
	 */
	function move( $target ) {
		wfDebugLog( 'imagemove', "Got request to move {$this->name} to " . $target->getText() );
		$this->lock();
		$batch = new LocalFileMoveBatch( $this, $target );
		$batch->addCurrent();
		$batch->addOlds();

		$status = $batch->execute();
		wfDebugLog( 'imagemove', "Finished moving {$this->name}" );
		$this->purgeEverything();
		$this->unlock();

		if ( $status->isOk() ) {
			// Now switch the object
			$this->title = $target;
			// Force regeneration of the name and hashpath
			unset( $this->name );
			unset( $this->hashPath );
			// Purge the new image
			$this->purgeEverything();
		}
		
		return $status;
	}

	/**
	 * Delete all versions of the file.
	 *
	 * Moves the files into an archive directory (or deletes them)
	 * and removes the database rows.
	 *
	 * Cache purging is done; logging is caller's responsibility.
	 *
	 * @param $reason
	 * @param $suppress
	 * @return FileRepoStatus object.
	 */
	function delete( $reason, $suppress = false ) {
		$this->lock();
		$batch = new LocalFileDeleteBatch( $this, $reason, $suppress );
		$batch->addCurrent();

		# Get old version relative paths
		$dbw = $this->repo->getMasterDB();
		$result = $dbw->select( 'oldimage',
			array( 'oi_archive_name' ),
			array( 'oi_name' => $this->getName() ) );
		while ( $row = $dbw->fetchObject( $result ) ) {
			$batch->addOld( $row->oi_archive_name );
		}
		$status = $batch->execute();

		if ( $status->ok ) {
			// Update site_stats
			$site_stats = $dbw->tableName( 'site_stats' );
			$dbw->query( "UPDATE $site_stats SET ss_images=ss_images-1", __METHOD__ );
			$this->purgeEverything();
		}

		$this->unlock();
		return $status;
	}

	/**
	 * Delete an old version of the file.
	 *
	 * Moves the file into an archive directory (or deletes it)
	 * and removes the database row.
	 *
	 * Cache purging is done; logging is caller's responsibility.
	 *
	 * @param $reason
	 * @param $suppress
	 * @throws MWException or FSException on database or filestore failure
	 * @return FileRepoStatus object.
	 */
	function deleteOld( $archiveName, $reason, $suppress=false ) {
		$this->lock();
		$batch = new LocalFileDeleteBatch( $this, $reason, $suppress );
		$batch->addOld( $archiveName );
		$status = $batch->execute();
		$this->unlock();
		if ( $status->ok ) {
			$this->purgeDescription();
			$this->purgeHistory();
		}
		return $status;
	}

	/**
	 * Restore all or specified deleted revisions to the given file.
	 * Permissions and logging are left to the caller.
	 *
	 * May throw database exceptions on error.
	 *
	 * @param $versions set of record ids of deleted items to restore,
	 *                    or empty to restore all revisions.
	 * @param $unuppress
	 * @return FileRepoStatus
	 */
	function restore( $versions = array(), $unsuppress = false ) {
		$batch = new LocalFileRestoreBatch( $this, $unsuppress );
		if ( !$versions ) {
			$batch->addAll();
		} else {
			$batch->addIds( $versions );
		}
		$status = $batch->execute();
		if ( !$status->ok ) {
			return $status;
		}

		$cleanupStatus = $batch->cleanup();
		$cleanupStatus->successCount = 0;
		$cleanupStatus->failCount = 0;
		$status->merge( $cleanupStatus );
		return $status;
	}

	/** isMultipage inherited */
	/** pageCount inherited */
	/** scaleHeight inherited */
	/** getImageSize inherited */

	/**
	 * Get the URL of the file description page.
	 */
	function getDescriptionUrl() {
		return $this->title->getLocalUrl();
	}

	/**
	 * Get the HTML text of the description page
	 * This is not used by ImagePage for local files, since (among other things)
	 * it skips the parser cache.
	 */
	function getDescriptionText() {
		global $wgParser;
		$revision = Revision::newFromTitle( $this->title );
		if ( !$revision ) return false;
		$text = $revision->getText();
		if ( !$text ) return false;
		$html = $wgParser->parse( $text, new ParserOptions );
		return $html;
	}

	function getDescription() {
		$this->load();
		return $this->description;
	}

	function getTimestamp() {
		$this->load();
		return $this->timestamp;
	}

	function getSha1() {
		$this->load();
		// Initialise now if necessary
		if ( $this->sha1 == '' && $this->fileExists ) {
			$this->sha1 = File::sha1Base36( $this->getPath() );
			if ( strval( $this->sha1 ) != '' ) {
				$dbw = $this->repo->getMasterDB();
				$dbw->update( 'image',
					array( 'img_sha1' => $this->sha1 ),
					array( 'img_name' => $this->getName() ),
					__METHOD__ );
				$this->saveToCache();
			}
		}

		return $this->sha1;
	}

	/**
	 * Start a transaction and lock the image for update
	 * Increments a reference counter if the lock is already held
	 * @return boolean True if the image exists, false otherwise
	 */
	function lock() {
		$dbw = $this->repo->getMasterDB();
		if ( !$this->locked ) {
			$dbw->begin();
			$this->locked++;
		}
		return $dbw->selectField( 'image', '1', array( 'img_name' => $this->getName() ), __METHOD__ );
	}

	/**
	 * Decrement the lock reference count. If the reference count is reduced to zero, commits
	 * the transaction and thereby releases the image lock.
	 */
	function unlock() {
		if ( $this->locked ) {
			--$this->locked;
			if ( !$this->locked ) {
				$dbw = $this->repo->getMasterDB();
				$dbw->commit();
			}
		}
	}

	/**
	 * Roll back the DB transaction and mark the image unlocked
	 */
	function unlockAndRollback() {
		$this->locked = false;
		$dbw = $this->repo->getMasterDB();
		$dbw->rollback();
	}
} // LocalFile class

#------------------------------------------------------------------------------

/**
 * Helper class for file deletion
 * @ingroup FileRepo
 */
class LocalFileDeleteBatch {
	var $file, $reason, $srcRels = array(), $archiveUrls = array(), $deletionBatch, $suppress;
	var $status;

	function __construct( File $file, $reason = '', $suppress = false ) {
		$this->file = $file;
		$this->reason = $reason;
		$this->suppress = $suppress;
		$this->status = $file->repo->newGood();
	}

	function addCurrent() {
		$this->srcRels['.'] = $this->file->getRel();
	}

	function addOld( $oldName ) {
		$this->srcRels[$oldName] = $this->file->getArchiveRel( $oldName );
		$this->archiveUrls[] = $this->file->getArchiveUrl( $oldName );
	}

	function getOldRels() {
		if ( !isset( $this->srcRels['.'] ) ) {
			$oldRels =& $this->srcRels;
			$deleteCurrent = false;
		} else {
			$oldRels = $this->srcRels;
			unset( $oldRels['.'] );
			$deleteCurrent = true;
		}
		return array( $oldRels, $deleteCurrent );
	}

	/*protected*/ function getHashes() {
		$hashes = array();
		list( $oldRels, $deleteCurrent ) = $this->getOldRels();
		if ( $deleteCurrent ) {
			$hashes['.'] = $this->file->getSha1();
		}
		if ( count( $oldRels ) ) {
			$dbw = $this->file->repo->getMasterDB();
			$res = $dbw->select( 'oldimage', array( 'oi_archive_name', 'oi_sha1' ),
				'oi_archive_name IN(' . $dbw->makeList( array_keys( $oldRels ) ) . ')',
				__METHOD__ );
			while ( $row = $dbw->fetchObject( $res ) ) {
				if ( rtrim( $row->oi_sha1, "\0" ) === '' ) {
					// Get the hash from the file
					$oldUrl = $this->file->getArchiveVirtualUrl( $row->oi_archive_name );
					$props = $this->file->repo->getFileProps( $oldUrl );
					if ( $props['fileExists'] ) {
						// Upgrade the oldimage row
						$dbw->update( 'oldimage',
							array( 'oi_sha1' => $props['sha1'] ),
							array( 'oi_name' => $this->file->getName(), 'oi_archive_name' => $row->oi_archive_name ),
							__METHOD__ );
						$hashes[$row->oi_archive_name] = $props['sha1'];
					} else {
						$hashes[$row->oi_archive_name] = false;
					}
				} else {
					$hashes[$row->oi_archive_name] = $row->oi_sha1;
				}
			}
		}
		$missing = array_diff_key( $this->srcRels, $hashes );
		foreach ( $missing as $name => $rel ) {
			$this->status->error( 'filedelete-old-unregistered', $name );
		}
		foreach ( $hashes as $name => $hash ) {
			if ( !$hash ) {
				$this->status->error( 'filedelete-missing', $this->srcRels[$name] );
				unset( $hashes[$name] );
			}
		}

		return $hashes;
	}

	function doDBInserts() {
		global $wgUser;
		$dbw = $this->file->repo->getMasterDB();
		$encTimestamp = $dbw->addQuotes( $dbw->timestamp() );
		$encUserId = $dbw->addQuotes( $wgUser->getId() );
		$encReason = $dbw->addQuotes( $this->reason );
		$encGroup = $dbw->addQuotes( 'deleted' );
		$ext = $this->file->getExtension();
		$dotExt = $ext === '' ? '' : ".$ext";
		$encExt = $dbw->addQuotes( $dotExt );
		list( $oldRels, $deleteCurrent ) = $this->getOldRels();

		// Bitfields to further suppress the content
		if ( $this->suppress ) {
			$bitfield = 0;
			// This should be 15...
			$bitfield |= Revision::DELETED_TEXT;
			$bitfield |= Revision::DELETED_COMMENT;
			$bitfield |= Revision::DELETED_USER;
			$bitfield |= Revision::DELETED_RESTRICTED;
		} else {
			$bitfield = 'oi_deleted';
		}

		if ( $deleteCurrent ) {
			$concat = $dbw->buildConcat( array( "img_sha1", $encExt ) );
			$where = array( 'img_name' => $this->file->getName() );
			$dbw->insertSelect( 'filearchive', 'image',
				array(
					'fa_storage_group' => $encGroup,
					'fa_storage_key'   => "CASE WHEN img_sha1='' THEN '' ELSE $concat END",
					'fa_deleted_user'      => $encUserId,
					'fa_deleted_timestamp' => $encTimestamp,
					'fa_deleted_reason'    => $encReason,
					'fa_deleted'		   => $this->suppress ? $bitfield : 0,

					'fa_name'         => 'img_name',
					'fa_archive_name' => 'NULL',
					'fa_size'         => 'img_size',
					'fa_width'        => 'img_width',
					'fa_height'       => 'img_height',
					'fa_metadata'     => 'img_metadata',
					'fa_bits'         => 'img_bits',
					'fa_media_type'   => 'img_media_type',
					'fa_major_mime'   => 'img_major_mime',
					'fa_minor_mime'   => 'img_minor_mime',
					'fa_description'  => 'img_description',
					'fa_user'         => 'img_user',
					'fa_user_text'    => 'img_user_text',
					'fa_timestamp'    => 'img_timestamp'
				), $where, __METHOD__ );
		}

		if ( count( $oldRels ) ) {
			$concat = $dbw->buildConcat( array( "oi_sha1", $encExt ) );
			$where = array(
				'oi_name' => $this->file->getName(),
				'oi_archive_name IN (' . $dbw->makeList( array_keys( $oldRels ) ) . ')' );
			$dbw->insertSelect( 'filearchive', 'oldimage',
				array(
					'fa_storage_group' => $encGroup,
					'fa_storage_key'   => "CASE WHEN oi_sha1='' THEN '' ELSE $concat END",
					'fa_deleted_user'      => $encUserId,
					'fa_deleted_timestamp' => $encTimestamp,
					'fa_deleted_reason'    => $encReason,
					'fa_deleted'		   => $this->suppress ? $bitfield : 'oi_deleted',

					'fa_name'         => 'oi_name',
					'fa_archive_name' => 'oi_archive_name',
					'fa_size'         => 'oi_size',
					'fa_width'        => 'oi_width',
					'fa_height'       => 'oi_height',
					'fa_metadata'     => 'oi_metadata',
					'fa_bits'         => 'oi_bits',
					'fa_media_type'   => 'oi_media_type',
					'fa_major_mime'   => 'oi_major_mime',
					'fa_minor_mime'   => 'oi_minor_mime',
					'fa_description'  => 'oi_description',
					'fa_user'         => 'oi_user',
					'fa_user_text'    => 'oi_user_text',
					'fa_timestamp'    => 'oi_timestamp',
					'fa_deleted'      => $bitfield
				), $where, __METHOD__ );
		}
	}

	function doDBDeletes() {
		$dbw = $this->file->repo->getMasterDB();
		list( $oldRels, $deleteCurrent ) = $this->getOldRels();
		if ( count( $oldRels ) ) {
			$dbw->delete( 'oldimage',
				array(
					'oi_name' => $this->file->getName(),
					'oi_archive_name IN (' . $dbw->makeList( array_keys( $oldRels ) ) . ')'
				), __METHOD__ );
		}
		if ( $deleteCurrent ) {
			$dbw->delete( 'image', array( 'img_name' => $this->file->getName() ), __METHOD__ );
		}
	}

	/**
	 * Run the transaction
	 */
	function execute() {
		global $wgUser, $wgUseSquid;
		wfProfileIn( __METHOD__ );

		$this->file->lock();
		// Leave private files alone
		$privateFiles = array();
		list( $oldRels, $deleteCurrent ) = $this->getOldRels();
		$dbw = $this->file->repo->getMasterDB();
		if( !empty( $oldRels ) ) {
			$res = $dbw->select( 'oldimage',
				array( 'oi_archive_name' ),
				array( 'oi_name' => $this->file->getName(),
					'oi_archive_name IN (' . $dbw->makeList( array_keys($oldRels) ) . ')',
					'oi_deleted & ' . File::DELETED_FILE => File::DELETED_FILE ),
				__METHOD__ );
			while( $row = $dbw->fetchObject( $res ) ) {
				$privateFiles[$row->oi_archive_name] = 1;
			}
		}
		// Prepare deletion batch
		$hashes = $this->getHashes();
		$this->deletionBatch = array();
		$ext = $this->file->getExtension();
		$dotExt = $ext === '' ? '' : ".$ext";
		foreach ( $this->srcRels as $name => $srcRel ) {
			// Skip files that have no hash (missing source).
			// Keep private files where they are.
			if ( isset($hashes[$name]) && !array_key_exists($name,$privateFiles) ) {
				$hash = $hashes[$name];
				$key = $hash . $dotExt;
				$dstRel = $this->file->repo->getDeletedHashPath( $key ) . $key;
				$this->deletionBatch[$name] = array( $srcRel, $dstRel );
			}
		}

		// Lock the filearchive rows so that the files don't get deleted by a cleanup operation
		// We acquire this lock by running the inserts now, before the file operations.
		//
		// This potentially has poor lock contention characteristics -- an alternative
		// scheme would be to insert stub filearchive entries with no fa_name and commit
		// them in a separate transaction, then run the file ops, then update the fa_name fields.
		$this->doDBInserts();

		// Execute the file deletion batch
		$status = $this->file->repo->deleteBatch( $this->deletionBatch );
		if ( !$status->isGood() ) {
			$this->status->merge( $status );
		}

		if ( !$this->status->ok ) {
			// Critical file deletion error
			// Roll back inserts, release lock and abort
			// TODO: delete the defunct filearchive rows if we are using a non-transactional DB
			$this->file->unlockAndRollback();
			return $this->status;
		}

		// Purge squid
		if ( $wgUseSquid ) {
			$urls = array();
			foreach ( $this->srcRels as $srcRel ) {
				$urlRel = str_replace( '%2F', '/', rawurlencode( $srcRel ) );
				$urls[] = $this->file->repo->getZoneUrl( 'public' ) . '/' . $urlRel;
			}
			SquidUpdate::purge( $urls );
		}

		// Delete image/oldimage rows
		$this->doDBDeletes();

		// Commit and return
		$this->file->unlock();
		wfProfileOut( __METHOD__ );
		return $this->status;
	}
}

#------------------------------------------------------------------------------

/**
 * Helper class for file undeletion
 * @ingroup FileRepo
 */
class LocalFileRestoreBatch {
	var $file, $cleanupBatch, $ids, $all, $unsuppress = false;

	function __construct( File $file, $unsuppress = false ) {
		$this->file = $file;
		$this->cleanupBatch = $this->ids = array();
		$this->ids = array();
		$this->unsuppress = $unsuppress;
	}

	/**
	 * Add a file by ID
	 */
	function addId( $fa_id ) {
		$this->ids[] = $fa_id;
	}

	/**
	 * Add a whole lot of files by ID
	 */
	function addIds( $ids ) {
		$this->ids = array_merge( $this->ids, $ids );
	}

	/**
	 * Add all revisions of the file
	 */
	function addAll() {
		$this->all = true;
	}

	/**
	 * Run the transaction, except the cleanup batch.
	 * The cleanup batch should be run in a separate transaction, because it locks different
	 * rows and there's no need to keep the image row locked while it's acquiring those locks
	 * The caller may have its own transaction open.
	 * So we save the batch and let the caller call cleanup()
	 */
	function execute() {
		global $wgUser, $wgLang;
		if ( !$this->all && !$this->ids ) {
			// Do nothing
			return $this->file->repo->newGood();
		}

		$exists = $this->file->lock();
		$dbw = $this->file->repo->getMasterDB();
		$status = $this->file->repo->newGood();

		// Fetch all or selected archived revisions for the file,
		// sorted from the most recent to the oldest.
		$conditions = array( 'fa_name' => $this->file->getName() );
		if( !$this->all ) {
			$conditions[] = 'fa_id IN (' . $dbw->makeList( $this->ids ) . ')';
		}

		$result = $dbw->select( 'filearchive', '*',
			$conditions,
			__METHOD__,
			array( 'ORDER BY' => 'fa_timestamp DESC' ) );

		$idsPresent = array();
		$storeBatch = array();
		$insertBatch = array();
		$insertCurrent = false;
		$deleteIds = array();
		$first = true;
		$archiveNames = array();
		while( $row = $dbw->fetchObject( $result ) ) {
			$idsPresent[] = $row->fa_id;

			if ( $row->fa_name != $this->file->getName() ) {
				$status->error( 'undelete-filename-mismatch', $wgLang->timeanddate( $row->fa_timestamp ) );
				$status->failCount++;
				continue;
			}
			if ( $row->fa_storage_key == '' ) {
				// Revision was missing pre-deletion
				$status->error( 'undelete-bad-store-key', $wgLang->timeanddate( $row->fa_timestamp ) );
				$status->failCount++;
				continue;
			}

			$deletedRel = $this->file->repo->getDeletedHashPath( $row->fa_storage_key ) . $row->fa_storage_key;
			$deletedUrl = $this->file->repo->getVirtualUrl() . '/deleted/' . $deletedRel;

			$sha1 = substr( $row->fa_storage_key, 0, strcspn( $row->fa_storage_key, '.' ) );
			# Fix leading zero
			if ( strlen( $sha1 ) == 32 && $sha1[0] == '0' ) {
				$sha1 = substr( $sha1, 1 );
			}

			if( is_null( $row->fa_major_mime ) || $row->fa_major_mime == 'unknown'
				|| is_null( $row->fa_minor_mime ) || $row->fa_minor_mime == 'unknown'
				|| is_null( $row->fa_media_type ) || $row->fa_media_type == 'UNKNOWN'
				|| is_null( $row->fa_metadata ) ) {
				// Refresh our metadata
				// Required for a new current revision; nice for older ones too. :)
				$props = RepoGroup::singleton()->getFileProps( $deletedUrl );
			} else {
				$props = array(
					'minor_mime' => $row->fa_minor_mime,
					'major_mime' => $row->fa_major_mime,
					'media_type' => $row->fa_media_type,
					'metadata' => $row->fa_metadata );
			}

			if ( $first && !$exists ) {
				// The live (current) version cannot be hidden!
				if( !$this->unsuppress && $row->fa_deleted ) {
					$this->file->unlock();
					return $status;
				}
				// This revision will be published as the new current version
				$destRel = $this->file->getRel();
				$insertCurrent = array(
					'img_name'        => $row->fa_name,
					'img_size'        => $row->fa_size,
					'img_width'       => $row->fa_width,
					'img_height'      => $row->fa_height,
					'img_metadata'    => $props['metadata'],
					'img_bits'        => $row->fa_bits,
					'img_media_type'  => $props['media_type'],
					'img_major_mime'  => $props['major_mime'],
					'img_minor_mime'  => $props['minor_mime'],
					'img_description' => $row->fa_description,
					'img_user'        => $row->fa_user,
					'img_user_text'   => $row->fa_user_text,
					'img_timestamp'   => $row->fa_timestamp,
					'img_sha1'        => $sha1);
			} else {
				$archiveName = $row->fa_archive_name;
				if( $archiveName == '' ) {
					// This was originally a current version; we
					// have to devise a new archive name for it.
					// Format is <timestamp of archiving>!<name>
					$timestamp = wfTimestamp( TS_UNIX, $row->fa_deleted_timestamp );
					do {
						$archiveName = wfTimestamp( TS_MW, $timestamp ) . '!' . $row->fa_name;
						$timestamp++;
					} while ( isset( $archiveNames[$archiveName] ) );
				}
				$archiveNames[$archiveName] = true;
				$destRel = $this->file->getArchiveRel( $archiveName );
				$insertBatch[] = array(
					'oi_name'         => $row->fa_name,
					'oi_archive_name' => $archiveName,
					'oi_size'         => $row->fa_size,
					'oi_width'        => $row->fa_width,
					'oi_height'       => $row->fa_height,
					'oi_bits'         => $row->fa_bits,
					'oi_description'  => $row->fa_description,
					'oi_user'         => $row->fa_user,
					'oi_user_text'    => $row->fa_user_text,
					'oi_timestamp'    => $row->fa_timestamp,
					'oi_metadata'     => $props['metadata'],
					'oi_media_type'   => $props['media_type'],
					'oi_major_mime'   => $props['major_mime'],
					'oi_minor_mime'   => $props['minor_mime'],
					'oi_deleted'      => $this->unsuppress ? 0 : $row->fa_deleted,
					'oi_sha1'         => $sha1 );
			}

			$deleteIds[] = $row->fa_id;
			if( !$this->unsuppress && $row->fa_deleted & File::DELETED_FILE ) {
				// private files can stay where they are
			} else {
				$storeBatch[] = array( $deletedUrl, 'public', $destRel );
				$this->cleanupBatch[] = $row->fa_storage_key;
			}
			$first = false;
		}
		unset( $result );

		// Add a warning to the status object for missing IDs
		$missingIds = array_diff( $this->ids, $idsPresent );
		foreach ( $missingIds as $id ) {
			$status->error( 'undelete-missing-filearchive', $id );
		}

		// Run the store batch
		// Use the OVERWRITE_SAME flag to smooth over a common error
		$storeStatus = $this->file->repo->storeBatch( $storeBatch, FileRepo::OVERWRITE_SAME );
		$status->merge( $storeStatus );

		if ( !$status->ok ) {
			// Store batch returned a critical error -- this usually means nothing was stored
			// Stop now and return an error
			$this->file->unlock();
			return $status;
		}

		// Run the DB updates
		// Because we have locked the image row, key conflicts should be rare.
		// If they do occur, we can roll back the transaction at this time with
		// no data loss, but leaving unregistered files scattered throughout the
		// public zone.
		// This is not ideal, which is why it's important to lock the image row.
		if ( $insertCurrent ) {
			$dbw->insert( 'image', $insertCurrent, __METHOD__ );
		}
		if ( $insertBatch ) {
			$dbw->insert( 'oldimage', $insertBatch, __METHOD__ );
		}
		if ( $deleteIds ) {
			$dbw->delete( 'filearchive',
				array( 'fa_id IN (' . $dbw->makeList( $deleteIds ) . ')' ),
				__METHOD__ );
		}

		if( $status->successCount > 0 ) {
			if( !$exists ) {
				wfDebug( __METHOD__." restored {$status->successCount} items, creating a new current\n" );

				// Update site_stats
				$site_stats = $dbw->tableName( 'site_stats' );
				$dbw->query( "UPDATE $site_stats SET ss_images=ss_images+1", __METHOD__ );

				$this->file->purgeEverything();
			} else {
				wfDebug( __METHOD__." restored {$status->successCount} as archived versions\n" );
				$this->file->purgeDescription();
				$this->file->purgeHistory();
			}
		}
		$this->file->unlock();
		return $status;
	}

	/**
	 * Delete unused files in the deleted zone.
	 * This should be called from outside the transaction in which execute() was called.
	 */
	function cleanup() {
		if ( !$this->cleanupBatch ) {
			return $this->file->repo->newGood();
		}
		$status = $this->file->repo->cleanupDeletedBatch( $this->cleanupBatch );
		return $status;
	}
}

#------------------------------------------------------------------------------

/**
 * Helper class for file movement
 * @ingroup FileRepo
 */
class LocalFileMoveBatch {
	var $file, $cur, $olds, $oldCount, $archive, $target, $db;

	function __construct( File $file, Title $target ) {
		$this->file = $file;
		$this->target = $target;
		$this->oldHash = $this->file->repo->getHashPath( $this->file->getName() );
		$this->newHash = $this->file->repo->getHashPath( $this->target->getDbKey() );
		$this->oldName = $this->file->getName();
		$this->newName = $this->file->repo->getNameFromTitle( $this->target );
		$this->oldRel = $this->oldHash . $this->oldName;
		$this->newRel = $this->newHash . $this->newName;
		$this->db = $file->repo->getMasterDb();
	}

	/*
	 * Add the current image to the batch
	 */
	function addCurrent() {
		$this->cur = array( $this->oldRel, $this->newRel );
	}

	/*
	 * Add the old versions of the image to the batch
	 */
	function addOlds() {
		$archiveBase = 'archive';
		$this->olds = array();
		$this->oldCount = 0;

		$result = $this->db->select( 'oldimage',
			array( 'oi_archive_name', 'oi_deleted' ),
			array( 'oi_name' => $this->oldName ),
			__METHOD__
		);
		while( $row = $this->db->fetchObject( $result ) ) {
			$oldName = $row->oi_archive_name;
			$bits = explode( '!', $oldName, 2 );
			if( count( $bits ) != 2 ) {
				wfDebug( 'Invalid old file name: ' . $oldName );
				continue;
			}
			list( $timestamp, $filename ) = $bits;
			if( $this->oldName != $filename ) {
				wfDebug( 'Invalid old file name:' . $oldName );
				continue;
			}
			$this->oldCount++;
			// Do we want to add those to oldCount?
			if( $row->oi_deleted & File::DELETED_FILE ) {
				continue;
			}
			$this->olds[] = array(
				"{$archiveBase}/{$this->oldHash}{$oldname}",
				"{$archiveBase}/{$this->newHash}{$timestamp}!{$this->newName}"
			);
		}
		$this->db->freeResult( $result );
	}

	/*
	 * Perform the move.
	 */
	function execute() {
		$repo = $this->file->repo;
		$status = $repo->newGood();
		$triplets = $this->getMoveTriplets();

		$statusDb = $this->doDBUpdates();
		wfDebugLog( 'imagemove', "Renamed {$this->file->name} in database: {$statusDb->successCount} successes, {$statusDb->failCount} failures" );
		$statusMove = $repo->storeBatch( $triplets, FSRepo::DELETE_SOURCE );
		wfDebugLog( 'imagemove', "Moved files for {$this->file->name}: {$statusMove->successCount} successes, {$statusMove->failCount} failures" );
		if( !$statusMove->isOk() ) {
			wfDebugLog( 'imagemove', "Error in moving files: " . $statusMove->getWikiText() );
			$this->db->rollback();
		}
		$status->merge( $statusDb );
		$status->merge( $statusMove );
		return $status;
	}

	/*
	 * Do the database updates and return a new WikiError indicating how many
	 * rows where updated.
	 */
	function doDBUpdates() {
		$repo = $this->file->repo;
		$status = $repo->newGood();
		$dbw = $this->db;

		// Update current image
		$dbw->update( 
			'image',
			array( 'img_name' => $this->newName ),
			array( 'img_name' => $this->oldName ),
			__METHOD__
		);
		if( $dbw->affectedRows() ) {
			$status->successCount++;
		} else {
			$status->failCount++;
		}

		// Update old images
		$dbw->update(
			'oldimage',
			array(
				'oi_name' => $this->newName,
				'oi_archive_name = ' . $dbw->strreplace( 'oi_archive_name', $dbw->addQuotes($this->oldName), $dbw->addQuotes($this->newName) ),
			),
			array( 'oi_name' => $this->oldName ),
			__METHOD__
		);
		$affected = $dbw->affectedRows();
		$total = $this->oldCount;
		$status->successCount += $affected;
		$status->failCount += $total - $affected;

		return $status;
	}

	/*
	 * Generate triplets for FSRepo::storeBatch().
	 */ 
	function getMoveTriplets() {
		$moves = array_merge( array( $this->cur ), $this->olds );
		$triplets = array();	// The format is: (srcUrl, destZone, destUrl)
		foreach( $moves as $move ) {
			// $move: (oldRelativePath, newRelativePath)
			$srcUrl = $this->file->repo->getVirtualUrl() . '/public/' . rawurlencode( $move[0] );
			$triplets[] = array( $srcUrl, 'public', $move[1] );
			wfDebugLog( 'imagemove', "Generated move triplet for {$this->file->name}: {$srcUrl} :: public :: {$move[1]}" );
		}
		return $triplets;
	}
}
