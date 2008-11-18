<?php # $Id: plugin_api.inc.php 661 2005-11-07 16:40:47Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (IN_serendipity !== true) {
    die ('Don\'t hack!');
}

include_once S9Y_INCLUDE_PATH . 'include/functions.inc.php';

/* This file defines the plugin API for serendipity.
 * By extending these classes, you can add your own code
 * to appear in the sidebar(s) of serendipity.
 *
 *
 * The system defines a number of built-in plugins; these are
 * identified by @class_name.
 *
 * Third-party plugins are identified by the name of the folder into
 * which they were uploaded (so there is no @ sign at the start of
 * their class name.
 *
 * The user creates instances of plugins; an instance is assigned
 * an identifier like this:
 *   classname:uniqid()
 *
 * The user can configure instances of plugins.
 */

class serendipity_plugin_api {
    function register_default_plugins()
    {
        /* Register default sidebar plugins, order matters */
        serendipity_plugin_api::create_plugin_instance('@serendipity_calendar_plugin');
        serendipity_plugin_api::create_plugin_instance('@serendipity_quicksearch_plugin');
        serendipity_plugin_api::create_plugin_instance('@serendipity_archives_plugin');
        serendipity_plugin_api::create_plugin_instance('@serendipity_categories_plugin');
        serendipity_plugin_api::create_plugin_instance('@serendipity_syndication_plugin');
        serendipity_plugin_api::create_plugin_instance('@serendipity_superuser_plugin');
        serendipity_plugin_api::create_plugin_instance('@serendipity_plug_plugin');

        /* Register default event plugins */
        serendipity_plugin_api::create_plugin_instance('serendipity_event_s9ymarkup', null, 'event');
        serendipity_plugin_api::create_plugin_instance('serendipity_event_emoticate', null, 'event');
        serendipity_plugin_api::create_plugin_instance('serendipity_event_nl2br', null, 'event');
        serendipity_plugin_api::create_plugin_instance('serendipity_event_browsercompatibility', null, 'event');
        serendipity_plugin_api::create_plugin_instance('serendipity_event_spamblock', null, 'event');
    }

    /* Create an instance of a plugin.
     * $plugin_class_id is of the form:
     *    @class_name        for a built-in plugin
     * or
     *    plugin_dir_name    for a third-party plugin
     * returns the instance identifier for the newly created plugin.
     *
     * TO BE IMPLEMENTED:
     * If $copy_from_instance is not null, and identifies another plugin
     * of the same class, then the persistent state will be copied.
     * This allows the user to clone a plugin.
     */
    function create_plugin_instance($plugin_class_id, $copy_from_instance = null, $default_placement = 'right', $authorid = '0', $pluginPath = '')
    {
        global $serendipity;

        $id = md5(uniqid(''));

        $key = $plugin_class_id . ':' . $id;

        // Secure Plugin path. No leading slashes, no backslashes, no "up" directories
        $pluginPath = preg_replace('@^(/)@', '', $pluginPath);
        $pluginPath = str_replace(array('..', "\\"), array('', '/'), serendipity_db_escape_string($pluginPath));

        $rs = serendipity_db_query("SELECT MAX(sort_order) as sort_order_max FROM {$serendipity['dbPrefix']}plugins WHERE placement = '$default_placement'", true, 'num');

        if (is_array($rs)) {
            $nextidx = intval($rs[0]+1);
        } else {
            $nextidx = 0;
        }

        $serendipity['debug']['pluginload'][] = "Installing plugin: " . print_r(func_get_args(), true);

        $iq = "INSERT INTO {$serendipity['dbPrefix']}plugins (name, sort_order, placement, authorid, path) values ('$key', $nextidx, '$default_placement', '$authorid', '$pluginPath')";
        $serendipity['debug']['pluginload'][] = $iq;
        serendipity_db_query($iq);
        serendipity_plugin_api::hook_event('backend_plugins_new_instance', $key, array('default_placement' => $default_placement));

        /* Check for multiple dependencies */
        $plugin =& serendipity_plugin_api::load_plugin($key, $authorid, $pluginPath);
        if (is_object($plugin)) {
            $bag    = new serendipity_property_bag;
            $plugin->introspect($bag);
            serendipity_plugin_api::get_event_plugins(false, true); // Refresh static list of plugins to allow execution of added plugin
            $plugin->register_dependencies(false, $authorid);
            $plugin->install();
        } else {
            $serendipity['debug']['pluginload'][] = "Loading plugin failed painfully. File not found?";
            echo ERROR . ': ' . $key . ' (' . $pluginPath . ')<br />';
        }

        return $key;
    }

