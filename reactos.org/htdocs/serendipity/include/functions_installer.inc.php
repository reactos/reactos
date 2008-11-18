<?php # $Id: functions_installer.inc.php 709 2005-11-17 15:15:11Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details


function serendipity_ini_bool($var) {
    return ($var === 'on' || $var == '1');
}

function serendipity_ini_bytesize($val) {
    if ( $val == '' )
        return 0;

    switch(substr($val, -1)) {
        case 'k':
        case 'K':
            return (int) $val * 1024;
            break;
        case 'm':
        case 'M':
            return (int) $val * 1048576;
            break;
        default:
            return $val;
   }
}

function serendipity_updateLocalConfig($dbName, $dbPrefix, $dbHost, $dbUser, $dbPass, $dbType, $dbPersistent, $privateVariables = null) {
    global $serendipity;
    umask(0000);

    $file = 'serendipity_config_local.inc.php';
    $path = $serendipity['serendipityPath'];

    $oldconfig = @file_get_contents($path . $file);
    $configfp  = fopen($path . $file, 'w');

    if (!is_resource($configfp)) {
        $errs[] = sprintf(FILE_WRITE_ERROR, $file);
        $errs[] = sprintf(DIRECTORY_RUN_CMD, 'chown -R www:www', $path) . ' (' . WWW_USER . ')';
        $errs[] = sprintf(DIRECTORY_RUN_CMD, 'chmod 770'       , $path);
        $errs[] = BROWSER_RELOAD;

        return $errs;
    }

    if (isset($_POST['sqlitedbName']) && !empty($_POST['sqlitedbName'])) {
        $dbName = $_POST['sqlitedbName'];
    }

    $file_start    = "<?php\n"
                   . "\t/*\n"
                   . "\t  Serendipity configuration file\n";
    $file_mark     = "\n\t// End of Serendipity configuration file"
                   . "\n\t// You can place your own special variables after here:\n";
    $file_end      = "\n?>\n";
    $file_personal = '';

    preg_match('@' . preg_quote($file_start) . '.*' . preg_quote($file_mark) . '(.+)' . preg_quote($file_end) . '@imsU', $oldconfig, $match);
    if (!empty($match[1])) {
        $file_personal = $match[1];
    }

    fwrite($configfp, $file_start);

    fwrite($configfp, "\t  Written on ". date('r') ."\n");
    fwrite($configfp, "\t*/\n\n");

    fwrite($configfp, "\t\$serendipity['versionInstalled']  = '{$serendipity['version']}';\n");
    fwrite($configfp, "\t\$serendipity['dbName']            = '{$dbName}';\n");
    fwrite($configfp, "\t\$serendipity['dbPrefix']          = '{$dbPrefix}';\n");
    fwrite($configfp, "\t\$serendipity['dbHost']            = '{$dbHost}';\n");
    fwrite($configfp, "\t\$serendipity['dbUser']            = '{$dbUser}';\n");
    fwrite($configfp, "\t\$serendipity['dbPass']            = '{$dbPass}';\n");
    fwrite($configfp, "\t\$serendipity['dbType']            = '{$dbType}';\n");
    fwrite($configfp, "\t\$serendipity['dbPersistent']      = ". (serendipity_db_bool($dbPersistent) ? 'true' : 'false') .";\n");

    if (is_array($privateVariables) && count($privateVariables) > 0) {
        foreach($privateVariables AS $p_idx => $p_val) {
            fwrite($configfp, "\t\$serendipity['{$p_idx}']  = '{$p_val}';\n");
        }
    }

    fwrite($configfp, $file_mark .  $file_personal . $file_end);

    fclose($configfp);

    @chmod($path . $file, 0700);
    return true;
}

/**
* Creates the needed tables - beware, they will be empty and need to be stuffed with
* default templates and such...
*/
function serendipity_installDatabase() {
  global $serendipity;
  $queries = serendipity_parse_sql_tables(S9Y_INCLUDE_PATH . 'sql/db.sql');
  $queries = str_replace('{PREFIX}', $serendipity['dbPrefix'], $queries);

  foreach ($queries as $query) {
      serendipity_db_schema_import($query);
  }
}

