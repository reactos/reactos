<?php
/**
 * Media-handling base classes and generic functionality
 * @file
 * @ingroup Media
 */

/**
 * Base media handler class
 *
 * @ingroup Media
 */
abstract class MediaHandler {
	const TRANSFORM_LATER = 1;

	/**
	 * Instance cache
	 */
	static $handlers = array();

	/**
	 * Get a MediaHandler for a given MIME type from the instance cache
	 */
	static function getHandler( $type ) {
		global $wgMediaHandlers;
		if ( !isset( $wgMediaHandlers[$type] ) ) {
			wfDebug( __METHOD__ . ": no handler found for $type.\n");
			return false;
		}
		$class = $wgMediaHandlers[$type];
		if ( !isset( self::$handlers[$class] ) ) {
			self::$handlers[$class] = new $class;
			if ( !self::$handlers[$class]->isEnabled() ) {
				self::$handlers[$class] = false;
			}
		}
		return self::$handlers[$class];
	}

	/**
	 * Get an associative array mapping magic word IDs to parameter names.
	 * Will be used by the parser to identify parameters.
	 */
	abstract function getParamMap();

	/*
	 * Validate a thumbnail parameter at parse time.
	 * Return true to accept the parameter, and false to reject it.
	 * If you return false, the parser will do something quiet and forgiving.
	 */
	abstract function validateParam( $name, $value );

	/**
	 * Merge a parameter array into a string appropriate for inclusion in filenames
	 */
	abstract function makeParamString( $params );

	/**
	 * Parse a param string made with makeParamString back into an array
	 */
	abstract function parseParamString( $str );

	/**
	 * Changes the parameter array as necessary, ready for transformation.
	 * Should be idempotent.
	 * Returns false if the parameters are unacceptable and the transform should fail
	 */
	abstract function normaliseParams( $image, &$params );

	/**
	 * Get an image size array like that returned by getimagesize(), or false if it
	 * can't be determined.
	 *
	 * @param Image $image The image object, or false if there isn't one
	 * @param string $fileName The filename
	 * @return array
	 */
	abstract function getImageSize( $image, $path );

	/**
	 * Get handler-specific metadata which will be saved in the img_metadata field.
	 *
	 * @param Image $image The image object, or false if there isn't one
	 * @param string $fileName The filename
	 * @return string
	 */
	function getMetadata( $image, $path ) { return ''; }

	/**
	 * Get a string describing the type of metadata, for display purposes.
	 */
	function getMetadataType( $image ) { return false; }

	/**
	 * Check if the metadata string is valid for this handler.
	 * If it returns false, Image will reload the metadata from the file and update the database
	 */
	function isMetadataValid( $image, $metadata ) { return true; }


	/**
	 * Get a MediaTransformOutput object representing an alternate of the transformed
	 * output which will call an intermediary thumbnail assist script.
	 *
	 * Used when the repository has a thumbnailScriptUrl option configured.
	 *
	 * Return false to fall back to the regular getTransform().
	 */
	function getScriptedTransform( $image, $script, $params ) {
		return false;
	}

	/**
	 * Get a MediaTransformOutput object representing the transformed output. Does not
	 * actually do the transform.
	 *
	 * @param Image $image The image object
	 * @param string $dstPath Filesystem destination path
	 * @param string $dstUrl Destination URL to use in output HTML
	 * @param array $params Arbitrary set of parameters validated by $this->validateParam()
	 */
	function getTransform( $image, $dstPath, $dstUrl, $params ) {
		return $this->doTransform( $image, $dstPath, $dstUrl, $params, self::TRANSFORM_LATER );
	}

	/**
	 * Get a MediaTransformOutput object representing the transformed output. Does the
	 * transform unless $flags contains self::TRANSFORM_LATER.
	 *
	 * @param Image $image The image object
	 * @param string $dstPath Filesystem destination path
	 * @param string $dstUrl Destination URL to use in output HTML
	 * @param array $params Arbitrary set of parameters validated by $this->validateParam()
	 * @param integer $flags A bitfield, may contain self::TRANSFORM_LATER
	 */
	abstract function doTransform( $image, $dstPath, $dstUrl, $params, $flags = 0 );

	/**
	 * Get the thumbnail extension and MIME type for a given source MIME type
	 * @return array thumbnail extension and MIME type
	 */
	function getThumbType( $ext, $mime ) {
		return array( $ext, $mime );
	}

