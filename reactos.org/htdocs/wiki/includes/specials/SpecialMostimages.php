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
 * implements Special:Mostimages
 * @ingroup SpecialPage
 */
class MostimagesPage extends ImageQueryPage {

	function getName() { return 'Mostimages'; }
	function isExpensive() { return true; }
	function isSyndicated() { return false; }

	function getSQL() {
		$dbr = wfGetDB( DB_SLAVE );
		$imagelinks = $dbr->tableName( 'imagelinks' );
		return
			"
			SELECT
				'Mostimages' as type,
				" . NS_IMAGE . " as namespace,
				il_to as title,
				COUNT(*) as value
			FROM $imagelinks
			GROUP BY il_to
			HAVING COUNT(*) > 1
			";
	}

	function getCellHtml( $row ) {
		global $wgLang;
		return wfMsgExt( 'nlinks',  array( 'parsemag', 'escape' ),
			$wgLang->formatNum( $row->value ) ) . '<br />';
	}

}

/**
 * Constructor
 */
function wfSpecialMostimages() {
	list( $limit, $offset ) = wfCheckLimits();

	$wpp = new MostimagesPage();

	$wpp->doQuery( $offset, $limit );
}
