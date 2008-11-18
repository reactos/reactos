<?php
/**
 * File for articles
 * @file
 */

/**
 * Class representing a MediaWiki article and history.
 *
 * See design.txt for an overview.
 * Note: edit user interface and cache support functions have been
 * moved to separate EditPage and HTMLFileCache classes.
 *
 */
class Article {
	/**@{{
	 * @private
	 */
	var $mComment;			//!<
	var $mContent;			//!<
	var $mContentLoaded;	//!<
	var $mCounter;			//!<
	var $mForUpdate;		//!<
	var $mGoodAdjustment;	//!<
	var $mLatest;			//!<
	var $mMinorEdit;		//!<
	var $mOldId;			//!<
	var $mRedirectedFrom;	//!<
	var $mRedirectUrl;		//!<
	var $mRevIdFetched;		//!<
	var $mRevision;			//!<
	var $mTimestamp;		//!<
	var $mTitle;			//!<
	var $mTotalAdjustment;	//!<
	var $mTouched;			//!<
	var $mUser;				//!<
	var $mUserText;			//!<
	var $mRedirectTarget;		//!<
	var $mIsRedirect;
	/**@}}*/

	/**
	 * Constructor and clear the article
	 * @param $title Reference to a Title object.
	 * @param $oldId Integer revision ID, null to fetch from request, zero for current
	 */
	function __construct( Title $title, $oldId = null ) {
		$this->mTitle =& $title;
		$this->mOldId = $oldId;
		$this->clear();
	}

	/**
	 * Tell the page view functions that this view was redirected
	 * from another page on the wiki.
	 * @param $from Title object.
	 */
	function setRedirectedFrom( $from ) {
		$this->mRedirectedFrom = $from;
	}

	/**
	 * If this page is a redirect, get its target
	 *
	 * The target will be fetched from the redirect table if possible.
	 * If this page doesn't have an entry there, call insertRedirect()
	 * @return mixed Title object, or null if this page is not a redirect
	 */
	public function getRedirectTarget() {
		if(!$this->mTitle || !$this->mTitle->isRedirect())
			return null;
		if(!is_null($this->mRedirectTarget))
			return $this->mRedirectTarget;

		# Query the redirect table
		$dbr = wfGetDB(DB_SLAVE);
		$res = $dbr->select('redirect',
				array('rd_namespace', 'rd_title'),
				array('rd_from' => $this->getID()),
				__METHOD__
		);
		$row = $dbr->fetchObject($res);
		if($row)
			return $this->mRedirectTarget = Title::makeTitle($row->rd_namespace, $row->rd_title);

		# This page doesn't have an entry in the redirect table
		return $this->mRedirectTarget = $this->insertRedirect();
	}

	/**
	 * Insert an entry for this page into the redirect table.
	 *
	 * Don't call this function directly unless you know what you're doing.
	 * @return Title object
	 */
	public function insertRedirect() {
		$retval = Title::newFromRedirect($this->getContent());
		if(!$retval)
			return null;
		$dbw = wfGetDB(DB_MASTER);
		$dbw->replace('redirect', array('rd_from'), array(
				'rd_from' => $this->getID(),
				'rd_namespace' => $retval->getNamespace(),
				'rd_title' => $retval->getDBKey()
		), __METHOD__);
		return $retval;
	}

	/**
	 * Get the Title object this page redirects to
	 *
	 * @return mixed false, Title of in-wiki target, or string with URL
	 */
	public function followRedirect() {
		$text = $this->getContent();
		return self::followRedirectText( $text );
	}
	
	/**
	 * Get the Title object this text redirects to
	 *
	 * @return mixed false, Title of in-wiki target, or string with URL
	 */
	public function followRedirectText( $text ) {
		$rt = Title::newFromRedirect( $text );
		# process if title object is valid and not special:userlogout
		if( $rt ) {
			if( $rt->getInterwiki() != '' ) {
				if( $rt->isLocal() ) {
					// Offsite wikis need an HTTP redirect.
					//
					// This can be hard to reverse and may produce loops,
					// so they may be disabled in the site configuration.

					$source = $this->mTitle->getFullURL( 'redirect=no' );
					return $rt->getFullURL( 'rdfrom=' . urlencode( $source ) );
				}
			} else {
				if( $rt->getNamespace() == NS_SPECIAL ) {
					// Gotta handle redirects to special pages differently:
					// Fill the HTTP response "Location" header and ignore
					// the rest of the page we're on.
					//
					// This can be hard to reverse, so they may be disabled.

					if( $rt->isSpecial( 'Userlogout' ) ) {
						// rolleyes
					} else {
						return $rt->getFullURL();
					}
				}
				return $rt;
			}
		}
		// No or invalid redirect
		return false;
	}

	/**
	 * get the title object of the article
	 */
	function getTitle() {
		return $this->mTitle;
	}

	/**
	  * Clear the object
	  * @private
	  */
	function clear() {
		$this->mDataLoaded    = false;
		$this->mContentLoaded = false;

		$this->mCurID = $this->mUser = $this->mCounter = -1; # Not loaded
		$this->mRedirectedFrom = null; # Title object if set
		$this->mRedirectTarget = null; # Title object if set
		$this->mUserText =
		$this->mTimestamp = $this->mComment = '';
		$this->mGoodAdjustment = $this->mTotalAdjustment = 0;
		$this->mTouched = '19700101000000';
		$this->mForUpdate = false;
		$this->mIsRedirect = false;
		$this->mRevIdFetched = 0;
		$this->mRedirectUrl = false;
		$this->mLatest = false;
		$this->mPreparedEdit = false;
	}

	/**
	 * Note that getContent/loadContent do not follow redirects anymore.
	 * If you need to fetch redirectable content easily, try
	 * the shortcut in Article::followContent()
	 * FIXME
	 * @todo There are still side-effects in this!
	 *        In general, you should use the Revision class, not Article,
	 *        to fetch text for purposes other than page views.
	 *
	 * @return Return the text of this revision
	*/
	function getContent() {
		global $wgUser, $wgOut, $wgMessageCache;

		wfProfileIn( __METHOD__ );

		if ( 0 == $this->getID() ) {
			wfProfileOut( __METHOD__ );
			$wgOut->setRobotpolicy( 'noindex,nofollow' );

			if ( $this->mTitle->getNamespace() == NS_MEDIAWIKI ) {
				$wgMessageCache->loadAllMessages();
				$ret = wfMsgWeirdKey ( $this->mTitle->getText() ) ;
			} else {
				$ret = wfMsg( $wgUser->isLoggedIn() ? 'noarticletext' : 'noarticletextanon' );
			}

			return "<div class='noarticletext'>\n$ret\n</div>";
		} else {
			$this->loadContent();
			wfProfileOut( __METHOD__ );
			return $this->mContent;
		}
	}

	/**
	 * This function returns the text of a section, specified by a number ($section).
	 * A section is text under a heading like == Heading == or \<h1\>Heading\</h1\>, or
	 * the first section before any such heading (section 0).
	 *
	 * If a section contains subsections, these are also returned.
	 *
	 * @param $text String: text to look in
	 * @param $section Integer: section number
	 * @return string text of the requested section
	 * @deprecated
	 */
	function getSection($text,$section) {
		global $wgParser;
		return $wgParser->getSection( $text, $section );
	}

	/**
	 * @return int The oldid of the article that is to be shown, 0 for the
	 *             current revision
	 */
	function getOldID() {
		if ( is_null( $this->mOldId ) ) {
			$this->mOldId = $this->getOldIDFromRequest();
		}
		return $this->mOldId;
	}

	/**
	 * Sets $this->mRedirectUrl to a correct URL if the query parameters are incorrect
	 *
	 * @return int The old id for the request
	 */
	function getOldIDFromRequest() {
		global $wgRequest;
		$this->mRedirectUrl = false;
		$oldid = $wgRequest->getVal( 'oldid' );
		if ( isset( $oldid ) ) {
			$oldid = intval( $oldid );
			if ( $wgRequest->getVal( 'direction' ) == 'next' ) {
				$nextid = $this->mTitle->getNextRevisionID( $oldid );
				if ( $nextid  ) {
					$oldid = $nextid;
				} else {
					$this->mRedirectUrl = $this->mTitle->getFullURL( 'redirect=no' );
				}
			} elseif ( $wgRequest->getVal( 'direction' ) == 'prev' ) {
				$previd = $this->mTitle->getPreviousRevisionID( $oldid );
				if ( $previd ) {
					$oldid = $previd;
				} else {
					# TODO
				}
			}
			# unused:
			# $lastid = $oldid;
		}

		if ( !$oldid ) {
			$oldid = 0;
		}
		return $oldid;
	}

	/**
	 * Load the revision (including text) into this object
	 */
	function loadContent() {
		if ( $this->mContentLoaded ) return;

		# Query variables :P
		$oldid = $this->getOldID();

		# Pre-fill content with error message so that if something
		# fails we'll have something telling us what we intended.
		$this->mOldId = $oldid;
		$this->fetchContent( $oldid );
	}


	/**
	 * Fetch a page record with the given conditions
	 * @param Database $dbr
	 * @param array    $conditions
	 * @private
	 */
	function pageData( $dbr, $conditions ) {
		$fields = array(
				'page_id',
				'page_namespace',
				'page_title',
				'page_restrictions',
				'page_counter',
				'page_is_redirect',
				'page_is_new',
				'page_random',
				'page_touched',
				'page_latest',
				'page_len',
		);
		wfRunHooks( 'ArticlePageDataBefore', array( &$this, &$fields ) );
		$row = $dbr->selectRow(
			'page',
			$fields,
			$conditions,
			__METHOD__
		);
		wfRunHooks( 'ArticlePageDataAfter', array( &$this, &$row ) );
		return $row ;
	}

	/**
	 * @param Database $dbr
	 * @param Title $title
	 */
	function pageDataFromTitle( $dbr, $title ) {
		return $this->pageData( $dbr, array(
			'page_namespace' => $title->getNamespace(),
			'page_title'     => $title->getDBkey() ) );
	}

	/**
	 * @param Database $dbr
	 * @param int $id
	 */
	function pageDataFromId( $dbr, $id ) {
		return $this->pageData( $dbr, array( 'page_id' => $id ) );
	}

	/**
	 * Set the general counter, title etc data loaded from
	 * some source.
	 *
	 * @param object $data
	 * @private
	 */
	function loadPageData( $data = 'fromdb' ) {
		if ( $data === 'fromdb' ) {
			$dbr = wfGetDB( DB_MASTER );
			$data = $this->pageDataFromId( $dbr, $this->getId() );
		}

		$lc = LinkCache::singleton();
		if ( $data ) {
			$lc->addGoodLinkObj( $data->page_id, $this->mTitle, $data->page_len, $data->page_is_redirect );

			$this->mTitle->mArticleID = $data->page_id;

			# Old-fashioned restrictions.
			$this->mTitle->loadRestrictions( $data->page_restrictions );

			$this->mCounter     = $data->page_counter;
			$this->mTouched     = wfTimestamp( TS_MW, $data->page_touched );
			$this->mIsRedirect  = $data->page_is_redirect;
			$this->mLatest      = $data->page_latest;
		} else {
			if ( is_object( $this->mTitle ) ) {
				$lc->addBadLinkObj( $this->mTitle );
			}
			$this->mTitle->mArticleID = 0;
		}

		$this->mDataLoaded  = true;
	}

	/**
	 * Get text of an article from database
	 * Does *NOT* follow redirects.
	 * @param int $oldid 0 for whatever the latest revision is
	 * @return string
	 */
	function fetchContent( $oldid = 0 ) {
		if ( $this->mContentLoaded ) {
			return $this->mContent;
		}

		$dbr = wfGetDB( DB_MASTER );

		# Pre-fill content with error message so that if something
		# fails we'll have something telling us what we intended.
		$t = $this->mTitle->getPrefixedText();
		$d = $oldid ? wfMsgExt( 'missingarticle-rev', array( 'escape' ), $oldid ) : '';
		$this->mContent = wfMsg( 'missing-article', $t, $d ) ;

		if( $oldid ) {
			$revision = Revision::newFromId( $oldid );
			if( is_null( $revision ) ) {
				wfDebug( __METHOD__." failed to retrieve specified revision, id $oldid\n" );
				return false;
			}
			$data = $this->pageDataFromId( $dbr, $revision->getPage() );
			if( !$data ) {
				wfDebug( __METHOD__." failed to get page data linked to revision id $oldid\n" );
				return false;
			}
			$this->mTitle = Title::makeTitle( $data->page_namespace, $data->page_title );
			$this->loadPageData( $data );
		} else {
			if( !$this->mDataLoaded ) {
				$data = $this->pageDataFromTitle( $dbr, $this->mTitle );
				if( !$data ) {
					wfDebug( __METHOD__." failed to find page data for title " . $this->mTitle->getPrefixedText() . "\n" );
					return false;
				}
				$this->loadPageData( $data );
			}
			$revision = Revision::newFromId( $this->mLatest );
			if( is_null( $revision ) ) {
				wfDebug( __METHOD__." failed to retrieve current page, rev_id {$data->page_latest}\n" );
				return false;
			}
		}

		// FIXME: Horrible, horrible! This content-loading interface just plain sucks.
		// We should instead work with the Revision object when we need it...
		$this->mContent   = $revision->revText(); // Loads if user is allowed

		$this->mUser      = $revision->getUser();
		$this->mUserText  = $revision->getUserText();
		$this->mComment   = $revision->getComment();
		$this->mTimestamp = wfTimestamp( TS_MW, $revision->getTimestamp() );

		$this->mRevIdFetched = $revision->getId();
		$this->mContentLoaded = true;
		$this->mRevision =& $revision;

		wfRunHooks( 'ArticleAfterFetchContent', array( &$this, &$this->mContent ) ) ;

		return $this->mContent;
	}

	/**
	 * Read/write accessor to select FOR UPDATE
	 *
	 * @param $x Mixed: FIXME
	 */
	function forUpdate( $x = NULL ) {
		return wfSetVar( $this->mForUpdate, $x );
	}

	/**
	 * Get the database which should be used for reads
	 *
	 * @return Database
	 * @deprecated - just call wfGetDB( DB_MASTER ) instead
	 */
	function getDB() {
		wfDeprecated( __METHOD__ );
		return wfGetDB( DB_MASTER );
	}

	/**
	 * Get options for all SELECT statements
	 *
	 * @param $options Array: an optional options array which'll be appended to
	 *                       the default
	 * @return Array: options
	 */
	function getSelectOptions( $options = '' ) {
		if ( $this->mForUpdate ) {
			if ( is_array( $options ) ) {
				$options[] = 'FOR UPDATE';
			} else {
				$options = 'FOR UPDATE';
			}
		}
		return $options;
	}

	/**
	 * @return int Page ID
	 */
	function getID() {
		if( $this->mTitle ) {
			return $this->mTitle->getArticleID();
		} else {
			return 0;
		}
	}

	/**
	 * @return bool Whether or not the page exists in the database
	 */
	function exists() {
		return $this->getId() != 0;
	}

	/**
	 * @return int The view count for the page
	 */
	function getCount() {
		if ( -1 == $this->mCounter ) {
			$id = $this->getID();
			if ( $id == 0 ) {
				$this->mCounter = 0;
			} else {
				$dbr = wfGetDB( DB_SLAVE );
				$this->mCounter = $dbr->selectField( 'page', 'page_counter', array( 'page_id' => $id ),
					'Article::getCount', $this->getSelectOptions() );
			}
		}
		return $this->mCounter;
	}

