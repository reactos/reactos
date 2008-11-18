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
* @ignore
*/
if (!defined('IN_PHPBB'))
{
	exit;
}

/**
* @package acp
*/
class acp_update
{
	var $u_action;

	function main($id, $mode)
	{
		global $config, $db, $user, $auth, $template, $cache;
		global $phpbb_root_path, $phpbb_admin_path, $phpEx;

		$user->add_lang('install');

		$this->tpl_name = 'acp_update';
		$this->page_title = 'ACP_VERSION_CHECK';

		// Get current and latest version
		$errstr = '';
		$errno = 0;

		$info = get_remote_file('www.phpbb.com', '/updatecheck', ((defined('PHPBB_QA')) ? '30x_qa.txt' : '30x.txt'), $errstr, $errno);

		if ($info === false)
		{
			trigger_error($errstr, E_USER_WARNING);
		}

		$info = explode("\n", $info);
		$latest_version = trim($info[0]);

		$announcement_url = trim($info[1]);
		$update_link = append_sid($phpbb_root_path . 'install/index.' . $phpEx, 'mode=update');

		// Determine automatic update...
		$sql = 'SELECT config_value
			FROM ' . CONFIG_TABLE . "
			WHERE config_name = 'version_update_from'";
		$result = $db->sql_query($sql);
		$version_update_from = (string) $db->sql_fetchfield('config_value');
		$db->sql_freeresult($result);

		$current_version = (!empty($version_update_from)) ? $version_update_from : $config['version'];

		$up_to_date_automatic = (version_compare(str_replace('rc', 'RC', strtolower($current_version)), str_replace('rc', 'RC', strtolower($latest_version)), '<')) ? false : true;
		$up_to_date = (version_compare(str_replace('rc', 'RC', strtolower($config['version'])), str_replace('rc', 'RC', strtolower($latest_version)), '<')) ? false : true;

		$template->assign_vars(array(
			'S_UP_TO_DATE'		=> $up_to_date,
			'S_UP_TO_DATE_AUTO'	=> $up_to_date_automatic,
			'S_VERSION_CHECK'	=> true,
			'U_ACTION'			=> $this->u_action,

			'LATEST_VERSION'	=> $latest_version,
			'CURRENT_VERSION'	=> $config['version'],
			'AUTO_VERSION'		=> $version_update_from,

			'UPDATE_INSTRUCTIONS'	=> sprintf($user->lang['UPDATE_INSTRUCTIONS'], $announcement_url, $update_link),
		));
	}
}

?>