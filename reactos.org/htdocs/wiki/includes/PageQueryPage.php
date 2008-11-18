<?php

/**
 * Variant of QueryPage which formats the result as a simple link to the page
 *
 * @ingroup SpecialPage
 */
class PageQueryPage extends QueryPage {

	/**
	 * Format the result as a simple link to the page
	 *
	 * @param $skin Skin
	 * @param $row Object: result row
	 * @return string
	 */
	public function formatResult( $skin, $row ) {
		global $wgContLang;
		$title = Title::makeTitleSafe( $row->namespace, $row->title );
		return $skin->makeKnownLinkObj( $title,
			htmlspecialchars( $wgContLang->convert( $title->getPrefixedText() ) ) );
	}
}
