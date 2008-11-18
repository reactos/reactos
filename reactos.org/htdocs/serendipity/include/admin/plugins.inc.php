<?php # $Id: plugins.inc.php 718 2005-11-21 09:56:37Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (IN_serendipity !== true) {
    die ('Don\'t hack!');
}

if (!serendipity_checkPermission('adminPlugins')) {
    return;
}

include_once S9Y_INCLUDE_PATH . 'include/plugin_api.inc.php';
include_once S9Y_INCLUDE_PATH . 'include/plugin_internal.inc.php';
include_once S9Y_INCLUDE_PATH . 'include/functions_entries_admin.inc.php';

function serendipity_groupname($group) {
    if (defined('PLUGIN_GROUP_' . $group)) {
        return constant('PLUGIN_GROUP_' . $group);
    } else {
        return $group;
    }
}

function serendipity_pluginListSort($x, $y) {
    return strnatcasecmp($x['name'] . ' - ' . $x['description'], $y['name'] . ' - ' . $y['description']);
}

function show_plugins($event_only = false)
{
    global $serendipity;
?>
    <form action="?serendipity[adminModule]=plugins" method="post">
        <?php echo serendipity_setFormToken(); ?>
        <table border="0" cellpadding="5" cellspacing="0" width="100%">
            <tr>
                <td colspan="2">&nbsp;</td>
                <td><strong><?php echo TITLE; ?></strong></td>
                <td><strong><?php echo PERMISSIONS; ?></strong></td>
<?php
    if (!$event_only) {
?>
                <td colspan="3" align="center"><strong><?php echo PLACEMENT; ?></strong></td>
<?php
    } else {
?>
                <td colspan="2">&nbsp;</dd>
<?php } ?>
            </tr>
<?php
    $sort_order = 0;
    $errors     = array();

    /* Block display the plugins per placement location. */
    if ($event_only) {
        $plugin_placements = array('event');
    } else {
        $plugin_placements = array('left', 'right', 'hide');
    }

    foreach ($plugin_placements as $plugin_placement) {
        $plugins = serendipity_plugin_api::enum_plugins($plugin_placement);

        if (!is_array($plugins)) {
            continue;
        }

        $sort_idx = 0;
        foreach ($plugins as $plugin_data) {
            $plugin =& serendipity_plugin_api::load_plugin($plugin_data['name'], $plugin_data['authorid']);
            $key    = urlencode($plugin_data['name']);
            $is_plugin_owner    = ($plugin_data['authorid'] == $serendipity['authorid'] || serendipity_checkPermission('adminPluginsMaintainOthers'));
            $is_plugin_editable = ($is_plugin_owner || $plugin_data['authorid'] == '0');

            if (!is_object($plugin)) {
                $name = $title = ERROR . '!';
                $desc          = ERROR . ': ' . $plugin_data['name'];
                $can_configure = false;
            } else {
                /* query for its name, description and configuration data */

                $bag = new serendipity_property_bag;
                $plugin->introspect($bag);

                $name  = htmlspecialchars($bag->get('name'));
                $desc  = htmlspecialchars($bag->get('description'));
                $desc .= '<br />' . VERSION  . ': <em>' . $bag->get('version') . '</em>';

                $title = serendipity_plugin_api::get_plugin_title($plugin, '[' . $name . ']');

                if ($bag->is_set('configuration') && ($plugin->protected === FALSE || $plugin_data['authorid'] == '0' || $plugin_data['authorid'] == $serendipity['authorid'] || serendipity_checkPermission('adminPluginsMaintainOthers'))) {
                    $can_configure = true;
                } else {
                    $can_configure = false;
                }
            }

            if ($event_only) {
                $place = '<input type="hidden" name="serendipity[placement][' . $plugin_data['name'] . ']" value="event" />';
                $event_only_uri = '&amp;serendipity[event_plugin]=true';
            } else {
                $place = placement_box('serendipity[placement][' . $plugin_data['name'] . ']', $plugin_data['placement'], $is_plugin_editable);
                $event_only_uri = '';
            }

            /* Only display UP/DOWN links if there's somewhere for the plugin to go */
            if ($sort_idx == 0) {
                $moveup   = '&nbsp;';
            } else {
                $moveup   = '<a href="?' . serendipity_setFormToken('url') . '&amp;serendipity[adminModule]=plugins&amp;submit=move+up&amp;serendipity[plugin_to_move]=' . $key . $event_only_uri . '" style="border: 0"><img src="' . serendipity_getTemplateFile('admin/img/uparrow.png') .'" height="16" width="16" border="0" alt="' . UP . '" /></a>';
            }

            if ($sort_idx == (count($plugins)-1)) {
                $movedown = '&nbsp;';
            } else {
                $movedown = ($moveup != '' ? '&nbsp;' : '') . '<a href="?' . serendipity_setFormToken('url') . '&amp;serendipity[adminModule]=plugins&amp;submit=move+down&serendipity[plugin_to_move]=' . $key . $event_only_uri . '" style="border: 0"><img src="' . serendipity_getTemplateFile('admin/img/downarrow.png') . '" height="16" width="16" alt="'. DOWN .'" border="0" /></a>';
            }
?>
            <tr>
                <td style="border-bottom: 1px solid #000000">
                    <div>
                    <?php if ($is_plugin_editable) { ?>
                        <input type="checkbox" name="serendipity[plugin_to_remove][]" value="<?php echo $plugin_data['name']; ?>" />
                    <?php } else { ?>
                        &nbsp;
                    <?php } ?>
                    </div>
                </td>
                <td style="border-bottom: 1px solid #000000" width="16">
                <?php if ( $can_configure ) { ?>
                    <a href="?serendipity[adminModule]=plugins&amp;serendipity[plugin_to_conf]=<?php echo $key ?>"><img src="<?php echo serendipity_getTemplateFile('admin/img/configure.png') ?>" style="border: 0; vertical-align: bottom;"></a>
                <?php } else { ?>
                    &nbsp;
                <?php } ?>
                </td>
                <td style="border-bottom: 1px solid #000000"><strong>
                <?php if ( $can_configure ) { ?>
                    <a href="?serendipity[adminModule]=plugins&amp;serendipity[plugin_to_conf]=<?php echo $key ?>"><?php echo $title; ?></a>
                <?php } else { ?>
                    <?php echo $title; ?>
                <?php } ?></strong><br />
                    <div style="font-size: 8pt"><?php echo $desc; ?></div>
                </td>
                <td style="border-bottom: 1px solid #000000" nowrap="nowrap"><?php ownership($plugin_data['authorid'], $plugin_data['name'], $is_plugin_owner); ?></td>
            <?php if ( !$event_only ) { ?>
                <td style="border-bottom: 1px solid #000000" nowrap="nowrap"><?php echo $place ?></td>
            <?php } ?>
                <td style="border-bottom: 1px solid #000000"><?php echo $moveup ?></td>
                <td style="border-bottom: 1px solid #000000"><?php echo $movedown ?></td>
            </tr>
<?php
            $sort_idx++;
        }
    }
?>
        </table>
        <br />
        <div>
            <input type="submit" name="REMOVE" title="<?php echo REMOVE_TICKED_PLUGINS; ?>"  value="<?php echo DELETE; ?>" class="serendipityPrettyButton" />
            <input type="submit" name="SAVE"   title="<?php echo SAVE_CHANGES_TO_LAYOUT; ?>" value="<?php echo SAVE; ?>" class="serendipityPrettyButton" />
        </div>
</form>
<?php
}

