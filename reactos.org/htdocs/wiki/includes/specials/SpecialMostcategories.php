<?php
/**
 * @file
 * @ingroup SpecialPage
 *
 * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>
 * @copyright Copyright © 2005, Ævar Arnfjörð Bjarmason
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */

/**
 * implements Special:Mostcategories
 * @ingroup SpecialPage
 */
class MostcategoriesPage extends QueryPage {

	function getName() { return 'Mostcategories'; }
	function isExpensive() { return true; }
	function isSyndicated() { return false; }

	function getSQL() {
		$dbr = wfGetDB( DB_SLAVE );
		list( $categorylinks, $page) = $dbr->tableNamesN( 'categorylinks', 'page' );
		return
			"
			SELECT
			 	'Mostcategories' as type,
				page_namespace as namespace,
				page_title as title,
				COUNT(*) as value
			FROM $categorylinks
			LEFT JOIN $page ON cl_from = page_id
			WHERE page_namespace = " . NS_MAIN . "
			GROUP BY page_namespace, page_title
			HAVING COUNT(*) > 1
			";
	}

	function formatResult( $skin, $result ) {
		global $wgLang;
		$title = Title::makeTitleSafe( $result->namespace, $result->title );
		if ( !$title instanceof Title ) { throw new MWException('Invalid title in database'); }
		$count = wfMsgExt( 'ncategories', array( 'parsemag', 'escape' ), $wgLang->formatNum( $result->value ) );
		$link = $skin->makeKnownLinkObj( $title, $title->getText() );
		return wfSpecialList( $link, $count );
	}
}

/**
 * constructor
 */
function wfSpecialMostcategories() {
	list( $limit, $offset ) = wfCheckLimits();

	$wpp = new MostcategoriesPage();

	$wpp->doQuery( $offset, $limit );
}
