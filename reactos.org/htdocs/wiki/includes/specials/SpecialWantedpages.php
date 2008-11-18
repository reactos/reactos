<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * implements Special:Wantedpages
 * @ingroup SpecialPage
 */
class WantedPagesPage extends QueryPage {
	var $nlinks;

	function WantedPagesPage( $inc = false, $nlinks = true ) {
		$this->setListoutput( $inc );
		$this->nlinks = $nlinks;
	}

	function getName() {
		return 'Wantedpages';
	}

	function isExpensive() {
		return true;
	}
	function isSyndicated() { return false; }

	function getSQL() {
		global $wgWantedPagesThreshold;
		$count = $wgWantedPagesThreshold - 1;
		$dbr = wfGetDB( DB_SLAVE );
		$pagelinks = $dbr->tableName( 'pagelinks' );
		$page      = $dbr->tableName( 'page' );
		return
			"SELECT 'Wantedpages' AS type,
			        pl_namespace AS namespace,
			        pl_title AS title,
			        COUNT(*) AS value
			 FROM $pagelinks
			 LEFT JOIN $page AS pg1
			 ON pl_namespace = pg1.page_namespace AND pl_title = pg1.page_title
			 LEFT JOIN $page AS pg2
			 ON pl_from = pg2.page_id
			 WHERE pg1.page_namespace IS NULL
			 AND pl_namespace NOT IN ( 2, 3 )
			 AND pg2.page_namespace != 8
			 GROUP BY pl_namespace, pl_title
			 HAVING COUNT(*) > $count";
	}

	/**
	 * Cache page existence for performance
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

	/**
	 * Format an individual result
	 *
	 * @param $skin Skin to use for UI elements
	 * @param $result Result row
	 * @return string
	 */
	public function formatResult( $skin, $result ) {
		$title = Title::makeTitleSafe( $result->namespace, $result->title );
		if( $title instanceof Title ) {
			if( $this->isCached() ) {
				$pageLink = $title->exists()
					? '<s>' . $skin->makeLinkObj( $title ) . '</s>'
					: $skin->makeBrokenLinkObj( $title );
			} else {
				$pageLink = $skin->makeBrokenLinkObj( $title );
			}
			return wfSpecialList( $pageLink, $this->makeWlhLink( $title, $skin, $result ) );
		} else {
			$tsafe = htmlspecialchars( $result->title );
			return "Invalid title in result set; {$tsafe}";
		}
	}

	/**
	 * Make a "what links here" link for a specified result if required
	 *
	 * @param $title Title to make the link for
	 * @param $skin Skin to use
	 * @param $result Result row
	 * @return string
	 */
	private function makeWlhLink( $title, $skin, $result ) {
		global $wgLang;
		if( $this->nlinks ) {
			$wlh = SpecialPage::getTitleFor( 'Whatlinkshere' );
			$label = wfMsgExt( 'nlinks', array( 'parsemag', 'escape' ),
				$wgLang->formatNum( $result->value ) );
			return $skin->makeKnownLinkObj( $wlh, $label, 'target=' . $title->getPrefixedUrl() );
		} else {
			return null;
		}
	}

}

/**
 * constructor
 */
function wfSpecialWantedpages( $par = null, $specialPage ) {
	$inc = $specialPage->including();

	if ( $inc ) {
		@list( $limit, $nlinks ) = explode( '/', $par, 2 );
		$limit = (int)$limit;
		$nlinks = $nlinks === 'nlinks';
		$offset = 0;
	} else {
		list( $limit, $offset ) = wfCheckLimits();
		$nlinks = true;
	}

	$wpp = new WantedPagesPage( $inc, $nlinks );

	$wpp->doQuery( $offset, $limit, !$inc );
}