    function remove_plugin_instance($plugin_instance_id)
    {
        global $serendipity;

        $plugin =& serendipity_plugin_api::load_plugin($plugin_instance_id);
        if (is_object($plugin)) {
            $bag    = new serendipity_property_bag;
            $plugin->introspect($bag);
            $plugin->uninstall($bag);
        }

        serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}plugins where name='$plugin_instance_id'");

        if (is_object($plugin)) {
            $plugin->register_dependencies(true);
        }

        serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}config  where name LIKE '$plugin_instance_id/%'");
    }

    function remove_plugin_value($plugin_instance_id, $where)
    {
        global $serendipity;
        $where_sql = array();
        foreach($where AS $key) {
            $where_sql[] = "(name LIKE '{$plugin_instance_id}/{$key}_%' AND value = '')";
        }

        $query = "DELETE FROM  {$serendipity['dbPrefix']}config
                                    WHERE  " . implode(' OR ', $where_sql);

        serendipity_db_query($query);
    }

    /* Retrieves a list of available plugins */
    function &enum_plugin_classes($event_only = false)
    {
        global $serendipity;

        $classes = array();

        /* built-in classes first */
        $cls = get_declared_classes();
        foreach ($cls as $class_name) {
            if (strncmp($class_name, 'serendipity_', 6)) {
                continue;
            }

            $p = get_parent_class($class_name);
            while ($p != 'serendipity_plugin' && $p != 'serendipity_event' && $p !== false) {
                $p = get_parent_class($p);
            }

            if ($p == 'serendipity_plugin' && $class_name != 'serendipity_event' && (!$event_only || is_null($event_only))) {
                $classes[$class_name] = array('name'       => '@' . $class_name,
                                              'type'       => 'internal_event',
                                              'true_name'  => $class_name,
                                              'pluginPath' => '');
            } elseif ($p == 'serendipity_event' && $class_name != 'serendipity_event' && ($event_only || is_null($event_only))) {
                $classes[$class_name] = array('name'       => '@' . $class_name,
                                              'type'       => 'internal_plugin',
                                              'true_name'  => $class_name,
                                              'pluginPath' => '');
            }
        }

        /* GLOBAL third-party classes next */
        $ppath = serendipity_getRealDir(__FILE__) . 'plugins';
        serendipity_plugin_api::traverse_plugin_dir($ppath, $classes, $event_only);

        /* LOCAL third-party classes next */
        $local_ppath = $serendipity['serendipityPath'] . 'plugins';
        if ($ppath != $local_ppath) {
            serendipity_plugin_api::traverse_plugin_dir($local_ppath, $classes, $event_only);
        }

        return $classes;
    }

    function traverse_plugin_dir($ppath, &$classes, $event_only, $maindir = '') {
        $d = @opendir($ppath);
        if ($d) {
            while (($f = readdir($d)) !== false) {
                if ($f{0} == '.' || $f == 'CVS' || !is_dir($ppath . '/' . $f) || !is_readable($ppath . '/' .$f)) {
                    continue;
                }
                
                $subd = opendir($ppath . '/' . $f);
                if (!$subd) {
                    continue;
                }

                // Instead of only looking for directories, search for files within subdirectories
                $final_loop = false;
                while (($subf = readdir($subd)) !== false) {
                    
                    if ($subf{0} == '.' || $subf == 'CVS') {
                        continue;
                    }

                    if (!$final_loop && is_dir($ppath . '/' . $f . '/' . $subf) && $maindir != $ppath . '/' . $f) {
                        // Search for another level of subdirectories
                        serendipity_plugin_api::traverse_plugin_dir($ppath . '/' . $f, $classes, $event_only, $f . '/');
                        // We can break after that operation because the current directory has been fully checked already.
                        $final_loop = true;
                    }

                    if (!preg_match('@^[^_]+_(event|plugin)_.+\.php$@i', $subf)) {
                        continue;
                    }

                    $class_name = str_replace('.php', '', $subf);
                    // If an external plugin/event already exists as internal, remove the internal reference because its redundant
                    if (isset($classes['@' . $class_name])) {
                        unset($classes['@' . $class_name]);
                    }

                    // A local plugin will be preferred over general plugins [used when calling this function the second time]
                    if (isset($classes[$class_name])) {
                        unset($classes[$class_name]);
                    }

                    if (!is_null($event_only) && $event_only && !serendipity_plugin_api::is_event_plugin($subf)) {
                        continue;
                    }

                    if (!is_null($event_only) && !$event_only && serendipity_plugin_api::is_event_plugin($subf)) {
                        continue;
                    }

                    $classes[$class_name] = array('name'       => $class_name,
                                                  'true_name'  => $class_name,
                                                  'type'       => 'additional_plugin',
                                                  'pluginPath' => $maindir . $f);
                }
                closedir($subd);
            }
            closedir($d);
        }
    }

    function get_installed_plugins($filter = '*') {
        $plugins = serendipity_plugin_api::enum_plugins($filter);
        $res = array();
        foreach ( (array)$plugins as $plugin ) {
            list($class_name) = explode(':', $plugin['name']);
            if ($class_name{0} == '@') {
                $class_name = substr($class_name, 1);
            }
            $res[] = $class_name;
        }
        return $res;
    }

    /* Retrieves a list of plugin instances */
    function enum_plugins($filter = '*', $negate = false, $classname = null, $id = null)
    {
        global $serendipity;

        $sql   = "SELECT * from {$serendipity['dbPrefix']}plugins ";
        $where = array();

        if ($filter !== '*') {
            if ($negate) {
                $where[] = " placement != '" . serendipity_db_escape_string($filter) . "' ";
            } else {
                $where[] = " placement =  '" . serendipity_db_escape_string($filter) . "' ";
            }
        }
        
        if (!empty($classname)) {
            $where[] = " (name LIKE '@" . serendipity_db_escape_string($classname) . "%' OR name LIKE '" . serendipity_db_escape_string($classname) . "%') ";
        }
        
        if (!empty($id)) {
            $where[] = " name = '" . serendipity_db_escape_string($id) . "' ";
        }
        
        if (count($where) > 0) {
            $sql .= ' WHERE ' . implode(' AND ', $where);
        }

        $sql .= ' ORDER BY placement, sort_order';

        return serendipity_db_query($sql);
    }

    /* Retrieves a list of plugin instances */
    function count_plugins($filter = '*', $negate = false)
    {
        global $serendipity;

        $sql = "SELECT COUNT(placement) AS count from {$serendipity['dbPrefix']}plugins ";

        if ($filter !== '*') {
            if ($negate) {
                $sql .= "WHERE placement != '$filter' ";
            } else {
                $sql .= "WHERE placement='$filter' ";
            }
        }

        $count = serendipity_db_query($sql, true);
        if (is_array($count) && isset($count[0])) {
            return (int)$count[0];
        }

        return 0;
    }

    function includePlugin($name, $pluginPath = '', $instance_id = '') {
        global $serendipity;

        if (empty($pluginPath)) {
            $pluginPath = $name;
        }
        
        $file = false;
        
        // First try the local path, and then (if existing) a shared library repository ...
        // Internal plugins ignored.
        if (!empty($instance_id) && $instance_id{0} == '@') {
            $file = S9Y_INCLUDE_PATH . 'include/plugin_internal.inc.php';
        } elseif (file_exists($serendipity['serendipityPath'] . 'plugins/' . $pluginPath . '/' . $name . '.php')) {
            $file = $serendipity['serendipityPath'] . 'plugins/' . $pluginPath . '/' . $name . '.php';
        } elseif (file_exists(S9Y_INCLUDE_PATH . 'plugins/' . $pluginPath . '/' . $name . '.php')) {
            $file = S9Y_INCLUDE_PATH . 'plugins/' . $pluginPath . '/' . $name . '.php';
        }
        
        return $file;
    }

    function getClassByInstanceID($instance_id, &$is_internal) {
        $instance = explode(':', $instance_id);
        $name     = $instance[0];

        if ($name{0} == '@') {
            $class_name = substr($name, 1);
        } else {
            $class_name =& $name;
        }
        
        return $class_name;
    }
    
    /* Probes for the plugin filename */
    function probePlugin($instance_id, &$class_name, &$pluginPath) {
        global $serendipity;

        $filename    = false;
        $is_internal = false;
        
        $class_name  = serendipity_plugin_api::getClassByInstanceID($instance_id, $is_internal);

        if (!$is_internal) {
            /* plugin from the plugins/ dir */
            // $serendipity['debug']['pluginload'][] = "Including plugin $class_name, $pluginPath";
            $filename = serendipity_plugin_api::includePlugin($class_name, $pluginPath, $instance_id);
            if (empty($filename) && !empty($instance_id)) {
                // $serendipity['debug']['pluginload'][] = "No valid path/filename found.";
                $sql = "SELECT path from {$serendipity['dbPrefix']}plugins WHERE name = '" . $instance_id . "'";
                $plugdata = serendipity_db_query($sql, true, 'both', false, false, false, true);
                if (is_array($plugdata) && isset($plugdata[0])) {
                    $pluginPath = $plugdata[0];
                }

                if (empty($pluginPath)) {
                    $pluginPath = $class_name;
                }
        
                // $serendipity['debug']['pluginload'][] = "Including plugin(2) $class_name, $pluginPath";
                $filename = serendipity_plugin_api::includePlugin($class_name, $pluginPath);
            }

            if (empty($filename)) {
                $serendipity['debug']['pluginload'][] = "No valid path/filename found. Aborting.";
                return false;
            }
        }

        // $serendipity['debug']['pluginload'][] = "Found plugin file $filename";
        return $filename;
    }
    
    /* Creates an instance of a named plugin */
    function &load_plugin($instance_id, $authorid = null, $pluginPath = '', $pluginFile = null) {
        global $serendipity;

        if ($pluginFile === null) {
            $class_name = '';
            // $serendipity['debug']['pluginload'][] = "Init probe for plugin $instance_id, $class_name, $pluginPath";
            $pluginFile = serendipity_plugin_api::probePlugin($instance_id, $class_name, $pluginPath);
        } else {
            $is_internal = false;
            // $serendipity['debug']['pluginload'][] = "getClassByInstanceID $instance_id, $is_internal";
            $class_name  = serendipity_plugin_api::getClassByInstanceID($instance_id, $is_internal);
        }
        
        if (!class_exists($class_name) && !empty($pluginFile)) {
            // $serendipity['debug']['pluginload'][] = "Classname does not exist. Including $pluginFile.";
            include_once($pluginFile);
        }

        if (!class_exists($class_name)) {
            $serendipity['debug']['pluginload'][] = "Classname $class_name still does not exist. Aborting.";
            $retval = false;
            return $retval;
        }
        
        // $serendipity['debug']['pluginload'][] = "Returning new $class_name($instance_id)";
        $p =& new $class_name($instance_id);
        if (!is_null($authorid)) {
            $p->serendipity_owner = $authorid;
        } else {
            $sql = "SELECT authorid from {$serendipity['dbPrefix']}plugins WHERE name = '" . $instance_id . "'";
            $owner = serendipity_db_query($sql, true);
            if (is_array($owner) && isset($owner[0])) {
                $p->serendipity_owner = $owner[0];
            }
        }

        return $p;
    }

    function &getPluginInfo(&$pluginFile, &$class_data, $type) {
        global $serendipity;

        static $pluginlist = null;
        
        if ($pluginlist === null) {
            $data = serendipity_db_query("SELECT p.*,
                                                 pc.category
                                            FROM {$serendipity['dbPrefix']}pluginlist AS p
                                 LEFT OUTER JOIN {$serendipity['dbPrefix']}plugincategories AS pc
                                              ON pc.class_name = p.class_name
                                           WHERE p.pluginlocation = 'local' AND
                                                 p.plugintype     = '" . serendipity_db_escape_string($type) . "'");
            if (is_array($data)) {
                foreach($data AS $p) {
                    if (!isset($pluginlist[$p['plugin_file']])) {
                        $pluginlist[$p['plugin_file']] = $p;
                    }

                    $pluginlist[$p['plugin_file']]['groups'][] = $p['category'];
                }
            }
        }
        
        if (is_array($pluginlist[$pluginFile]) && !preg_match('@plugin_internal\.inc\.php@', $pluginFile)) {
            $data = $pluginlist[$pluginFile];
            if ((int)filemtime($pluginFile) == (int)$data['last_modified']) {
                $data['stackable']    = serendipity_db_bool($data['stackable']);

                $plugin    = $data;
                return $plugin;
            }
        }
        
        $plugin =& serendipity_plugin_api::load_plugin($class_data['name'], null, $class_data['pluginPath'], $pluginFile);
        
        return $plugin;
    }
    
    function &setPluginInfo(&$plugin, &$pluginFile, &$bag, &$class_data, $pluginlocation = 'local') {
        global $serendipity;
        
        static $dbfields = array(
            'plugin_file',
            'class_name',
            'plugin_class',
            'pluginPath',
            'name',
            'description',
            'version',
            'upgrade_version',
            'plugintype',
            'pluginlocation',
            'stackable',
            'author',
            'requirements',
            'website',
            'last_modified'
        );

        serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}pluginlist WHERE plugin_file = '" . serendipity_db_escape_string($pluginFile) . "' AND pluginlocation = '" . serendipity_db_escape_string($pluginlocation) . "'");
        
        if (!empty($pluginFile) && file_exists($pluginFile)) {
            $lastModified = filemtime($pluginFile); 
        } else {
            $lastModified = 0;
        }

        if (is_object($plugin)) {
            $data = array(
                'class_name'      => get_class($plugin),
                'stackable'       => $bag->get('stackable'),
                'name'            => $bag->get('name'),
                'description'     => $bag->get('description'),
                'author'          => $bag->get('author'),
                'version'         => $bag->get('version'),
                'upgrade_version' => isset($class_data['upgrade_version']) ? $class_data['upgrade_version'] : $bag->get('version'),
                'requirements'    => serialize($bag->get('requirements')),
                'website'         => $bag->get('website'),
                'plugin_class'    => $class_data['name'],
                'pluginPath'      => $class_data['pluginPath'],
                'plugin_file'     => $pluginFile,
                'pluginlocation'  => $pluginlocation,
                'plugintype'      => $serendipity['GET']['type'],
                'last_modified'   => $lastModified 
            );
            $groups = $bag->get('groups');
        } elseif (is_array($plugin)) {
            $data = $plugin;
            $groups = $data['groups'];
            unset($data['installable']);
            unset($data['true_name']);
            unset($data['customURI']);
            unset($data['groups']);
            $data['requirements'] = serialize($data['requirements']);
        }

        if (!isset($data['stackable']) || empty($data['stackable'])) {
            $data['stackable'] = '0';
        }
        
        // Only insert data keys that exist in the DB.
        $insertdata = array();
        foreach($dbfields AS $field) {
            $insertdata[$field] = $data[$field];
        }
        
        if ($data['upgradable']) {
            serendipity_db_query("UPDATE {$serendipity['dbPrefix']}pluginlist 
                                     SET upgrade_version = '" . serendipity_db_escape_string($data['upgrade_version']) . "' 
                                   WHERE plugin_class    = '" . serendipity_db_escape_string($data['plugin_class']) . "'");
        }
        serendipity_db_insert('pluginlist', $insertdata);

        serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}plugincategories WHERE class_name = '" . serendipity_db_escape_string($data['class_name']) . "'");
        foreach((array)$groups AS $group) {
            if (empty($group)) {
                continue;
            }

            $cat = array(
                'class_name'  => $data['class_name'],
                'category'    => $group
            );
            serendipity_db_insert('plugincategories', $cat);
        }
        
        $data['groups'] = $groups;

        return $data;
    }

    function update_plugin_placement($name, $placement, $order=null)
    {
        global $serendipity;

        $admin = '';
        if (!serendipity_checkPermission('adminPlugins') && $placement == 'hidden') {
            // Only administrators can set plugins to 'hidden' if they are not the owners.
            $admin = " AND (authorid = 0 OR authorid = {$serendipity['authorid']})";
        }

        $sql = "UPDATE {$serendipity['dbPrefix']}plugins set placement='$placement' ";

        if ($order !== null) {
            $sql .= ", sort_order=$order ";
        }

        $sql .= "WHERE name='$name' $admin";

        return serendipity_db_query($sql);
    }

    function update_plugin_owner($name, $authorid)
    {
        global $serendipity;

        if (empty($authorid) && $authorid != '0') {
            return;
        }

        $admin = '';
        if (!serendipity_checkPermission('adminPlugins')) {
            $admin = " AND (authorid = 0 OR authorid = {$serendipity['authorid']})";
        }

        $sql = "UPDATE {$serendipity['dbPrefix']}plugins SET authorid='$authorid' WHERE name='$name' $admin";

        return serendipity_db_query($sql);
    }

    function generate_plugins($side, $tag = '', $negate = false, $class = null, $id = null)
    {
        global $serendipity;

        /* $tag parameter is deprecated and used in Smarty templates instead. Only use it in function
         * header for layout.php BC.
         */

        $plugins = serendipity_plugin_api::enum_plugins($side, $negate, $class, $id);

        if (!is_array($plugins)) {
            return;
        }
        
        if (!isset($serendipity['smarty'])) {
            $serendipity['smarty_raw_mode'] = true;
            serendipity_smarty_init();
        }

        $pluginData = array();
        $addData    = func_get_args();
        serendipity_plugin_api::hook_event('frontend_generate_plugins', $plugins, $addData);

        foreach ($plugins as $plugin_data) {
            $plugin =& serendipity_plugin_api::load_plugin($plugin_data['name'], $plugin_data['authorid'], $plugin_data['path']);
            if (is_object($plugin)) {
                $class  = get_class($plugin);
                $title  = '';

                /* TODO: make generate_content NOT echo its output */
                ob_start();
                $show_plugin = $plugin->generate_content($title);
                $content = ob_get_contents();
                ob_end_clean();

                if ($show_plugin !== FALSE) {
                    $pluginData[] = array('side'    => $side,
                                          'class'   => $class,
                                          'title'   => $title,
                                          'content' => $content);
                }
            } else {
                    $pluginData[] = array('side'          => $side,
                                          'title'         => ERROR,
                                          'class'         => $class,
                                          'content'       => sprintf(INCLUDE_ERROR, $plugin_data['name']));
            }
        }

        $serendipity['smarty']->assign(
            array(
                'plugindata' => $pluginData,
                'pluginside' => ucfirst($side)
            )
        );


        return serendipity_smarty_fetch('sidebar_'. $side, 'sidebar.tpl', true);
    }

    function get_plugin_title(&$plugin, $default_title = '')
    {
        global $serendipity;

        // Generate plugin output. Make sure that by probing the plugin, no events are actually called. After that,
        // restore setting of 'no_events'.

        if (!is_null($plugin->title)) {
            // Preferred way of fetching a plugins title
            $title = &$plugin->title;
        } else {
            $ne = (isset($serendipity['no_events']) && $serendipity['no_events'] ? TRUE : FALSE);
            $serendipity['no_events'] = TRUE;
            ob_start();
            $plugin->generate_content($title);
            ob_end_clean();
            $serendipity['no_events'] = $ne;
        }

        if (strlen(trim($title)) == 0) {
            if (!empty($default_title)) {
                $title = $default_title;
            } else {
                $title = $plugin->instance;
            }
        }

        return $title;
    }

    /* conditional to see if the named plugin is an event plugin
        Refactoring: decompose conditional */
    function is_event_plugin($name) {
        return (strstr($name, '_event_'));
    }

    function &get_event_plugins($getInstance = false, $refresh = false) {
        static $event_plugins;
        static $false = false;

        if (!$refresh && isset($event_plugins) && is_array($event_plugins)) {
            if ($getInstance) {
                if (isset($event_plugins[$getInstance]['p'])) {
                    return $event_plugins[$getInstance]['p'];
                }
                return $false;
            }
            return $event_plugins;
        }

        $plugins = serendipity_plugin_api::enum_plugins('event');

        if (!is_array($plugins)) {
            return $false;
        }

        $event_plugins = array();
        foreach($plugins AS $plugin_data) {
            if ($event_plugins[$plugin_data['name']]['p'] = &serendipity_plugin_api::load_plugin($plugin_data['name'], $plugin_data['authorid'], $plugin_data['path'])) {
                /* query for its name, description and configuration data */
                $event_plugins[$plugin_data['name']]['b'] = new serendipity_property_bag;
                $event_plugins[$plugin_data['name']]['p']->introspect($event_plugins[$plugin_data['name']]['b']);
                $event_plugins[$plugin_data['name']]['t'] = serendipity_plugin_api::get_plugin_title($event_plugins[$plugin_data['name']]['p']);
            } else {
                unset($event_plugins[$plugin_data['name']]); // Unset failed plugins
            }
        }

        if ($getInstance) {
            if (isset($event_plugins[$getInstance]['p'])) {
                return $event_plugins[$getInstance]['p'];
            }
            return $false;
        }
        
        return $event_plugins;
    }

    function hook_event($event_name, &$eventData, $addData = null) {
        global $serendipity;

        // Can be bypassed globally by setting $serendipity['no_events'] = TRUE;
        if (isset($serendipity['no_events']) && $serendipity['no_events'] == true) {
            return false;
        }
        
        $plugins = serendipity_plugin_api::get_event_plugins();
        
        if (is_array($plugins)) {
            // foreach() operates on copies of values, but we want to operate on references, so we use while()
            @reset($plugins);
            while(list($plugin, $plugin_data) = each($plugins)) {
                $bag = &$plugin_data['b'];
                if (array_key_exists($event_name, $bag->get('event_hooks'))) {
                    // Check for cachable events.
                    if (isset($eventData['is_cached']) && $eventData['is_cached'] && array_key_exists($event_name, (array)$bag->get('cachable_events'))) {
                        continue;
                    }

                    $plugin_data['p']->event_hook($event_name, $bag, $eventData, $addData);
                }
            }
        }

        return true;
    }

    function exists($instance_id) {
        global $serendipity;

        if (!strstr($instance_id, ':')) {
            $instance_id .= ':';
        }

        $existing = serendipity_db_query("SELECT name FROM {$serendipity['dbPrefix']}plugins WHERE name LIKE '%$instance_id%'");

        if (is_array($existing) && !empty($existing[0][0])) {
            return $existing[0][0];
        }

        return false;
    }

    function &autodetect_instance($plugin_name, $authorid, $is_event_plugin = false) {
        if ($is_event_plugin) {
            $side = 'event';
        } else {
            $side = 'right';
        }

        $classes = serendipity_plugin_api::enum_plugin_classes(null);
        if (isset($classes[$plugin_name])) {
            $instance = serendipity_plugin_api::create_plugin_instance($plugin_name, null, $side, $authorid, $classes[$plugin_name]['pluginPath']);
        } else {
            $instance = false;
        }

        return $instance;
    }
}

