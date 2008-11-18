<?php # $Id: serendipity_event_entryproperties.php 598 2005-10-25 09:50:43Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_EVENT_ENTRYPROPERTIES_TITLE', 'Extended properties for entries');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_DESC', '(cache, non-public articles, sticky posts)');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_STICKYPOSTS', 'Mark this entry as a Sticky Post');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS', 'Entries can be read by');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PRIVATE', 'Myself');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_MEMBERS', 'Co-authors');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PUBLIC', 'Everyone');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE', 'Allow to cache entries?');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DESC', 'If enabled, a cached version will be generated on every saving. Caching will increase performance, but maybe decrease flexibility for other plugins.');
@define('PLUGIN_EVENT_ENTRYPROPERTY_BUILDCACHE', 'Build cached entries');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNEXT', 'Fetching next batch of entries...');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNO', 'Fetching entries %d to %d');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_BUILDING', 'Building cache for entry #%d, <em>%s</em>...');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHED', 'Entry cached.');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DONE', 'Entry caching completed.');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_ABORTED', 'Entry caching ABORTED.');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_TOTAL', ' (totalling %d entries)...');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_NL2BR', 'Disable nl2br');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_NO_FRONTPAGE', 'Hide from article overview / frontpage');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_GROUPS', 'Use group-based restrictions');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_GROUPS_DESC', 'If enabled, you can define which users of a usergroup may be able to read entries. This option has a large impact on the performance of your article overview. Only enable this if you are really going to use this feature.');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_USERS', 'Use user-based restrictions');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_USERS_DESC', 'If enabled, you can define which specific users may be able to read entries. This option has a large impact on the performance of your article overview. Only enable this if you are really going to use this feature.');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_HIDERSS', 'Hide content in RSS');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_HIDERSS_DESC', 'If enabled, the content of this entry will not be shown inside the RSS feed(s).');

@define('PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS', 'Custom Fields');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC1', 'Additional custom fields can be used in your template at places where you want them to show up. For that, edit your entries.tpl template file and place Smarty tags like {$entry.properties.ep_MyCustomField} in the HTML where you like. Note the prefix ep_ for each field. ');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC2', 'Here you can enter a list of commaseparated field names that you can use to enter for every entry - do not use special characters or spaces for those fieldnames. Example: "Customfield1, Customfield2". ' . PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC1);
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC3', 'The list of available custom fields can be changed in the <a href="%s" target="_blank" title="' . PLUGIN_EVENT_ENTRYPROPERTIES_TITLE . '">plugin configuration</a>.');

