<?php
/**
*
* @package mcp
* @version $Id: mcp_ban.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class mcp_ban_info
{
	function module()
	{
		return array(
			'filename'	=> 'mcp_ban',
			'title'		=> 'MCP_BAN',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'user'		=> array('title' => 'MCP_BAN_USERNAMES', 'auth' => 'acl_m_ban', 'cat' => array('MCP_BAN')),
				'ip'		=> array('title' => 'MCP_BAN_IPS', 'auth' => 'acl_m_ban', 'cat' => array('MCP_BAN')),
				'email'		=> array('title' => 'MCP_BAN_EMAILS', 'auth' => 'acl_m_ban', 'cat' => array('MCP_BAN')),
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