/* holds a bunch of properties; since serendipity 0.8 only one value per key is allowed [was never really useful] */
class serendipity_property_bag {
    var $properties = array();
    var $name       = null;

    function add($name, $value)
    {
        $this->properties[$name] = $value;
    }

    function &get($name)
    {
        return $this->properties[$name];
    }

    function is_set($name)
    {
        if (isset($this->properties[$name])) {
            return true;
        }

        return false;
    }
}

class serendipity_plugin {
    var $instance      = null;
    var $protected     = false;
    var $wrap_class    = 'serendipitySideBarItem';
    var $title_class   = 'serendipitySideBarTitle';
    var $content_class = 'serendipitySideBarContent';
    var $title         = null;

    /* Be sure to call this method from your derived classes constructors,
     * otherwise your config data will not be stored or retrieved correctly
     */

    function serendipity_plugin($instance)
    {
        $this->instance = $instance;
    }

    /* Called by Serendipity when the plugin is first installed.
     * Can be used to install database tables etc.
     */
    function install()
    {
        return true;
    }

    /* Called by Serendipity when the plugin is removed/uninstalled.
     * Can be used to drop installed database tables etc.
     */
    function uninstall(&$propbag)
    {
        return true;
    }

    /* Called by serendipity when it wants to display information
     * about your plugin.
     * You need to override this method in your child class.
     */
    function introspect(&$propbag)
    {
        $propbag->add('copyright', 'MIT License');
        $propbag->add('name'     , get_class($this));

        // $propbag->add(
        //   'configuration',
        //   array(
        //     'text field',
        //     'foo bar'
        //   )
        // );

        $this->protected = FALSE; // If set to TRUE, only allows the owner of the plugin to modify its configuration

        return true;
    }

