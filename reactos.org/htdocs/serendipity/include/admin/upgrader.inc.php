<?php # $Id: upgrader.inc.php 146 2005-06-05 20:39:34Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (IN_serendipity !== true) {
    die ('Don\'t hack!');
}

require_once(S9Y_INCLUDE_PATH . 'include/functions_installer.inc.php');
require_once(S9Y_INCLUDE_PATH . 'include/functions_upgrader.inc.php');

define('S9Y_U_ERROR', -1);
define('S9Y_U_WARNING', 0);
define('S9Y_U_SUCCESS', 1);

function serendipity_upgraderResultDiagnose($result, $s) {
    global $errorCount;

    if ( $result === S9Y_U_SUCCESS ) {
        return '<span style="color: green; font-weight: bold">'. $s .'</span>';
    }

    if ( $result === S9Y_U_WARNING ) {
        return '<span style="color: orange; font-weight: bold">'. $s .'</span>';
    }

    if ( $result === S9Y_U_ERROR ) {
        $errorCount++;
        return '<span style="color: red; font-weight: bold">'. $s .'</span>';
    }
}

// Setting this value to 'FALSE' is recommended only for SHARED BLOG INSTALLATIONS. This enforces all shared blogs with a common
// codebase to only allow upgrading, no bypassing and thus causing instabilities.
// This variable can also be set as $serendipity['UpgraderShowAbort'] inside serendipity_config_local.inc.php to prevent
// your setting being changed when updating serendipity in first place.
$showAbort  = (isset($serendipity['UpgraderShowAbort']) ? $serendipity['UpgraderShowAbort'] : true);

$abortLoc   = $serendipity['serendipityHTTPPath'] . 'serendipity_admin.php?serendipity[action]=ignore';
$upgradeLoc = $serendipity['serendipityHTTPPath'] . 'serendipity_admin.php?serendipity[action]=upgrade';

