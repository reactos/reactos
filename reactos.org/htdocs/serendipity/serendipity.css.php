<?php # $Id: serendipity.css.php 316 2005-07-28 14:54:20Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

/* This is a small hack to allow CSS display during installations and upgrades */
define('IN_installer', true);
define('IN_upgrader', true);
define('IN_CSS', true);

session_cache_limiter('public');
include_once('serendipity_config.inc.php');

if (!isset($css_mode)) {
    if (!empty($serendipity['GET']['css_mode'])) {
        $css_mode = $serendipity['GET']['css_mode'];
    } else {
        $css_mode = 'serendipity.css';
    }
}

switch($css_mode) {
    case 'serendipity.css':
    default:
        $css_hook = 'css';
        $css_file = 'style.css';
        break;

    case 'serendipity_admin.css':
        $css_hook = 'css_backend';
        $css_file = 'admin/style.css';
        break;
}

function serendipity_printStylesheet($file) {
    global $serendipity;
    return str_replace(
             array(
               '{TEMPLATE_PATH}',
               '{LANG_DIRECTION}'
             ),

             array(
               dirname($file) . '/',
               LANG_DIRECTION
             ),

             @file_get_contents($file, 1));
}


if (strpos($_SERVER['HTTP_USER_AGENT'], 'MSIE') !== false) {
    header('Cache-Control: no-cache');
} else {
    header('Cache-Control:');
    header('Pragma:');
    header('Expires: ' . gmdate('D, d M Y H:i:s \G\M\T', time()+3600));
}
header('Content-type: text/css');

if (IS_installed === false) {
    if (file_exists('templates/' . $serendipity['defaultTemplate'] . '/' . $css_file)) {
        echo serendipity_printStylesheet('templates/' . $serendipity['defaultTemplate'] . '/' . $css_file);
    }
    die();
}


$out = serendipity_printStylesheet(serendipity_getTemplateFile($css_file, ''));

serendipity_plugin_api::hook_event($css_hook, $out);

echo $out;

/* vim: set sts=4 ts=4 expandtab : */
?>