function ownership($authorid, $name, $is_plugin_owner = false) {
    global $serendipity;

    static $users = array();
    if (empty($users)) {
        $users = serendipity_fetchUsers();
    }

    if ($is_plugin_owner) {
?>
<select name="serendipity[ownership][<?php echo $name; ?>]">
    <option value="0"><?php echo ALL_AUTHORS; ?></option>
<?php
    }

    foreach($users AS $user) {
        if (!$is_plugin_owner && $user['authorid'] == $authorid) {
            $realname = htmlspecialchars($user['realname']);
        } elseif ($is_plugin_owner) {
?>
    <option value="<?php echo $user['authorid']; ?>"<?php echo ($user['authorid'] == $authorid ? ' selected="selected"' : ''); ?>><?php echo htmlspecialchars($user['realname']); ?></option>
<?php
        }
    }

    if ($is_plugin_owner) {
?>
</select>
<?php
    } else {
        echo (empty($realname) ? ALL_AUTHORS : $realname);
    }
}

function placement_box($name, $val, $is_plugin_editable = false)
{
    static $opts = array(
                    'left'  => LEFT,
                    'right' => RIGHT,
                    'hide'  => HIDDEN
        );

    $x = "\n<select name=\"$name\">\n";
    foreach ($opts as $k => $v) {
        if (!$is_plugin_editable && $k == 'hide') {
            continue;
        }

        $x .= "    <option value=\"$k\"" . ($k == $val ? ' selected="selected"' : '') . ">$v</option>\n";
    }
    return $x . "</select>\n";
}