function serendipity_query_default($optname, $default, $usertemplate = false, $type = 'string') {
    global $serendipity;

    /* I won't tell you the password, it's MD5 anyway, you can't do anything with it */
    if ($type == 'protected' && IS_installed === true) {
        return '';
    }

    switch ($optname) {
        case 'permalinkStructure':
            return $default;

        case 'dbType' :
            if (extension_loaded('mysqli')) {
                $type = 'mysqli';
            }
            if (extension_loaded('pgsql')) {
                $type = 'postgres';
            }
            if (extension_loaded('mysql')) {
                $type = 'mysql';
            }
            return $type;

        case 'serendipityPath':
            $test_path1 = $_SERVER['DOCUMENT_ROOT'] . rtrim(dirname($_SERVER['PHP_SELF']), '/') . '/';
            $test_path2 = serendipity_getRealDir(__FILE__);
            if (file_exists($test_path1 . 'serendipity_admin.php')) {
                return $test_path1;
            } else {
                return $test_path2;
            }

        case 'serendipityHTTPPath':
            return rtrim(dirname($_SERVER['PHP_SELF']), '/') .'/';

        case 'baseURL':
            $ssl  = isset($_SERVER['HTTPS']) && $_SERVER['HTTPS'] == 'on';
            $port = $_SERVER['SERVER_PORT'];

            return sprintf('http%s://%s%s%s',

                            $ssl ? 's' : '',
                            preg_replace('@^([^:]+):?.*$@', '\1', $_SERVER['HTTP_HOST']),
                            (($ssl && $port != 443) || (!$ssl && $port != 80)) ? (':' . $port) : '',
                            rtrim(dirname($_SERVER['PHP_SELF']), '/') .'/'
                   );

        case 'convert':
            $path = array();

            $path[] = ini_get('safe_mode_exec_dir');

            if (isset($_SERVER['PATH'])) {
                $path = array_merge($path, explode(PATH_SEPARATOR, $_SERVER['PATH']));
            }

            /* add some other possible locations to the path while we are at it,
             * as these are not always included in the apache path */
            $path[] = '/usr/X11R6/bin';
            $path[] = '/usr/bin';
            $path[] = '/usr/local/bin';

            foreach ($path as $dir) {
                if (!empty($dir) && (function_exists('is_executable') && @is_executable($dir . '/convert')) || @is_file($dir . '/convert')) {
                    return $dir . '/convert';
                }

                if (!empty($dir) && (function_exists('is_executable') && @is_executable($dir . '/convert.exe')) || @is_file($dir . '/convert.exe')) {
                    return $dir . '/convert.exe';
                }
            }
            return $default;

        case 'rewrite':
            return serendipity_check_rewrite($default);

        default:
            if ($usertemplate) {
                return serendipity_get_user_var($optname, $serendipity['authorid'], $default);
            }

            return $default;
    }
}

function serendipity_parseTemplate($filename, $areas = null, $onlyFlags=null) {
    global $serendipity;

    $userlevel = $serendipity['serendipityUserlevel'];

    if ( !IS_installed ) {
        $userlevel = USERLEVEL_ADMIN;
    }

    $config = @include($filename);

    foreach ( $config as $n => $category ) {
        /* If $areas is an array, we filter out those categories, not within the array */
        if ( is_array($areas) && !in_array($n, $areas) ) {
            unset($config[$n]);
            continue;
        }

        foreach ( $category['items'] as $i => $item ) {
            $items = &$config[$n]['items'][$i];

            if (!isset($items['userlevel']) || !is_numeric($items['userlevel'])) {
                $items['userlevel'] = USERLEVEL_ADMIN;
            }

            if (!isset($items['permission']) && $userlevel < $items['userlevel']) {
                unset($config[$n]['items'][$i]);
                continue;
            } elseif (!is_array($items['permission']) && !serendipity_checkPermission($items['permission'])) {
                unset($config[$n]['items'][$i]);
                continue;
            } elseif (is_array($items['permission'])) {
                $one_found = false;
                $all_found = true;
                foreach($items['permission'] AS $check_permission) {
                    if (serendipity_checkPermission($check_permission)) {
                        $one_found = true;
                    } else {
                        $all_found = false;
                    }
                }

                if (!isset($items['perm_mode'])) {
                    $items['perm_mode'] = 'or';
                }

                if ($items['perm_mode'] == 'or' && !$one_found) {
                    unset($config[$n]['items'][$i]);
                    continue;
                } elseif ($items['perm_mode'] == 'and' && !$one_found && !$all_found) {
                    unset($config[$n]['items'][$i]);
                    continue;
                }
            }

            if (!isset($items['flags']) || !is_array($items['flags'])) {
                $items['flags'] = array();
            }

            if ( is_array($onlyFlags) ) {
                foreach ( $onlyFlags as $onlyFlag ) {
                    if ( !in_array($onlyFlag, $items['flags']) ) {
                        unset($config[$n]['items'][$i]);
                        continue;
                    }
                }
            }
        }

        if (sizeof($config[$n]['items']) < 1) {
            unset($config[$n]);
        }
    }

    return $config;
}