	/**
	 * Determine whether a page  would be suitable for being counted as an
	 * article in the site_stats table based on the title & its content
	 *
	 * @param $text String: text to analyze
	 * @return bool
	 */
	function isCountable( $text ) {
		global $wgUseCommaCount;

		$token = $wgUseCommaCount ? ',' : '[[';
		return
			$this->mTitle->isContentPage()
			&& !$this->isRedirect( $text )
			&& in_string( $token, $text );
	}

	/**
	 * Tests if the article text represents a redirect
	 *
	 * @param $text String: FIXME
	 * @return bool
	 */
	function isRedirect( $text = false ) {
		if ( $text === false ) {
			if ( $this->mDataLoaded ) 
				return $this->mIsRedirect;
			
			// Apparently loadPageData was never called
			$this->loadContent();
			$titleObj = Title::newFromRedirect( $this->fetchContent() );
		} else {
			$titleObj = Title::newFromRedirect( $text );
		}
		return $titleObj !== NULL;
	}

	/**
	 * Returns true if the currently-referenced revision is the current edit
	 * to this page (and it exists).
	 * @return bool
	 */
	function isCurrent() {
		# If no oldid, this is the current version.
		if ($this->getOldID() == 0)
			return true;

		return $this->exists() &&
			isset( $this->mRevision ) &&
			$this->mRevision->isCurrent();
	}

	/**
	 * Loads everything except the text
	 * This isn't necessary for all uses, so it's only done if needed.
	 * @private
	 */
	function loadLastEdit() {
		if ( -1 != $this->mUser )
			return;

		# New or non-existent articles have no user information
		$id = $this->getID();
		if ( 0 == $id ) return;

		$this->mLastRevision = Revision::loadFromPageId( wfGetDB( DB_MASTER ), $id );
		if( !is_null( $this->mLastRevision ) ) {
			$this->mUser      = $this->mLastRevision->getUser();
			$this->mUserText  = $this->mLastRevision->getUserText();
			$this->mTimestamp = $this->mLastRevision->getTimestamp();
			$this->mComment   = $this->mLastRevision->getComment();
			$this->mMinorEdit = $this->mLastRevision->isMinor();
			$this->mRevIdFetched = $this->mLastRevision->getId();
		}
	}

	function getTimestamp() {
		// Check if the field has been filled by ParserCache::get()
		if ( !$this->mTimestamp ) {
			$this->loadLastEdit();
		}
		return wfTimestamp(TS_MW, $this->mTimestamp);
	}

	function getUser() {
		$this->loadLastEdit();
		return $this->mUser;
	}

	function getUserText() {
		$this->loadLastEdit();
		return $this->mUserText;
	}

	function getComment() {
		$this->loadLastEdit();
		return $this->mComment;
	}

	function getMinorEdit() {
		$this->loadLastEdit();
		return $this->mMinorEdit;
	}

	function getRevIdFetched() {
		$this->loadLastEdit();
		return $this->mRevIdFetched;
	}

	/**
	 * @param $limit Integer: default 0.
	 * @param $offset Integer: default 0.
	 */
	function getContributors($limit = 0, $offset = 0) {
		# XXX: this is expensive; cache this info somewhere.

		$contribs = array();
		$dbr = wfGetDB( DB_SLAVE );
		$revTable = $dbr->tableName( 'revision' );
		$userTable = $dbr->tableName( 'user' );
		$user = $this->getUser();
		$pageId = $this->getId();

		$sql = "SELECT rev_user, rev_user_text, user_real_name, MAX(rev_timestamp) as timestamp
			FROM $revTable LEFT JOIN $userTable ON rev_user = user_id
			WHERE rev_page = $pageId
			AND rev_user != $user
			GROUP BY rev_user, rev_user_text, user_real_name
			ORDER BY timestamp DESC";

		if ($limit > 0) { $sql .= ' LIMIT '.$limit; }
		if ($offset > 0) { $sql .= ' OFFSET '.$offset; }
		
		$sql .= ' '. $this->getSelectOptions();

		$res = $dbr->query($sql, __METHOD__);

		while ( $line = $dbr->fetchObject( $res ) ) {
			$contribs[] = array($line->rev_user, $line->rev_user_text, $line->user_real_name);
		}

		$dbr->freeResult($res);
		return $contribs;
	}

	/**
	 * This is the default action of the script: just view the page of
	 * the given title.
	*/
	function view() {
		global $wgUser, $wgOut, $wgRequest, $wgContLang;
		global $wgEnableParserCache, $wgStylePath, $wgParser;
		global $wgUseTrackbacks, $wgNamespaceRobotPolicies, $wgArticleRobotPolicies;
		global $wgDefaultRobotPolicy;
		$sk = $wgUser->getSkin();

		wfProfileIn( __METHOD__ );

		$parserCache = ParserCache::singleton();
		$ns = $this->mTitle->getNamespace(); # shortcut

		# Get variables from query string
		$oldid = $this->getOldID();

		# getOldID may want us to redirect somewhere else
		if ( $this->mRedirectUrl ) {
			$wgOut->redirect( $this->mRedirectUrl );
			wfProfileOut( __METHOD__ );
			return;
		}

		$diff = $wgRequest->getVal( 'diff' );
		$rcid = $wgRequest->getVal( 'rcid' );
		$rdfrom = $wgRequest->getVal( 'rdfrom' );
		$diffOnly = $wgRequest->getBool( 'diffonly', $wgUser->getOption( 'diffonly' ) );
		$purge = $wgRequest->getVal( 'action' ) == 'purge';

		$wgOut->setArticleFlag( true );

		# Discourage indexing of printable versions, but encourage following
		if( $wgOut->isPrintable() ) {
			$policy = 'noindex,follow';
		} elseif ( isset( $wgArticleRobotPolicies[$this->mTitle->getPrefixedText()] ) ) {
			$policy = $wgArticleRobotPolicies[$this->mTitle->getPrefixedText()];
		} elseif( isset( $wgNamespaceRobotPolicies[$ns] ) ) {
			# Honour customised robot policies for this namespace
			$policy = $wgNamespaceRobotPolicies[$ns];
		} else {
			$policy = $wgDefaultRobotPolicy;
		}
		$wgOut->setRobotPolicy( $policy );

		# If we got diff and oldid in the query, we want to see a
		# diff page instead of the article.

		if ( !is_null( $diff ) ) {
			$wgOut->setPageTitle( $this->mTitle->getPrefixedText() );

			$de = new DifferenceEngine( $this->mTitle, $oldid, $diff, $rcid, $purge );
			// DifferenceEngine directly fetched the revision:
			$this->mRevIdFetched = $de->mNewid;
			$de->showDiffPage( $diffOnly );

			// Needed to get the page's current revision
			$this->loadPageData();
			if( $diff == 0 || $diff == $this->mLatest ) {
				# Run view updates for current revision only
				$this->viewUpdates();
			}
			wfProfileOut( __METHOD__ );
			return;
		}

		if ( empty( $oldid ) && $this->checkTouched() ) {
			$wgOut->setETag($parserCache->getETag($this, $wgUser));

			if( $wgOut->checkLastModified( $this->mTouched ) ){
				wfProfileOut( __METHOD__ );
				return;
			} else if ( $this->tryFileCache() ) {
				# tell wgOut that output is taken care of
				$wgOut->disable();
				$this->viewUpdates();
				wfProfileOut( __METHOD__ );
				return;
			}
		}

		# Should the parser cache be used?
		$pcache = $this->useParserCache( $oldid );
		wfDebug( 'Article::view using parser cache: ' . ($pcache ? 'yes' : 'no' ) . "\n" );
		if ( $wgUser->getOption( 'stubthreshold' ) ) {
			wfIncrStats( 'pcache_miss_stub' );
		}

		$wasRedirected = false;
		if ( isset( $this->mRedirectedFrom ) ) {
			// This is an internally redirected page view.
			// We'll need a backlink to the source page for navigation.
			if ( wfRunHooks( 'ArticleViewRedirect', array( &$this ) ) ) {
				$redir = $sk->makeKnownLinkObj( $this->mRedirectedFrom, '', 'redirect=no' );
				$s = wfMsg( 'redirectedfrom', $redir );
				$wgOut->setSubtitle( $s );

				// Set the fragment if one was specified in the redirect
				if ( strval( $this->mTitle->getFragment() ) != '' ) {
					$fragment = Xml::escapeJsString( $this->mTitle->getFragmentForURL() );
					$wgOut->addInlineScript( "redirectToFragment(\"$fragment\");" );
				}
				$wasRedirected = true;
			}
		} elseif ( !empty( $rdfrom ) ) {
			// This is an externally redirected view, from some other wiki.
			// If it was reported from a trusted site, supply a backlink.
			global $wgRedirectSources;
			if( $wgRedirectSources && preg_match( $wgRedirectSources, $rdfrom ) ) {
				$redir = $sk->makeExternalLink( $rdfrom, $rdfrom );
				$s = wfMsg( 'redirectedfrom', $redir );
				$wgOut->setSubtitle( $s );
				$wasRedirected = true;
			}
		}

		$outputDone = false;
		wfRunHooks( 'ArticleViewHeader', array( &$this, &$outputDone, &$pcache ) );
		if ( $pcache ) {
			if ( $wgOut->tryParserCache( $this, $wgUser ) ) {
				// Ensure that UI elements requiring revision ID have
				// the correct version information.
				$wgOut->setRevisionId( $this->mLatest );
				$outputDone = true;
			}
		}
		# Fetch content and check for errors
		if ( !$outputDone ) {
			$text = $this->getContent();
			if ( $text === false ) {
				# Failed to load, replace text with error message
				$t = $this->mTitle->getPrefixedText();
				if( $oldid ) {
					$d = wfMsgExt( 'missingarticle-rev', array( 'escape' ), $oldid );
					$text = wfMsg( 'missing-article', $t, $d );
				} else {
					$text = wfMsg( 'noarticletext' );
				}
			}

			# Another whitelist check in case oldid is altering the title
			if ( !$this->mTitle->userCanRead() ) {
				$wgOut->loginToUse();
				$wgOut->output();
				wfProfileOut( __METHOD__ );
				exit;
			}

			# We're looking at an old revision
			if ( !empty( $oldid ) ) {
				$wgOut->setRobotpolicy( 'noindex,nofollow' );
				if( is_null( $this->mRevision ) ) {
					// FIXME: This would be a nice place to load the 'no such page' text.
				} else {
					$this->setOldSubtitle( isset($this->mOldId) ? $this->mOldId : $oldid );
					if( $this->mRevision->isDeleted( Revision::DELETED_TEXT ) ) {
						if( !$this->mRevision->userCan( Revision::DELETED_TEXT ) ) {
							$wgOut->addWikiMsg( 'rev-deleted-text-permission' );
							$wgOut->setPageTitle( $this->mTitle->getPrefixedText() );
							wfProfileOut( __METHOD__ );
							return;
						} else {
							$wgOut->addWikiMsg( 'rev-deleted-text-view' );
							// and we are allowed to see...
						}
					}
				}
			}
			
			$wgOut->setRevisionId( $this->getRevIdFetched() );

			 // Pages containing custom CSS or JavaScript get special treatment
			if( $this->mTitle->isCssOrJsPage() || $this->mTitle->isCssJsSubpage() ) {
				$wgOut->addHtml( wfMsgExt( 'clearyourcache', 'parse' ) );
				// Give hooks a chance to customise the output
				if( wfRunHooks( 'ShowRawCssJs', array( $this->mContent, $this->mTitle, $wgOut ) ) ) {
					// Wrap the whole lot in a <pre> and don't parse
					$m = array();
					preg_match( '!\.(css|js)$!u', $this->mTitle->getText(), $m );
					$wgOut->addHtml( "<pre class=\"mw-code mw-{$m[1]}\" dir=\"ltr\">\n" );
					$wgOut->addHtml( htmlspecialchars( $this->mContent ) );
					$wgOut->addHtml( "\n</pre>\n" );
				}
			} else if ( $rt = Title::newFromRedirect( $text ) ) {
				# Don't append the subtitle if this was an old revision
				$this->viewRedirect( $rt, !$wasRedirected && $this->isCurrent() );
				$parseout = $wgParser->parse($text, $this->mTitle, ParserOptions::newFromUser($wgUser));
				$wgOut->addParserOutputNoText( $parseout );
			} else if ( $pcache ) {
				# Display content and save to parser cache
				$this->outputWikiText( $text );
			} else {
				# Display content, don't attempt to save to parser cache
				# Don't show section-edit links on old revisions... this way lies madness.
				if( !$this->isCurrent() ) {
					$oldEditSectionSetting = $wgOut->parserOptions()->setEditSection( false );
				}
				# Display content and don't save to parser cache
				# With timing hack -- TS 2006-07-26
				$time = -wfTime();
				$this->outputWikiText( $text, false );
				$time += wfTime();

				# Timing hack
				if ( $time > 3 ) {
					wfDebugLog( 'slow-parse', sprintf( "%-5.2f %s", $time,
						$this->mTitle->getPrefixedDBkey()));
				}

				if( !$this->isCurrent() ) {
					$wgOut->parserOptions()->setEditSection( $oldEditSectionSetting );
				}
			}
		}
		/* title may have been set from the cache */
		$t = $wgOut->getPageTitle();
		if( empty( $t ) ) {
			$wgOut->setPageTitle( $this->mTitle->getPrefixedText() );
		}

		# check if we're displaying a [[User talk:x.x.x.x]] anonymous talk page
		if( $ns == NS_USER_TALK && IP::isValid( $this->mTitle->getText() ) ) {
			$wgOut->addWikiMsg('anontalkpagetext');
		}

		# If we have been passed an &rcid= parameter, we want to give the user a
		# chance to mark this new article as patrolled.
		if( !is_null( $rcid ) && $rcid != 0 && $wgUser->isAllowed( 'patrol' ) && $this->mTitle->exists() ) {
			$wgOut->addHTML(
				"<div class='patrollink'>" .
					wfMsgHtml( 'markaspatrolledlink',
					$sk->makeKnownLinkObj( $this->mTitle, wfMsgHtml('markaspatrolledtext'),
						"action=markpatrolled&rcid=$rcid" )
			 		) .
				'</div>'
			 );
		}

		# Trackbacks
		if ($wgUseTrackbacks)
			$this->addTrackbacks();

		$this->viewUpdates();
		wfProfileOut( __METHOD__ );
	}
	
	/* 
	* Should the parser cache be used?
	*/
	protected function useParserCache( $oldid ) {
		global $wgUser, $wgEnableParserCache;
		
		return $wgEnableParserCache
			&& intval( $wgUser->getOption( 'stubthreshold' ) ) == 0
			&& $this->exists()
			&& empty( $oldid )
			&& !$this->mTitle->isCssOrJsPage()
			&& !$this->mTitle->isCssJsSubpage();
	}
	
	protected function viewRedirect( $target, $appendSubtitle = true, $forceKnown = false ) {
		global $wgParser, $wgOut, $wgContLang, $wgStylePath, $wgUser;
		
		# Display redirect
		$imageDir = $wgContLang->isRTL() ? 'rtl' : 'ltr';
		$imageUrl = $wgStylePath.'/common/images/redirect' . $imageDir . '.png';
		
		if( $appendSubtitle ) {
			$wgOut->appendSubtitle( wfMsgHtml( 'redirectpagesub' ) );
		}
		$sk = $wgUser->getSkin();
		if ( $forceKnown )
			$link = $sk->makeKnownLinkObj( $target, htmlspecialchars( $target->getFullText() ) );
		else
			$link = $sk->makeLinkObj( $target, htmlspecialchars( $target->getFullText() ) );

		$wgOut->addHTML( '<img src="'.$imageUrl.'" alt="#REDIRECT " />' .
			'<span class="redirectText">'.$link.'</span>' );
		
	}

