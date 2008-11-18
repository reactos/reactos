<?php

/**
 * Base class for file repositories
 * Do not instantiate, use a derived class.
 * @ingroup FileRepo
 */
abstract class FileRepo {
	const DELETE_SOURCE = 1;
	const FIND_PRIVATE = 1;
	const FIND_IGNORE_REDIRECT = 2;
	const OVERWRITE = 2;
	const OVERWRITE_SAME = 4;

	var $thumbScriptUrl, $transformVia404;
	var $descBaseUrl, $scriptDirUrl, $articleUrl, $fetchDescription, $initialCapital;
	var $pathDisclosureProtection = 'paranoid';

	/**
	 * Factory functions for creating new files
	 * Override these in the base class
	 */
	var $fileFactory = false, $oldFileFactory = false;
	var $fileFactoryKey = false, $oldFileFactoryKey = false;

	function __construct( $info ) {
		// Required settings
		$this->name = $info['name'];

		// Optional settings
		$this->initialCapital = true; // by default
		foreach ( array( 'descBaseUrl', 'scriptDirUrl', 'articleUrl', 'fetchDescription',
			'thumbScriptUrl', 'initialCapital', 'pathDisclosureProtection', 'descriptionCacheExpiry' ) as $var )
		{
			if ( isset( $info[$var] ) ) {
				$this->$var = $info[$var];
			}
		}
		$this->transformVia404 = !empty( $info['transformVia404'] );
	}

	/**
	 * Determine if a string is an mwrepo:// URL
	 */
	static function isVirtualUrl( $url ) {
		return substr( $url, 0, 9 ) == 'mwrepo://';
	}

	/**
	 * Create a new File object from the local repository
	 * @param mixed $title Title object or string
	 * @param mixed $time Time at which the image was uploaded.
	 *                    If this is specified, the returned object will be an
	 *                    instance of the repository's old file class instead of
	 *                    a current file. Repositories not supporting version
	 *                    control should return false if this parameter is set.
	 */
	function newFile( $title, $time = false ) {
		if ( !($title instanceof Title) ) {
			$title = Title::makeTitleSafe( NS_IMAGE, $title );
			if ( !is_object( $title ) ) {
				return null;
			}
		}
		if ( $time ) {
			if ( $this->oldFileFactory ) {
				return call_user_func( $this->oldFileFactory, $title, $this, $time );
			} else {
				return false;
			}
		} else {
			return call_user_func( $this->fileFactory, $title, $this );
		}
	}

	/**
	 * Find an instance of the named file created at the specified time
	 * Returns false if the file does not exist. Repositories not supporting
	 * version control should return false if the time is specified.
	 *
	 * @param mixed $title Title object or string
	 * @param mixed $time 14-character timestamp, or false for the current version
	 */
	function findFile( $title, $time = false, $flags = 0 ) {
		if ( !($title instanceof Title) ) {
			$title = Title::makeTitleSafe( NS_IMAGE, $title );
			if ( !is_object( $title ) ) {
				return false;
			}
		}
		# First try the current version of the file to see if it precedes the timestamp
		$img = $this->newFile( $title );
		if ( !$img ) {
			return false;
		}
		if ( $img->exists() && ( !$time || $img->getTimestamp() == $time ) ) {
			return $img;
		}
		# Now try an old version of the file
		if ( $time !== false ) {
			$img = $this->newFile( $title, $time );
			if ( $img->exists() ) {
				if ( !$img->isDeleted(File::DELETED_FILE) ) {
					return $img;
				} else if ( ($flags & FileRepo::FIND_PRIVATE) && $img->userCan(File::DELETED_FILE) ) {
					return $img;
				}
			}
		}
				
		# Now try redirects
		if ( $flags & FileRepo::FIND_IGNORE_REDIRECT ) {
			return false;
		}
		$redir = $this->checkRedirect( $title );		
		if( $redir && $redir->getNamespace() == NS_IMAGE) {
			$img = $this->newFile( $redir );
			if( !$img ) {
				return false;
			}
			if( $img->exists() ) {
				$img->redirectedFrom( $title->getDBkey() );
				return $img;
			}
		}
		return false;
	}
	
