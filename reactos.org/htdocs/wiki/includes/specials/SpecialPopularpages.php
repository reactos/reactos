<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * implements Special:Popularpages
 * @ingroup SpecialPage
 */
class PopularPagesPage extends QueryPage {

	function getName() {
		return "Popularpages";
	}

	function isExpensive() {
		# page_counter is not indexed
		return true;
	}
	function isSyndicated() { return false; }

	function getSQL() {
		$dbr = wfGetDB( DB_SLAVE );
		$page = $dbr->tableName( 'page' );

		$query =
			"SELECT 'Popularpages' as type,
			        page_namespace as namespace,
			        page_title as title,
			        page_counter as value
			FROM $page ";
		$where =
			"WHERE page_is_redirect=0 AND page_namespace";

		global $wgContentNamespaces;
		if( empty( $wgContentNamespaces ) ) {
			$where .= '='.NS_MAIN;
		} else if( count( $wgContentNamespaces ) > 1 ) {
			$where .= ' in (' . implode( ', ', $wgContentNamespaces ) . ')';
		} else {
			$where .= '='.$wgContentNamespaces[0];
		}

		return $query . $where;
	}

	function formatResult( $skin, $result ) {
		global $wgLang, $wgContLang;
		$title = Title::makeTitle( $result->namespace, $result->title );
		$link = $skin->makeKnownLinkObj( $title, htmlspecialchars( $wgContLang->convert( $title->getPrefixedText() ) ) );
		$nv = wfMsgExt( 'nviews', array( 'parsemag', 'escape'),
			$wgLang->formatNum( $result->value ) );
		return wfSpecialList($link, $nv);
	}
}

/**
 * Constructor
 */
function wfSpecialPopularpages() {
	list( $limit, $offset ) = wfCheckLimits();

	$ppp = new PopularPagesPage();

	return $ppp->doQuery( $offset, $limit );
}
