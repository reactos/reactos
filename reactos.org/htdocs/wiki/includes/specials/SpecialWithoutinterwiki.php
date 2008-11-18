<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * Special page lists pages without language links
 *
 * @ingroup SpecialPage
 * @author Rob Church <robchur@gmail.com>
 */
class WithoutInterwikiPage extends PageQueryPage {
	private $prefix = '';

	function getName() {
		return 'Withoutinterwiki';
	}

	function getPageHeader() {
		global $wgScript, $wgMiserMode;

		# Do not show useless input form if wiki is running in misermode
		if( $wgMiserMode ) {
			return '';
		}

		$prefix = $this->prefix;
		$t = SpecialPage::getTitleFor( $this->getName() );

		return 	Xml::openElement( 'form', array( 'method' => 'get', 'action' => $wgScript ) ) .
			Xml::openElement( 'fieldset' ) .
			Xml::element( 'legend', null, wfMsg( 'withoutinterwiki-legend' ) ) .
			Xml::hidden( 'title', $t->getPrefixedText() ) .
			Xml::inputLabel( wfMsg( 'allpagesprefix' ), 'prefix', 'wiprefix', 20, $prefix ) . ' ' .
			Xml::submitButton( wfMsg( 'withoutinterwiki-submit' ) ) .
			Xml::closeElement( 'fieldset' ) .
			Xml::closeElement( 'form' );
	}

	function sortDescending() {
		return false;
	}

	function isExpensive() {
		return true;
	}

	function isSyndicated() {
		return false;
	}

	function getSQL() {
		$dbr = wfGetDB( DB_SLAVE );
		list( $page, $langlinks ) = $dbr->tableNamesN( 'page', 'langlinks' );
		$prefix = $this->prefix ? "AND page_title LIKE '" . $dbr->escapeLike( $this->prefix ) . "%'" : '';
		return
		  "SELECT 'Withoutinterwiki'  AS type,
		          page_namespace AS namespace,
		          page_title     AS title,
		          page_title     AS value
		     FROM $page
		LEFT JOIN $langlinks
		       ON ll_from = page_id
		    WHERE ll_title IS NULL
		      AND page_namespace=" . NS_MAIN . "
		      AND page_is_redirect = 0
			  {$prefix}";
	}

	function setPrefix( $prefix = '' ) {
		$this->prefix = $prefix;
	}

}

function wfSpecialWithoutinterwiki() {
	global $wgRequest, $wgContLang, $wgCapitalLinks;
	list( $limit, $offset ) = wfCheckLimits();
	if( $wgCapitalLinks ) {
		$prefix = $wgContLang->ucfirst( $wgRequest->getVal( 'prefix' ) );
	} else {
		$prefix = $wgRequest->getVal( 'prefix' );
	}
	$wip = new WithoutInterwikiPage();
	$wip->setPrefix( $prefix );
	$wip->doQuery( $offset, $limit );
}