	/**
	 * True if the handled types can be transformed
	 */
	function canRender( $file ) { return true; }
	/**
	 * True if handled types cannot be displayed directly in a browser
	 * but can be rendered
	 */
	function mustRender( $file ) { return false; }
	/**
	 * True if the type has multi-page capabilities
	 */
	function isMultiPage( $file ) { return false; }
	/**
	 * Page count for a multi-page document, false if unsupported or unknown
	 */
	function pageCount( $file ) { return false; }
	/**
	 * False if the handler is disabled for all files
	 */
	function isEnabled() { return true; }

	/**
	 * Get an associative array of page dimensions
	 * Currently "width" and "height" are understood, but this might be
	 * expanded in the future.
	 * Returns false if unknown or if the document is not multi-page.
	 */
	function getPageDimensions( $image, $page ) {
		$gis = $this->getImageSize( $image, $image->getPath() );
		return array(
			'width' => $gis[0],
			'height' => $gis[1]
		);
	}

	/**
	 * Get an array structure that looks like this:
	 *
	 * array(
	 *    'visible' => array(
	 *       'Human-readable name' => 'Human readable value',
	 *       ...
	 *    ),
	 *    'collapsed' => array(
	 *       'Human-readable name' => 'Human readable value',
	 *       ...
	 *    )
	 * )
	 * The UI will format this into a table where the visible fields are always
	 * visible, and the collapsed fields are optionally visible.
	 *
	 * The function should return false if there is no metadata to display.
	 */

	/**
	 * FIXME: I don't really like this interface, it's not very flexible
	 * I think the media handler should generate HTML instead. It can do
	 * all the formatting according to some standard. That makes it possible
	 * to do things like visual indication of grouped and chained streams
	 * in ogg container files.
	 */
	function formatMetadata( $image ) {
		return false;
	}

	/**
	 * @fixme document this!
	 * 'value' thingy goes into a wikitext table; it used to be escaped but
	 * that was incompatible with previous practice of customized display
	 * with wikitext formatting via messages such as 'exif-model-value'.
	 * So the escaping is taken back out, but generally this seems a confusing
	 * interface.
	 */
	protected static function addMeta( &$array, $visibility, $type, $id, $value, $param = false ) {
		$array[$visibility][] = array(
			'id' => "$type-$id",
			'name' => wfMsg( "$type-$id", $param ),
			'value' => $value
		);
	}

	function getShortDesc( $file ) {
		global $wgLang;
		$nbytes = '(' . wfMsgExt( 'nbytes', array( 'parsemag', 'escape' ),
			$wgLang->formatNum( $file->getSize() ) ) . ')';
		return "$nbytes";
	}

	function getLongDesc( $file ) {
		global $wgUser;
		$sk = $wgUser->getSkin();
		return wfMsgExt( 'file-info', 'parseinline',
			$sk->formatSize( $file->getSize() ),
			$file->getMimeType() );
	}

	function getDimensionsString( $file ) {
		return '';
	}

	/**
	 * Modify the parser object post-transform
	 */
	function parserTransformHook( $parser, $file ) {}

	/**
	 * Check for zero-sized thumbnails. These can be generated when
	 * no disk space is available or some other error occurs
	 *
	 * @param $dstPath The location of the suspect file
	 * @param $retval Return value of some shell process, file will be deleted if this is non-zero
	 * @return true if removed, false otherwise
	 */
	function removeBadFile( $dstPath, $retval = 0 ) {
		if( file_exists( $dstPath ) ) {
			$thumbstat = stat( $dstPath );
			if( $thumbstat['size'] == 0 || $retval != 0 ) {
				wfDebugLog( 'thumbnail',
					sprintf( 'Removing bad %d-byte thumbnail "%s"',
						$thumbstat['size'], $dstPath ) );
				unlink( $dstPath );
				return true;
			}
		}
		return false;
	}
}

/**
 * Media handler abstract base class for images
 *
 * @ingroup Media
 */
abstract class ImageHandler extends MediaHandler {
	function canRender( $file ) {
		if ( $file->getWidth() && $file->getHeight() ) {
			return true;
		} else {
			return false;
		}
	}

	function getParamMap() {
		return array( 'img_width' => 'width' );
	}

	function validateParam( $name, $value ) {
		if ( in_array( $name, array( 'width', 'height' ) ) ) {
			if ( $value <= 0 ) {
				return false;
			} else {
				return true;
			}
		} else {
			return false;
		}
	}