	function addTrackbacks() {
		global $wgOut, $wgUser;

		$dbr = wfGetDB(DB_SLAVE);
		$tbs = $dbr->select(
				/* FROM   */ 'trackbacks',
				/* SELECT */ array('tb_id', 'tb_title', 'tb_url', 'tb_ex', 'tb_name'),
				/* WHERE  */ array('tb_page' => $this->getID())
		);

		if (!$dbr->numrows($tbs))
			return;

		$tbtext = "";
		while ($o = $dbr->fetchObject($tbs)) {
			$rmvtxt = "";
			if ($wgUser->isAllowed( 'trackback' )) {
				$delurl = $this->mTitle->getFullURL("action=deletetrackback&tbid="
						. $o->tb_id . "&token=" . urlencode( $wgUser->editToken() ) );
				$rmvtxt = wfMsg( 'trackbackremove', htmlspecialchars( $delurl ) );
			}
			$tbtext .= "\n";
			$tbtext .= wfMsg(strlen($o->tb_ex) ? 'trackbackexcerpt' : 'trackback',
					$o->tb_title,
					$o->tb_url,
					$o->tb_ex,
					$o->tb_name,
					$rmvtxt);
		}
		$wgOut->addWikiMsg( 'trackbackbox', $tbtext );
	}

	function deletetrackback() {
		global $wgUser, $wgRequest, $wgOut, $wgTitle;

		if (!$wgUser->matchEditToken($wgRequest->getVal('token'))) {
			$wgOut->addWikiMsg( 'sessionfailure' );
			return;
		}

		$permission_errors = $this->mTitle->getUserPermissionsErrors( 'delete', $wgUser );

		if (count($permission_errors)>0)
		{
			$wgOut->showPermissionsErrorPage( $permission_errors );
			return;
		}

		$db = wfGetDB(DB_MASTER);
		$db->delete('trackbacks', array('tb_id' => $wgRequest->getInt('tbid')));
		$wgTitle->invalidateCache();
		$wgOut->addWikiMsg('trackbackdeleteok');
	}

	function render() {
		global $wgOut;

		$wgOut->setArticleBodyOnly(true);
		$this->view();
	}

	/**
	 * Handle action=purge
	 */
	function purge() {
		global $wgUser, $wgRequest, $wgOut;

		if ( $wgUser->isAllowed( 'purge' ) || $wgRequest->wasPosted() ) {
			if( wfRunHooks( 'ArticlePurge', array( &$this ) ) ) {
				$this->doPurge();
			}
		} else {
			$msg = $wgOut->parse( wfMsg( 'confirm_purge' ) );
			$action = htmlspecialchars( $_SERVER['REQUEST_URI'] );
			$button = htmlspecialchars( wfMsg( 'confirm_purge_button' ) );
			$msg = str_replace( '$1',
				"<form method=\"post\" action=\"$action\">\n" .
				"<input type=\"submit\" name=\"submit\" value=\"$button\" />\n" .
				"</form>\n", $msg );

			$wgOut->setPageTitle( $this->mTitle->getPrefixedText() );
			$wgOut->setRobotpolicy( 'noindex,nofollow' );
			$wgOut->addHTML( $msg );
		}
	}

	/**
	 * Perform the actions of a page purging
	 */
	function doPurge() {
		global $wgUseSquid;
		// Invalidate the cache
		$this->mTitle->invalidateCache();

		if ( $wgUseSquid ) {
			// Commit the transaction before the purge is sent
			$dbw = wfGetDB( DB_MASTER );
			$dbw->immediateCommit();

			// Send purge
			$update = SquidUpdate::newSimplePurge( $this->mTitle );
			$update->doUpdate();
		}
		if ( $this->mTitle->getNamespace() == NS_MEDIAWIKI ) {
			global $wgMessageCache;
			if ( $this->getID() == 0 ) {
				$text = false;
			} else {
				$text = $this->getContent();
			}
			$wgMessageCache->replace( $this->mTitle->getDBkey(), $text );
		}
		$this->view();
	}

	/**
	 * Insert a new empty page record for this article.
	 * This *must* be followed up by creating a revision
	 * and running $this->updateToLatest( $rev_id );
	 * or else the record will be left in a funky state.
	 * Best if all done inside a transaction.
	 *
	 * @param Database $dbw
	 * @return int     The newly created page_id key
	 * @private
	 */
	function insertOn( $dbw ) {
		wfProfileIn( __METHOD__ );

		$page_id = $dbw->nextSequenceValue( 'page_page_id_seq' );
		$dbw->insert( 'page', array(
			'page_id'           => $page_id,
			'page_namespace'    => $this->mTitle->getNamespace(),
			'page_title'        => $this->mTitle->getDBkey(),
			'page_counter'      => 0,
			'page_restrictions' => '',
			'page_is_redirect'  => 0, # Will set this shortly...
			'page_is_new'       => 1,
			'page_random'       => wfRandom(),
			'page_touched'      => $dbw->timestamp(),
			'page_latest'       => 0, # Fill this in shortly...
			'page_len'          => 0, # Fill this in shortly...
		), __METHOD__ );
		$newid = $dbw->insertId();

		$this->mTitle->resetArticleId( $newid );

		wfProfileOut( __METHOD__ );
		return $newid;
	}

	/**
	 * Update the page record to point to a newly saved revision.
	 *
	 * @param Database $dbw
	 * @param Revision $revision For ID number, and text used to set
	                             length and redirect status fields
	 * @param int $lastRevision If given, will not overwrite the page field
	 *                          when different from the currently set value.
	 *                          Giving 0 indicates the new page flag should
	 *                          be set on.
	 * @param bool $lastRevIsRedirect If given, will optimize adding and
	 * 							removing rows in redirect table.
	 * @return bool true on success, false on failure
	 * @private
	 */
	function updateRevisionOn( &$dbw, $revision, $lastRevision = null, $lastRevIsRedirect = null ) {
		wfProfileIn( __METHOD__ );

		$text = $revision->getText();
		$rt = Title::newFromRedirect( $text );

		$conditions = array( 'page_id' => $this->getId() );
		if( !is_null( $lastRevision ) ) {
			# An extra check against threads stepping on each other
			$conditions['page_latest'] = $lastRevision;
		}

		$dbw->update( 'page',
			array( /* SET */
				'page_latest'      => $revision->getId(),
				'page_touched'     => $dbw->timestamp(),
				'page_is_new'      => ($lastRevision === 0) ? 1 : 0,
				'page_is_redirect' => $rt !== NULL ? 1 : 0,
				'page_len'         => strlen( $text ),
			),
			$conditions,
			__METHOD__ );

		$result = $dbw->affectedRows() != 0;

		if ($result) {
			$this->updateRedirectOn( $dbw, $rt, $lastRevIsRedirect );
		}

		wfProfileOut( __METHOD__ );
		return $result;
	}

	/**
	 * Add row to the redirect table if this is a redirect, remove otherwise.
	 *
	 * @param Database $dbw
	 * @param $redirectTitle a title object pointing to the redirect target,
	 * 							or NULL if this is not a redirect
	 * @param bool $lastRevIsRedirect If given, will optimize adding and
	 * 							removing rows in redirect table.
	 * @return bool true on success, false on failure
	 * @private
	 */
	function updateRedirectOn( &$dbw, $redirectTitle, $lastRevIsRedirect = null ) {

		// Always update redirects (target link might have changed)
		// Update/Insert if we don't know if the last revision was a redirect or not
		// Delete if changing from redirect to non-redirect
		$isRedirect = !is_null($redirectTitle);
		if ($isRedirect || is_null($lastRevIsRedirect) || $lastRevIsRedirect !== $isRedirect) {

			wfProfileIn( __METHOD__ );

			if ($isRedirect) {

				// This title is a redirect, Add/Update row in the redirect table
				$set = array( /* SET */
					'rd_namespace' => $redirectTitle->getNamespace(),
					'rd_title'     => $redirectTitle->getDBkey(),
					'rd_from'      => $this->getId(),
				);

				$dbw->replace( 'redirect', array( 'rd_from' ), $set, __METHOD__ );
			} else {
				// This is not a redirect, remove row from redirect table
				$where = array( 'rd_from' => $this->getId() );
				$dbw->delete( 'redirect', $where, __METHOD__);
			}

			if( $this->getTitle()->getNamespace() == NS_IMAGE )
				RepoGroup::singleton()->getLocalRepo()->invalidateImageRedirect( $this->getTitle() );
			wfProfileOut( __METHOD__ );
			return ( $dbw->affectedRows() != 0 );
		}

		return true;
	}

	/**
	 * If the given revision is newer than the currently set page_latest,
	 * update the page record. Otherwise, do nothing.
	 *
	 * @param Database $dbw
	 * @param Revision $revision
	 */
	function updateIfNewerOn( &$dbw, $revision ) {
		wfProfileIn( __METHOD__ );

		$row = $dbw->selectRow(
			array( 'revision', 'page' ),
			array( 'rev_id', 'rev_timestamp', 'page_is_redirect' ),
			array(
				'page_id' => $this->getId(),
				'page_latest=rev_id' ),
			__METHOD__ );
		if( $row ) {
			if( wfTimestamp(TS_MW, $row->rev_timestamp) >= $revision->getTimestamp() ) {
				wfProfileOut( __METHOD__ );
				return false;
			}
			$prev = $row->rev_id;
			$lastRevIsRedirect = (bool)$row->page_is_redirect;
		} else {
			# No or missing previous revision; mark the page as new
			$prev = 0;
			$lastRevIsRedirect = null;
		}

		$ret = $this->updateRevisionOn( $dbw, $revision, $prev, $lastRevIsRedirect );
		wfProfileOut( __METHOD__ );
		return $ret;
	}

	/**
	 * @return string Complete article text, or null if error
	 */
	function replaceSection($section, $text, $summary = '', $edittime = NULL) {
		wfProfileIn( __METHOD__ );

		if( $section == '' ) {
			// Whole-page edit; let the text through unmolested.
		} else {
			if( is_null( $edittime ) ) {
				$rev = Revision::newFromTitle( $this->mTitle );
			} else {
				$dbw = wfGetDB( DB_MASTER );
				$rev = Revision::loadFromTimestamp( $dbw, $this->mTitle, $edittime );
			}
			if( is_null( $rev ) ) {
				wfDebug( "Article::replaceSection asked for bogus section (page: " .
					$this->getId() . "; section: $section; edittime: $edittime)\n" );
				return null;
			}
			$oldtext = $rev->getText();

			if( $section == 'new' ) {
				# Inserting a new section
				$subject = $summary ? wfMsgForContent('newsectionheaderdefaultlevel',$summary) . "\n\n" : '';
				$text = strlen( trim( $oldtext ) ) > 0
						? "{$oldtext}\n\n{$subject}{$text}"
						: "{$subject}{$text}";
			} else {
				# Replacing an existing section; roll out the big guns
				global $wgParser;
				$text = $wgParser->replaceSection( $oldtext, $section, $text );
			}

		}

		wfProfileOut( __METHOD__ );
		return $text;
	}

	/**
	 * @deprecated use Article::doEdit()
	 */
	function insertNewArticle( $text, $summary, $isminor, $watchthis, $suppressRC=false, $comment=false, $bot=false ) {
		$flags = EDIT_NEW | EDIT_DEFER_UPDATES | EDIT_AUTOSUMMARY |
			( $isminor ? EDIT_MINOR : 0 ) |
			( $suppressRC ? EDIT_SUPPRESS_RC : 0 ) |
			( $bot ? EDIT_FORCE_BOT : 0 );

		# If this is a comment, add the summary as headline
		if ( $comment && $summary != "" ) {
			$text = wfMsgForContent('newsectionheaderdefaultlevel',$summary) . "\n\n".$text;
		}

		$this->doEdit( $text, $summary, $flags );

		$dbw = wfGetDB( DB_MASTER );
		if ($watchthis) {
			if (!$this->mTitle->userIsWatching()) {
				$dbw->begin();
				$this->doWatch();
				$dbw->commit();
			}
		} else {
			if ( $this->mTitle->userIsWatching() ) {
				$dbw->begin();
				$this->doUnwatch();
				$dbw->commit();
			}
		}
		$this->doRedirect( $this->isRedirect( $text ) );
	}

	/**
	 * @deprecated use Article::doEdit()
	 */
	function updateArticle( $text, $summary, $minor, $watchthis, $forceBot = false, $sectionanchor = '' ) {
		$flags = EDIT_UPDATE | EDIT_DEFER_UPDATES | EDIT_AUTOSUMMARY |
			( $minor ? EDIT_MINOR : 0 ) |
			( $forceBot ? EDIT_FORCE_BOT : 0 );

		$good = $this->doEdit( $text, $summary, $flags );
		if ( $good ) {
			$dbw = wfGetDB( DB_MASTER );
			if ($watchthis) {
				if (!$this->mTitle->userIsWatching()) {
					$dbw->begin();
					$this->doWatch();
					$dbw->commit();
				}
			} else {
				if ( $this->mTitle->userIsWatching() ) {
					$dbw->begin();
					$this->doUnwatch();
					$dbw->commit();
				}
			}

			$extraQuery = ''; // Give extensions a chance to modify URL query on update
			wfRunHooks( 'ArticleUpdateBeforeRedirect', array( $this, &$sectionanchor, &$extraQuery ) );

			$this->doRedirect( $this->isRedirect( $text ), $sectionanchor, $extraQuery );
		}
		return $good;
	}

