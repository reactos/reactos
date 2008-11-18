<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * implements Special:Uncategorizedcategories
 * @ingroup SpecialPage
 */
class UncategorizedCategoriesPage extends UncategorizedPagesPage {
	function UncategorizedCategoriesPage() {
		$this->requestedNamespace = NS_CATEGORY;
	}

	function getName() {
		return "Uncategorizedcategories";
	}
}

/**
 * constructor
 */
function wfSpecialUncategorizedcategories() {
	list( $limit, $offset ) = wfCheckLimits();

	$lpp = new UncategorizedCategoriesPage();

	return $lpp->doQuery( $offset, $limit );
}
