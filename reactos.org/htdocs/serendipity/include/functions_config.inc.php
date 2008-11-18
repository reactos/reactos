<?php # $Id: functions_config.inc.php 517 2005-10-01 02:55:17Z jtate $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

function serendipity_addAuthor($username, $password, $realname, $email, $userlevel=0) {
    global $serendipity;
    $password = md5($password);
    $query = "INSERT INTO {$serendipity['dbPrefix']}authors (username, password, realname, email, userlevel)
                        VALUES  ('" . serendipity_db_escape_string($username) . "',
                                 '" . serendipity_db_escape_String($password) . "',
                                 '" . serendipity_db_escape_String($realname) . "',
                                 '" . serendipity_db_escape_String($email) . "',
                                 '" . serendipity_db_escape_String($userlevel) . "')";
    serendipity_db_query($query);
    $cid = serendipity_db_insert_id('authors', 'authorid');
    
    $data = array(
        'authorid' => $cid,
        'username' => $username,
        'realname' => $realname,
        'email'    => $email
    );

    serendipity_insertPermalink($data, 'author');
    return $cid;
}

function serendipity_deleteAuthor($authorid) {
    global $serendipity;

    if (!serendipity_checkPermission('adminUsersDelete')) {
        return false;
    }
    
    if (serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}authors WHERE authorid=" . (int)$authorid)) {
        serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}permalinks WHERE entry_id=" . (int)$authorid ." and type='author'");
    }
    return true;
}

function serendipity_remove_config_var($name, $authorid = 0) {
    global $serendipity;
    serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}config where name='" . serendipity_db_escape_string($name) . "' AND authorid = " . (int)$authorid);
}

function serendipity_set_config_var($name, $val, $authorid = 0) {
    global $serendipity;

    serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}config where name='" . serendipity_db_escape_string($name) . "' AND authorid = " . (int)$authorid);
    $r = serendipity_db_insert('config', array('name' => $name, 'value' => $val, 'authorid' => $authorid));
    
    if ($authorid === 0 || $authorid === $serendipity['authorid']) {
        if ($val === 'false') {
            $serendipity[$name] = false;
        } else {
            $serendipity[$name] = $val;
        }
    }

    if (is_string($r)) {
        echo $r;
    }
}

function serendipity_get_config_var($name, $defval = false, $empty = false) {
    global $serendipity;
    if (isset($serendipity[$name])) {
        if ($empty && gettype($serendipity[$name]) == 'string' && $serendipity[$name] === '') {
            return $defval;
        } else {
            return $serendipity[$name];
        }
    } else {
        return $defval;
    }
}

function serendipity_get_user_config_var($name, $authorid, $default = '') {
    global $serendipity;

    $author_sql = '';
    if (!empty($authorid)) {
        $author_sql = "authorid = " . (int)$authorid . " AND ";
    }

    $r = serendipity_db_query("SELECT value FROM {$serendipity['dbPrefix']}config WHERE $author_sql name = '" . $name . "' LIMIT 1", true);

    if (is_array($r)) {
        return $r[0];
    } else {
        return $default;
    }
}

function serendipity_get_user_var($name, $authorid, $default) {
    global $serendipity;

    $r = serendipity_db_query("SELECT $name FROM {$serendipity['dbPrefix']}authors WHERE authorid = " . (int)$authorid, true);

    if (is_array($r)) {
        return $r[0];
    } else {
        return $default;
    }
}

function serendipity_set_user_var($name, $val, $authorid, $copy_to_s9y = true) {
    global $serendipity;

    // When inserting a DB value, this array maps the new values to the corresponding s9y variables
    static $user_map_array = array(
        'username'  => 'serendipityUser',
        'email'     => 'serendipityEmail',
        'userlevel' => 'serendipityUserlevel'
    );

    // Special case for inserting a password
    switch($name) {
        case 'check_password':
            //Skip this field.  It doesn't need to be stored.
            return;
        case 'password':
            if (empty($val)) {
                return;
            }

            $val = md5($val);
            $copy_to_s9y = false;
            break;

        case 'right_publish':
        case 'mail_comments':
        case 'mail_trackbacks':
            $val = (serendipity_db_bool($val) ? 1 : '0');
            break;
    }

    serendipity_db_query("UPDATE {$serendipity['dbPrefix']}authors SET $name = '" . serendipity_db_escape_string($val) . "' WHERE authorid = " . (int)$authorid);

    if ($copy_to_s9y) {
        if (isset($user_map_array[$name])) {
            $key = $user_map_array[$name];
        } else {
            $key = 'serendipity' . ucfirst($name);
        }

        $_SESSION[$key] = $serendipity[$key] = $val;
    }
}

