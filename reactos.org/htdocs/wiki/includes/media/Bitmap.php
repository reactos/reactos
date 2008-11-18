<?php
/**
 * @file
 * @ingroup Media
 */

/**
 * @ingroup Media
 */
class BitmapHandler extends ImageHandler {
	function normaliseParams( $image, &$params ) {
		global $wgMaxImageArea;
		if ( !parent::normaliseParams( $image, $params ) ) {
			return false;
		}

		$mimeType = $image->getMimeType();
		$srcWidth = $image->getWidth( $params['page'] );
		$srcHeight = $image->getHeight( $params['page'] );

		# Don't thumbnail an image so big that it will fill hard drives and send servers into swap
		# JPEG has the handy property of allowing thumbnailing without full decompression, so we make
		# an exception for it.
		if ( $mimeType !== 'image/jpeg' &&
			$srcWidth * $srcHeight > $wgMaxImageArea )
		{
			return false;
		}

		# Don't make an image bigger than the source
		$params['physicalWidth'] = $params['width'];
		$params['physicalHeight'] = $params['height'];

		if ( $params['physicalWidth'] >= $srcWidth ) {
			$params['physicalWidth'] = $srcWidth;
			$params['physicalHeight'] = $srcHeight;
			return true;
		}

		return true;
	}

