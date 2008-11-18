<?php
/**
 * @file
 * @ingroup SpecialPage
 */
 
/**
 * Special page lists templates with a large number of
 * transclusion links, i.e. "most used" templates
 *
 * @ingroup SpecialPage
 * @author Rob Church <robchur@gmail.com>
 */
class SpecialMostlinkedtemplates extends QueryPage {

	/**
	 * Name of the report
	 *
	 * @return string
	 */
	public function getName() {
		return 'Mostlinkedtemplates';
	}

	/**
	 * Is this report expensive, i.e should it be cached?
	 *
	 * @return bool
	 */
	public function isExpensive() {
		return true;
	}

	/**
	 * Is there a feed available?
	 *
	 * @return bool
	 */
	public function isSyndicated() {
		return false;
	}

	/**
	 * Sort the results in descending order?
	 *
	 * @return bool
	 */
	public function sortDescending() {
		return true;
	}

	/**
	 * Generate SQL for the report
	 *
	 * @return string
	 */
	public function getSql() {
		$dbr = wfGetDB( DB_SLAVE );
		$templatelinks = $dbr->tableName( 'templatelinks' );
		$name = $dbr->addQuotes( $this->getName() );
		return "SELECT {$name} AS type,
			" . NS_TEMPLATE . " AS namespace,
			tl_title AS title,
			COUNT(*) AS value
			FROM {$templatelinks}
			WHERE tl_namespace = " . NS_TEMPLATE . "
			GROUP BY tl_title";
	}

	/**
	 * Pre-cache page existence to speed up link generation
	 *
	 * @param Database $dbr Database connection
	 * @param int $res Result pointer
	 */
	public function preprocessResults( $db, $res ) {
		$batch = new LinkBatch();
		while( $row = $db->fetchObject( $res ) ) {
			$batch->add( $row->namespace, $row->title );
		}
		$batch->execute();
		if( $db->numRows( $res ) > 0 )
			$db->dataSeek( $res, 0 );
	}

	/**
	 * Format a result row
	 *
	 * @param Skin $skin Skin to use for UI elements
	 * @param object $result Result row
	 * @return string
	 */
	public function formatResult( $skin, $result ) {
		$title = Title::makeTitleSafe( $result->namespace, $result->title );
		if( $title instanceof Title ) {
			return wfSpecialList(
				$skin->makeLinkObj( $title ),
				$this->makeWlhLink( $title, $skin, $result )
			);
		} else {
			$tsafe = htmlspecialchars( $result->title );
			return "Invalid title in result set; {$tsafe}";
		}
	}

	/**
	 * Make a "what links here" link for a given title
	 *
	 * @param Title $title Title to make the link for
	 * @param Skin $skin Skin to use
	 * @param object $result Result row
	 * @return string
	 */
	private function makeWlhLink( $title, $skin, $result ) {
		global $wgLang;
		$wlh = SpecialPage::getTitleFor( 'Whatlinkshere' );
		$label = wfMsgExt( 'nlinks', array( 'parsemag', 'escape' ),
			$wgLang->formatNum( $result->value ) );
		return $skin->makeKnownLinkObj( $wlh, $label, 'target=' . $title->getPrefixedUrl() );
	}
}

/**
 * Execution function
 *
 * @param mixed $par Parameters passed to the page
 */
function wfSpecialMostlinkedtemplates( $par = false ) {
	list( $limit, $offset ) = wfCheckLimits();
	$mlt = new SpecialMostlinkedtemplates();
	$mlt->doQuery( $offset, $limit );
}
