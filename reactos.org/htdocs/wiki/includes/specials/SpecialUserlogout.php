<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * constructor
 */
function wfSpecialUserlogout() {
	global $wgUser;

	$wgUser->logout();
	header("Location: ../roscms/?page=logout");
	exit;
}
