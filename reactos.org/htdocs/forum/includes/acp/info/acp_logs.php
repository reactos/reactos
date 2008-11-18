<?php
/**
*
* @package acp
* @version $Id: acp_logs.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class acp_logs_info
{
	function module()
	{
		return array(
			'filename'	=> 'acp_logs',
			'title'		=> 'ACP_LOGGING',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'admin'		=> array('title' => 'ACP_ADMIN_LOGS', 'auth' => 'acl_a_viewlogs', 'cat' => array('ACP_FORUM_LOGS')),
				'mod'		=> array('title' => 'ACP_MOD_LOGS', 'auth' => 'acl_a_viewlogs', 'cat' => array('ACP_FORUM_LOGS')),
				'users'		=> array('title' => 'ACP_USERS_LOGS', 'auth' => 'acl_a_viewlogs', 'cat' => array('ACP_FORUM_LOGS')),
				'critical'	=> array('title' => 'ACP_CRITICAL_LOGS', 'auth' => 'acl_a_viewlogs', 'cat' => array('ACP_FORUM_LOGS')),
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