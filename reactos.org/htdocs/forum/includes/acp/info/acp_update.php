<?php
/**
*
* @package acp
* @version $Id: acp_update.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class acp_update_info
{
	function module()
	{
		return array(
			'filename'	=> 'acp_update',
			'title'		=> 'ACP_UPDATE',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'version_check'		=> array('title' => 'ACP_VERSION_CHECK', 'auth' => 'acl_a_board', 'cat' => array('ACP_AUTOMATION')),
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