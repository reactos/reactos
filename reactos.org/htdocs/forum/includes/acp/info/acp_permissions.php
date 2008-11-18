<?php
/**
*
* @package acp
* @version $Id: acp_permissions.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class acp_permissions_info
{
	function module()
	{
		return array(
			'filename'	=> 'acp_permissions',
			'title'		=> 'ACP_PERMISSIONS',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'intro'					=> array('title' => 'ACP_PERMISSIONS', 'auth' => 'acl_a_authusers || acl_a_authgroups || acl_a_viewauth', 'cat' => array('ACP_CAT_PERMISSIONS')),
				'trace'					=> array('title' => 'ACP_PERMISSION_TRACE', 'auth' => 'acl_a_viewauth', 'display' => false, 'cat' => array('ACP_PERMISSION_MASKS')),

				'setting_forum_local'	=> array('title' => 'ACP_FORUM_PERMISSIONS', 'auth' => 'acl_a_fauth && (acl_a_authusers || acl_a_authgroups)', 'cat' => array('ACP_FORUM_BASED_PERMISSIONS')),
				'setting_mod_local'		=> array('title' => 'ACP_FORUM_MODERATORS', 'auth' => 'acl_a_mauth && (acl_a_authusers || acl_a_authgroups)', 'cat' => array('ACP_FORUM_BASED_PERMISSIONS')),
				'setting_user_global'	=> array('title' => 'ACP_USERS_PERMISSIONS', 'auth' => 'acl_a_authusers && (acl_a_aauth || acl_a_mauth || acl_a_uauth)', 'cat' => array('ACP_GLOBAL_PERMISSIONS', 'ACP_CAT_USERS')),
				'setting_user_local'	=> array('title' => 'ACP_USERS_FORUM_PERMISSIONS', 'auth' => 'acl_a_authusers && (acl_a_mauth || acl_a_fauth)', 'cat' => array('ACP_FORUM_BASED_PERMISSIONS', 'ACP_CAT_USERS')),
				'setting_group_global'	=> array('title' => 'ACP_GROUPS_PERMISSIONS', 'auth' => 'acl_a_authgroups && (acl_a_aauth || acl_a_mauth || acl_a_uauth)', 'cat' => array('ACP_GLOBAL_PERMISSIONS', 'ACP_GROUPS')),
				'setting_group_local'	=> array('title' => 'ACP_GROUPS_FORUM_PERMISSIONS', 'auth' => 'acl_a_authgroups && (acl_a_mauth || acl_a_fauth)', 'cat' => array('ACP_FORUM_BASED_PERMISSIONS', 'ACP_GROUPS')),
				'setting_admin_global'	=> array('title' => 'ACP_ADMINISTRATORS', 'auth' => 'acl_a_aauth && (acl_a_authusers || acl_a_authgroups)', 'cat' => array('ACP_GLOBAL_PERMISSIONS')),
				'setting_mod_global'	=> array('title' => 'ACP_GLOBAL_MODERATORS', 'auth' => 'acl_a_mauth && (acl_a_authusers || acl_a_authgroups)', 'cat' => array('ACP_GLOBAL_PERMISSIONS')),

				'view_admin_global'		=> array('title' => 'ACP_VIEW_ADMIN_PERMISSIONS', 'auth' => 'acl_a_viewauth', 'cat' => array('ACP_PERMISSION_MASKS')),
				'view_user_global'		=> array('title' => 'ACP_VIEW_USER_PERMISSIONS', 'auth' => 'acl_a_viewauth', 'cat' => array('ACP_PERMISSION_MASKS')),
				'view_mod_global'		=> array('title' => 'ACP_VIEW_GLOBAL_MOD_PERMISSIONS', 'auth' => 'acl_a_viewauth', 'cat' => array('ACP_PERMISSION_MASKS')),
				'view_mod_local'		=> array('title' => 'ACP_VIEW_FORUM_MOD_PERMISSIONS', 'auth' => 'acl_a_viewauth', 'cat' => array('ACP_PERMISSION_MASKS')),
				'view_forum_local'		=> array('title' => 'ACP_VIEW_FORUM_PERMISSIONS', 'auth' => 'acl_a_viewauth', 'cat' => array('ACP_PERMISSION_MASKS')),
			),
		);
	}

	function install()
	{
	}

	function uninstall()
	{
	}
}

?>