function serendipity_getTemplateFile($file, $key = 'serendipityHTTPPath') {
    global $serendipity;

    $directories = array();

    $directories[] = isset($serendipity['template']) ? $serendipity['template'] . '/' : '';
    if ( isset($serendipity['template_engine']) ) {
         $directories[] = $serendipity['template_engine'] . '/';
    }
    $directories[] = $serendipity['defaultTemplate'] .'/';
    $directories[] = 'default/';

    foreach ($directories as $directory) {
        $templateFile = $serendipity['templatePath'] . $directory . $file;

        if (file_exists($serendipity['serendipityPath'] . $templateFile)) {
            return $serendipity[$key] . $templateFile;
        }
    }
    return false;
}

function serendipity_load_configuration($author = null) {
    global $serendipity;

    if (!empty($author)) {
        // Replace default configuration directives with user-relevant data
        $rows = serendipity_db_query("SELECT name,value
                                        FROM {$serendipity['dbPrefix']}config
                                        WHERE authorid = '". (int)$author ."'");
    } else {
        // Only get default variables, user-independent (frontend)
        $rows = serendipity_db_query("SELECT name, value
                                        FROM {$serendipity['dbPrefix']}config
                                        WHERE authorid = 0");
    }

    if (is_array($rows)) {
        foreach ($rows as $row) {
            // Convert 'true' and 'false' into booleans
            $serendipity[$row['name']] = serendipity_get_bool($row['value']);
        }
    }
}

function serendipity_logout() {
    $_SESSION['serendipityAuthedUser'] = false;
    @session_destroy();
    serendipity_deleteCookie('author_information');
}

function serendipity_login($use_external = true) {
    global $serendipity;

    if (serendipity_authenticate_author('', '', false, $use_external)) {
        #The session has this data already
        #we previously just checked the value of $_SESSION['serendipityAuthedUser'] but
        #we need the authorid still, so call serendipity_authenticate_author with blank
        #params
        return true;
    }

    if (serendipity_authenticate_author($serendipity['POST']['user'], $serendipity['POST']['pass'], false, $use_external)) {
        if (empty($serendipity['POST']['auto'])) {
            serendipity_deleteCookie('author_information');
            return false;
        } else {
            $package = serialize(array('username' => $serendipity['POST']['user'],
                                       'password' => $serendipity['POST']['pass']));
            serendipity_setCookie('author_information', base64_encode($package));
            return true;
        }
    } elseif ( isset($serendipity['COOKIE']['author_information']) ) {
        $cookie = unserialize(base64_decode($serendipity['COOKIE']['author_information']));
        if (serendipity_authenticate_author($cookie['username'], $cookie['password'], false, $use_external)) {
            return true;
        } else {
            serendipity_deleteCookie('author_information');
            return false;
        }
    }
}

function serendipity_userLoggedIn() {
    if ($_SESSION['serendipityAuthedUser'] === true && IS_installed) {
        return true;
    } else {
        return false;
    }
}

function serendipity_restoreVar(&$source, &$target) {
    global $serendipity;

    if (isset($source) && !isset($target)) {
        $target = $source;
        return true;
    }

    return false;
}

function serendipity_JSsetCookie($name, $value) {
    $name  = str_replace('"', '\"', $name);
    $value = str_replace('"', '\"', $value);

    echo '<script type="text/javascript">SetCookie("' . $name . '", "' . $value . '")</script>' . "\n";
}

function serendipity_setCookie($name,$value) {
    global $serendipity;

    setcookie("serendipity[$name]", $value, time()+60*60*24*30, $serendipity['serendipityHTTPPath']);
    $_COOKIE[$name] = $value;
    $serendipity['COOKIE'][$name] = $value;
}

function serendipity_deleteCookie($name) {
    global $serendipity;

    setcookie("serendipity[$name]", '', time()-4000);
    unset($_COOKIE[$name]);
    unset($serendipity['COOKIE'][$name]);
}

require_once(ROSCMS_PATH . "logon/subsys_login.php");

function serendipity_authenticate_author($username = '', $password = '', $is_md5 = false, $use_external = true) {
    global $serendipity;

    $authorid = roscms_subsys_login('blogs',
                                    $use_external ? ROSCMS_LOGIN_REQUIRED :
                                                    ROSCMS_LOGIN_OPTIONAL,
                                    $serendipity['serendipityHTTPPath'] .
                                    ($serendipity['rewrite'] == 'none' ?
                                     $serendipity['indexFile'] .'?/' : '') .
                                    PATH_ADMIN);
    if (0 == $authorid) {
        $_SESSION['serendipityAuthedUser'] = false;
        return false;
    }
    $query = "SELECT DISTINCT
                email, authorid, userlevel, right_publish
              FROM
                {$serendipity['dbPrefix']}authors
              WHERE
                authorid = $authorid";
    $row = serendipity_db_query($query, true, 'assoc');

    if (is_array($row)) {
        serendipity_setCookie('old_session', session_id());
        $_SESSION['serendipityUser']        = $serendipity['serendipityUser']         = $row['username'];
        $_SESSION['serendipityPassword']    = $serendipity['serendipityPassword']     = '';
        $_SESSION['serendipityEmail']       = $serendipity['serendipityEmail']        = $row['email'];
        $_SESSION['serendipityAuthorid']    = $serendipity['authorid']                = $row['authorid'];
        $_SESSION['serendipityUserlevel']   = $serendipity['serendipityUserlevel']    = $row['userlevel'];
        $_SESSION['serendipityAuthedUser']  = $serendipity['serendipityAuthedUser']   = true;
        $_SESSION['serendipityRightPublish']= $serendipity['serendipityRightPublish'] = $row['right_publish'];
        serendipity_load_configuration($serendipity['authorid']);
        return true;
    } else {
        $_SESSION['serendipityAuthedUser'] = false;
        @session_destroy();
    }

    return false;
}

function serendipity_is_iframe() {
    global $serendipity;

    if ($serendipity['GET']['is_iframe'] && is_array($_SESSION['save_entry'])) {
        include_once S9Y_INCLUDE_PATH . 'include/functions_entries_admin.inc.php';
        // An iframe may NOT contain <html> and </html> tags, that's why we emit different headers here than on serendipity_admin.php
?>
    <head>
        <title><?php echo SERENDIPITY_ADMIN_SUITE; ?></title>
        <meta http-equiv="Content-Type" content="text/html; charset=<?php echo LANG_CHARSET; ?>" />
        <link rel="stylesheet" type="text/css" href="<?php echo (isset($serendipity['serendipityHTTPPath']) ? $serendipity['serendipityHTTPPath'] : ''); ?>serendipity.css.php" />
        <script type="text/javascript">
           window.onload = function() {
             parent.document.getElementById('serendipity_iframe').style.height = document.getElementById('mainpane').offsetHeight
                                                                               + parseInt(document.getElementById('mainpane').style.marginTop)
                                                                               + parseInt(document.getElementById('mainpane').style.marginBottom)
                                                                               + 'px';
             parent.document.getElementById('serendipity_iframe').scrolling    = 'no';
             parent.document.getElementById('serendipity_iframe').style.border = 0;
           }
        </script>
    </head>

    <body style="padding: 0px; margin: 0px;">
        <div id="mainpane" style="padding: 0px; margin: 5px auto 5px auto; width: 98%;">
            <div id="content" style="padding: 5px; margin: 0px;">
<?php
        // We need to restore GET/POST variables to that depending plugins inside the iframe
        // can still fetch all that variables; and we also tighten security by not allowing
        // to pass any different GET/POST variables to our iframe.
        $iframe_mode         = $serendipity['GET']['iframe_mode'];
        $serendipity['POST'] = &$_SESSION['save_entry_POST'];
        $serendipity['GET']  = &$_SESSION['save_entry_POST']; // GET-Vars are the same as POST to ensure compatibility.
        ignore_user_abort(true);
        serendipity_iframe($_SESSION['save_entry'], $iframe_mode);
?>
            </div>
        </div>
    </body>
<?php
        return true;
    }
    return false;
}

function serendipity_iframe(&$entry, $mode = null) {
    global $serendipity;

    if (empty($mode) || !is_array($entry)) {
        return false;
    }

    switch ($mode) {
        case 'save':
            echo '<div style="float: left; height: 75px"></div>';
            $res = serendipity_updertEntry($entry);

            if (is_string($res)) {
                echo '<div class="serendipity_msg_error">' . ERROR . ': <b>' . $res . '</b></div>';
            } else {
                if (!empty($serendipity['lastSavedEntry'])) {
                    // Last saved entry must be propagated to entry form so that if the user re-edits it,
                    // it needs to be stored with the new ID.
                    echo '<script type="text/javascript">parent.document.forms[\'serendipityEntry\'][\'serendipity[id]\'].value = "' . $serendipity['lastSavedEntry'] . '";</script>';
                }
                echo '<div class="serendipityAdminMsgSuccess">' . ENTRY_SAVED . '</div>';
            }
            echo '<br style="clear: both" />';

            return true;
            break;

        case 'preview':
            echo '<div style="float: left; height: 225px"></div>';
            $serendipity['smarty_raw_mode'] = true; // Force output of Smarty stuff in the backend
            serendipity_smarty_init();
            $serendipity['smarty']->assign('is_preview',  true);

            serendipity_printEntries(array($entry), ($entry['extended'] != '' ? 1 : 0), true);
            echo '<br style="clear: both" />';

            return true;
            break;
    }

    return false;
}

function serendipity_iframe_create($mode, &$entry) {
    global $serendipity;

    if (!empty($serendipity['POST']['no_save'])) {
        return true;
    }

    $_SESSION['save_entry']      = $entry;
    $_SESSION['save_entry_POST'] = $serendipity['POST'];

    $attr = '';
    switch($mode) {
        case 'save':
            $attr = ' height="100" ';
            break;

        case 'preview':
            $attr = ' height="300" ';
            break;
    }

    echo '<iframe src="serendipity_admin.php?serendipity[is_iframe]=true&amp;serendipity[iframe_mode]=' . $mode . '" id="serendipity_iframe" name="serendipity_iframe" ' . $attr . ' width="100%" frameborder="0" marginwidth="0" marginheight="0" scrolling="auto" title="Serendipity">'
         . IFRAME_WARNING
         . '</iframe><br /><br />';
}

function serendipity_probeInstallation($item) {
    global $serendipity;
    $res = NULL;

    switch ( $item ) {
        case 'dbType' :
            $res =  array();
            if (extension_loaded('mysql')) {
                $res['mysql'] = 'MySQL';
            }
            if (extension_loaded('pgsql')) {
                $res['postgres'] = 'PostgreSQL';
            }
            if (extension_loaded('mysqli')) {
                $res['mysqli'] = 'MySQLi';
            }
            if (extension_loaded('sqlite')) {
                $res['sqlite'] = 'SQLite';
            }
            break;

        case 'rewrite' :
            $res = array();
            $res['none'] = 'Disable URL Rewriting';
            $res['errordocs'] = 'Use Apache errorhandling';
            if( !function_exists('apache_get_modules') || in_array('mod_rewrite', apache_get_modules()) ) {
                $res['rewrite'] = 'Use Apache mod_rewrite';
            }
            break;
    }

    return $res;
}

function serendipity_header($header) {
    if (!headers_sent()) {
        header($header);
    }
}

/* TODO:
  This previously was handled inside a plugin with an event hook, but caching
  the event plugins that early in sequence created trouble with plugins not
  having loaded the right language.
  Find a way to let plugins hook into that sequence :-) */
function serendipity_getSessionLanguage() {
    global $serendipity;

    // Store default language
    $serendipity['default_lang'] = $serendipity['lang'];

    if ($_SESSION['serendipityAuthedUser']) {
        serendipity_header('X-Serendipity-InterfaceLangSource: Database');
        return $serendipity['lang'];
    }

    if (isset($_REQUEST['user_language']) && (!empty($serendipity['languages'][$_REQUEST['user_language']])) && !headers_sent()) {
        serendipity_setCookie('serendipityLanguage', $_REQUEST['user_language']);
    }

    if (isset($serendipity['COOKIE']['serendipityLanguage'])) {
        serendipity_header('X-Serendipity-InterfaceLangSource: Cookie');
        $lang = $serendipity['COOKIE']['serendipityLanguage'];
    } elseif (!empty($serendipity['languages'][$serendipity['GET']['lang_selected']])) {
        serendipity_header('X-Serendipity-InterfaceLangSource: GET');
        $lang = $serendipity['GET']['lang_selected'];
    } elseif (serendipity_db_bool($serendipity['lang_content_negotiation'])) {
        serendipity_header('X-Serendipity-InterfaceLangSource: Content-Negotiation');
        $lang = serendipity_detectLang();
    }

    if (!isset($lang) || !isset($serendipity['languages'][$lang])) {
        $lang = $serendipity['lang'];
    }

    serendipity_header('X-Serendipity-InterfaceLang: ' . $lang);

    if ($lang != $serendipity['lang']) {
        $serendipity['content_lang'] = $lang;
    }

    return $lang;
}

function &serendipity_getPermissions($authorid) {
    global $serendipity;

        // Get group information
        $groups = serendipity_db_query("SELECT ag.groupid, g.name, gc.property, gc.value
                                          FROM {$serendipity['dbPrefix']}authorgroups AS ag
                               LEFT OUTER JOIN {$serendipity['dbPrefix']}groups AS g
                                            ON ag.groupid = g.id
                               LEFT OUTER JOIN {$serendipity['dbPrefix']}groupconfig AS gc
                                            ON gc.id = g.id
                                         WHERE ag.authorid = " . (int)$authorid);
        $perm = array('membership' => array());
        if (is_array($groups)) {
            foreach($groups AS $group) {
                $perm['membership'][$group['groupid']]       = $group['groupid'];
                $perm[$group['groupid']][$group['property']] = $group['value'];
            }
        }
        return $perm;
}

function serendipity_getPermissionNames() {
    return array(
        'personalConfiguration'  
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF, USERLEVEL_EDITOR),
        'personalConfigurationUserlevel'  
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),
        'personalConfigurationNoCreate'  
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),
        'personalConfigurationRightPublish'  
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),
        'siteConfiguration'      
            => array(USERLEVEL_ADMIN),
        'blogConfiguration'      
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),

        'adminEntries'           
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF, USERLEVEL_EDITOR),
        'adminEntriesMaintainOthers' 
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),

        'adminImport'
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),

        'adminCategories'
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF, USERLEVEL_EDITOR),
        'adminCategoriesMaintainOthers'
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),
        'adminCategoriesDelete'
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),

        'adminUsers'             
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),
        'adminUsersDelete'       
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),
        'adminUsersEditUserlevel'             
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),
        'adminUsersMaintainSame'             
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),
        'adminUsersMaintainOthers'             
            => array(USERLEVEL_ADMIN),
        'adminUsersCreateNew'             
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),
        'adminUsersGroups'             
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),

        'adminPlugins'           
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),
        'adminPluginsMaintainOthers'           
            => array(USERLEVEL_ADMIN),

        'adminImages'            
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF, USERLEVEL_EDITOR),
        'adminImagesDirectories' 
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),
        'adminImagesAdd' 
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF, USERLEVEL_EDITOR),
        'adminImagesDelete'     
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF, USERLEVEL_EDITOR),
        'adminImagesMaintainOthers' 
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),
        'adminImagesViewOthers' 
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF, USERLEVEL_EDITOR),
        'adminImagesView' 
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF, USERLEVEL_EDITOR),
        'adminImagesSync' 
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),

        'adminComments'          
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),

        'adminTemplates'         
            => array(USERLEVEL_ADMIN, USERLEVEL_CHIEF),
    );
}

