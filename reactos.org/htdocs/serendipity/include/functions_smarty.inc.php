<?php # $Id: functions_smarty.inc.php 565 2005-10-17 13:04:11Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

function serendipity_fetchTrackbacks($id, $limit = null, $showAll = false) {
    global $serendipity;

    if (!$showAll) {
        $and = "AND status = 'approved'";
    }

    $query = "SELECT * FROM {$serendipity['dbPrefix']}comments WHERE entry_id = '". (int)$id ."' AND type = 'TRACKBACK' $and ORDER BY id";
    if (isset($limit)) {
        $limit  = serendipity_db_limit_sql($limit);
        $query .= " $limit";
    }

    $comments = serendipity_db_query($query);
    if (!is_array($comments)) {
        return array();
    }

    return $comments;
}

function serendipity_printTrackbacks($trackbacks) {
    global $serendipity;

    $serendipity['smarty']->assign('trackbacks', $trackbacks);

    return serendipity_smarty_fetch('TRACKBACKS', 'trackbacks.tpl');
}

function &serendipity_smarty_fetch($block, $file, $echo = false) {
    global $serendipity;

    $output = $serendipity['smarty']->fetch('file:'. serendipity_getTemplateFile($file, 'serendipityPath'), null, null, ($echo === true && $serendipity['smarty_raw_mode']));
    $serendipity['smarty']->assign($block, $output);

    return $output;
}

function serendipity_emptyPrefix($string, $prefix = ': ') {
    return (!empty($string) ? $prefix . htmlspecialchars($string) : '');
}

function serendipity_smarty_showPlugin($params, &$smarty) {
    global $serendipity;

    if (!isset($params['class']) && !isset($params['id'])) {
        $smarty->trigger_error(__FUNCTION__ .": missing 'class' or 'id' parameter");
        return;
    }

    if (!isset($params['side'])) {
        $params['side'] = '*';
    }

    if (!isset($params['negate'])) {
        $params['negate'] = null;
    }

    return serendipity_plugin_api::generate_plugins($params['side'], null, $params['negate'], $params['class'], $params['id']);
}

function serendipity_smarty_hookPlugin($params, &$smarty) {
    global $serendipity;
    static $hookable = array('frontend_header',
                             'entries_header',
                             'entries_footer',
                             'frontend_comment',
                             'frontend_footer');

    if (!isset($params['hook'])) {
        $smarty->trigger_error(__FUNCTION__ .": missing 'hook' parameter");
        return;
    }

    if (!in_array($params['hook'], $hookable) && $params['hookAll'] != 'true') {
        $smarty->trigger_error(__FUNCTION__ .": illegal hook '". $params['hook'] ."'");
        return;
    }

    if (!isset($params['data'])) {
        $params['data'] = &$serendipity;
    }

    if (!isset($params['addData'])) {
        $params['addData'] = null;
    }

    serendipity_plugin_api::hook_event($params['hook'], $params['data'], $params['addData']);
}


function serendipity_smarty_printSidebar($params, &$smarty) {
    if ( !isset($params['side']) ) {
        $smarty->trigger_error(__FUNCTION__ .": missing 'side' parameter");
        return;
    }
    return serendipity_plugin_api::generate_plugins($params['side']);
}

function serendipity_smarty_getFile($params, &$smarty) {
    if ( !isset($params['file']) ) {
        $smarty->trigger_error(__FUNCTION__ .": missing 'file' parameter");
        return;
    }
    return serendipity_getTemplateFile($params['file']);
}


function serendipity_smarty_formatTime($timestamp, $format, $useOffset = true) {
    if (defined($format)) {
        return serendipity_formatTime(constant($format), $timestamp, $useOffset);
    } else {
        return serendipity_formatTime($format, $timestamp, $useOffset);
    }
}

