<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * A querypage to show categories ordered in descending order by the pages  in them
 *
 * @ingroup SpecialPage
 *
 * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>
 * @copyright Copyright © 2005, Ævar Arnfjörð Bjarmason
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */
class MostlinkedCategoriesPage extends QueryPage {

	function getName() { return 'Mostlinkedcategories'; }
	function isExpensive() { return true; }
	function isSyndicated() { return false; }

	function getSQL() {
		$dbr = wfGetDB( DB_SLAVE );
		$categorylinks = $dbr->tableName( 'categorylinks' );
		$name = $dbr->addQuotes( $this->getName() );
		return
			"
			SELECT
				$name as type,
				" . NS_CATEGORY . " as namespace,
				cl_to as title,
				COUNT(*) as value
			FROM $categorylinks
			GROUP BY cl_to
			";
	}

	function sortDescending() { return true; }

	/**
	 * Fetch user page links and cache their existence
	 */
	function preprocessResults( $db, $res ) {
		$batch = new LinkBatch;
		while ( $row = $db->fetchObject( $res ) )
			$batch->add( $row->namespace, $row->title );
		$batch->execute();

		// Back to start for display
		if ( $db->numRows( $res ) > 0 )
			// If there are no rows we get an error seeking.
			$db->dataSeek( $res, 0 );
	}

	function formatResult( $skin, $result ) {
		global $wgLang, $wgContLang;

		$nt = Title::makeTitle( $result->namespace, $result->title );
		$text = $wgContLang->convert( $nt->getText() );

		$plink = $skin->makeLinkObj( $nt, htmlspecialchars( $text ) );

		$nlinks = wfMsgExt( 'nmembers', array( 'parsemag', 'escape'),
			$wgLang->formatNum( $result->value ) );
		return wfSpecialList($plink, $nlinks);
	}
}

/**
 * constructor
 */
function wfSpecialMostlinkedCategories() {
	list( $limit, $offset ) = wfCheckLimits();

	$wpp = new MostlinkedCategoriesPage();

	$wpp->doQuery( $offset, $limit );
}
