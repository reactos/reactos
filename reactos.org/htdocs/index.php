<?php

// RosCMS Config File:
define('ROSCMS_PATH','roscms/');
require_once(ROSCMS_PATH.'/config.php');
require_once(ROSCMS_PATH.'/lib/RosCMS_Autoloader.class.php');

// Language detection
$page_lang = null;
if (RosCMS::getInstance()->multiLanguage()) {

  // check for aquired language
  if (isset($_REQUEST['lang'])) {
    $page_lang = Language::validate($_REQUEST['lang']);
  }

  // check for saved language
  elseif (isset($_COOKIE[RosCMS::getInstance()->cookieLanguage()])) {
    $page_lang = Language::validate($_COOKIE[RosCMS::getInstance()->cookieLanguage()]);
  }

  // check for client language
  elseif (isset($_SERVER['HTTP_ACCEPT_LANGUAGE'])) {
    $page_lang = Language::validate($_SERVER['HTTP_ACCEPT_LANGUAGE']);
  }

  // set cookie to remember selected language
  if ($page_lang !== null) {
    Cookie::write(RosCMS::getInstance()->cookieLanguage(), $page_lang, time() + 5 * 30 * 24 * 3600);
  }
}

// no language found, try to get standard language
if ($page_lang === null) {
  $page_lang = Language::validate('invalid language code');
}

// { (replace later with check, if that page exists, and what type of extension it has set)
  // standard extension
  $page_extension = 'html';

  // standard extension
  $page_name = ((isset($_GET['page']) && preg_match('/^[a-z_\-0-9]+$/i',$_GET['page'])) ? $_GET['page'] : 'index');
// }

// redirect to selected page
header('Location: '.ROSCMS_PATH.RosCMS::getInstance()->pathGenerated().$page_lang.'/'.$page_name.'.'.$page_extension);
exit();
?>
