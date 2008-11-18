<?php
/**
 * @file
 * @ingroup Ajax
 */

if( !defined( 'MEDIAWIKI' ) ) {
	die( 1 );
}

/**
 * @todo document
 * @ingroup Ajax
 */
class AjaxResponse {

	/** Number of seconds to get the response cached by a proxy */
	private $mCacheDuration;

	/** HTTP header Content-Type */
	private $mContentType;

	/** @todo document */
	private $mDisabled;

	/** Date for the HTTP header Last-modified */
	private $mLastModified;

	/** HTTP response code */
	private $mResponseCode;

	/** HTTP Vary header */
	private $mVary;

	/** Content of our HTTP response */
	private $mText;

	function __construct( $text = NULL ) {
		$this->mCacheDuration = NULL;
		$this->mVary = NULL;

		$this->mDisabled = false;
		$this->mText = '';
		$this->mResponseCode = '200 OK';
		$this->mLastModified = false;
		$this->mContentType= 'text/html; charset=utf-8';

		if ( $text ) {
			$this->addText( $text );
		}
	}

	function setCacheDuration( $duration ) {
		$this->mCacheDuration = $duration;
	}

	function setVary( $vary ) {
		$this->mVary = $vary;
	}

	function setResponseCode( $code ) {
		$this->mResponseCode = $code;
	}

	function setContentType( $type ) {
		$this->mContentType = $type;
	}

	function disable() {
		$this->mDisabled = true;
	}

	/** Add content to the response */
	function addText( $text ) {
		if ( ! $this->mDisabled && $text ) {
			$this->mText .= $text;
		}
	}

	/** Output text */
	function printText() {
		if ( ! $this->mDisabled ) {
			print $this->mText;
		}
	}

	/** Construct the header and output it */
	function sendHeaders() {
		global $wgUseSquid, $wgUseESI;

		if ( $this->mResponseCode ) {
			$n = preg_replace( '/^ *(\d+)/', '\1', $this->mResponseCode );
			header( "Status: " . $this->mResponseCode, true, (int)$n );
		}

		header ("Content-Type: " . $this->mContentType );

		if ( $this->mLastModified ) {
			header ("Last-Modified: " . $this->mLastModified );
		}
		else {
			header ("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT");
		}

		if ( $this->mCacheDuration ) {

			# If squid caches are configured, tell them to cache the response,
			# and tell the client to always check with the squid. Otherwise,
			# tell the client to use a cached copy, without a way to purge it.

			if( $wgUseSquid ) {

				# Expect explicite purge of the proxy cache, but require end user agents
				# to revalidate against the proxy on each visit.
				# Surrogate-Control controls our Squid, Cache-Control downstream caches

				if ( $wgUseESI ) {
					header( 'Surrogate-Control: max-age='.$this->mCacheDuration.', content="ESI/1.0"');
					header( 'Cache-Control: s-maxage=0, must-revalidate, max-age=0' );
				} else {
					header( 'Cache-Control: s-maxage='.$this->mCacheDuration.', must-revalidate, max-age=0' );
				}

			} else {

				# Let the client do the caching. Cache is not purged.
				header ("Expires: " . gmdate( "D, d M Y H:i:s", time() + $this->mCacheDuration ) . " GMT");
				header ("Cache-Control: s-max-age={$this->mCacheDuration},public,max-age={$this->mCacheDuration}");
			}

		} else {
			# always expired, always modified
			header ("Expires: Mon, 26 Jul 1997 05:00:00 GMT");    // Date in the past
			header ("Cache-Control: no-cache, must-revalidate");  // HTTP/1.1
			header ("Pragma: no-cache");                          // HTTP/1.0
		}

		if ( $this->mVary ) {
			header ( "Vary: " . $this->mVary );
		}
	}

	/**
	 * checkLastModified tells the client to use the client-cached response if
	 * possible. If sucessful, the AjaxResponse is disabled so that
	 * any future call to AjaxResponse::printText() have no effect. The method
	 * returns true iff the response code was set to 304 Not Modified.
	 */
	function checkLastModified ( $timestamp ) {
		global $wgCachePages, $wgCacheEpoch, $wgUser;
		$fname = 'AjaxResponse::checkLastModified';

		if ( !$timestamp || $timestamp == '19700101000000' ) {
			wfDebug( "$fname: CACHE DISABLED, NO TIMESTAMP\n" );
			return;
		}
		if( !$wgCachePages ) {
			wfDebug( "$fname: CACHE DISABLED\n", false );
			return;
		}
		if( $wgUser->getOption( 'nocache' ) ) {
			wfDebug( "$fname: USER DISABLED CACHE\n", false );
			return;
		}

		$timestamp = wfTimestamp( TS_MW, $timestamp );
		$lastmod = wfTimestamp( TS_RFC2822, max( $timestamp, $wgUser->mTouched, $wgCacheEpoch ) );

		if( !empty( $_SERVER['HTTP_IF_MODIFIED_SINCE'] ) ) {
			# IE sends sizes after the date like this:
			# Wed, 20 Aug 2003 06:51:19 GMT; length=5202
			# this breaks strtotime().
			$modsince = preg_replace( '/;.*$/', '', $_SERVER["HTTP_IF_MODIFIED_SINCE"] );
			$modsinceTime = strtotime( $modsince );
			$ismodsince = wfTimestamp( TS_MW, $modsinceTime ? $modsinceTime : 1 );
			wfDebug( "$fname: -- client send If-Modified-Since: " . $modsince . "\n", false );
			wfDebug( "$fname: --  we might send Last-Modified : $lastmod\n", false );
			if( ($ismodsince >= $timestamp ) && $wgUser->validateCache( $ismodsince ) && $ismodsince >= $wgCacheEpoch ) {
				$this->setResponseCode( "304 Not Modified" );
				$this->disable();
				$this->mLastModified = $lastmod;

				wfDebug( "$fname: CACHED client: $ismodsince ; user: $wgUser->mTouched ; page: $timestamp ; site $wgCacheEpoch\n", false );

				return true;
			} else {
				wfDebug( "$fname: READY  client: $ismodsince ; user: $wgUser->mTouched ; page: $timestamp ; site $wgCacheEpoch\n", false );
				$this->mLastModified = $lastmod;
			}
		} else {
			wfDebug( "$fname: client did not send If-Modified-Since header\n", false );
			$this->mLastModified = $lastmod;
		}
	}

	function loadFromMemcached( $mckey, $touched ) {
		global $wgMemc;
		if ( !$touched ) return false;

		$mcvalue = $wgMemc->get( $mckey );
		if ( $mcvalue ) {
			# Check to see if the value has been invalidated
			if ( $touched <= $mcvalue['timestamp'] ) {
				wfDebug( "Got $mckey from cache\n" );
				$this->mText = $mcvalue['value'];
				return true;
			} else {
				wfDebug( "$mckey has expired\n" );
			}
		}

		return false;
	}

	function storeInMemcached( $mckey, $expiry = 86400 ) {
		global $wgMemc;

		$wgMemc->set( $mckey,
			array(
				'timestamp' => wfTimestampNow(),
				'value' => $this->mText
			), $expiry
		);

		return true;
	}
}
