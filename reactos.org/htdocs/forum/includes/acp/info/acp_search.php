<?php
/**
*
* @package acp
* @version $Id: acp_search.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class acp_search_info
{
	function module()
	{
		return array(
			'filename'	=> 'acp_search',
			'title'		=> 'ACP_SEARCH',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'settings'	=> array('title' => 'ACP_SEARCH_SETTINGS', 'auth' => 'acl_a_search', 'cat' => array('ACP_SERVER_CONFIGURATION')),
				'index'		=> array('title' => 'ACP_SEARCH_INDEX', 'auth' => 'acl_a_search', 'cat' => array('ACP_CAT_DATABASE')),
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