function serendipity_checkPermission($permName, $authorid = null, $returnMyGroups = false) {
    global $serendipity;
    
    // Define old serendipity permissions
    static $permissions = null;
    static $group = null;
    
    if (IS_installed !== true) {
        return true;
    }

    if ($permissions === null) {
        $permissions = serendipity_getPermissionNames();
    }

    if ($group === null) {
        $group = array();
    }
    
    if ($authorid === null) {
        $authorid = $serendipity['authorid'];
    }
    
    if (!isset($group[$authorid])) {
        $group[$authorid] = serendipity_getPermissions($authorid);
    }
    
    if ($returnMyGroups) {
        return $group[$authorid]['membership'];
    }

    if ($authorid == $serendipity['authorid'] && $serendipity['no_create']) {
        // This no_create user privilege overrides other permissions.
        return false;
    }

    $return = true;

    foreach($group[$authorid] AS $item) {
        if (!isset($item[$permName])) {
            continue;
        }

        if ($item[$permName] === 'true') {
            return true;
        } else {
            $return = false;
        }
    }
    
    // If the function did not yet return it means there's a check for a permission which is not defined anywhere.
    // Let's use a backwards compatible way.
    if ($return && isset($permissions[$permName]) && in_array($serendipity['serendipityUserlevel'], $permissions[$permName])) {
        return true;
    }
    
    return false;
}

