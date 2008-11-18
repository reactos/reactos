<?php # $Id: serendipity_event_spartacus.php 654 2005-11-06 21:25:32Z garvinhicking $

/************
  TODO:

  - Perform Serendipity version checks to only install plugins available for version
  - Allow fetching files from mirrors / different locations - don't use ViewCVS hack (revision 1.999 dumbness)

 ***********/

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_EVENT_SPARTACUS_NAME', 'Spartacus');
@define('PLUGIN_EVENT_SPARTACUS_DESC', '[S]erendipity [P]lugin [A]ccess [R]epository [T]ool [A]nd [C]ustomization/[U]nification [S]ystem - Allows you to download plugins from our online repository');
@define('PLUGIN_EVENT_SPARTACUS_FETCH', 'Click here to fetch a new %s from the Serendipity Online Repository');
@define('PLUGIN_EVENT_SPARTACUS_FETCHERROR', 'The URL %s could not be opened. Maybe the Serendipity or SourceForge.net Server is down - we are sorry, you need to try again later.');
@define('PLUGIN_EVENT_SPARTACUS_FETCHING', 'Trying to open URL %s...');
@define('PLUGIN_EVENT_SPARTACUS_FETCHED_BYTES_URL', 'Fetched %s bytes from the URL above. Saving file as %s...');
@define('PLUGIN_EVENT_SPARTACUS_FETCHED_BYTES_CACHE', 'Fetched %s bytes from already existing file on your server. Saving file as %s...');
@define('PLUGIN_EVENT_SPARTACUS_FETCHED_DONE', 'Data successfully fetched.');
@define('PLUGIN_EVENT_SPARTACUS_MIRROR_XML', 'File/Mirror location (XML metadata)');
@define('PLUGIN_EVENT_SPARTACUS_MIRROR_FILES', 'File/Mirror location (files)');
@define('PLUGIN_EVENT_SPARTACUS_MIRROR_DESC', 'Choose a download location. Do NOT change this value unless you know what you are doing and if servers get oudated. This option is available mainly for forward compatibility.');
@define('PLUGIN_EVENT_SPARTACUS_CHOWN', 'Owner of downloaded files');
@define('PLUGIN_EVENT_SPARTACUS_CHOWN_DESC', 'Here you can enter the (FTP/Shell) owner (like "nobody") of files downloaded by Spartacus. If empty, no changes are made to the ownership.');
@define('PLUGIN_EVENT_SPARTACUS_CHMOD', 'Permissions downloaded files');
@define('PLUGIN_EVENT_SPARTACUS_CHMOD_DESC', 'Here you can enter the octal mode (like "0777") of the file permissions for files (FTP/Shell) downloaded by Spartacus. If empty, the default permission mask of the system are used. Note that not all servers allow changing/setting permissions. Pay attention that the applied permissions allow reading and writing for the webserver user. Else spartacus/Serendipity cannot overwrite existing files.');
@define('PLUGIN_EVENT_SPARTACUS_CHMOD_DIR', 'Permissions downloaded directories');
@define('PLUGIN_EVENT_SPARTACUS_CHMOD_DIR_DESC', 'Here you can enter the octal mode (like "0777") of the directory permissions for directories (FTP/Shell) downloaded by Spartacus. If empty, the default permission mask of the system are used. Note that not all servers allow changing/setting permissions. Pay attention that the applied permissions allow reading and writing for the webserver user. Else spartacus/Serendipity cannot overwrite existing directories.');

class serendipity_event_spartacus extends serendipity_event
{
    var $title = PLUGIN_EVENT_SPARTACUS_NAME;
    var $purgeCache = false;

