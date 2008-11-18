<?php # $Id: serendipity_config.inc.php 627 2005-10-30 11:29:01Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (!headers_sent()) {
    session_start();
}

if (!defined('S9Y_INCLUDE_PATH')) {
    define('S9Y_INCLUDE_PATH', dirname(__FILE__) . '/');
}
define('S9Y_CONFIG_TEMPLATE',     S9Y_INCLUDE_PATH . 'include/tpl/config_local.inc.php');
define('S9Y_CONFIG_USERTEMPLATE', S9Y_INCLUDE_PATH . 'include/tpl/config_personal.inc.php');

define('IS_installed', file_exists('serendipity_config_local.inc.php'));

if (IS_installed === true && !defined('IN_serendipity')) {
    define('IN_serendipity', true);
}

include_once(S9Y_INCLUDE_PATH . 'include/compat.inc.php');

// The version string
$serendipity['version']         = '0.9.1';

// Name of folder for the default theme
$serendipity['defaultTemplate'] = 'default';

// Setting this to 'false' will enable debugging output. All alpa/beta/cvs snapshot versions will emit debug information by default. To increase the debug level (to enable Smarty debugging), set this flag to 'debug'.
$serendipity['production']      = (preg_match('@\-(alpha|beta|cvs)@', $serendipity['version']) ? false : true);

// Set error reporting
error_reporting(E_ALL & ~E_NOTICE);

if ($serendipity['production'] !== true) {
    if ($serendipity['production'] === 'debug') {
        error_reporting(E_ALL);
    }
    @ini_set('display_errors', 'on');
}

// Default rewrite method
$serendipity['rewrite']         = 'none';

// Message container
$serendipity['messagestack']    = array();

// Can the user change the date of publishing for an entry?
$serendipity['allowDateManipulation'] = true;

// How much time is allowed to pass since the publising of an entry, so that a comment to that entry
// will update it's LastModified stamp? If the time is passed, a comment to an old entry will no longer
// push an article as being updated.
$serendipity['max_last_modified']    = 60 * 60 * 24 * 7;

// Clients can send a If-Modified Header to the RSS Feed (Conditional Get) and receive all articles beyond
// that date. However it is still limited by the number below of maximum entries
$serendipity['max_fetch_limit']      = 50;

// How many bytes are allowed for fetching trackbacks, so that no binary files get accidently trackbacked?
$serendipity['trackback_filelimit']  = 150 * 1024;

if (!isset($serendipity['fetchLimit'])) {
    $serendipity['fetchLimit'] = 15;
}

if (!isset($serendipity['use_PEAR'])) {
    $serendipity['use_PEAR'] = false;
}

// Should IFRAMEs be used for previewing entries and sending trackbacks?
$serendipity['use_iframe'] = true;

/* Default language for autodetection */
$serendipity['autolang'] = 'en';

/* Availiable languages */
$serendipity['languages'] = array('en' => 'English',
                                  'de' => 'German',
                                  'da' => 'Danish',
                                  'es' => 'Spanish',
                                  'fr' => 'French',
                                  'fi' => 'Finnish',
                                  'cs' => 'Czech (Win-1250)',
                                  'cz' => 'Czech (ISO-8859-2)',
                                  'nl' => 'Dutch',
                                  'is' => 'Icelandic',
                                  'se' => 'Swedish',
                                  'pt' => 'Portuguese Brazilian',
                                  'pt_PT' => 'Portuguese European',
                                  'bg' => 'Bulgarian',
                                  'hu' => 'Hungarian',
                                  'no' => 'Norwegian',
                                  'ro' => 'Romanian',
                                  'it' => 'Italian',
                                  'ru' => 'Russian',
                                  'fa' => 'Persian',
                                  'tw' => 'Traditional Chinese (Big5)',
                                  'tn' => 'Traditional Chinese (UTF-8)',
                                  'zh' => 'Simplified Chinese (GB2312)',
                                  'cn' => 'Simplified Chinese (UTF-8)',
                                  'ja' => 'Japanese',
                                  'ko' => 'Korean');