    /* Called by serendipity when it wants to display the configuration
     * editor for your plugin.
     * $name is the name of a configuration item you added in
     * your instrospect method.
     * You need to fill the property bag with appropriate items
     * that describe the type and value(s) for that particular
     * configuration option.
     * You need to override this method in your child class if
     * you have configuration options.
     */
    function introspect_config_item($name, &$propbag)
    {
        return false;
    }

    /* Called to validate a plugin option */
    function validate($config_item, &$cbag, &$value) {
        static $pattern_mail  = '([\.\-\+~@_0-9a-z]+?)';
        static $pattern_url   = '([@!=~\?:&;0-9a-z#\.\-_\/]+?)';

        $validate = $cbag->get('validate');
        $valid    = true;

        if (!empty($validate)) {
            switch ($validate) {
                case 'string':
                    if (!preg_match('@^\w*$@i', $value)) {
                        $valid = false;
                    }
                    break;

                case 'words':
                    if (!preg_match('@^[\w\s\r\n,\.\-!\?:;&_/=%\$]*$@i', $value)) {
                        $valid = false;
                    }
                    break;
                
                case 'number':
                    if (!preg_match('@^[\d]*$@', $value)) {
                        $valid = false;
                    }
                    break;

                case 'url':
                    if (!preg_match('§^' . $pattern_url . '$§', $value)) {
                        $valid = false;
                    }
                    break;

                case 'mail':
                    if (!preg_match('§^' . $pattern_mail . '$§', $value)) {
                        $valid = false;
                    }
                    break;

                case 'path':
                    if (!preg_match('@^[\w/_.\-~]$@', $value)) {
                        $valid = false;
                    }
                    break;

                default:
                    if (!preg_match($validate, $value)) {
                        $valid = false;
                    }
                    break;
            }
            
            $error = $cbag->get('validate_error');
            if ($valid) {
                return true;
            } elseif (!empty($error)) {
                return $error;
            } else {
                return sprintf(PLUGIN_API_VALIDATE_ERROR, $config_item, $validate); 
            }
        }
        
        return true;
    }
    
