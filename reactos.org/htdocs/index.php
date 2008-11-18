<?php

	// RosCMS Config File:
	include("roscms/roscms_config.php");
	
	// Browser Detection:
	$userAgent = strtolower($_SERVER['HTTP_USER_AGENT']);
	if ($is_opera = strchr($userAgent,"opera")) $is_opera = 1;
	if ($is_saf = strchr($userAgent,"applewebkit") || $is_saf = strchr($userAgent,"apple computer, inc.")) $is_saf = 1;
	if ($is_webtv = strchr($userAgent,"webtv")) $is_webtv = 1;
	if ($is_ie = strchr($userAgent,"msie") && (!$is_opera) && (!$is_webtv)) $is_ie = 1;
	if (($is_ie) && $is_ie4 = strchr($userAgent,"msie 4.")) $is_ie4 = 1;
	if ($is_moz = strchr($userAgent,"gecko") && (!$is_saf)) $is_moz = 1;
	if (($is_moz) && $is_mozff = strchr($userAgent,"firefox")) $is_mozff = 1;
	if ($is_kon = strchr($userAgent,"konqueror")) $is_kon = 1;
	if ($is_ns = strchr($userAgent,"compatible") && $is_ns = strchr($userAgent,"mozilla") && (!$is_ie) && (!$is_opera) && (!$is_webtv) && (!$is_saf)) $is_ns = 1;
	
	session_start();
	
	$roscms_page_format = ".html";
	 
	// Language detection
	require_once("roscms/inc/language_detection.php");
	
	if (array_key_exists("page", $_GET)) $rpm_page=htmlspecialchars($_GET["page"]);
	$rpm_lang = check_lang($_REQUEST['lang']);
	if (array_key_exists("forma", $_GET)) $rpm_forma=htmlspecialchars($_GET["forma"]);
	if (array_key_exists("format", $_GET)) $rpm_forma=htmlspecialchars($_GET["format"]);
	if (array_key_exists("skin", $_GET)) $rpm_skin=htmlspecialchars($_GET["skin"]);
	
	
	if ($rpm_lang == '' && isset($_COOKIE['roscms_usrset_lang'])) {
		$rpm_lang = $_COOKIE['roscms_usrset_lang'];
		if (substr($rpm_lang, -1) == '/') {
			$rpm_lang = substr($rpm_lang, strlen($rpm_lang) - 1);
		}
		$rpm_lang = check_lang($rpm_lang);
	}
	
	if ($rpm_lang == '') {
		/* After parameter and cookie processing, we still don't have a valid
			   language. So check whether the HTTP Accept-language header can
			   help us. */
		$accept_language = $_SERVER['HTTP_ACCEPT_LANGUAGE'];
		$best_q = 0;
		while (preg_match('/^\s*([^,]+)((,(.*))|$)/',
						  $accept_language, $matches)) {
			$lang_range = $matches[1];
			$accept_language = $matches[4];
			if (preg_match('/^(([a-zA-Z]+)(-[a-zA-Z]+)?)(;q=([0-1](\.[0-9]{1,3})?))?/',
						   $lang_range, $matches)) {
				$lang = check_lang($matches[1]);
				if ($lang != '') {
					$q = $matches[5];
					if ($q == "") {
						$q = 1;
					}
					else {
						settype($q, 'float');
					}
					if ($best_q < $q) {
						$rpm_lang = $lang;
						$best_q = $q;
					}
				}
			}
		}
	}
	if ($rpm_lang == '') {
		/* If all else fails, use the default language */
		$rpm_lang = check_lang('*');
	}
	
	$roscms_page_lang = $rpm_lang . '/';
	

	$rpm_pic = "";
	if (array_key_exists("pic", $_GET)) $rpm_pic=htmlspecialchars($_GET["pic"]);

	
	switch ($rpm_page) {
		case "home":
		case "index":
		default: // Frontpage
			if ($rpm_pic == "/reactos.gif") {
				require("stats/stats.php"); // stats
				exit;
			}
			else {
				$roscms_page_content = "index";
			}
			break;
		case "cms": // CMS Interface
		case "roscms":
			$roscms_page_content = "roscms/?page=home";
			$roscms_page_format = "";
			$roscms_page_lang = "";
			break;
		case "admin":
			$roscms_page_content = "roscms/?page=admin";
			$roscms_page_format = "";
			$roscms_page_lang = "";
			break;
		case "support":
			$roscms_page_content = "support/";
			$roscms_page_format = "";
			$roscms_page_lang = "";
			break;
		case "development":
			$roscms_page_content = "development";
			break;
		case "community":
			$roscms_page_content = "community";
			break;
		case "info":
			$roscms_page_content = "info";
			break;
		case "download":
			$roscms_page_content = "download";
			break;
	}
	
	if ($rpm_page && $roscms_page_content == "index") {
		$roscms_page_content = $rpm_page;
	}
	
	
	if (isset($_COOKIE['roscms_usrset_lang']) || isset($_REQUEST['lang'])) {
		/* Delete an existing cookie (if any) which uses the full hostname */
		setcookie('roscms_usrset_lang', '', -3600);
		/* Add cookie using just the domain name */
		require_once('roscms/inc/utils.php');
		setcookie('roscms_usrset_lang', $rpm_lang, time() + 5 * 30 * 24 * 3600,
				  '/', cookie_domain());
	}
	
	header("Location: " . $roscms_page_lang . $roscms_page_content . $roscms_page_format);
?>
