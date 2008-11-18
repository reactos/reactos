<?php
/**
*
* @package ucp
* @version $Id: ucp_remind.php 8479 2008-03-29 00:22:48Z naderman $
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
* ucp_remind
* Sending password reminders
* @package ucp
*/
class ucp_remind
{
	var $u_action;

	function main($id, $mode)
	{
		global $config, $phpbb_root_path, $phpEx;
		global $db, $user, $auth, $template;

		$username	= request_var('username', '', true);
		$email		= strtolower(request_var('email', ''));
		$submit		= (isset($_POST['submit'])) ? true : false;

		if ($submit)
		{
			$sql = 'SELECT user_id, username, user_email, user_jabber, user_notify_type, user_type, user_lang, user_inactive_reason
				FROM ' . USERS_TABLE . "
				WHERE user_email = '" . $db->sql_escape($email) . "'
					AND username_clean = '" . $db->sql_escape(utf8_clean_string($username)) . "'";
			$result = $db->sql_query($sql);
			$user_row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			if (!$user_row)
			{
				trigger_error('NO_EMAIL_USER');
			}

			if ($user_row['user_type'] == USER_IGNORE)
			{
				trigger_error('NO_USER');
			}

			if ($user_row['user_type'] == USER_INACTIVE)
			{
				if ($user_row['user_inactive_reason'] == INACTIVE_MANUAL)
				{
					trigger_error('ACCOUNT_DEACTIVATED');
				}
				else
				{
					trigger_error('ACCOUNT_NOT_ACTIVATED');
				}
			}

			$server_url = generate_board_url();

			$key_len = 54 - strlen($server_url);
			$key_len = max(6, $key_len); // we want at least 6
			$key_len = ($config['max_pass_chars']) ? min($key_len, $config['max_pass_chars']) : $key_len; // we want at most $config['max_pass_chars']
			$user_actkey = substr(gen_rand_string(10), 0, $key_len);
			$user_password = gen_rand_string(8);

			$sql = 'UPDATE ' . USERS_TABLE . "
				SET user_newpasswd = '" . $db->sql_escape(phpbb_hash($user_password)) . "', user_actkey = '" . $db->sql_escape($user_actkey) . "'
				WHERE user_id = " . $user_row['user_id'];
			$db->sql_query($sql);

			include_once($phpbb_root_path . 'includes/functions_messenger.' . $phpEx);

			$messenger = new messenger(false);

			$messenger->template('user_activate_passwd', $user_row['user_lang']);

			$messenger->to($user_row['user_email'], $user_row['username']);
			$messenger->im($user_row['user_jabber'], $user_row['username']);

			$messenger->assign_vars(array(
				'USERNAME'		=> htmlspecialchars_decode($user_row['username']),
				'PASSWORD'		=> htmlspecialchars_decode($user_password),
				'U_ACTIVATE'	=> "$server_url/ucp.$phpEx?mode=activate&u={$user_row['user_id']}&k=$user_actkey")
			);

			$messenger->send($user_row['user_notify_type']);

			meta_refresh(3, append_sid("{$phpbb_root_path}index.$phpEx"));

			$message = $user->lang['PASSWORD_UPDATED'] . '<br /><br />' . sprintf($user->lang['RETURN_INDEX'], '<a href="' . append_sid("{$phpbb_root_path}index.$phpEx") . '">', '</a>');
			trigger_error($message);
		}

		$template->assign_vars(array(
			'USERNAME'			=> $username,
			'EMAIL'				=> $email,
			'S_PROFILE_ACTION'	=> append_sid($phpbb_root_path . 'ucp.' . $phpEx, 'mode=sendpassword'))
		);

		$this->tpl_name = 'ucp_remind';
		$this->page_title = 'UCP_REMIND';
	}
}

?>