if (isset($_GET['serendipity']['plugin_to_move']) && isset($_GET['submit']) && serendipity_checkFormToken()) {
    if (isset($_GET['serendipity']['event_plugin'])) {
        $plugins = serendipity_plugin_api::enum_plugins('event', false);
    } else {
        $plugins = serendipity_plugin_api::enum_plugins('event', true);
    }

    /* Renumber the sort order to be certain that one actually exists
        Also look for the one we're going to move */
    $idx_to_move = -1;
    for($idx = 0; $idx < count($plugins); $idx++) {
        $plugins[$idx]['sort_order'] = $idx;

        if ($plugins[$idx]['name'] == $_GET['serendipity']['plugin_to_move']) {
            $idx_to_move = $idx;
        }
    }

    /* If idx_to_move is still -1 then we never found it (shouldn't happen under normal conditions)
        Also make sure the swaping idx is around */
    if ($idx_to_move >= 0 && (($_GET['submit'] == 'move down' && $idx_to_move < (count($plugins)-1)) || ($_GET['submit'] == 'move up' && $idx_to_move > 0))) {

        /* Swap the one were moving with the one that's in the spot we're moving to */
        $tmp = $plugins[$idx_to_move]['sort_order'];

        $plugins[$idx_to_move]['sort_order'] = (int)$plugins[$idx_to_move + ($_GET['submit'] == 'move down' ? 1 : -1)]['sort_order'];
        $plugins[$idx_to_move + ($_GET['submit'] == 'move down' ? 1 : -1)]['sort_order'] = (int)$tmp;

        /* Update table */
        foreach($plugins as $plugin) {
            $key = serendipity_db_escape_string($plugin['name']);
            serendipity_db_query("UPDATE {$serendipity['dbPrefix']}plugins SET sort_order = {$plugin['sort_order']} WHERE name='$key'");
        }
    }

    /* TODO: Moving The first Right oriented plugin up,
            or the last left oriented plugin down
            should not be displayed to the user as an option.
            It's a behavior which really has no meaning. */
}