function &serendipity_smarty_printComments($params, &$smarty) {
    global $serendipity;

    if (!isset($params['entry'])) {
        $smarty->trigger_error(__FUNCTION__ .": missing 'entry' parameter");
        return;
    }

    if (!isset($params['mode'])) {
        $params['mode'] = VIEWMODE_THREADED;
    }

    $comments = serendipity_fetchComments($params['entry']);

    if (!empty($serendipity['POST']['preview'])) {
        $comments[] =
            array(
                    'email'     => $serendipity['POST']['email'],
                    'author'    => $serendipity['POST']['name'],
                    'body'      => $serendipity['POST']['comment'],
                    'url'       => $serendipity['POST']['url'],
                    'parent_id' => $serendipity['POST']['replyTo'],
                    'timestamp' => time()
            );
    }

    $out = serendipity_printComments($comments, $params['mode']);
    return $out;
}

function serendipity_smarty_printTrackbacks($params, &$smarty) {
    if ( !isset($params['entry']) ) {
        $smarty->trigger_error(__FUNCTION__ .": missing 'entry' parameter");
        return;
    }

    return serendipity_printTrackbacks(serendipity_fetchTrackbacks($params['entry']));
}

function &serendipity_replaceSmartyVars(&$tpl_source, $smarty) {
    $tpl_source = str_replace('$CONST.', '$smarty.const.', $tpl_source);
    return $tpl_source;
}