function serendipity_replaceEmbeddedConfigVars ($s) {
    return str_replace(
                  array(
                    '%clock%'
                  ),

                  array(
                    date('H:i')
                  ),

                  $s);
}

function serendipity_guessInput($type, $name, $value='', $default='') {
    global $serendipity;

    switch ($type) {
        case 'bool':
            $value = serendipity_get_bool($value);
            echo '<input id="radio_cfg_' . $name . '_yes" type="radio" name="' . $name . '" value="true" ';
            echo (($value == true) ? 'checked="checked"' : ''). ' /><label for="radio_cfg_' . $name . '_yes"> ' . YES . '</label>&nbsp;';
            echo '<input id="radio_cfg_' . $name . '_no" type="radio" name="' . $name . '" value="false" ';
            echo (($value == true) ? '' : 'checked="checked"'). ' /><label for="radio_cfg_' . $name . '_no"> ' . NO . '</label>';
            break;

        case 'protected':
            echo '<input type="password" size="30" name="' . $name . '" value="' . htmlspecialchars($value) . '" />';
            break;

        case 'multilist':
            echo '<select name="'. $name .'[]" multiple="multiple">';
            foreach ((array)$default as $k => $v) {
                print_r($v);
                $selected = false;
                foreach((array)$value AS $vk => $vv) {
                    if ($vv['confkey'] == $v['confkey']) {
                        $selected = true;
                    }
                }

                printf('<option value="%s"%s>%s</option>'. "\n",
                      $v['confkey'],
                      ($selected ? ' selected="selected"' : ''),
                      $v['confvalue']);
            }
            echo '</select>';
            break;

        case 'list':
            echo '<select name="'. $name .'">';

            foreach ((array)$default as $k => $v) {
                $selected = ($k == $value);

                printf('<option value="%s"%s>%s</option>'. "\n",
                      $k,
                      ($selected ? ' selected="selected"' : ''),
                      $v);
            }
            echo '</select>';
            break;

        case 'file':
            echo '<input type="file" size="30" name="' . $name . '" />';
            break;

        default:
            echo '<input type="text" size="30" name="' . $name . '" value="' . htmlspecialchars($value) . '" />';
            break;
    }
}

