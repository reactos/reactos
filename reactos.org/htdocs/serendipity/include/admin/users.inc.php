<?php # $Id: users.inc.php 724 2005-11-23 11:30:28Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (IN_serendipity !== true) {
    die ('Don\'t hack!');
}

if (!serendipity_checkPermission('adminUsers')) {
    return;
}

require_once(S9Y_INCLUDE_PATH . 'include/functions_installer.inc.php');

/* Delete a user */
if (isset($_POST['DELETE_YES']) && serendipity_checkFormToken()) {
    $user = serendipity_fetchUsers($serendipity['POST']['user']);
    if ($user[0]['userlevel'] >= $serendipity['serendipityUserlevel'] || !serendipity_checkPermission('adminUsersDelete')) {
        echo '<div class="serendipityAdminMsgError">' . CREATE_NOT_AUTHORIZED . '</div>';
    } elseif ($_POST['userlevel'] > $serendipity['serendipityUserlevel']) {
        echo '<div class="serendipityAdminMsgError">' . CREATE_NOT_AUTHORIZED_USERLEVEL . '</div>';
    } else {
        $group_intersect = serendipity_intersectGroup($user[0]['authorid']);
        
        if (serendipity_checkPermission('adminUsersMaintainOthers') ||
            (serendipity_checkPermission('adminUsersMaintainSame') && $group_intersect)) {
            serendipity_deleteAuthor($user[0]['authorid']);
            printf('<div class="serendipityAdminMsgSuccess">' . DELETED_USER . '</div>', $serendipity['POST']['user'], $user[0]['realname']);
            serendipity_plugin_api::hook_event('backend_users_delete', $user[0]);
        } else {
            echo '<div class="serendipityAdminMsgError">' . CREATE_NOT_AUTHORIZED_USERLEVEL . '</div>';
        }
    }
}


/* Save new user */
if (isset($_POST['SAVE_NEW']) && serendipity_checkFormToken()) {
    if ($_POST['userlevel'] >= $serendipity['serendipityUserlevel'] || !serendipity_checkPermission('adminUsersCreateNew')) {
        echo '<div class="serendipityAdminMsgError">' . CREATE_NOT_AUTHORIZED . '</div>';
    } else {
        $serendipity['POST']['user'] = serendipity_addAuthor($_POST['username'], $_POST['pass'], $_POST['realname'], $_POST['email'], $_POST['userlevel']);

        $valid_groups = serendipity_getGroups($serendipity['authorid'], true);
        /* Save all the properties */
        $config = serendipity_parseTemplate(S9Y_CONFIG_USERTEMPLATE);
        foreach($config as $category) {
            foreach ($category['items'] as $item) {
                if (in_array('groups', $item['flags'])) {
                    if (serendipity_checkPermission('adminUsersMaintainOthers')) {

                        // Void, no fixing neccessarry

                    } elseif (serendipity_checkPermission('adminUsersMaintainSame')) {

                        // Check that no user may assign groups he's not allowed to.
                        foreach($_POST[$item['var']] AS $groupkey => $groupval) {
                            if (in_array($group_val, $valid_groups)) {
                                continue;
                            }

                            unset($_POST[$item['var']][$groupkey]);
                        }

                    } else {
                        continue;
                    }

                    serendipity_updateGroups($_POST[$item['var']], $serendipity['POST']['user']);
                    continue;
                }

                if (serendipity_checkConfigItemFlags($item, 'local')) {
                    serendipity_set_user_var($item['var'], $_POST[$item['var']], $serendipity['POST']['user'], ($serendipity['authorid'] == $serendipity['POST']['authorid'] ? true : false));
                }

                if (serendipity_checkConfigItemFlags($item, 'configuration')) {
                    serendipity_set_config_var($item['var'], $_POST[$item['var']], $serendipity['POST']['user']);
                }
            }
        }

        serendipity_plugin_api::hook_event('backend_users_add', $serendipity['POST']['user']);
        printf('<div class="serendipityAdminMsgSuccess">' . CREATED_USER . '</div>', '#' . $serendipity['POST']['user'] . ', ' . $_POST['realname']);
    }
}