	function doTransform( $image, $dstPath, $dstUrl, $params, $flags = 0 ) {
		global $wgUseImageMagick, $wgImageMagickConvertCommand;
		global $wgCustomConvertCommand;
		global $wgSharpenParameter, $wgSharpenReductionThreshold;

		if ( !$this->normaliseParams( $image, $params ) ) {
			return new TransformParameterError( $params );
		}
		$physicalWidth = $params['physicalWidth'];
		$physicalHeight = $params['physicalHeight'];
		$clientWidth = $params['width'];
		$clientHeight = $params['height'];
		$srcWidth = $image->getWidth();
		$srcHeight = $image->getHeight();
		$mimeType = $image->getMimeType();
		$srcPath = $image->getPath();
		$retval = 0;
		wfDebug( __METHOD__.": creating {$physicalWidth}x{$physicalHeight} thumbnail at $dstPath\n" );

		if ( $physicalWidth == $srcWidth && $physicalHeight == $srcHeight ) {
			# normaliseParams (or the user) wants us to return the unscaled image
			wfDebug( __METHOD__.": returning unscaled image\n" );
			return new ThumbnailImage( $image, $image->getURL(), $clientWidth, $clientHeight, $srcPath );
		}

		if ( !$dstPath ) {
			// No output path available, client side scaling only
			$scaler = 'client';
		} elseif ( $wgUseImageMagick ) {
			$scaler = 'im';
		} elseif ( $wgCustomConvertCommand ) {
			$scaler = 'custom';
		} elseif ( function_exists( 'imagecreatetruecolor' ) ) {
			$scaler = 'gd';
		} else {
			$scaler = 'client';
		}

		if ( $scaler == 'client' ) {
			# Client-side image scaling, use the source URL
			# Using the destination URL in a TRANSFORM_LATER request would be incorrect
			return new ThumbnailImage( $image, $image->getURL(), $clientWidth, $clientHeight, $srcPath );
		}

		if ( $flags & self::TRANSFORM_LATER ) {
			return new ThumbnailImage( $image, $dstUrl, $clientWidth, $clientHeight, $dstPath );
		}

		if ( !wfMkdirParents( dirname( $dstPath ) ) ) {
			wfDebug( "Unable to create thumbnail destination directory, falling back to client scaling\n" );
			return new ThumbnailImage( $image, $image->getURL(), $clientWidth, $clientHeight, $srcPath );
		}

		if ( $scaler == 'im' ) {
			# use ImageMagick

			$sharpen = '';
			if ( $mimeType == 'image/jpeg' ) {
				$quality = "-quality 80"; // 80%
				# Sharpening, see bug 6193
				if ( ( $physicalWidth + $physicalHeight ) / ( $srcWidth + $srcHeight ) < $wgSharpenReductionThreshold ) {
					$sharpen = "-sharpen " . wfEscapeShellArg( $wgSharpenParameter );
				}
			} elseif ( $mimeType == 'image/png' ) {
				$quality = "-quality 95"; // zlib 9, adaptive filtering
			} else {
				$quality = ''; // default
			}

			# Specify white background color, will be used for transparent images
			# in Internet Explorer/Windows instead of default black.

			# Note, we specify "-size {$physicalWidth}" and NOT "-size {$physicalWidth}x{$physicalHeight}".
			# It seems that ImageMagick has a bug wherein it produces thumbnails of
			# the wrong size in the second case.

			$cmd  =  wfEscapeShellArg($wgImageMagickConvertCommand) .
				" {$quality} -background white -size {$physicalWidth} ".
				wfEscapeShellArg($srcPath) .
				// Coalesce is needed to scale animated GIFs properly (bug 1017).
				' -coalesce ' .
				// For the -resize option a "!" is needed to force exact size,
				// or ImageMagick may decide your ratio is wrong and slice off
				// a pixel.
				" -thumbnail " . wfEscapeShellArg( "{$physicalWidth}x{$physicalHeight}!" ) .
				" -depth 8 $sharpen " .
				wfEscapeShellArg($dstPath) . " 2>&1";
			wfDebug( __METHOD__.": running ImageMagick: $cmd\n");
			wfProfileIn( 'convert' );
			$err = wfShellExec( $cmd, $retval );
			wfProfileOut( 'convert' );
		} elseif( $scaler == 'custom' ) {
			# Use a custom convert command
			# Variables: %s %d %w %h
			$src = wfEscapeShellArg( $srcPath );
			$dst = wfEscapeShellArg( $dstPath );
			$cmd = $wgCustomConvertCommand;
			$cmd = str_replace( '%s', $src, str_replace( '%d', $dst, $cmd ) ); # Filenames
			$cmd = str_replace( '%h', $physicalHeight, str_replace( '%w', $physicalWidth, $cmd ) ); # Size
			wfDebug( __METHOD__.": Running custom convert command $cmd\n" );
			wfProfileIn( 'convert' );
			$err = wfShellExec( $cmd, $retval );
			wfProfileOut( 'convert' );
		} else /* $scaler == 'gd' */ {
			# Use PHP's builtin GD library functions.
			#
			# First find out what kind of file this is, and select the correct
			# input routine for this.

			$typemap = array(
				'image/gif'          => array( 'imagecreatefromgif',  'palette',   'imagegif'  ),
				'image/jpeg'         => array( 'imagecreatefromjpeg', 'truecolor', array( __CLASS__, 'imageJpegWrapper' ) ),
				'image/png'          => array( 'imagecreatefrompng',  'bits',      'imagepng'  ),
				'image/vnd.wap.wbmp' => array( 'imagecreatefromwbmp', 'palette',   'imagewbmp'  ),
				'image/xbm'          => array( 'imagecreatefromxbm',  'palette',   'imagexbm'  ),
			);
			if( !isset( $typemap[$mimeType] ) ) {
				$err = 'Image type not supported';
				wfDebug( "$err\n" );
				return new MediaTransformError( 'thumbnail_error', $clientWidth, $clientHeight, $err );
			}
			list( $loader, $colorStyle, $saveType ) = $typemap[$mimeType];

			if( !function_exists( $loader ) ) {
				$err = "Incomplete GD library configuration: missing function $loader";
				wfDebug( "$err\n" );
				return new MediaTransformError( 'thumbnail_error', $clientWidth, $clientHeight, $err );
			}

			$src_image = call_user_func( $loader, $srcPath );
			$dst_image = imagecreatetruecolor( $physicalWidth, $physicalHeight );

			// Initialise the destination image to transparent instead of
			// the default solid black, to support PNG and GIF transparency nicely
			$background = imagecolorallocate( $dst_image, 0, 0, 0 );
			imagecolortransparent( $dst_image, $background );
			imagealphablending( $dst_image, false );

			if( $colorStyle == 'palette' ) {
				// Don't resample for paletted GIF images.
				// It may just uglify them, and completely breaks transparency.
				imagecopyresized( $dst_image, $src_image,
					0,0,0,0,
					$physicalWidth, $physicalHeight, imagesx( $src_image ), imagesy( $src_image ) );
			} else {
				imagecopyresampled( $dst_image, $src_image,
					0,0,0,0,
					$physicalWidth, $physicalHeight, imagesx( $src_image ), imagesy( $src_image ) );
			}

			imagesavealpha( $dst_image, true );

			call_user_func( $saveType, $dst_image, $dstPath );
			imagedestroy( $dst_image );
			imagedestroy( $src_image );
			$retval = 0;
		}

		$removed = $this->removeBadFile( $dstPath, $retval );
		if ( $retval != 0 || $removed ) {
			wfDebugLog( 'thumbnail',
				sprintf( 'thumbnail failed on %s: error %d "%s" from "%s"',
					wfHostname(), $retval, trim($err), $cmd ) );
			return new MediaTransformError( 'thumbnail_error', $clientWidth, $clientHeight, $err );
		} else {
			return new ThumbnailImage( $image, $dstUrl, $clientWidth, $clientHeight, $dstPath );
		}
	}

