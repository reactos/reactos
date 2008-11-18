<?php
/**
 * Copyright (C) 2004 Gabriel Wicke <wicke@wikidev.net>
 * http://wikidev.net/
 * Based on PageHistory and SpecialExport
 *
 * License: GPL (http://www.gnu.org/copyleft/gpl.html)
 *
 * @author Gabriel Wicke <wicke@wikidev.net>
 * @file
 */

/**
 * A simple method to retrieve the plain source of an article,
 * using "action=raw" in the GET request string.
 */
class RawPage {
	var $mArticle, $mTitle, $mRequest;
	var $mOldId, $mGen, $mCharset, $mSection;
	var $mSmaxage, $mMaxage;
	var $mContentType, $mExpandTemplates;

	function __construct( &$article, $request = false ) {
		global $wgRequest, $wgInputEncoding, $wgSquidMaxage, $wgJsMimeType, $wgForcedRawSMaxage, $wgGroupPermissions;

		$allowedCTypes = array('text/x-wiki', $wgJsMimeType, 'text/css', 'application/x-zope-edit');
		$this->mArticle =& $article;
		$this->mTitle =& $article->mTitle;

		if ( $request === false ) {
			$this->mRequest =& $wgRequest;
		} else {
			$this->mRequest = $request;
		}

		$ctype = $this->mRequest->getVal( 'ctype' );
		$smaxage = $this->mRequest->getIntOrNull( 'smaxage', $wgSquidMaxage );
		$maxage = $this->mRequest->getInt( 'maxage', $wgSquidMaxage );

		$this->mExpandTemplates = $this->mRequest->getVal( 'templates' ) === 'expand';
		$this->mUseMessageCache = $this->mRequest->getBool( 'usemsgcache' );

		$this->mSection = $this->mRequest->getIntOrNull( 'section' );

		$oldid = $this->mRequest->getInt( 'oldid' );

		switch ( $wgRequest->getText( 'direction' ) ) {
			case 'next':
				# output next revision, or nothing if there isn't one
				if ( $oldid ) {
					$oldid = $this->mTitle->getNextRevisionId( $oldid );
				}
				$oldid = $oldid ? $oldid : -1;
				break;
			case 'prev':
				# output previous revision, or nothing if there isn't one
				if ( ! $oldid ) {
					# get the current revision so we can get the penultimate one
					$this->mArticle->getTouched();
					$oldid = $this->mArticle->mLatest;
				}
				$prev = $this->mTitle->getPreviousRevisionId( $oldid );
				$oldid = $prev ? $prev : -1 ;
				break;
			case 'cur':
				$oldid = 0;
				break;
		}
		$this->mOldId = $oldid;

		# special case for 'generated' raw things: user css/js
		$gen = $this->mRequest->getVal( 'gen' );

		if($gen == 'css') {
			$this->mGen = $gen;
			if( is_null( $smaxage ) ) $smaxage = $wgSquidMaxage;
			if($ctype == '') $ctype = 'text/css';
		} elseif ($gen == 'js') {
			$this->mGen = $gen;
			if( is_null( $smaxage ) ) $smaxage = $wgSquidMaxage;
			if($ctype == '') $ctype = $wgJsMimeType;
		} else {
			$this->mGen = false;
		}
		$this->mCharset = $wgInputEncoding;

		# Force caching for CSS and JS raw content, default: 5 minutes
		if (is_null($smaxage) and ($ctype=='text/css' or $ctype==$wgJsMimeType)) {
			$this->mSmaxage = intval($wgForcedRawSMaxage);
		} else {
			$this->mSmaxage = intval( $smaxage );
		}
		$this->mMaxage = $maxage;

		# Output may contain user-specific data;
		# vary generated content for open sessions and private wikis
		if ($this->mGen or !$wgGroupPermissions['*']['read']) {
			$this->mPrivateCache = ( $this->mSmaxage == 0 ) ||
				( session_id() != '' );
		} else {
			$this->mPrivateCache = false;
		}

		if ( $ctype == '' or ! in_array( $ctype, $allowedCTypes ) ) {
			$this->mContentType = 'text/x-wiki';
		} else {
			$this->mContentType = $ctype;
		}
	}

