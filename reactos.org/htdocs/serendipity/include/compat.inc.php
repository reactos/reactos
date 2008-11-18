<?php # $Id: compat.inc.php 448 2005-09-05 21:14:49Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

$serendipity = array();
@ini_set('magic_quotes_runtime', 'off');

if (!defined('PATH_SEPARATOR')) {
    if (strtoupper(substr(PHP_OS, 0, 3)) === 'WIN') {
        define('PATH_SEPARATOR', ';');
    } else {
        define('PATH_SEPARATOR', ':');
    }
}

if (!defined('DIRECTORY_SEPARATOR')) {
    if (strtoupper(substr(PHP_OS, 0, 3)) === 'WIN') {
        define('DIRECTORY_SEPARATOR', '\\');
    } else {
        define('DIRECTORY_SEPARATOR', '/');
    }
}

/* Function to get a snapshot uf used memory */
function memSnap($tshow = '') {
    static $avail    = null;
    static $show     = true;
    static $memUsage = 0;
    
    if (!$show) {
        return false;
    }
    
    if ($avail === false) {
        return true;
    } elseif ($avail === null) {
        if (function_exists('memory_get_usage')) {
            $avail = memory_get_usage();
        } else {
            $avail = false;
            return false;
        }
    }
    
    if ($memUsage === 0) {
        $memUsage = $avail;
    }
    
    $current = memory_get_usage();
    echo '[' . date('d.m.Y H:i') . '] ' . number_format($current - $memUsage, 2, ',', '.') . ' label "' . $tshow . '", totalling ' . number_format($current, 2, ',', '.') . '<br />' . "\n";
    $memUsage = $current;
}

if (!function_exists('file_get_contents')) {
    function file_get_contents($filename, $use_include_path = 0) {
        $file = fopen($filename, 'rb', $use_include_path);
        $data = '';
        if ($file) {
            while (!feof($file)) {
                $data .= fread($file, 4096);
            }
            fclose($file);
        }

        return $data;
    }
}

if (!isset($_REQUEST)) {
    $_REQUEST = &$HTTP_REQUEST_VARS;
}
if (!isset($_POST)) {
    $_POST = &$HTTP_POST_VARS;
}

if (!isset($_GET)) {
    $_GET = &$HTTP_GET_VARS;
}

if (!isset($_SESSION)) {
    $_SESSION = &$HTTP_SESSION_VARS;
}

if (!isset($_COOKIE)) {
    $_COOKIE = &$HTTP_COOKIE_VARS;
}

if (!isset($_SERVER)) {
    $_SERVER = &$HTTP_SERVER_VARS;
}

/*
 *  Avoid magic_quotes_gpc issues
 *  courtesy of iliaa@php.net
 */
function serendipity_strip_quotes(&$var)
{
    if (is_array($var)) {
        foreach ($var as $k => $v) {
            if (is_array($v)) {
                array_walk($var[$k], 'serendipity_strip_quotes');
            } else {
                $var[$k] = stripslashes($v);
            }
        }
    } else {
        $var = stripslashes($var);
    }
}

if (ini_get('magic_quotes_gpc')) {
    if (@count($_REQUEST)) {
        array_walk($_REQUEST, 'serendipity_strip_quotes');
    }

    if (@count($_GET)) {
        array_walk($_GET,     'serendipity_strip_quotes');
    }

    if (@count($_POST)) {
        array_walk($_POST,    'serendipity_strip_quotes');
    }

    if (@count($_COOKIE)) {
        array_walk($_COOKIE, 'serendipity_strip_quotes');
    }

    if (@count($_FILES) && strtoupper(substr(PHP_OS, 0, 3)) !== 'WIN') {
        array_walk($_FILES,   'serendipity_strip_quotes');
    }
}


// Merge get and post into the serendipity array
$serendipity['GET']    = &$_GET['serendipity'];
$serendipity['POST']   = &$_POST['serendipity'];
$serendipity['COOKIE'] = &$_COOKIE['serendipity'];

// Attempt to fix IIS compatibility
if (empty($_SERVER['REQUEST_URI'])) {
    $_SERVER['REQUEST_URI'] = $_SERVER['SCRIPT_NAME'] . '?' . (!empty($_SERVER['QUERY_STRING']) ? $_SERVER['QUERY_STRING'] : '');
}

// Some security issues
if (isset($serendipity['GET']['searchTerm'])) {
    $serendipity['GET']['searchTerm'] = htmlspecialchars(strip_tags($serendipity['GET']['searchTerm']));
}

function serendipity_get_bool($item) {
    static $translation = array('true'  => true,
                                'false' => false);

    if (isset($translation[$item])) {
        return $translation[$item];
    } else {
        return $item;
    }
}

// Needs to be included here so that it's globally available also when installing
function serendipity_detectLang($use_include = false) {
    global $serendipity;

    $supported_languages = array_keys($serendipity['languages']);
    $possible_languages = explode(',', (isset($_SERVER['HTTP_ACCEPT_LANGUAGE']) ? $_SERVER['HTTP_ACCEPT_LANGUAGE'] : ''));
    if (is_array($possible_languages)) {
        foreach($possible_languages as $index => $lang) {
            $preferred_language = strtolower(preg_replace('@^([^\-_;]*)_?.*$@', '\1', $lang));
            if (in_array($preferred_language, $supported_languages)) {
                if ($use_include) {
                    @include_once(S9Y_INCLUDE_PATH . 'lang/serendipity_lang_' . $preferred_language . '.inc.php');
                    $serendipity['autolang'] = $preferred_language;
                }
                return $preferred_language;
            } // endif
        } // endforeach
    } // endif

    return $serendipity['lang'];
}

function serendipity_getCoreVersion($version) {
    return preg_replace('@^([0-9\.]+).*$@', '\1', $version);
}

function serendipity_die($html) {
    die(
        '<html>
            <head>
                <meta http-equiv="Content-Type" content="text/html; charset=' . LANG_CHARSET . '">
            </head>
            <body>' . $html . '</body>
        </html>');
}

/*
 *  Some defaults for our config vars.
 *  They are likely to be overwritten later in the code
 */
$serendipity['templatePath'] = 'templates/';
if (!isset($serendipity['serendipityPath'])) {
    $serendipity['serendipityPath'] = (defined('S9Y_INCLUDE_PATH') ? S9Y_INCLUDE_PATH : './');
}

$serendipity['indexFile'] = 'index.php';

/* vim: set sts=4 ts=4 expandtab : */
?>