function serendipity_updateGroups($groups, $authorid, $apply_acl = true) {
    global $serendipity;

    if ($apply_acl && !serendipity_checkPermission('adminUsersMaintainOthers')) {
        return false;
    }

    serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}authorgroups WHERE authorid = " . (int)$authorid);

    foreach($groups AS $group) {
        serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}authorgroups (authorid, groupid) VALUES (" . (int)$authorid . ", " . (int)$group . ")"); 
    }
    return true;
}

function &serendipity_getAllGroups($apply_ACL_user = false) {
    global $serendipity;

    if ($apply_ACL_user) {
        $groups =& serendipity_db_query("SELECT g.id   AS confkey, 
                                                g.name AS confvalue,
                                                g.id   AS id,
                                                g.name AS name
                                           FROM {$serendipity['dbPrefix']}authorgroups AS ag
                                LEFT OUTER JOIN {$serendipity['dbPrefix']}groups AS g
                                             ON g.id = ag.groupid
                                          WHERE ag.authorid = " . (int)$apply_ACL_user . "
                                       ORDER BY g.name", false, 'assoc');
    } else {
        $groups =& serendipity_db_query("SELECT g.id   AS confkey, 
                                                g.name AS confvalue,
                                                g.id   AS id,
                                                g.name AS name
                                          FROM {$serendipity['dbPrefix']}groups AS g
                                      ORDER BY  g.name", false, 'assoc');
    }
    if (is_array($groups)) {
        foreach ($groups as $k => $v) {
            if ('USERLEVEL_' == substr($v['confvalue'], 0, 10)) {
                $groups[$k]['confvalue'] = $groups[$k]['name'] = constant($v['confvalue']);
            }
        }
    }
    return $groups;
}

function &serendipity_fetchGroup($groupid) {
    global $serendipity;

    $conf = array();
    $groups =& serendipity_db_query("SELECT g.id        AS confkey, 
                                            g.name      AS confvalue,
                                            g.id        AS id,
                                            g.name      AS name,

                                            gc.property AS property,
                                            gc.value    AS value
                                      FROM {$serendipity['dbPrefix']}groups AS g
                           LEFT OUTER JOIN {$serendipity['dbPrefix']}groupconfig AS gc
                                        ON g.id = gc.id
                                     WHERE g.id = " . (int)$groupid, false, 'assoc');
    foreach($groups AS $group) {
        $conf[$group['property']] = $group['value'];
    }
    
    // The following are unique
    $conf['name']      = $groups[0]['name'];
    $conf['id']        = $groups[0]['id'];
    $conf['confkey']   = $groups[0]['confkey'];
    $conf['confvalue'] = $groups[0]['confvalue'];

    return $conf;
}


function &serendipity_getGroups($authorid, $sequence = false) {
    global $serendipity;

    $groups =& serendipity_db_query("SELECT g.id   AS confkey, 
                                            g.name AS confvalue,
                                            g.id   AS id,
                                            g.name AS name
                                      FROM {$serendipity['dbPrefix']}authorgroups AS ag
                           LEFT OUTER JOIN {$serendipity['dbPrefix']}groups AS g
                                        ON g.id = ag.groupid
                                     WHERE ag.authorid = " . (int)$authorid, false, 'assoc');
    if (!is_array($groups)) {
        $groups = array();
    }
    
    if ($sequence) {
        $_groups = $groups;
        $groups  = array();
        foreach($_groups AS $grouprow) {
            $groups[] = $grouprow['confkey'];
        }
    }

    return $groups;
}

function &serendipity_getGroupUsers($groupid) {
    global $serendipity;

    $groups =& serendipity_db_query("SELECT g.name     AS name,
                                            a.realname AS author,
                                            a.authorid AS id
                                      FROM {$serendipity['dbPrefix']}authorgroups AS ag
                           LEFT OUTER JOIN {$serendipity['dbPrefix']}groups AS g
                                        ON g.id = ag.groupid
                           LEFT OUTER JOIN {$serendipity['dbPrefix']}authors AS a
                                        ON ag.authorid = a.authorid
                                     WHERE ag.groupid = " . (int)$groupid, false, 'assoc');
    return $groups;
}

function serendipity_deleteGroup($groupid) {
    global $serendipity;

    if (!serendipity_checkPermission('adminUsersGroups')) {
        return false;
    }
    
    if (!serendipity_checkPermission('adminUsersMaintainOthers')) {
        // Only groups should be accessible where a user has access rights.
        $my_groups = serendipity_getGroups($serendipity['authorid'], true);
        if (!in_array($groupid, $my_groups)) {
            return false;
        }
    } 

    serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}groups       WHERE id = " . (int)$groupid);
    serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}authorgroups WHERE groupid = " . (int)$groupid);

    return true;
}

function serendipity_addGroup($name) {
    global $serendipity;

    serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}groups (name) VALUES ('" . serendipity_db_escape_string($name) . "')");
    $gid = serendipity_db_insert_id('groups', 'id');

    return $gid;
}

function &serendipity_getDBPermissionNames() {
    global $serendipity;
    
    $config =& serendipity_db_query("SELECT property FROM {$serendipity['dbPrefix']}groupconfig GROUP BY property ORDER BY property", false, 'assoc');

    return $config;
}

function &serendipity_getAllPermissionNames() {
    global $serendipity;

    $DBperms =& serendipity_getDBPermissionNames();
    $perms   =& serendipity_getPermissionNames();
    
    foreach($DBperms AS $perm) {
        if (!isset($perms[$perm['property']])) {
            $perms[$perm['property']] = array();
        }
    }

    return $perms;
}

function serendipity_intersectGroup($checkuser = null, $myself = null) {
    global $serendipity;

    if ($myself === null) {
        $myself = $serendipity['authorid'];
    }
    
    $my_groups  = serendipity_getGroups($myself, true);
    $his_groups = serendipity_getGroups($checkuser, true);

    foreach($his_groups AS $his_group) {
        if (in_array($his_group, $my_groups)) {
            return true;
        }
    }
    
    return false;
}

function serendipity_updateGroupConfig($groupid, &$perms, &$values) {
    global $serendipity;

    if (!serendipity_checkPermission('adminUsersGroups')) {
        return false;
    }

    if (!serendipity_checkPermission('adminUsersMaintainOthers')) {
        // Only groups should be accessible where a user has access rights.
        $my_groups = serendipity_getGroups($serendipity['authorid'], true);
        if (!in_array($groupid, $my_groups)) {
            return false;
        }
    } 

    $storage = serendipity_fetchGroup($groupid);    

    serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}groupconfig WHERE id = " . (int)$groupid); 
    foreach ($perms AS $perm => $userlevels) {
        if (isset($values[$perm]) && $values[$perm] == 'true') {
            $value = 'true';
        } elseif (isset($values[$perm]) && $values[$perm] === 'false') {
            $value = 'false';
        } elseif (isset($values[$perm])) {
            $value = $values[$perm];
        } else {
            $value = 'false';
        }
        
        if (!serendipity_checkPermission($perm)) {
            if (!isset($storage[$perm])) {
                $value = 'false';
            } else {
                $value = $storage[$perm];
            }
        }

        serendipity_db_query(
            sprintf("INSERT INTO {$serendipity['dbPrefix']}groupconfig (id, property, value) VALUES (%d, '%s', '%s')",
                (int)$groupid,
                serendipity_db_escape_string($perm),
                serendipity_db_escape_string($value)
            )
        );
    }
    
    serendipity_db_query("UPDATE {$serendipity['dbPrefix']}groups SET name = '" . serendipity_db_escape_string($values['name']) . "' WHERE id = " . (int)$groupid);

    if (is_array($values['members'])) {
        serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}authorgroups WHERE groupid = " . (int)$groupid);
        foreach($values['members'] AS $member) {
            serendipity_db_query(
                sprintf("INSERT INTO {$serendipity['dbPrefix']}authorgroups (groupid, authorid) VALUES (%d, %d)",
                    (int)$groupid,
                    (int)$member
                )
            );
        }
    }

    return true;
}

