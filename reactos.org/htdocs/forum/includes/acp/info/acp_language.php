<?php
/**
*
* @package acp
* @version $Id: acp_language.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class acp_language_info
{
	function module()
	{
		return array(
			'filename'	=> 'acp_language',
			'title'		=> 'ACP_LANGUAGE',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'lang_packs'		=> array('title' => 'ACP_LANGUAGE_PACKS', 'auth' => 'acl_a_language', 'cat' => array('ACP_GENERAL_TASKS')),
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