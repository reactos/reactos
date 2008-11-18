<?php

  require_once('custom.php');

	// Server Timezone
	$rdf_server_timezone = -2; // for UTC

	// User Account
	$rdf_user_timezone = 0;
	$rdf_user_timezone_name = "UTC";

	// System Branding
	$rdf_name = "ReactOS.org";
	$rdf_name_long = "ReactOS Website";
	
	// E-Mail Addresses
	$rdf_system_brand = "ReactOS";
	$rdf_system_email = "noreply@reactos.org";
	$rdf_system_email_str = "ReactOS<noreply@reactos.org>";
	$rdf_support_email = "support@reactos.org";
	$rdf_support_email_str = "support at reactos.org";
	
	// Logon System
	$rdf_logon_system_name = "myReactOS";

	// Logon System Cookie Names
	$rdf_login_cookie_usrkey = "roscmsusrkey"; // user session key
	$rdf_login_cookie_usrname = "roscmsusrname"; // user login name
	$rdf_login_cookie_usrpwd = "roscmsusrpwd"; // user login password
	$rdf_login_cookie_loginname = "roscmslogon"; // login name ??
	$rdf_login_cookie_seckey = "roscmsseckey"; // secure login setting
	$rdf_login_cookie_lang = "roscms_usrset_lang"; // interface language
	
	// Logon System Register Limits
	$rdf_register_user_name_min = 4; // user-name minimum chars
	$rdf_register_user_name_max = 20; // user-name maximum chars
	$rdf_register_user_pwd_min = 5; // user-password minimum chars
	$rdf_register_user_pwd_max = 50; // user-password maximum chars
	$rdf_register_valid_email_regex = "/^[\\w\\.\\+\\-=]+@[\\w\\.-]+\\.[\\w\\-]+$/"; // check out: http://www.regular-expressions.info/email.html

?>