function serendipity_addDefaultGroup($name, $level) {
    global $serendipity;

    static $perms = null;
    if ($perms === null) {
        $perms = serendipity_getPermissionNames();
    }
    
    serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}groups (name) VALUES ('" . serendipity_db_escape_string($name) . "')");
    $gid = (int)serendipity_db_insert_id('groups', 'id');
    serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}groupconfig (id, property, value) VALUES ($gid, 'userlevel', '" . (int)$level . "')");

    $authors = serendipity_db_query("SELECT * FROM {$serendipity['dbPrefix']}authors WHERE userlevel = " . (int)$level);
    
    if (is_array($authors)) {
        foreach($authors AS $author) {
            serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}authorgroups (authorid, groupid) VALUES ('{$author['authorid']}', '$gid')");
        }
    }    

    foreach($perms AS $permName => $permArray) {
        if (in_array($level, $permArray)) {
            serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}groupconfig (id, property, value) VALUES ($gid, '" . serendipity_db_escape_string($permName) . "', 'true')");
        } else {
            serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}groupconfig (id, property, value) VALUES ($gid, '" . serendipity_db_escape_string($permName) . "', 'false')");
        }
    }
    
    return true;
}

function serendipity_ACLGrant($artifact_id, $artifact_type, $artifact_mode, $groups) {
    global $serendipity;

    if (empty($groups) || !is_array($groups)) {
        return false;
    }
    
    // Delete all old existing relations.
    serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}access
                                WHERE artifact_id   = " . (int)$artifact_id . " 
                                  AND artifact_type = '" . serendipity_db_escape_string($artifact_type) . "'
                                  AND artifact_mode = '" . serendipity_db_escape_string($artifact_mode) . "'");

    $data = array(
        'artifact_id'    => (int)$artifact_id,
        'artifact_type'  => $artifact_type,
        'artifact_mode'  => $artifact_mode,
        'artifact_index' => ''
    );

    if (count($data) < 1) {
        return true;
    }

    foreach($groups AS $group) {
        $data['groupid'] = $group;
        serendipity_db_insert('access', $data);
    }
    
    return true;
}

