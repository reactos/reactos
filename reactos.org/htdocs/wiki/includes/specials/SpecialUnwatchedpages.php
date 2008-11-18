<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * A special page that displays a list of pages that are not on anyones watchlist.
 * Implements Special:Unwatchedpages
 *
 * @ingroup SpecialPage
 * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>
 * @copyright Copyright © 2005, Ævar Arnfjörð Bjarmason
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */
class UnwatchedpagesPage extends QueryPage {

	function getName() { return 'Unwatchedpages'; }
	function isExpensive() { return true; }
	function isSyndicated() { return false; }

	function getSQL() {
		$dbr = wfGetDB( DB_SLAVE );
		list( $page, $watchlist ) = $dbr->tableNamesN( 'page', 'watchlist' );
		$mwns = NS_MEDIAWIKI;
		return
			"
			SELECT
				'Unwatchedpages' as type,
				page_namespace as namespace,
				page_title as title,
				page_namespace as value
			FROM $page
			LEFT JOIN $watchlist ON wl_namespace = page_namespace AND page_title = wl_title
			WHERE wl_title IS NULL AND page_is_redirect = 0 AND page_namespace<>$mwns
			";
	}

	function sortDescending() { return false; }

	function formatResult( $skin, $result ) {
		global $wgContLang;

		$nt = Title::makeTitle( $result->namespace, $result->title );
		$text = $wgContLang->convert( $nt->getPrefixedText() );

		$plink = $skin->makeKnownLinkObj( $nt, htmlspecialchars( $text ) );
		$wlink = $skin->makeKnownLinkObj( $nt, wfMsgHtml( 'watch' ), 'action=watch' );

		return wfSpecialList( $plink, $wlink );
	}
}

/**
 * constructor
 */
function wfSpecialUnwatchedpages() {
	global $wgUser, $wgOut;

	if ( ! $wgUser->isAllowed( 'unwatchedpages' ) )
		return $wgOut->permissionRequired( 'unwatchedpages' );

	list( $limit, $offset ) = wfCheckLimits();

	$wpp = new UnwatchedpagesPage();

	$wpp->doQuery( $offset, $limit );
}