	/*
	 * Find many files at once. 
	 * @param array $titles, an array of titles
	 * @param int $flags
	 */
	function findFiles( $titles, $flags ) {
		$result = array();
		foreach ( $titles as $index => $title ) {
			$file = $this->findFile( $title, $flags );
			if ( $file )
				$result[$file->getTitle()->getDBkey()] = $file;
		}
		return $result;
	}
	
	/**
	 * Create a new File object from the local repository
	 * @param mixed $sha1 SHA-1 key
	 * @param mixed $time Time at which the image was uploaded.
	 *                    If this is specified, the returned object will be an
	 *                    instance of the repository's old file class instead of
	 *                    a current file. Repositories not supporting version
	 *                    control should return false if this parameter is set.
	 */
	function newFileFromKey( $sha1, $time = false ) {
		if ( $time ) {
			if ( $this->oldFileFactoryKey ) {
				return call_user_func( $this->oldFileFactoryKey, $sha1, $this, $time );
			} else {
				return false;
			}
		} else {
			return call_user_func( $this->fileFactoryKey, $sha1, $this );
		}
	}
	
	/**
	 * Find an instance of the file with this key, created at the specified time
	 * Returns false if the file does not exist. Repositories not supporting
	 * version control should return false if the time is specified.
	 *
	 * @param string $sha1 string
	 * @param mixed $time 14-character timestamp, or false for the current version
	 */
	function findFileFromKey( $sha1, $time = false, $flags = 0 ) {
		# First try the current version of the file to see if it precedes the timestamp
		$img = $this->newFileFromKey( $sha1 );
		if ( !$img ) {
			return false;
		}
		if ( $img->exists() && ( !$time || $img->getTimestamp() == $time ) ) {
			return $img;
		}
		# Now try an old version of the file
		if ( $time !== false ) {
			$img = $this->newFileFromKey( $sha1, $time );
			if ( $img->exists() ) {
				if ( !$img->isDeleted(File::DELETED_FILE) ) {
					return $img;
				} else if ( ($flags & FileRepo::FIND_PRIVATE) && $img->userCan(File::DELETED_FILE) ) {
					return $img;
				}
			}
		}
		return false;
	}

	/**
	 * Get the URL of thumb.php
	 */
	function getThumbScriptUrl() {
		return $this->thumbScriptUrl;
	}

	/**
	 * Returns true if the repository can transform files via a 404 handler
	 */
	function canTransformVia404() {
		return $this->transformVia404;
	}

	/**
	 * Get the name of an image from its title object
	 */
	function getNameFromTitle( $title ) {
		global $wgCapitalLinks;
		if ( $this->initialCapital != $wgCapitalLinks ) {
			global $wgContLang;
			$name = $title->getUserCaseDBKey();
			if ( $this->initialCapital ) {
				$name = $wgContLang->ucfirst( $name );
			}
		} else {
			$name = $title->getDBkey();
		}
		return $name;
	}

	static function getHashPathForLevel( $name, $levels ) {
		if ( $levels == 0 ) {
			return '';
		} else {
			$hash = md5( $name );
			$path = '';
			for ( $i = 1; $i <= $levels; $i++ ) {
				$path .= substr( $hash, 0, $i ) . '/';
			}
			return $path;
		}
	}

	/**
	 * Get the name of this repository, as specified by $info['name]' to the constructor
	 */
	function getName() {
		return $this->name;
	}

	/**
	 * Get the file description page base URL, or false if there isn't one.
	 * @private
	 */
	function getDescBaseUrl() {
		if ( is_null( $this->descBaseUrl ) ) {
			if ( !is_null( $this->articleUrl ) ) {
				$this->descBaseUrl = str_replace( '$1',
					wfUrlencode( MWNamespace::getCanonicalName( NS_IMAGE ) ) . ':', $this->articleUrl );
			} elseif ( !is_null( $this->scriptDirUrl ) ) {
				$this->descBaseUrl = $this->scriptDirUrl . '/index.php?title=' .
					wfUrlencode( MWNamespace::getCanonicalName( NS_IMAGE ) ) . ':';
			} else {
				$this->descBaseUrl = false;
			}
		}
		return $this->descBaseUrl;
	}

