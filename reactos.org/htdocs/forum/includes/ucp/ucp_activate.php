<?php
/**
*
* @package ucp
* @version $Id: ucp_activate.php 8479 2008-03-29 00:22:48Z naderman $
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
* ucp_activate
* User activation
* @package ucp
*/
class ucp_activate
{
	var $u_action;

	function main($id, $mode)
	{
		global $config, $phpbb_root_path, $phpEx;
		global $db, $user, $auth, $template;

		$user_id = request_var('u', 0);
		$key = request_var('k', '');

		$sql = 'SELECT user_id, username, user_type, user_email, user_newpasswd, user_lang, user_notify_type, user_actkey, user_inactive_reason
			FROM ' . USERS_TABLE . "
			WHERE user_id = $user_id";
		$result = $db->sql_query($sql);
		$user_row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if (!$user_row)
		{
			trigger_error('NO_USER');
		}

		if ($user_row['user_type'] <> USER_INACTIVE && !$user_row['user_newpasswd'])
		{
			meta_refresh(3, append_sid("{$phpbb_root_path}index.$phpEx"));
			trigger_error('ALREADY_ACTIVATED');
		}

		if ($user_row['user_actkey'] != $key)
		{
			trigger_error('WRONG_ACTIVATION');
		}

		$update_password = ($user_row['user_newpasswd']) ? true : false;

		if ($update_password)
		{
			$sql_ary = array(
				'user_actkey'		=> '',
				'user_password'		=> $user_row['user_newpasswd'],
				'user_newpasswd'	=> '',
				'user_pass_convert'	=> 0,
			);

			$sql = 'UPDATE ' . USERS_TABLE . '
				SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
				WHERE user_id = ' . $user_row['user_id'];
			$db->sql_query($sql);
		}

		if (!$update_password)
		{
			include_once($phpbb_root_path . 'includes/functions_user.' . $phpEx);

			user_active_flip('activate', $user_row['user_id']);

			$sql = 'UPDATE ' . USERS_TABLE . "
				SET user_actkey = ''
				WHERE user_id = {$user_row['user_id']}";
			$db->sql_query($sql);
		}

		if ($config['require_activation'] == USER_ACTIVATION_ADMIN && !$update_password)
		{
			include_once($phpbb_root_path . 'includes/functions_messenger.' . $phpEx);

			$messenger = new messenger(false);

			$messenger->template('admin_welcome_activated', $user_row['user_lang']);

			$messenger->to($user_row['user_email'], $user_row['username']);

			$messenger->headers('X-AntiAbuse: Board servername - ' . $config['server_name']);
			$messenger->headers('X-AntiAbuse: User_id - ' . $user->data['user_id']);
			$messenger->headers('X-AntiAbuse: Username - ' . $user->data['username']);
			$messenger->headers('X-AntiAbuse: User IP - ' . $user->ip);

			$messenger->assign_vars(array(
				'USERNAME'	=> htmlspecialchars_decode($user_row['username']))
			);

			$messenger->send($user_row['user_notify_type']);

			$message = 'ACCOUNT_ACTIVE_ADMIN';
		}
		else
		{
			if (!$update_password)
			{
				$message = ($user_row['user_inactive_reason'] == INACTIVE_PROFILE) ? 'ACCOUNT_ACTIVE_PROFILE' : 'ACCOUNT_ACTIVE';
			}
			else
			{
				$message = 'PASSWORD_ACTIVATED';
			}
		}

		meta_refresh(3, append_sid("{$phpbb_root_path}index.$phpEx"));
		trigger_error($user->lang[$message]);
	}
}

?>