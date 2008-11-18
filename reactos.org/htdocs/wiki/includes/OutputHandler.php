<?php

/**
 * Standard output handler for use with ob_start
 */
function wfOutputHandler( $s ) {
	global $wgDisableOutputCompression, $wgValidateAllHtml;
    $s = wfMangleFlashPolicy( $s );
    if ( $wgValidateAllHtml ) {
		$headers = apache_response_headers();
		$isHTML = true;
		foreach ( $headers as $name => $value ) {
			if ( strtolower( $name ) == 'content-type' && strpos( $value, 'text/html' ) === false ) {
				$isHTML = false;
				break;
			}
		}
		if ( $isHTML ) {
			$s = wfHtmlValidationHandler( $s );
		}
	}
	if ( !$wgDisableOutputCompression && !ini_get( 'zlib.output_compression' ) ) {
		if ( !defined( 'MW_NO_OUTPUT_COMPRESSION' ) ) {
			$s = wfGzipHandler( $s );
		}
		if ( !ini_get( 'output_handler' ) ) {
			wfDoContentLength( strlen( $s ) );
		}
	}
	return $s;
}

/**
 * Get the "file extension" that some client apps will estimate from
 * the currently-requested URL.
 * This isn't on WebRequest because we need it when things aren't initialized
 * @private
 */
function wfRequestExtension() {
	/// @fixme -- this sort of dupes some code in WebRequest::getRequestUrl()
	if( isset( $_SERVER['REQUEST_URI'] ) ) {
		// Strip the query string...
		list( $path ) = explode( '?', $_SERVER['REQUEST_URI'], 2 );
	} elseif( isset( $_SERVER['SCRIPT_NAME'] ) ) {
		// Probably IIS. QUERY_STRING appears separately.
		$path = $_SERVER['SCRIPT_NAME'];
	} else {
		// Can't get the path from the server? :(
		return '';
	}

	$period = strrpos( $path, '.' );
	if( $period !== false ) {
		return strtolower( substr( $path, $period ) );
	}
	return '';
}

/**
 * Handler that compresses data with gzip if allowed by the Accept header.
 * Unlike ob_gzhandler, it works for HEAD requests too.
 */
function wfGzipHandler( $s ) {
	if( !function_exists( 'gzencode' ) || headers_sent() ) {
		return $s;
	}

	$ext = wfRequestExtension();
	if( $ext == '.gz' || $ext == '.tgz' ) {
		// Don't do gzip compression if the URL path ends in .gz or .tgz
		// This confuses Safari and triggers a download of the page,
		// even though it's pretty clearly labeled as viewable HTML.
		// Bad Safari! Bad!
		return $s;
	}

	if( isset( $_SERVER['HTTP_ACCEPT_ENCODING'] ) ) {
		$tokens = preg_split( '/[,; ]/', $_SERVER['HTTP_ACCEPT_ENCODING'] );
		if ( in_array( 'gzip', $tokens ) ) {
			header( 'Content-Encoding: gzip' );
			$s = gzencode( $s, 3 );
		}
	}

	// Set vary header if it hasn't been set already
	$headers = headers_list();
	$foundVary = false;
	foreach ( $headers as $header ) {
		if ( substr( $header, 0, 5 ) == 'Vary:' ) {
			$foundVary = true;
			break;
		}
	}
	if ( !$foundVary ) {
		header( 'Vary: Accept-Encoding' );
		header( 'X-Vary-Options: Accept-Encoding;list-contains=gzip' );
	}
	return $s;
}

/**
 * Mangle flash policy tags which open up the site to XSS attacks.
 */
function wfMangleFlashPolicy( $s ) {
	# Avoid weird excessive memory usage in PCRE on big articles
	if ( preg_match( '/\<\s*cross-domain-policy\s*\>/i', $s ) ) {
		return preg_replace( '/\<\s*cross-domain-policy\s*\>/i', '<NOT-cross-domain-policy>', $s );
	} else {
		return $s;
	}
}

/**
 * Add a Content-Length header if possible. This makes it cooperate with squid better.
 */
function wfDoContentLength( $length ) {
	if ( !headers_sent() && $_SERVER['SERVER_PROTOCOL'] == 'HTTP/1.0' ) {
		header( "Content-Length: $length" );
	}
}

/**
 * Replace the output with an error if the HTML is not valid
 */
function wfHtmlValidationHandler( $s ) {
	global $IP;
	$tidy = new tidy;
	$tidy->parseString( $s, "$IP/includes/tidy.conf", 'utf8' );
	if ( $tidy->getStatus() == 0 ) {
		return $s;
	}

	header( 'Cache-Control: no-cache' );

	$out = <<<EOT
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<title>HTML validation error</title>
<style>
.highlight { background-color: #ffc }
li { white-space: pre }
</style>
</head>
<body>
<h1>HTML validation error</h1>
<ul>
EOT;

	$error = strtok( $tidy->errorBuffer, "\n" );
	$badLines = array();
	while ( $error !== false ) {
		if ( preg_match( '/^line (\d+)/', $error, $m ) ) {
			$lineNum = intval( $m[1] );
			$badLines[$lineNum] = true;
			$out .= "<li><a href=\"#line-{$lineNum}\">" . htmlspecialchars( $error ) . "</a></li>\n";
		}
		$error = strtok( "\n" );
	}

	$out .= '<pre>' . htmlspecialchars( $tidy->errorBuffer ) . '</pre>';
	$out .= '<ol>';
	$line = strtok( $s, "\n" );
	$i = 1;
	while ( $line !== false ) {
		if ( isset( $badLines[$i] ) ) {
			$out .= "<li class=\"highlight\" id=\"line-$i\">";
		} else {
			$out .= '<li>';
		}
		$out .= htmlspecialchars( $line ) . '</li>';
		$line = strtok( "\n" );
		$i++;
	}
	$out .= '</ol></body></html>';
	return $out;
}
