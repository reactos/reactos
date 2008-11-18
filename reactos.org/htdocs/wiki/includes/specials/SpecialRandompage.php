<?php

/**
 * Special page to direct the user to a random page
 *
 * @ingroup SpecialPage
 * @author Rob Church <robchur@gmail.com>, Ilmari Karonen
 * @license GNU General Public Licence 2.0 or later
 */
class RandomPage extends SpecialPage {
	private $namespace = NS_MAIN;  // namespace to select pages from

	function __construct( $name = 'Randompage' ){
		parent::__construct( $name );
	}

	public function getNamespace() {
		return $this->namespace;
	}

	public function setNamespace ( $ns ) {
		if( $ns < NS_MAIN ) $ns = NS_MAIN;
		$this->namespace = $ns;
	}

	// select redirects instead of normal pages?
	// Overriden by SpecialRandomredirect
	public function isRedirect(){
		return false;
	}

	public function execute( $par ) {
		global $wgOut, $wgContLang;

		if ($par)
			$this->setNamespace( $wgContLang->getNsIndex( $par ) );

		$title = $this->getRandomTitle();

		if( is_null( $title ) ) {
			$this->setHeaders();
			$wgOut->addWikiMsg( strtolower( $this->mName ) . '-nopages' );
			return;
		}

		$query = $this->isRedirect() ? 'redirect=no' : '';
		$wgOut->redirect( $title->getFullUrl( $query ) );
	}


	/**
	 * Choose a random title.
	 * @return Title object (or null if nothing to choose from)
	 */
	public function getRandomTitle() {
		$randstr = wfRandom();
		$row = $this->selectRandomPageFromDB( $randstr );

		/* If we picked a value that was higher than any in
		 * the DB, wrap around and select the page with the
		 * lowest value instead!  One might think this would
		 * skew the distribution, but in fact it won't cause
		 * any more bias than what the page_random scheme
		 * causes anyway.  Trust me, I'm a mathematician. :)
		 */
		if( !$row )
			$row = $this->selectRandomPageFromDB( "0" );

		if( $row )
			return Title::makeTitleSafe( $this->namespace, $row->page_title );
		else
			return null;
	}

	private function selectRandomPageFromDB( $randstr ) {
		global $wgExtraRandompageSQL;
		$fname = 'RandomPage::selectRandomPageFromDB';

		$dbr = wfGetDB( DB_SLAVE );

		$use_index = $dbr->useIndexClause( 'page_random' );
		$page = $dbr->tableName( 'page' );

		$ns = (int) $this->namespace;
		$redirect = $this->isRedirect() ? 1 : 0;

		$extra = $wgExtraRandompageSQL ? "AND ($wgExtraRandompageSQL)" : "";
		$sql = "SELECT page_title
			FROM $page $use_index
			WHERE page_namespace = $ns
			AND page_is_redirect = $redirect
			AND page_random >= $randstr
			$extra
			ORDER BY page_random";

		$sql = $dbr->limitResult( $sql, 1, 0 );
		$res = $dbr->query( $sql, $fname );
		return $dbr->fetchObject( $res );
	}
}
