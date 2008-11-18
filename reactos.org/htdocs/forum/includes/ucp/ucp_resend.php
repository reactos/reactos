<?php
/**
*
* @package ucp
* @version $Id: ucp_resend.php 8479 2008-03-29 00:22:48Z naderman $
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
* ucp_resend
* Resending activation emails
* @package ucp
*/
class ucp_resend
{
	var $u_action;

	function main($id, $mode)
	{
		global $config, $phpbb_root_path, $phpEx;
		global $db, $user, $auth, $template;

		$username	= request_var('username', '', true);
		$email		= strtolower(request_var('email', ''));
		$submit		= (isset($_POST['submit'])) ? true : false;

		add_form_key('ucp_resend');

		if ($submit)
		{
			if (!check_form_key('ucp_resend'))
			{
				trigger_error('FORM_INVALID');
			}

			$sql = 'SELECT user_id, group_id, username, user_email, user_type, user_lang, user_actkey, user_inactive_reason
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

			if (!$user_row['user_actkey'] && $user_row['user_type'] != USER_INACTIVE)
			{
				trigger_error('ACCOUNT_ALREADY_ACTIVATED');
			}

			if (!$user_row['user_actkey'] || ($user_row['user_type'] == USER_INACTIVE && $user_row['user_inactive_reason'] == INACTIVE_MANUAL))
			{
				trigger_error('ACCOUNT_DEACTIVATED');
			}

			// Determine coppa status on group (REGISTERED(_COPPA))
			$sql = 'SELECT group_name, group_type
				FROM ' . GROUPS_TABLE . '
				WHERE group_id = ' . $user_row['group_id'];
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			if (!$row)
			{
				trigger_error('NO_GROUP');
			}

			$coppa = ($row['group_name'] == 'REGISTERED_COPPA' && $row['group_type'] == GROUP_SPECIAL) ? true : false;

			include_once($phpbb_root_path . 'includes/functions_messenger.' . $phpEx);
			$messenger = new messenger(false);

			if ($config['require_activation'] == USER_ACTIVATION_SELF || $coppa)
			{
				$messenger->template(($coppa) ? 'coppa_resend_inactive' : 'user_resend_inactive', $user_row['user_lang']);
				$messenger->to($user_row['user_email'], $user_row['username']);

				$messenger->headers('X-AntiAbuse: Board servername - ' . $config['server_name']);
				$messenger->headers('X-AntiAbuse: User_id - ' . $user->data['user_id']);
				$messenger->headers('X-AntiAbuse: Username - ' . $user->data['username']);
				$messenger->headers('X-AntiAbuse: User IP - ' . $user->ip);

				$messenger->assign_vars(array(
					'WELCOME_MSG'	=> htmlspecialchars_decode(sprintf($user->lang['WELCOME_SUBJECT'], $config['sitename'])),
					'USERNAME'		=> htmlspecialchars_decode($user_row['username']),
					'U_ACTIVATE'	=> generate_board_url() . "/ucp.$phpEx?mode=activate&u={$user_row['user_id']}&k={$user_row['user_actkey']}")
				);

				if ($coppa)
				{
					$messenger->assign_vars(array(
						'FAX_INFO'		=> $config['coppa_fax'],
						'MAIL_INFO'		=> $config['coppa_mail'],
						'EMAIL_ADDRESS'	=> $user_row['user_email'])
					);
				}

				$messenger->send(NOTIFY_EMAIL);
			}

			if ($config['require_activation'] == USER_ACTIVATION_ADMIN)
			{
				// Grab an array of user_id's with a_user permissions ... these users can activate a user
				$admin_ary = $auth->acl_get_list(false, 'a_user', false);

				$sql = 'SELECT user_id, username, user_email, user_lang, user_jabber, user_notify_type
					FROM ' . USERS_TABLE . '
					WHERE ' . $db->sql_in_set('user_id', $admin_ary[0]['a_user']);
				$result = $db->sql_query($sql);

				while ($row = $db->sql_fetchrow($result))
				{
					$messenger->template('admin_activate', $row['user_lang']);
					$messenger->to($row['user_email'], $row['username']);
					$messenger->im($row['user_jabber'], $row['username']);

					$messenger->assign_vars(array(
						'USERNAME'			=> htmlspecialchars_decode($user_row['username']),
						'U_USER_DETAILS'	=> generate_board_url() . "/memberlist.$phpEx?mode=viewprofile&u={$user_row['user_id']}",
						'U_ACTIVATE'		=> generate_board_url() . "/ucp.$phpEx?mode=activate&u={$user_row['user_id']}&k={$user_row['user_actkey']}")
					);

					$messenger->send($row['user_notify_type']);
				}
				$db->sql_freeresult($result);
			}

			meta_refresh(3, append_sid("{$phpbb_root_path}index.$phpEx"));

			$message = ($config['require_activation'] == USER_ACTIVATION_ADMIN) ? $user->lang['ACIVATION_EMAIL_SENT_ADMIN'] : $user->lang['ACTIVATION_EMAIL_SENT'];
			$message .= '<br /><br />' . sprintf($user->lang['RETURN_INDEX'], '<a href="' . append_sid("{$phpbb_root_path}index.$phpEx") . '">', '</a>');
			trigger_error($message);
		}

		$template->assign_vars(array(
			'USERNAME'			=> $username,
			'EMAIL'				=> $email,
			'S_PROFILE_ACTION'	=> append_sid($phpbb_root_path . 'ucp.' . $phpEx, 'mode=resend_act'))
		);

		$this->tpl_name = 'ucp_resend';
		$this->page_title = 'UCP_RESEND';
	}
}

?>