	/**
	 * Article::doEdit()
	 *
	 * Change an existing article or create a new article. Updates RC and all necessary caches,
	 * optionally via the deferred update array.
	 *
	 * $wgUser must be set before calling this function.
	 *
	 * @param string $text New text
	 * @param string $summary Edit summary
	 * @param integer $flags bitfield:
	 *      EDIT_NEW
	 *          Article is known or assumed to be non-existent, create a new one
	 *      EDIT_UPDATE
	 *          Article is known or assumed to be pre-existing, update it
	 *      EDIT_MINOR
	 *          Mark this edit minor, if the user is allowed to do so
	 *      EDIT_SUPPRESS_RC
	 *          Do not log the change in recentchanges
	 *      EDIT_FORCE_BOT
	 *          Mark the edit a "bot" edit regardless of user rights
	 *      EDIT_DEFER_UPDATES
	 *          Defer some of the updates until the end of index.php
	 *      EDIT_AUTOSUMMARY
	 *          Fill in blank summaries with generated text where possible
	 *
	 * If neither EDIT_NEW nor EDIT_UPDATE is specified, the status of the article will be detected.
	 * If EDIT_UPDATE is specified and the article doesn't exist, the function will return false. If
	 * EDIT_NEW is specified and the article does exist, a duplicate key error will cause an exception
	 * to be thrown from the Database. These two conditions are also possible with auto-detection due
	 * to MediaWiki's performance-optimised locking strategy.
	 * @param $baseRevId, the revision ID this edit was based off, if any
	 *
	 * @return bool success
	 */
	function doEdit( $text, $summary, $flags = 0, $baseRevId = false ) {
		global $wgUser, $wgDBtransactions, $wgUseAutomaticEditSummaries;

		wfProfileIn( __METHOD__ );
		$good = true;

		if ( !($flags & EDIT_NEW) && !($flags & EDIT_UPDATE) ) {
			$aid = $this->mTitle->getArticleID( GAID_FOR_UPDATE );
			if ( $aid ) {
				$flags |= EDIT_UPDATE;
			} else {
				$flags |= EDIT_NEW;
			}
		}

		if( !wfRunHooks( 'ArticleSave', array( &$this, &$wgUser, &$text,
			&$summary, $flags & EDIT_MINOR,
			null, null, &$flags ) ) )
		{
			wfDebug( __METHOD__ . ": ArticleSave hook aborted save!\n" );
			wfProfileOut( __METHOD__ );
			return false;
		}

		# Silently ignore EDIT_MINOR if not allowed
		$isminor = ( $flags & EDIT_MINOR ) && $wgUser->isAllowed('minoredit');
		$bot = $flags & EDIT_FORCE_BOT;

		$oldtext = $this->getContent();
		$oldsize = strlen( $oldtext );

		# Provide autosummaries if one is not provided and autosummaries are enabled.
		if( $wgUseAutomaticEditSummaries && $flags & EDIT_AUTOSUMMARY && $summary == '' ) {
			$summary = $this->getAutosummary( $oldtext, $text, $flags );
		}

		$editInfo = $this->prepareTextForEdit( $text );
		$text = $editInfo->pst;
		$newsize = strlen( $text );

		$dbw = wfGetDB( DB_MASTER );
		$now = wfTimestampNow();

		if ( $flags & EDIT_UPDATE ) {
			# Update article, but only if changed.

			# Make sure the revision is either completely inserted or not inserted at all
			if( !$wgDBtransactions ) {
				$userAbort = ignore_user_abort( true );
			}

			$lastRevision = 0;
			$revisionId = 0;

			$changed = ( strcmp( $text, $oldtext ) != 0 );

			if ( $changed ) {
				$this->mGoodAdjustment = (int)$this->isCountable( $text )
				  - (int)$this->isCountable( $oldtext );
				$this->mTotalAdjustment = 0;

				$lastRevision = $dbw->selectField(
					'page', 'page_latest', array( 'page_id' => $this->getId() ) );

				if ( !$lastRevision ) {
					# Article gone missing
					wfDebug( __METHOD__.": EDIT_UPDATE specified but article doesn't exist\n" );
					wfProfileOut( __METHOD__ );
					return false;
				}

				$revision = new Revision( array(
					'page'       => $this->getId(),
					'comment'    => $summary,
					'minor_edit' => $isminor,
					'text'       => $text,
					'parent_id'  => $lastRevision
					) );

				$dbw->begin();
				$revisionId = $revision->insertOn( $dbw );

				# Update page
				$ok = $this->updateRevisionOn( $dbw, $revision, $lastRevision );

				if( !$ok ) {
					/* Belated edit conflict! Run away!! */
					$good = false;
					$dbw->rollback();
				} else {
					wfRunHooks( 'NewRevisionFromEditComplete', array( $this, $revision, $baseRevId ) );

					# Update recentchanges
					if( !( $flags & EDIT_SUPPRESS_RC ) ) {
						$rcid = RecentChange::notifyEdit( $now, $this->mTitle, $isminor, $wgUser, $summary,
							$lastRevision, $this->getTimestamp(), $bot, '', $oldsize, $newsize,
							$revisionId );

						# Mark as patrolled if the user can do so
						if( $GLOBALS['wgUseRCPatrol'] && $wgUser->isAllowed( 'autopatrol' ) ) {
							RecentChange::markPatrolled( $rcid );
							PatrolLog::record( $rcid, true );
						}
					}
					$wgUser->incEditCount();
					$dbw->commit();
				}
			} else {
				$revision = null;
				// Keep the same revision ID, but do some updates on it
				$revisionId = $this->getRevIdFetched();
				// Update page_touched, this is usually implicit in the page update
				// Other cache updates are done in onArticleEdit()
				$this->mTitle->invalidateCache();
			}

			if( !$wgDBtransactions ) {
				ignore_user_abort( $userAbort );
			}

			if ( $good ) {
				# Invalidate cache of this article and all pages using this article
				# as a template. Partly deferred.
				Article::onArticleEdit( $this->mTitle );

				# Update links tables, site stats, etc.
				$this->editUpdates( $text, $summary, $isminor, $now, $revisionId, $changed );
			}
		} else {
			# Create new article

			# Set statistics members
			# We work out if it's countable after PST to avoid counter drift
			# when articles are created with {{subst:}}
			$this->mGoodAdjustment = (int)$this->isCountable( $text );
			$this->mTotalAdjustment = 1;

			$dbw->begin();

			# Add the page record; stake our claim on this title!
			# This will fail with a database query exception if the article already exists
			$newid = $this->insertOn( $dbw );

			# Save the revision text...
			$revision = new Revision( array(
				'page'       => $newid,
				'comment'    => $summary,
				'minor_edit' => $isminor,
				'text'       => $text
				) );
			$revisionId = $revision->insertOn( $dbw );

			$this->mTitle->resetArticleID( $newid );

			# Update the page record with revision data
			$this->updateRevisionOn( $dbw, $revision, 0 );
			
			wfRunHooks( 'NewRevisionFromEditComplete', array($this, $revision, false) );

			if( !( $flags & EDIT_SUPPRESS_RC ) ) {
				$rcid = RecentChange::notifyNew( $now, $this->mTitle, $isminor, $wgUser, $summary, $bot,
				  '', strlen( $text ), $revisionId );
				# Mark as patrolled if the user can
				if( ($GLOBALS['wgUseRCPatrol'] || $GLOBALS['wgUseNPPatrol']) && $wgUser->isAllowed( 'autopatrol' ) ) {
					RecentChange::markPatrolled( $rcid );
					PatrolLog::record( $rcid, true );
				}
			}
			$wgUser->incEditCount();
			$dbw->commit();

			# Update links, etc.
			$this->editUpdates( $text, $summary, $isminor, $now, $revisionId, true );

			# Clear caches
			Article::onArticleCreate( $this->mTitle );

			wfRunHooks( 'ArticleInsertComplete', array( &$this, &$wgUser, $text, $summary,
			 $flags & EDIT_MINOR, null, null, &$flags, $revision ) );
		}

		if ( $good && !( $flags & EDIT_DEFER_UPDATES ) ) {
			wfDoUpdates();
		}

		if ( $good ) {
			wfRunHooks( 'ArticleSaveComplete', array( &$this, &$wgUser, $text, $summary,
				$flags & EDIT_MINOR, null, null, &$flags, $revision ) );
		}

		wfProfileOut( __METHOD__ );
		return $good;
	}

	/**
	 * @deprecated wrapper for doRedirect
	 */
	function showArticle( $text, $subtitle , $sectionanchor = '', $me2, $now, $summary, $oldid ) {
		$this->doRedirect( $this->isRedirect( $text ), $sectionanchor );
	}

	/**
	 * Output a redirect back to the article.
	 * This is typically used after an edit.
	 *
	 * @param boolean $noRedir Add redirect=no
	 * @param string $sectionAnchor section to redirect to, including "#"
	 * @param string $extraQuery, extra query params
	 */
	function doRedirect( $noRedir = false, $sectionAnchor = '', $extraQuery = '' ) {
		global $wgOut;
		if ( $noRedir ) {
			$query = 'redirect=no';
			if( $extraQuery )
				$query .= "&$query";
		} else {
			$query = $extraQuery;
		}
		$wgOut->redirect( $this->mTitle->getFullURL( $query ) . $sectionAnchor );
	}

	/**
	 * Mark this particular edit/page as patrolled
	 */
	function markpatrolled() {
		global $wgOut, $wgRequest, $wgUseRCPatrol, $wgUseNPPatrol, $wgUser;
		$wgOut->setRobotPolicy( 'noindex,nofollow' );

		# Check patrol config options

		if ( !($wgUseNPPatrol || $wgUseRCPatrol)) {
			$wgOut->showErrorPage( 'rcpatroldisabled', 'rcpatroldisabledtext' );
			return;
		}

		# If we haven't been given an rc_id value, we can't do anything
		$rcid = (int) $wgRequest->getVal('rcid');
		$rc = $rcid ? RecentChange::newFromId($rcid) : null;
		if ( is_null ( $rc ) )
		{
			$wgOut->showErrorPage( 'markedaspatrollederror', 'markedaspatrollederrortext' );
			return;
		}

		if ( !$wgUseRCPatrol && $rc->getAttribute( 'rc_type' ) != RC_NEW) {
			// Only new pages can be patrolled if the general patrolling is off....???
			// @fixme -- is this necessary? Shouldn't we only bother controlling the
			// front end here?
			$wgOut->showErrorPage( 'rcpatroldisabled', 'rcpatroldisabledtext' );
			return;
		}

		# Check permissions
		$permission_errors = $this->mTitle->getUserPermissionsErrors( 'patrol', $wgUser );

		if (count($permission_errors)>0)
		{
			$wgOut->showPermissionsErrorPage( $permission_errors );
			return;
		}

		# Handle the 'MarkPatrolled' hook
		if( !wfRunHooks( 'MarkPatrolled', array( $rcid, &$wgUser, false ) ) ) {
			return;
		}

		#It would be nice to see where the user had actually come from, but for now just guess
		$returnto = $rc->getAttribute( 'rc_type' ) == RC_NEW ? 'Newpages' : 'Recentchanges';
		$return = Title::makeTitle( NS_SPECIAL, $returnto );

		# If it's left up to us, check that the user is allowed to patrol this edit
		# If the user has the "autopatrol" right, then we'll assume there are no
		# other conditions stopping them doing so
		if( !$wgUser->isAllowed( 'autopatrol' ) ) {
			$rc = RecentChange::newFromId( $rcid );
			# Graceful error handling, as we've done before here...
			# (If the recent change doesn't exist, then it doesn't matter whether
			# the user is allowed to patrol it or not; nothing is going to happen
			if( is_object( $rc ) && $wgUser->getName() == $rc->getAttribute( 'rc_user_text' ) ) {
				# The user made this edit, and can't patrol it
				# Tell them so, and then back off
				$wgOut->setPageTitle( wfMsg( 'markedaspatrollederror' ) );
				$wgOut->addWikiMsg( 'markedaspatrollederror-noautopatrol' );
				$wgOut->returnToMain( false, $return );
				return;
			}
		}

		# Check that the revision isn't patrolled already
		# Prevents duplicate log entries
		if( !$rc->getAttribute( 'rc_patrolled' ) ) {
			# Mark the edit as patrolled
			RecentChange::markPatrolled( $rcid );
			PatrolLog::record( $rcid );
			wfRunHooks( 'MarkPatrolledComplete', array( &$rcid, &$wgUser, false ) );
		}

		# Inform the user
		$wgOut->setPageTitle( wfMsg( 'markedaspatrolled' ) );
		$wgOut->addWikiMsg( 'markedaspatrolledtext' );
		$wgOut->returnToMain( false, $return );
	}

	/**
	 * User-interface handler for the "watch" action
	 */

	function watch() {

		global $wgUser, $wgOut;

		if ( $wgUser->isAnon() ) {
			$wgOut->showErrorPage( 'watchnologin', 'watchnologintext' );
			return;
		}
		if ( wfReadOnly() ) {
			$wgOut->readOnlyPage();
			return;
		}

		if( $this->doWatch() ) {
			$wgOut->setPagetitle( wfMsg( 'addedwatch' ) );
			$wgOut->setRobotpolicy( 'noindex,nofollow' );

			$wgOut->addWikiMsg( 'addedwatchtext', $this->mTitle->getPrefixedText() );
		}

		$wgOut->returnToMain( true, $this->mTitle->getPrefixedText() );
	}

	/**
	 * Add this page to $wgUser's watchlist
	 * @return bool true on successful watch operation
	 */
	function doWatch() {
		global $wgUser;
		if( $wgUser->isAnon() ) {
			return false;
		}

		if (wfRunHooks('WatchArticle', array(&$wgUser, &$this))) {
			$wgUser->addWatch( $this->mTitle );

			return wfRunHooks('WatchArticleComplete', array(&$wgUser, &$this));
		}

		return false;
	}

	/**
	 * User interface handler for the "unwatch" action.
	 */
	function unwatch() {

		global $wgUser, $wgOut;

		if ( $wgUser->isAnon() ) {
			$wgOut->showErrorPage( 'watchnologin', 'watchnologintext' );
			return;
		}
		if ( wfReadOnly() ) {
			$wgOut->readOnlyPage();
			return;
		}

		if( $this->doUnwatch() ) {
			$wgOut->setPagetitle( wfMsg( 'removedwatch' ) );
			$wgOut->setRobotpolicy( 'noindex,nofollow' );

			$wgOut->addWikiMsg( 'removedwatchtext', $this->mTitle->getPrefixedText() );
		}

		$wgOut->returnToMain( true, $this->mTitle->getPrefixedText() );
	}

	/**
	 * Stop watching a page
	 * @return bool true on successful unwatch
	 */
	function doUnwatch() {
		global $wgUser;
		if( $wgUser->isAnon() ) {
			return false;
		}

		if (wfRunHooks('UnwatchArticle', array(&$wgUser, &$this))) {
			$wgUser->removeWatch( $this->mTitle );

			return wfRunHooks('UnwatchArticleComplete', array(&$wgUser, &$this));
		}

		return false;
	}

	/**
	 * action=protect handler
	 */
	function protect() {
		$form = new ProtectionForm( $this );
		$form->execute();
	}

	/**
	 * action=unprotect handler (alias)
	 */
	function unprotect() {
		$this->protect();
	}