	function view() {
		global $wgOut, $wgScript;

		if( isset( $_SERVER['SCRIPT_URL'] ) ) {
			# Normally we use PHP_SELF to get the URL to the script
			# as it was called, minus the query string.
			#
			# Some sites use Apache rewrite rules to handle subdomains,
			# and have PHP set up in a weird way that causes PHP_SELF
			# to contain the rewritten URL instead of the one that the
			# outside world sees.
			#
			# If in this mode, use SCRIPT_URL instead, which mod_rewrite
			# provides containing the "before" URL.
			$url = $_SERVER['SCRIPT_URL'];
		} else {
			$url = $_SERVER['PHP_SELF'];
		}

		if( strcmp( $wgScript, $url ) ) {
			# Internet Explorer will ignore the Content-Type header if it
			# thinks it sees a file extension it recognizes. Make sure that
			# all raw requests are done through the script node, which will
			# have eg '.php' and should remain safe.
			#
			# We used to redirect to a canonical-form URL as a general
			# backwards-compatibility / good-citizen nice thing. However
			# a lot of servers are set up in buggy ways, resulting in
			# redirect loops which hang the browser until the CSS load
			# times out.
			#
			# Just return a 403 Forbidden and get it over with.
			wfHttpError( 403, 'Forbidden',
				'Raw pages must be accessed through the primary script entry point.' );
			return;
		}

		header( "Content-type: ".$this->mContentType.'; charset='.$this->mCharset );
		# allow the client to cache this for 24 hours
		$mode = $this->mPrivateCache ? 'private' : 'public';
		header( 'Cache-Control: '.$mode.', s-maxage='.$this->mSmaxage.', max-age='.$this->mMaxage );
		$text = $this->getRawText();

		if( !wfRunHooks( 'RawPageViewBeforeOutput', array( &$this, &$text ) ) ) {
			wfDebug( __METHOD__ . ': RawPageViewBeforeOutput hook broke raw page output.' );
		}

		echo $text;
		$wgOut->disable();
	}

	function getRawText() {
		global $wgUser, $wgOut, $wgRequest;
		if($this->mGen) {
			$sk = $wgUser->getSkin();
			$sk->initPage($wgOut);
			if($this->mGen == 'css') {
				return $sk->getUserStylesheet();
			} else if($this->mGen == 'js') {
				return $sk->getUserJs();
			}
		} else {
			return $this->getArticleText();
		}
	}

	function getArticleText() {
		$found = false;
		$text = '';
		if( $this->mTitle ) {
			// If it's a MediaWiki message we can just hit the message cache
			if ( $this->mUseMessageCache && $this->mTitle->getNamespace() == NS_MEDIAWIKI ) {
				$key = $this->mTitle->getDBkey();
				$text = wfMsgForContentNoTrans( $key );
				# If the message doesn't exist, return a blank
				if( wfEmptyMsg( $key, $text ) )
					$text = '';
				$found = true;
			} else {
				// Get it from the DB
				$rev = Revision::newFromTitle( $this->mTitle, $this->mOldId );
				if ( $rev ) {
					$lastmod = wfTimestamp( TS_RFC2822, $rev->getTimestamp() );
					header( "Last-modified: $lastmod" );

					if ( !is_null($this->mSection ) ) {
						global $wgParser;
						$text = $wgParser->getSection ( $rev->getText(), $this->mSection );
					} else
						$text = $rev->getText();
					$found = true;
				}
			}
		}

		# Bad title or page does not exist
		if( !$found && $this->mContentType == 'text/x-wiki' ) {
			# Don't return a 404 response for CSS or JavaScript;
			# 404s aren't generally cached and it would create
			# extra hits when user CSS/JS are on and the user doesn't
			# have the pages.
			header( "HTTP/1.0 404 Not Found" );
		}

		// Special-case for empty CSS/JS
		//
		// Internet Explorer for Mac handles empty files badly;
		// particularly so when keep-alive is active. It can lead
		// to long timeouts as it seems to sit there waiting for
		// more data that never comes.
		//
		// Give it a comment...
		if( strlen( $text ) == 0 &&
			($this->mContentType == 'text/css' ||
				$this->mContentType == 'text/javascript' ) ) {
			return "/* Empty */";
		}

		return $this->parseArticleText( $text );
	}

	function parseArticleText( $text ) {
		if ( $text === '' )
			return '';
		else
			if ( $this->mExpandTemplates ) {
				global $wgParser;
				return $wgParser->preprocess( $text, $this->mTitle, new ParserOptions() );
			} else
				return $text;
	}
}