/* Edit a user */
if (isset($_POST['SAVE_EDIT']) && serendipity_checkFormToken()) {
    $user = serendipity_fetchUsers($serendipity['POST']['user']);
    if (!serendipity_checkPermission('adminUsersMaintainOthers') && $user[0]['userlevel'] >= $serendipity['serendipityUserlevel']) {
        echo '<div class="serendipityAdminMsgError">' . CREATE_NOT_AUTHORIZED . '</div>';
    } elseif ($_POST['userlevel'] > $serendipity['serendipityUserlevel']) {
        echo '<div class="serendipityAdminMsgError">' . CREATE_NOT_AUTHORIZED_USERLEVEL . '</div>';
    } else {
        $valid_groups = serendipity_getGroups($serendipity['authorid'], true);
        $config = serendipity_parseTemplate(S9Y_CONFIG_USERTEMPLATE);
        foreach($config as $category) {
            foreach ($category['items'] as $item) {
                if (in_array('groups', $item['flags'])) {
                    if (serendipity_checkPermission('adminUsersMaintainOthers')) {

                        // Void, no fixing neccessarry

                    } elseif (serendipity_checkPermission('adminUsersMaintainSame')) {

                        // Check that no user may assign groups he's not allowed to.
                        foreach($_POST[$item['var']] AS $groupkey => $groupval) {
                            if (in_array($group_val, $valid_groups)) {
                                continue;
                            }

                            unset($_POST[$item['var']][$groupkey]);
                        }

                    } else {
                        continue;
                    }

                    serendipity_updateGroups($_POST[$item['var']], $serendipity['POST']['user']);
                    continue;
                }

                if (serendipity_checkConfigItemFlags($item, 'local')) {
                    serendipity_set_user_var($item['var'], $_POST[$item['var']], $serendipity['POST']['user'], ($serendipity['authorid'] == $serendipity['POST']['user'] ? true : false));
                }

                if (serendipity_checkConfigItemFlags($item, 'configuration')) {
                    serendipity_set_config_var($item['var'], $_POST[$item['var']], $serendipity['POST']['user']);
                }
            }
        }

        $pl_data = array(
            'id'       => $serendipity['POST']['authorid'],
            'authorid' => $serendipity['POST']['authorid'],
            'username' => $_POST['username'],
            'realname' => $_POST['realname'],
            'email'    => $_POST['email']
        );
        serendipity_updatePermalink($pl_data, 'author');

        serendipity_plugin_api::hook_event('backend_users_edit', $pl_data);
        printf('<div class="serendipityAdminMsgSuccess">' . MODIFIED_USER . '</div>', $_POST['realname']);
    }
}

if ($serendipity['GET']['adminAction'] != 'delete') {
?>
    <table width="100%">
        <tr>
            <td><strong><?php echo USER; ?></strong></td>
            <td width="100" align="center"><strong><?php echo USER_LEVEL ?></strong></td>
            <td width="200">&nbsp;</td>
        </tr>
        <tr>
            <td colspan="3">
<?php
if (serendipity_checkPermission('adminUsersMaintainOthers')) {
    $users = serendipity_fetchUsers('');
} elseif (serendipity_checkPermission('adminUsersMaintainSame')) {
    $users = serendipity_fetchUsers('', serendipity_getGroups($serendipity['authorid'], true));
} else {
    $users = serendipity_fetchUsers($serendipity['authorid']);
}

$i = 0;
foreach($users as $user) {
    if ($user['userlevel'] < $serendipity['serendipityUserlevel'] || $user['authorid'] == $serendipity['authorid'] || $serendipity['serendipityUserlevel'] >= USERLEVEL_ADMIN ) {
        if ( $user['userlevel'] >= USERLEVEL_ADMIN ) {
            $img = serendipity_getTemplateFile('admin/img/user_admin.png');
        } elseif ( $user['userlevel'] >= USERLEVEL_CHIEF ) {
            $img = serendipity_getTemplateFile('admin/img/user_chief.png');
        } else {
            $img = serendipity_getTemplateFile('admin/img/user_editor.png');
        }
?>
<div class="serendipity_admin_list_item serendipity_admin_list_item_<?php echo ($i++ % 2) ? 'even' : 'uneven' ?>">
<table width="100%">
    <tr>
<?php /* TODO: Add username to list once tom figures out how to fix uneven rowstyles */ ?>
        <td><img src="<?php echo $img ?>" alt="" style="border: 0px none ; vertical-align: bottom; display: inline;" /> <?php echo htmlspecialchars($user['realname']); ?></td>
        <td width="100" align="center"><?php echo $user['userlevel']; ?></td>
        <td width="200" align="right"> [<a href="?serendipity[adminModule]=users&amp;serendipity[adminAction]=edit&amp;serendipity[userid]=<?php echo $user['authorid'] ?>"><?php echo EDIT ?></a>]
                                     - [<a href="?serendipity[adminModule]=users&amp;serendipity[adminAction]=delete&amp;serendipity[userid]=<?php echo $user['authorid'] ?>"><?php echo DELETE ?></a>]</td>
    </tr>
</table>
</div>
<?php
    }
}
?>
            </tr>
        </tr>
<?php if ( !isset($_POST['NEW']) && serendipity_checkPermission('adminUsersCreateNew')) { ?>
        <tr>
            <td colspan="3" align="right">
                <form action="?serendipity[adminModule]=users" method="post">
                    <input type="submit" name="NEW"   value="<?php echo CREATE_NEW_USER; ?>" class="serendipityPrettyButton" />
                </form>
            </td>
        </tr>
<?php } ?>
    </table>

<?php
}


