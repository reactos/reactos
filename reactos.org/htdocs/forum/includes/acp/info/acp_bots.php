<?php
/**
*
* @package acp
* @version $Id: acp_bots.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class acp_bots_info
{
	function module()
	{
		return array(
			'filename'	=> 'acp_bots',
			'title'		=> 'ACP_BOTS',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'bots'		=> array('title' => 'ACP_BOTS', 'auth' => 'acl_a_bots', 'cat' => array('ACP_GENERAL_TASKS')),
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