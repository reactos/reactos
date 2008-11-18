<?php
/**
*
* @package acp
* @version $Id: acp_jabber.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
* @todo Check/enter/update transport info
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
class acp_jabber
{
	var $u_action;

	function main($id, $mode)
	{
		global $db, $user, $auth, $template;
		global $config, $phpbb_root_path, $phpbb_admin_path, $phpEx;

		$user->add_lang('acp/board');

		include_once($phpbb_root_path . 'includes/functions_jabber.' . $phpEx);

		$action	= request_var('action', '');
		$submit = (isset($_POST['submit'])) ? true : false;

		if ($mode != 'settings')
		{
			return;
		}

		$this->tpl_name = 'acp_jabber';
		$this->page_title = 'ACP_JABBER_SETTINGS';

		$jab_enable			= request_var('jab_enable', $config['jab_enable']);
		$jab_host			= request_var('jab_host', $config['jab_host']);
		$jab_port			= request_var('jab_port', $config['jab_port']);
		$jab_username		= request_var('jab_username', $config['jab_username']);
		$jab_password		= request_var('jab_password', $config['jab_password']);
		$jab_package_size	= request_var('jab_package_size', $config['jab_package_size']);
		$jab_use_ssl		= request_var('jab_use_ssl', $config['jab_use_ssl']);

		$form_name = 'acp_jabber';
		add_form_key($form_name);

		if ($submit)
		{
			if (!check_form_key($form_name))
			{
				trigger_error($user->lang['FORM_INVALID']. adm_back_link($this->u_action), E_USER_WARNING);
			}

			$error = array();

			$message = $user->lang['JAB_SETTINGS_CHANGED'];
			$log = 'JAB_SETTINGS_CHANGED';

			// Is this feature enabled? Then try to establish a connection
			if ($jab_enable)
			{
				$jabber = new jabber($jab_host, $jab_port, $jab_username, $jab_password, $jab_use_ssl);

				if (!$jabber->connect())
				{
					trigger_error($user->lang['ERR_JAB_CONNECT'] . '<br /><br />' . $jabber->get_log() . adm_back_link($this->u_action), E_USER_WARNING);
				}

				// We'll try to authorise using this account
				if (!$jabber->login())
				{
					trigger_error($user->lang['ERR_JAB_AUTH'] . '<br /><br />' . $jabber->get_log() . adm_back_link($this->u_action), E_USER_WARNING);
				}

				$jabber->disconnect();
			}

			set_config('jab_enable', $jab_enable);
			set_config('jab_host', $jab_host);
			set_config('jab_port', $jab_port);
			set_config('jab_username', $jab_username);
			set_config('jab_password', $jab_password);
			set_config('jab_package_size', $jab_package_size);
			set_config('jab_use_ssl', $jab_use_ssl);

			add_log('admin', 'LOG_' . $log);
			trigger_error($message . adm_back_link($this->u_action));
		}

		$template->assign_vars(array(
			'U_ACTION'				=> $this->u_action,
			'JAB_ENABLE'			=> $jab_enable,
			'L_JAB_SERVER_EXPLAIN'	=> sprintf($user->lang['JAB_SERVER_EXPLAIN'], '<a href="http://www.jabber.org/">', '</a>'),
			'JAB_HOST'				=> $jab_host,
			'JAB_PORT'				=> $jab_port,
			'JAB_USERNAME'			=> $jab_username,
			'JAB_PASSWORD'			=> $jab_password,
			'JAB_PACKAGE_SIZE'		=> $jab_package_size,
			'JAB_USE_SSL'			=> $jab_use_ssl,
			'S_CAN_USE_SSL'			=> jabber::can_use_ssl(),
			'S_GTALK_NOTE'			=> (!@function_exists('dns_get_record')) ? true : false,
		));
	}
}

?>