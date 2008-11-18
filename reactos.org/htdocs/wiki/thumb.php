<?php

/**
 * PHP script to stream out an image thumbnail.
 *
 * @file
 * @ingroup Media
 */
define( 'MW_NO_OUTPUT_COMPRESSION', 1 );
require_once( './includes/WebStart.php' );

$wgTrivialMimeDetection = true; //don't use fancy mime detection, just check the file extension for jpg/gif/png.

require_once( "$IP/includes/StreamFile.php" );

wfThumbMain();
wfLogProfilingData();

//--------------------------------------------------------------------------

function wfThumbMain() {
	wfProfileIn( __METHOD__ );
	// Get input parameters
	if ( get_magic_quotes_gpc() ) {
		$params = array_map( 'stripslashes', $_REQUEST );
	} else {
		$params = $_REQUEST;
	}

	$fileName = isset( $params['f'] ) ? $params['f'] : '';
	unset( $params['f'] );

	// Backwards compatibility parameters
	if ( isset( $params['w'] ) ) {
		$params['width'] = $params['w'];
		unset( $params['w'] );
	}
	if ( isset( $params['p'] ) ) {
		$params['page'] = $params['p'];
	}
	unset( $params['r'] );

	// Some basic input validation
	$fileName = strtr( $fileName, '\\/', '__' );

	$img = wfLocalFile( $fileName );
	if ( !$img ) {
		wfThumbError( 404, wfMsg( 'badtitletext' ) );
		return;
	}
	if ( !$img->exists() ) {
		wfThumbError( 404, 'The source file for the specified thumbnail does not exist.' );
		return;
	}
	$sourcePath = $img->getPath();
	if ( $sourcePath === false ) {
		wfThumbError( 500, 'The source file is not locally accessible.' );
		return;
	}

	// Check IMS against the source file
	// This means that clients can keep a cached copy even after it has been deleted on the server
	if ( !empty( $_SERVER['HTTP_IF_MODIFIED_SINCE'] ) ) {
		// Fix IE brokenness
		$imsString = preg_replace( '/;.*$/', '', $_SERVER["HTTP_IF_MODIFIED_SINCE"] );
		// Calculate time
		wfSuppressWarnings();
		$imsUnix = strtotime( $imsString );
		wfRestoreWarnings();
		$stat = @stat( $sourcePath );
		if ( $stat['mtime'] <= $imsUnix ) {
			header( 'HTTP/1.1 304 Not Modified' );
			return;
		}
	}

	// Stream the file if it exists already
	try {
		if ( false != ( $thumbName = $img->thumbName( $params ) ) ) {
			$thumbPath = $img->getThumbPath( $thumbName );

			if ( is_file( $thumbPath ) ) {
				wfStreamFile( $thumbPath );
				return;
			}
		}
	} catch ( MWException $e ) {
		wfThumbError( 500, $e->getHTML() );
		return;
	}

	try {
		$thumb = $img->transform( $params, File::RENDER_NOW );
	} catch( Exception $ex ) {
		// Tried to select a page on a non-paged file?
		$thumb = false;
	}

	$errorMsg = false;
	if ( !$thumb ) {
		$errorMsg = wfMsgHtml( 'thumbnail_error', 'File::transform() returned false' );
	} elseif ( $thumb->isError() ) {
		$errorMsg = $thumb->getHtmlMsg();
	} elseif ( !$thumb->getPath() ) {
		$errorMsg = wfMsgHtml( 'thumbnail_error', 'No path supplied in thumbnail object' );
	} elseif ( $thumb->getPath() == $img->getPath() ) {
		$errorMsg = wfMsgHtml( 'thumbnail_error', 'Image was not scaled, ' .
			'is the requested width bigger than the source?' );
	} else {
		wfStreamFile( $thumb->getPath() );
	}
	if ( $errorMsg !== false ) {
		wfThumbError( 500, $errorMsg );
	}

	wfProfileOut( __METHOD__ );
}

function wfThumbError( $status, $msg ) {
	global $wgShowHostnames;
	header( 'Cache-Control: no-cache' );
	header( 'Content-Type: text/html; charset=utf-8' );
	if ( $status == 404 ) {
		header( 'HTTP/1.1 404 Not found' );
	} else {
		header( 'HTTP/1.1 500 Internal server error' );
	}
	if( $wgShowHostnames ) {
		$url = htmlspecialchars( @$_SERVER['REQUEST_URI'] );
		$hostname = htmlspecialchars( wfHostname() );
		$debug = "<!-- $url -->\n<!-- $hostname -->\n";
	} else {
		$debug = "";
	}
	echo <<<EOT
<html><head><title>Error generating thumbnail</title></head>
<body>
<h1>Error generating thumbnail</h1>
<p>
$msg
</p>
$debug
</body>
</html>

EOT;
}