function serendipity_ACLGet($artifact_id, $artifact_type, $artifact_mode) {
    global $serendipity;
    
    $sql = "SELECT groupid, artifact_index FROM {$serendipity['dbPrefix']}access
                    WHERE artifact_type = '" . serendipity_db_escape_string($artifact_type) . "'
                      AND artifact_id   = '" . (int)$artifact_id . "'
                      AND artifact_mode = '" . serendipity_db_escape_string($artifact_mode) . "'";
    $rows = serendipity_db_query($sql, false, 'assoc');
    
    if (!is_array($rows)) {
        return false;
    }

    $acl = array();
    foreach($rows AS $row) {
        $acl[$row['groupid']] = $row['artifact_index'];
    }
    
    return $acl;
}

function serendipity_ACLCheck($authorid, $artifact_id, $artifact_type, $artifact_mode) {
    global $serendipity;
    
    $artifact_sql = array();
    
    // TODO: If more artifact_types are available, the JOIN needs to be edited so that the first AND portion is not required, and the join is fully made on that conditiion. 
    switch($artifact_type) {
        default:
        case 'category':
            $artifact_sql['unique']= "atf.categoryid";
            $artifact_sql['cond']  = "atf.categoryid = " . (int)$artifact_id;
            $artifact_sql['where'] = "     ag.groupid = a.groupid 
                                        OR a.groupid  = 0 
                                        OR (a.artifact_type IS NULL AND (atf.authorid = " . (int)$authorid . " OR atf.authorid = 0 OR atf.authorid IS NULL))";
            $artifact_sql['table'] = 'category';
    }
    
    $sql = "SELECT {$artifact_sql['unique']} AS result
              FROM {$serendipity['dbPrefix']}{$artifact_sql['table']} AS atf
   LEFT OUTER JOIN {$serendipity['dbPrefix']}authorgroups AS ag
                ON ag.authorid = ". (int)$authorid . "
   LEFT OUTER JOIN {$serendipity['dbPrefix']}access AS a 
                ON (    a.artifact_type = '" . serendipity_db_escape_string($artifact_type) . "' 
                    AND a.artifact_id   = " . (int)$artifact_id . "
                    AND a.artifact_mode = '" . serendipity_db_escape_string($artifact_mode) . "' 
                   )

             WHERE {$artifact_sql['cond']} 
               AND ( {$artifact_sql['where']} )
          GROUP BY result";

    $res = serendipity_db_query($sql, true, 'assoc');
    if (is_array($res) && !empty($res['result'])) {
        return true;
    }
    
    return false;
}