    /* Called by serendipity when it wants your plugin to display itself.
     * You need to set $title to be whatever text you want want to
     * appear in the item caption space.
     * Simply echo/print your content to the output; serendipity will
     * capture it and make things work.
     * You need to override this method in your child class.
     */
    function generate_content(&$title)
    {
        $title = 'Sample!';
        echo     'This is a sample!';
    }

    /* Fetches a configuration value for this plugin */
    function get_config($name, $defaultvalue = null, $empty = true)
    {
        $_res = serendipity_get_config_var($this->instance . '/' . $name, $defaultvalue, $empty);

        if (is_null($_res)) {
            // A protected plugin by a specific owner may not have its values stored in $serendipity
            // because of the special authorid. To display such contents, we need to fetch it
            // seperately from the DB.
            $_res = serendipity_get_user_config_var($this->instance . '/' . $name, null, $defaultvalue);
        }

        if (is_null($_res)) {
            $cbag = new serendipity_property_bag;
            $this->introspect_config_item($name, $cbag);
            $_res = $cbag->get('default');
            unset($cbag);
        }

        return $_res;
    }

    function set_config($name, $value)
    {
        $name = $this->instance . '/' . $name;

        return serendipity_set_config_var($name, $value);
    }

    /* Called by serendipity after insertion of a config item. If you want to kick out certain
     * elements based on contents, create the corresponding function here.
     */
    function cleanup()
    {
        // Cleanup. Remove all empty configs on SAVECONF-Submit.
        // serendipity_plugin_api::remove_plugin_value($this->instance, array('title', 'description'));
        return true;
    }