function serendipity_printConfigTemplate($config, $from = false, $noForm = false, $folded = true, $allowToggle = true) {
    global $serendipity;
    if (!isset($serendipity['XHTML11'])) {
        $serendipity['XHTML11'] = FALSE;
    }
    if ( $allowToggle ) {
?>
<script type="text/javascript" language="JavaScript">
function showConfig(id) {
    if (document.getElementById) {
        el = document.getElementById(id);
        if (el.style.display == 'none') {
            document.getElementById('option' + id).src = '<?php echo serendipity_getTemplateFile('img/minus.png') ?>';
            el.style.display = '';
        } else {
            document.getElementById('option' + id).src = '<?php echo serendipity_getTemplateFile('img/plus.png') ?>';
            el.style.display = 'none';
        }
    }
}

var state='<?php echo ($folded === true ? '' : 'none'); ?>';
function showConfigAll(count) {
    if (document.getElementById) {
        for (i = 1; i <= count; i++) {
            document.getElementById('el' + i).style.display = state;
            document.getElementById('optionel' + i).src = (state == '' ? '<?php echo serendipity_getTemplateFile('img/minus.png') ?>' : '<?php echo serendipity_getTemplateFile('img/plus.png') ?>');
        }

        if (state == '') {
            document.getElementById('optionall').src = '<?php echo serendipity_getTemplateFile('img/minus.png') ?>';
            state = 'none';
        } else {
            document.getElementById('optionall').src = '<?php echo serendipity_getTemplateFile('img/plus.png') ?>';
            state = '';
        }
    }
}
</script>

<?php
    }

    if (!$noForm) {
?>
<form action="?" method="POST">
    <div>
        <input type="hidden" name="serendipity[adminModule]" value="installer" />
        <input type="hidden" name="installAction" value="check" />
        <?php echo serendipity_setFormToken(); ?>
        <br />
<?php   }
    if (sizeof($config) > 1 && $allowToggle) { ?>
        <div align="right">
            <a style="border:0; text-decoration: none" href="#" onClick="showConfigAll(<?php echo sizeof($config); ?>)" title="<?php echo TOGGLE_ALL; ?>"><img src="<?php echo serendipity_getTemplateFile('img/'. ($folded === true ? 'plus' : 'minus') .'.png') ?>" id="optionall" alt="+/-" border="0" />&nbsp;<?php echo TOGGLE_ALL; ?></a></a><br />
        </div>
<?php
    }
    $el_count = 0;
    foreach ($config as $category) {
        $el_count++;
?>
        <table width="100%" cellspacing="2">
<?php
        if (sizeof($config) > 1) {
?>
            <tr>
                <th align="left" colspan="2" style="padding-left: 15px;">
<?php if ( $allowToggle ) { ?>
                    <a style="border:0; text-decoration: none;" href="#" onClick="showConfig('el<?php echo $el_count; ?>'); return false" title="<?php echo TOGGLE_OPTION; ?>"><img src="<?php echo serendipity_getTemplateFile('img/'. ($folded === true ? 'plus' : 'minus') .'.png') ?>" id="optionel<?php echo $el_count; ?>" alt="+/-" border="0" />&nbsp;<?php echo $category['title']; ?></a>
<?php } else { ?>
                    <?php echo $category['title']; ?>
<?php } ?>
                </th>
            </tr>
<?php   } ?>
            <tr>
                <td>
                    <table width="100%" cellspacing="0" cellpadding="3" id="el<?php echo $el_count; ?>">
                        <tr>
                            <td style="padding-left: 20px;" colspan="2">
                                <?php echo $category['description'] ?>
                            </td>
                        </tr>

<?php
        foreach ( $category['items'] as $item ) {

            $value = $from[$item['var']];

            /* Calculate value if we are not installed, how clever :) */
            if ($from == false) {
                $value = serendipity_query_default($item['var'], $item['default']);
            }

            /* Check for installOnly flag */
            if ( in_array('installOnly', $item['flags']) && IS_installed === true ) {
                continue;
            }

            if ( in_array('hideValue', $item['flags']) ) {
                $value = '';
            }

            if (in_array('config', $item['flags']) && isset($from['authorid'])) {
                $value = serendipity_get_user_config_var($item['var'], $from['authorid'], $item['default']);
            }

            if (in_array('parseDescription', $item['flags'])) {
                $item['description'] = serendipity_replaceEmbeddedConfigVars($item['description']);
            }

            if (in_array('probeDefault', $item['flags'])) {
                $item['default'] = serendipity_probeInstallation($item['var']);
            }

            if (in_array('ifEmpty', $item['flags']) && empty($value)) {
                $value = serendipity_query_default($item['var'], $item['default']);
            }
?>
                        <tr>
                            <td style="border-bottom: 1px #000000 solid" align="left" valign="top" width="75%">
                                <strong><?php echo $item['title']; ?></strong>
                                <br />
                                <span style="color: #5E7A94; font-size: 8pt;"><?php echo $item['description']; ?></span>
                            </td>
                            <td style="border-bottom: 1px #000000 solid; font-size: 8pt" align="left" valign="middle" width="25%">
                                <?php echo ($serendipity['XHTML11'] ? '<span style="white-space: nowrap">' : '<nobr>'); ?><?php echo serendipity_guessInput($item['type'], $item['var'], $value, $item['default']); ?><?php echo ($serendipity['XHTML11'] ? '</span>' : '</nobr>'); ?>
                            </td>
                        </tr>
<?php
        }
?>
                    </table><br /><br />
                </td>
            </tr>
        </table>
<?php
    }

    if ($folded && $allowToggle) {
        echo '<script type="text/javascript" language="JavaScript">';
        for ($i = 1; $i <= $el_count; $i++) {
            echo 'document.getElementById("el' . $i . '").style.display = "none";' . "\n";
        }
        echo '</script>';
    }

    if (!$noForm) {
?>
        <input type="submit" value="<?php echo CHECK_N_SAVE; ?>" class="serendipityPrettyButton" />
    </div>
</form>
<?php
    }
}

function serendipity_parse_sql_tables($filename) {
    $in_table = 0;
    $queries = array();

    $fp = fopen($filename, 'r', 1);
    if ($fp) {
        while (!@feof($fp)) {
            $line = trim(fgets($fp, 4096));
            if ($in_table) {
                $def .= $line;
                if (preg_match('/^\)\s*(type\=\S+|\{UTF_8\})?\s*\;$/i', $line)) {
                    $in_table = 0;
                    array_push($queries, $def);
                }
            } else {
                if (preg_match('#^create table \{PREFIX\}\S+\s*\(#i', $line)) {
                    $in_table = 1;
                    $def = $line;
                }

                if (preg_match('#^create\s*(\{fulltext\}|unique|\{fulltext_mysql\})?\s*index#i', $line)) {
                    array_push($queries, $line);
                }
            }
        }
        fclose($fp);
    }

    return $queries;
}

