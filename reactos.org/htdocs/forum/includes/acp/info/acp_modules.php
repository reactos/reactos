<?php
/**
*
* @package acp
* @version $Id: acp_modules.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class acp_modules_info
{
	function module()
	{
		return array(
			'filename'	=> 'acp_modules',
			'title'		=> 'ACP_MODULE_MANAGEMENT',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'acp'		=> array('title' => 'ACP', 'auth' => 'acl_a_modules', 'cat' => array('ACP_MODULE_MANAGEMENT')),
				'ucp'		=> array('title' => 'UCP', 'auth' => 'acl_a_modules', 'cat' => array('ACP_MODULE_MANAGEMENT')),
				'mcp'		=> array('title' => 'MCP', 'auth' => 'acl_a_modules', 'cat' => array('ACP_MODULE_MANAGEMENT')),
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