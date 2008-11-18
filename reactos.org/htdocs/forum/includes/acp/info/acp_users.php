<?php
/**
*
* @package acp
* @version $Id: acp_users.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class acp_users_info
{
	function module()
	{
		return array(
			'filename'	=> 'acp_users',
			'title'		=> 'ACP_USER_MANAGEMENT',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'overview'		=> array('title' => 'ACP_MANAGE_USERS', 'auth' => 'acl_a_user', 'cat' => array('ACP_CAT_USERS')),
				'feedback'		=> array('title' => 'ACP_USER_FEEDBACK', 'auth' => 'acl_a_user', 'display' => false, 'cat' => array('ACP_CAT_USERS')),
				'profile'		=> array('title' => 'ACP_USER_PROFILE', 'auth' => 'acl_a_user', 'display' => false, 'cat' => array('ACP_CAT_USERS')),
				'prefs'			=> array('title' => 'ACP_USER_PREFS', 'auth' => 'acl_a_user', 'display' => false, 'cat' => array('ACP_CAT_USERS')),
				'avatar'		=> array('title' => 'ACP_USER_AVATAR', 'auth' => 'acl_a_user', 'display' => false, 'cat' => array('ACP_CAT_USERS')),
				'rank'			=> array('title' => 'ACP_USER_RANK', 'auth' => 'acl_a_user', 'display' => false, 'cat' => array('ACP_CAT_USERS')),
				'sig'			=> array('title' => 'ACP_USER_SIG', 'auth' => 'acl_a_user', 'display' => false, 'cat' => array('ACP_CAT_USERS')),
				'groups'		=> array('title' => 'ACP_USER_GROUPS', 'auth' => 'acl_a_user && acl_a_group', 'display' => false, 'cat' => array('ACP_CAT_USERS')),
				'perm'			=> array('title' => 'ACP_USER_PERM', 'auth' => 'acl_a_user && acl_a_viewauth', 'display' => false, 'cat' => array('ACP_CAT_USERS')),
				'attach'		=> array('title' => 'ACP_USER_ATTACH', 'auth' => 'acl_a_user', 'display' => false, 'cat' => array('ACP_CAT_USERS')),
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