class serendipity_event_entryproperties extends serendipity_event
{
    var $services;
    var $title = PLUGIN_EVENT_ENTRYPROPERTIES_TITLE;

    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_EVENT_ENTRYPROPERTIES_TITLE);
        $propbag->add('description',   PLUGIN_EVENT_ENTRYPROPERTIES_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Garvin Hicking');
        $propbag->add('version',       '1.6');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('event_hooks',    array(
            'frontend_fetchentries'                             => true,
            'frontend_fetchentry'                               => true,
            'backend_publish'                                   => true,
            'backend_save'                                      => true,
            'backend_display'                                   => true,
            'backend_import_entry'                              => true,
            'entry_display'                                     => true,
            'frontend_entryproperties'                          => true,
            'backend_sidebar_entries_event_display_buildcache'  => true,
            'backend_sidebar_entries'                           => true,
            'backend_cache_entries'                             => true,
            'backend_cache_purge'                               => true,
            'backend_plugins_new_instance'                      => true,
            'frontend_entryproperties_query'                    => true,
            'frontend_entries_rss'                              => true
        ));
        $propbag->add('groups', array('BACKEND_EDITOR'));
        $propbag->add('configuration', array('cache', 'use_groups', 'use_users', 'default_read', 'customfields'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'default_read':
                $propbag->add('type',        'radio');
                $propbag->add('name',        USE_DEFAULT . ': ' . PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS);
                $propbag->add('description', '');
                $propbag->add('radio', array(
                    'value' => array('private', 'public', 'member'),
                    'desc'  => array(PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PRIVATE, PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PUBLIC, PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_MEMBERS)
                ));
                $propbag->add('default',     'public');
                $propbag->add('radio_per_row', '1');

                break;

            case 'customfields':
                $propbag->add('type',        'text');
                $propbag->add('name',        PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS);
                $propbag->add('description', PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC2);
                $propbag->add('default',     'CustomField1, CustomField2, CustomField3');
                break;

            case 'use_groups':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        PLUGIN_EVENT_ENTRYPROPERTIES_GROUPS);
                $propbag->add('description', PLUGIN_EVENT_ENTRYPROPERTIES_GROUPS_DESC);
                $propbag->add('default',     'false');
                break;

            case 'use_users':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        PLUGIN_EVENT_ENTRYPROPERTIES_USERS);
                $propbag->add('description', PLUGIN_EVENT_ENTRYPROPERTIES_USERS_DESC);
                $propbag->add('default',     'false');
                break;

            case 'cache':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        PLUGIN_EVENT_ENTRYPROPERTIES_CACHE);
                $propbag->add('description', PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DESC);
                $propbag->add('default',     'true');
                break;
        }
        return true;
    }

    function generate_content(&$title) {
        $title = $this->title;
    }

    function install() {
        serendipity_plugin_api::hook_event('backend_cache_entries', $this->title);
    }

    function uninstall() {
        serendipity_plugin_api::hook_event('backend_cache_purge', $this->title);
    }

    function &getValidAuthors() {
        global $serendipity;

        if (serendipity_checkPermission('adminUsersMaintainOthers')) {
            $users = serendipity_fetchUsers('');
        } elseif (serendipity_checkPermission('adminUsersMaintainSame')) {
            $users = serendipity_fetchUsers('', serendipity_getGroups($serendipity['authorid'], true));
        } else {
            $users = serendipity_fetchUsers($serendipity['authorid']);
        }
        
        return $users;
    }


    function updateCache(&$entry) {
        global $serendipity;

        serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}entryproperties WHERE entryid = " . (int)$entry['id'] . " AND property LIKE 'ep_cache_%'");
        serendipity_plugin_api::hook_event('frontend_display', $entry);
        serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}entryproperties (entryid, property, value) VALUES (" . (int)$entry['id'] . ", 'ep_cache_body', '" . serendipity_db_escape_string($entry['body']) . "')");
        serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}entryproperties (entryid, property, value) VALUES (" . (int)$entry['id'] . ", 'ep_cache_extended', '" . serendipity_db_escape_string($entry['extended']) . "')");
    }

    function getSupportedProperties() {
        static $supported_properties = null;
        
        if ($supported_properties === null) {
            $supported_properties = array('is_sticky', 'access', 'access_groups', 'access_users', 'cache_body', 'cache_extended', 'no_nl2br', 'no_frontpage', 'hiderss');

            $fields = explode(',', trim($this->get_config('customfields')));
            if (is_array($fields) && count($fields) > 0) {
                foreach($fields AS $field) {
                    $field = trim($field);
                    if (!empty($field)) {
                        $supported_properties[] = $field;
                    }
                }
            }
        }

        return $supported_properties;
    }

    function returnQueryCondition($is_cache) {
        $and = '';
        if (!$is_cache) {
            $and = " AND property NOT LIKE 'ep_cache_%' ";
        }
        
        return $and;
    }

    function addProperties(&$properties, &$eventData) {
        global $serendipity;
        // Get existing data
        $property = serendipity_fetchEntryProperties($eventData['id']);
        $supported_properties = serendipity_event_entryproperties::getSupportedProperties();

        foreach($supported_properties AS $prop_key) {
            $prop_val = (isset($properties[$prop_key]) ? $properties[$prop_key] : null);
            $prop_key = 'ep_' . $prop_key;
            
            if (is_array($prop_val)) {
                $prop_val = ";" . implode(';', $prop_val) . ";";
            }
            
            $q = "DELETE FROM {$serendipity['dbPrefix']}entryproperties WHERE entryid = " . (int)$eventData['id'] . " AND property = '" . serendipity_db_escape_string($prop_key) . "'";
            serendipity_db_query($q);

            if (!empty($prop_val)) {
                $q = "INSERT INTO {$serendipity['dbPrefix']}entryproperties (entryid, property, value) VALUES (" . (int)$eventData['id'] . ", '" . serendipity_db_escape_string($prop_key) . "', '" . serendipity_db_escape_string($prop_val) . "')";
                serendipity_db_query($q);
            }
        }
    }

    function event_hook($event, &$bag, &$eventData, $addData = null) {
        global $serendipity;

        $hooks = &$bag->get('event_hooks');
        $is_cache   = serendipity_db_bool($this->get_config('cache', 'true'));
        $use_groups = serendipity_db_bool($this->get_config('use_groups'));
        $use_users  = serendipity_db_bool($this->get_config('use_users'));

        if (isset($hooks[$event])) {
            switch($event) {
                case 'frontend_entryproperties_query':
                    $eventData['and'] = $this->returnQueryCondition($is_cache);
                    return true;
                    break;

                case 'backend_display':
                    $is_sticky =    (isset($eventData['properties']['ep_is_sticky']) && serendipity_db_bool($eventData['properties']['ep_is_sticky']))
                                 || (isset($serendipity['POST']['properties']['is_sticky']) && serendipity_db_bool($serendipity['POST']['properties']['is_sticky']))
                               ? 'checked="checked"'
                               : '';
                    $nl2br     =    (isset($eventData['properties']['ep_no_nl2br']) && serendipity_db_bool($eventData['properties']['ep_no_nl2br']))
                                 || (isset($serendipity['POST']['properties']['no_nl2br']) && serendipity_db_bool($serendipity['POST']['properties']['no_nl2br']))
                               ? 'checked="checked"'
                               : '';

                    $no_frontpage =    (isset($eventData['properties']['ep_no_frontpage']) && serendipity_db_bool($eventData['properties']['ep_no_frontpage']))
                                 || (isset($serendipity['POST']['properties']['no_frontpage']) && serendipity_db_bool($serendipity['POST']['properties']['no_frontpage']))
                               ? 'checked="checked"'
                               : '';

                    $hiderss      =     (isset($eventData['properties']['ep_hiderss']) && serendipity_db_bool($eventData['properties']['ep_hiderss']))
                                 || (isset($serendipity['POST']['properties']['hiderss']) && serendipity_db_bool($serendipity['POST']['properties']['hiderss']))
                               ? 'checked="checked"'
                               : '';

                    $access_values = array(
                        PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PRIVATE => 'private',
                        PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PUBLIC  => 'public',
                        PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_MEMBERS => 'member',
                    );

                    if (isset($eventData['properties']['ep_access'])) {
                        $access = $eventData['properties']['ep_access'];
                    } elseif (isset($serendipity['POST']['properties']['access'])) {
                        $access = $serendipity['POST']['properties']['access'];
                    } else {
                        $access = $this->get_config('default_read', 'public');
                    }
                    
                    if (isset($eventData['properties']['ep_access_groups'])) {
                        $access_groups = explode(';', $eventData['properties']['ep_access_groups']);
                    } elseif (isset($serendipity['POST']['properties']['access_groups'])) {
                        $access_groups = $serendipity['POST']['properties']['access_groups'];
                    } else {
                        $access_groups = array();
                    }

                    if (isset($eventData['properties']['ep_access_users'])) {
                        $access_users = explode(';', $eventData['properties']['ep_access_users']);
                    } elseif (isset($serendipity['POST']['properties']['access_users'])) {
                        $access_users = $serendipity['POST']['properties']['access_users'];
                    } else {
                        $access_users = array();
                    }
?>
                    <fieldset style="margin: 5px">
                        <legend><?php echo PLUGIN_EVENT_ENTRYPROPERTIES_TITLE; ?></legend>
                            <input type="checkbox" name="serendipity[properties][is_sticky]" id="properties_is_sticky" value="true" <?php echo $is_sticky; ?> />
                                <label title="<?php echo PLUGIN_EVENT_ENTRYPROPERTIES_STICKYPOSTS; ?>" for="properties_is_sticky">&nbsp;<?php echo PLUGIN_EVENT_ENTRYPROPERTIES_STICKYPOSTS; ?>&nbsp;&nbsp;</label><br />
                            <input type="checkbox" name="serendipity[properties][no_frontpage]" id="properties_no_frontpage" value="true" <?php echo $no_frontpage; ?> />
                                <label title="<?php echo PLUGIN_EVENT_ENTRYPROPERTIES_NO_FRONTPAGE; ?>" for="properties_no_frontpage">&nbsp;<?php echo PLUGIN_EVENT_ENTRYPROPERTIES_NO_FRONTPAGE; ?>&nbsp;&nbsp;</label><br />
<?php if (class_exists('serendipity_event_nl2br')){ ?>
                            <input type="checkbox" name="serendipity[properties][no_nl2br]" id="properties_no_nl2br" value="true" <?php echo $nl2br; ?> />
                                <label title="<?php echo PLUGIN_EVENT_ENTRYPROPERTIES_NL2BR; ?>" for="properties_no_nl2br">&nbsp;<?php echo PLUGIN_EVENT_ENTRYPROPERTIES_NL2BR; ?>&nbsp;&nbsp;</label><br />

<?php } ?>
                            <input type="checkbox" name="serendipity[properties][hiderss]" id="properties_hiderss" value="true" <?php echo $hiderss; ?> />
                                <label title="<?php echo PLUGIN_EVENT_ENTRYPROPERTIES_HIDERSS_DESC; ?>" for="properties_hiderss">&nbsp;<?php echo PLUGIN_EVENT_ENTRYPROPERTIES_HIDERSS; ?>&nbsp;&nbsp;</label><br />

                            <br />
                            <?php echo PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS; ?>:<br />
                            <div style="margin-left: 10px">
<?php
                    foreach($access_values AS $radio_title => $radio_value) {
?>
                            <input type="radio" name="serendipity[properties][access]" id="properties_access_<?php echo $radio_value; ?>" value="<?php echo $radio_value; ?>" <?php echo $radio_value == $access ? 'checked="checked"' : ''; ?> />
                                <label title="<?php echo $radio_title; ?>" for="properties_access_<?php echo $radio_value; ?>">&nbsp;<?php echo $radio_title; ?>&nbsp;&nbsp;</label>
<?php
                    }
                    
                    if ($use_groups) {
                        $my_groups = serendipity_getGroups($serendipity['authorid']);
?>
                            <br /><select onchange="document.getElementById('properties_access_member').checked = true;" style="margin-left: 5px" multiple="multiple" name="serendipity[properties][access_groups][]" size="4">
<?php
                        foreach($my_groups AS $group) {
?>
                                <option value="<?php echo $group['id']; ?>" <?php echo (in_array($group['id'], $access_groups) ? 'selected="selected"' : ''); ?>><?php echo htmlspecialchars($group['name']); ?></option>
<?php
                        }
                        echo '</select>';
                    }

                    if ($use_users) {
?>
                        <br /><select onchange="document.getElementById('properties_access_member').checked = true;" style="margin-left: 5px" multiple="multiple" name="serendipity[properties][access_users][]" size="4">
<?php
                        $users = serendipity_fetchUsers();
                        foreach($users AS $user) {
?>
                            <option value="<?php echo $user['authorid']; ?>" <?php echo (in_array($user['authorid'], $access_users) ? 'selected="selected"' : ''); ?>><?php echo htmlspecialchars($user['realname']); ?></option>
<?php
                        }
                        echo '</select>';
                    }
?>
                            </div>
                            <br />
                            <?php echo AUTHOR; ?>:<br />
                            <div style="margin-left: 10px">
                                <select name="serendipity[change_author]">
                                <?php
                                if (isset($serendipity['POST']['change_author'])) {
                                    $selected_user = $serendipity['POST']['change_author'];
                                } elseif (!empty($eventData['authorid'])) {
                                    $selected_user = $eventData['authorid'];
                                } else {
                                    $selected_user = $serendipity['authorid'];
                                }
                                
                                $avail_users =& $this->getValidAuthors();

                                foreach($avail_users AS $user) {
                                    echo '<option value="' . $user['authorid'] . '" ' . ($selected_user == $user['authorid'] ? ' selected="selected"' : '') . '>' . $user['realname'] . '</option>' . "\n";
                                }
                                ?>
                                </select>
                            </div>

                            <?php
                                $fields = trim($this->get_config('customfields'));
                                if (!empty($fields)) {
                                    $fields = explode(',', $fields);
                                }
                                if (is_array($fields) && count($fields) > 0) { ?>
                            <br />
                            <?php echo PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS; ?>:<br />
                            <em><?php echo PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC1 . '<br />' . sprintf(PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC3, 'serendipity_admin.php?serendipity[adminModule]=plugins&amp;serendipity[plugin_to_conf]=' . $this->instance); ?></em><br />
                            <div style="margin-left: 10px">
                                <table id="serendipity_customfields">
                            <?php
                                    foreach($fields AS $fieldname) {
                                        $fieldname = trim($fieldname);
    
                                        if (isset($serendipity['POST']['properties'][$fieldname])) {
                                            $value = $serendipity['POST']['properties'][$fieldname];
                                        } elseif (!empty($eventData['properties']['ep_' . $fieldname])) {
                                            $value = $eventData['properties']['ep_' . $fieldname];
                                        } else {
                                            $value = '';
                                        }
                            ?>
                                <tr>
                                    <td class="customfield_name"><strong><?php echo $fieldname; ?></strong></td>
                                    <td class="customfield_value"><textarea name="serendipity[properties][<?php echo htmlspecialchars($fieldname); ?>]"><?php echo htmlspecialchars($value); ?></textarea></td>
                                </tr>
                            <?php
                                    }
                            ?>
                                </table>
                            </div>
                            <?php
                                }
                            ?>
                    </fieldset>
<?php
                    return true;
                    break;

                case 'backend_sidebar_entries':
                    if ($is_cache) {
?>
                        <li><a href="?serendipity[adminModule]=event_display&amp;serendipity[adminAction]=buildcache"><?php echo PLUGIN_EVENT_ENTRYPROPERTY_BUILDCACHE; ?></a></li>
<?php
                    }
                    return true;
                    break;
                case 'backend_import_entry':
                    //TODO: (ph) Maybe handle caching?
                    if (is_array($addData) && !$addData['nl2br']){
                        $props = array();
                        $props['no_nl2br'] = 'true';
                        $this->addProperties($props, $eventData);
                    }
                break;
                case 'backend_sidebar_entries_event_display_buildcache':
                    if ($is_cache) {
                        $per_fetch = 25;
                        $page = (isset($serendipity['GET']['page']) ? $serendipity['GET']['page'] : 1);
                        $from = ($page-1)*$per_fetch;
                        $to   = ($page)*$per_fetch;
                        printf(PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNO, $from, $to);
                        $entries = serendipity_fetchEntries(
                            null,
                            true,
                            $per_fetch,
                            false,
                            false,
                            'timestamp DESC',
                            '',
                            true
                        );

                        $total = serendipity_getTotalEntries();
                        printf(PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_TOTAL . '<br />', $total);

                        if (is_array($entries)) {
                            foreach($entries AS $idx => $entry) {
                                printf(PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_BUILDING . '<br />', $entry['id'], htmlspecialchars($entry['title']));
                                $this->updateCache($entry);
                                echo PLUGIN_EVENT_ENTRYPROPERTIES_CACHED . '<br /><br />';
                            }
                        }

                        echo '<br />';

                        if ($to < $total) {
?>
                        <script type="text/javascript">
                            if (confirm("<?php echo htmlspecialchars(PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNEXT); ?>")) {
                                location.href = "?serendipity[adminModule]=event_display&serendipity[adminAction]=buildcache&serendipity[page]=<?php echo ($page+1); ?>";
                            } else {
                                alert("<?php echo htmlspecialchars(PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_ABORTED); ?>");
                            }
                        </script>
<?php
                        } else {
                            echo '<div class="serendipity_msg_notice">' . PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DONE . '</div>';
                        }
                    }
                    return true;
                    break;

                case 'backend_cache_entries':
                    if (!$is_cache) {
                        return true;
                    }

                    $entries = serendipity_fetchEntries(
                        null,
                        true,
                        $serendipity['fetchLimit'],
                        false,
                        false,
                        'timestamp DESC',
                        '',
                        true
                    );

                    foreach($entries AS $idx => $entry) {
                        $this->updateCache($entry);
                    }
                    return true;
                    break;

                case 'backend_cache_purge':
                    if (!$is_cache) {
                        return true;
                    }

                    serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}entryproperties WHERE property LIKE 'ep_cache_%'");
                    break;

                case 'backend_publish':
                case 'backend_save':
                    if (!isset($serendipity['POST']['properties']) || !is_array($serendipity['POST']['properties']) || !isset($eventData['id'])) {
                        return true;
                    }
                    
                    if (!empty($serendipity['POST']['change_author']) && $serendipity['POST']['change_author'] != $eventData['id']) {
                        // Check again if the POSTed value is an author that the current user has "access" to.
                        $avail_users =& $this->getValidAuthors();
                        $new_authorid = (int)$serendipity['POST']['change_author'];
                        foreach($avail_users AS $user) {
                            if ($new_authorid == $user['authorid']) {
                                serendipity_db_query("UPDATE {$serendipity['dbPrefix']}entries SET authorid = " . $new_authorid . " WHERE id = " . (int)$eventData['id']);
                            }
                        }
                    }

                    if ($is_cache) {
                        $serendipity['POST']['properties']['cache_body']     = $eventData['body'];
                        $serendipity['POST']['properties']['cache_extended'] = $eventData['extended'];
                    }
                    
                    if (is_array($serendipity['POST']['properties']['access_groups']) && $serendipity['POST']['properties']['access'] != 'member') {
                        unset($serendipity['POST']['properties']['access_groups']);
                    }

                    if (is_array($serendipity['POST']['properties']['access_users']) && $serendipity['POST']['properties']['access'] != 'member') {
                        unset($serendipity['POST']['properties']['access_users']);
                    }

                    $this->addProperties($serendipity['POST']['properties'], $eventData);

                    return true;
                    break;

                case 'frontend_entryproperties':
                    $and = $this->returnQueryCondition($is_cache);
                    $q = "SELECT entryid, property, value FROM {$serendipity['dbPrefix']}entryproperties WHERE entryid IN (" . implode(', ', array_keys($addData)) . ") $and";

                    $properties = serendipity_db_query($q);
                    if (!is_array($properties)) {
                        return true;
                    }

                    foreach($properties AS $idx => $row) {
                        $eventData[$addData[$row['entryid']]]['properties'][$row['property']] = $row['value'];
                    }
                    return true;
                    break;

                case 'entry_display':
                    // PH: This is done after Garvins suggestion to patchup $eventData in case an entry
                    //     is in the process of being created. This must be done for the extended properties
                    //     to be applied in the preview.
                    if (is_array($serendipity['POST']['properties']) && count($serendipity['POST']['properties']) > 0){
                        $parr = array();
                        $supported_properties = serendipity_event_entryproperties::getSupportedProperties();
                        foreach($supported_properties AS $prop_key) {
                            if (isset($serendipity['POST']['properties'][$prop_key]))
                                $parr['ep_' . $prop_key] = $serendipity['POST']['properties'][$prop_key];
                        }
                        $eventData[0]['properties'] = $parr;
                    }
                    break;

                case 'frontend_fetchentries':
                case 'frontend_fetchentry':
                    $joins = array();
                    $conds = array();
                    if ($_SESSION['serendipityAuthedUser'] === true) {
                        $conds[] = " (ep_access.property IS NULL OR ep_access.value = 'member' OR ep_access.value = 'public' OR (ep_access.value = 'private' AND e.authorid = " . (int)$serendipity['authorid'] . ")) ";

                        if ($use_groups) {
                            $mygroups  = serendipity_checkPermission(null, null, true);
                            $groupcond = array();
                            foreach((array)$mygroups AS $mygroup) {
                                $groupcond[] .= "ep_access_groups.value LIKE '%;$mygroup;%'";
                            }
                            if (count($groupcond) > 0) {
                                $conds[] = " (ep_access_groups.property IS NULL OR (ep_access.value = 'member' AND (" . implode(' OR ', $groupcond) . ")))";
                            }
                        }
                        
                        if ($use_users) {
                            $conds[] = " (ep_access_users.property IS NULL OR (ep_access.value = 'member' AND (ep_access_users.value LIKE '%;" . (int)$serendipity['authorid'] . ";%' OR e.authorid = " . (int)$serendipity['authorid'] . "))) ";
                        }
                    } else {
                        $conds[] = " (ep_access.property IS NULL OR ep_access.value = 'public')";
                    }

                    if (!isset($serendipity['GET']['category']) && !isset($serendipity['GET']['adminModule']) && $event == 'frontend_fetchentries') {
                        $conds[] = " (ep_no_frontpage.property IS NULL OR ep_no_frontpage.value != 'true') ";
                        $joins[] = " LEFT OUTER JOIN {$serendipity['dbPrefix']}entryproperties ep_no_frontpage
                                                  ON (e.id = ep_no_frontpage.entryid AND ep_no_frontpage.property = 'ep_no_frontpage')";
                    }

                    if (count($conds) > 0) {
                        $cond = implode(' AND ', $conds);
                        if (empty($eventData['and'])) {
                            $eventData['and'] = " WHERE $cond ";
                        } else {
                            $eventData['and'] .= " AND $cond ";
                        }
                    }

                    $conds = array();
                    if (!isset($addData['noSticky']) || $addData['noSticky'] !== true) {
                        $conds[] = 'ep_sticky.value AS orderkey,';
                    } else {
                        $conds[] = 'e.isdraft AS orderkey,';
                    }

                    if ($is_cache && (!isset($addData['noCache']) || !$addData['noCache'])) {
                        $conds[] = 'ep_cache_extended.value AS ep_cache_extended,';
                        $conds[] = 'ep_cache_body.value     AS ep_cache_body,';
                    }
                    
                    $cond = implode("\n", $conds);
                    if (empty($eventData['addkey'])) {
                        $eventData['addkey'] = $cond;
                    } else {
                        $eventData['addkey'] .= $cond;
                    }

                    if ($serendipity['dbType'] == 'postgres') {
                        // PostgreSQL is a bit weird here. Empty columns with NULL or "" content for 
                        // orderkey would get sorted on top when using DESC, and only after those 
                        // the "true" content would be inserted. Thus we order ASC in postgreSQL, 
                        // and silently wonder. Thanks to Nate Johnston for working this out!
                        $cond = 'orderkey ASC';
                    } else {
                        $cond = 'orderkey DESC';
                    }

                    if (empty($eventData['orderby'])) {
                        $eventData['orderby'] = $cond;
                    } else {
                        $eventData['orderby'] = $cond . ', ' . $eventData['orderby'];
                    }

                    if ($is_cache && (!isset($addData['noCache']) || !$addData['noCache'])) {
                        $joins[] = " LEFT OUTER JOIN {$serendipity['dbPrefix']}entryproperties ep_cache_extended
                                                  ON (e.id = ep_cache_extended.entryid AND ep_cache_extended.property = 'ep_cache_extended')";
                        $joins[] = " LEFT OUTER JOIN {$serendipity['dbPrefix']}entryproperties ep_cache_body
                                                  ON (e.id = ep_cache_body.entryid AND ep_cache_body.property = 'ep_cache_body')";
                    }
                    $joins[] = " LEFT OUTER JOIN {$serendipity['dbPrefix']}entryproperties ep_access
                                              ON (e.id = ep_access.entryid AND ep_access.property = 'ep_access')";
                    if ($use_groups) {
                        $joins[] = " LEFT OUTER JOIN {$serendipity['dbPrefix']}entryproperties ep_access_groups
                                                  ON (e.id = ep_access_groups.entryid AND ep_access_groups.property = 'ep_access_groups')";
                    }

                    if ($use_users) {
                        $joins[] = " LEFT OUTER JOIN {$serendipity['dbPrefix']}entryproperties ep_access_users
                                                  ON (e.id = ep_access_users.entryid AND ep_access_users.property = 'ep_access_users')";
                    }

                    if (!isset($addData['noSticky']) || $addData['noSticky'] !== true) {
                        $joins[] = " LEFT JOIN {$serendipity['dbPrefix']}entryproperties ep_sticky
                                            ON (e.id = ep_sticky.entryid AND ep_sticky.property = 'ep_is_sticky')";
                    }

                    $cond = implode("\n", $joins);
                    if (empty($eventData['joins'])) {
                        $eventData['joins'] = $cond;
                    } else {
                        $eventData['joins'] .= $cond;
                    }

                    return true;
                    break;

                case 'frontend_entries_rss':
                    if (is_array($eventData)) {
                        foreach($eventData AS $idx => $entry) {
                            if (is_array($entry['properties']) && isset($entry['properties']['ep_hiderss']) && $entry['properties']['ep_hiderss']) {
                                unset($eventData[$idx]['body']);
                                unset($eventData[$idx]['extended']);
                                unset($eventData[$idx]['exflag']);
                            }
                        }
                    }
                    return true;
                    break;

                case 'backend_plugins_new_instance':
                    // This hook will always push the entryproperties plugin as last in queue.
                    // Happens always when a new plugin is added.
                    // This is important because of its caching mechanism!

                    // Fetch maximum sort_order value. This will be the new value of our current plugin.
                    $q  = "SELECT MAX(sort_order) as sort_order_max FROM {$serendipity['dbPrefix']}plugins WHERE placement = '" . $addData['default_placement'] . "'";
                    $rs  = serendipity_db_query($q, true, 'num');

                    // Fetch current sort_order of current plugin.
                    $q   = "SELECT sort_order FROM {$serendipity['dbPrefix']}plugins WHERE name = '" . $this->instance . "'";
                    $cur = serendipity_db_query($q, true, 'num');
                    
                    // Decrease sort_order of all plugins after current plugin by one.
                    $q = "UPDATE {$serendipity['dbPrefix']}plugins SET sort_order = sort_order - 1 WHERE placement = '" . $addData['default_placement'] . "' AND sort_order > " . intval($cur[0]);
                    serendipity_db_query($q);

                    // Set current plugin as last plugin in queue.
                    $q = "UPDATE {$serendipity['dbPrefix']}plugins SET sort_order = " . intval($rs[0]) . " WHERE name = '" . $this->instance . "'";
                    serendipity_db_query($q);

                    return true;
                    break;

                default:
                    return false;
                    break;
            }
        } else {
            return false;
        }
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>