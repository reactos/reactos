<?php # $Id: lang.inc.php 350 2005-08-01 18:34:14Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (!defined('serendipity_LANG_LOADED') || serendipity_LANG_LOADED !== true) {
    // Try and include preferred language from the configurated setting
    if (@include(S9Y_INCLUDE_PATH . 'lang/' . $serendipity['charset'] . 'serendipity_lang_'. $serendipity['lang'] .'.inc.php') ) {
        // Only here can we truely say the language is loaded
        define('serendipity_LANG_LOADED', true);
    } elseif (IS_installed === false || (defined('IS_up2date') && IS_up2date === false)) {   /* -- Auto-Guess -- */
        // If no config file is loaded, language includes are not available.
        // Now include one. Try to auto-guess the language by looking up the HTTP_ACCEPT_LANGUAGE.
        serendipity_detectLang(true);
    }  //endif

    // Do fallback to english
    if (IS_installed === false || (defined('IS_up2date') && IS_up2date === false)) {
        @include_once(S9Y_INCLUDE_PATH . 'lang/serendipity_lang_en.inc.php');
    }
}

if (!defined('serendipity_MB_LOADED') && defined('serendipity_LANG_LOADED')) {
    // Needs to be included here because we need access to constant LANG_CHARSET definied in languages (not available for compat.inc.php)

    if (function_exists('mb_language')) {
        @mb_language($serendipity['lang']);
    }
    
    if (function_exists('mb_internal_encoding')) {
        @mb_internal_encoding(LANG_CHARSET);
    }

    // Multibyte string functions wrapper:
    // strlen(), strpos(), strrpos(), strtolower(), strtoupper(), substr(), ucfirst()
    function serendipity_mb() {
        static $mbstring = null;

        if (is_null($mbstring)) {
            $mbstring = (extension_loaded('mbstring') ? 1 : 0);
            if ($mbstring === 1) {
                if (function_exists('mb_strtoupper')) {
                    $mbstring = 2;
                }
            }
        }

        $args = func_get_args();
        $func = $args[0];
        unset($args[0]);

        switch($func) {
            case 'ucfirst':
                // there's no mb_ucfirst, so emulate it
                if ($mbstring === 2) {
                    return mb_strtoupper(mb_substr($args[1], 0, 1)) . mb_substr($args[1], 1);
                } else {
                    return ucfirst($args[1]);
                }

            case 'strtolower':
            case 'strtoupper':
                if ($mbstring === 2) {
                    return call_user_func_array('mb_' . $func, $args);
                } else {
                    return call_user_func_array($func, $args);
                }

            default:
                if ($mbstring) {
                    return call_user_func_array('mb_' . $func, $args);
                } else {
                    return call_user_func_array($func, $args);
                }
        }
    }

    define('serendipity_MB_LOADED', true);
}

/* vim: set sts=4 ts=4 expandtab : */
?>