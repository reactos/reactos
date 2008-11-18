<?php

/**
 * implements Special:Newpages
 * @ingroup SpecialPage
 */
class SpecialNewpages extends SpecialPage {

	// Stored objects
	protected $opts, $skin;

	// Some internal settings
	protected $showNavigation = false;

	public function __construct(){
		parent::__construct( 'Newpages' );
		$this->includable( true );	
	}

	protected function setup( $par ) {
		global $wgRequest, $wgUser, $wgEnableNewpagesUserFilter;

		// Options
		$opts = new FormOptions();
		$this->opts = $opts; // bind
		$opts->add( 'hideliu', false );
		$opts->add( 'hidepatrolled', false );
		$opts->add( 'hidebots', false );
		$opts->add( 'limit', 50 );
		$opts->add( 'offset', '' );
		$opts->add( 'namespace', '0' );
		$opts->add( 'username', '' );
		$opts->add( 'feed', '' );

		// Set values
		$opts->fetchValuesFromRequest( $wgRequest );
		if ( $par ) $this->parseParams( $par );

		// Validate
		$opts->validateIntBounds( 'limit', 0, 5000 );
		if( !$wgEnableNewpagesUserFilter ) {
			$opts->setValue( 'username', '' );
		}

		// Store some objects
		$this->skin = $wgUser->getSkin();
	}

	protected function parseParams( $par ) {
		global $wgLang;
		$bits = preg_split( '/\s*,\s*/', trim( $par ) );
		foreach ( $bits as $bit ) {
			if ( 'shownav' == $bit )
				$this->showNavigation = true;
			if ( 'hideliu' === $bit )
				$this->opts->setValue( 'hideliu', true );
			if ( 'hidepatrolled' == $bit )
				$this->opts->setValue( 'hidepatrolled', true );
			if ( 'hidebots' == $bit )
				$this->opts->setValue( 'hidebots', true );
			if ( is_numeric( $bit ) )
				$this->opts->setValue( 'limit', intval( $bit ) );

			$m = array();
			if ( preg_match( '/^limit=(\d+)$/', $bit, $m ) )
				$this->opts->setValue( 'limit', intval($m[1]) );
			// PG offsets not just digits!
			if ( preg_match( '/^offset=([^=]+)$/', $bit, $m ) )
				$this->opts->setValue( 'offset',  intval($m[1]) );
			if ( preg_match( '/^namespace=(.*)$/', $bit, $m ) ) {
				$ns = $wgLang->getNsIndex( $m[1] );
				if( $ns !== false ) {
					$this->opts->setValue( 'namespace',  $ns );
				}
			}
		}
	}

	/**
	 * Show a form for filtering namespace and username
	 *
	 * @param string $par
	 * @return string
	 */
	public function execute( $par ) {
		global $wgLang, $wgGroupPermissions, $wgUser, $wgOut;

		$this->setHeaders();
		$this->outputHeader();

		$this->showNavigation = !$this->including(); // Maybe changed in setup
		$this->setup( $par );

		if( !$this->including() ) {
			// Settings
			$this->form();

			$this->setSyndicated();
			$feedType = $this->opts->getValue( 'feed' );
			if( $feedType ) {
				return $this->feed( $feedType );
			}
		}

		$pager = new NewPagesPager( $this, $this->opts );
		$pager->mLimit = $this->opts->getValue( 'limit' );
		$pager->mOffset = $this->opts->getValue( 'offset' );

		if( $pager->getNumRows() ) {
			$navigation = '';
			if ( $this->showNavigation ) $navigation = $pager->getNavigationBar();
			$wgOut->addHTML( $navigation . $pager->getBody() . $navigation );
		} else {
			$wgOut->addWikiMsg( 'specialpage-empty' );
		}
	}

	protected function filterLinks() {
		global $wgGroupPermissions, $wgUser;

		// show/hide links
		$showhide = array( wfMsgHtml( 'show' ), wfMsgHtml( 'hide' ) );

		// Option value -> message mapping
		$filters = array(
			'hideliu' => 'rcshowhideliu',
			'hidepatrolled' => 'rcshowhidepatr',
			'hidebots' => 'rcshowhidebots'
		);

		// Disable some if needed
		if ( $wgGroupPermissions['*']['createpage'] !== true )
			unset($filters['hideliu']);

		if ( !$wgUser->useNPPatrol() )
			unset($filters['hidepatrolled']);

		$links = array();
		$changed = $this->opts->getChangedValues();
		unset($changed['offset']); // Reset offset if query type changes

		$self = $this->getTitle();
		foreach ( $filters as $key => $msg ) {
			$onoff = 1 - $this->opts->getValue($key);
			$link = $this->skin->makeKnownLinkObj( $self, $showhide[$onoff],
				wfArrayToCGI( array( $key => $onoff ), $changed )
			);
			$links[$key] = wfMsgHtml( $msg, $link );
		}

		return implode( ' | ', $links );
	}

