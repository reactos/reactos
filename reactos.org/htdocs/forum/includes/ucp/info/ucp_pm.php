<?php
/**
* @package ucp
* @version $Id: ucp_pm.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class ucp_pm_info
{
	function module()
	{
		return array(
			'filename'	=> 'ucp_pm',
			'title'		=> 'UCP_PM',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'view'		=> array('title' => 'UCP_PM_VIEW', 'auth' => 'cfg_allow_privmsg', 'display' => false, 'cat' => array('UCP_PM')),
				'compose'	=> array('title' => 'UCP_PM_COMPOSE', 'auth' => 'cfg_allow_privmsg', 'cat' => array('UCP_PM')),
				'drafts'	=> array('title' => 'UCP_PM_DRAFTS', 'auth' => 'cfg_allow_privmsg', 'cat' => array('UCP_PM')),
				'options'	=> array('title' => 'UCP_PM_OPTIONS', 'auth' => 'cfg_allow_privmsg', 'cat' => array('UCP_PM')),
				'popup'		=> array('title' => 'UCP_PM_POPUP_TITLE', 'auth' => 'cfg_allow_privmsg', 'display' => false, 'cat' => array('UCP_PM')),
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