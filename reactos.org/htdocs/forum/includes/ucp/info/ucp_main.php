<?php
/**
*
* @package ucp
* @version $Id: ucp_main.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class ucp_main_info
{
	function module()
	{
		return array(
			'filename'	=> 'ucp_main',
			'title'		=> 'UCP_MAIN',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'front'			=> array('title' => 'UCP_MAIN_FRONT', 'auth' => '', 'cat' => array('UCP_MAIN')),
				'subscribed'	=> array('title' => 'UCP_MAIN_SUBSCRIBED', 'auth' => '', 'cat' => array('UCP_MAIN')),
				'bookmarks'		=> array('title' => 'UCP_MAIN_BOOKMARKS', 'auth' => 'cfg_allow_bookmarks', 'cat' => array('UCP_MAIN')),
				'drafts'		=> array('title' => 'UCP_MAIN_DRAFTS', 'auth' => '', 'cat' => array('UCP_MAIN')),
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