if ( ($serendipity['GET']['adminAction'] == 'edit' && serendipity_checkPermission('adminUsersDelete')) || (isset($_POST['NEW']) && serendipity_checkPermission('adminUsersCreateNew')) ) {
?>
<br />
<br />
<hr noshade="noshade">
<form action="?serendipity[adminModule]=users" method="post">
<?php echo serendipity_setFormToken(); ?>
    <div>
    <h3>
<?php
if ($serendipity['GET']['adminAction'] == 'edit') {
    $user = serendipity_fetchUsers($serendipity['GET']['userid']);
    $group_intersect = serendipity_intersectGroup($user[0]['authorid']);
        
    if ($user[0]['userlevel'] >= $serendipity['serendipityUserlevel'] && $user[0]['authorid'] != $serendipity['authorid'] && !serendipity_checkPermission('adminUsersMaintainOthers')) {
        echo '<strong>' . CREATE_NOT_AUTHORIZED . '</strong><br />';
        echo EDIT;
        $from = array();
    } elseif (serendipity_checkPermission('adminUsersMaintainOthers') ||
            (serendipity_checkPermission('adminUsersMaintainSame') && $group_intersect)) {
        echo EDIT;
        $from = &$user[0];
        unset($from['password']);
        echo '<input type="hidden" name="serendipity[user]" value="' . $from['authorid'] . '" />';
    } else {
        echo '<strong>' . CREATE_NOT_AUTHORIZED . '</strong><br />';
        echo EDIT;
        $from = array();
    }
} else {
    echo CREATE;
    $from = array();
}
?>
    </h3>

<?php
$config = serendipity_parseTemplate(S9Y_CONFIG_USERTEMPLATE);
if (!empty($serendipity['GET']['userid'])) {
    $from['groups'] = serendipity_getGroups($serendipity['GET']['userid']);
} else {
    $from['groups'] = array();
} 

serendipity_printConfigTemplate($config, $from, true, false);

if ($serendipity['GET']['adminAction'] == 'edit') { ?>
        <input type="submit" name="SAVE_EDIT"   value="<?php echo SAVE; ?>" class="serendipityPrettyButton" />
<?php } else { ?>
        <input type="submit" name="SAVE_NEW" value="<?php echo CREATE_NEW_USER; ?>" class="serendipityPrettyButton" />
<?php } ?>

    </div>
</form>
<?php
} elseif ($serendipity['GET']['adminAction'] == 'delete' && serendipity_checkPermission('adminUsersDelete')) {
    $user = serendipity_fetchUsers($serendipity['GET']['userid']);
    $group_intersect = serendipity_intersectGroup($user[0]['authorid']);

    if (serendipity_checkPermission('adminUsersMaintainOthers') ||
                (serendipity_checkPermission('adminUsersMaintainSame') && $group_intersect)) {
?>
<form action="?serendipity[adminModule]=users" method="post">
    <div>
    <?php printf(DELETE_USER, $serendipity['GET']['userid'], $user[0]['realname']); ?>
        <br /><br />
        <?php echo serendipity_setFormToken(); ?>
        <input type="hidden" name="serendipity[user]" value="<?php echo $serendipity['GET']['userid']; ?>" />
        <input type="submit" name="DELETE_YES" value="<?php echo DUMP_IT; ?>" class="serendipityPrettyButton" />
        <input type="submit" name="NO" value="<?php echo NOT_REALLY; ?>" class="serendipityPrettyButton" />
    </div>
</form>
<?php
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>