    function register_dependencies($remove = false, $authorid = '0')
    {
        global $serendipity;

        if (isset($this->dependencies) && is_array($this->dependencies)) {

            if ($remove) {
                $dependencies = @explode(';', $this->get_config('dependencies'));
                $modes        = @explode(';', $this->get_config('dependency_modes'));

                if (!empty($dependencies) && is_array($dependencies)) {
                    foreach($dependencies AS $idx => $dependency) {
                        if ($modes[$idx] == 'remove' && serendipity_plugin_api::exists($dependency)) {
                            serendipity_plugin_api::remove_plugin_instance($dependency);
                        }
                    }
                }
            } else {
                $keys  = array();
                $modes = array();
                foreach($this->dependencies AS $dependency => $mode) {
                    $exists = serendipity_plugin_api::exists($dependency);
                    if (!$exists) {
                        if (serendipity_plugin_api::is_event_plugin($dependency)) {
                            $keys[] = serendipity_plugin_api::autodetect_instance($dependency, $authorid, true);
                        } else {
                            $keys[] = serendipity_plugin_api::autodetect_instance($dependency, $authorid, false);
                        }
                    } else {
                        $keys[] = $exists;
                    }

                    $modes[] = $mode;
                }

                $this->set_config('dependencies',     implode(';', $keys));
                $this->set_config('dependency_modes', implode(';', $modes));
            }
        }

        return true;
    }
}