function serendipity_ACL_SQL(&$cond, $append_category = false) {
    global $serendipity;

    if (!isset($serendipity['enableACL']) || $serendipity['enableACL'] == true) {
        if ($_SESSION['serendipityAuthedUser'] === true) {
            $read_id = (int)$serendipity['authorid'];
            $read_id_sql = 'acl_a.groupid OR acl_acc.groupid = 0';
        } else {
            // "0" as category property counts as "anonymous viewers" 
            $read_id     = 0;
            $read_id_sql = 0;
        }

        if ($append_category) {
            if ($append_category !== 'limited') {
                $cond['joins'] .= " LEFT JOIN {$serendipity['dbPrefix']}entrycat ec
                                           ON e.id = ec.entryid";
            }
            
            $cond['joins'] .= " LEFT JOIN {$serendipity['dbPrefix']}category c
                                       ON ec.categoryid = c.categoryid";
        }

        $cond['joins'] .= " LEFT JOIN {$serendipity['dbPrefix']}authorgroups AS acl_a
                                   ON acl_a.authorid = " . $read_id . "
                            LEFT JOIN {$serendipity['dbPrefix']}access AS acl_acc
                                   ON (    acl_acc.artifact_mode = 'read' 
                                       AND acl_acc.artifact_type = 'category'
                                       AND acl_acc.artifact_id   = c.categoryid 
                                      )";

        if (empty($cond['and'])) {
            $cond['and'] .= ' WHERE ';
        } else {
            $cond['and'] .= ' AND ';
        }

        // When in Admin-Mode, apply readership permissions.
        $cond['and'] .= "    (
                                 c.categoryid IS NULL 
                                 OR ( acl_acc.groupid = " . $read_id_sql . ")
                                 OR ( acl_acc.artifact_id IS NULL 
                                      " . (isset($serendipity['GET']['adminModule']) && 
                                           $serendipity['GET']['adminModule'] == 'entries' && 
                                           !serendipity_checkPermission('adminEntriesMaintainOthers') 
                                        ? "AND (c.authorid IS NULL OR c.authorid = 0 OR c.authorid = " . $read_id . ")"
                                        : "") . "
                                    )
                               )";
        return true;
    }    

    return false;
}

