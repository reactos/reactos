<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * implements Special:Unusedtemplates
 * @author Rob Church <robchur@gmail.com>
 * @copyright © 2006 Rob Church
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 * @ingroup SpecialPage
 */
class UnusedtemplatesPage extends QueryPage {

	function getName() { return( 'Unusedtemplates' ); }
	function isExpensive() { return true; }
	function isSyndicated() { return false; }
	function sortDescending() { return false; }

	function getSQL() {
		$dbr = wfGetDB( DB_SLAVE );
		list( $page, $templatelinks) = $dbr->tableNamesN( 'page', 'templatelinks' );
		$sql = "SELECT 'Unusedtemplates' AS type, page_title AS title,
			page_namespace AS namespace, 0 AS value
			FROM $page
			LEFT JOIN $templatelinks
			ON page_namespace = tl_namespace AND page_title = tl_title
			WHERE page_namespace = 10 AND tl_from IS NULL
			AND page_is_redirect = 0";
		return $sql;
	}

	function formatResult( $skin, $result ) {
		$title = Title::makeTitle( NS_TEMPLATE, $result->title );
		$pageLink = $skin->makeKnownLinkObj( $title, '', 'redirect=no' );
		$wlhLink = $skin->makeKnownLinkObj(
			SpecialPage::getTitleFor( 'Whatlinkshere' ),
			wfMsgHtml( 'unusedtemplateswlh' ),
			'target=' . $title->getPrefixedUrl() );
		return wfSpecialList( $pageLink, $wlhLink );
	}

	function getPageHeader() {
		return wfMsgExt( 'unusedtemplatestext', array( 'parse' ) );
	}

}

function wfSpecialUnusedtemplates() {
	list( $limit, $offset ) = wfCheckLimits();
	$utp = new UnusedtemplatesPage();
	$utp->doQuery( $offset, $limit );
}