/* Functions which needs to be run if installed version is equal or lower */
$tasks = array(array('version'   => '0.5.1',
                     'function'  => 'serendipity_syncThumbs',
                     'title'     => 'Image Sync',
                     'desc'      => 'Version 0.5.1 introduces image sync with the database'. "\n" .
                                    'With your permission I would like to perform the image sync'),

               array('version'   => '0.6.5',
                     'function'  => 'serendipity_rebuildCategoryTree',
                     'title'     => 'Nested subcategories, post to multiple categories',
                     'desc'      => 'This update will update the categories table of your database and update the relations from entries to categories.'. "\n" .
                                    'This is a possibly dangerous task to perform, so <strong style="color: red">make sure you have a backup of your database!</strong>'),

               array('version'   => '0.6.8',
                     'function'  => 'serendipity_installFiles',
                     'title'     => 'Update of .htaccess file',
                     'desc'      => 'Changes were made to the .htaccess file, you need to regenerate it'),

               array('version'   => '0.6.10',
                     'functon'   => 'serendipity_installFiles',
                     'title'     => 'Update of .htaccess file',
                     'desc'      => 'Changes were made to the .htaccess file, you need to regenerate it'),

               array('version'   => '0.6.12',
                     'function'  => 'serendipity_installFiles',
                     'title'     => 'Update of .htaccess file',
                     'desc'      => 'Changes were made to the .htaccess file, you need to regenerate it'),

               array('version'   => '0.8-alpha3',
                     'function'  => 'serendipity_removeFiles',
                     'title'     => 'Removal of obsolete files',
                     'arguments' => array($obsolete_files),
                     'desc'      => 'The directory structure has been reworked. The following files will be moved to a folder called "backup". If you made manual changes to those files, be sure to read the file docs/CHANGED_FILES to re-implement your changes.<br /><div style="font-size: x-small; margin: 15px">' . implode(', ', $obsolete_files) . '</div>'),

               array('version'   => '0.8-alpha4',
                     'function'  => 'serendipity_removeFiles',
                     'title'     => 'Removal of serendipity_entries.php',
                     'arguments' => array(array('serendipity_entries.php')),
                     'desc'      => 'In order to implement the new administration, we have to remove the leftovers'),

               array('version'   => '0.8-alpha4',
                     'function'  => 'serendipity_installFiles',
                     'title'     => 'Update of .htaccess file',
                     'desc'      => 'In order to implement the new administration, changes were made to the .htaccess file, you need to regenerate it'),

               array('version'   => '0.8-alpha7',
                     'function'  => 'serendipity_removeObsoleteVars',
                     'title'     => 'Removal of obsolete configuration variables',
                     'desc'      => 'Because of the new configuration parsing methods, some database variables are now only stored in serendipity_config_local.inc.php. Those obsolete variables will be removed from the database'),

               array('version'   => '0.8-alpha8',
                     'function'  => array('serendipity_plugin_api', 'create_plugin_instance'),
                     'arguments' => array('serendipity_event_browsercompatibility', null, 'event'),
                     'title'     => 'Plugin for Browser Compatibility',
                     'desc'      => 'Includes some CSS-behaviours and other functions to maximize browser compatibility'),

               array('version'   => '0.8-alpha9',
                     'function'  => 'serendipity_installFiles',
                     'title'     => 'Update of .htaccess file',
                     'desc'      => 'In order to implement author views, changes were made to the .htaccess file, you need to regenerate it'),

               array('version'   => '0.8-alpha11',
                     'function'  => 'serendipity_installFiles',
                     'title'     => 'Update of .htaccess file',
                     'desc'      => 'In order to implement URL rewrite improvement, changes were made to the .htaccess file, you need to regenerate it'),

               array('version'   => '0.8-alpha12',
                     'type'      => 'TEMPLATE_NOTICE',
                     'function'  => '',
                     'title'     => '<b>TEMPLATE_NOTICE:</b> The template file "entries.tpl" has changed.',
                     'desc'      => 'Authors can now have longer real names instead of only their loginnames. Those new fields need to be displayed in your Template, if you manually created one. Following variables were changes:
                                     <b>{$entry.username}</b> =&gt; <b>{$entry.author}</b>
                                     <b>{$entry.link_username}</b> =&gt; <b>{$entry.link_author}</b>
                                     Those variables have been replaced in all bundled templates and those in our additional_themes repository.
                                     ' . serendipity_upgraderResultDiagnose(S9Y_U_WARNING, 'Manual user interaction is required! This can NOT be done automatically!')),

               array('version'   => '0.8-beta3',
                     'function'  => 'serendipity_fixPlugins',
                     'arguments' => array('markup_column_names'),
                     'title'     => 'Configuration options of markup plugins',
                     'desc'      => 'Because of the latest multilingual improvements in Serendipity, the database key names for certain configuration directives only found in markup plugins need to be renamed.<br />'
                                    . 'This will be automatically handled by Serendipity for all internally bundled and external plugins. If you are using the external plugins "GeShi" and "Markdown", please make sure you will upgrade to their latest versions!<br />'
                                    . 'We also advise that you check the plugin configuration of all your markup plugins (like emoticate, nl2br, s9ymarkup, bbcode) and see if the settings you made are all properly migrated.'),

               array('version'   => '0.8-beta5',
                     'function'  => 'serendipity_smarty_purge',
                     'title'     => 'Clear Smarty compiled templates',
                     'desc'      => 'Smarty has been upgraded to its latest stable version, and we therefore need to purge all compiled templates and cache'),

               array('version'   => '0.9-alpha2',
                     'function'  => 'serendipity_buildPermalinks',
                     'title'     => 'Build permalink patterns',
                     'desc'      => 'This version introduces user-configurable Permalinks and needs to pre-cache the list of all permalinks to be later able to fetch the corresponding entries for a permalink.'),

               array('version'   => '0.9-alpha3',
                     'function'  => 'serendipity_addDefaultGroups',
                     'title'     => 'Introduce author groups',
                     'desc'      => 'This version introduces customizable user groups. Your existing users will be migrated into the new default groups.'),

);

/* Fetch SQL files which needs to be run */
$dir      = opendir(S9Y_INCLUDE_PATH . 'sql/');
$tmpfiles = array();
while (($file = readdir($dir)) !== false ) {
    if (preg_match('@db_update_(.*)_(.*)_(.*).sql@', $file, $res)) {
        list(, $verFrom, $verTo, $dbType) = $res;
        if (version_compare($verFrom, $serendipity['versionInstalled']) >= 0) {
            $tmpFiles[$verFrom][$dbType] = $file;
        }
    }
}

$sqlfiles = array();
if (is_array($tmpFiles)) {
    foreach ($tmpFiles as $version => $db) {
        if (array_key_exists($serendipity['dbType'], $db) === false ) {
            $sqlfiles[$version] = $db['mysql'];
        } else {
            $sqlfiles[$version] = $db[$serendipity['dbType']];
        }
    }
}

@uksort($sqlfiles, "strnatcasecmp");

if ($serendipity['GET']['action'] == 'ignore') {
    /* Todo: Don't know what to put here? */

} elseif ($serendipity['GET']['action'] == 'upgrade') {

    $errors = array();

    /* Install SQL files */
    foreach ($sqlfiles as $sqlfile) {
        $sql = file_get_contents(S9Y_INCLUDE_PATH .'sql/'. $sqlfile);
        $sql = str_replace('{PREFIX}', $serendipity['dbPrefix'], $sql);
        preg_match_all("@(.*);@iUs", $sql, $res);
        foreach ($res[0] as $sql) {
            $r = serendipity_db_schema_import($sql);
            if (is_string($r)) {
                $errors[] = trim($r);
            }
        }
    }

    /* Call functions */
    foreach ($tasks as $task) {
        if (!empty($task['function']) && version_compare($serendipity['versionInstalled'], $task['version'], '<') ) {
            if (is_callable($task['function'])) {
                echo sprintf('Calling %s ...<br />', (is_array($task['function']) ? $task['function'][0] . '::'. $task['function'][1] : $task['function']));;

                if (empty($task['arguments'])) {
                    call_user_func($task['function']);
                } else {
                    call_user_func_array($task['function'], $task['arguments']);
                }
            } else {
                $errors[] = 'Unable to call '. $task['function'];
            }
        }
    }

    if (sizeof($errors)) {
        echo DIAGNOSTIC_ERROR . '<br /><br />';
        echo '<span class="serendipityAdminMsgError">- ' . implode('<br />', $errors) . '</span><br /><br />';
    }

    /* I don't care what you told me, I will always nuke Smarty cache */
    serendipity_smarty_purge();

}

if (($showAbort && $serendipity['GET']['action'] == 'ignore') || $serendipity['GET']['action'] == 'upgrade') {
    $privateVariables = array();
    if (isset($serendipity['UpgraderShowAbort'])) {
        $privateVariables['UpgraderShowAbort'] = $serendipity['UpgraderShowAbort'];
    }

    $r = serendipity_updateLocalConfig(
           $serendipity['dbName'],
           $serendipity['dbPrefix'],
           $serendipity['dbHost'],
           $serendipity['dbUser'],
           $serendipity['dbPass'],
           $serendipity['dbType'],
           $serendipity['dbPersistent'],
           $privateVariables
    );

    if ($serendipity['GET']['action'] == 'ignore') {
        echo SERENDIPITY_UPGRADER_YOU_HAVE_IGNORED;
    } elseif ($serendipity['GET']['action'] == 'upgrade') {
        printf('<div class="serendipityAdminMsgSuccess">'. SERENDIPITY_UPGRADER_NOW_UPGRADED .'</div>', $serendipity['version']);
    }
    echo '<br />';
    printf('<div align="center">'. SERENDIPITY_UPGRADER_RETURN_HERE .'</div>', '<a href="'. $serendipity['serendipityHTTPPath'] .'">', '</a>');
    $_SESSION['serendipityAuthedUser'] = false;
    @session_destroy();
} else {
    echo '<h2>' . SERENDIPITY_UPGRADER_WELCOME . '</h2>';
    printf(SERENDIPITY_UPGRADER_PURPOSE . '<br />', $serendipity['versionInstalled']);
    printf(SERENDIPITY_UPGRADER_WHY . '.', $serendipity['version']);
    echo '<br />' . FIRST_WE_TAKE_A_LOOK  . '.';
?>
<br /><br />
<div align="center"><?php printf(ERRORS_ARE_DISPLAYED_IN, serendipity_upgraderResultDiagnose(S9Y_U_ERROR, RED), serendipity_upgraderResultDiagnose(S9Y_U_WARNING, YELLOW), serendipity_upgraderResultDiagnose(S9Y_U_SUCCESS, GREEN)); ?>.<br />
<?php
    $errorCount = 0;
    $showWritableNote = false;
    $basedir = $serendipity['serendipityPath'];
?>
<div align="center">
<table class="serendipity_admin_list_item serendipity_admin_list_item_even" width="90%" align="center">
    <tr>
        <td colspan="2" style="font-weight: bold"><?php echo PERMISSIONS ?></td>
    </tr>
    <tr>
        <td><?php echo $basedir ?></td>
        <td width="200"><?php
            if ( is_writable($basedir) ) {
                echo serendipity_upgraderResultDiagnose(S9Y_U_SUCCESS, WRITABLE);
            } else {
                echo serendipity_upgraderResultDiagnose(S9Y_U_ERROR, NOT_WRITABLE);
                $showWritableNote = true;
            }
     ?></td>
    </tr>
    <tr>
        <td><?php echo $basedir . PATH_SMARTY_COMPILE?></td>
        <td width="200"><?php
            if ( is_writable($basedir . PATH_SMARTY_COMPILE) ) {
                echo serendipity_upgraderResultDiagnose(S9Y_U_SUCCESS, WRITABLE);
            } else {
                echo serendipity_upgraderResultDiagnose(S9Y_U_ERROR, NOT_WRITABLE);
                $showWritableNote = true;
            }
     ?></td>
    </tr>
<?php if (is_dir($basedir . $serendipity['uploadHTTPPath'])) { ?>
    <tr>
        <td><?php echo $basedir . $serendipity['uploadHTTPPath']; ?></td>
        <td width="200"><?php
            if (is_writable($basedir . $serendipity['uploadHTTPPath'])) {
                echo serendipity_upgraderResultDiagnose(S9Y_U_SUCCESS, WRITABLE);
            } else {
                echo serendipity_upgraderResultDiagnose(S9Y_U_ERROR, NOT_WRITABLE);
                $showWritableNote = true;
            }
     ?></td>
    </tr>
<?php } ?>
</table>
</div>
<?php if ($showWritableNote === true) { ?>
    <div class="serendipityAdminMsgNote"><?php echo sprintf(PROBLEM_PERMISSIONS_HOWTO, 'chmod 1777') ?></div>
<?php }

    if ($errorCount > 0) { ?>
    <div align="center">
        <div class="serendipityAdminMsgError"><?php echo PROBLEM_DIAGNOSTIC ?></div>
        <h2><a href="serendipity_admin.php"><?php echo RECHECK_INSTALLATION ?></a></h2>
    </div>
<?php }
?>
</div>

<?php
    if ($errorCount < 1) {
        if (sizeof($sqlfiles) > 0) { ?>
    <br />
    <h3><?php printf(SERENDIPITY_UPGRADER_DATABASE_UPDATES, $serendipity['dbType']) ?>:</h3>
<?php echo SERENDIPITY_UPGRADER_FOUND_SQL_FILES ?>:<br />
<?php
            foreach ($sqlfiles as $sqlfile) {
                echo '<div style="padding-left: 5px"><strong>'. $sqlfile .'</strong></div>';
            }
        }
?>
    <br />

    <h3><?php echo SERENDIPITY_UPGRADER_VERSION_SPECIFIC ?>:</h3>
<?php
        $taskCount = 0;

        foreach ( $tasks as $task ) {
            if (version_compare($serendipity['versionInstalled'], $task['version'], '<'))  {
                echo '<div><strong>'. $task['version'] .' - '. $task['title'] .'</strong></div>';
                echo '<div style="padding-left: 5px">'. nl2br($task['desc']) .'</div><br />';
                $taskCount++;
            }
        }

        if ($taskCount == 0) {
            echo SERENDIPITY_UPGRADER_NO_VERSION_SPECIFIC;
        }
?>

    <br /><br />
    <hr noshade="noshade">
<?php if ($taskCount > 0 || sizeof($sqlfiles) > 0) { ?>
        <strong><?php echo SERENDIPITY_UPGRADER_PROCEED_QUESTION ?></strong>
        <br /><br /><a href="<?php echo $upgradeLoc; ?>" class="serendipityPrettyButton"><?php echo SERENDIPITY_UPGRADER_PROCEED_DOIT ?></a> <?php if ($showAbort) { ?><a href="<?php echo $abortLoc; ?>" class="serendipityPrettyButton"><?php echo SERENDIPITY_UPGRADER_PROCEED_ABORT ?></a><?php } ?>
<?php } else { ?>
        <strong><?php echo SERENDIPITY_UPGRADER_NO_UPGRADES ?></strong>
        <br /><br /><a href="<?php echo $upgradeLoc; ?>" class="serendipityPrettyButton"><?php echo SERENDIPITY_UPGRADER_CONSIDER_DONE ?></a>
<?php }
    }
}
?>
