<?php
/**
*
* @package mcp
* @version $Id: mcp_main.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class mcp_main_info
{
	function module()
	{
		return array(
			'filename'	=> 'mcp_main',
			'title'		=> 'MCP_MAIN',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'front'			=> array('title' => 'MCP_MAIN_FRONT', 'auth' => '', 'cat' => array('MCP_MAIN')),
				'forum_view'	=> array('title' => 'MCP_MAIN_FORUM_VIEW', 'auth' => 'acl_m_,$id', 'cat' => array('MCP_MAIN')),
				'topic_view'	=> array('title' => 'MCP_MAIN_TOPIC_VIEW', 'auth' => 'acl_m_,$id', 'cat' => array('MCP_MAIN')),
				'post_details'	=> array('title' => 'MCP_MAIN_POST_DETAILS', 'auth' => 'acl_m_,$id || (!$id && aclf_m_)', 'cat' => array('MCP_MAIN')),
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