	/**
	 * Get the URL of an image description page. May return false if it is
	 * unknown or not applicable. In general this should only be called by the
	 * File class, since it may return invalid results for certain kinds of
	 * repositories. Use File::getDescriptionUrl() in user code.
	 *
	 * In particular, it uses the article paths as specified to the repository
	 * constructor, whereas local repositories use the local Title functions.
	 */
	function getDescriptionUrl( $name ) {
		$base = $this->getDescBaseUrl();
		if ( $base ) {
			return $base . wfUrlencode( $name );
		} else {
			return false;
		}
	}

	/**
	 * Get the URL of the content-only fragment of the description page. For
	 * MediaWiki this means action=render. This should only be called by the
	 * repository's file class, since it may return invalid results. User code
	 * should use File::getDescriptionText().
	 */
	function getDescriptionRenderUrl( $name ) {
		if ( isset( $this->scriptDirUrl ) ) {
			return $this->scriptDirUrl . '/index.php?title=' .
				wfUrlencode( MWNamespace::getCanonicalName( NS_IMAGE ) . ':' . $name ) .
				'&action=render';
		} else {
			$descBase = $this->getDescBaseUrl();
			if ( $descBase ) {
				return wfAppendQuery( $descBase . wfUrlencode( $name ), 'action=render' );
			} else {
				return false;
			}
		}
	}

	/**
	 * Store a file to a given destination.
	 *
	 * @param string $srcPath Source path or virtual URL
	 * @param string $dstZone Destination zone
	 * @param string $dstRel Destination relative path
	 * @param integer $flags Bitwise combination of the following flags:
	 *     self::DELETE_SOURCE     Delete the source file after upload
	 *     self::OVERWRITE         Overwrite an existing destination file instead of failing
	 *     self::OVERWRITE_SAME    Overwrite the file if the destination exists and has the
	 *                             same contents as the source
	 * @return FileRepoStatus
	 */
	function store( $srcPath, $dstZone, $dstRel, $flags = 0 ) {
		$status = $this->storeBatch( array( array( $srcPath, $dstZone, $dstRel ) ), $flags );
		if ( $status->successCount == 0 ) {
			$status->ok = false;
		}
		return $status;
	}

	/**
	 * Store a batch of files
	 *
	 * @param array $triplets (src,zone,dest) triplets as per store()
	 * @param integer $flags Flags as per store
	 */
	abstract function storeBatch( $triplets, $flags = 0 );

	/**
	 * Pick a random name in the temp zone and store a file to it.
	 * Returns a FileRepoStatus object with the URL in the value.
	 *
	 * @param string $originalName The base name of the file as specified
	 *     by the user. The file extension will be maintained.
	 * @param string $srcPath The current location of the file.
	 */
	abstract function storeTemp( $originalName, $srcPath );

	/**
	 * Remove a temporary file or mark it for garbage collection
	 * @param string $virtualUrl The virtual URL returned by storeTemp
	 * @return boolean True on success, false on failure
	 * STUB
	 */
	function freeTemp( $virtualUrl ) {
		return true;
	}

	/**
	 * Copy or move a file either from the local filesystem or from an mwrepo://
	 * virtual URL, into this repository at the specified destination location.
	 *
	 * Returns a FileRepoStatus object. On success, the value contains "new" or
	 * "archived", to indicate whether the file was new with that name.
	 *
	 * @param string $srcPath The source path or URL
	 * @param string $dstRel The destination relative path
	 * @param string $archiveRel The relative path where the existing file is to
	 *        be archived, if there is one. Relative to the public zone root.
	 * @param integer $flags Bitfield, may be FileRepo::DELETE_SOURCE to indicate
	 *        that the source file should be deleted if possible
	 */
	function publish( $srcPath, $dstRel, $archiveRel, $flags = 0 ) {
		$status = $this->publishBatch( array( array( $srcPath, $dstRel, $archiveRel ) ), $flags );
		if ( $status->successCount == 0 ) {
			$status->ok = false;
		}
		if ( isset( $status->value[0] ) ) {
			$status->value = $status->value[0];
		} else {
			$status->value = false;
		}
		return $status;
	}

	/**
	 * Publish a batch of files
	 * @param array $triplets (source,dest,archive) triplets as per publish()
	 * @param integer $flags Bitfield, may be FileRepo::DELETE_SOURCE to indicate
	 *        that the source files should be deleted if possible
	 */
	abstract function publishBatch( $triplets, $flags = 0 );

