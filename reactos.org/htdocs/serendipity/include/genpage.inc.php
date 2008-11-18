<?php # $Id: genpage.inc.php 484 2005-09-21 14:37:09Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

include_once('serendipity_config.inc.php');
include_once(S9Y_INCLUDE_PATH . 'include/plugin_api.inc.php');
include_once(S9Y_INCLUDE_PATH . 'include/plugin_internal.inc.php');

serendipity_plugin_api::hook_event('genpage', $uri);
serendipity_smarty_init();

// For advanced usage, we allow template authors to create a file 'config.inc.php' where they can
// setup custom smarty variables, modifiers etc. to use in their templates.
@include_once $serendipity['smarty']->config_dir . '/config.inc.php';

$serendipity['smarty']->assign(
    array(
        'leftSidebarElements'       => serendipity_plugin_api::count_plugins('left'),
        'rightSidebarElements'      => serendipity_plugin_api::count_plugins('right')
    )
);

if ($serendipity['smarty_raw_mode']) {
    /* For BC reasons, we have to ask for layout.php */
    @include_once(serendipity_getTemplateFile('layout.php', 'serendipityPath'));
} else {
    switch ($serendipity['GET']['action']) {
        // User wants to read the diary
        case 'read':
            if (isset($serendipity['GET']['id'])) {
                $entry = array(serendipity_fetchEntry('id', $serendipity['GET']['id']));
                if (!is_array($entry) || count($entry) < 1) {
                    unset($serendipity['GET']['id']);
                    $entry = array(array());
                }

                serendipity_printEntries($entry, 1);
            } else {
                serendipity_printEntries(serendipity_fetchEntries($serendipity['range'], true, $serendipity['fetchLimit']));
            }
            break;

        // User searches
        case 'search':
            $r = serendipity_searchEntries($serendipity['GET']['searchTerm']);
            if (strlen($serendipity['GET']['searchTerm']) <= 3) {
                $serendipity['smarty']->assign(
                    array(
                        'content_message'       => SEARCH_TOO_SHORT,
                        'searchresult_tooShort' => true
                    )
                );
                break;
            }

            if (is_string($r) && $r !== true) {
                $serendipity['smarty']->assign(
                    array(
                        'content_message'    => sprintf(SEARCH_ERROR, $serendipity['dbPrefix'], $r),
                        'searchresult_error' => true
                    )
                );
                break;
            } elseif ($r === true) {
                $serendipity['smarty']->assign(
                    array(
                        'content_message'        => sprintf(NO_ENTRIES_BLAHBLAH, $serendipity['GET']['searchTerm']),
                        'searchresult_noEntries' => true
                    )
                );
                break;
            }

            $serendipity['smarty']->assign(
                array(
                    'content_message'      => sprintf(YOUR_SEARCH_RETURNED_BLAHBLAH, $serendipity['GET']['searchTerm'], serendipity_getTotalEntries()),
                    'searchresult_results' => true
                )
            );

            serendipity_printEntries($r);
            break;

        // Show the archive
        case 'archives':
            serendipity_printArchives();
            break;

        // Welcome screen or whatever
        default:
            serendipity_printEntries(serendipity_fetchEntries(null, true, $serendipity['fetchLimit']));
            break;
    }

    serendipity_smarty_fetch('CONTENT', 'content.tpl');
}

/* vim: set sts=4 ts=4 expandtab : */
?>
