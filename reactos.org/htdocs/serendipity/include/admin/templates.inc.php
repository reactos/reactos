<?php # $Id: templates.inc.php 236 2005-07-08 13:31:58Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (IN_serendipity !== true) {
    die ("Don't hack!");
}

if (!serendipity_checkPermission('adminTemplates')) {
    return;
}

if ($serendipity['GET']['adminAction'] == 'install' ) {
    serendipity_plugin_api::hook_event('backend_templates_fetchtemplate', $serendipity);

    $themeInfo = serendipity_fetchTemplateInfo($serendipity['GET']['theme']);

    serendipity_set_config_var('template', $serendipity['GET']['theme']);
    serendipity_set_config_var('template_engine', isset($themeInfo['engine']) ? $themeInfo['engine'] : 'default');

    echo '<div class="serendipityAdminMsgSuccess">'. sprintf(TEMPLATE_SET, $serendipity['GET']['theme']) .'</div>';
}
?>

<?php
    if ( @file_exists($serendipity['serendipityPath'] . $serendipity['templatePath'] . $serendipity['template'] .'/layout.php') ) {
        echo '<div class="serendipityAdminMsgNote">'. WARNING_TEMPLATE_DEPRECATED .'</div>';
    }
?>


<?php echo SELECT_TEMPLATE; ?>
<br /><br />
<?php
    $i = 0;
    $stack = array();
    serendipity_plugin_api::hook_event('backend_templates_fetchlist', $stack);
    $themes = serendipity_fetchTemplates();
    foreach($themes AS $theme) {
        $stack[$theme] = serendipity_fetchTemplateInfo($theme);
    }
    ksort($stack);

    foreach ($stack as $theme => $info) {
        $i++;

        /* Sorry, but we don't display engines */
        if ( strtolower($info['engine']) == 'yes' ) {
            continue;
        }


        if (file_exists($serendipity['serendipityPath'] . $serendipity['templatePath'] . $theme . '/preview.png')) {
            $preview = '<img src="' . $serendipity['templatePath'] . $theme . '/preview.png" width="100" style="border: 1px #000000 solid" />';
        } elseif (!empty($info['previewURL'])) {
            $preview = '<img src="' . $info['previewURL'] . '" width="100" style="border: 1px #000000 solid" />';        
        } else {
            $preview = '&nbsp;';
        }
        
        if (empty($info['customURI'])) {
            $info['customURI'] = '';
        }

        $unmetRequirements = array();
        if ( isset($info['require serendipity']) && version_compare($info['require serendipity'], serendipity_getCoreVersion($serendipity['version']), '>') ) {
            $unmetRequirements[] = 'Serendipity '. $info['require serendipity'];
        }

        /* TODO: Smarty versioncheck */

        $class = (($i % 2) ? 'even' : 'uneven');

?>
<div class="serendipity_admin_list_item serendipity_admin_list_item_<?php echo $class ?>">
    <table width="100%" id="serendipity_theme_<?php echo $theme; ?>">
        <tr>
            <td colspan="2"><strong><?php echo $info['name']; ?></strong></td>
            <td valign="middle" align="center" width="70" rowspan="2">
<?php
    if ( $serendipity['template'] != $theme ) {
        if ( !sizeof($unmetRequirements) ) {
?>
            <a href="?serendipity[adminModule]=templates&amp;serendipity[adminAction]=install&amp;serendipity[theme]=<?php echo $theme . $info['customURI']; ?>"><img src="<?php echo serendipity_getTemplateFile('admin/img/install_now' . $info['customIcon'] . '.png') ?>" alt="<?php echo SET_AS_TEMPLATE ?>" title="<?php echo SET_AS_TEMPLATE ?>" border="0" /></a>
<?php   } else { ?>
        <span style="color: #cccccc"><?php echo sprintf(UNMET_REQUIREMENTS, implode(', ', $unmetRequirements)); ?></span>
<?php
        }
    } ?>
            </td>
        </tr>

        <tr>
            <td width="100" style="padding-left: 10px"><?php echo $preview; ?></td>
            <td valign="top">
                <?php echo AUTHOR;       ?>: <?php echo $info['author'];?><br />
                <?php echo LAST_UPDATED; ?>: <?php echo $info['date'];  ?>
            </td>
        </tr>
    </table>
</div>
<?php
    }
?>
<?php
/* vim: set sts=4 ts=4 expandtab : */
?>