	protected function form() {
		global $wgOut, $wgEnableNewpagesUserFilter, $wgScript;

		// Consume values
		$this->opts->consumeValue( 'offset' ); // don't carry offset, DWIW
		$namespace = $this->opts->consumeValue( 'namespace' );
		$username = $this->opts->consumeValue( 'username' );

		// Check username input validity
		$ut = Title::makeTitleSafe( NS_USER, $username );
		$userText = $ut ? $ut->getText() : '';

		// Store query values in hidden fields so that form submission doesn't lose them
		$hidden = array();
		foreach ( $this->opts->getUnconsumedValues() as $key => $value ) {
			$hidden[] = Xml::hidden( $key, $value );
		}
		$hidden = implode( "\n", $hidden );

		$form = Xml::openElement( 'form', array( 'action' => $wgScript ) ) .
			Xml::hidden( 'title', $this->getTitle()->getPrefixedDBkey() ) .
			Xml::fieldset( wfMsg( 'newpages' ) ) .
			Xml::openElement( 'table', array( 'id' => 'mw-newpages-table' ) ) .
			"<tr>
				<td class='mw-label'>" .
					Xml::label( wfMsg( 'namespace' ), 'namespace' ) .
				"</td>
				<td class='mw-input'>" .
					Xml::namespaceSelector( $namespace, 'all' ) .
				"</td>
			</tr>" .
			($wgEnableNewpagesUserFilter ?
			"<tr>
				<td class='mw-label'>" .
					Xml::label( wfMsg( 'newpages-username' ), 'mw-np-username' ) .
				"</td>
				<td class='mw-input'>" .
					Xml::input( 'username', 30, $userText, array( 'id' => 'mw-np-username' ) ) .
				"</td>
			</tr>" : "" ) .
			"<tr> <td></td>
				<td class='mw-submit'>" .
					Xml::submitButton( wfMsg( 'allpagessubmit' ) ) .
				"</td>
			</tr>" .
			"<tr>
				<td></td>
				<td class='mw-input'>" .
					$this->filterLinks() .
				"</td>
			</tr>" .
			Xml::closeElement( 'table' ) .
			Xml::closeElement( 'fieldset' ) .
			$hidden .
			Xml::closeElement( 'form' );

		$wgOut->addHTML( $form );
	}

	protected function setSyndicated() {
		global $wgOut;
		$queryParams = array(
			'namespace' => $this->opts->getValue( 'namespace' ),
			'username' => $this->opts->getValue( 'username' )
		);
		$wgOut->setSyndicated( true );
		$wgOut->setFeedAppendQuery( wfArrayToCGI( $queryParams ) );
	}

	/**
	 * Format a row, providing the timestamp, links to the page/history, size, user links, and a comment
	 *
	 * @param $skin Skin to use
	 * @param $result Result row
	 * @return string
	 */
	public function formatRow( $result ) {
		global $wgLang, $wgContLang, $wgUser;
		$dm = $wgContLang->getDirMark();

		$title = Title::makeTitleSafe( $result->page_namespace, $result->page_title );
		$time = $wgLang->timeAndDate( $result->rc_timestamp, true );
		$plink = $this->skin->makeKnownLinkObj( $title, '', $this->patrollable( $result ) ? 'rcid=' . $result->rc_id : '' );
		$hist = $this->skin->makeKnownLinkObj( $title, wfMsgHtml( 'hist' ), 'action=history' );
		$length = wfMsgExt( 'nbytes', array( 'parsemag', 'escape' ),
			$wgLang->formatNum( $result->length ) );
		$ulink = $this->skin->userLink( $result->rc_user, $result->rc_user_text ) . ' ' .
			$this->skin->userToolLinks( $result->rc_user, $result->rc_user_text );
		$comment = $this->skin->commentBlock( $result->rc_comment );
		$css = $this->patrollable( $result ) ? " class='not-patrolled'" : '';

		return "<li{$css}>{$time} {$dm}{$plink} ({$hist}) {$dm}[{$length}] {$dm}{$ulink} {$comment}</li>\n";
	}

	/**
	 * Should a specific result row provide "patrollable" links?
	 *
	 * @param $result Result row
	 * @return bool
	 */
	protected function patrollable( $result ) {
		global $wgUser;
		return ( $wgUser->useNPPatrol() && !$result->rc_patrolled );
	}

	/**
	 * Output a subscription feed listing recent edits to this page.
	 * @param string $type
	 */
	protected function feed( $type ) {
		global $wgFeed, $wgFeedClasses;

		if ( !$wgFeed ) {
			global $wgOut;
			$wgOut->addWikiMsg( 'feed-unavailable' );
			return;
		}

		if( !isset( $wgFeedClasses[$type] ) ) {
			global $wgOut;
			$wgOut->addWikiMsg( 'feed-invalid' );
			return;
		}

		$feed = new $wgFeedClasses[$type](
			$this->feedTitle(),
			wfMsg( 'tagline' ),
			$this->getTitle()->getFullUrl() );

		$pager = new NewPagesPager( $this, $this->opts );
		$limit = $this->opts->getValue( 'limit' );
		global $wgFeedLimit;
		if( $limit > $wgFeedLimit ) {
			$limit = $wgFeedLimit;
		}
		$pager->mLimit = $limit;

		$feed->outHeader();
		if( $pager->getNumRows() > 0 ) {
			while( $row = $pager->mResult->fetchObject() ) {
				$feed->outItem( $this->feedItem( $row ) );
			}
		}
		$feed->outFooter();
	}

	protected function feedTitle() {
		global $wgContLanguageCode, $wgSitename;
		$page = SpecialPage::getPage( 'Newpages' );
		$desc = $page->getDescription();
		return "$wgSitename - $desc [$wgContLanguageCode]";
	}

	protected function feedItem( $row ) {
		$title = Title::MakeTitle( intval( $row->page_namespace ), $row->page_title );
		if( $title ) {
			$date = $row->rc_timestamp;
			$comments = $title->getTalkPage()->getFullURL();

			return new FeedItem(
				$title->getPrefixedText(),
				$this->feedItemDesc( $row ),
				$title->getFullURL(),
				$date,
				$this->feedItemAuthor( $row ),
				$comments);
		} else {
			return NULL;
		}
	}

	/**
	 * Quickie hack... strip out wikilinks to more legible form from the comment.
	 */
	protected function stripComment( $text ) {
		return preg_replace( '/\[\[([^]]*\|)?([^]]+)\]\]/', '\2', $text );
	}

	protected function feedItemAuthor( $row ) {
		return isset( $row->rc_user_text ) ? $row->rc_user_text : '';
	}

	protected function feedItemDesc( $row ) {
		$revision = Revision::newFromId( $row->rev_id );
		if( $revision ) {
			return '<p>' . htmlspecialchars( $revision->getUserText() ) . ': ' .
				htmlspecialchars( $revision->getComment() ) . 
				"</p>\n<hr />\n<div>" .
				nl2br( htmlspecialchars( $revision->getText() ) ) . "</div>";
		}
		return '';
	}
}

/**
 * @ingroup SpecialPage Pager
 */
class NewPagesPager extends ReverseChronologicalPager {
	// Stored opts
	protected $opts, $mForm;