/* Available Calendars */
$serendipity['calendars'] = array('gregorian'   => 'Gregorian',
                                  'jalali-utf8' => 'Jalali (utf8)');

/*
 *   Load main language file
 */
include($serendipity['serendipityPath'] . 'include/lang.inc.php');

$serendipity['charsets'] = array(
    'UTF-8/' => 'UTF-8',
    ''        => CHARSET_NATIVE
);

@define('PATH_SMARTY_COMPILE', 'templates_c'); // will be placed inside the template directory
@define('USERLEVEL_ADMIN', 255);
@define('USERLEVEL_CHIEF', 1);
@define('USERLEVEL_EDITOR', 0);

@define('VIEWMODE_THREADED', 'threaded');
@define('VIEWMODE_LINEAR', 'linear');

/*
 *   Kill the script if we are not installed, and not inside the installer
 */
if ( !defined('IN_installer') && IS_installed === false ) {
    header('Location: ' . ($_SERVER['HTTPS'] == 'on' ? 'https://' : 'http://') . $_SERVER['HTTP_HOST'] . str_replace('\\', '/', dirname($_SERVER['PHP_SELF'])) . '/serendipity_admin.php');
    serendipity_die(sprintf(SERENDIPITY_NOT_INSTALLED, 'serendipity_admin.php'));
}

// Check whether local or global PEAR should be used. You can put a
// $serendipity['use_PEAR'] = true;
// in your serendipity_config_local.inc.php file to enable this.
// The required PEAR (and other) packages are mentioned in the file
// bundled-libs/.current_version
$old_include = @ini_get('include_path');
if (@ini_set('include_path', $old_include . PATH_SEPARATOR . $serendipity['serendipityPath'] . PATH_SEPARATOR . $serendipity['serendipityPath'] . 'bundled-libs/') && $serendipity['use_PEAR']) {
    @define('S9Y_PEAR', true);
    @define('S9Y_PEAR_PATH', '');
} else {
    @define('S9Y_PEAR', false);
    @define('S9Y_PEAR_PATH', S9Y_INCLUDE_PATH . 'bundled-libs/');
}

if (defined('IN_installer') && IS_installed === false) {
    $serendipity['lang'] = $serendipity['autolang'];
    $css_mode            = 'serendipity_admin.css';
    return 1;
}

/*
 *   Load DB configuration information
 *   Load Functions
 *   Make sure that the file included is in the current directory and not any possible
 *   include path
 */
if (file_exists($_SERVER['DOCUMENT_ROOT'] . dirname($_SERVER['PHP_SELF']) . '/serendipity_config_local.inc.php')) {
    $local_config = $_SERVER['DOCUMENT_ROOT'] . dirname($_SERVER['PHP_SELF']) . '/serendipity_config_local.inc.php';
} elseif (file_exists($serendipity['serendipityPath'] . '/serendipity_config_local.inc.php')) {
    $local_config = $serendipity['serendipityPath'] . '/serendipity_config_local.inc.php';
} else {
    $local_config = S9Y_INCLUDE_PATH . '/serendipity_config_local.inc.php';
}

if (is_readable($local_config)) {
    include_once($local_config);
} else {
    serendipity_die(sprintf(INCLUDE_ERROR, $local_config));
}

define('IS_up2date', version_compare($serendipity['version'], $serendipity['versionInstalled'], '<='));

/*
 *  Include main functions
 */
include_once(S9Y_INCLUDE_PATH . 'include/functions.inc.php');

if (serendipity_FUNCTIONS_LOADED!== true) {
    serendipity_die(sprintf(INCLUDE_ERROR, 'include/functions.inc.php'));
}

/*
 *   Attempt to connect to the database
 */
if (!serendipity_db_connect()) {
    serendipity_die(DATABASE_ERROR);
}

/*
 *   Load Configuration options from the database
 */

serendipity_load_configuration();

/*
 * If a user is logged in, fetch his preferences. He possibly wants to have a different language
 */

if (IS_installed === true) {
    serendipity_login(false);
}

$serendipity['lang'] = serendipity_getSessionLanguage(); // @see function declaration for todo