	/**
	 * Update the article's restriction field, and leave a log entry.
	 *
	 * @param array $limit set of restriction keys
	 * @param string $reason
	 * @return bool true on success
	 */
	function updateRestrictions( $limit = array(), $reason = '', $cascade = 0, $expiry = null ) {
		global $wgUser, $wgRestrictionTypes, $wgContLang;

		$id = $this->mTitle->getArticleID();
		if( array() != $this->mTitle->getUserPermissionsErrors( 'protect', $wgUser ) || wfReadOnly() || $id == 0 ) {
			return false;
		}

		if (!$cascade) {
			$cascade = false;
		}

		// Take this opportunity to purge out expired restrictions
		Title::purgeExpiredRestrictions();

		# FIXME: Same limitations as described in ProtectionForm.php (line 37);
		# we expect a single selection, but the schema allows otherwise.
		$current = array();
		foreach( $wgRestrictionTypes as $action )
			$current[$action] = implode( '', $this->mTitle->getRestrictions( $action ) );

		$current = Article::flattenRestrictions( $current );
		$updated = Article::flattenRestrictions( $limit );

		$changed = ( $current != $updated );
		$changed = $changed || ($updated && $this->mTitle->areRestrictionsCascading() != $cascade);
		$changed = $changed || ($updated && $this->mTitle->mRestrictionsExpiry != $expiry);
		$protect = ( $updated != '' );

		# If nothing's changed, do nothing
		if( $changed ) {
			global $wgGroupPermissions;
			if( wfRunHooks( 'ArticleProtect', array( &$this, &$wgUser, $limit, $reason ) ) ) {

				$dbw = wfGetDB( DB_MASTER );

				$encodedExpiry = Block::encodeExpiry($expiry, $dbw );

				$expiry_description = '';
				if ( $encodedExpiry != 'infinity' ) {
					$expiry_description = ' (' . wfMsgForContent( 'protect-expiring', $wgContLang->timeanddate( $expiry, false, false ) ).')';
				}

				# Prepare a null revision to be added to the history
				$modified = $current != '' && $protect;
				if ( $protect ) {
					$comment_type = $modified ? 'modifiedarticleprotection' : 'protectedarticle';
				} else {
					$comment_type = 'unprotectedarticle';
				}
				$comment = $wgContLang->ucfirst( wfMsgForContent( $comment_type, $this->mTitle->getPrefixedText() ) );

				# Only restrictions with the 'protect' right can cascade...
				# Otherwise, people who cannot normally protect can "protect" pages via transclusion
				foreach( $limit as $action => $restriction ) {
					# FIXME: can $restriction be an array or what? (same as fixme above)
					if( $restriction != 'protect' && $restriction != 'sysop' ) {
						$cascade = false;
						break;
					}
				}

				$cascade_description = '';
				if ($cascade) {
					$cascade_description = ' ['.wfMsg('protect-summary-cascade').']';
				}

				if( $reason )
					$comment .= ": $reason";
				if( $protect )
					$comment .= " [$updated]";
				if ( $expiry_description && $protect )
					$comment .= "$expiry_description";
				if ( $cascade )
					$comment .= "$cascade_description";

				# Update restrictions table
				foreach( $limit as $action => $restrictions ) {
					if ($restrictions != '' ) {
						$dbw->replace( 'page_restrictions', array(array('pr_page', 'pr_type')),
							array( 'pr_page' => $id, 'pr_type' => $action
								, 'pr_level' => $restrictions, 'pr_cascade' => $cascade ? 1 : 0
								, 'pr_expiry' => $encodedExpiry ), __METHOD__  );
					} else {
						$dbw->delete( 'page_restrictions', array( 'pr_page' => $id,
							'pr_type' => $action ), __METHOD__ );
					}
				}

				# Insert a null revision
				$nullRevision = Revision::newNullRevision( $dbw, $id, $comment, true );
				$nullRevId = $nullRevision->insertOn( $dbw );

				# Update page record
				$dbw->update( 'page',
					array( /* SET */
						'page_touched' => $dbw->timestamp(),
						'page_restrictions' => '',
						'page_latest' => $nullRevId
					), array( /* WHERE */
						'page_id' => $id
					), 'Article::protect'
				);
				
				wfRunHooks( 'NewRevisionFromEditComplete', array($this, $nullRevision, false) );
				wfRunHooks( 'ArticleProtectComplete', array( &$this, &$wgUser, $limit, $reason ) );

				# Update the protection log
				$log = new LogPage( 'protect' );
				if( $protect ) {
					$log->addEntry( $modified ? 'modify' : 'protect', $this->mTitle, 
						trim( $reason . " [$updated]$cascade_description$expiry_description" ) );
				} else {
					$log->addEntry( 'unprotect', $this->mTitle, $reason );
				}

			} # End hook
		} # End "changed" check

		return true;
	}

	/**
	 * Take an array of page restrictions and flatten it to a string
	 * suitable for insertion into the page_restrictions field.
	 * @param array $limit
	 * @return string
	 * @private
	 */
	function flattenRestrictions( $limit ) {
		if( !is_array( $limit ) ) {
			throw new MWException( 'Article::flattenRestrictions given non-array restriction set' );
		}
		$bits = array();
		ksort( $limit );
		foreach( $limit as $action => $restrictions ) {
			if( $restrictions != '' ) {
				$bits[] = "$action=$restrictions";
			}
		}
		return implode( ':', $bits );
	}

	/**
	 * Auto-generates a deletion reason
	 * @param bool &$hasHistory Whether the page has a history
	 */
	public function generateReason(&$hasHistory)
	{
		global $wgContLang;
		$dbw = wfGetDB(DB_MASTER);
		// Get the last revision
		$rev = Revision::newFromTitle($this->mTitle);
		if(is_null($rev))
			return false;
		// Get the article's contents
		$contents = $rev->getText();
		$blank = false;
		// If the page is blank, use the text from the previous revision,
		// which can only be blank if there's a move/import/protect dummy revision involved
		if($contents == '')
		{
			$prev = $rev->getPrevious();
			if($prev)
			{
				$contents = $prev->getText();
				$blank = true;
			}
		}

		// Find out if there was only one contributor
		// Only scan the last 20 revisions
		$limit = 20;
		$res = $dbw->select('revision', 'rev_user_text', array('rev_page' => $this->getID()), __METHOD__,
				array('LIMIT' => $limit));
		if($res === false)
			// This page has no revisions, which is very weird
			return false;
		if($res->numRows() > 1)
				$hasHistory = true;
		else
				$hasHistory = false;
		$row = $dbw->fetchObject($res);
		$onlyAuthor = $row->rev_user_text;
		// Try to find a second contributor
		while( $row = $dbw->fetchObject($res) ) {
			if($row->rev_user_text != $onlyAuthor) {
				$onlyAuthor = false;
				break;
			}
		}
		$dbw->freeResult($res);

		// Generate the summary with a '$1' placeholder
		if($blank) {
			// The current revision is blank and the one before is also
			// blank. It's just not our lucky day
			$reason = wfMsgForContent('exbeforeblank', '$1');
		} else {
			if($onlyAuthor)
				$reason = wfMsgForContent('excontentauthor', '$1', $onlyAuthor);
			else
				$reason = wfMsgForContent('excontent', '$1');
		}

		// Replace newlines with spaces to prevent uglyness
		$contents = preg_replace("/[\n\r]/", ' ', $contents);
		// Calculate the maximum amount of chars to get
		// Max content length = max comment length - length of the comment (excl. $1) - '...'
		$maxLength = 255 - (strlen($reason) - 2) - 3;
		$contents = $wgContLang->truncate($contents, $maxLength, '...');
		// Remove possible unfinished links
		$contents = preg_replace( '/\[\[([^\]]*)\]?$/', '$1', $contents );
		// Now replace the '$1' placeholder
		$reason = str_replace( '$1', $contents, $reason );
		return $reason;
	}


	/*
	 * UI entry point for page deletion
	 */
	function delete() {
		global $wgUser, $wgOut, $wgRequest;

		$confirm = $wgRequest->wasPosted() &&
				$wgUser->matchEditToken( $wgRequest->getVal( 'wpEditToken' ) );

		$this->DeleteReasonList = $wgRequest->getText( 'wpDeleteReasonList', 'other' );
		$this->DeleteReason = $wgRequest->getText( 'wpReason' );

		$reason = $this->DeleteReasonList;

		if ( $reason != 'other' && $this->DeleteReason != '') {
			// Entry from drop down menu + additional comment
			$reason .= ': ' . $this->DeleteReason;
		} elseif ( $reason == 'other' ) {
			$reason = $this->DeleteReason;
		}
		# Flag to hide all contents of the archived revisions
		$suppress = $wgRequest->getVal( 'wpSuppress' ) && $wgUser->isAllowed('suppressrevision');

		# This code desperately needs to be totally rewritten

		# Read-only check...
		if ( wfReadOnly() ) {
			$wgOut->readOnlyPage();
			return;
		}

		# Check permissions
		$permission_errors = $this->mTitle->getUserPermissionsErrors( 'delete', $wgUser );

		if (count($permission_errors)>0) {
			$wgOut->showPermissionsErrorPage( $permission_errors );
			return;
		}

		$wgOut->setPagetitle( wfMsg( 'delete-confirm', $this->mTitle->getPrefixedText() ) );

		# Better double-check that it hasn't been deleted yet!
		$dbw = wfGetDB( DB_MASTER );
		$conds = $this->mTitle->pageCond();
		$latest = $dbw->selectField( 'page', 'page_latest', $conds, __METHOD__ );
		if ( $latest === false ) {
			$wgOut->showFatalError( wfMsg( 'cannotdelete' ) );
			return;
		}

		# Hack for big sites
		$bigHistory = $this->isBigDeletion();
		if( $bigHistory && !$this->mTitle->userCan( 'bigdelete' ) ) {
			global $wgLang, $wgDeleteRevisionsLimit;
			$wgOut->wrapWikiMsg( "<div class='error'>\n$1</div>\n",
				array( 'delete-toobig', $wgLang->formatNum( $wgDeleteRevisionsLimit ) ) );
			return;
		}

		if( $confirm ) {
			$this->doDelete( $reason, $suppress );
			if( $wgRequest->getCheck( 'wpWatch' ) ) {
				$this->doWatch();
			} elseif( $this->mTitle->userIsWatching() ) {
				$this->doUnwatch();
			}
			return;
		}

		// Generate deletion reason
		$hasHistory = false;
		if ( !$reason ) $reason = $this->generateReason($hasHistory);

		// If the page has a history, insert a warning
		if( $hasHistory && !$confirm ) {
			$skin=$wgUser->getSkin();
			$wgOut->addHTML( '<strong>' . wfMsg( 'historywarning' ) . ' ' . $skin->historyLink() . '</strong>' );
			if( $bigHistory ) {
				global $wgLang, $wgDeleteRevisionsLimit;
				$wgOut->wrapWikiMsg( "<div class='error'>\n$1</div>\n",
					array( 'delete-warning-toobig', $wgLang->formatNum( $wgDeleteRevisionsLimit ) ) );
			}
		}

		return $this->confirmDelete( $reason );
	}

	/**
	 * @return bool whether or not the page surpasses $wgDeleteRevisionsLimit revisions
	 */
	function isBigDeletion() {
		global $wgDeleteRevisionsLimit;
		if( $wgDeleteRevisionsLimit ) {
			$revCount = $this->estimateRevisionCount();
			return $revCount > $wgDeleteRevisionsLimit;
		}
		return false;
	}

	/**
	 * @return int approximate revision count
	 */
	function estimateRevisionCount() {
		$dbr = wfGetDB();
		// For an exact count...
		//return $dbr->selectField( 'revision', 'COUNT(*)',
		//	array( 'rev_page' => $this->getId() ), __METHOD__ );
		return $dbr->estimateRowCount( 'revision', '*',
		 	array( 'rev_page' => $this->getId() ), __METHOD__ );
	}

	/**
	 * Get the last N authors
	 * @param int $num Number of revisions to get
	 * @param string $revLatest The latest rev_id, selected from the master (optional)
	 * @return array Array of authors, duplicates not removed
	 */
	function getLastNAuthors( $num, $revLatest = 0 ) {
		wfProfileIn( __METHOD__ );

		// First try the slave
		// If that doesn't have the latest revision, try the master
		$continue = 2;
		$db = wfGetDB( DB_SLAVE );
		do {
			$res = $db->select( array( 'page', 'revision' ),
				array( 'rev_id', 'rev_user_text' ),
				array(
					'page_namespace' => $this->mTitle->getNamespace(),
					'page_title' => $this->mTitle->getDBkey(),
					'rev_page = page_id'
				), __METHOD__, $this->getSelectOptions( array(
					'ORDER BY' => 'rev_timestamp DESC',
					'LIMIT' => $num
				) )
			);
			if ( !$res ) {
				wfProfileOut( __METHOD__ );
				return array();
			}
			$row = $db->fetchObject( $res );
			if ( $continue == 2 && $revLatest && $row->rev_id != $revLatest ) {
				$db = wfGetDB( DB_MASTER );
				$continue--;
			} else {
				$continue = 0;
			}
		} while ( $continue );

		$authors = array( $row->rev_user_text );
		while ( $row = $db->fetchObject( $res ) ) {
			$authors[] = $row->rev_user_text;
		}
		wfProfileOut( __METHOD__ );
		return $authors;
	}

	/**
	 * Output deletion confirmation dialog
	 * @param $reason string Prefilled reason
	 */
	function confirmDelete( $reason ) {
		global $wgOut, $wgUser, $wgContLang;
		$align = $wgContLang->isRtl() ? 'left' : 'right';

		wfDebug( "Article::confirmDelete\n" );

		$wgOut->setSubtitle( wfMsg( 'delete-backlink', $wgUser->getSkin()->makeKnownLinkObj( $this->mTitle ) ) );
		$wgOut->setRobotpolicy( 'noindex,nofollow' );
		$wgOut->addWikiMsg( 'confirmdeletetext' );

		if( $wgUser->isAllowed( 'suppressrevision' ) ) {
			$suppress = "<tr id=\"wpDeleteSuppressRow\" name=\"wpDeleteSuppressRow\"><td></td><td>";
			$suppress .= Xml::checkLabel( wfMsg( 'revdelete-suppress' ), 'wpSuppress', 'wpSuppress', false, array( 'tabindex' => '2' ) );
			$suppress .= "</td></tr>";
		} else {
			$suppress = '';
		}

		$form = Xml::openElement( 'form', array( 'method' => 'post', 'action' => $this->mTitle->getLocalURL( 'action=delete' ), 'id' => 'deleteconfirm' ) ) .
			Xml::openElement( 'fieldset', array( 'id' => 'mw-delete-table' ) ) .
			Xml::tags( 'legend', null, wfMsgExt( 'delete-legend', array( 'parsemag', 'escapenoentities' ) ) ) .
			Xml::openElement( 'table' ) .
			"<tr id=\"wpDeleteReasonListRow\">
				<td align='$align'>" .
					Xml::label( wfMsg( 'deletecomment' ), 'wpDeleteReasonList' ) .
				"</td>
				<td>" .
					Xml::listDropDown( 'wpDeleteReasonList',
						wfMsgForContent( 'deletereason-dropdown' ),
						wfMsgForContent( 'deletereasonotherlist' ), '', 'wpReasonDropDown', 1 ) .
				"</td>
			</tr>
			<tr id=\"wpDeleteReasonRow\">
				<td align='$align'>" .
					Xml::label( wfMsg( 'deleteotherreason' ), 'wpReason' ) .
				"</td>
				<td>" .
					Xml::input( 'wpReason', 60, $reason, array( 'type' => 'text', 'maxlength' => '255', 'tabindex' => '2', 'id' => 'wpReason' ) ) .
				"</td>
			</tr>
			<tr>
				<td></td>
				<td>" .
					Xml::checkLabel( wfMsg( 'watchthis' ), 'wpWatch', 'wpWatch', $wgUser->getBoolOption( 'watchdeletion' ) || $this->mTitle->userIsWatching(), array( 'tabindex' => '3' ) ) .
				"</td>
			</tr>
			$suppress
			<tr>
				<td></td>
				<td>" .
					Xml::submitButton( wfMsg( 'deletepage' ), array( 'name' => 'wpConfirmB', 'id' => 'wpConfirmB', 'tabindex' => '4' ) ) .
				"</td>
			</tr>" .
			Xml::closeElement( 'table' ) .
			Xml::closeElement( 'fieldset' ) .
			Xml::hidden( 'wpEditToken', $wgUser->editToken() ) .
			Xml::closeElement( 'form' );

			if ( $wgUser->isAllowed( 'editinterface' ) ) {
				$skin = $wgUser->getSkin();
				$link = $skin->makeLink ( 'MediaWiki:Deletereason-dropdown', wfMsgHtml( 'delete-edit-reasonlist' ) );
				$form .= '<p class="mw-delete-editreasons">' . $link . '</p>';
			}

		$wgOut->addHTML( $form );
		$this->showLogExtract( $wgOut );
	}


