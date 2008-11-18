<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * SpecialShortpages extends QueryPage. It is used to return the shortest
 * pages in the database.
 * @ingroup SpecialPage
 */
class ShortPagesPage extends QueryPage {

	function getName() {
		return 'Shortpages';
	}

	/**
	 * This query is indexed as of 1.5
	 */
	function isExpensive() {
		return true;
	}

	function isSyndicated() {
		return false;
	}

	function getSQL() {
		global $wgContentNamespaces;

		$dbr = wfGetDB( DB_SLAVE );
		$page = $dbr->tableName( 'page' );
		$name = $dbr->addQuotes( $this->getName() );

		$forceindex = $dbr->useIndexClause("page_len");

		if ($wgContentNamespaces)
			$nsclause = "page_namespace IN (" . $dbr->makeList($wgContentNamespaces) . ")";
		else
			$nsclause = "page_namespace = " . NS_MAIN;

		return
			"SELECT $name as type,
				page_namespace as namespace,
			        page_title as title,
			        page_len AS value
			FROM $page $forceindex
			WHERE $nsclause AND page_is_redirect=0";
	}

	function preprocessResults( $db, $res ) {
		# There's no point doing a batch check if we aren't caching results;
		# the page must exist for it to have been pulled out of the table
		if( $this->isCached() ) {
			$batch = new LinkBatch();
			while( $row = $db->fetchObject( $res ) )
				$batch->add( $row->namespace, $row->title );
			$batch->execute();
			if( $db->numRows( $res ) > 0 )
				$db->dataSeek( $res, 0 );
		}
	}

	function sortDescending() {
		return false;
	}

	function formatResult( $skin, $result ) {
		global $wgLang, $wgContLang;
		$dm = $wgContLang->getDirMark();

		$title = Title::makeTitleSafe( $result->namespace, $result->title );
		if ( !$title ) {
			return '<!-- Invalid title ' .  htmlspecialchars( "{$result->namespace}:{$result->title}" ). '-->';
		}
		$hlink = $skin->makeKnownLinkObj( $title, wfMsgHtml( 'hist' ), 'action=history' );
		$plink = $this->isCached()
					? $skin->makeLinkObj( $title )
					: $skin->makeKnownLinkObj( $title );
		$size = wfMsgExt( 'nbytes', array( 'parsemag', 'escape' ), $wgLang->formatNum( htmlspecialchars( $result->value ) ) );

		return $title->exists()
				? "({$hlink}) {$dm}{$plink} {$dm}[{$size}]"
				: "<s>({$hlink}) {$dm}{$plink} {$dm}[{$size}]</s>";
	}
}

/**
 * constructor
 */
function wfSpecialShortpages() {
	list( $limit, $offset ) = wfCheckLimits();

	$spp = new ShortPagesPage();

	return $spp->doQuery( $offset, $limit );
}