function serendipity_checkXSRF() {
    global $serendipity;

    // If no module was requested, the user has just logged in and no action will be performed.
    if (empty($serendipity['GET']['adminModule'])) {
        return false;
    }

    // The referrer was empty. Deny access.
    if (empty($_SERVER['HTTP_REFERER'])) {
        echo serendipity_reportXSRF(1, true, true);
        return false;
    }
    
    // Parse the Referrer host. Abort if not parseable.
    $hostinfo = @parse_url($_SERVER['HTTP_REFERER']);
    if (!is_array($hostinfo)) {
        echo serendipity_reportXSRF(2, true, true);
        return true;
    }

    // Get the server against we will perform the XSRF check.
    $server = '';
    if (empty($_SERVER['HTTP_HOST'])) {
        $myhost = @parse_url($serendipity['baseURL']);
        if (is_array($myhost)) {
            $server = $myhost['host'];
        }
    } else {
        $server = $_SERVER['HTTP_HOST'];
    }

    // If the current server is different than the referred server, deny access.
    if ($hostinfo['host'] != $server) {
        echo serendipity_reportXSRF(3, true, true);
        return true;
    }
    
    return false;
}

function serendipity_reportXSRF($type = 0, $reset = true, $use_config = false) {
    global $serendipity;

    // Set this in your serendipity_config_local.inc.php if you want HTTP Referrer blocking:
    // $serendipity['referrerXSRF'] = true;

    $string = '<div class="serendipityAdminMsgError XSRF_' . $type . '">' . ERROR_XSRF . '</div>';
    if ($reset) {
        // Config key "referrerXSRF" can be set to enable blocking based on HTTP Referrer. Recommended for Paranoia.
        if (($use_config && isset($serendipity['referrerXSRF']) && $serendipity['referrerXSRF']) || $use_config === false) {
            $serendipity['GET']['adminModule'] = '';
        } else {
            // Paranoia not enabled. Do not report XSRF.
            $string = '';
        }
    }
    return $string;
}

function serendipity_checkFormToken() {
    global $serendipity;
    
    $token = '';
    if (!empty($serendipity['POST']['token'])) {
        $token = $serendipity['POST']['token'];
    } elseif (!empty($serendipity['GET']['token'])) {
        $token = $serendipity['GET']['token'];
    }

    if (empty($token)) {
        echo serendipity_reportXSRF('token', false);
        return false;
    }

    if ($token != md5(session_id()) &&
        $token != md5($serendipity['COOKIE']['old_session'])) {
        echo serendipity_reportXSRF('token', false);
        return false;
    }
    
    return true;
}

function serendipity_setFormToken($type = 'form') {
    global $serendipity;
    
    if ($type == 'form') {
        return '<input type="hidden" name="serendipity[token]" value="' . md5(session_id()) . '" />';
    } elseif ($type == 'url') {
        return 'serendipity[token]=' . md5(session_id());
    } else {
        return md5(session_id());
    }
}

/* vim: set sts=4 ts=4 expandtab : */
