<?php


if ($rpm_lang == '' && isset($_COOKIE['roscms_usrset_lang'])) {
	$rpm_lang = $_COOKIE['roscms_usrset_lang'];
	if (substr($rpm_lang, -1) == '/') {
		$rpm_lang = substr($rpm_lang, strlen($rpm_lang) - 1);
	}
	$rpm_lang = Language::validate($rpm_lang);
}

if ($rpm_lang == '') {
	/* After parameter and cookie processing, we still don't have a valid
           language. So check whether the HTTP Accept-language header can
           help us. */
	if (isset($_SERVER['HTTP_ACCEPT_LANGUAGE'])) {
		$accept_language = $_SERVER['HTTP_ACCEPT_LANGUAGE'];
	}
	else {
		$accept_language = "en";
	}
	$best_q = 0;
	while (preg_match('/^\s*([^,]+)((,(.*))|$)/',
	                  $accept_language, $matches)) {
		$lang_range = @$matches[1];
		$accept_language = @$matches[4];
		if (preg_match('/^(([a-zA-Z]+)(-[a-zA-Z]+)?)(;q=([0-1](\.[0-9]{1,3})?))?/',
		               $lang_range, $matches)) {
			$lang = Language::validate($matches[1]);
			if ($lang != '') {
				$q = @$matches[5];
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
	$rpm_lang = Language::validate('*');
}

$roscms_page_lang = $rpm_lang . '/';
$rpm_lang_session = $rpm_lang . '/';

if (isset($_COOKIE['roscms_usrset_lang']) || isset($_REQUEST['lang'])) {
	/* Delete an existing cookie (if any) which uses the full hostname */
	Cookie::write('roscms_usrset_lang', '', -3600);
	/* Add cookie using just the domain name */
  
	Cookie::write('roscms_usrset_lang', $rpm_lang, time() + 5 * 30 * 24 * 3600);
}

	//echo "<h1>".$rpm_lang."</h1>";

	require("lang/en.php"); // preload the english language text
	@require("lang/".$rpm_lang.".php"); // load the and overwrite the language text


?>