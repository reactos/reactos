<?php
/**
 * Special page lists images which haven't been categorised
 *
 * @file
 * @ingroup SpecialPage
 * @author Rob Church <robchur@gmail.com>
 */

/**
 * @ingroup SpecialPage
 */
class UncategorizedImagesPage extends ImageQueryPage {

	function getName() {
		return 'Uncategorizedimages';
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
		list( $page, $categorylinks ) = $dbr->tableNamesN( 'page', 'categorylinks' );
		$ns = NS_IMAGE;

		return "SELECT 'Uncategorizedimages' AS type, page_namespace AS namespace,
				page_title AS title, page_title AS value
				FROM {$page} LEFT JOIN {$categorylinks} ON page_id = cl_from
				WHERE cl_from IS NULL AND page_namespace = {$ns} AND page_is_redirect = 0";
	}

}

function wfSpecialUncategorizedimages() {
	$uip = new UncategorizedImagesPage();
	list( $limit, $offset ) = wfCheckLimits();
	return $uip->doQuery( $offset, $limit );
}
