<?php
/**
*
* @package acp
* @version $Id: acp_email.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class acp_email_info
{
	function module()
	{
		return array(
			'filename'	=> 'acp_email',
			'title'		=> 'ACP_MASS_EMAIL',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'email'		=> array('title' => 'ACP_MASS_EMAIL', 'auth' => 'acl_a_email', 'cat' => array('ACP_GENERAL_TASKS')),
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