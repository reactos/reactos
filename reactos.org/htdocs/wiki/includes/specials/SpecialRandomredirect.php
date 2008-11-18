<?php

/**
 * Special page to direct the user to a random redirect page (minus the second redirect)
 *
 * @ingroup SpecialPage
 * @author Rob Church <robchur@gmail.com>, Ilmari Karonen
 * @license GNU General Public Licence 2.0 or later
 */
class SpecialRandomredirect extends RandomPage {
	function __construct(){
		parent::__construct( 'Randomredirect' );
	}

	// Override parent::isRedirect()
	public function isRedirect(){
		return true;
	}
}
