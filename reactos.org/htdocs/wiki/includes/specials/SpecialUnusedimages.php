<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * implements Special:Unusedimages
 * @ingroup SpecialPage
 */
class UnusedimagesPage extends ImageQueryPage {

	function isExpensive() { return true; }

	function getName() {
		return 'Unusedimages';
	}

	function sortDescending() {
		return false;
	}
	function isSyndicated() { return false; }

	function getSQL() {
		global $wgCountCategorizedImagesAsUsed;
		$dbr = wfGetDB( DB_SLAVE );

		if ( $wgCountCategorizedImagesAsUsed ) {
			list( $page, $image, $imagelinks, $categorylinks ) = $dbr->tableNamesN( 'page', 'image', 'imagelinks', 'categorylinks' );

			return "SELECT 'Unusedimages' as type, 6 as namespace, img_name as title, img_timestamp as value,
						img_user, img_user_text,  img_description
					FROM ((($page AS I LEFT JOIN $categorylinks AS L ON I.page_id = L.cl_from)
						LEFT JOIN $imagelinks AS P ON I.page_title = P.il_to)
						INNER JOIN $image AS G ON I.page_title = G.img_name)
					WHERE I.page_namespace = ".NS_IMAGE." AND L.cl_from IS NULL AND P.il_to IS NULL";
		} else {
			list( $image, $imagelinks ) = $dbr->tableNamesN( 'image','imagelinks' );

			return "SELECT 'Unusedimages' as type, 6 as namespace, img_name as title, img_timestamp as value,
				img_user, img_user_text,  img_description
				FROM $image LEFT JOIN $imagelinks ON img_name=il_to WHERE il_to IS NULL ";
		}
	}

	function getPageHeader() {
		return wfMsgExt( 'unusedimagestext', array( 'parse') );
	}

}

/**
 * Entry point
 */
function wfSpecialUnusedimages() {
	list( $limit, $offset ) = wfCheckLimits();
	$uip = new UnusedimagesPage();

	return $uip->doQuery( $offset, $limit );
}