function serendipity_checkInstallation() {
    global $serendipity, $umask;

    $errs = array();

    serendipity_initPermalinks();

    // Check dirs
    if (!is_dir($_POST['serendipityPath'])) {
        $errs[] = sprintf(DIRECTORY_NON_EXISTANT, $_POST['serendipityPath']);
    }
    elseif (!is_writable($_POST['serendipityPath']) ) {
        $errs[] = sprintf(DIRECTORY_WRITE_ERROR, $_POST['serendipityPath']);
    }
    elseif (!is_dir($_POST['serendipityPath'] . $_POST['uploadPath'] ) && @mkdir($_POST['serendipityPath'] . $_POST['uploadPath'], $umask) !== true) {
        $errs[] = sprintf(DIRECTORY_CREATE_ERROR, $_POST['serendipityPath'] . $_POST['uploadPath']);
    }
    elseif (!is_writable($_POST['serendipityPath'] . $_POST['uploadPath'])) {
        $errs[] = sprintf(DIRECTORY_WRITE_ERROR, $_POST['serendipityPath'] . $_POST['uploadPath']);
        $errs[] = sprintf(DIRECTORY_RUN_CMD    , 'chmod go+rws', $_POST['serendipityPath'] . $_POST['uploadPath']);
    }

    // Attempt to create the template compile directory, it might already be there, but we just want to be sure
    if (!is_dir($_POST['serendipityPath'] . PATH_SMARTY_COMPILE) && @mkdir($_POST['serendipityPath'] . PATH_SMARTY_COMPILE, $umask) !== true) {
        $errs[] = sprintf(DIRECTORY_CREATE_ERROR, $_POST['serendipityPath'] . PATH_ARCHIVES);
        $errs[] = sprintf(DIRECTORY_RUN_CMD     , 'mkdir'      , $_POST['serendipityPath'] . PATH_SMARTY_COMPILE);
        $errs[] = sprintf(DIRECTORY_RUN_CMD     , 'chmod go+rwx', $_POST['serendipityPath'] . PATH_SMARTY_COMPILE);
    } elseif (is_dir($_POST['serendipityPath'] . PATH_SMARTY_COMPILE) && !is_writeable($_POST['serendipityPath'] . PATH_SMARTY_COMPILE) && @chmod($_POST['serendipityPath'] . PATH_SMARTY_COMPILE, $umask) !== true) {
        $errs[] = sprintf(DIRECTORY_RUN_CMD     , 'chmod go+rwx', $_POST['serendipityPath'] . PATH_SMARTY_COMPILE);
    }

    // Attempt to create the archives directory
    if (!is_dir($_POST['serendipityPath'] . PATH_ARCHIVES) && @mkdir($_POST['serendipityPath'] . PATH_ARCHIVES, $umask) !== true) {
        $errs[] = sprintf(DIRECTORY_CREATE_ERROR, $_POST['serendipityPath'] . PATH_ARCHIVES);
        $errs[] = sprintf(DIRECTORY_RUN_CMD     , 'mkdir'      , $_POST['serendipityPath'] . PATH_ARCHIVES);
        $errs[] = sprintf(DIRECTORY_RUN_CMD     , 'chmod go+rwx', $_POST['serendipityPath'] . PATH_ARCHIVES);
    }

    // Check imagick
    if ($_POST['magick'] == 'true' && function_exists('is_executable') && !@is_executable($_POST['convert'])) {
        $errs[] = sprintf(CANT_EXECUTE_BINARY, 'convert imagemagick');
    }

    if ($_POST['dbType'] == 'sqlite') {
        // We don't want that our SQLite db file can be guessed from other applications on a server
        // and have access to our's. So we randomize the SQLite dbname.
        $_POST['sqlitedbName'] = $_POST['dbName'] . '_' . md5(time());
    }

    if (empty($_POST['dbPrefix']) && empty($serendipity['dbPrefix'])) {
        $errs[] = sprintf(EMPTY_SETTING, INSTALL_DBPREFIX);
    }

    $serendipity['dbType'] = $_POST['dbType'];
    // Probe database
    // (do it after the dir stuff, as we need to be able to create the sqlite database)
    include_once($_POST['serendipityPath'] . 'include/db/db.inc.php');
    // For shared installations, probe the file on include path
    include_once(S9Y_INCLUDE_PATH . 'include/db/db.inc.php');

    if (S9Y_DB_INCLUDED) {
        serendipity_db_probe($_POST, $errs);
    }

    return (count($errs) > 0 ? $errs : '');
}