	/**
	 * Move a group of files to the deletion archive.
	 *
	 * If no valid deletion archive is configured, this may either delete the
	 * file or throw an exception, depending on the preference of the repository.
	 *
	 * The overwrite policy is determined by the repository -- currently FSRepo
	 * assumes a naming scheme in the deleted zone based on content hash, as
	 * opposed to the public zone which is assumed to be unique.
	 *
	 * @param array $sourceDestPairs Array of source/destination pairs. Each element
	 *        is a two-element array containing the source file path relative to the
	 *        public root in the first element, and the archive file path relative
	 *        to the deleted zone root in the second element.
	 * @return FileRepoStatus
	 */
	abstract function deleteBatch( $sourceDestPairs );

	/**
	 * Move a file to the deletion archive.
	 * If no valid deletion archive exists, this may either delete the file
	 * or throw an exception, depending on the preference of the repository
	 * @param mixed $srcRel Relative path for the file to be deleted
	 * @param mixed $archiveRel Relative path for the archive location.
	 *        Relative to a private archive directory.
	 * @return WikiError object (wikitext-formatted), or true for success
	 */
	function delete( $srcRel, $archiveRel ) {
		return $this->deleteBatch( array( array( $srcRel, $archiveRel ) ) );
	}

	/**
	 * Get properties of a file with a given virtual URL
	 * The virtual URL must refer to this repo
	 * Properties should ultimately be obtained via File::getPropsFromPath()
	 */
	abstract function getFileProps( $virtualUrl );

	/**
	 * Call a callback function for every file in the repository
	 * May use either the database or the filesystem
	 * STUB
	 */
	function enumFiles( $callback ) {
		throw new MWException( 'enumFiles is not supported by ' . get_class( $this ) );
	}

	/**
	 * Determine if a relative path is valid, i.e. not blank or involving directory traveral
	 */
	function validateFilename( $filename ) {
		if ( strval( $filename ) == '' ) {
			return false;
		}
		if ( wfIsWindows() ) {
			$filename = strtr( $filename, '\\', '/' );
		}
		/**
		 * Use the same traversal protection as Title::secureAndSplit()
		 */
		if ( strpos( $filename, '.' ) !== false &&
		     ( $filename === '.' || $filename === '..' ||
		       strpos( $filename, './' ) === 0  ||
		       strpos( $filename, '../' ) === 0 ||
		       strpos( $filename, '/./' ) !== false ||
		       strpos( $filename, '/../' ) !== false ) )
		{
			return false;
		} else {
			return true;
		}
	}

	/**#@+
	 * Path disclosure protection functions
	 */
	function paranoidClean( $param ) { return '[hidden]'; }
	function passThrough( $param ) { return $param; }

	/**
	 * Get a callback function to use for cleaning error message parameters
	 */
	function getErrorCleanupFunction() {
		switch ( $this->pathDisclosureProtection ) {
			case 'none':
				$callback = array( $this, 'passThrough' );
				break;
			default: // 'paranoid'
				$callback = array( $this, 'paranoidClean' );
		}
		return $callback;
	}
	/**#@-*/

	/**
	 * Create a new fatal error
	 */
	function newFatal( $message /*, parameters...*/ ) {
		$params = func_get_args();
		array_unshift( $params, $this );
		return call_user_func_array( array( 'FileRepoStatus', 'newFatal' ), $params );
	}

	/**
	 * Create a new good result
	 */
	function newGood( $value = null ) {
		return FileRepoStatus::newGood( $this, $value );
	}

	/**
	 * Delete files in the deleted directory if they are not referenced in the filearchive table
	 * STUB
	 */
	function cleanupDeletedBatch( $storageKeys ) {}

	/**
	 * Checks if there is a redirect named as $title
	 * STUB
	 *
	 * @param Title $title Title of image
	 */
	function checkRedirect( $title ) {
		return false;
	}

	/**
	 * Invalidates image redirect cache related to that image
	 * STUB
	 *
	 * @param Title $title Title of image
	 */
	function invalidateImageRedirect( $title ) {
	}
	
	function findBySha1( $hash ) {
		return array();
	}
}