	/**
	 * Show relevant lines from the deletion log
	 */
	function showLogExtract( $out ) {
		$out->addHtml( Xml::element( 'h2', null, LogPage::logName( 'delete' ) ) );
		LogEventsList::showLogExtract( $out, 'delete', $this->mTitle->getPrefixedText() );
	}


	/**
	 * Perform a deletion and output success or failure messages
	 */
	function doDelete( $reason, $suppress = false ) {
		global $wgOut, $wgUser;
		wfDebug( __METHOD__."\n" );
		
		$id = $this->getId();
		
		$error = '';

		if (wfRunHooks('ArticleDelete', array(&$this, &$wgUser, &$reason, &$error))) {
			if ( $this->doDeleteArticle( $reason, $suppress ) ) {
				$deleted = $this->mTitle->getPrefixedText();

				$wgOut->setPagetitle( wfMsg( 'actioncomplete' ) );
				$wgOut->setRobotpolicy( 'noindex,nofollow' );

				$loglink = '[[Special:Log/delete|' . wfMsgNoTrans( 'deletionlog' ) . ']]';

				$wgOut->addWikiMsg( 'deletedtext', $deleted, $loglink );
				$wgOut->returnToMain( false );
				wfRunHooks('ArticleDeleteComplete', array(&$this, &$wgUser, $reason, $id));
			} else {
				if ($error = '')
					$wgOut->showFatalError( wfMsg( 'cannotdelete' ) );
				else
					$wgOut->showFatalError( $error );
			}
		}
	}

	/**
	 * Back-end article deletion
	 * Deletes the article with database consistency, writes logs, purges caches
	 * Returns success
	 */
	function doDeleteArticle( $reason, $suppress = false ) {
		global $wgUseSquid, $wgDeferredUpdateList;
		global $wgUseTrackbacks;

		wfDebug( __METHOD__."\n" );

		$dbw = wfGetDB( DB_MASTER );
		$ns = $this->mTitle->getNamespace();
		$t = $this->mTitle->getDBkey();
		$id = $this->mTitle->getArticleID();

		if ( $t == '' || $id == 0 ) {
			return false;
		}

		$u = new SiteStatsUpdate( 0, 1, -(int)$this->isCountable( $this->getContent() ), -1 );
		array_push( $wgDeferredUpdateList, $u );

		// Bitfields to further suppress the content
		if ( $suppress ) {
			$bitfield = 0;
			// This should be 15...
			$bitfield |= Revision::DELETED_TEXT;
			$bitfield |= Revision::DELETED_COMMENT;
			$bitfield |= Revision::DELETED_USER;
			$bitfield |= Revision::DELETED_RESTRICTED;
		} else {
			$bitfield = 'rev_deleted';
		}

		$dbw->begin();
		// For now, shunt the revision data into the archive table.
		// Text is *not* removed from the text table; bulk storage
		// is left intact to avoid breaking block-compression or
		// immutable storage schemes.
		//
		// For backwards compatibility, note that some older archive
		// table entries will have ar_text and ar_flags fields still.
		//
		// In the future, we may keep revisions and mark them with
		// the rev_deleted field, which is reserved for this purpose.
		$dbw->insertSelect( 'archive', array( 'page', 'revision' ),
			array(
				'ar_namespace'  => 'page_namespace',
				'ar_title'      => 'page_title',
				'ar_comment'    => 'rev_comment',
				'ar_user'       => 'rev_user',
				'ar_user_text'  => 'rev_user_text',
				'ar_timestamp'  => 'rev_timestamp',
				'ar_minor_edit' => 'rev_minor_edit',
				'ar_rev_id'     => 'rev_id',
				'ar_text_id'    => 'rev_text_id',
				'ar_text'       => '\'\'', // Be explicit to appease
				'ar_flags'      => '\'\'', // MySQL's "strict mode"...
				'ar_len'        => 'rev_len',
				'ar_page_id'    => 'page_id',
				'ar_deleted'    => $bitfield
			), array(
				'page_id' => $id,
				'page_id = rev_page'
			), __METHOD__
		);

		# Delete restrictions for it
		$dbw->delete( 'page_restrictions', array ( 'pr_page' => $id ), __METHOD__ );

		# Fix category table counts
		$cats = array();
		$res = $dbw->select( 'categorylinks', 'cl_to',
			array( 'cl_from' => $id ), __METHOD__ );
		foreach( $res as $row ) {
			$cats []= $row->cl_to;
		}
		$this->updateCategoryCounts( array(), $cats );

		# Now that it's safely backed up, delete it
		$dbw->delete( 'page', array( 'page_id' => $id ), __METHOD__);
		$ok = ( $dbw->affectedRows() > 0 ); // getArticleId() uses slave, could be laggy
		if( !$ok ) {
			$dbw->rollback();
			return false;
		}

		# If using cascading deletes, we can skip some explicit deletes
		if ( !$dbw->cascadingDeletes() ) {
			$dbw->delete( 'revision', array( 'rev_page' => $id ), __METHOD__ );

			if ($wgUseTrackbacks)
				$dbw->delete( 'trackbacks', array( 'tb_page' => $id ), __METHOD__ );

			# Delete outgoing links
			$dbw->delete( 'pagelinks', array( 'pl_from' => $id ) );
			$dbw->delete( 'imagelinks', array( 'il_from' => $id ) );
			$dbw->delete( 'categorylinks', array( 'cl_from' => $id ) );
			$dbw->delete( 'templatelinks', array( 'tl_from' => $id ) );
			$dbw->delete( 'externallinks', array( 'el_from' => $id ) );
			$dbw->delete( 'langlinks', array( 'll_from' => $id ) );
			$dbw->delete( 'redirect', array( 'rd_from' => $id ) );
		}

		# If using cleanup triggers, we can skip some manual deletes
		if ( !$dbw->cleanupTriggers() ) {

			# Clean up recentchanges entries...
			$dbw->delete( 'recentchanges',
				array( 'rc_namespace' => $ns, 'rc_title' => $t, 'rc_type != '.RC_LOG ),
				__METHOD__ );
		}
		$dbw->commit();

		# Clear caches
		Article::onArticleDelete( $this->mTitle );

		# Clear the cached article id so the interface doesn't act like we exist
		$this->mTitle->resetArticleID( 0 );
		$this->mTitle->mArticleID = 0;

		# Log the deletion, if the page was suppressed, log it at Oversight instead
		$logtype = $suppress ? 'suppress' : 'delete';
		$log = new LogPage( $logtype );

		# Make sure logging got through
		$log->addEntry( 'delete', $this->mTitle, $reason, array() );

		return true;
	}

	/**
	 * Roll back the most recent consecutive set of edits to a page
	 * from the same user; fails if there are no eligible edits to
	 * roll back to, e.g. user is the sole contributor. This function
	 * performs permissions checks on $wgUser, then calls commitRollback()
	 * to do the dirty work
	 *
	 * @param string $fromP - Name of the user whose edits to rollback.
	 * @param string $summary - Custom summary. Set to default summary if empty.
	 * @param string $token - Rollback token.
	 * @param bool   $bot - If true, mark all reverted edits as bot.
	 *
	 * @param array $resultDetails contains result-specific array of additional values
	 *    'alreadyrolled' : 'current' (rev)
	 *    success        : 'summary' (str), 'current' (rev), 'target' (rev)
	 *
	 * @return array of errors, each error formatted as
	 *   array(messagekey, param1, param2, ...).
	 * On success, the array is empty.  This array can also be passed to
	 * OutputPage::showPermissionsErrorPage().
	 */
	public function doRollback( $fromP, $summary, $token, $bot, &$resultDetails ) {
		global $wgUser;
		$resultDetails = null;

		# Check permissions
		$errors = array_merge( $this->mTitle->getUserPermissionsErrors( 'edit', $wgUser ),
						$this->mTitle->getUserPermissionsErrors( 'rollback', $wgUser ) );
		if( !$wgUser->matchEditToken( $token, array( $this->mTitle->getPrefixedText(), $fromP ) ) )
			$errors[] = array( 'sessionfailure' );

		if ( $wgUser->pingLimiter('rollback') || $wgUser->pingLimiter() ) {
			$errors[] = array( 'actionthrottledtext' );
		}
		# If there were errors, bail out now
		if(!empty($errors))
			return $errors;

		return $this->commitRollback($fromP, $summary, $bot, $resultDetails);
	}

	/**
	 * Backend implementation of doRollback(), please refer there for parameter
	 * and return value documentation
	 *
	 * NOTE: This function does NOT check ANY permissions, it just commits the
	 * rollback to the DB Therefore, you should only call this function direct-
	 * ly if you want to use custom permissions checks. If you don't, use
	 * doRollback() instead.
	 */
	public function commitRollback($fromP, $summary, $bot, &$resultDetails) {
		global $wgUseRCPatrol, $wgUser, $wgLang;
		$dbw = wfGetDB( DB_MASTER );

		if( wfReadOnly() ) {
			return array( array( 'readonlytext' ) );
		}

		# Get the last editor
		$current = Revision::newFromTitle( $this->mTitle );
		if( is_null( $current ) ) {
			# Something wrong... no page?
			return array(array('notanarticle'));
		}

		$from = str_replace( '_', ' ', $fromP );
		if( $from != $current->getUserText() ) {
			$resultDetails = array( 'current' => $current );
			return array(array('alreadyrolled',
				htmlspecialchars($this->mTitle->getPrefixedText()),
				htmlspecialchars($fromP),
				htmlspecialchars($current->getUserText())
			));
		}

		# Get the last edit not by this guy
		$user = intval( $current->getUser() );
		$user_text = $dbw->addQuotes( $current->getUserText() );
		$s = $dbw->selectRow( 'revision',
			array( 'rev_id', 'rev_timestamp', 'rev_deleted' ),
			array(	'rev_page' => $current->getPage(),
				"rev_user <> {$user} OR rev_user_text <> {$user_text}"
			), __METHOD__,
			array(	'USE INDEX' => 'page_timestamp',
				'ORDER BY'  => 'rev_timestamp DESC' )
			);
		if( $s === false ) {
			# No one else ever edited this page
			return array(array('cantrollback'));
		} else if( $s->rev_deleted & REVISION::DELETED_TEXT || $s->rev_deleted & REVISION::DELETED_USER ) {
			# Only admins can see this text
			return array(array('notvisiblerev'));
		}

		$set = array();
		if ( $bot && $wgUser->isAllowed('markbotedits') ) {
			# Mark all reverted edits as bot
			$set['rc_bot'] = 1;
		}
		if ( $wgUseRCPatrol ) {
			# Mark all reverted edits as patrolled
			$set['rc_patrolled'] = 1;
		}

		if ( $set ) {
			$dbw->update( 'recentchanges', $set,
					array( /* WHERE */
						'rc_cur_id' => $current->getPage(),
						'rc_user_text' => $current->getUserText(),
						"rc_timestamp > '{$s->rev_timestamp}'",
					), __METHOD__
				);
		}

		# Generate the edit summary if necessary
		$target = Revision::newFromId( $s->rev_id );
		if( empty( $summary ) ){
			$summary = wfMsgForContent( 'revertpage' );
		}
		
		# Allow the custom summary to use the same args as the default message
		$args = array(
			$target->getUserText(), $from, $s->rev_id,
			$wgLang->timeanddate(wfTimestamp(TS_MW, $s->rev_timestamp), true),
			$current->getId(), $wgLang->timeanddate($current->getTimestamp())
		);
		$summary = wfMsgReplaceArgs( $summary, $args ); 

		# Save
		$flags = EDIT_UPDATE;

		if ($wgUser->isAllowed('minoredit'))
			$flags |= EDIT_MINOR;

		if( $bot && ($wgUser->isAllowed('markbotedits') || $wgUser->isAllowed('bot')) )
			$flags |= EDIT_FORCE_BOT;
		$this->doEdit( $target->getText(), $summary, $flags, $target->getId() );

		wfRunHooks( 'ArticleRollbackComplete', array( $this, $wgUser, $target ) );

		$resultDetails = array(
			'summary' => $summary,
			'current' => $current,
			'target' => $target,
		);
		return array();
	}

	/**
	 * User interface for rollback operations
	 */
	function rollback() {
		global $wgUser, $wgOut, $wgRequest, $wgUseRCPatrol;
		$details = null;

		$result = $this->doRollback(
			$wgRequest->getVal( 'from' ),
			$wgRequest->getText( 'summary' ),
			$wgRequest->getVal( 'token' ),
			$wgRequest->getBool( 'bot' ),
			$details
		);

		if( in_array( array( 'blocked' ), $result ) ) {
			$wgOut->blockedPage();
			return;
		}
		if( in_array( array( 'actionthrottledtext' ), $result ) ) {
			$wgOut->rateLimited();
			return;
		}
		if( isset( $result[0][0] ) && ( $result[0][0] == 'alreadyrolled' || $result[0][0] == 'cantrollback' ) ){
			$wgOut->setPageTitle( wfMsg( 'rollbackfailed' ) );
			$errArray = $result[0];
			$errMsg = array_shift( $errArray );
			$wgOut->addWikiMsgArray( $errMsg, $errArray );
			if( isset( $details['current'] ) ){
				$current = $details['current'];
				if( $current->getComment() != '' ) {
					$wgOut->addWikiMsgArray( 'editcomment', array( $wgUser->getSkin()->formatComment( $current->getComment() ) ), array( 'replaceafter' ) );
				}
			}
			return;
		}
		# Display permissions errors before read-only message -- there's no
		# point in misleading the user into thinking the inability to rollback
		# is only temporary.
		if( !empty($result) && $result !== array( array('readonlytext') ) ) {
			# array_diff is completely broken for arrays of arrays, sigh.  Re-
			# move any 'readonlytext' error manually.
			$out = array();
			foreach( $result as $error ) {
				if( $error != array( 'readonlytext' ) ) {
					$out []= $error;
				}
			}
			$wgOut->showPermissionsErrorPage( $out );
			return;
		}
		if( $result == array( array('readonlytext') ) ) {
			$wgOut->readOnlyPage();
			return;
		}

		$current = $details['current'];
		$target = $details['target'];
		$wgOut->setPageTitle( wfMsg( 'actioncomplete' ) );
		$wgOut->setRobotPolicy( 'noindex,nofollow' );
		$old = $wgUser->getSkin()->userLink( $current->getUser(), $current->getUserText() )
			. $wgUser->getSkin()->userToolLinks( $current->getUser(), $current->getUserText() );
		$new = $wgUser->getSkin()->userLink( $target->getUser(), $target->getUserText() )
			. $wgUser->getSkin()->userToolLinks( $target->getUser(), $target->getUserText() );
		$wgOut->addHtml( wfMsgExt( 'rollback-success', array( 'parse', 'replaceafter' ), $old, $new ) );
		$wgOut->returnToMain( false, $this->mTitle );
		
		if( !$wgRequest->getBool( 'hidediff', false ) ) {
			$de = new DifferenceEngine( $this->mTitle, $current->getId(), 'next', false, true );
			$de->showDiff( '', '' );
		}
	}


	/**
	 * Do standard deferred updates after page view
	 * @private
	 */
	function viewUpdates() {
		global $wgDeferredUpdateList, $wgUser;

		if ( 0 != $this->getID() ) {
			# Don't update page view counters on views from bot users (bug 14044)
			global $wgDisableCounters;
			if( !$wgDisableCounters && !$wgUser->isAllowed( 'bot' ) ) {
				Article::incViewCount( $this->getID() );
				$u = new SiteStatsUpdate( 1, 0, 0 );
				array_push( $wgDeferredUpdateList, $u );
			}
		}

		# Update newtalk / watchlist notification status
		$wgUser->clearNotification( $this->mTitle );
	}

