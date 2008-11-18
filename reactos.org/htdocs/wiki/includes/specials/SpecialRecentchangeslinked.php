<?php

/**
 * This is to display changes made to all articles linked in an article.
 * @ingroup SpecialPage
 */
class SpecialRecentchangeslinked extends SpecialRecentchanges {

	function __construct(){
		SpecialPage::SpecialPage( 'Recentchangeslinked' );	
	}

	public function getDefaultOptions() {
		$opts = parent::getDefaultOptions();
		$opts->add( 'target', '' );
		$opts->add( 'showlinkedto', false );
		return $opts;
	}

	public function parseParameters( $par, FormOptions $opts ) {
		$opts['target'] = $par;
	}

	public function feedSetup(){
		global $wgRequest;
		$opts = parent::feedSetup();
		$opts['target'] = $wgRequest->getVal( 'target' );
		return $opts;
	}

	public function getFeedObject( $feedFormat ){
		$feed = new ChangesFeed( $feedFormat, false );
		$feedObj = $feed->getFeedObject(
			wfMsgForContent( 'recentchangeslinked-title', $this->mTargetTitle->getPrefixedText() ),
			wfMsgForContent( 'recentchangeslinked' )
		);
		return array( $feed, $feedObj );
	}

	public function doMainQuery( $conds, $opts ) {
		global $wgUser, $wgOut;

		$target = $opts['target'];
		$showlinkedto = $opts['showlinkedto'];
		$limit = $opts['limit'];

		if ( $target === '' ) {
			return false;
		}
		$title = Title::newFromURL( $target );
		if( !$title || $title->getInterwiki() != '' ){
			$wgOut->wrapWikiMsg( '<div class="errorbox">$1</div><br clear="both" />', 'allpagesbadtitle' );
			return false;
		}
		$this->mTargetTitle = $title;

		$wgOut->setPageTitle( wfMsg( 'recentchangeslinked-title', $title->getPrefixedText() ) );

		/*
		 * Ordinary links are in the pagelinks table, while transclusions are
		 * in the templatelinks table, categorizations in categorylinks and
		 * image use in imagelinks.  We need to somehow combine all these.
		 * Special:Whatlinkshere does this by firing multiple queries and
		 * merging the results, but the code we inherit from our parent class
		 * expects only one result set so we use UNION instead.
		 */

		$dbr = wfGetDB( DB_SLAVE, 'recentchangeslinked' );
		$id = $title->getArticleId();
		$ns = $title->getNamespace();
		$dbkey = $title->getDBkey();

		$tables = array( 'recentchanges' );
		$select = array( $dbr->tableName( 'recentchanges' ) . '.*' );
		$join_conds = array();

		// left join with watchlist table to highlight watched rows
		if( $uid = $wgUser->getId() ) {
			$tables[] = 'watchlist';
			$select[] = 'wl_user';
			$join_conds['watchlist'] = array( 'LEFT JOIN', "wl_user={$uid} AND wl_title=rc_title AND wl_namespace=rc_namespace" );
		}

		// XXX: parent class does this, should we too?
		// wfRunHooks('SpecialRecentChangesQuery', array( &$conds, &$tables, &$join_conds, $opts ) );

		if( $ns == NS_CATEGORY && !$showlinkedto ) {
			// special handling for categories
			// XXX: should try to make this less klugy
			$link_tables = array( 'categorylinks' );
			$showlinkedto = true;
		} else {
			// for now, always join on these tables; really should be configurable as in whatlinkshere
			$link_tables = array( 'pagelinks', 'templatelinks' );
			// imagelinks only contains links to pages in NS_IMAGE
			if( $ns == NS_IMAGE || !$showlinkedto ) $link_tables[] = 'imagelinks';
		}

		// field name prefixes for all the various tables we might want to join with
		$prefix = array( 'pagelinks' => 'pl', 'templatelinks' => 'tl', 'categorylinks' => 'cl', 'imagelinks' => 'il' );

		$subsql = array(); // SELECT statements to combine with UNION

		foreach( $link_tables as $link_table ) {
			$pfx = $prefix[$link_table];

			// imagelinks and categorylinks tables have no xx_namespace field, and have xx_to instead of xx_title
			if( $link_table == 'imagelinks' ) $link_ns = NS_IMAGE;
			else if( $link_table == 'categorylinks' ) $link_ns = NS_CATEGORY;
			else $link_ns = 0;

			if( $showlinkedto ) {
				// find changes to pages linking to this page
				if( $link_ns ) {
					if( $ns != $link_ns ) continue; // should never happen, but check anyway
					$subconds = array( "{$pfx}_to" => $dbkey );
				} else {
					$subconds = array( "{$pfx}_namespace" => $ns, "{$pfx}_title" => $dbkey );
				}
				$subjoin = "rc_cur_id = {$pfx}_from";
			} else {
				// find changes to pages linked from this page
				$subconds = array( "{$pfx}_from" => $id );
				if( $link_table == 'imagelinks' || $link_table == 'categorylinks' ) {
					$subconds["rc_namespace"] = $link_ns;
					$subjoin = "rc_title = {$pfx}_to";
				} else {
					$subjoin = "rc_namespace = {$pfx}_namespace AND rc_title = {$pfx}_title";
				}
			}

			$subsql[] = $dbr->selectSQLText( array_merge( $tables, array( $link_table ) ), $select, $conds + $subconds,
							 __METHOD__, array( 'ORDER BY' => 'rc_timestamp DESC', 'LIMIT' => $limit ),
							 $join_conds + array( $link_table => array( 'INNER JOIN', $subjoin ) ) );
		}

		if( count($subsql) == 0 )
			return false; // should never happen
		if( count($subsql) == 1 )
			$sql = $subsql[0];
		else {
			// need to resort and relimit after union
			$sql = "(" . implode( ") UNION (", $subsql ) . ") ORDER BY rc_timestamp DESC LIMIT {$limit}";
		}

		$res = $dbr->query( $sql, __METHOD__ );

		if( $dbr->numRows( $res ) == 0 )
			$this->mResultEmpty = true;

		return $res;
	}
	
	function getExtraOptions( $opts ){
		$opts->consumeValues( array( 'showlinkedto', 'target' ) );
		$extraOpts = array();
		$extraOpts['namespace'] = $this->namespaceFilterForm( $opts );
		$extraOpts['target'] = array( wfMsg( 'recentchangeslinked-page' ),
			Xml::input( 'target', 40, str_replace('_',' ',$opts['target']) ) .
			Xml::check( 'showlinkedto', $opts['showlinkedto'], array('id' => 'showlinkedto') ) . ' ' .
			Xml::label( wfMsg("recentchangeslinked-to"), 'showlinkedto' ) );
		$extraOpts['submit'] = Xml::submitbutton( wfMsg('allpagessubmit') );
		return $extraOpts;
	}
	
	function setTopText( &$out, $opts ){}
	
	function setBottomText( &$out, $opts ){
		if( isset( $this->mTargetTitle ) && is_object( $this->mTargetTitle ) ){
			global $wgUser;
			$out->setFeedAppendQuery( "target=" . urlencode( $this->mTargetTitle->getPrefixedDBkey() ) );
			$out->addHTML("&lt; ".$wgUser->getSkin()->makeLinkObj( $this->mTargetTitle, "", "redirect=no" )."<hr />\n");
		}
		if( isset( $this->mResultEmpty ) && $this->mResultEmpty ){
			$out->addWikiMsg( 'recentchangeslinked-noresult' );	
		}
	}
}