function serendipity_smarty_init() {
    global $serendipity;

    if (!isset($serendipity['smarty'])) {
        @define('SMARTY_DIR', S9Y_PEAR_PATH . 'Smarty/libs/');
        require_once SMARTY_DIR . 'Smarty.class.php';
        $serendipity['smarty'] = new Smarty;
        if ($serendipity['production'] === 'debug') {
            $serendipity['smarty']->force_compile   = true;
            $serendipity['smarty']->debugging       = true;
        }
        $serendipity['smarty']->template_dir  = array(
        	$serendipity['serendipityPath'] . $serendipity['templatePath'] . $serendipity['template'],
        	$serendipity['serendipityPath'] . $serendipity['templatePath'] . 'default'
        );
        $serendipity['smarty']->compile_dir   = $serendipity['serendipityPath'] . PATH_SMARTY_COMPILE;
        $serendipity['smarty']->config_dir    = &$serendipity['smarty']->template_dir[0];
        $serendipity['smarty']->secure_dir    = array($serendipity['serendipityPath'] . $serendipity['templatePath']);
        $serendipity['smarty']->security_settings['MODIFIER_FUNCS']  = array('sprintf', 'sizeof', 'count', 'rand');
        $serendipity['smarty']->security_settings['ALLOW_CONSTANTS'] = true;
        $serendipity['smarty']->security      = true;
        $serendipity['smarty']->use_sub_dirs  = false;
        $serendipity['smarty']->compile_check = true;
        $serendipity['smarty']->compile_id    = &$serendipity['template'];

        $serendipity['smarty']->register_modifier('makeFilename', 'serendipity_makeFilename');
        $serendipity['smarty']->register_modifier('xhtml_target', 'serendipity_xhtml_target');
        $serendipity['smarty']->register_modifier('emptyPrefix', 'serendipity_emptyPrefix');
        $serendipity['smarty']->register_modifier('formatTime', 'serendipity_smarty_formatTime');
        $serendipity['smarty']->register_function('serendipity_printSidebar', 'serendipity_smarty_printSidebar');
        $serendipity['smarty']->register_function('serendipity_hookPlugin', 'serendipity_smarty_hookPlugin');
        $serendipity['smarty']->register_function('serendipity_showPlugin', 'serendipity_smarty_showPlugin');
        $serendipity['smarty']->register_function('serendipity_getFile', 'serendipity_smarty_getFile');
        $serendipity['smarty']->register_function('serendipity_printComments', 'serendipity_smarty_printComments');
        $serendipity['smarty']->register_function('serendipity_printTrackbacks', 'serendipity_smarty_printTrackbacks');
        $serendipity['smarty']->register_prefilter('serendipity_replaceSmartyVars');
    }

    if (!isset($serendipity['smarty_raw_mode'])) {
        if (file_exists($serendipity['smarty']->config_dir . '/layout.php') && $serendipity['template'] != 'default') {
            $serendipity['smarty_raw_mode'] = true;
        } else {
            $serendipity['smarty_raw_mode'] = false;
        }
    }

    if (!isset($serendipity['smarty_file'])) {
        $serendipity['smarty_file'] = 'index.tpl';
    }

    $category      = false;
    $category_info = array();
    if (isset($serendipity['GET']['category'])) {
        $category = (int)$serendipity['GET']['category'];
        if (isset($GLOBALS['cInfo'])) {
            $category_info = $GLOBALS['cInfo'];
        } else {
            $category_info = serendipity_fetchCategoryInfo($category);
        }
    }
    
    if (!isset($serendipity['smarty_vars']['head_link_stylesheet'])) {
        $serendipity['smarty_vars']['head_link_stylesheet'] = serendipity_rewriteURL('serendipity.css');
    }

    $serendipity['smarty']->assign(
        array(
            'head_charset'              => LANG_CHARSET,
            'head_version'              => $serendipity['version'],
            'head_title'                => $serendipity['head_title'],
            'head_subtitle'             => $serendipity['head_subtitle'],
            'head_link_stylesheet'      => $serendipity['smarty_vars']['head_link_stylesheet'],

            'is_xhtml'                  => $serendipity['XHTML11'],
            'use_popups'                => $serendipity['enablePopup'],
            'is_embedded'               => (!$serendipity['embed'] || $serendipity['embed'] === 'false' || $serendipity['embed'] === false) ? false : true,
            'is_raw_mode'               => $serendipity['smarty_raw_mode'],

            'entry_id'                  => (isset($serendipity['GET']['id']) && is_numeric($serendipity['GET']['id'])) ? $serendipity['GET']['id'] : false,
            'is_single_entry'           => (isset($serendipity['GET']['id']) && is_numeric($serendipity['GET']['id'])),

            'blogTitle'                 => htmlspecialchars($serendipity['blogTitle']),
            'blogSubTitle'              => (!empty($serendipity['blogSubTitle']) ? htmlspecialchars($serendipity['blogSubTitle']) : ''),
            'blogDescription'           => htmlspecialchars($serendipity['blogDescription']),

            'serendipityHTTPPath'       => $serendipity['serendipityHTTPPath'],
            'serendipityBaseURL'        => $serendipity['baseURL'],
            'serendipityRewritePrefix'  => $serendipity['rewrite'] == 'none' ? $serendipity['indexFile'] . '?/' : '',
            'serendipityIndexFile'      => $serendipity['indexFile'],
            'serendipityVersion'        => $serendipity['version'],

            'lang'                      => $serendipity['lang'],
            'category'                  => $category,
            'category_info'             => $category_info,
            'template'                  => $serendipity['template'],

            'dateRange'                 => (!empty($serendipity['range']) ? $serendipity['range'] : array())
        )
    );

    return true;
}

/* Nukes all Smarty compiled templates and cache */
function serendipity_smarty_purge() {
    global $serendipity;

    /* Attempt to init Smarty, brrr */
    serendipity_smarty_init();

    $files = serendipity_traversePath($serendipity['smarty']->compile_dir, '', false, '/.+\.tpl\.php$/');

    if ( !is_array($files) ) {
        return false;
    }

    foreach ( $files as $file ) {
        @unlink($serendipity['smarty']->compile_dir . DIRECTORY_SEPARATOR . $file['name']);
    }
}

/* Function can be called from foreign applications. ob_start() needs to
  have been called before, and will be parsed into Smarty here */
function serendipity_smarty_shutdown($serendipity_directory = '') {
global $serendipity;

    $cwd = getcwd();
    chdir($serendipity_directory);
    $raw_data = ob_get_contents();
    ob_end_clean();
    $serendipity['smarty']->assign('content_message', $raw_data);

    serendipity_smarty_fetch('CONTENT', 'content.tpl');
    if (empty($serendipity['smarty_file'])) {
        $serendipity['smarty_file'] = '404.tpl';
    }
    $serendipity['smarty']->display(serendipity_getTemplateFile($serendipity['smarty_file'], 'serendipityPath'));
}