	/**
	 * Prepare text which is about to be saved.
	 * Returns a stdclass with source, pst and output members
	 */
	function prepareTextForEdit( $text, $revid=null ) {
		if ( $this->mPreparedEdit && $this->mPreparedEdit->newText == $text && $this->mPreparedEdit->revid == $revid) {
			// Already prepared
			return $this->mPreparedEdit;
		}
		global $wgParser;
		$edit = (object)array();
		$edit->revid = $revid;
		$edit->newText = $text;
		$edit->pst = $this->preSaveTransform( $text );
		$options = new ParserOptions;
		$options->setTidy( true );
		$options->enableLimitReport();
		$edit->output = $wgParser->parse( $edit->pst, $this->mTitle, $options, true, true, $revid );
		$edit->oldText = $this->getContent();
		$this->mPreparedEdit = $edit;
		return $edit;
	}

	/**
	 * Do standard deferred updates after page edit.
	 * Update links tables, site stats, search index and message cache.
	 * Every 100th edit, prune the recent changes table.
	 *
	 * @private
	 * @param $text New text of the article
	 * @param $summary Edit summary
	 * @param $minoredit Minor edit
	 * @param $timestamp_of_pagechange Timestamp associated with the page change
	 * @param $newid rev_id value of the new revision
	 * @param $changed Whether or not the content actually changed
	 */
	function editUpdates( $text, $summary, $minoredit, $timestamp_of_pagechange, $newid, $changed = true ) {
		global $wgDeferredUpdateList, $wgMessageCache, $wgUser, $wgParser, $wgEnableParserCache;

		wfProfileIn( __METHOD__ );

		# Parse the text
		# Be careful not to double-PST: $text is usually already PST-ed once
		if ( !$this->mPreparedEdit || $this->mPreparedEdit->output->getFlag( 'vary-revision' ) ) {
			wfDebug( __METHOD__ . ": No prepared edit or vary-revision is set...\n" );
			$editInfo = $this->prepareTextForEdit( $text, $newid );
		} else {
			wfDebug( __METHOD__ . ": No vary-revision, using prepared edit...\n" );
			$editInfo = $this->mPreparedEdit;
		}

		# Save it to the parser cache
		if ( $wgEnableParserCache ) {
			$parserCache = ParserCache::singleton();
			$parserCache->save( $editInfo->output, $this, $wgUser );
		}

		# Update the links tables
		$u = new LinksUpdate( $this->mTitle, $editInfo->output );
		$u->doUpdate();

		if( wfRunHooks( 'ArticleEditUpdatesDeleteFromRecentchanges', array( &$this ) ) ) {
			if ( 0 == mt_rand( 0, 99 ) ) {
				// Flush old entries from the `recentchanges` table; we do this on
				// random requests so as to avoid an increase in writes for no good reason
				global $wgRCMaxAge;
				$dbw = wfGetDB( DB_MASTER );
				$cutoff = $dbw->timestamp( time() - $wgRCMaxAge );
				$recentchanges = $dbw->tableName( 'recentchanges' );
				$sql = "DELETE FROM $recentchanges WHERE rc_timestamp < '{$cutoff}'";
				$dbw->query( $sql );
			}
		}

		$id = $this->getID();
		$title = $this->mTitle->getPrefixedDBkey();
		$shortTitle = $this->mTitle->getDBkey();

		if ( 0 == $id ) {
			wfProfileOut( __METHOD__ );
			return;
		}

		$u = new SiteStatsUpdate( 0, 1, $this->mGoodAdjustment, $this->mTotalAdjustment );
		array_push( $wgDeferredUpdateList, $u );
		$u = new SearchUpdate( $id, $title, $text );
		array_push( $wgDeferredUpdateList, $u );

		# If this is another user's talk page, update newtalk
		# Don't do this if $changed = false otherwise some idiot can null-edit a
		# load of user talk pages and piss people off, nor if it's a minor edit
		# by a properly-flagged bot.
		if( $this->mTitle->getNamespace() == NS_USER_TALK && $shortTitle != $wgUser->getTitleKey() && $changed
			&& !($minoredit && $wgUser->isAllowed('nominornewtalk') ) ) {
			if (wfRunHooks('ArticleEditUpdateNewTalk', array(&$this)) ) {
				$other = User::newFromName( $shortTitle );
				if( is_null( $other ) && User::isIP( $shortTitle ) ) {
					// An anonymous user
					$other = new User();
					$other->setName( $shortTitle );
				}
				if( $other ) {
					$other->setNewtalk( true );
				}
			}
		}

		if ( $this->mTitle->getNamespace() == NS_MEDIAWIKI ) {
			$wgMessageCache->replace( $shortTitle, $text );
		}

		wfProfileOut( __METHOD__ );
	}

	/**
	 * Perform article updates on a special page creation.
	 *
	 * @param Revision $rev
	 *
	 * @todo This is a shitty interface function. Kill it and replace the
	 * other shitty functions like editUpdates and such so it's not needed
	 * anymore.
	 */
	function createUpdates( $rev ) {
		$this->mGoodAdjustment = $this->isCountable( $rev->getText() );
		$this->mTotalAdjustment = 1;
		$this->editUpdates( $rev->getText(), $rev->getComment(),
			$rev->isMinor(), wfTimestamp(), $rev->getId(), true );
	}

	/**
	 * Generate the navigation links when browsing through an article revisions
	 * It shows the information as:
	 *   Revision as of \<date\>; view current revision
	 *   \<- Previous version | Next Version -\>
	 *
	 * @private
	 * @param string $oldid		Revision ID of this article revision
	 */
	function setOldSubtitle( $oldid=0 ) {
		global $wgLang, $wgOut, $wgUser;

		if ( !wfRunHooks( 'DisplayOldSubtitle', array(&$this, &$oldid) ) ) {
				return;
		}

		$revision = Revision::newFromId( $oldid );

		$current = ( $oldid == $this->mLatest );
		$td = $wgLang->timeanddate( $this->mTimestamp, true );
		$sk = $wgUser->getSkin();
		$lnk = $current
			? wfMsg( 'currentrevisionlink' )
			: $sk->makeKnownLinkObj( $this->mTitle, wfMsg( 'currentrevisionlink' ) );
		$curdiff = $current
			? wfMsg( 'diff' )
			: $sk->makeKnownLinkObj( $this->mTitle, wfMsg( 'diff' ), 'diff=cur&oldid='.$oldid );
		$prev = $this->mTitle->getPreviousRevisionID( $oldid ) ;
		$prevlink = $prev
			? $sk->makeKnownLinkObj( $this->mTitle, wfMsg( 'previousrevision' ), 'direction=prev&oldid='.$oldid )
			: wfMsg( 'previousrevision' );
		$prevdiff = $prev
			? $sk->makeKnownLinkObj( $this->mTitle, wfMsg( 'diff' ), 'diff=prev&oldid='.$oldid )
			: wfMsg( 'diff' );
		$nextlink = $current
			? wfMsg( 'nextrevision' )
			: $sk->makeKnownLinkObj( $this->mTitle, wfMsg( 'nextrevision' ), 'direction=next&oldid='.$oldid );
		$nextdiff = $current
			? wfMsg( 'diff' )
			: $sk->makeKnownLinkObj( $this->mTitle, wfMsg( 'diff' ), 'diff=next&oldid='.$oldid );

		$cdel='';
		if( $wgUser->isAllowed( 'deleterevision' ) ) {
			$revdel = SpecialPage::getTitleFor( 'Revisiondelete' );
			if( $revision->isCurrent() ) {
			// We don't handle top deleted edits too well
				$cdel = wfMsgHtml('rev-delundel');
			} else if( !$revision->userCan( Revision::DELETED_RESTRICTED ) ) {
			// If revision was hidden from sysops
				$cdel = wfMsgHtml('rev-delundel');
			} else {
				$cdel = $sk->makeKnownLinkObj( $revdel,
					wfMsgHtml('rev-delundel'),
					'target=' . urlencode( $this->mTitle->getPrefixedDbkey() ) .
					'&oldid=' . urlencode( $oldid ) );
				// Bolden oversighted content
				if( $revision->isDeleted( Revision::DELETED_RESTRICTED ) )
					$cdel = "<strong>$cdel</strong>";
			}
			$cdel = "(<small>$cdel</small>) ";
		}
		# Show user links if allowed to see them. Normally they
		# are hidden regardless, but since we can already see the text here...
		$userlinks = $sk->revUserTools( $revision, false );

		$m = wfMsg( 'revision-info-current' );
		$infomsg = $current && !wfEmptyMsg( 'revision-info-current', $m ) && $m != '-'
			? 'revision-info-current'
			: 'revision-info';

		$r = "\n\t\t\t\t<div id=\"mw-{$infomsg}\">" . wfMsg( $infomsg, $td, $userlinks ) . "</div>\n" .

		     "\n\t\t\t\t<div id=\"mw-revision-nav\">" . $cdel . wfMsg( 'revision-nav', $prevdiff, $prevlink, $lnk, $curdiff, $nextlink, $nextdiff ) . "</div>\n\t\t\t";
		$wgOut->setSubtitle( $r );
	}

	/**
	 * This function is called right before saving the wikitext,
	 * so we can do things like signatures and links-in-context.
	 *
	 * @param string $text
	 */
	function preSaveTransform( $text ) {
		global $wgParser, $wgUser;
		return $wgParser->preSaveTransform( $text, $this->mTitle, $wgUser, ParserOptions::newFromUser( $wgUser ) );
	}

	/* Caching functions */

	/**
	 * checkLastModified returns true if it has taken care of all
	 * output to the client that is necessary for this request.
	 * (that is, it has sent a cached version of the page)
	 */
	function tryFileCache() {
		static $called = false;
		if( $called ) {
			wfDebug( "Article::tryFileCache(): called twice!?\n" );
			return;
		}
		$called = true;
		if($this->isFileCacheable()) {
			$touched = $this->mTouched;
			$cache = new HTMLFileCache( $this->mTitle );
			if($cache->isFileCacheGood( $touched )) {
				wfDebug( "Article::tryFileCache(): about to load file\n" );
				$cache->loadFromFileCache();
				return true;
			} else {
				wfDebug( "Article::tryFileCache(): starting buffer\n" );
				ob_start( array(&$cache, 'saveToFileCache' ) );
			}
		} else {
			wfDebug( "Article::tryFileCache(): not cacheable\n" );
		}
	}

	/**
	 * Check if the page can be cached
	 * @return bool
	 */
	function isFileCacheable() {
		global $wgUser, $wgUseFileCache, $wgShowIPinHeader, $wgRequest, $wgLang, $wgContLang;
		$action    = $wgRequest->getVal( 'action'    );
		$oldid     = $wgRequest->getVal( 'oldid'     );
		$diff      = $wgRequest->getVal( 'diff'      );
		$redirect  = $wgRequest->getVal( 'redirect'  );
		$printable = $wgRequest->getVal( 'printable' );
		$page      = $wgRequest->getVal( 'page' );

		//check for non-standard user language; this covers uselang,
		//and extensions for auto-detecting user language.
		$ulang     = $wgLang->getCode();
		$clang     = $wgContLang->getCode();

		$cacheable = $wgUseFileCache
			&& (!$wgShowIPinHeader)
			&& ($this->getID() != 0)
			&& ($wgUser->isAnon())
			&& (!$wgUser->getNewtalk())
			&& ($this->mTitle->getNamespace() != NS_SPECIAL )
			&& (empty( $action ) || $action == 'view')
			&& (!isset($oldid))
			&& (!isset($diff))
			&& (!isset($redirect))
			&& (!isset($printable))
			&& !isset($page)
			&& (!$this->mRedirectedFrom)
			&& ($ulang === $clang);

		if ( $cacheable ) {
			//extension may have reason to disable file caching on some pages.
			$cacheable = wfRunHooks( 'IsFileCacheable', array( $this ) );
		}

		return $cacheable;
	}

	/**
	 * Loads page_touched and returns a value indicating if it should be used
	 *
	 */
	function checkTouched() {
		if( !$this->mDataLoaded ) {
			$this->loadPageData();
		}
		return !$this->mIsRedirect;
	}

	/**
	 * Get the page_touched field
	 */
	function getTouched() {
		# Ensure that page data has been loaded
		if( !$this->mDataLoaded ) {
			$this->loadPageData();
		}
		return $this->mTouched;
	}

	/**
	 * Get the page_latest field
	 */
	function getLatest() {
		if ( !$this->mDataLoaded ) {
			$this->loadPageData();
		}
		return $this->mLatest;
	}

	/**
	 * Edit an article without doing all that other stuff
	 * The article must already exist; link tables etc
	 * are not updated, caches are not flushed.
	 *
	 * @param string $text text submitted
	 * @param string $comment comment submitted
	 * @param bool $minor whereas it's a minor modification
	 */
	function quickEdit( $text, $comment = '', $minor = 0 ) {
		wfProfileIn( __METHOD__ );

		$dbw = wfGetDB( DB_MASTER );
		$dbw->begin();
		$revision = new Revision( array(
			'page'       => $this->getId(),
			'text'       => $text,
			'comment'    => $comment,
			'minor_edit' => $minor ? 1 : 0,
			) );
		$revision->insertOn( $dbw );
		$this->updateRevisionOn( $dbw, $revision );
		$dbw->commit();
		
		wfRunHooks( 'NewRevisionFromEditComplete', array($this, $revision, false) );

		wfProfileOut( __METHOD__ );
	}

	/**
	 * Used to increment the view counter
	 *
	 * @static
	 * @param integer $id article id
	 */
	function incViewCount( $id ) {
		$id = intval( $id );
		global $wgHitcounterUpdateFreq, $wgDBtype;

		$dbw = wfGetDB( DB_MASTER );
		$pageTable = $dbw->tableName( 'page' );
		$hitcounterTable = $dbw->tableName( 'hitcounter' );
		$acchitsTable = $dbw->tableName( 'acchits' );

		if( $wgHitcounterUpdateFreq <= 1 ) {
			$dbw->query( "UPDATE $pageTable SET page_counter = page_counter + 1 WHERE page_id = $id" );
			return;
		}

		# Not important enough to warrant an error page in case of failure
		$oldignore = $dbw->ignoreErrors( true );

		$dbw->query( "INSERT INTO $hitcounterTable (hc_id) VALUES ({$id})" );

		$checkfreq = intval( $wgHitcounterUpdateFreq/25 + 1 );
		if( (rand() % $checkfreq != 0) or ($dbw->lastErrno() != 0) ){
			# Most of the time (or on SQL errors), skip row count check
			$dbw->ignoreErrors( $oldignore );
			return;
		}

		$res = $dbw->query("SELECT COUNT(*) as n FROM $hitcounterTable");
		$row = $dbw->fetchObject( $res );
		$rown = intval( $row->n );
		if( $rown >= $wgHitcounterUpdateFreq ){
			wfProfileIn( 'Article::incViewCount-collect' );
			$old_user_abort = ignore_user_abort( true );

			if ($wgDBtype == 'mysql')
				$dbw->query("LOCK TABLES $hitcounterTable WRITE");
			$tabletype = $wgDBtype == 'mysql' ? "ENGINE=HEAP " : '';
			$dbw->query("CREATE TEMPORARY TABLE $acchitsTable $tabletype AS ".
				"SELECT hc_id,COUNT(*) AS hc_n FROM $hitcounterTable ".
				'GROUP BY hc_id');
			$dbw->query("DELETE FROM $hitcounterTable");
			if ($wgDBtype == 'mysql') {
				$dbw->query('UNLOCK TABLES');
				$dbw->query("UPDATE $pageTable,$acchitsTable SET page_counter=page_counter + hc_n ".
					'WHERE page_id = hc_id');
			}
			else {
				$dbw->query("UPDATE $pageTable SET page_counter=page_counter + hc_n ".
					"FROM $acchitsTable WHERE page_id = hc_id");
			}
			$dbw->query("DROP TABLE $acchitsTable");

			ignore_user_abort( $old_user_abort );
			wfProfileOut( 'Article::incViewCount-collect' );
		}
		$dbw->ignoreErrors( $oldignore );
	}

