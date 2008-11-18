<?php # $Id: configuration.inc.php 602 2005-10-25 11:04:04Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

umask(0000);
$umask = 0775;
@define('IN_installer', true);

if (!isset($_POST['installAction'])) {
    $_POST['installAction'] = '';
}

if (!serendipity_checkPermission('siteConfiguration') && !serendipity_checkPermission('blogConfiguration')) {
    return;
}

switch ($_POST['installAction'] && serendipity_checkFormToken()) {
    case 'check':
        $oldConfig = $serendipity;
        $res = serendipity_updateConfiguration();
        if (is_array($res)) {
            echo DIAGNOSTIC_ERROR;
            echo '<span class="serendipityAdminMsgError">- ' . implode('<br />', $res) . '</span><br /><br />';
        } else {
            /* If we have new rewrite rules, then install them */
            $permalinkOld = array(
                $oldConfig['serendipityHTTPPath'],
                $oldConfig['serendipityPath'],
                $oldConfig['baseURL'],
                $oldConfig['indexFile'],
                $oldConfig['rewrite']);

            $permalinkNew = array(
                $serendipity['serendipityHTTPPath'],
                $serendipity['serendipityPath'],
                $serendipity['baseURL'],
                $serendipity['indexFile'],
                $serendipity['rewrite']);

            // Compare all old permalink section values against new one. A change in any of those
            // will force to update the .htaccess for rewrite rules.
            if ($serendipity['rewrite'] != 'none') {
                $permconf = serendipity_parseTemplate(S9Y_CONFIG_TEMPLATE);
                if (is_array($permconf) && is_array($permconf['permalinks']['items'])) {
                    foreach($permconf['permalinks']['items'] AS $permitem) {
                        $permalinkOld[] = $oldConfig[$permitem['var']];
                        $permalinkNew[] = $serendipity[$permitem['var']];
                    }
                }
            }            

            if (serendipity_checkPermission('siteConfiguration') && serialize($permalinkOld) != serialize($permalinkNew)) {
                printf(ATTEMPT_WRITE_FILE, $serendipity['serendipityPath'] . '.htaccess');
                $res = serendipity_installFiles($serendipity['serendipityPath']);
                if (is_array($res)) {
                    echo implode('<br />', $res);
                } else {
                    echo DONE . '<br />';
                }

                serendipity_buildPermalinks();
            }

            echo '<br /><div class="serendipityAdminMsgSuccess">'. WRITTEN_N_SAVED .'</div>';
        }

        break;

    default:
        $from = &$serendipity;
        $t = serendipity_parseTemplate(S9Y_CONFIG_TEMPLATE);
        serendipity_printConfigTemplate($t, $from, false, true);
        break;
}

/* vim: set sts=4 ts=4 expandtab : */
?>
