<?php

/**
 * Implements some public methods and some protected utility functions which
 * are required by multiple child classes. Contains stub functionality for
 * unimplemented public methods.
 *
 * Stub functions which should be overridden are marked with STUB. Some more
 * concrete functions are also typically overridden by child classes.
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
abstract class File {
	const DELETED_FILE = 1;
	const DELETED_COMMENT = 2;
	const DELETED_USER = 4;
	const DELETED_RESTRICTED = 8;
	const RENDER_NOW = 1;

	const DELETE_SOURCE = 1;

	/**
	 * Some member variables can be lazy-initialised using __get(). The
	 * initialisation function for these variables is always a function named
	 * like getVar(), where Var is the variable name with upper-case first
	 * letter.
	 *
	 * The following variables are initialised in this way in this base class:
	 *    name, extension, handler, path, canRender, isSafeFile,
	 *    transformScript, hashPath, pageCount, url
	 *
	 * Code within this class should generally use the accessor function
	 * directly, since __get() isn't re-entrant and therefore causes bugs that
	 * depend on initialisation order.
	 */

	/**
	 * The following member variables are not lazy-initialised
	 */
	var $repo, $title, $lastError, $redirected, $redirectedTitle;

	/**
	 * Call this constructor from child classes
	 */
	function __construct( $title, $repo ) {
		$this->title = $title;
		$this->repo = $repo;
	}

	function __get( $name ) {
		$function = array( $this, 'get' . ucfirst( $name ) );
		if ( !is_callable( $function ) ) {
			return null;
		} else {
			$this->$name = call_user_func( $function );
			return $this->$name;
		}
	}

	/**
	 * Normalize a file extension to the common form, and ensure it's clean.
	 * Extensions with non-alphanumeric characters will be discarded.
	 *
	 * @param $ext string (without the .)
	 * @return string
	 */
	static function normalizeExtension( $ext ) {
		$lower = strtolower( $ext );
		$squish = array(
			'htm' => 'html',
			'jpeg' => 'jpg',
			'mpeg' => 'mpg',
			'tiff' => 'tif',
			'ogv' => 'ogg' );
		if( isset( $squish[$lower] ) ) {
			return $squish[$lower];
		} elseif( preg_match( '/^[0-9a-z]+$/', $lower ) ) {
			return $lower;
		} else {
			return '';
		}
	}

	/**
	 * Checks if file extensions are compatible
	 *
	 * @param $old File Old file
	 * @param $new string New name
	 */
	static function checkExtensionCompatibility( File $old, $new ) {
		$oldMime = $old->getMimeType();
		$n = strrpos( $new, '.' );
		$newExt = self::normalizeExtension(
			$n ? substr( $new, $n + 1 ) : '' );
		$mimeMagic = MimeMagic::singleton();
		return $mimeMagic->isMatchingExtension( $newExt, $oldMime );
	}

	/**
	 * Upgrade the database row if there is one
	 * Called by ImagePage
	 * STUB
	 */
	function upgradeRow() {}

	/**
	 * Split an internet media type into its two components; if not
	 * a two-part name, set the minor type to 'unknown'.
	 *
	 * @param $mime "text/html" etc
	 * @return array ("text", "html") etc
	 */
	static function splitMime( $mime ) {
		if( strpos( $mime, '/' ) !== false ) {
			return explode( '/', $mime, 2 );
		} else {
			return array( $mime, 'unknown' );
		}
	}

	/**
	 * Return the name of this file
	 */
	public function getName() {
		if ( !isset( $this->name ) ) {
			$this->name = $this->repo->getNameFromTitle( $this->title );
		}
		return $this->name;
	}

	/**
	 * Get the file extension, e.g. "svg"
	 */
	function getExtension() {
		if ( !isset( $this->extension ) ) {
			$n = strrpos( $this->getName(), '.' );
			$this->extension = self::normalizeExtension(
				$n ? substr( $this->getName(), $n + 1 ) : '' );
		}
		return $this->extension;
	}

	/**
	 * Return the associated title object
	 */
	public function getTitle() { return $this->title; }
	
	/**
	 * Return the title used to find this file
	 */
	public function getOriginalTitle() {
		if ( $this->redirected )
			return $this->getRedirectedTitle();
		return $this->title;
	}

	/**
	 * Return the URL of the file
	 */
	public function getUrl() {
		if ( !isset( $this->url ) ) {
			$this->url = $this->repo->getZoneUrl( 'public' ) . '/' . $this->getUrlRel();
		}
		return $this->url;
	}

	/**
	 * Return a fully-qualified URL to the file.
	 * Upload URL paths _may or may not_ be fully qualified, so
	 * we check. Local paths are assumed to belong on $wgServer.
	 * @return string
	 */
	public function getFullUrl() {
		return wfExpandUrl( $this->getUrl() );
	}

	function getViewURL() {
		if( $this->mustRender()) {
			if( $this->canRender() ) {
				return $this->createThumb( $this->getWidth() );
			}
			else {
				wfDebug(__METHOD__.': supposed to render '.$this->getName().' ('.$this->getMimeType()."), but can't!\n");
				return $this->getURL(); #hm... return NULL?
			}
		} else {
			return $this->getURL();
		}
	}

	/**
	* Return the full filesystem path to the file. Note that this does
	* not mean that a file actually exists under that location.
	*
	* This path depends on whether directory hashing is active or not,
	* i.e. whether the files are all found in the same directory,
	* or in hashed paths like /images/3/3c.
	*
	* May return false if the file is not locally accessible.
	*/
	public function getPath() {
		if ( !isset( $this->path ) ) {
			$this->path = $this->repo->getZonePath('public') . '/' . $this->getRel();
		}
		return $this->path;
	}

	/**
	* Alias for getPath()
	*/
	public function getFullPath() {
		return $this->getPath();
	}

	/**
	 * Return the width of the image. Returns false if the width is unknown
	 * or undefined.
	 *
	 * STUB
	 * Overridden by LocalFile, UnregisteredLocalFile
	 */
	public function getWidth( $page = 1 ) { return false; }

	/**
	 * Return the height of the image. Returns false if the height is unknown
	 * or undefined
	 *
	 * STUB
	 * Overridden by LocalFile, UnregisteredLocalFile
	 */
	public function getHeight( $page = 1 ) { return false; }

	/**
	 * Returns ID or name of user who uploaded the file
	 * STUB
	 *
	 * @param $type string 'text' or 'id'
	 */
	public function getUser( $type='text' ) { return null; }

	/**
	 * Get the duration of a media file in seconds
	 */
	public function getLength() {
		$handler = $this->getHandler();
		if ( $handler ) {
			return $handler->getLength( $this );
		} else {
			return 0;
		}
	}

	/**
	 * Get handler-specific metadata
	 * Overridden by LocalFile, UnregisteredLocalFile
	 * STUB
	 */
	function getMetadata() { return false; }

	/**
	 * Return the size of the image file, in bytes
	 * Overridden by LocalFile, UnregisteredLocalFile
	 * STUB
	 */
	public function getSize() { return false; }

	/**
	 * Returns the mime type of the file.
	 * Overridden by LocalFile, UnregisteredLocalFile
	 * STUB
	 */
	function getMimeType() { return 'unknown/unknown'; }

	/**
	 * Return the type of the media in the file.
	 * Use the value returned by this function with the MEDIATYPE_xxx constants.
	 * Overridden by LocalFile,
	 * STUB
	 */
	function getMediaType() { return MEDIATYPE_UNKNOWN; }

	/**
	 * Checks if the output of transform() for this file is likely
	 * to be valid. If this is false, various user elements will
	 * display a placeholder instead.
	 *
	 * Currently, this checks if the file is an image format
	 * that can be converted to a format
	 * supported by all browsers (namely GIF, PNG and JPEG),
	 * or if it is an SVG image and SVG conversion is enabled.
	 */
	function canRender() {
		if ( !isset( $this->canRender ) ) {
			$this->canRender = $this->getHandler() && $this->handler->canRender( $this );
		}
		return $this->canRender;
	}

	/**
	 * Accessor for __get()
	 */
	protected function getCanRender() {
		return $this->canRender();
	}

	/**
	 * Return true if the file is of a type that can't be directly
	 * rendered by typical browsers and needs to be re-rasterized.
	 *
	 * This returns true for everything but the bitmap types
	 * supported by all browsers, i.e. JPEG; GIF and PNG. It will
	 * also return true for any non-image formats.
	 *
	 * @return bool
	 */
	function mustRender() {
		return $this->getHandler() && $this->handler->mustRender( $this );
	}

	/**
	 * Alias for canRender()
	 */
	function allowInlineDisplay() {
		return $this->canRender();
	}

	/**
	 * Determines if this media file is in a format that is unlikely to
	 * contain viruses or malicious content. It uses the global
	 * $wgTrustedMediaFormats list to determine if the file is safe.
	 *
	 * This is used to show a warning on the description page of non-safe files.
	 * It may also be used to disallow direct [[media:...]] links to such files.
	 *
	 * Note that this function will always return true if allowInlineDisplay()
	 * or isTrustedFile() is true for this file.
	 */
	function isSafeFile() {
		if ( !isset( $this->isSafeFile ) ) {
			$this->isSafeFile = $this->_getIsSafeFile();
		}
		return $this->isSafeFile;
	}

	/** Accessor for __get() */
	protected function getIsSafeFile() {
		return $this->isSafeFile();
	}

	/** Uncached accessor */
	protected function _getIsSafeFile() {
		if ($this->allowInlineDisplay()) return true;
		if ($this->isTrustedFile()) return true;

		global $wgTrustedMediaFormats;

		$type= $this->getMediaType();
		$mime= $this->getMimeType();
		#wfDebug("LocalFile::isSafeFile: type= $type, mime= $mime\n");

		if (!$type || $type===MEDIATYPE_UNKNOWN) return false; #unknown type, not trusted
		if ( in_array( $type, $wgTrustedMediaFormats) ) return true;

		if ($mime==="unknown/unknown") return false; #unknown type, not trusted
		if ( in_array( $mime, $wgTrustedMediaFormats) ) return true;

		return false;
	}

	/** Returns true if the file is flagged as trusted. Files flagged that way
	* can be linked to directly, even if that is not allowed for this type of
	* file normally.
	*
	* This is a dummy function right now and always returns false. It could be
	* implemented to extract a flag from the database. The trusted flag could be
	* set on upload, if the user has sufficient privileges, to bypass script-
	* and html-filters. It may even be coupled with cryptographics signatures
	* or such.
	*/
	function isTrustedFile() {
		#this could be implemented to check a flag in the databas,
		#look for signatures, etc
		return false;
	}

	/**
	 * Returns true if file exists in the repository.
	 *
	 * Overridden by LocalFile to avoid unnecessary stat calls.
	 *
	 * @return boolean Whether file exists in the repository.
	 */
	public function exists() {
		return $this->getPath() && file_exists( $this->path );
	}

	/**
	 * Returns true if file exists in the repository and can be included in a page.
	 * It would be unsafe to include private images, making public thumbnails inadvertently
	 *
	 * @return boolean Whether file exists in the repository and is includable.
	 * @public
	 */
	function isVisible() {
		return $this->exists();
	}

	function getTransformScript() {
		if ( !isset( $this->transformScript ) ) {
			$this->transformScript = false;
			if ( $this->repo ) {
				$script = $this->repo->getThumbScriptUrl();
				if ( $script ) {
					$this->transformScript = "$script?f=" . urlencode( $this->getName() );
				}
			}
		}
		return $this->transformScript;
	}

	/**
	 * Get a ThumbnailImage which is the same size as the source
	 */
	function getUnscaledThumb( $page = false ) {
		$width = $this->getWidth( $page );
		if ( !$width ) {
			return $this->iconThumb();
		}
		if ( $page ) {
			$params = array(
				'page' => $page,
				'width' => $this->getWidth( $page )
			);
		} else {
			$params = array( 'width' => $this->getWidth() );
		}
		return $this->transform( $params );
	}

	/**
	 * Return the file name of a thumbnail with the specified parameters
	 *
	 * @param array $params Handler-specific parameters
	 * @private -ish
	 */
	function thumbName( $params ) {
		if ( !$this->getHandler() ) {
			return null;
		}
		$extension = $this->getExtension();
		list( $thumbExt, $thumbMime ) = $this->handler->getThumbType( $extension, $this->getMimeType() );
		$thumbName = $this->handler->makeParamString( $params ) . '-' . $this->getName();
		if ( $thumbExt != $extension ) {
			$thumbName .= ".$thumbExt";
		}
		return $thumbName;
	}

	/**
	 * Create a thumbnail of the image having the specified width/height.
	 * The thumbnail will not be created if the width is larger than the
	 * image's width. Let the browser do the scaling in this case.
	 * The thumbnail is stored on disk and is only computed if the thumbnail
	 * file does not exist OR if it is older than the image.
	 * Returns the URL.
	 *
	 * Keeps aspect ratio of original image. If both width and height are
	 * specified, the generated image will be no bigger than width x height,
	 * and will also have correct aspect ratio.
	 *
	 * @param integer $width	maximum width of the generated thumbnail
	 * @param integer $height	maximum height of the image (optional)
	 */
	public function createThumb( $width, $height = -1 ) {
		$params = array( 'width' => $width );
		if ( $height != -1 ) {
			$params['height'] = $height;
		}
		$thumb = $this->transform( $params );
		if( is_null( $thumb ) || $thumb->isError() ) return '';
		return $thumb->getUrl();
	}

	/**
	 * As createThumb, but returns a ThumbnailImage object. This can
	 * provide access to the actual file, the real size of the thumb,
	 * and can produce a convenient <img> tag for you.
	 *
	 * For non-image formats, this may return a filetype-specific icon.
	 *
	 * @param integer $width	maximum width of the generated thumbnail
	 * @param integer $height	maximum height of the image (optional)
	 * @param boolean $render	True to render the thumbnail if it doesn't exist,
	 *                       	false to just return the URL
	 *
	 * @return ThumbnailImage or null on failure
	 *
	 * @deprecated use transform()
	 */
	public function getThumbnail( $width, $height=-1, $render = true ) {
		$params = array( 'width' => $width );
		if ( $height != -1 ) {
			$params['height'] = $height;
		}
		$flags = $render ? self::RENDER_NOW : 0;
		return $this->transform( $params, $flags );
	}

	/**
	 * Transform a media file
	 *
	 * @param array $params An associative array of handler-specific parameters. Typical
	 *                      keys are width, height and page.
	 * @param integer $flags A bitfield, may contain self::RENDER_NOW to force rendering
	 * @return MediaTransformOutput
	 */
	function transform( $params, $flags = 0 ) {
		global $wgUseSquid, $wgIgnoreImageErrors;

		wfProfileIn( __METHOD__ );
		do {
			if ( !$this->canRender() ) {
				// not a bitmap or renderable image, don't try.
				$thumb = $this->iconThumb();
				break;
			}

			$script = $this->getTransformScript();
			if ( $script && !($flags & self::RENDER_NOW) ) {
				// Use a script to transform on client request, if possible
				$thumb = $this->handler->getScriptedTransform( $this, $script, $params );
				if( $thumb ) {
					break;
				}
			}

			$normalisedParams = $params;
			$this->handler->normaliseParams( $this, $normalisedParams );
			$thumbName = $this->thumbName( $normalisedParams );
			$thumbPath = $this->getThumbPath( $thumbName );
			$thumbUrl = $this->getThumbUrl( $thumbName );

			if ( $this->repo->canTransformVia404() && !($flags & self::RENDER_NOW ) ) {
				$thumb = $this->handler->getTransform( $this, $thumbPath, $thumbUrl, $params );
				break;
			}

			wfDebug( __METHOD__.": Doing stat for $thumbPath\n" );
			$this->migrateThumbFile( $thumbName );
			if ( file_exists( $thumbPath ) ) {
				$thumb = $this->handler->getTransform( $this, $thumbPath, $thumbUrl, $params );
				break;
			}
			$thumb = $this->handler->doTransform( $this, $thumbPath, $thumbUrl, $params );

			// Ignore errors if requested
			if ( !$thumb ) {
				$thumb = null;
			} elseif ( $thumb->isError() ) {
				$this->lastError = $thumb->toText();
				if ( $wgIgnoreImageErrors && !($flags & self::RENDER_NOW) ) {
					$thumb = $this->handler->getTransform( $this, $thumbPath, $thumbUrl, $params );
				}
			}
			
			// Purge. Useful in the event of Core -> Squid connection failure or squid 
			// purge collisions from elsewhere during failure. Don't keep triggering for 
			// "thumbs" which have the main image URL though (bug 13776)
			if ( $wgUseSquid && ($thumb->isError() || $thumb->getUrl() != $this->getURL()) ) {
				SquidUpdate::purge( array( $thumbUrl ) );
			}
		} while (false);

		wfProfileOut( __METHOD__ );
		return $thumb;
	}

	/**
	 * Hook into transform() to allow migration of thumbnail files
	 * STUB
	 * Overridden by LocalFile
	 */
	function migrateThumbFile( $thumbName ) {}

	/**
	 * Get a MediaHandler instance for this file
	 */
	function getHandler() {
		if ( !isset( $this->handler ) ) {
			$this->handler = MediaHandler::getHandler( $this->getMimeType() );
		}
		return $this->handler;
	}

	/**
	 * Get a ThumbnailImage representing a file type icon
	 * @return ThumbnailImage
	 */
	function iconThumb() {
		global $wgStylePath, $wgStyleDirectory;

		$try = array( 'fileicon-' . $this->getExtension() . '.png', 'fileicon.png' );
		foreach( $try as $icon ) {
			$path = '/common/images/icons/' . $icon;
			$filepath = $wgStyleDirectory . $path;
			if( file_exists( $filepath ) ) {
				return new ThumbnailImage( $this, $wgStylePath . $path, 120, 120 );
			}
		}
		return null;
	}

	/**
	 * Get last thumbnailing error.
	 * Largely obsolete.
	 */
	function getLastError() {
		return $this->lastError;
	}

	/**
	 * Get all thumbnail names previously generated for this file
	 * STUB
	 * Overridden by LocalFile
	 */
	function getThumbnails() { return array(); }

	/**
	 * Purge shared caches such as thumbnails and DB data caching
	 * STUB
	 * Overridden by LocalFile
	 */
	function purgeCache() {}

	/**
	 * Purge the file description page, but don't go after
	 * pages using the file. Use when modifying file history
	 * but not the current data.
	 */
	function purgeDescription() {
		$title = $this->getTitle();
		if ( $title ) {
			$title->invalidateCache();
			$title->purgeSquid();
		}
	}

	/**
	 * Purge metadata and all affected pages when the file is created,
	 * deleted, or majorly updated.
	 */
	function purgeEverything() {
		// Delete thumbnails and refresh file metadata cache
		$this->purgeCache();
		$this->purgeDescription();

		// Purge cache of all pages using this file
		$title = $this->getTitle();
		if ( $title ) {
			$update = new HTMLCacheUpdate( $title, 'imagelinks' );
			$update->doUpdate();
		}
	}

	/**
	 * Return a fragment of the history of file.
	 *
	 * STUB
	 * @param $limit integer Limit of rows to return
	 * @param $start timestamp Only revisions older than $start will be returned
	 * @param $end timestamp Only revisions newer than $end will be returned
	 */
	function getHistory($limit = null, $start = null, $end = null) {
		return array();
	}

	/**
	 * Return the history of this file, line by line. Starts with current version,
	 * then old versions. Should return an object similar to an image/oldimage
	 * database row.
	 *
	 * STUB
	 * Overridden in LocalFile
	 */
	public function nextHistoryLine() {
		return false;
	}

	/**
	 * Reset the history pointer to the first element of the history.
	 * Always call this function after using nextHistoryLine() to free db resources
	 * STUB
	 * Overridden in LocalFile.
	 */
	public function resetHistory() {}

	/**
	 * Get the filename hash component of the directory including trailing slash,
	 * e.g. f/fa/
	 * If the repository is not hashed, returns an empty string.
	 */
	function getHashPath() {
		if ( !isset( $this->hashPath ) ) {
			$this->hashPath = $this->repo->getHashPath( $this->getName() );
		}
		return $this->hashPath;
	}

	/**
	 * Get the path of the file relative to the public zone root
	 */
	function getRel() {
		return $this->getHashPath() . $this->getName();
	}

	/**
	 * Get urlencoded relative path of the file
	 */
	function getUrlRel() {
		return $this->getHashPath() . rawurlencode( $this->getName() );
	}

	/** Get the relative path for an archive file */
	function getArchiveRel( $suffix = false ) {
		$path = 'archive/' . $this->getHashPath();
		if ( $suffix === false ) {
			$path = substr( $path, 0, -1 );
		} else {
			$path .= $suffix;
		}
		return $path;
	}

	/** Get relative path for a thumbnail file */
	function getThumbRel( $suffix = false ) {
		$path = 'thumb/' . $this->getRel();
		if ( $suffix !== false ) {
			$path .= '/' . $suffix;
		}
		return $path;
	}

	/** Get the path of the archive directory, or a particular file if $suffix is specified */
	function getArchivePath( $suffix = false ) {
		return $this->repo->getZonePath('public') . '/' . $this->getArchiveRel( $suffix );
	}

	/** Get the path of the thumbnail directory, or a particular file if $suffix is specified */
	function getThumbPath( $suffix = false ) {
		return $this->repo->getZonePath('public') . '/' . $this->getThumbRel( $suffix );
	}

	/** Get the URL of the archive directory, or a particular file if $suffix is specified */
	function getArchiveUrl( $suffix = false ) {
		$path = $this->repo->getZoneUrl('public') . '/archive/' . $this->getHashPath();
		if ( $suffix === false ) {
			$path = substr( $path, 0, -1 );
		} else {
			$path .= rawurlencode( $suffix );
		}
		return $path;
	}

	/** Get the URL of the thumbnail directory, or a particular file if $suffix is specified */
	function getThumbUrl( $suffix = false ) {
		$path = $this->repo->getZoneUrl('public') . '/thumb/' . $this->getUrlRel();
		if ( $suffix !== false ) {
			$path .= '/' . rawurlencode( $suffix );
		}
		return $path;
	}

	/** Get the virtual URL for an archive file or directory */
	function getArchiveVirtualUrl( $suffix = false ) {
		$path = $this->repo->getVirtualUrl() . '/public/archive/' . $this->getHashPath();
		if ( $suffix === false ) {
			$path = substr( $path, 0, -1 );
		} else {
			$path .= rawurlencode( $suffix );
		}
		return $path;
	}

	/** Get the virtual URL for a thumbnail file or directory */
	function getThumbVirtualUrl( $suffix = false ) {
		$path = $this->repo->getVirtualUrl() . '/public/thumb/' . $this->getUrlRel();
		if ( $suffix !== false ) {
			$path .= '/' . rawurlencode( $suffix );
		}
		return $path;
	}

	/** Get the virtual URL for the file itself */
	function getVirtualUrl( $suffix = false ) {
		$path = $this->repo->getVirtualUrl() . '/public/' . $this->getUrlRel();
		if ( $suffix !== false ) {
			$path .= '/' . rawurlencode( $suffix );
		}
		return $path;
	}

	/**
	 * @return bool
	 */
	function isHashed() {
		return $this->repo->isHashed();
	}

	function readOnlyError() {
		throw new MWException( get_class($this) . ': write operations are not supported' );
	}

	/**
	 * Record a file upload in the upload log and the image table
	 * STUB
	 * Overridden by LocalFile
	 */
	function recordUpload( $oldver, $desc, $license = '', $copyStatus = '', $source = '', $watch = false ) {
		$this->readOnlyError();
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
	 * @return The archive name on success or an empty string if it was a new
	 *     file, and a wikitext-formatted WikiError object on failure.
	 *
	 * STUB
	 * Overridden by LocalFile
	 */
	function publish( $srcPath, $flags = 0 ) {
		$this->readOnlyError();
	}

	/**
	 * Get an array of Title objects which are articles which use this file
	 * Also adds their IDs to the link cache
	 *
	 * This is mostly copied from Title::getLinksTo()
	 *
	 * @deprecated Use HTMLCacheUpdate, this function uses too much memory
	 */
	function getLinksTo( $options = '' ) {
		wfProfileIn( __METHOD__ );

		// Note: use local DB not repo DB, we want to know local links
		if ( $options ) {
			$db = wfGetDB( DB_MASTER );
		} else {
			$db = wfGetDB( DB_SLAVE );
		}
		$linkCache = LinkCache::singleton();

		list( $page, $imagelinks ) = $db->tableNamesN( 'page', 'imagelinks' );
		$encName = $db->addQuotes( $this->getName() );
		$sql = "SELECT page_namespace,page_title,page_id,page_len,page_is_redirect,
			FROM $page,$imagelinks WHERE page_id=il_from AND il_to=$encName $options";
		$res = $db->query( $sql, __METHOD__ );

		$retVal = array();
		if ( $db->numRows( $res ) ) {
			while ( $row = $db->fetchObject( $res ) ) {
				if ( $titleObj = Title::newFromRow( $row ) ) {
					$linkCache->addGoodLinkObj( $row->page_id, $titleObj, $row->page_len, $row->page_is_redirect );
					$retVal[] = $titleObj;
				}
			}
		}
		$db->freeResult( $res );
		wfProfileOut( __METHOD__ );
		return $retVal;
	}

	function formatMetadata() {
		if ( !$this->getHandler() ) {
			return false;
		}
		return $this->getHandler()->formatMetadata( $this, $this->getMetadata() );
	}

	/**
	 * Returns true if the file comes from the local file repository.
	 *
	 * @return bool
	 */
	function isLocal() {
		return $this->getRepoName() == 'local';
	}

	/**
	 * Returns the name of the repository.
	 *
	 * @return string
	 */
	function getRepoName() {
		return $this->repo ? $this->repo->getName() : 'unknown';
	}
	/*
	 * Returns the repository
	 */
	function getRepo() {
		return $this->repo;
	}

	/**
	 * Returns true if the image is an old version
	 * STUB
	 */
	function isOld() {
		return false;
	}

	/**
	 * Is this file a "deleted" file in a private archive?
	 * STUB
	 */
	function isDeleted( $field ) {
		return false;
	}

	/**
	 * Was this file ever deleted from the wiki?
	 *
	 * @return bool
	 */
	function wasDeleted() {
		$title = $this->getTitle();
		return $title && $title->isDeleted() > 0;
	}

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
		$this->readOnlyError();
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
	 * @param $suppress, hide content from sysops?
	 * @return true on success, false on some kind of failure
	 * STUB
	 * Overridden by LocalFile
	 */
	function delete( $reason, $suppress = false ) {
		$this->readOnlyError();
	}

	/**
	 * Restore all or specified deleted revisions to the given file.
	 * Permissions and logging are left to the caller.
	 *
	 * May throw database exceptions on error.
	 *
	 * @param $versions set of record ids of deleted items to restore,
	 *                    or empty to restore all revisions.
	 * @param $unsuppress, remove restrictions on content upon restoration?
	 * @return the number of file revisions restored if successful,
	 *         or false on failure
	 * STUB
	 * Overridden by LocalFile
	 */
	function restore( $versions=array(), $unsuppress=false ) {
		$this->readOnlyError();
	}

	/**
	 * Returns 'true' if this image is a multipage document, e.g. a DJVU
	 * document.
	 *
	 * @return Bool
	 */
	function isMultipage() {
		return $this->getHandler() && $this->handler->isMultiPage( $this );
	}

	/**
	 * Returns the number of pages of a multipage document, or NULL for
	 * documents which aren't multipage documents
	 */
	function pageCount() {
		if ( !isset( $this->pageCount ) ) {
			if ( $this->getHandler() && $this->handler->isMultiPage( $this ) ) {
				$this->pageCount = $this->handler->pageCount( $this );
			} else {
				$this->pageCount = false;
			}
		}
		return $this->pageCount;
	}

	/**
	 * Calculate the height of a thumbnail using the source and destination width
	 */
	static function scaleHeight( $srcWidth, $srcHeight, $dstWidth ) {
		// Exact integer multiply followed by division
		if ( $srcWidth == 0 ) {
			return 0;
		} else {
			return round( $srcHeight * $dstWidth / $srcWidth );
		}
	}

	/**
	 * Get an image size array like that returned by getimagesize(), or false if it
	 * can't be determined.
	 *
	 * @param string $fileName The filename
	 * @return array
	 */
	function getImageSize( $fileName ) {
		if ( !$this->getHandler() ) {
			return false;
		}
		return $this->handler->getImageSize( $this, $fileName );
	}

	/**
	 * Get the URL of the image description page. May return false if it is
	 * unknown or not applicable.
	 */
	function getDescriptionUrl() {
		return $this->repo->getDescriptionUrl( $this->getName() );
	}

	/**
	 * Get the HTML text of the description page, if available
	 */
	function getDescriptionText() {
		global $wgMemc;
		if ( !$this->repo->fetchDescription ) {
			return false;
		}
		$renderUrl = $this->repo->getDescriptionRenderUrl( $this->getName() );
		if ( $renderUrl ) {
			if ( $this->repo->descriptionCacheExpiry > 0 ) {
				wfDebug("Attempting to get the description from cache...");
				$key = wfMemcKey( 'RemoteFileDescription', 'url', md5($renderUrl) );
				$obj = $wgMemc->get($key);
				if ($obj) {
					wfDebug("success!\n");
					return $obj;
				}
				wfDebug("miss\n");
			}
			wfDebug( "Fetching shared description from $renderUrl\n" );
			$res = Http::get( $renderUrl );
			if ( $res && $this->repo->descriptionCacheExpiry > 0 ) $wgMemc->set( $key, $res, $this->repo->descriptionCacheExpiry );
			return $res;
		} else {
			return false;
		}
	}

	/**
	 * Get discription of file revision
	 * STUB
	 */
	function getDescription() {
		return null;
	}

	/**
	 * Get the 14-character timestamp of the file upload, or false if
	 * it doesn't exist
	 */
	function getTimestamp() {
		$path = $this->getPath();
		if ( !file_exists( $path ) ) {
			return false;
		}
		return wfTimestamp( TS_MW, filemtime( $path ) );
	}

	/**
	 * Get the SHA-1 base 36 hash of the file
	 */
	function getSha1() {
		return self::sha1Base36( $this->getPath() );
	}

	/**
	 * Determine if the current user is allowed to view a particular
	 * field of this file, if it's marked as deleted.
	 * STUB
	 * @param int $field
	 * @return bool
	 */
	function userCan( $field ) {
		return true;
	}

	/**
	 * Get an associative array containing information about a file in the local filesystem.
	 *
	 * @param string $path Absolute local filesystem path
	 * @param mixed $ext The file extension, or true to extract it from the filename.
	 *                   Set it to false to ignore the extension.
	 */
	static function getPropsFromPath( $path, $ext = true ) {
		wfProfileIn( __METHOD__ );
		wfDebug( __METHOD__.": Getting file info for $path\n" );
		$info = array(
			'fileExists' => file_exists( $path ) && !is_dir( $path )
		);
		$gis = false;
		if ( $info['fileExists'] ) {
			$magic = MimeMagic::singleton();

			$info['mime'] = $magic->guessMimeType( $path, $ext );
			list( $info['major_mime'], $info['minor_mime'] ) = self::splitMime( $info['mime'] );
			$info['media_type'] = $magic->getMediaType( $path, $info['mime'] );

			# Get size in bytes
			$info['size'] = filesize( $path );

			# Height, width and metadata
			$handler = MediaHandler::getHandler( $info['mime'] );
			if ( $handler ) {
				$tempImage = (object)array();
				$info['metadata'] = $handler->getMetadata( $tempImage, $path );
				$gis = $handler->getImageSize( $tempImage, $path, $info['metadata'] );
			} else {
				$gis = false;
				$info['metadata'] = '';
			}
			$info['sha1'] = self::sha1Base36( $path );

			wfDebug(__METHOD__.": $path loaded, {$info['size']} bytes, {$info['mime']}.\n");
		} else {
			$info['mime'] = NULL;
			$info['media_type'] = MEDIATYPE_UNKNOWN;
			$info['metadata'] = '';
			$info['sha1'] = '';
			wfDebug(__METHOD__.": $path NOT FOUND!\n");
		}
		if( $gis ) {
			# NOTE: $gis[2] contains a code for the image type. This is no longer used.
			$info['width'] = $gis[0];
			$info['height'] = $gis[1];
			if ( isset( $gis['bits'] ) ) {
				$info['bits'] = $gis['bits'];
			} else {
				$info['bits'] = 0;
			}
		} else {
			$info['width'] = 0;
			$info['height'] = 0;
			$info['bits'] = 0;
		}
		wfProfileOut( __METHOD__ );
		return $info;
	}

	/**
	 * Get a SHA-1 hash of a file in the local filesystem, in base-36 lower case
	 * encoding, zero padded to 31 digits.
	 *
	 * 160 log 2 / log 36 = 30.95, so the 160-bit hash fills 31 digits in base 36
	 * fairly neatly.
	 *
	 * Returns false on failure
	 */
	static function sha1Base36( $path ) {
		wfSuppressWarnings();
		$hash = sha1_file( $path );
		wfRestoreWarnings();
		if ( $hash === false ) {
			return false;
		} else {
			return wfBaseConvert( $hash, 16, 36, 31 );
		}
	}

	function getLongDesc() {
		$handler = $this->getHandler();
		if ( $handler ) {
			return $handler->getLongDesc( $this );
		} else {
			return MediaHandler::getLongDesc( $this );
		}
	}

	function getShortDesc() {
		$handler = $this->getHandler();
		if ( $handler ) {
			return $handler->getShortDesc( $this );
		} else {
			return MediaHandler::getShortDesc( $this );
		}
	}

	function getDimensionsString() {
		$handler = $this->getHandler();
		if ( $handler ) {
			return $handler->getDimensionsString( $this );
		} else {
			return '';
		}
	}

	function getRedirected() {
		return $this->redirected;
	}
	
	function getRedirectedTitle() {
		if ( $this->redirected ) {
			if ( !$this->redirectTitle )
				$this->redirectTitle = Title::makeTitle( NS_IMAGE, $this->redirected );
			return $this->redirectTitle;
		}
	}

	function redirectedFrom( $from ) {
		$this->redirected = $from;
	}
}
/**
 * Aliases for backwards compatibility with 1.6
 */
define( 'MW_IMG_DELETED_FILE', File::DELETED_FILE );
define( 'MW_IMG_DELETED_COMMENT', File::DELETED_COMMENT );
define( 'MW_IMG_DELETED_USER', File::DELETED_USER );
define( 'MW_IMG_DELETED_RESTRICTED', File::DELETED_RESTRICTED );