	/**#@+
	 * The onArticle*() functions are supposed to be a kind of hooks
	 * which should be called whenever any of the specified actions
	 * are done.
	 *
	 * This is a good place to put code to clear caches, for instance.
	 *
	 * This is called on page move and undelete, as well as edit
	 * @static
	 * @param $title_obj a title object
	 */

	static function onArticleCreate($title) {
		# The talk page isn't in the regular link tables, so we need to update manually:
		if ( $title->isTalkPage() ) {
			$other = $title->getSubjectPage();
		} else {
			$other = $title->getTalkPage();
		}
		$other->invalidateCache();
		$other->purgeSquid();

		$title->touchLinks();
		$title->purgeSquid();
		$title->deleteTitleProtection();
	}

	static function onArticleDelete( $title ) {
		global $wgUseFileCache, $wgMessageCache;

		// Update existence markers on article/talk tabs...
		if( $title->isTalkPage() ) {
			$other = $title->getSubjectPage();
		} else {
			$other = $title->getTalkPage();
		}
		$other->invalidateCache();
		$other->purgeSquid();

		$title->touchLinks();
		$title->purgeSquid();

		# File cache
		if ( $wgUseFileCache ) {
			$cm = new HTMLFileCache( $title );
			@unlink( $cm->fileCacheName() );
		}

		# Messages
		if( $title->getNamespace() == NS_MEDIAWIKI ) {
			$wgMessageCache->replace( $title->getDBkey(), false );
		}
		# Images
		if( $title->getNamespace() == NS_IMAGE ) {
			$update = new HTMLCacheUpdate( $title, 'imagelinks' );
			$update->doUpdate();
		}
		# User talk pages
		if( $title->getNamespace() == NS_USER_TALK ) {
			$user = User::newFromName( $title->getText(), false );
			$user->setNewtalk( false );
		}
	}

	/**
	 * Purge caches on page update etc
	 */
	static function onArticleEdit( $title ) {
		global $wgDeferredUpdateList, $wgUseFileCache;

		// Invalidate caches of articles which include this page
		$wgDeferredUpdateList[] = new HTMLCacheUpdate( $title, 'templatelinks' );

		// Invalidate the caches of all pages which redirect here
		$wgDeferredUpdateList[] = new HTMLCacheUpdate( $title, 'redirect' );

		# Purge squid for this page only
		$title->purgeSquid();

		# Clear file cache
		if ( $wgUseFileCache ) {
			$cm = new HTMLFileCache( $title );
			@unlink( $cm->fileCacheName() );
		}
	}

	/**#@-*/

	/**
	 * Overriden by ImagePage class, only present here to avoid a fatal error
	 * Called for ?action=revert
	 */
	public function revert(){
		global $wgOut;
		$wgOut->showErrorPage( 'nosuchaction', 'nosuchactiontext' );
	}

	/**
	 * Info about this page
	 * Called for ?action=info when $wgAllowPageInfo is on.
	 *
	 * @public
	 */
	function info() {
		global $wgLang, $wgOut, $wgAllowPageInfo, $wgUser;

		if ( !$wgAllowPageInfo ) {
			$wgOut->showErrorPage( 'nosuchaction', 'nosuchactiontext' );
			return;
		}

		$page = $this->mTitle->getSubjectPage();

		$wgOut->setPagetitle( $page->getPrefixedText() );
		$wgOut->setPageTitleActionText( wfMsg( 'info_short' ) );
		$wgOut->setSubtitle( wfMsg( 'infosubtitle' ) );

		if( !$this->mTitle->exists() ) {
			$wgOut->addHtml( '<div class="noarticletext">' );
			if( $this->mTitle->getNamespace() == NS_MEDIAWIKI ) {
				// This doesn't quite make sense; the user is asking for
				// information about the _page_, not the message... -- RC
				$wgOut->addHtml( htmlspecialchars( wfMsgWeirdKey( $this->mTitle->getText() ) ) );
			} else {
				$msg = $wgUser->isLoggedIn()
					? 'noarticletext'
					: 'noarticletextanon';
				$wgOut->addHtml( wfMsgExt( $msg, 'parse' ) );
			}
			$wgOut->addHtml( '</div>' );
		} else {
			$dbr = wfGetDB( DB_SLAVE );
			$wl_clause = array(
				'wl_title'     => $page->getDBkey(),
				'wl_namespace' => $page->getNamespace() );
			$numwatchers = $dbr->selectField(
				'watchlist',
				'COUNT(*)',
				$wl_clause,
				__METHOD__,
				$this->getSelectOptions() );

			$pageInfo = $this->pageCountInfo( $page );
			$talkInfo = $this->pageCountInfo( $page->getTalkPage() );

			$wgOut->addHTML( "<ul><li>" . wfMsg("numwatchers", $wgLang->formatNum( $numwatchers ) ) . '</li>' );
			$wgOut->addHTML( "<li>" . wfMsg('numedits', $wgLang->formatNum( $pageInfo['edits'] ) ) . '</li>');
			if( $talkInfo ) {
				$wgOut->addHTML( '<li>' . wfMsg("numtalkedits", $wgLang->formatNum( $talkInfo['edits'] ) ) . '</li>');
			}
			$wgOut->addHTML( '<li>' . wfMsg("numauthors", $wgLang->formatNum( $pageInfo['authors'] ) ) . '</li>' );
			if( $talkInfo ) {
				$wgOut->addHTML( '<li>' . wfMsg('numtalkauthors', $wgLang->formatNum( $talkInfo['authors'] ) ) . '</li>' );
			}
			$wgOut->addHTML( '</ul>' );

		}
	}

	/**
	 * Return the total number of edits and number of unique editors
	 * on a given page. If page does not exist, returns false.
	 *
	 * @param Title $title
	 * @return array
	 * @private
	 */
	function pageCountInfo( $title ) {
		$id = $title->getArticleId();
		if( $id == 0 ) {
			return false;
		}

		$dbr = wfGetDB( DB_SLAVE );

		$rev_clause = array( 'rev_page' => $id );

		$edits = $dbr->selectField(
			'revision',
			'COUNT(rev_page)',
			$rev_clause,
			__METHOD__,
			$this->getSelectOptions() );

		$authors = $dbr->selectField(
			'revision',
			'COUNT(DISTINCT rev_user_text)',
			$rev_clause,
			__METHOD__,
			$this->getSelectOptions() );

		return array( 'edits' => $edits, 'authors' => $authors );
	}

	/**
	 * Return a list of templates used by this article.
	 * Uses the templatelinks table
	 *
	 * @return array Array of Title objects
	 */
	function getUsedTemplates() {
		$result = array();
		$id = $this->mTitle->getArticleID();
		if( $id == 0 ) {
			return array();
		}

		$dbr = wfGetDB( DB_SLAVE );
		$res = $dbr->select( array( 'templatelinks' ),
			array( 'tl_namespace', 'tl_title' ),
			array( 'tl_from' => $id ),
			'Article:getUsedTemplates' );
		if ( false !== $res ) {
			if ( $dbr->numRows( $res ) ) {
				while ( $row = $dbr->fetchObject( $res ) ) {
					$result[] = Title::makeTitle( $row->tl_namespace, $row->tl_title );
				}
			}
		}
		$dbr->freeResult( $res );
		return $result;
	}

	/**
	 * Returns a list of hidden categories this page is a member of.
	 * Uses the page_props and categorylinks tables.
	 *
	 * @return array Array of Title objects
	 */
	function getHiddenCategories() {
		$result = array();
		$id = $this->mTitle->getArticleID();
		if( $id == 0 ) {
			return array();
		}

		$dbr = wfGetDB( DB_SLAVE );
		$res = $dbr->select( array( 'categorylinks', 'page_props', 'page' ),
			array( 'cl_to' ),
			array( 'cl_from' => $id, 'pp_page=page_id', 'pp_propname' => 'hiddencat',
				'page_namespace' => NS_CATEGORY, 'page_title=cl_to'),
			'Article:getHiddenCategories' );
		if ( false !== $res ) {
			if ( $dbr->numRows( $res ) ) {
				while ( $row = $dbr->fetchObject( $res ) ) {
					$result[] = Title::makeTitle( NS_CATEGORY, $row->cl_to );
				}
			}
		}
		$dbr->freeResult( $res );
		return $result;
	}

	/**
	* Return an applicable autosummary if one exists for the given edit.
	* @param string $oldtext The previous text of the page.
	* @param string $newtext The submitted text of the page.
	* @param bitmask $flags A bitmask of flags submitted for the edit.
	* @return string An appropriate autosummary, or an empty string.
	*/
	public static function getAutosummary( $oldtext, $newtext, $flags ) {
		# Decide what kind of autosummary is needed.

		# Redirect autosummaries
		$rt = Title::newFromRedirect( $newtext );
		if( is_object( $rt ) ) {
			return wfMsgForContent( 'autoredircomment', $rt->getFullText() );
		}

		# New page autosummaries
		if( $flags & EDIT_NEW && strlen( $newtext ) ) {
			# If they're making a new article, give its text, truncated, in the summary.
			global $wgContLang;
			$truncatedtext = $wgContLang->truncate(
				str_replace("\n", ' ', $newtext),
				max( 0, 200 - strlen( wfMsgForContent( 'autosumm-new') ) ),
				'...' );
			return wfMsgForContent( 'autosumm-new', $truncatedtext );
		}

		# Blanking autosummaries
		if( $oldtext != '' && $newtext == '' ) {
			return wfMsgForContent('autosumm-blank');
		} elseif( strlen( $oldtext ) > 10 * strlen( $newtext ) && strlen( $newtext ) < 500) {
			# Removing more than 90% of the article
			global $wgContLang;
			$truncatedtext = $wgContLang->truncate(
				$newtext,
				max( 0, 200 - strlen( wfMsgForContent( 'autosumm-replace' ) ) ),
				'...'
			);
			return wfMsgForContent( 'autosumm-replace', $truncatedtext );
		}

		# If we reach this point, there's no applicable autosummary for our case, so our
		# autosummary is empty.
		return '';
	}

	/**
	 * Add the primary page-view wikitext to the output buffer
	 * Saves the text into the parser cache if possible.
	 * Updates templatelinks if it is out of date.
	 *
	 * @param string  $text
	 * @param bool    $cache
	 */
	public function outputWikiText( $text, $cache = true ) {
		global $wgParser, $wgUser, $wgOut, $wgEnableParserCache;

		$popts = $wgOut->parserOptions();
		$popts->setTidy(true);
		$popts->enableLimitReport();
		$parserOutput = $wgParser->parse( $text, $this->mTitle,
			$popts, true, true, $this->getRevIdFetched() );
		$popts->setTidy(false);
		$popts->enableLimitReport( false );
		if ( $wgEnableParserCache && $cache && $this && $parserOutput->getCacheTime() != -1 ) {
			$parserCache = ParserCache::singleton();
			$parserCache->save( $parserOutput, $this, $wgUser );
		}

		if ( !wfReadOnly() && $this->mTitle->areRestrictionsCascading() ) {
			// templatelinks table may have become out of sync,
			// especially if using variable-based transclusions.
			// For paranoia, check if things have changed and if
			// so apply updates to the database. This will ensure
			// that cascaded protections apply as soon as the changes
			// are visible.

			# Get templates from templatelinks
			$id = $this->mTitle->getArticleID();

			$tlTemplates = array();

			$dbr = wfGetDB( DB_SLAVE );
			$res = $dbr->select( array( 'templatelinks' ),
				array( 'tl_namespace', 'tl_title' ),
				array( 'tl_from' => $id ),
				'Article:getUsedTemplates' );

			global $wgContLang;

			if ( false !== $res ) {
				if ( $dbr->numRows( $res ) ) {
					while ( $row = $dbr->fetchObject( $res ) ) {
						$tlTemplates[] = $wgContLang->getNsText( $row->tl_namespace ) . ':' . $row->tl_title ;
					}
				}
			}

			# Get templates from parser output.
			$poTemplates_allns = $parserOutput->getTemplates();

			$poTemplates = array ();
			foreach ( $poTemplates_allns as $ns_templates ) {
				$poTemplates = array_merge( $poTemplates, $ns_templates );
			}

			# Get the diff
			$templates_diff = array_diff( $poTemplates, $tlTemplates );

			if ( count( $templates_diff ) > 0 ) {
				# Whee, link updates time.
				$u = new LinksUpdate( $this->mTitle, $parserOutput );

				$dbw = wfGetDb( DB_MASTER );
				$dbw->begin();

				$u->doUpdate();

				$dbw->commit();
			}
		}

		$wgOut->addParserOutput( $parserOutput );
	}

	/**
	 * Update all the appropriate counts in the category table, given that
	 * we've added the categories $added and deleted the categories $deleted.
	 *
	 * @param $added array   The names of categories that were added
	 * @param $deleted array The names of categories that were deleted
	 * @return null
	 */
	public function updateCategoryCounts( $added, $deleted ) {
		$ns = $this->mTitle->getNamespace();
		$dbw = wfGetDB( DB_MASTER );

		# First make sure the rows exist.  If one of the "deleted" ones didn't
		# exist, we might legitimately not create it, but it's simpler to just
		# create it and then give it a negative value, since the value is bogus
		# anyway.
		#
		# Sometimes I wish we had INSERT ... ON DUPLICATE KEY UPDATE.
		$insertCats = array_merge( $added, $deleted );
		if( !$insertCats ) {
			# Okay, nothing to do
			return;
		}
		$insertRows = array();
		foreach( $insertCats as $cat ) {
			$insertRows[] = array( 'cat_title' => $cat );
		}
		$dbw->insert( 'category', $insertRows, __METHOD__, 'IGNORE' );

		$addFields    = array( 'cat_pages = cat_pages + 1' );
		$removeFields = array( 'cat_pages = cat_pages - 1' );
		if( $ns == NS_CATEGORY ) {
			$addFields[]    = 'cat_subcats = cat_subcats + 1';
			$removeFields[] = 'cat_subcats = cat_subcats - 1';
		} elseif( $ns == NS_IMAGE ) {
			$addFields[]    = 'cat_files = cat_files + 1';
			$removeFields[] = 'cat_files = cat_files - 1';
		}

		if ( $added ) {
			$dbw->update(
				'category',
				$addFields,
				array( 'cat_title' => $added ),
				__METHOD__
			);
		}
		if ( $deleted ) {
			$dbw->update(
				'category',
				$removeFields,
				array( 'cat_title' => $deleted ),
				__METHOD__
			);
		}
	}
}