if (isset($_SESSION['serendipityAuthorid'])) {
    serendipity_load_configuration($_SESSION['serendipityAuthorid']);
}

// Try to fix some path settings. It seems common users have this setting wrong
// when s9y is installed into the root directory, especially 0.7.1 upgrade users.
if (empty($serendipity['serendipityHTTPPath'])) {
    $serendipity['serendipityHTTPPath'] = '/';
}

/* Changing this is NOT recommended, rewrite rules does not take them into account - yet */
serendipity_initPermalinks();

// Apply constants/definitions from custom permalinks
serendipity_permalinkPatterns();

/*
 *   Load main language file again, because now we have the preferred language
 */
include(S9Y_INCLUDE_PATH . 'include/lang.inc.php');

/*
 * Reset charset definition now that final language is known
 */
$serendipity['charsets'] = array(
    'UTF-8/' => 'UTF-8',
    ''        => CHARSET_NATIVE
);

/*
 *   Set current locale, if any has been defined
 */
if (defined('DATE_LOCALES')) {
    $locales = explode(',', DATE_LOCALES);
    foreach ($locales as $locale) {
        $locale = trim($locale);
        if (setlocale(LC_TIME, $locale) == $locale) {
            break;
        }
    }
}

/*
 *   Fallback charset, if none is defined in the language files
 */
@define('LANG_CHARSET', 'ISO-8859-1');

/*
 *  Create array of permission levels, with descriptions
 */
$serendipity['permissionLevels'] = array(USERLEVEL_EDITOR => USERLEVEL_EDITOR_DESC,
                                         USERLEVEL_CHIEF => USERLEVEL_CHIEF_DESC,
                                         USERLEVEL_ADMIN => USERLEVEL_ADMIN_DESC);


if ( (isset($serendipity['autodetect_baseURL']) && serendipity_db_bool($serendipity['autodetect_baseURL'])) ||
     (isset($serendipity['embed']) && serendipity_db_bool($serendipity['embed'])) ) {
    $serendipity['baseURL'] = 'http' . (isset($_SERVER['HTTPS']) && $_SERVER['HTTPS'] == 'on' ? 's' : '') . '://' . $_SERVER['HTTP_HOST'] . (!strstr($_SERVER['HTTP_HOST'], ':') && !empty($_SERVER['SERVER_PORT']) && $_SERVER['SERVER_PORT'] != '80' ? ':' . $_SERVER['SERVER_PORT'] : '') . $serendipity['serendipityHTTPPath'];
}

/*
 *  Check if the installed version is higher than the version of the config
 */

if (IS_up2date === false && !defined('IN_upgrader')) {
    if (preg_match(PAT_CSS, $_SERVER['REQUEST_URI'], $matches)) {
        $css_mode = 'serendipity_admin.css';
        return 1;
    }

    serendipity_die(sprintf(SERENDIPITY_NEEDS_UPGRADE, $serendipity['versionInstalled'], $serendipity['version'], $serendipity['serendipityHTTPPath'] . 'serendipity_admin.php'));
}

// We don't care who tells us what to do
if (!isset($serendipity['GET']['action'])) {
    $serendipity['GET']['action'] = (isset($serendipity['POST']['action']) ? $serendipity['POST']['action'] : '');
}

if (!isset($serendipity['GET']['adminAction'])) {
    $serendipity['GET']['adminAction'] = (isset($serendipity['POST']['adminAction']) ? $serendipity['POST']['adminAction'] : '');
}

// Some stuff...
if (!isset($_SESSION['serendipityAuthedUser'])) {
    $_SESSION['serendipityAuthedUser'] = false;
}

if (isset($_SESSION['serendipityUser'])) {
    $serendipity['user']  = $_SESSION['serendipityUser'];
}

if (isset($_SESSION['serendipityEmail'])) {
    $serendipity['email'] = $_SESSION['serendipityEmail'];
}

serendipity_plugin_api::hook_event('frontend_configure', $serendipity);

/* vim: set sts=4 ts=4 expandtab : */
