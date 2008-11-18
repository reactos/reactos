<?php # $Id: serendipity_admin.php 697 2005-11-14 08:30:25Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

define('IN_installer', true);
define('IN_upgrader', true);
define('IN_serendipity', true);
define('IN_serendipity_admin', true);
include('serendipity_config.inc.php');

header('Content-Type: text/html; charset=' . LANG_CHARSET);

if (IS_installed === false) {
    require_once(S9Y_INCLUDE_PATH . 'include/functions_permalinks.inc.php');
    require_once(S9Y_INCLUDE_PATH . 'include/functions_installer.inc.php');
    require_once S9Y_INCLUDE_PATH . 'include/functions_config.inc.php';
    $css_file = 'serendipity.css.php?serendipity[css_mode]=serendipity_admin.css';
} else {
    $css_file = serendipity_rewriteURL('serendipity_admin.css');
    if (defined('IS_up2date') && IS_up2date === true) {
        serendipity_plugin_api::hook_event('backend_configure', $serendipity);
    }
}

if (isset($serendipity['GET']['adminModule']) && $serendipity['GET']['adminModule'] == 'logout') {
    serendipity_logout();
} else {
    if (IS_installed === true && !serendipity_userLoggedIn()) {
        // Try again to log in, this time with enabled external authentication event hook
        serendipity_login(true);
    }
}

// If we are inside an iframe, halt the script
if (serendipity_is_iframe()) {
    return true;
}

?>
<html>
    <head>
        <title><?php echo SERENDIPITY_ADMIN_SUITE; ?></title>
        <meta http-equiv="Content-Type" content="text/html; charset=<?php echo LANG_CHARSET; ?>" />
        <link rel="stylesheet" type="text/css" href="<?php echo $css_file; ?>" />
        <script type="text/javascript">
        function spawn() {
            if (self.Spawnextended) {
                Spawnextended();
            }

            if (self.Spawnbody) {
                Spawnbody();
            }

            if (self.Spawnnugget) {
                Spawnnugget();
            }
        }
        
        function SetCookie(name, value) {
            var today  = new Date();
            var expire = new Date();
            expire.setTime(today.getTime() + (60*60*24*30));
            document.cookie = 'serendipity[' + name + ']='+escape(value) + ';expires=' + expire.toGMTString();
        }

        function addLoadEvent(func) {
          var oldonload = window.onload;
          if (typeof window.onload != 'function') {
            window.onload = func;
          } else {
            window.onload = function() {
              oldonload();
              func();
            }
          }
        }

        </script>
    </head>
    <body id="serendipity_admin_page" onload="spawn()">
        <table cellspacing="0" cellpadding="0" border="0" id="serendipityAdminFrame">
            <tr>
                <td colspan="2" id="serendipityAdminBanner">
                <?php if ( IS_installed === true && IS_up2date === true ) { ?>
                    <h1><?php echo SERENDIPITY_ADMIN_SUITE ?></h1>
                    <h2><?php echo $serendipity['blogTitle'] ?></h2>
                <?php } elseif ( IS_installed === false || IS_up2date === false ) { ?>
                    <h1><?php echo SERENDIPITY_INSTALLATION ?></h1>
                <?php } ?>
                </td>
            </tr>
            <tr>
                <td colspan="2" id="serendipityAdminInfopane">
                    <?php if (serendipity_userLoggedIn()) { ?>
                        <?php echo sprintf(USER_SELF_INFO, $serendipity['serendipityUser'], $serendipity['permissionLevels'][$serendipity['serendipityUserlevel']]) ?>
                    <?php } ?>
                </td>
            </tr>
            <tr valign="top">