function serendipity_installFiles($serendipity_core = '') {
    global $serendipity;

    // This variable is transmitted from serendipity_admin_installer. If an empty variable is used,
    // this means that serendipity_installFiles() was called from the auto-updater facility.
    if (empty($serendipity_core)) {
        $serendipity_core = $serendipity['serendipityPath'];
    }

    $htaccess = @file_get_contents($serendipity_core . '.htaccess');

    // Let this function be callable outside installation and let it use existing settings.
    $import = array('rewrite', 'serendipityHTTPPath', 'indexFile');
    foreach($import AS $key) {
        if (empty($_POST[$key]) && isset($serendipity[$key])) {
            $$key = $serendipity[$key];
        } else {
            $$key = $_POST[$key];
        }
    }

    if (php_sapi_name() == 'cgi' || php_sapi_name() == 'cgi-fcgi') {
        $htaccess_cgi = '_cgi';
    } else {
        $htaccess_cgi = '';
    }


    /* Detect comptability with php_value directives */
    if ($htaccess_cgi == '') {
        $response = '';
        $serendipity_root = dirname($_SERVER['PHP_SELF']) . '/';
        $serendipity_host = preg_replace('@^([^:]+):?.*$@', '\1', $_SERVER['HTTP_HOST']);

        $old_htaccess = @file_get_contents($serendipity_core . '.htaccess');
        $fp = @fopen($serendipity_core . '.htaccess', 'w');
        if ($fp) {
            fwrite($fp, 'php_value register_globals off'. "\n" .'php_value session.use_trans_sid 0');
            fclose($fp);

            $sock = @fsockopen($serendipity_host, $_SERVER['SERVER_PORT'], $errorno, $errorstring, 10);
            if ($sock) {
                fputs($sock, "GET {$serendipityHTTPPath} HTTP/1.0\r\n");
                fputs($sock, "Host: $serendipity_host\r\n");
                fputs($sock, "User-Agent: Serendipity/{$serendipity['version']}\r\n");
                fputs($sock, "Connection: close\r\n\r\n");

                while (!feof($sock) && strlen($response) < 4096) {
                    $response .= fgets($sock, 400);
                }
                fclose($sock);
            }

            /* If we get HTTP 500 Internal Server Error, we have to use the .cgi template */
            if (preg_match('@^HTTP/\d\.\d 500@', $response)) {
                $htaccess_cgi = '_cgi';
            }

            if (!empty($old_htaccess)) {
                $fp = @fopen($serendipity_core . '.htaccess', 'w');
                fwrite($fp, $old_htaccess);
                fclose($fp);
            } else {
                @unlink($serendipity_core . '.htaccess');
            }
        }
    }


    if ($rewrite == 'rewrite') {
        $template = 'htaccess' . $htaccess_cgi . '_rewrite.tpl';
    } elseif ($rewrite == 'errordocs') {
        $template = 'htaccess' . $htaccess_cgi . '_errordocs.tpl';
    } else {
        $template = 'htaccess' . $htaccess_cgi . '_normal.tpl';
    }

    if (!($a = file(S9Y_INCLUDE_PATH . 'include/tpl/' . $template, 1))) {
        $errs[] = ERROR_TEMPLATE_FILE;
    }

    // When we write this file we cannot rely on the constants defined
    // earlier, as they do not yet contain the updated contents from the
    // new config. Thus we re-define those. We do still use constants
    // for backwards/code compatibility.

    $PAT = serendipity_permalinkPatterns(true);

    $content = str_replace(
                 array(
                   '{PREFIX}',
                   '{indexFile}',
                   '{PAT_UNSUBSCRIBE}', '{PATH_UNSUBSCRIBE}',
                   '{PAT_ARCHIVES}', '{PATH_ARCHIVES}',
                   '{PAT_FEEDS}', '{PATH_FEEDS}',
                   '{PAT_FEED}',
                   '{PAT_ADMIN}', '{PATH_ADMIN}',
                   '{PAT_ARCHIVE}', '{PATH_ARCHIVE}',
                   '{PAT_PLUGIN}', '{PATH_PLUGIN}',
                   '{PAT_DELETE}', '{PATH_DELETE}',
                   '{PAT_APPROVE}', '{PATH_APPROVE}',
                   '{PAT_SEARCH}', '{PATH_SEARCH}',
                   '{PAT_CSS}',
                   '{PAT_PERMALINK}',
                   '{PAT_PERMALINK_AUTHORS}',
                   '{PAT_PERMALINK_FEEDCATEGORIES}',
                   '{PAT_PERMALINK_CATEGORIES}',
                   '{PAT_PERMALINK_FEEDAUTHORS}'
                 ),

                 array(
                   $serendipityHTTPPath,
                   $indexFile,
                   trim($PAT['UNSUBSCRIBE'], '@/i'), $serendipity['permalinkUnsubscribePath'],
                   trim($PAT['ARCHIVES'], '@/i'),    $serendipity['permalinkArchivesPath'],
                   trim($PAT['FEEDS'], '@/i'),       $serendipity['permalinkFeedsPath'],
                   trim(PAT_FEED, '@/i'),
                   trim($PAT['ADMIN'], '@/i'),       $serendipity['permalinkAdminPath'],
                   trim($PAT['ARCHIVE'], '@/i'),     $serendipity['permalinkArchivePath'],
                   trim($PAT['PLUGIN'], '@/i'),      $serendipity['permalinkPluginPath'],
                   trim($PAT['DELETE'], '@/i'),      $serendipity['permalinkDeletePath'],
                   trim($PAT['APPROVE'], '@/i'),     $serendipity['permalinkApprovePath'],
                   trim($PAT['SEARCH'], '@/i'),      $serendipity['permalinkSearchPath'],
                   trim(PAT_CSS, '@/i'),
                   trim($PAT['PERMALINK'], '@/i'),
                   trim($PAT['PERMALINK_AUTHORS'], '@/i'),
                   trim($PAT['PERMALINK_FEEDCATEGORIES'], '@/i'),
                   trim($PAT['PERMALINK_CATEGORIES'], '@/i'),
                   trim($PAT['PERMALINK_FEEDAUTHORS'], '@/i')
                 ),

                 implode('', $a)
              );

    $fp = @fopen($serendipity_core . '.htaccess', 'w');
    if (!$fp) {
        $errs[] = sprintf(FILE_WRITE_ERROR, $serendipity_core . '.htaccess') . ' ' . FILE_CREATE_YOURSELF;
        $errs[] = sprintf(COPY_CODE_BELOW , $serendipity_core . '.htaccess', 'serendipity', htmlspecialchars($content));
        return $errs;
    } else {
        // Check if an old htaccess file existed and try to preserve its contents. Otherwise completely wipe the file.
        if ($htaccess != '' && preg_match('@^(.*)#\s+BEGIN\s+s9y.*#\s+END\s+s9y(.*)$@isU', $htaccess, $match)) {
            // Code outside from s9y-code was found.
            fwrite($fp, $match[1] . $content . $match[2]);
        } else {
            fwrite($fp, $content);
        }
        fclose($fp);
        return true;
    }

}