	private $hideliu, $hidepatrolled, $hidebots, $namespace, $user, $spTitle;

	function __construct( $form, FormOptions $opts ) {
		parent::__construct();
		$this->mForm = $form;
		$this->opts = $opts;
	}

	function getTitle(){
		static $title = null;
		if ( $title === null )
			$title = $this->mForm->getTitle();
		return $title;
	}

	function getQueryInfo() {
		global $wgEnableNewpagesUserFilter, $wgGroupPermissions, $wgUser;
		$conds = array();
		$conds['rc_new'] = 1;

		$namespace = $this->opts->getValue( 'namespace' );
		$namespace = ( $namespace === 'all' ) ? false : intval( $namespace );

		$username = $this->opts->getValue( 'username' );
		$user = Title::makeTitleSafe( NS_USER, $username );

		if( $namespace !== false ) {
			$conds['page_namespace'] = $namespace;
			$rcIndexes = array( 'new_name_timestamp' );
		} else {
			$rcIndexes = array( 'rc_timestamp' );
		}
		$conds[] = 'page_id = rc_cur_id';
		$conds['page_is_redirect'] = 0;
		# $wgEnableNewpagesUserFilter - temp WMF hack
		if( $wgEnableNewpagesUserFilter && $user ) {
			$conds['rc_user_text'] = $user->getText();
			$rcIndexes = 'rc_user_text';
		# If anons cannot make new pages, don't "exclude logged in users"!
		} elseif( $wgGroupPermissions['*']['createpage'] && $this->opts->getValue( 'hideliu' ) ) {
			$conds['rc_user'] = 0;
		}
		# If this user cannot see patrolled edits or they are off, don't do dumb queries!
		if( $this->opts->getValue( 'hidepatrolled' ) && $wgUser->useNPPatrol() ) {
			$conds['rc_patrolled'] = 0;
		}
		if( $this->opts->getValue( 'hidebots' ) ) {
			$conds['rc_bot'] = 0;
		}

		return array(
			'tables' => array( 'recentchanges', 'page' ),
			'fields' => 'page_namespace,page_title, rc_cur_id, rc_user,rc_user_text,rc_comment,
				rc_timestamp,rc_patrolled,rc_id,page_len as length, page_latest as rev_id',
			'conds' => $conds,
			'options' => array( 'USE INDEX' => array('recentchanges' => $rcIndexes) )
		);
	}

	function getIndexField() {
		return 'rc_timestamp';
	}

	function formatRow( $row ) {
		return $this->mForm->formatRow( $row );
	}

	function getStartBody() {
		# Do a batch existence check on pages
		$linkBatch = new LinkBatch();
		while( $row = $this->mResult->fetchObject() ) {
			$linkBatch->add( NS_USER, $row->rc_user_text );
			$linkBatch->add( NS_USER_TALK, $row->rc_user_text );
			$linkBatch->add( $row->page_namespace, $row->page_title );
		}
		$linkBatch->execute();
		return "<ul>";
	}

	function getEndBody() {
		return "</ul>";
	}
}