	function makeParamString( $params ) {
		if ( isset( $params['physicalWidth'] ) ) {
			$width = $params['physicalWidth'];
		} elseif ( isset( $params['width'] ) ) {
			$width = $params['width'];
		} else {
			throw new MWException( 'No width specified to '.__METHOD__ );
		}
		# Removed for ProofreadPage
		#$width = intval( $width );
		return "{$width}px";
	}

	function parseParamString( $str ) {
		$m = false;
		if ( preg_match( '/^(\d+)px$/', $str, $m ) ) {
			return array( 'width' => $m[1] );
		} else {
			return false;
		}
	}

	function getScriptParams( $params ) {
		return array( 'width' => $params['width'] );
	}

	function normaliseParams( $image, &$params ) {
		$mimeType = $image->getMimeType();

		if ( !isset( $params['width'] ) ) {
			return false;
		}
		if ( !isset( $params['page'] ) ) {
			$params['page'] = 1;
		}
		$srcWidth = $image->getWidth( $params['page'] );
		$srcHeight = $image->getHeight( $params['page'] );
		if ( isset( $params['height'] ) && $params['height'] != -1 ) {
			if ( $params['width'] * $srcHeight > $params['height'] * $srcWidth ) {
				$params['width'] = wfFitBoxWidth( $srcWidth, $srcHeight, $params['height'] );
			}
		}
		$params['height'] = File::scaleHeight( $srcWidth, $srcHeight, $params['width'] );
		if ( !$this->validateThumbParams( $params['width'], $params['height'], $srcWidth, $srcHeight, $mimeType ) ) {
			return false;
		}
		return true;
	}

	/**
	 * Get a transform output object without actually doing the transform
	 */
	function getTransform( $image, $dstPath, $dstUrl, $params ) {
		return $this->doTransform( $image, $dstPath, $dstUrl, $params, self::TRANSFORM_LATER );
	}

	/**
	 * Validate thumbnail parameters and fill in the correct height
	 *
	 * @param integer &$width Specified width (input/output)
	 * @param integer &$height Height (output only)
	 * @return false to indicate that an error should be returned to the user.
	 */
	function validateThumbParams( &$width, &$height, $srcWidth, $srcHeight, $mimeType ) {
		$width = intval( $width );

		# Sanity check $width
		if( $width <= 0) {
			wfDebug( __METHOD__.": Invalid destination width: $width\n" );
			return false;
		}
		if ( $srcWidth <= 0 ) {
			wfDebug( __METHOD__.": Invalid source width: $srcWidth\n" );
			return false;
		}

		$height = File::scaleHeight( $srcWidth, $srcHeight, $width );
		return true;
	}

	function getScriptedTransform( $image, $script, $params ) {
		if ( !$this->normaliseParams( $image, $params ) ) {
			return false;
		}
		$url = $script . '&' . wfArrayToCGI( $this->getScriptParams( $params ) );
		$page = isset( $params['page'] ) ? $params['page'] : false;

		if( $image->mustRender() || $params['width'] < $image->getWidth() ) {
			return new ThumbnailImage( $image, $url, $params['width'], $params['height'], $page );
		}
	}

	function getImageSize( $image, $path ) {
		wfSuppressWarnings();
		$gis = getimagesize( $path );
		wfRestoreWarnings();
		return $gis;
	}

	function getShortDesc( $file ) {
		global $wgLang;
		$nbytes = wfMsgExt( 'nbytes', array( 'parsemag', 'escape' ),
			$wgLang->formatNum( $file->getSize() ) );
		$widthheight = wfMsgHtml( 'widthheight', $wgLang->formatNum( $file->getWidth() ) ,$wgLang->formatNum( $file->getHeight() ) );

		return "$widthheight ($nbytes)";
	}

	function getLongDesc( $file ) {
		global $wgLang;
		return wfMsgExt('file-info-size', 'parseinline',
			$wgLang->formatNum( $file->getWidth() ),
			$wgLang->formatNum( $file->getHeight() ),
			$wgLang->formatSize( $file->getSize() ),
			$file->getMimeType() );
	}

	function getDimensionsString( $file ) {
		global $wgLang;
		$pages = $file->pageCount();
		$width = $wgLang->formatNum( $file->getWidth() );
		$height = $wgLang->formatNum( $file->getHeight() );
		$pagesFmt = $wgLang->formatNum( $pages );

		if ( $pages > 1 ) {
			return wfMsgExt( 'widthheightpage', 'parsemag', $width, $height, $pagesFmt );
		} else {
			return wfMsg( 'widthheight', $width, $height );
		}
	}
}
