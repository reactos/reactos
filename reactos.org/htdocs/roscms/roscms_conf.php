<?php

	$roscms_intern_version="RosCMS - http://www.reactos.org/"; // RosCMS info
	$roscms_intern_path="/"; // the dirs after http://www.reactos.org
	$roscms_intern_path_html=""; // subfolder
	$roscms_intern_path_xhtml="xhtml/"; // subfolder
	$roscms_intern_path_gererator="roscms/"; // subfolder
	$roscms_intern_path_server="http://www.reactos.org/"; // complete server path
	
	$roscms_intern_fileformat_html="html"; // htm
	$roscms_intern_w3cformat_html="html"; // html
	$roscms_intern_fileformat_xhtml="html"; // htm
	$roscms_intern_w3cformat_xhtml="xhtml"; // html

	$roscms_standard_output_format="both"; // html/xhtml/both	

	$roscms_standard_language="en"; // en/de/fr/...
	$roscms_standard_language_full="English"; // en/de/fr/...

	$roscms_branch = "website";



	// System Paths
	$roscms_SET_dirname = "reactos.org/roscms";
	$roscms_SET_path = "http://www.".$roscms_SET_dirname."/";
	$roscms_SET_path_ex = $roscms_SET_path."index.php/";

	// URI
	$rdf_URI_tree_counter = -3;



	// Server Timezone
	$rdf_server_timezone = -1; // for CET
	
	// Database Name
	$rdf_dbname = "roscms";

	// System Branding
	$rdf_ident = "reactos";
	$rdf_name = "ReactOS.org";
	$rdf_name_long = "ReactOS Website";
	
	// System Colors
	$rdf_color_logon_background = "#e1eafb";
	
	// E-Mail Addresses
	$rdf_system_brand = "ReactOS";
	$rdf_system_email = "noreply@reactos.org";
	$rdf_system_email_str = "ReactOS<noreply@reactos.org>";
	$rdf_support_email = "support@reactos.org";
	$rdf_support_email_str = "support at reactos.org";
	
	// Logon System
	$rdf_logon_system_name = "myReactOS";
	$rdf_uri_service_agreement = $roscms_intern_path_server."?page=service_agreement";

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
	
	
	// User Account
	$rdf_user_id = 0;
	$rdf_user_timezone = 1;
	$rdf_user_timezone_name = "UTC";

?>