<?php
/**
 * A special page to show pages in the
 *
 * @ingroup SpecialPage
 *
 * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>
 * @copyright Copyright © 2005, Ævar Arnfjörð Bjarmason
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */

/**
 * @ingroup SpecialPage
 */
class MostrevisionsPage extends QueryPage {

	function getName() { return 'Mostrevisions'; }
	function isExpensive() { return true; }
	function isSyndicated() { return false; }

	function getSQL() {
		$dbr = wfGetDB( DB_SLAVE );
		list( $revision, $page ) = $dbr->tableNamesN( 'revision', 'page' );
		return
			"
			SELECT
				'Mostrevisions' as type,
				page_namespace as namespace,
				page_title as title,
				COUNT(*) as value
			FROM $revision
			JOIN $page ON page_id = rev_page
			WHERE page_namespace = " . NS_MAIN . "
			GROUP BY page_namespace, page_title
			HAVING COUNT(*) > 1
			";
	}

	function formatResult( $skin, $result ) {
		global $wgLang, $wgContLang;

		$nt = Title::makeTitle( $result->namespace, $result->title );
		$text = $wgContLang->convert( $nt->getPrefixedText() );

		$plink = $skin->makeKnownLinkObj( $nt, $text );

		$nl = wfMsgExt( 'nrevisions', array( 'parsemag', 'escape'),
			$wgLang->formatNum( $result->value ) );
		$nlink = $skin->makeKnownLinkObj( $nt, $nl, 'action=history' );

		return wfSpecialList($plink, $nlink);
	}
}

/**
 * constructor
 */
function wfSpecialMostrevisions() {
	list( $limit, $offset ) = wfCheckLimits();

	$wpp = new MostrevisionsPage();

	$wpp->doQuery( $offset, $limit );
}
