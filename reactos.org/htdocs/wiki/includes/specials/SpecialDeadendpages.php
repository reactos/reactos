<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * @ingroup SpecialPage
 */
class DeadendPagesPage extends PageQueryPage {

	function getName( ) {
		return "Deadendpages";
	}

	function getPageHeader() {
		return wfMsgExt( 'deadendpagestext', array( 'parse' ) );
	}

	/**
	 * LEFT JOIN is expensive
	 *
	 * @return true
	 */
	function isExpensive( ) {
		return 1;
	}

	function isSyndicated() { return false; }

	/**
	 * @return false
	 */
	function sortDescending() {
		return false;
	}

	/**
	 * @return string an sqlquery
	 */
	function getSQL() {
		$dbr = wfGetDB( DB_SLAVE );
		list( $page, $pagelinks ) = $dbr->tableNamesN( 'page', 'pagelinks' );
		return "SELECT 'Deadendpages' as type, page_namespace AS namespace, page_title as title, page_title AS value " .
	"FROM $page LEFT JOIN $pagelinks ON page_id = pl_from " .
	"WHERE pl_from IS NULL " .
	"AND page_namespace = 0 " .
	"AND page_is_redirect = 0";
	}
}

/**
 * Constructor
 */
function wfSpecialDeadendpages() {

	list( $limit, $offset ) = wfCheckLimits();

	$depp = new DeadendPagesPage();

	return $depp->doQuery( $offset, $limit );
}