<?php
if (!isset($serendipity['serendipityPath']) || IS_installed === false || IS_up2date === false ) {
    if (IS_installed === false) {
        $file = 'include/admin/installer.inc.php';
    } elseif ( IS_up2date === false ) {
        $file = 'include/admin/upgrader.inc.php';
    } else {
        $file = ''; // For register_global, safety
    }
?>
                <td class="serendipityAdminContent" colspan="2">
                    <?php require_once(S9Y_INCLUDE_PATH . $file); ?>
<?php


} elseif ( serendipity_userLoggedIn() == false ) {
        $out = array();
        serendipity_plugin_api::hook_event('backend_login_page', $out);
?>
                <td colspan="2" class="serendipityAdminContent">
                    <div align="center"><?php echo WELCOME_TO_ADMIN ?><br /><?php echo PLEASE_ENTER_CREDENTIALS ?></div>
                    <?php echo $out['header']; ?>
                    <br />
                    <?php if ( isset($serendipity['POST']['action']) && !serendipity_userLoggedIn() ) { ?>
                    <div class="serendipityAdminMsgError"><?php echo WRONG_USERNAME_OR_PASSWORD; ?></div>
                    <?php } ?>
                    <form action="serendipity_admin.php" method="post">
                        <input type="hidden" name="serendipity[action]" value="admin" />
                        <table cellspacing="10" cellpadding="0" border="0" align="center">
                            <tr>
                                <td><?php echo USERNAME ?></td>
                                <td><input type="text" name="serendipity[user]" /></td>
                            </tr>
                            <tr>
                                <td><?php echo PASSWORD ?></td>
                                <td><input type="password" name="serendipity[pass]" /></td>
                            </tr>
                            <tr>
                                <td colspan="2"><input id="autologin" type="checkbox" name="serendipity[auto]" /><label for="autologin"> <?php echo AUTOMATIC_LOGIN ?></label></td>
                            </tr>
                            <tr>
                                <td colspan="2" align="right"><input type="submit" name="submit" value="<?php echo LOGIN ?> &gt;" class="serendipityPrettyButton" /></td>
                            </tr>
                            <?php echo $out['table']; ?>
                        </table>
                    </form>
                    <?php echo $out['footer']; ?>
                    <a href="<?php echo $serendipity['serendipityHTTPPath']; ?>"><?php echo BACK_TO_BLOG;?></a>
<?php



} else {
?>
                <td id="serendipitySideBar">
                    <ul class="serendipitySideBarMenu">
                        <li><a href="serendipity_admin.php"><?php echo ADMIN_FRONTPAGE; ?></a></li>
<?php if (serendipity_checkPermission('personalConfiguration')) { ?>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=personal"><?php echo PERSONAL_SETTINGS; ?></a></li>
<?php } ?>
                    </ul>
                    <br />
<?php if (serendipity_checkPermission('adminEntries')) { ?>
                    <ul class="serendipitySideBarMenu">
                        <li class="serendipitySideBarMenuHead"><?php echo ADMIN_ENTRIES ?></li>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=entries&amp;serendipity[adminAction]=new"><?php echo NEW_ENTRY; ?></a></li>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=entries&amp;serendipity[adminAction]=editSelect"><?php echo EDIT_ENTRIES; ?></a></li>
<?php if (serendipity_checkPermission('adminComments')) { ?>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=comments"><?php echo COMMENTS; ?></a></li>
<?php } ?>
<?php if (serendipity_checkPermission('adminCategories')) { ?>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=category&amp;serendipity[adminAction]=view"><?php echo CATEGORIES; ?></a></li>
<?php } ?>
                        <?php serendipity_plugin_api::hook_event('backend_sidebar_entries', $serendipity); ?>
                    </ul>
<?php } ?>
<?php if (serendipity_checkPermission('adminImages')) { ?>
                    <ul class="serendipitySideBarMenu">
                        <li class="serendipitySideBarMenuHead"><?php echo MEDIA; ?></li>
<?php if (serendipity_checkPermission('adminImagesAdd')) { ?>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=media&amp;serendipity[adminAction]=addSelect"><?php echo ADD_MEDIA; ?></a></li>
<?php } ?>
<?php if (serendipity_checkPermission('adminImagesView')) { ?>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=media"><?php echo MEDIA_LIBRARY; ?></a></li>
<?php } ?>
<?php if (serendipity_checkPermission('adminImagesDirectories')) { ?>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=media&amp;serendipity[adminAction]=directorySelect"><?php echo MANAGE_DIRECTORIES; ?></a></li>
<?php } ?>
<?php if (serendipity_checkPermission('adminImagesSync')) { ?>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=media&amp;serendipity[adminAction]=sync" onclick="return confirm('<?php echo WARNING_THIS_BLAHBLAH; ?>');"><?php echo CREATE_THUMBS; ?></a></li>
<?php } ?>
                        <?php serendipity_plugin_api::hook_event('backend_sidebar_entries_images', $serendipity); ?>
                    </ul>
<?php } ?>
<?php if (serendipity_checkPermission('adminTemplates') || serendipity_checkPermission('adminPlugins')) { ?>
                    <ul class="serendipitySideBarMenu">
                        <li class="serendipitySideBarMenuHead"><?php echo APPEARANCE; ?></li>
<?php if (serendipity_checkPermission('adminTemplates')) { ?>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=templates"><?php echo MANAGE_STYLES; ?></a></li>
<?php } ?>
<?php if (serendipity_checkPermission('adminPlugins')) { ?>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=plugins"><?php echo CONFIGURE_PLUGINS; ?></a></li>
<?php } ?>
                        <?php serendipity_plugin_api::hook_event('backend_sidebar_admin_appearance', $serendipity); ?>
                    </ul>
<?php } ?>
<?php if (serendipity_checkPermission('siteConfiguration') || serendipity_checkPermission('blogConfiguration') || serendipity_checkPermission('adminUsers') || serendipity_checkPermission('adminUsersGroups') || serendipity_checkPermission('adminImport')) { ?>
                    <ul class="serendipitySideBarMenu">
                        <li class="serendipitySideBarMenuHead"><?php echo ADMIN; ?></li>
<?php if (serendipity_checkPermission('siteConfiguration') || serendipity_checkPermission('blogConfiguration')) { ?>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=configuration"><?php echo CONFIGURATION; ?></a></li>
<?php } ?>
<?php if (serendipity_checkPermission('adminUsers')) { ?>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=users"><?php echo MANAGE_USERS; ?></a></li>
<?php } ?>
<?php if (serendipity_checkPermission('adminUsersGroups')) { ?>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=groups"><?php echo MANAGE_GROUPS; ?></a></li>
<?php } ?>
<?php if (serendipity_checkPermission('adminImport')) { ?>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=import"><?php echo IMPORT_ENTRIES; ?></a></li>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=export"><?php echo EXPORT_ENTRIES; ?></a></li>
<?php } ?>
                        <?php serendipity_plugin_api::hook_event('backend_sidebar_admin', $serendipity); ?>
                    </ul>
<?php } ?>
                    <br />
                    <ul class="serendipitySideBarMenu">
                        <li><a href="<?php echo $serendipity['baseURL']; ?>"><?php echo BACK_TO_BLOG; ?></a></li>
                        <li><a href="serendipity_admin.php?serendipity[adminModule]=logout"><?php echo LOGOUT; ?></a></li>
                    </ul>

                </td>
                <td class="serendipityAdminContent">
<?php
    if (!isset($serendipity['GET']['adminModule'])) {
        $serendipity['GET']['adminModule'] = (isset($serendipity['POST']['adminModule']) ? $serendipity['POST']['adminModule'] : '');
    }
    
    serendipity_checkXSRF();

    switch($serendipity['GET']['adminModule']) {
        case 'installer':
        case 'configuration':
            if (!serendipity_checkPermission('siteConfiguration') && !serendipity_checkPermission('blogConfiguration')) {
                break;
            }

            include S9Y_INCLUDE_PATH . 'include/admin/configuration.inc.php';
            break;

        case 'media':
        case 'images':
            if (!serendipity_checkPermission('adminImages')) {
                break;
            }

            include S9Y_INCLUDE_PATH . 'include/admin/images.inc.php';
            break;

        case 'templates':
            if (!serendipity_checkPermission('adminTemplates')) {
                break;
            }

            include S9Y_INCLUDE_PATH . 'include/admin/templates.inc.php';
            break;

        case 'plugins':
            if (!serendipity_checkPermission('adminPlugins')) {
                break;
            }

            include S9Y_INCLUDE_PATH . 'include/admin/plugins.inc.php';
            break;

        case 'users':
            if (!serendipity_checkPermission('adminUsers')) {
                break;
            }

            include S9Y_INCLUDE_PATH . 'include/admin/users.inc.php';
            break;

        case 'groups':
            if (!serendipity_checkPermission('adminUsersGroups')) {
                break;
            }

            include S9Y_INCLUDE_PATH . 'include/admin/groups.inc.php';
            break;

        case 'personal':
            if (!serendipity_checkPermission('personalConfiguration')) {
                break;
            }

            include S9Y_INCLUDE_PATH . 'include/admin/personal.inc.php';
            break;

        case 'export':
            if (!serendipity_checkPermission('adminImport')) {
                break;
            }

            include S9Y_INCLUDE_PATH . 'include/admin/export.inc.php';
            break;

        case 'import':
            if (!serendipity_checkPermission('adminImport')) {
                break;
            }

            include S9Y_INCLUDE_PATH . 'include/admin/import.inc.php';
            break;

        case 'entries':
            if (!serendipity_checkPermission('adminEntries')) {
                break;
            }

            include S9Y_INCLUDE_PATH . 'include/admin/entries.inc.php';
            break;

        case 'comments':
            if (!serendipity_checkPermission('adminComments')) {
                break;
            }

            include S9Y_INCLUDE_PATH . 'include/admin/comments.inc.php';
            break;

        case 'category':
        case 'categories':
            if (!serendipity_checkPermission('adminCategories')) {
                break;
            }

            include S9Y_INCLUDE_PATH . 'include/admin/category.inc.php';
            break;

        case 'logout':
            echo LOGGEDOUT;
            break;

        case 'event_display':
            serendipity_plugin_api::hook_event('backend_sidebar_entries_event_display_' . $serendipity['GET']['adminAction'], $serendipity);
            break;

        case 'logout':
            echo LOGGEDOUT;
            break;

        default:
            include S9Y_INCLUDE_PATH . 'include/admin/overview.inc.php';
            break;
    }
}
?>
                </td>
            </tr>
        </table>
        <br />
        <div id="serendipityAdminFooter"><?php echo sprintf(ADMIN_FOOTER_POWERED_BY, $serendipity['versionInstalled'], phpversion()); ?></div>
    </body>
</html>
<?php
/* vim: set sts=4 ts=4 expandtab : */
?>