/* Takes the item data from a config var, and checks the flags against the current area */
function serendipity_checkConfigItemFlags(&$item, $area) {

    if ( in_array('nosave', $item['flags']) ) {
        return false;
    }

    if ( in_array('local', $item['flags']) && $area == 'configuration' ) {
        return false;
    }

    if ( in_array('config', $item['flags']) && $area == 'local' ) {
        return false;
    }

    return true;
}

function serendipity_updateConfiguration() {
    global $serendipity, $umask;

    // Save all basic config variables to the database
    $config = serendipity_parseTemplate(S9Y_CONFIG_TEMPLATE);

    if (isset($_POST['sqlitedbName']) && !empty($_POST['sqlitedbName'])) {
        $_POST['dbName'] = $_POST['sqlitedbName'];
    }

    // Password can be hidden in re-configuring, but we need to store old password
    if (empty($_POST['dbPass']) && !empty($serendipity['dbPass'])) {
        $_POST['dbPass'] = $serendipity['dbPass'];
    }

    foreach($config as $category) {
        foreach ( $category['items'] as $item ) {

            /* Don't save trash */
            if ( !serendipity_checkConfigItemFlags($item, 'configuration') ) {
                continue;
            }

            if (!isset($item['userlevel'])) {
                $item['userlevel'] = USERLEVEL_ADMIN;
            }

            // Check permission set. Changes to blogConfiguration or siteConfiguration items
            // always required authorid = 0, so that it be not specific to a userlogin
            if ( $serendipity['serendipityUserlevel'] >= $item['userlevel'] || IS_installed === false ) {
                $authorid = 0;
            } elseif ($item['permission'] == 'blogConfiguration' && serendipity_checkPermission('blogConfiguration')) {
                $authorid = 0;
            } elseif ($item['permission'] == 'siteConfiguration' && serendipity_checkPermission('siteConfiguration')) {
                $authorid = 0;
            } else {
                $authorid = $serendipity['authorid'];
            }

            if (is_array($_POST[$item['var']])) {
                // Arrays not allowed. Use first index value.
                list($a_key, $a_val) = each($_POST[$item['var']]);
                $_POST[$item['var']] = $a_key;

                // If it still is an array, munge it all together.
                if (is_array($_POST[$item['var']])) {
                    $_POST[$item['var']] = @implode(',', $_POST[$item['var']]);
                }
            }

            serendipity_set_config_var($item['var'], $_POST[$item['var']], $authorid);
        }
    }

    if (IS_installed === false || serendipity_checkPermission('siteConfiguration')) {
        return serendipity_updateLocalConfig($_POST['dbName'],
                                             $_POST['dbPrefix'],
                                             $_POST['dbHost'],
                                             $_POST['dbUser'],
                                             $_POST['dbPass'],
                                             $_POST['dbType'],
                                             $_POST['dbPersistent']);
    } else {
        return true;
    }
}