    function microtime_float() {
        list($usec, $sec) = explode(" ", microtime());
        return ((float)$usec + (float)$sec);
    }

    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_EVENT_SPARTACUS_NAME);
        $propbag->add('description',   PLUGIN_EVENT_SPARTACUS_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Garvin Hicking');
        $propbag->add('version',       '2.5');
        $propbag->add('requirements',  array(
            'serendipity' => '0.9',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('event_hooks',    array(
            'backend_plugins_fetchlist'          => true,
            'backend_plugins_fetchplugin'        => true,

            'backend_templates_fetchlist'        => true,
            'backend_templates_fetchtemplate'    => true
        ));
        $propbag->add('groups', array('BACKEND_FEATURES'));
        $propbag->add('configuration', array('mirror_xml', 'mirror_files', 'chown', 'chmod_files', 'chmod_dir'));
    }

    function generate_content(&$title) {
        $title = $this->title;
    }

    function cleanup() {
        global $serendipity;

        // Purge DB cache
        serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}pluginlist");
        serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}plugincategories");

        // Purge cached XML files.
        $files = serendipity_traversePath($serendipity['serendipityPath'] . PATH_SMARTY_COMPILE, '', false, '/package_.+\.xml$/');
    
        if (!is_array($files)) {
            return false;
        }
    
        foreach ($files as $file) {
            printf(DELETING_FILE . '<br />', $file['name']);
            @unlink($serendipity['serendipityPath'] . PATH_SMARTY_COMPILE . '/' . $file['name']);
        }
    }

    function &getMirrors($type = 'xml', $loc = false) {
        static $mirror = array(
            'xml' => array(
                'Netmirror.org',
                's9y.org'
            ),
            
            'files' => array(
                'SourceForge.net',
                'Netmirror.org',
                's9y.org',
                'BerliOS.de'
            )
        );
        
        static $http = array(
            'xml' => array(
                'http://netmirror.org/mirror/serendipity/',
                'http://s9y.org/mirror/'
            ),
            
            'files' => array(
                'http://cvs.sourceforge.net/viewcvs.py/*checkout*/php-blog/',
                'http://netmirror.org/mirror/serendipity/',
                'http://s9y.org/mirror/',
                'http://svn.berlios.de/viewcvs/serendipity/'
            )
        );
        
        if ($loc) {
            return $http[$type];
        } else {
            return $mirror[$type];
        }
    }

    function introspect_config_item($name, &$propbag) {
        global $serendipity;

        switch($name) {
            case 'chmod_files':
                $propbag->add('type',        'string');
                $propbag->add('name',        PLUGIN_EVENT_SPARTACUS_CHMOD);
                $propbag->add('description', PLUGIN_EVENT_SPARTACUS_CHMOD_DESC);
                $propbag->add('default',     '');
                break;

            case 'chmod_dir':
                $propbag->add('type',        'string');
                $propbag->add('name',        PLUGIN_EVENT_SPARTACUS_CHMOD_DIR);
                $propbag->add('description', PLUGIN_EVENT_SPARTACUS_CHMOD_DIR_DESC);
                $propbag->add('default',     '');
                break;

            case 'chown':
                $propbag->add('type',        'string');
                $propbag->add('name',        PLUGIN_EVENT_SPARTACUS_CHOWN);
                $propbag->add('description', PLUGIN_EVENT_SPARTACUS_CHOWN_DESC);
                $propbag->add('default',     '');
                break;

            case 'mirror_xml':
                $propbag->add('type',        'select');
                $propbag->add('name',        PLUGIN_EVENT_SPARTACUS_MIRROR_XML);
                $propbag->add('description', PLUGIN_EVENT_SPARTACUS_MIRROR_DESC);
                $propbag->add('select_values', $this->getMirrors('xml'));
                $propbag->add('default',     0);
                break;

            case 'mirror_files':
                $propbag->add('type',        'select');
                $propbag->add('name',        PLUGIN_EVENT_SPARTACUS_MIRROR_FILES);
                $propbag->add('description', PLUGIN_EVENT_SPARTACUS_MIRROR_DESC);
                $propbag->add('select_values', $this->getMirrors('files'));
                $propbag->add('default',     0);
                break;

            default:
                return false;
        }
        return true;
    }

    function GetChildren(&$vals, &$i) {
        $children = array();
        $cnt = sizeof($vals);
        while (++$i < $cnt) {
            // compare type
            switch ($vals[$i]['type']) {
                case 'cdata':
                    $children[] = $vals[$i]['value'];
                    break;

                case 'complete':
                    $children[] = array(
                        'tag'        => $vals[$i]['tag'],
                        'attributes' => $vals[$i]['attributes'],
                        'value'      => $vals[$i]['value']
                    );
                    break;

                case 'open':
                    $children[] = array(
                        'tag'        => $vals[$i]['tag'],
                        'attributes' => $vals[$i]['attributes'],
                        'value'      => $vals[$i]['value'],
                        'children'   => $this->GetChildren($vals, $i)
                    );
                    break;

                case 'close':
                    return $children;
            }
        }
    }

    // Create recursive directories; begins at serendipity plugin root folder level
    function rmkdir($dir, $sub = 'plugins') {
        global $serendipity;

        $spaths = explode('/', $serendipity['serendipityPath'] . $sub . '/'); 
        $paths  = explode('/', $dir);
        
        $stack  = ''; 
        foreach($paths AS $pathid => $path) {
            $stack .= $path . '/';

            if ($spaths[$pathid] == $path) {
                continue;
            }
            
            if (!is_dir($stack) && !mkdir($stack)) {
                return false;
            } else {
                $this->fileperm($stack, true);
            }
        }
        
        return true;
    }

    // Apply file permission settings.
    function fileperm($stack, $is_dir) {
        $chmod_dir   = intval($this->get_config('chmod_dir'), 8);
        $chmod_files = intval($this->get_config('chmod_files'), 8);
        $chown       = $this->get_config('chown');

        if ($is_dir && !empty($chmod_dir) && function_exists('chmod')) {
            @chmod($stack, $chmod_dir); // Always ensure directory traversal.
        }

        if (!$is_dir && !empty($chmod_files) && function_exists('chmod')) {
            @chmod($stack, $chmod_files); // Always ensure directory traversal.
        }

        if (!empty($chown) && function_exists('chown')) {
            $own = explode('.', $chown);
            if (isset($own[1])) {
                @chgrp($stack, $own[1]);
            }
            @chown($stack, $own[0]);
        }
        
        
        return true;
    }

    function &fetchfile($url, $target, $cacheTimeout = 0, $decode_utf8 = false, $sub = 'plugins') {
        static $error = false;

        printf(PLUGIN_EVENT_SPARTACUS_FETCHING, '<a href="' . $url . '">' . basename($url) . '</a>');
        echo '<br />';

        if (file_exists($target) && filesize($target) > 0 && filemtime($target) >= (time()-$cacheTimeout)) {
            $data = file_get_contents($target);
            printf(PLUGIN_EVENT_SPARTACUS_FETCHED_BYTES_CACHE, strlen($data), $target);
            echo '<br />';
        } else {
            require_once S9Y_PEAR_PATH . 'HTTP/Request.php';
            $req = &new HTTP_Request($url);

            if (PEAR::isError($req->sendRequest()) || $req->getResponseCode() != '200') {
                printf(PLUGIN_EVENT_SPARTACUS_FETCHERROR, $url);
                echo '<br />';
                if (file_exists($target) && filesize($target) > 0) {
                    $data = file_get_contents($target);
                    printf(PLUGIN_EVENT_SPARTACUS_FETCHED_BYTES_CACHE, strlen($data), $target);
                    echo '<br />';
                }
            } else {
                // Fetch file
                $data = $req->getResponseBody();
                printf(PLUGIN_EVENT_SPARTACUS_FETCHED_BYTES_URL, strlen($data), $target);
                echo '<br />';
                $tdir = dirname($target);
                if (!is_dir($tdir) && !$this->rmkdir($tdir, $sub)) {
                    printf(FILE_WRITE_ERROR, $tdir);
                    echo '<br />';
                    return $error;
                }

                $fp = @fopen($target, 'w');

                if (!$fp) {
                    printf(FILE_WRITE_ERROR, $target);
                    echo '<br />';
                    return $error;
                }
                
                if ($decode_utf8) {
                    $data = str_replace('<?xml version="1.0" encoding="UTF-8" ?>', '<?xml version="1.0" encoding="' . LANG_CHARSET . '" ?>', $data);
                    $this->decode($data, true);
                }

                fwrite($fp, $data);
                fclose($fp);

                $this->fileperm($target, false);

                echo PLUGIN_EVENT_SPARTACUS_FETCHED_DONE;                
                echo '<br />';
                $this->purgeCache = true;
            }
        }

        return $data;
    }

    function decode(&$data, $force = false) {
        // xml_parser_* functions to recoding from ISO-8859-1/UTF-8
        if ($force === false && (LANG_CHARSET == 'ISO-8859-1' || LANG_CHARSET == 'UTF-8')) {
            return true;
        }

        switch (strtolower(LANG_CHARSET)) {
            case 'utf-8':
                // The XML file is UTF-8 format. No changes needed.
                break;
                
            case 'iso-8859-1':
                $data = utf8_decode($data);
                break;
                
            default:
                if (function_exists('iconv')) {
                    $data = iconv('UTF-8', LANG_CHARSET, $data);
                } elseif (function_exists('recode')) {
                    $data = recode('utf-8..' . LANG_CHARSET, $data);
                }
                break;
        }
    }

    function &fetchOnline($type, $no_cache = false) {
        global $serendipity;

        switch($type) {
            // Sanitize to not fetch other URLs
            default:
            case 'event':
                $url_type = 'event';
                $i18n     = true;
                break;

            case 'sidebar':
                $url_type = 'sidebar';
                $i18n     = true;
                break;

            case 'template':
                $url_type = 'template';
                $i18n     = false;
                break;
        }
        
        if (!$i18n) {
            $lang = '';
        } elseif (isset($serendipity['languages'][$serendipity['lang']])) {
            $lang = '_' . $serendipity['lang'];
        } else {
            $lang = '_en';
        }

        $mirrors = $this->getMirrors('xml', true);
        $mirror  = $mirrors[$this->get_config('mirror_xml', 0)];

        $url    = $mirror . '/package_' . $url_type .  $lang . '.xml';
        $cacheTimeout = 60*60*12; // XML file is cached for half a day
        $target = $serendipity['serendipityPath'] . PATH_SMARTY_COMPILE . '/package_' . $url_type . $lang . '.xml';

        $xml = $this->fetchfile($url, $target, $cacheTimeout, true);
        echo '<br /><br />';

        $new_crc  = md5($xml);
        $last_crc = $this->get_config('last_crc_' . $url_type);
        
        if (!$no_cache && !$this->purgeCache && $last_crc == $new_crc) {
            $out = 'cached';
            return $out;
        }

        // XML functions
        $xml_string = '<?xml version="1.0" encoding="UTF-8" ?>';
        if (preg_match('@(<\?xml.+\?>)@imsU', $xml, $xml_head)) {
            $xml_string = $xml_head[1];
        }

        $encoding = 'UTF-8';
        if (preg_match('@encoding="([^"]+)"@', $xml_string, $xml_encoding)) {
            $encoding = $xml_encoding[1];
        }
        
        preg_match_all('@(<package version="[^"]+">.*</package>)@imsU', $xml, $xml_matches);
        if (!is_array($xml_matches)) {
            $out = 'cached';
            return $out;
        }
        
        $i = 0;
        $tree = array();
        $tree[$i] = array(
            'tag'        => 'packages',
            'attributes' => '',
            'value'      => '',
            'children'   => array()
        );
        
        foreach($xml_matches[0] as $xml_index => $xml_package) {
            $i = 0;

            switch(strtolower($encoding)) {
                case 'iso-8859-1':
                case 'utf-8':
                    $p = xml_parser_create($encoding);
                    break;
                
                default:
                    $p = xml_parser_create('');
            }

            xml_parser_set_option($p, XML_OPTION_CASE_FOLDING, 0);
            @xml_parser_set_option($this->parser, XML_OPTION_TARGET_ENCODING, LANG_CHARSET);
            $xml_package = $xml_string . "\n" . $xml_package;
            xml_parse_into_struct($p, $xml_package, $vals);
            xml_parser_free($p);
            $tree[0]['children'][] = array(
                'tag'        => $vals[$i]['tag'],
                'attributes' => $vals[$i]['attributes'],
                'value'      => $vals[$i]['value'], 
                'children'   => $this->GetChildren($vals, $i)
            );
            unset($vals);
        }
        
        $this->set_config('last_crc_' . $url_type, $new_crc);

        return $tree;
    }

    function &getCachedPlugins(&$plugins, $type) {
        global $serendipity;
        static $pluginlist = null;

        if ($pluginlist === null) {
            $pluginlist = array();
            $data = serendipity_db_query("SELECT p.*,
                                                 pc.category
                                            FROM {$serendipity['dbPrefix']}pluginlist AS p
                                 LEFT OUTER JOIN {$serendipity['dbPrefix']}plugincategories AS pc
                                              ON pc.class_name = p.class_name
                                           WHERE p.pluginlocation = 'Spartacus' AND
                                                 p.plugintype     = '" . serendipity_db_escape_string($type) . "'");

            if (is_array($data)) {
                foreach($data AS $p) {
                    $p['stackable']    = serendipity_db_bool($p['stackable']);
                    $p['requirements'] = unserialize($p['requirements']);
                    $this->checkPlugin($p, $plugins, $p['plugintype']);

                    if (!isset($pluginlist[$p['plugin_file']])) {
                        $pluginlist[$p['plugin_file']] = $p;
                    }

                    $pluginlist[$p['plugin_file']]['groups'][] = $p['category'];
                }
            }
        }
        
        return $pluginlist;
    }

    function checkPlugin(&$data, &$plugins, $type) {
        $installable = true;
        $upgradeLink = '';
        if (in_array($data['class_name'], $plugins)) {
            $infoplugin =& serendipity_plugin_api::load_plugin($data['class_name']);
            if (is_object($infoplugin)) {
                $bag    = new serendipity_property_bag;
                $infoplugin->introspect($bag);
                if ($bag->get('version') == $data['version']) {
                    $installable = false;
                } elseif (version_compare($bag->get('version'), $data['version'], '<')) {
                    $data['upgradable']      = true;
                    $data['upgrade_version'] = $data['version'];
                    $data['version']         = $bag->get('version');
                    $upgradeLink             = '&amp;serendipity[spartacus_upgrade]=true';
                }
            }
        }

        $data['installable']     = $installable;
        $data['pluginPath']      = 'online_repository';
        $data['pluginlocation']  = 'Spartacus';
        $data['plugintype']      = $type;
        $data['customURI']       = '&amp;serendipity[spartacus_fetch]=' . $type . $upgradeLink;

        return true;
    }
    
    function &buildList(&$tree, $type) {
        $plugins = serendipity_plugin_api::get_installed_plugins();

        if ($tree === 'cached') {
            return $this->getCachedPlugins($plugins, $type);
        }

        $pluginstack = array();
        $i = 0;
        
        $this->checkArray($tree);

        foreach($tree[0]['children'] AS $idx => $subtree) {
            if ($subtree['tag'] == 'package') {
                $i++;

                foreach($subtree['children'] AS $child => $childtree) {
                    switch($childtree['tag']) {
                        case 'name':
                            $pluginstack[$i]['plugin_class']    = 
                                $pluginstack[$i]['plugin_file'] = 
                                $pluginstack[$i]['class_name']  = 
                                $pluginstack[$i]['true_name']   = $childtree['value'];
                            break;

                        case 'summary':
                            $pluginstack[$i]['name']         = $childtree['value'];
                            break;

                        case 'groups':
                            $pluginstack[$i]['groups']       = explode(',', $childtree['value']);
                            break;

                        case 'description':
                            $pluginstack[$i]['description']  = $childtree['value'];
                            break;

                        case 'release':
                            $pluginstack[$i]['version']      = $childtree['children'][0]['value'];
                            
                            $pluginstack[$i]['requirements'] = array(
                                'serendipity' => '',
                                'php'         => '',
                                'smarty'      => ''
                            );

                            foreach((array)$childtree['children'] AS $relInfo) {
                                if ($relInfo['tag'] == 'requirements:s9yVersion') {
                                    $pluginstack[$i]['requirements']['serendipity'] = $relInfo['value'];
                                }
                            }
                            break;

                        case 'maintainers':
                            $pluginstack[$i]['author']       = $childtree['children'][0]['children'][0]['value']; // I dig my PHP arrays ;-)
                            break;
                    }
                }
                
                $this->checkPlugin($pluginstack[$i], $plugins, $type);

                serendipity_plugin_api::setPluginInfo($pluginstack[$i], $pluginstack[$i]['plugin_file'], $i, $i, 'Spartacus');
                // Remove the temporary $i reference, as the array should be associative
                $plugname = $pluginstack[$i]['true_name'];
                $pluginstack[$plugname] = $pluginstack[$i];
                unset($pluginstack[$i]);
            }
        }

        return $pluginstack;
    }

    function checkArray(&$tree) {
        if (!is_array($tree) || !is_array($tree[0]['children'])) {
           echo "DEBUG: Tree not an array, but: " . print_r($tree, true) . ". Please report this bug. This might be an error with the downloaded XML file. You can try to go to the plugin configuration of the Spartacus Plugin and simply click on 'Save' - this will purge all cached XML files and try to download it again.<br />\n";
        }
    }

    function &buildTemplateList(&$tree) {
        $pluginstack = array();
        $i = 0;
        
        $mirrors = $this->getMirrors('files', true);
        $mirror  = $mirrors[$this->get_config('mirror_files', 0)];
        
        $this->checkArray($tree);
        
        foreach($tree[0]['children'] AS $idx => $subtree) {
            if ($subtree['tag'] == 'package') {
                $i++;

                foreach($subtree['children'] AS $child => $childtree) {
                    switch($childtree['tag']) {
                        case 'name':
                            $pluginstack[$i]['name']         = $childtree['value'];
                            break;

                        case 'summary':
                            $pluginstack[$i]['summary']      = $childtree['value'];
                            break;

                        case 'template':
                            $pluginstack[$i]['template']  = $childtree['value'];
                            break;

                        case 'description':
                            $pluginstack[$i]['description']  = $childtree['value'];
                            break;

                        case 'release':
                            $pluginstack[$i]['version']      = $childtree['children'][0]['value'];
                            
                            $pluginstack[$i]['requirements'] = array(
                                'serendipity' => '',
                                'php'         => '',
                                'smarty'      => ''
                            );

                            foreach((array)$childtree['children'] AS $relInfo) {
                                if ($relInfo['tag'] == 'requirements:s9yVersion') {
                                    $pluginstack[$i]['requirements']['serendipity'] = $relInfo['value'];
                                }

                                if ($relInfo['tag'] == 'date') {
                                    $pluginstack[$i]['date'] = $relInfo['value'];
                                }

                            }
                            
                            $pluginstack[$i]['require serendipity'] = $pluginstack[$i]['requirements']['serendipity'];
                            break;

                        case 'maintainers':
                            $pluginstack[$i]['author']       = $childtree['children'][0]['children'][0]['value']; // I dig my PHP arrays ;-)
                            break;
                    }
                }

                $plugname = $pluginstack[$i]['template'];
                $pluginstack[$i]['previewURL'] = $mirror . '/additional_themes/' . $plugname . '/preview.png?rev=1.9999';
                $pluginstack[$i]['customURI']  = '&amp;serendipity[spartacus_fetch]=' . $plugname;
                $pluginstack[$i]['customIcon'] = '_spartacus';

                // Remove the temporary $i reference, as the array should be associative
                $pluginstack[$plugname] = $pluginstack[$i];
                unset($pluginstack[$i]);
            }
        }

        return $pluginstack;
    }

    function download(&$tree, $plugin_to_install, $sub = 'plugins') {
        global $serendipity;

        switch($sub) {
            case 'plugins':
            default:
                $sfloc = 'additional_plugins';
                break;
            
            case 'templates':
                $sfloc = 'additional_themes';
                break;
        }

        $pdir = $serendipity['serendipityPath'] . '/' . $sub . '/';
        if (!is_writable($pdir)) {
            printf(DIRECTORY_WRITE_ERROR, $pdir);
            echo '<br />';
            return false;
        }

        $files = array();
        $found = false;

        $this->checkArray($tree);

        foreach($tree[0]['children'] AS $idx => $subtree) {
            if ($subtree['tag'] != 'package') {
                continue;
            }

            foreach($subtree['children'] AS $child => $childtree) {
                if ($sub == 'templates' && $childtree['tag'] == 'template' && $childtree['value'] == $plugin_to_install) {
                    $found = true;
                } elseif ($sub == 'plugins' && $childtree['tag'] == 'name' && $childtree['value'] == $plugin_to_install) {
                    $found = true;
                }

                if (!$found || $childtree['tag'] != 'release') {
                    continue;
                }
                
                foreach($childtree['children'] AS $child2 => $childtree2) {
                    if ($childtree2['tag'] != 'serendipityFilelist') {
                        continue;
                    }

                    foreach($childtree2['children'] AS $idx => $_files) {
                        if ($_files['tag'] == 'file' && !empty($_files['value'])) {
                            $files[] = $_files['value'];
                        }
                    }
                }

                $found = false;
            }
        }

        if (count($files) == 0) {
            echo "DEBUG: ERROR: XML tree did not contain requested plugin:<br />\n";
            print_r($tree);
        }

        $mirrors = $this->getMirrors('files', true);
        $mirror  = $mirrors[$this->get_config('mirror_files', 0)];

        foreach($files AS $file) {
            $url    = $mirror . '/' . $sfloc . '/' . $file . '?rev=1.9999';
            $target = $pdir . $file;
            @mkdir($pdir . $plugin_to_install);
            $this->fetchfile($url, $target);
            if (!isset($baseDir)) {
                $baseDirs = explode('/', $file);
                $baseDir  = $baseDirs[0];
            }
        }
        
        if (isset($baseDir)) {
            return $baseDir;
        }
    }

    function event_hook($event, &$bag, &$eventData) {
        global $serendipity;

        $hooks = &$bag->get('event_hooks');

        if (isset($hooks[$event])) {
            switch($event) {
                case 'backend_templates_fetchlist':
                    $eventData = $this->buildTemplateList($this->fetchOnline('template', true), 'template');

                    return true;
                    break;

                case 'backend_templates_fetchtemplate':
                    if (!empty($eventData['GET']['spartacus_fetch'])) {
                        $this->download(
                            $this->fetchOnline('template', true), 
                            $eventData['GET']['theme'],
                            'templates'
                        );
                    }

                    return false;
                    break;
                    
                case 'backend_plugins_fetchlist':
                    $type = (isset($serendipity['GET']['type']) ? $serendipity['GET']['type'] : 'sidebar');

                    $eventData = array(
                       'pluginstack' => $this->buildList($this->fetchOnline($type), $type),
                       'errorstack'  => array(),
                       'upgradeURI'  => '&amp;serendipity[spartacus_upgrade]=true',
                       'baseURI'     => '&amp;serendipity[spartacus_fetch]=' . $type
                    );

                    return true;
                    break;

                case 'backend_plugins_fetchplugin':
                    if (!empty($eventData['GET']['spartacus_fetch'])) {
                        $baseDir = $this->download($this->fetchOnline($eventData['GET']['spartacus_fetch'], true), $eventData['GET']['install_plugin']);
                        
                        if ($baseDir === false) {
                            $eventData['install'] = false;
                        } elseif (!empty($baseDir)) {
                            $eventData['GET']['pluginPath'] = $baseDir;
                        } else {
                            $eventData['GET']['pluginPath'] = $eventData['GET']['install_plugin'];
                        }
                        
                        if ($eventData['GET']['spartacus_upgrade']) {
                            $eventData['install'] = false;
                        }
                    }

                    return false;
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
