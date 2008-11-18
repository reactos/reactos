<?php
/**
 * @file
 * @ingroup SpecialPage
 */

require_once(ROSCMS_PATH . "logon/subsys_login.php");

/**
 * constructor
 */
function wfSpecialUserlogin( $par = '' ) {
	/* Login to RosCMS */
	$target = "/wiki";
	roscms_subsys_login('wiki', ROSCMS_LOGIN_REQUIRED, $target);
	
	/* Just redirect us to the main page in case we were called but already logged in */
	header("Location: $target");
	exit;
}