function serendipity_httpCoreDir() {
    if (!empty($_SERVER['SCRIPT_FILENAME']) && substr(php_sapi_name(), 0, 3) != 'cgi') {
        return dirname($_SERVER['SCRIPT_FILENAME']) . '/';
    }

    return $_SERVER['DOCUMENT_ROOT'] . dirname($_SERVER['PHP_SELF']) . '/';
}

function serendipity_removeFiles($files = null) {
    global $serendipity, $errors;

    if (!is_array($files)) {
        return;
    }

    $backupdir = S9Y_INCLUDE_PATH . 'backup';
    if (!is_dir($backupdir)) {
        @mkdir($backupdir, 0777);
        if (!is_dir($backupdir)) {
            $errors[] = sprintf(DIRECTORY_CREATE_ERROR, $backupdir);
            return false;
        }
    }

    if (!is_writable($backupdir)) {
        $errors[] = sprintf(DIRECTORY_WRITE_ERROR, $backupdir);
        return false;
    }

    foreach($files AS $file) {
        $source   = S9Y_INCLUDE_PATH . $file;
        $sanefile = str_replace('/', '_', $file);
        $target   = $backupdir . '/' . $sanefile;

        if (!file_exists($source)) {
            continue;
        }

        if (file_exists($target)) {
            $target = $backupdir . '/' . time() . '.' . $sanefile; // Backupped file already exists. Append with timestamp as name.
        }

        if (!is_writable($source)) {
            $errors[] = sprintf(FILE_WRITE_ERROR, $source) . '<br />';
        } else {
            rename($source, $target);
        }
    }
}

function serendipity_getRealDir($file) {
    $dir = str_replace( "\\", "/", dirname($file));
    $base = preg_replace('@/include$@', '', $dir) . '/';
    return $base;
}

function serendipity_check_rewrite($default) {
    global $serendipity;

    if (IS_installed == true) {
        return $default;
    }

    $serendipity_root = dirname($_SERVER['PHP_SELF']) . '/';
    $serendipity_core = serendipity_httpCoreDir();
    $old_htaccess     = @file_get_contents($serendipity_core . '.htaccess');
    $fp               = @fopen($serendipity_core . '.htaccess', 'w');
    $serendipity_host = preg_replace('@^([^:]+):?.*$@', '\1', $_SERVER['HTTP_HOST']);

    if (!$fp) {
        printf(HTACCESS_ERROR,
          '<b>chmod go+rwx ' . getcwd() . '/</b>'
        );
        return $default;
    } else {
        fwrite($fp, 'ErrorDocument 404 ' . $serendipity_root . 'index.php');
        fclose($fp);

        // Do a request on a nonexistant file to see, if our htaccess allows ErrorDocument
        $sock = @fsockopen($serendipity_host, $_SERVER['SERVER_PORT'], $errorno, $errorstring, 10);
        $response = '';

        if ($sock) {
            fputs($sock, "GET {$_SERVER['PHP_SELF']}nonexistant HTTP/1.0\r\n");
            fputs($sock, "Host: $serendipity_host\r\n");
            fputs($sock, "User-Agent: Serendipity/{$serendipity['version']}\r\n");
            fputs($sock, "Connection: close\r\n\r\n");

            while (!feof($sock) && strlen($response) < 4096) {
                $response .= fgets($sock, 400);
            }
            fclose($sock);
        }

        if (preg_match('@^HTTP/\d\.\d 200@', $response) && preg_match('@X\-Blog: Serendipity@', $response)) {
            $default = 'errordocs';
        } else {
            $default = 'none';
        }

        if (!empty($old_htaccess)) {
            $fp = @fopen($serendipity_core . '.htaccess', 'w');
            fwrite($fp, $old_htaccess);
            fclose($fp);
        } else {
            @unlink($serendipity_core . '.htaccess');
        }

        return $default;
    }
}

function serendipity_removeObsoleteVars() {
global $serendipity;

    $config = serendipity_parseTemplate(S9Y_CONFIG_TEMPLATE);
    foreach($config as $category) {
        foreach($category['items'] as $item) {
            /* Remove trash */
            if (!serendipity_checkConfigItemFlags($item, 'remove')) {
                serendipity_remove_config_var($item['var'], 0);
            }
        }
    }
}

?>
