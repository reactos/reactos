<?php
/**
 * @file
 * @ingroup SpecialPage
 *
 * @author Rob Church <robchur@gmail.com>
 * @copyright © 2006 Rob Church
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */

/**
 * Special:Listredirects - Lists all the redirects on the wiki.
 * @ingroup SpecialPage
 */
class ListredirectsPage extends QueryPage {

	function getName() { return( 'Listredirects' ); }
	function isExpensive() { return( true ); }
	function isSyndicated() { return( false ); }
	function sortDescending() { return( false ); }

	function getSQL() {
		$dbr = wfGetDB( DB_SLAVE );
		$page = $dbr->tableName( 'page' );
		$sql = "SELECT 'Listredirects' AS type, page_title AS title, page_namespace AS namespace, 0 AS value FROM $page WHERE page_is_redirect = 1";
		return( $sql );
	}

	function formatResult( $skin, $result ) {
		global $wgContLang;

		# Make a link to the redirect itself
		$rd_title = Title::makeTitle( $result->namespace, $result->title );
		$rd_link = $skin->makeLinkObj( $rd_title, '', 'redirect=no' );

		# Find out where the redirect leads
		$revision = Revision::newFromTitle( $rd_title );
		if( $revision ) {
			# Make a link to the destination page
			$target = Title::newFromRedirect( $revision->getText() );
			if( $target ) {
				$arr = $wgContLang->getArrow() . $wgContLang->getDirMark();
				$targetLink = $skin->makeLinkObj( $target );
				return "$rd_link $arr $targetLink";
			} else {
				return "<s>$rd_link</s>";
			}
		} else {
			return "<s>$rd_link</s>";
		}
	}
}

function wfSpecialListredirects() {
	list( $limit, $offset ) = wfCheckLimits();
	$lrp = new ListredirectsPage();
	$lrp->doQuery( $offset, $limit );
}