	static function imageJpegWrapper( $dst_image, $thumbPath ) {
		imageinterlace( $dst_image );
		imagejpeg( $dst_image, $thumbPath, 95 );
	}


	function getMetadata( $image, $filename ) {
		global $wgShowEXIF;
		if( $wgShowEXIF && file_exists( $filename ) ) {
			$exif = new Exif( $filename );
			$data = $exif->getFilteredData();
			if ( $data ) {
				$data['MEDIAWIKI_EXIF_VERSION'] = Exif::version();
				return serialize( $data );
			} else {
				return '0';
			}
		} else {
			return '';
		}
	}

	function getMetadataType( $image ) {
		return 'exif';
	}

	function isMetadataValid( $image, $metadata ) {
		global $wgShowEXIF;
		if ( !$wgShowEXIF ) {
			# Metadata disabled and so an empty field is expected
			return true;
		}
		if ( $metadata === '0' ) {
			# Special value indicating that there is no EXIF data in the file
			return true;
		}
		$exif = @unserialize( $metadata );
		if ( !isset( $exif['MEDIAWIKI_EXIF_VERSION'] ) ||
			$exif['MEDIAWIKI_EXIF_VERSION'] != Exif::version() )
		{
			# Wrong version
			wfDebug( __METHOD__.": wrong version\n" );
			return false;
		}
		return true;
	}

	/**
	 * Get a list of EXIF metadata items which should be displayed when
	 * the metadata table is collapsed.
	 *
	 * @return array of strings
	 * @access private
	 */
	function visibleMetadataFields() {
		$fields = array();
		$lines = explode( "\n", wfMsgForContent( 'metadata-fields' ) );
		foreach( $lines as $line ) {
			$matches = array();
			if( preg_match( '/^\\*\s*(.*?)\s*$/', $line, $matches ) ) {
				$fields[] = $matches[1];
			}
		}
		$fields = array_map( 'strtolower', $fields );
		return $fields;
	}

	function formatMetadata( $image ) {
		$result = array(
			'visible' => array(),
			'collapsed' => array()
		);
		$metadata = $image->getMetadata();
		if ( !$metadata ) {
			return false;
		}
		$exif = unserialize( $metadata );
		if ( !$exif ) {
			return false;
		}
		unset( $exif['MEDIAWIKI_EXIF_VERSION'] );
		$format = new FormatExif( $exif );

		$formatted = $format->getFormattedData();
		// Sort fields into visible and collapsed
		$visibleFields = $this->visibleMetadataFields();
		foreach ( $formatted as $name => $value ) {
			$tag = strtolower( $name );
			self::addMeta( $result,
				in_array( $tag, $visibleFields ) ? 'visible' : 'collapsed',
				'exif',
				$tag,
				$value
			);
		}
		return $result;
	}
}
