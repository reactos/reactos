<?php
/**
*
* @package acp
* @version $Id: acp_profile.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class acp_profile_info
{
	function module()
	{
		return array(
			'filename'	=> 'acp_profile',
			'title'		=> 'ACP_CUSTOM_PROFILE_FIELDS',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'profile'	=> array('title' => 'ACP_CUSTOM_PROFILE_FIELDS', 'auth' => 'acl_a_profile', 'cat' => array('ACP_CAT_USERS')),
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