if (isset($_GET['serendipity']['plugin_to_conf'])) {
    /* configure a specific instance */
    $plugin =& serendipity_plugin_api::load_plugin($_GET['serendipity']['plugin_to_conf']);

    if (!($plugin->protected === FALSE || $plugin->serendipity_owner == '0' || $plugin->serendipity_owner == $serendipity['authorid'] || serendipity_checkPermission('adminPluginsMaintainOthers'))) {
        return;
    }

    $bag  = new serendipity_property_bag;
    $plugin->introspect($bag);
    $name = htmlspecialchars($bag->get('name'));
    $desc = htmlspecialchars($bag->get('description'));

    $config_names = $bag->get('configuration');

    if (isset($_POST['SAVECONF']) && serendipity_checkFormToken()) {
        /* enum properties and set their values */

        $save_errors = array();
        foreach ($config_names as $config_item) {
            $cbag = new serendipity_property_bag;
            if ($plugin->introspect_config_item($config_item, $cbag)) {
                $value    = $_POST['serendipity']['plugin'][$config_item];

                $validate = $plugin->validate($config_item, $cbag, $value);

                if ($validate === true) {
//                    echo $config_item . " validated: $validate<br />\n";
                    $plugin->set_config($config_item, $value);
                } else {
                    $save_errors[] = $validate;
                }
            }
        }

        $plugin->cleanup();
    }
?>

<?php if ( isset($save_errors) && is_array($save_errors) && count($save_errors) > 0 ) { ?>
    <div class="serendipityAdminMsgError">
    <?php
    echo ERROR . ":<br />\n";
    echo "<ul>\n";
    foreach($save_errors AS $save_error) {
        echo '<li>' . $save_error . "</li>\n";
    }
    echo "</ul>\n";
    ?>
    </div>
<?php } elseif ( isset($_POST['SAVECONF']) ) { ?>
    <div class="serendipityAdminMsgSuccess"><?php echo DONE .': '. sprintf(SETTINGS_SAVED_AT, serendipity_strftime('%H:%M:%S')); ?></div>
<?php } ?>

<form method="post" name="serendipityPluginConfigure">
    <?php echo serendipity_setFormToken(); ?>
    <table cellpadding="5" style="border: 1px dashed" width="90%" align="center">
        <tr>
            <th width="100"><?php echo NAME; ?></th>
            <td><?php echo $name; ?></td>
        </tr>

        <tr>
            <th><?php echo DESCRIPTION; ?></th>
            <td><?php echo $desc; ?></td>
        </tr>
    </table>
<br />
    <table border="0" cellspacing="0" cellpadding="3" width="100%">
<?php
    $elcount = 0;
    $htmlnugget = array();
    foreach ($config_names as $config_item) {
        $elcount++;
        $cbag = new serendipity_property_bag;
        $plugin->introspect_config_item($config_item, $cbag);

        $cname      = htmlspecialchars($cbag->get('name'));
        $cdesc      = htmlspecialchars($cbag->get('description'));
        $value      = $plugin->get_config($config_item, 'unset');
        $lang_direction = htmlspecialchars($cbag->get('lang_direction'));

        if (empty($lang_direction)) {
            $lang_direction = LANG_DIRECTION;
        }

        /* Apparently no value was set for this config item */
        if ($value === 'unset') {
            /* Try and the default value for the config item */
            $value = $cbag->get('default');

            /* Still, we don't have a value, try and get (bool)false - from an old plugin */
            if ($value === '') {
                $value = $plugin->get_config($config_item, false, true);
            }
        }
        $hvalue   = (isset($_POST['serendipity']['plugin'][$config_item]) ? htmlspecialchars($_POST['serendipity']['plugin'][$config_item]) : htmlspecialchars($value));
        $radio    = array();
        $select   = array();
        $per_row  = null;

        switch ($cbag->get('type')) {
            case 'seperator':
?>
        <tr>
            <td colspan="2"><hr noshade="noshade" size="1" /></td>
        </tr>
<?php
                break;

            case 'select':
                $select = $cbag->get('select_values');
?>
        <tr>
            <td style="border-bottom: 1px solid #000000; vertical-align: top"><strong><?php echo $cname; ?></strong>
<?php
    if ($cdesc != '') {
?>
                <br><span  style="color: #5E7A94; font-size: 8pt;">&nbsp;<?php echo $cdesc; ?></span>
<?php } ?>
            </td>
            <td style="border-bottom: 1px solid #000000; vertical-align: middle" width="250">
                <div>
                    <select class="direction_<?php echo $lang_direction; ?>" name="serendipity[plugin][<?php echo $config_item; ?>]">
<?php
                foreach($select AS $select_value => $select_desc) {
                    $id = htmlspecialchars($config_item . $select_value);
?>
                        <option value="<?php echo $select_value; ?>" <?php echo ($select_value == $hvalue ? 'selected="selected"' : ''); ?> title="<?php echo htmlspecialchars($select_desc); ?>">
                            <?php echo htmlspecialchars($select_desc); ?>
                        </option>
<?php
                }
?>
                    </select>
                </div>
            </td>
        </tr>
<?php
                break;

            case 'tristate':
                $per_row = 3;
                $radio['value'][] = 'default';
                $radio['desc'][]  = USE_DEFAULT;

            case 'boolean':
                $radio['value'][] = 'true';
                $radio['desc'][]  = YES;

                $radio['value'][] = 'false';
                $radio['desc'][]  = NO;

           case 'radio':
                if (!count($radio) > 0) {
                    $radio = $cbag->get('radio');
                }

                if (empty($per_row)) {
                    $per_row = $cbag->get('radio_per_row');
                    if (empty($per_row)) {
                        $per_row = 2;
                    }
                }
?>
        <tr>
            <td style="border-bottom: 1px solid #000000; vertical-align: top"><strong><?php echo $cname; ?></strong>
<?php
                if ($cdesc != '') {
?>
                <br /><span  style="color: #5E7A94; font-size: 8pt;">&nbsp;<?php echo $cdesc; ?></span>
<?php
                }
?>
            </td>
            <td style="border-bottom: 1px solid #000000; vertical-align: middle;" width="250">
<?php
                $counter = 0;
                foreach($radio['value'] AS $radio_index => $radio_value) {
                    $id = htmlspecialchars($config_item . $radio_value);
                    $counter++;
                    $checked = "";

                    if ($radio_value == 'true' && ($hvalue === '1' || $hvalue === 'true')) {
                        $checked = " checked";
                    } elseif ($radio_value == 'false' && ($hvalue === '' || $hvalue === 'false')) {
                        $checked = " checked";
                    } elseif ($radio_value == $hvalue) {
                        $checked = " checked";
                    }

                    if ($counter == 1) {
?>
                <div>
<?php
                    }
?>
                    <input class="direction_<?php echo $lang_direction; ?>" type="radio" id="serendipity_plugin_<?php echo $id; ?>" name="serendipity[plugin][<?php echo $config_item; ?>]" value="<?php echo $radio_value; ?>" <?php echo $checked ?> title="<?php echo htmlspecialchars($radio['desc'][$radio_index]); ?>" />
                        <label for="serendipity_plugin_<?php echo $id; ?>"><?php echo htmlspecialchars($radio['desc'][$radio_index]); ?></label>
<?php
                    if ($counter == $per_row) {
                        $counter = 0;
?>
                </div>
<?php
                    }
                }
?>
            </td>
        </tr>
<?php
                break;

            case 'string':
?>
        <tr>
            <td style="border-bottom: 1px solid #000000">
                    <strong><?php echo $cname; ?></strong>
                    <br><span style="color: #5E7A94; font-size: 8pt;">&nbsp;<?php echo $cdesc; ?></span>
            </td>
            <td style="border-bottom: 1px solid #000000" width="250">
                <div>
                    <input class="direction_<?php echo $lang_direction; ?>" type="text" name="serendipity[plugin][<?php echo $config_item; ?>]" value="<?php echo $hvalue; ?>" size="30" />
                </div>
            </td>
        </tr>
<?php
                break;

            case 'html':
            case 'text':
?>
        <tr>
            <td colspan="2"><strong><?php echo $cname; ?></strong>
                &nbsp;<span style="color: #5E7A94; font-size: 8pt;">&nbsp;<?php echo $cdesc; ?></span>
            </td>
        </tr>

        <tr>
            <td colspan="2">
                <div>
                    <textarea class="direction_<?php echo $lang_direction; ?>" style="width: 100%" id="nuggets<?php echo $elcount; ?>" name="serendipity[plugin][<?php echo $config_item; ?>]" rows="20" cols="80"><?php echo $hvalue; ?></textarea>
                </div>
            </td>
        </tr>
<?php
                if ($cbag->get('type') == 'html') {
                    $htmlnugget[] = $elcount;
                    serendipity_emit_htmlarea_code('nuggets', 'nuggets', true);
                }
                break;

            case 'content':
                ?><tr><td colspan="2"><?php echo $cbag->get('default'); ?></td></tr><?php
                break;

            case 'hidden':
                ?><tr><td colspan="2"><input class="direction_<?php echo $lang_direction; ?>" type="hidden" name="serendipity[plugin][<?php echo $config_item; ?>]" value="<?php echo $cbag->get('value'); ?>" /></td></tr><?php
                break;
        }
    }
?>
    </table>
<br />
    <div style="padding-left: 20px">
        <input type="submit" name="SAVECONF" value="<?php echo SAVE; ?>" class="serendipityPrettyButton" />
    </div>
<?php
    if (method_exists($plugin, 'example') ) {
?>
    <div>
        <?php echo $plugin->example() ?>
    </div>
<?php
    }

    if (isset($serendipity['wysiwyg']) && $serendipity['wysiwyg'] && count($htmlnugget) > 0) {
        $ev = array('nuggets' => $htmlnugget, 'skip_nuggets' => false);
        serendipity_plugin_api::hook_event('backend_wysiwyg_nuggets', $ev);

        if ($ev['skip_nuggets'] === false) {
?>
    <script type="text/javascript">
    function Spawnnugget() {
        <?php foreach($htmlnugget AS $htmlnuggetid) { ?>
        Spawnnuggets('<?php echo $htmlnuggetid; ?>');
        <?php } ?>
    }
    </script>
<?php
        }
    }
?>
    </div>
</form>
<?php

} elseif ( $serendipity['GET']['adminAction'] == 'addnew' ) {
?>
<?php if ( $serendipity['GET']['type'] == 'event' ) { ?>
    <h2><?php echo EVENT_PLUGINS ?></h2>
<?php } else { ?>
    <h2><?php echo SIDEBAR_PLUGINS ?></h2>
<?php } ?>
<br />
<?php
    $foreignPlugins = $pluginstack = $errorstack = array();
    serendipity_plugin_api::hook_event('backend_plugins_fetchlist', $foreignPlugins);
    $pluginstack = array_merge((array)$foreignPlugins['pluginstack'], $pluginstack);
    $errorstack  = array_merge((array)$foreignPlugins['errorstack'], $errorstack);

    $plugins = serendipity_plugin_api::get_installed_plugins();
    $classes = serendipity_plugin_api::enum_plugin_classes(($serendipity['GET']['type'] == 'event'));
    usort($classes, 'serendipity_pluginListSort');

    $counter    = 0;
    foreach ($classes as $class_data) {
        $pluginFile =  serendipity_plugin_api::probePlugin($class_data['name'], $class_data['classname'], $class_data['pluginPath']);
        $plugin     =& serendipity_plugin_api::getPluginInfo($pluginFile, $class_data, $serendipity['GET']['type']);

        if (is_object($plugin)) {
            // Object is returned when a plugin could not be cached.
            $bag = new serendipity_property_bag;
            $plugin->introspect($bag);

            // If a foreign plugin is upgradable, keep the new version number.
            if (isset($foreignPlugins['pluginstack'][$class_data['name']]) && $foreignPlugins['pluginstack'][$class_data['name']]['upgradable']) {
                $class_data['upgrade_version'] = $foreignPlugins['pluginstack'][$class_data['name']]['upgrade_version'];
            }

            $props = serendipity_plugin_api::setPluginInfo($plugin, $pluginFile, $bag, $class_data, 'local', $foreignPlugins);

            $counter++;
        } elseif (is_array($plugin)) {
            // Array is returned if a plugin could be fetched from info cache
            $props = $plugin;
        } else {
            $props = false;
        }

        if (is_array($props)) {
            if (version_compare($props['version'], $props['upgrade_version'], '<')) {
                $props['upgradable']      = true;
                $props['customURI']      .= $foreignPlugins['baseURI'] . $foreignPlugins['upgradeURI'];
            }

            $props['installable']  = !($props['stackable'] === false && in_array($class_data['true_name'], $plugins));
            $props['requirements'] = unserialize($props['requirements']);

            $pluginstack[$class_data['true_name']] = $props;
        } else {
            // False is returned if a plugin could not be instantiated
            $errorstack[] = $class_data['true_name'];
        }
    }

    usort($pluginstack, 'serendipity_pluginListSort');
    $pluggroups     = array();
    $pluggroups[''] = array();
    foreach($pluginstack AS $plugname => $plugdata) {
        if (is_array($plugdata['groups'])) {
            foreach($plugdata['groups'] AS $group) {
                $pluggroups[$group][] = $plugdata;
            }
        } else {
            $pluggroups[''][] = $plugdata;
        }
    }
    ksort($pluggroups);

    foreach($errorstack as $e_idx => $e_name) {
        echo ERROR . ': ' . $e_name . '<br />';
    }
?>
<table cellspacing="0" cellpadding="0" border="0" width="100%">
<?php
    $available_groups = array_keys($pluggroups);
    foreach($pluggroups AS $pluggroup => $groupstack) {
        if (empty($pluggroup)) {
        ?>
    <tr>
        <td colspan="2" class="serendipity_pluginlist_header">
            <form action="serendipity_admin.php" method="get">
                <?php echo serendipity_setFormToken(); ?>
                <input type="hidden" name="serendipity[adminModule]" value="plugins" />
                <input type="hidden" name="serendipity[adminAction]" value="addnew" />
                <input type="hidden" name="serendipity[type]" value="<?php echo htmlspecialchars($serendipity['GET']['type']); ?>" />
                <?php echo FILTERS; ?>: <select name="serendipity[only_group]">
            <?php foreach((array)$available_groups AS $available_group) {
                    ?>
                    <option value="<?php echo $available_group; ?>" <?php echo ($serendipity['GET']['only_group'] == $available_group ? 'selected="selected"' : ''); ?>><?php echo serendipity_groupname($available_group); ?>
            <?php } ?>
                </select>
                <input type="submit" value="<?php echo GO; ?>" />
            </form>
        </td>
    </tr>
        <?php
            if (!empty($serendipity['GET']['only_group'])) {
                continue;
            }
        } elseif (!empty($serendipity['GET']['only_group']) && $pluggroup != $serendipity['GET']['only_group']) {
            continue;
        } else {
        ?>
    <tr>
        <td colspan="2" class="serendipity_pluginlist_section"><strong><?php echo serendipity_groupname($pluggroup); ?></strong></td>
    </tr>
        <?php
        }
?>
    <tr>
        <td><strong>Plugin</strong></td>
        <td width="100" align="center"><strong>Action</strong></td>
    </tr>
<?php
        foreach ($groupstack as $plug) {
            $jsLine = " onmouseout=\"document.getElementById('serendipity_plugin_". $plug['class_name'] ."').className='';\"";
            $jsLine .= " onmouseover=\"document.getElementById('serendipity_plugin_". $plug['class_name'] ."').className='serendipity_admin_list_item_uneven';\"";

            $pluginInfo = $notice = array();
            if (!empty($plug['author'])) {
                $pluginInfo[] = AUTHOR  . ': ' . $plug['author'];
            }

            if (!empty($plug['version'])) {
                $pluginInfo[] = VERSION  . ': ' . $plug['version'];
            }

            if (!empty($plug['upgrade_version']) && $plug['upgrade_version'] != $plug['version']) {
                $pluginInfo[] = sprintf(UPGRADE_TO_VERSION, $plug['upgrade_version']);
            }

            if (!empty($plug['pluginlocation']) && $plug['pluginlocation'] != 'local') {
                $pluginInfo[] = '(' . htmlspecialchars($plug['pluginlocation']) . ')';
                $installimage = serendipity_getTemplateFile('admin/img/install_now_' . strtolower($plug['pluginlocation']) . '.png');
            } else {
                $installimage = serendipity_getTemplateFile('admin/img/install_now.png');
            }

            if (!isset($plug['customURI'])) {
                $plug['customURI'] = '';
            }

            if ( !empty($plug['requirements']['serendipity']) && version_compare($plug['requirements']['serendipity'], serendipity_getCoreVersion($serendipity['version']), '>') ) {
                $notice['requirements_failures'][] = 's9y ' . $plug['requirements']['serendipity'];
            }

            if ( !empty($plug['requirements']['php']) && version_compare($plug['requirements']['php'], phpversion(), '>') ) {
                $notice['requirements_failures'][] = 'PHP ' . $plug['requirements']['php'];
            }

            /* Enable after Smarty 2.6.7 upgrade.
             * TODO: How can we get current Smarty version here? $smarty is not created!
            if ( !empty($plug['requirements']['smarty']) && version_compare($plug['requirements']['smarty'], '2.6.7', '>') ) {
                $notice['requirements_failures'][] = 'Smarty: ' . $plug['requirements']['smarty'];
            }
            */

            if (count($notice['requirements_failures']) > 0) {
                $plug['requirements_fail'] = true;
            }

?>
    <tr id="serendipity_plugin_<?php echo $plug['class_name']; ?>">
        <td colspan="2" <?php echo $jsLine ?>>
            <table width="100%" cellpadding="3" cellspacing="0" border="0">
                <tr>
                    <td><strong><?php echo $plug['name'] ?></strong></td>
                    <td width="100" align="center" valign="middle" rowspan="3">
                        <?php if ( $plug['requirements_fail'] == true ) { ?>
                            <span style="color: #cccccc"><?php printf(UNMET_REQUIREMENTS, implode(', ', $notice['requirements_failures'])); ?></span>
                        <?php } elseif ( $plug['upgradable'] == true ) { ?>
                            <a href="?serendipity[adminModule]=plugins&amp;serendipity[pluginPath]=<?php echo $plug['pluginPath']; ?>&amp;serendipity[install_plugin]=<?php echo $plug['plugin_class'] . $plug['customURI'] ?>"><img src="<?php echo serendipity_getTemplateFile('admin/img/upgrade_now.png') ?>" title="<?php echo UPGRADE ?>" alt="<?php echo UPGRADE ?>" border="0"></a>
                        <?php } elseif ($plug['installable'] == true) { ?>
                            <a href="?serendipity[adminModule]=plugins&amp;serendipity[pluginPath]=<?php echo $plug['pluginPath']; ?>&amp;serendipity[install_plugin]=<?php echo $plug['plugin_class'] . $plug['customURI'] ?>"><img src="<?php echo $installimage ?>" title="<?php echo INSTALL ?>" alt="<?php echo INSTALL ?>" border="0"></a>
                        <?php } else { ?>
                            <span style="color: #cccccc"><?php echo ALREADY_INSTALLED ?></span>
                        <?php } ?>
                    </td>
                </tr>
                <tr>
                    <td style="padding-left: 10px"><?php echo $plug['description'] ?></td>
                </tr>
<?php       if (count($pluginInfo) > 0) { ?>
                <tr>
                    <td style="padding-left: 10px; font-size: x-small"><?php echo implode('; ', $pluginInfo); ?></td>
                </tr>
<?php       } ?>
            </table>
        </td>
    </tr>
<?php
        }
    }
?>
</table>

<?php
} else {
    /* show general plugin list */

    if (isset($_POST['SAVE']) && isset($_POST['serendipity']['placement']) && serendipity_checkFormToken()) {
        foreach ($_POST['serendipity']['placement'] as $plugin_name => $placement) {
            serendipity_plugin_api::update_plugin_placement(
                addslashes($plugin_name),
                addslashes($placement)
            );

            serendipity_plugin_api::update_plugin_owner(
                addslashes($plugin_name),
                addslashes($_POST['serendipity']['ownership'][$plugin_name])
            );
        }
    }

    if (isset($serendipity['GET']['install_plugin'])) {
        $authorid = $serendipity['authorid'];
        if (serendipity_checkPermission('adminPluginsMaintainOthers')) {
            $authorid = '0';
        }

        $fetchplugin_data = array('GET'     => &$serendipity['GET'],
                                  'install' => true);
        serendipity_plugin_api::hook_event('backend_plugins_fetchplugin', $fetchplugin_data);

        if ($fetchplugin_data['install']) {
            $serendipity['debug']['pluginload'] = array();
            $inst = serendipity_plugin_api::create_plugin_instance($serendipity['GET']['install_plugin'], null, (serendipity_plugin_api::is_event_plugin($serendipity['GET']['install_plugin']) ? 'event': 'right'), $authorid, serendipity_db_escape_string($serendipity['GET']['pluginPath']));

            /* Load the new plugin */
            $plugin = &serendipity_plugin_api::load_plugin($inst);
            if (!is_object($plugin)) {
                echo "DEBUG: Plugin $inst not an object: " . print_r($plugin, true) . ".<br />Input: " . print_r($serendipity['GET'], true) . ".<br /><br />\n\nPlease report this bug. This error can happen if a plugin was not properly downloaded (check your plugins directory if the requested plugin was downloaded) or the inclusion of a file failed (permissions?)<br />\n";
                echo "Backtrace:<br />\n" . implode("<br />\n", $serendipity['debug']['pluginload']) . "<br />";
            }
            $bag  = new serendipity_property_bag;
            $plugin->introspect($bag);

            if ($bag->is_set('configuration')) {
                /* Only play with the plugin if there is something to play with */
                echo '<script type="text/javascript">location.href = \'' . $serendipity['baseurl'] . '?serendipity[adminModule]=plugins&serendipity[plugin_to_conf]=' . $inst . '\';</script>';
                die();
            } else {
                /* If no config is available, redirect to plugin overview, because we do not want that a user can install the plugin a second time via accidental browser refresh */
                echo '<script type="text/javascript">location.href = \'' . $serendipity['baseurl'] . '?serendipity[adminModule]=plugins\';</script>';
                die();
            }
        }
    }

    if (isset($_POST['REMOVE']) && serendipity_checkFormToken()) {
        if (is_array($_POST['serendipity']['plugin_to_remove'])) {
            foreach ($_POST['serendipity']['plugin_to_remove'] as $key) {
                $plugin =& serendipity_plugin_api::load_plugin($key);

                if ($plugin->serendipity_owner == '0' || $plugin->serendipity_owner == $serendipity['authorid'] || serendipity_checkPermission('adminPluginsMaintainOthers')) {
                    serendipity_plugin_api::remove_plugin_instance($key);
                }
            }
        }
    }
?>
    <?php echo BELOW_IS_A_LIST_OF_INSTALLED_PLUGINS ?>
    <br />
    <br />

    <h3><?php echo SIDEBAR_PLUGINS ?></h3>
    <a href="?serendipity[adminModule]=plugins&amp;serendipity[adminAction]=addnew" class="serendipityIconLink"><img src="<?php echo serendipity_getTemplateFile('admin/img/install.png') ?>" style="border: 0px none ; vertical-align: middle; display: inline;" alt=""><?php echo sprintf(CLICK_HERE_TO_INSTALL_PLUGIN, SIDEBAR_PLUGIN) ?></a>
    <?php serendipity_plugin_api::hook_event('backend_plugins_sidebar_header', $serendipity); ?>
    <?php show_plugins(false); ?>

    <br />
    <br />

    <h3><?php echo EVENT_PLUGINS ?></h3>
    <a href="?serendipity[adminModule]=plugins&amp;serendipity[adminAction]=addnew&amp;serendipity[type]=event" class="serendipityIconLink"><img src="<?php echo serendipity_getTemplateFile('admin/img/install.png') ?>" style="border: 0px none ; vertical-align: middle; display: inline;" alt=""><?php echo sprintf(CLICK_HERE_TO_INSTALL_PLUGIN, EVENT_PLUGIN) ?></a>
    <?php serendipity_plugin_api::hook_event('backend_plugins_event_header', $serendipity); ?>
    <?php show_plugins(true); ?>

<?php
}
/* vim: set sts=4 ts=4 expandtab : */
