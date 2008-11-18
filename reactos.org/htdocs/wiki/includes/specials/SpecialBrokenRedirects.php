<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * A special page listing redirects to non existent page. Those should be
 * fixed to point to an existing page.
 * @ingroup SpecialPage
 */
class BrokenRedirectsPage extends PageQueryPage {
	var $targets = array();

	function getName() {
		return 'BrokenRedirects';
	}

	function isExpensive( ) { return true; }
	function isSyndicated() { return false; }

	function getPageHeader( ) {
		return wfMsgExt( 'brokenredirectstext', array( 'parse' ) );
	}

	function getSQL() {
		$dbr = wfGetDB( DB_SLAVE );
		list( $page, $redirect ) = $dbr->tableNamesN( 'page', 'redirect' );

		$sql = "SELECT 'BrokenRedirects'  AS type,
		                p1.page_namespace AS namespace,
		                p1.page_title     AS title,
		                rd_namespace,
		                rd_title
		           FROM $redirect AS rd
                   JOIN $page p1 ON (rd.rd_from=p1.page_id)
		      LEFT JOIN $page AS p2 ON (rd_namespace=p2.page_namespace AND rd_title=p2.page_title )
			  	  WHERE rd_namespace >= 0
				    AND p2.page_namespace IS NULL";
		return $sql;
	}

	function getOrder() {
		return '';
	}

	function formatResult( $skin, $result ) {
		global $wgUser, $wgContLang;

		$fromObj = Title::makeTitle( $result->namespace, $result->title );
		if ( isset( $result->rd_title ) ) {
			$toObj = Title::makeTitle( $result->rd_namespace, $result->rd_title );
		} else {
			$blinks = $fromObj->getBrokenLinksFrom(); # TODO: check for redirect, not for links
			if ( $blinks ) {
				$toObj = $blinks[0];
			} else {
				$toObj = false;
			}
		}

		// $toObj may very easily be false if the $result list is cached
		if ( !is_object( $toObj ) ) {
			return '<s>' . $skin->makeLinkObj( $fromObj ) . '</s>';
		}

		$from = $skin->makeKnownLinkObj( $fromObj ,'', 'redirect=no' );
		$edit = $skin->makeKnownLinkObj( $fromObj, wfMsgHtml( 'brokenredirects-edit' ), 'action=edit' );
		$to   = $skin->makeBrokenLinkObj( $toObj );
		$arr = $wgContLang->getArrow();

		$out = "{$from} {$edit}";

		if( $wgUser->isAllowed( 'delete' ) ) {
			$delete = $skin->makeKnownLinkObj( $fromObj, wfMsgHtml( 'brokenredirects-delete' ), 'action=delete' );
			$out .= " {$delete}";
		}

		$out .= " {$arr} {$to}";
		return $out;
	}
}

/**
 * constructor
 */
function wfSpecialBrokenRedirects() {
	list( $limit, $offset ) = wfCheckLimits();

	$sbr = new BrokenRedirectsPage();

	return $sbr->doQuery( $offset, $limit );
}
