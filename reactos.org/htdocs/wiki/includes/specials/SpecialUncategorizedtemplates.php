<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * Special page lists all uncategorised pages in the
 * template namespace
 *
 * @ingroup SpecialPage
 * @author Rob Church <robchur@gmail.com>
 */
class UncategorizedTemplatesPage extends UncategorizedPagesPage {

	var $requestedNamespace = NS_TEMPLATE;

	public function getName() {
		return 'Uncategorizedtemplates';
	}

}

/**
 * Main execution point
 *
 * @param mixed $par Parameter passed to the page
 */
function wfSpecialUncategorizedtemplates() {
	list( $limit, $offset ) = wfCheckLimits();
	$utp = new UncategorizedTemplatesPage();
	$utp->doQuery( $offset, $limit );
}
