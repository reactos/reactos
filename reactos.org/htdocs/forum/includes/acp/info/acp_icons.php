<?php
/**
*
* @package acp
* @version $Id: acp_icons.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class acp_icons_info
{
	function module()
	{
		return array(
			'filename'	=> 'acp_icons',
			'title'		=> 'ACP_ICONS_SMILIES',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'icons'		=> array('title' => 'ACP_ICONS', 'auth' => 'acl_a_icons', 'cat' => array('ACP_MESSAGES')),
				'smilies'	=> array('title' => 'ACP_SMILIES', 'auth' => 'acl_a_icons', 'cat' => array('ACP_MESSAGES')),
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