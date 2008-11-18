<?php
/**
*
* @package acp
* @version $Id: acp_bbcodes.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class acp_bbcodes_info
{
	function module()
	{
		return array(
			'filename'	=> 'acp_bbcodes',
			'title'		=> 'ACP_BBCODES',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'bbcodes'		=> array('title' => 'ACP_BBCODES', 'auth' => 'acl_a_bbcode', 'cat' => array('ACP_MESSAGES')),
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