class serendipity_event extends serendipity_plugin {
    /* Events can be called on several occasions when s9y performs an action.
     * One or multiple plugin can be registered for each of those hooks.
     */

    /* Be sure to call this method from your derived classes constructors,
     * otherwise your config data will not be stored or retrieved correctly
     */

    function serendipity_event($instance)
    {
        $this->instance = $instance;
    }

    function &getFieldReference($fieldname = 'body', &$eventData) {
        // Get a reference to a content field (body/extended) of
        // $entries input data. This is a unifying function because
        // several plugins are using similar fields.

        if (is_array($eventData) && isset($eventData[0]) && is_array($eventData[0]['properties'])) {
            if (!empty($eventData[0]['properties']['ep_cache_' . $fieldname])) {

                // It may happen that there is no extended entry to concatenate to. In that case,
                // create a dummy extended entry.
                if (!isset($eventData[0]['properties']['ep_cache_' . $fieldname])) {
                    $eventData[0]['properties']['ep_cache_' . $fieldname] = '';
                }

                $key = &$eventData[0]['properties']['ep_cache_' . $fieldname];
            } else {
                $key = &$eventData[0][$fieldname];
            }
        } elseif (is_array($eventData) && is_array($eventData['properties'])) {
            if (!empty($eventData['properties']['ep_cache_' . $fieldname])) {
                $key = &$eventData['properties']['ep_cache_' . $fieldname];
            } else {
                $key = &$eventData[$fieldname];
            }
        } elseif (isset($eventData[0][$fieldname])) {
            $key = &$eventData[0][$fieldname];
        } else {
            $key = '';
        }

        return $key;
    }

    function event_hook($event, &$bag, &$eventData, $addData = null) {
        // Define event hooks here, if you want you plugin to execute those instead of being a sidebar item.
        // Look at external plugins 'serendipity_event_mailer' or 'serendipity_event_weblogping' for usage.
        // Currently available events:
        //   backend_publish [after insertion of a new article in your s9y-backend]
        //   backend_display [after displaying an article in your s9y-backend]
        //   frontend_display [before displaying an article in your s9y-frontend]
        //   frontend_comment [after displaying the "enter comment" dialog]
        return true;
    }
}

include_once S9Y_INCLUDE_PATH . 'include/plugin_internal.inc.php';

/* vim: set sts=4 ts=4 expandtab : */
?>
