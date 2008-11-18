<?php
/**
*
* @package acp
* @version $Id: acp_users.php 8479 2008-03-29 00:22:48Z naderman $
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
class acp_users
{
	var $u_action;
	var $p_master;

	function acp_users(&$p_master)
	{
		$this->p_master = &$p_master;
	}

	function main($id, $mode)
	{
		global $config, $db, $user, $auth, $template, $cache;
		global $phpbb_root_path, $phpbb_admin_path, $phpEx, $table_prefix, $file_uploads;

		$user->add_lang(array('posting', 'ucp', 'acp/users'));
		$this->tpl_name = 'acp_users';
		$this->page_title = 'ACP_USER_' . strtoupper($mode);

		$error		= array();
		$username	= utf8_normalize_nfc(request_var('username', '', true));
		$user_id	= request_var('u', 0);
		$action		= request_var('action', '');

		$submit		= (isset($_POST['update']) && !isset($_POST['cancel'])) ? true : false;

		$form_name = 'acp_users';
		add_form_key($form_name);

		// Whois (special case)
		if ($action == 'whois')
		{
			include($phpbb_root_path . 'includes/functions_user.' . $phpEx);

			$this->page_title = 'WHOIS';
			$this->tpl_name = 'simple_body';

			$user_ip = request_var('user_ip', '');
			$domain = gethostbyaddr($user_ip);
			$ipwhois = user_ipwhois($user_ip);

			$template->assign_vars(array(
				'MESSAGE_TITLE'		=> sprintf($user->lang['IP_WHOIS_FOR'], $domain),
				'MESSAGE_TEXT'		=> nl2br($ipwhois))
			);

			return;
		}

		// Show user selection mask
		if (!$username && !$user_id)
		{
			$this->page_title = 'SELECT_USER';

			$template->assign_vars(array(
				'U_ACTION'			=> $this->u_action,
				'ANONYMOUS_USER_ID'	=> ANONYMOUS,

				'S_SELECT_USER'		=> true,
				'U_FIND_USERNAME'	=> append_sid("{$phpbb_root_path}memberlist.$phpEx", 'mode=searchuser&amp;form=select_user&amp;field=username&amp;select_single=true'),
			));

			return;
		}

		if (!$user_id)
		{
			$sql = 'SELECT user_id
				FROM ' . USERS_TABLE . "
				WHERE username_clean = '" . $db->sql_escape(utf8_clean_string($username)) . "'";
			$result = $db->sql_query($sql);
			$user_id = (int) $db->sql_fetchfield('user_id');
			$db->sql_freeresult($result);

			if (!$user_id)
			{
				trigger_error($user->lang['NO_USER'] . adm_back_link($this->u_action), E_USER_WARNING);
			}
		}

		// Generate content for all modes
		$sql = 'SELECT u.*, s.*
			FROM ' . USERS_TABLE . ' u
				LEFT JOIN ' . SESSIONS_TABLE . ' s ON (s.session_user_id = u.user_id)
			WHERE u.user_id = ' . $user_id . '
			ORDER BY s.session_time DESC';
		$result = $db->sql_query($sql);
		$user_row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if (!$user_row)
		{
			trigger_error($user->lang['NO_USER'] . adm_back_link($this->u_action), E_USER_WARNING);
		}

		// Generate overall "header" for user admin
		$s_form_options = '';

		// Build modes dropdown list
		$sql = 'SELECT module_mode, module_auth
			FROM ' . MODULES_TABLE . "
			WHERE module_basename = 'users'
				AND module_enabled = 1
				AND module_class = 'acp'
			ORDER BY left_id, module_mode";
		$result = $db->sql_query($sql);

		$dropdown_modes = array();
		while ($row = $db->sql_fetchrow($result))
		{
			if (!$this->p_master->module_auth($row['module_auth']))
			{
				continue;
			}

			$dropdown_modes[$row['module_mode']] = true;
		}
		$db->sql_freeresult($result);

		foreach ($dropdown_modes as $module_mode => $null)
		{
			$selected = ($mode == $module_mode) ? ' selected="selected"' : '';
			$s_form_options .= '<option value="' . $module_mode . '"' . $selected . '>' . $user->lang['ACP_USER_' . strtoupper($module_mode)] . '</option>';
		}

		$template->assign_vars(array(
			'U_BACK'			=> $this->u_action,
			'U_MODE_SELECT'		=> append_sid("{$phpbb_admin_path}index.$phpEx", "i=$id&amp;u=$user_id"),
			'U_ACTION'			=> $this->u_action . '&amp;u=' . $user_id,
			'S_FORM_OPTIONS'	=> $s_form_options,
			'MANAGED_USERNAME'	=> $user_row['username'])
		);

		// Prevent normal users/admins change/view founders if they are not a founder by themselves
		if ($user->data['user_type'] != USER_FOUNDER && $user_row['user_type'] == USER_FOUNDER)
		{
			trigger_error($user->lang['NOT_MANAGE_FOUNDER'] . adm_back_link($this->u_action), E_USER_WARNING);
		}

		switch ($mode)
		{
			case 'overview':

				include($phpbb_root_path . 'includes/functions_user.' . $phpEx);

				$user->add_lang('acp/ban');

				$delete			= request_var('delete', 0);
				$delete_type	= request_var('delete_type', '');
				$ip				= request_var('ip', 'ip');

				if ($submit)
				{
					// You can't delete the founder
					if ($delete && $user_row['user_type'] != USER_FOUNDER)
					{
						if (!$auth->acl_get('a_userdel'))
						{
							trigger_error($user->lang['NO_AUTH_OPERATION'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
						}

						// Check if the user wants to remove himself or the guest user account
						if ($user_id == ANONYMOUS)
						{
							trigger_error($user->lang['CANNOT_REMOVE_ANONYMOUS'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
						}

						if ($user_id == $user->data['user_id'])
						{
							trigger_error($user->lang['CANNOT_REMOVE_YOURSELF'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
						}

						if (confirm_box(true))
						{
							user_delete($delete_type, $user_id, $user_row['username']);

							add_log('admin', 'LOG_USER_DELETED', $user_row['username']);
							trigger_error($user->lang['USER_DELETED'] . adm_back_link($this->u_action));
						}
						else
						{
							confirm_box(false, $user->lang['CONFIRM_OPERATION'], build_hidden_fields(array(
								'u'				=> $user_id,
								'i'				=> $id,
								'mode'			=> $mode,
								'action'		=> $action,
								'update'		=> true,
								'delete'		=> 1,
								'delete_type'	=> $delete_type))
							);
						}
					}

					// Handle quicktool actions
					switch ($action)
					{
						case 'banuser':
						case 'banemail':
						case 'banip':

							if ($user_id == $user->data['user_id'])
							{
								trigger_error($user->lang['CANNOT_BAN_YOURSELF'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							if ($user_row['user_type'] == USER_FOUNDER)
							{
								trigger_error($user->lang['CANNOT_BAN_FOUNDER'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							if (!check_form_key($form_name))
							{
								trigger_error($user->lang['FORM_INVALID'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							$ban = array();

							switch ($action)
							{
								case 'banuser':
									$ban[] = $user_row['username'];
									$reason = 'USER_ADMIN_BAN_NAME_REASON';
									$log = 'LOG_USER_BAN_USER';
								break;

								case 'banemail':
									$ban[] = $user_row['user_email'];
									$reason = 'USER_ADMIN_BAN_EMAIL_REASON';
									$log = 'LOG_USER_BAN_EMAIL';
								break;

								case 'banip':
									$ban[] = $user_row['user_ip'];

									$sql = 'SELECT DISTINCT poster_ip
										FROM ' . POSTS_TABLE . "
										WHERE poster_id = $user_id";
									$result = $db->sql_query($sql);

									while ($row = $db->sql_fetchrow($result))
									{
										$ban[] = $row['poster_ip'];
									}
									$db->sql_freeresult($result);

									$reason = 'USER_ADMIN_BAN_IP_REASON';
									$log = 'LOG_USER_BAN_IP';
								break;
							}

							$ban_reason = utf8_normalize_nfc(request_var('ban_reason', $user->lang[$reason], true));
							$ban_give_reason = utf8_normalize_nfc(request_var('ban_give_reason', '', true));

							// Log not used at the moment, we simply utilize the ban function.
							$result = user_ban(substr($action, 3), $ban, 0, 0, 0, $ban_reason, $ban_give_reason);

							trigger_error((($result === false) ? $user->lang['BAN_ALREADY_ENTERED'] : $user->lang['BAN_SUCCESSFUL']) . adm_back_link($this->u_action . '&amp;u=' . $user_id));

						break;

						case 'reactivate':

							if ($user_id == $user->data['user_id'])
							{
								trigger_error($user->lang['CANNOT_FORCE_REACT_YOURSELF'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							if (!check_form_key($form_name))
							{
								trigger_error($user->lang['FORM_INVALID'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							if ($user_row['user_type'] == USER_FOUNDER)
							{
								trigger_error($user->lang['CANNOT_FORCE_REACT_FOUNDER'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							if ($user_row['user_type'] == USER_IGNORE)
							{
								trigger_error($user->lang['CANNOT_FORCE_REACT_BOT'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							if ($config['email_enable'])
							{
								include_once($phpbb_root_path . 'includes/functions_messenger.' . $phpEx);

								$server_url = generate_board_url();

								$user_actkey = gen_rand_string(10);
								$key_len = 54 - (strlen($server_url));
								$key_len = ($key_len > 6) ? $key_len : 6;
								$user_actkey = substr($user_actkey, 0, $key_len);
								$email_template = ($user_row['user_type'] == USER_NORMAL) ? 'user_reactivate_account' : 'user_resend_inactive';

								if ($user_row['user_type'] == USER_NORMAL)
								{
									user_active_flip('deactivate', $user_id, INACTIVE_REMIND);

									$sql = 'UPDATE ' . USERS_TABLE . "
										SET user_actkey = '" . $db->sql_escape($user_actkey) . "'
										WHERE user_id = $user_id";
									$db->sql_query($sql);
								}
								else
								{
									// Grabbing the last confirm key - we only send a reminder
									$sql = 'SELECT user_actkey
										FROM ' . USERS_TABLE . '
										WHERE user_id = ' . $user_id;
									$result = $db->sql_query($sql);
									$user_actkey = (string) $db->sql_fetchfield('user_actkey');
									$db->sql_freeresult($result);
								}

								$messenger = new messenger(false);

								$messenger->template($email_template, $user_row['user_lang']);

								$messenger->to($user_row['user_email'], $user_row['username']);

								$messenger->headers('X-AntiAbuse: Board servername - ' . $config['server_name']);
								$messenger->headers('X-AntiAbuse: User_id - ' . $user->data['user_id']);
								$messenger->headers('X-AntiAbuse: Username - ' . $user->data['username']);
								$messenger->headers('X-AntiAbuse: User IP - ' . $user->ip);

								$messenger->assign_vars(array(
									'WELCOME_MSG'	=> htmlspecialchars_decode(sprintf($user->lang['WELCOME_SUBJECT'], $config['sitename'])),
									'USERNAME'		=> htmlspecialchars_decode($user_row['username']),
									'U_ACTIVATE'	=> "$server_url/ucp.$phpEx?mode=activate&u={$user_row['user_id']}&k=$user_actkey")
								);

								$messenger->send(NOTIFY_EMAIL);

								add_log('admin', 'LOG_USER_REACTIVATE', $user_row['username']);
								add_log('user', $user_id, 'LOG_USER_REACTIVATE_USER');

								trigger_error($user->lang['FORCE_REACTIVATION_SUCCESS'] . adm_back_link($this->u_action . '&amp;u=' . $user_id));
							}

						break;

						case 'active':

							if ($user_id == $user->data['user_id'])
							{
								// It is only deactivation since the user is already activated (else he would not have reached this page)
								trigger_error($user->lang['CANNOT_DEACTIVATE_YOURSELF'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							if (!check_form_key($form_name))
							{
								trigger_error($user->lang['FORM_INVALID'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							if ($user_row['user_type'] == USER_FOUNDER)
							{
								trigger_error($user->lang['CANNOT_DEACTIVATE_FOUNDER'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							if ($user_row['user_type'] == USER_IGNORE)
							{
								trigger_error($user->lang['CANNOT_DEACTIVATE_BOT'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							user_active_flip('flip', $user_id);

							$message = ($user_row['user_type'] == USER_INACTIVE) ? 'USER_ADMIN_ACTIVATED' : 'USER_ADMIN_DEACTIVED';
							$log = ($user_row['user_type'] == USER_INACTIVE) ? 'LOG_USER_ACTIVE' : 'LOG_USER_INACTIVE';

							add_log('admin', $log, $user_row['username']);
							add_log('user', $user_id, $log . '_USER');

							trigger_error($user->lang[$message] . adm_back_link($this->u_action . '&amp;u=' . $user_id));

						break;

						case 'delsig':

							if (!check_form_key($form_name))
							{
								trigger_error($user->lang['FORM_INVALID'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							$sql_ary = array(
								'user_sig'					=> '',
								'user_sig_bbcode_uid'		=> '',
								'user_sig_bbcode_bitfield'	=> ''
							);

							$sql = 'UPDATE ' . USERS_TABLE . ' SET ' . $db->sql_build_array('UPDATE', $sql_ary) . "
								WHERE user_id = $user_id";
							$db->sql_query($sql);

							add_log('admin', 'LOG_USER_DEL_SIG', $user_row['username']);
							add_log('user', $user_id, 'LOG_USER_DEL_SIG_USER');

							trigger_error($user->lang['USER_ADMIN_SIG_REMOVED'] . adm_back_link($this->u_action . '&amp;u=' . $user_id));

						break;

						case 'delavatar':

							if (!check_form_key($form_name))
							{
								trigger_error($user->lang['FORM_INVALID'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							$sql_ary = array(
								'user_avatar'			=> '',
								'user_avatar_type'		=> 0,
								'user_avatar_width'		=> 0,
								'user_avatar_height'	=> 0,
							);

							$sql = 'UPDATE ' . USERS_TABLE . '
								SET ' . $db->sql_build_array('UPDATE', $sql_ary) . "
								WHERE user_id = $user_id";
							$db->sql_query($sql);

							// Delete old avatar if present
							if ($user_row['user_avatar'] && $user_row['user_avatar_type'] != AVATAR_GALLERY)
							{
								avatar_delete('user', $user_row);
							}

							add_log('admin', 'LOG_USER_DEL_AVATAR', $user_row['username']);
							add_log('user', $user_id, 'LOG_USER_DEL_AVATAR_USER');

							trigger_error($user->lang['USER_ADMIN_AVATAR_REMOVED'] . adm_back_link($this->u_action . '&amp;u=' . $user_id));
						break;

						case 'delposts':

							if (confirm_box(true))
							{
								// Delete posts, attachments, etc.
								delete_posts('poster_id', $user_id);

								add_log('admin', 'LOG_USER_DEL_POSTS', $user_row['username']);
								trigger_error($user->lang['USER_POSTS_DELETED'] . adm_back_link($this->u_action . '&amp;u=' . $user_id));
							}
							else
							{
								confirm_box(false, $user->lang['CONFIRM_OPERATION'], build_hidden_fields(array(
									'u'				=> $user_id,
									'i'				=> $id,
									'mode'			=> $mode,
									'action'		=> $action,
									'update'		=> true))
								);
							}

						break;

						case 'delattach':

							if (confirm_box(true))
							{
								delete_attachments('user', $user_id);

								add_log('admin', 'LOG_USER_DEL_ATTACH', $user_row['username']);
								trigger_error($user->lang['USER_ATTACHMENTS_REMOVED'] . adm_back_link($this->u_action . '&amp;u=' . $user_id));
							}
							else
							{
								confirm_box(false, $user->lang['CONFIRM_OPERATION'], build_hidden_fields(array(
									'u'				=> $user_id,
									'i'				=> $id,
									'mode'			=> $mode,
									'action'		=> $action,
									'update'		=> true))
								);
							}

						break;

						case 'moveposts':

							if (!check_form_key($form_name))
							{
								trigger_error($user->lang['FORM_INVALID'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							$user->add_lang('acp/forums');

							$new_forum_id = request_var('new_f', 0);

							if (!$new_forum_id)
							{
								$this->page_title = 'USER_ADMIN_MOVE_POSTS';

								$template->assign_vars(array(
									'S_SELECT_FORUM'		=> true,
									'U_ACTION'				=> $this->u_action . "&amp;action=$action&amp;u=$user_id",
									'U_BACK'				=> $this->u_action . "&amp;u=$user_id",
									'S_FORUM_OPTIONS'		=> make_forum_select(false, false, false, true))
								);

								return;
							}

							// Is the new forum postable to?
							$sql = 'SELECT forum_name, forum_type
								FROM ' . FORUMS_TABLE . "
								WHERE forum_id = $new_forum_id";
							$result = $db->sql_query($sql);
							$forum_info = $db->sql_fetchrow($result);
							$db->sql_freeresult($result);

							if (!$forum_info)
							{
								trigger_error($user->lang['NO_FORUM'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							if ($forum_info['forum_type'] != FORUM_POST)
							{
								trigger_error($user->lang['MOVE_POSTS_NO_POSTABLE_FORUM'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							// Two stage?
							// Move topics comprising only posts from this user
							$topic_id_ary = $move_topic_ary = $move_post_ary = $new_topic_id_ary = array();
							$forum_id_ary = array($new_forum_id);

							$sql = 'SELECT topic_id, COUNT(post_id) AS total_posts
								FROM ' . POSTS_TABLE . "
								WHERE poster_id = $user_id
									AND forum_id <> $new_forum_id
								GROUP BY topic_id";
							$result = $db->sql_query($sql);

							while ($row = $db->sql_fetchrow($result))
							{
								$topic_id_ary[$row['topic_id']] = $row['total_posts'];
							}
							$db->sql_freeresult($result);

							if (sizeof($topic_id_ary))
							{
								$sql = 'SELECT topic_id, forum_id, topic_title, topic_replies, topic_replies_real, topic_attachment
									FROM ' . TOPICS_TABLE . '
									WHERE ' . $db->sql_in_set('topic_id', array_keys($topic_id_ary));
								$result = $db->sql_query($sql);

								while ($row = $db->sql_fetchrow($result))
								{
									if (max($row['topic_replies'], $row['topic_replies_real']) + 1 == $topic_id_ary[$row['topic_id']])
									{
										$move_topic_ary[] = $row['topic_id'];
									}
									else
									{
										$move_post_ary[$row['topic_id']]['title'] = $row['topic_title'];
										$move_post_ary[$row['topic_id']]['attach'] = ($row['topic_attachment']) ? 1 : 0;
									}

									$forum_id_ary[] = $row['forum_id'];
								}
								$db->sql_freeresult($result);
							}

							// Entire topic comprises posts by this user, move these topics
							if (sizeof($move_topic_ary))
							{
								move_topics($move_topic_ary, $new_forum_id, false);
							}

							if (sizeof($move_post_ary))
							{
								// Create new topic
								// Update post_ids, report_ids, attachment_ids
								foreach ($move_post_ary as $topic_id => $post_ary)
								{
									// Create new topic
									$sql = 'INSERT INTO ' . TOPICS_TABLE . ' ' . $db->sql_build_array('INSERT', array(
										'topic_poster'				=> $user_id,
										'topic_time'				=> time(),
										'forum_id' 					=> $new_forum_id,
										'icon_id'					=> 0,
										'topic_approved'			=> 1,
										'topic_title' 				=> $post_ary['title'],
										'topic_first_poster_name'	=> $user_row['username'],
										'topic_type'				=> POST_NORMAL,
										'topic_time_limit'			=> 0,
										'topic_attachment'			=> $post_ary['attach'])
									);
									$db->sql_query($sql);

									$new_topic_id = $db->sql_nextid();

									// Move posts
									$sql = 'UPDATE ' . POSTS_TABLE . "
										SET forum_id = $new_forum_id, topic_id = $new_topic_id
										WHERE topic_id = $topic_id
											AND poster_id = $user_id";
									$db->sql_query($sql);

									if ($post_ary['attach'])
									{
										$sql = 'UPDATE ' . ATTACHMENTS_TABLE . "
											SET topic_id = $new_topic_id
											WHERE topic_id = $topic_id
												AND poster_id = $user_id";
										$db->sql_query($sql);
									}

									$new_topic_id_ary[] = $new_topic_id;
								}
							}

							$forum_id_ary = array_unique($forum_id_ary);
							$topic_id_ary = array_unique(array_merge(array_keys($topic_id_ary), $new_topic_id_ary));

							if (sizeof($topic_id_ary))
							{
								sync('reported', 'topic_id', $topic_id_ary);
								sync('topic', 'topic_id', $topic_id_ary);
							}

							if (sizeof($forum_id_ary))
							{
								sync('forum', 'forum_id', $forum_id_ary, false, true);
							}


							add_log('admin', 'LOG_USER_MOVE_POSTS', $user_row['username'], $forum_info['forum_name']);
							add_log('user', $user_id, 'LOG_USER_MOVE_POSTS_USER', $forum_info['forum_name']);

							trigger_error($user->lang['USER_POSTS_MOVED'] . adm_back_link($this->u_action . '&amp;u=' . $user_id));

						break;
					}

					// Handle registration info updates
					$data = array(
						'username'			=> utf8_normalize_nfc(request_var('user', $user_row['username'], true)),
						'user_founder'		=> request_var('user_founder', ($user_row['user_type'] == USER_FOUNDER) ? 1 : 0),
						'email'				=> strtolower(request_var('user_email', $user_row['user_email'])),
						'email_confirm'		=> strtolower(request_var('email_confirm', '')),
						'new_password'		=> request_var('new_password', '', true),
						'password_confirm'	=> request_var('password_confirm', '', true),
					);

					// Validation data - we do not check the password complexity setting here
					$check_ary = array(
						'new_password'		=> array(
							array('string', true, $config['min_pass_chars'], $config['max_pass_chars']),
							array('password')),
						'password_confirm'	=> array('string', true, $config['min_pass_chars'], $config['max_pass_chars']),
					);

					// Check username if altered
					if ($data['username'] != $user_row['username'])
					{
						$check_ary += array(
							'username'			=> array(
								array('string', false, $config['min_name_chars'], $config['max_name_chars']),
								array('username', $user_row['username'])
							),
						);
					}

					// Check email if altered
					if ($data['email'] != $user_row['user_email'])
					{
						$check_ary += array(
							'email'				=> array(
								array('string', false, 6, 60),
								array('email', $user_row['user_email'])
							),
							'email_confirm'		=> array('string', true, 6, 60)
						);
					}

					$error = validate_data($data, $check_ary);

					if ($data['new_password'] && $data['password_confirm'] != $data['new_password'])
					{
						$error[] = 'NEW_PASSWORD_ERROR';
					}

					if ($data['email'] != $user_row['user_email'] && $data['email_confirm'] != $data['email'])
					{
						$error[] = 'NEW_EMAIL_ERROR';
					}

					if (!check_form_key($form_name))
					{
						$error[] = 'FORM_INVALID';
					}

					// Which updates do we need to do?
					$update_username = ($user_row['username'] != $data['username']) ? $data['username'] : false;
					$update_password = ($data['new_password'] && !phpbb_check_hash($user_row['user_password'], $data['new_password'])) ? true : false;
					$update_email = ($data['email'] != $user_row['user_email']) ? $data['email'] : false;

					if (!sizeof($error))
					{
						$sql_ary = array();

						if ($user_row['user_type'] != USER_FOUNDER || $user->data['user_type'] == USER_FOUNDER)
						{
							// Only allow founders updating the founder status...
							if ($user->data['user_type'] == USER_FOUNDER)
							{
								// Setting a normal member to be a founder
								if ($data['user_founder'] && $user_row['user_type'] != USER_FOUNDER)
								{
									// Make sure the user is not setting an Inactive or ignored user to be a founder
									if ($user_row['user_type'] == USER_IGNORE)
									{
										trigger_error($user->lang['CANNOT_SET_FOUNDER_IGNORED'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
									}

									if ($user_row['user_type'] == USER_INACTIVE)
									{
										trigger_error($user->lang['CANNOT_SET_FOUNDER_INACTIVE'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
									}

									$sql_ary['user_type'] = USER_FOUNDER;
								}
								else if (!$data['user_founder'] && $user_row['user_type'] == USER_FOUNDER)
								{
									// Check if at least one founder is present
									$sql = 'SELECT user_id
										FROM ' . USERS_TABLE . '
										WHERE user_type = ' . USER_FOUNDER . '
											AND user_id <> ' . $user_id;
									$result = $db->sql_query_limit($sql, 1);
									$row = $db->sql_fetchrow($result);
									$db->sql_freeresult($result);

									if ($row)
									{
										$sql_ary['user_type'] = USER_NORMAL;
									}
									else
									{
										trigger_error($user->lang['AT_LEAST_ONE_FOUNDER'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
									}
								}
							}
						}

						if ($update_username !== false)
						{
							$sql_ary['username'] = $update_username;
							$sql_ary['username_clean'] = utf8_clean_string($update_username);

							add_log('user', $user_id, 'LOG_USER_UPDATE_NAME', $user_row['username'], $update_username);
						}

						if ($update_email !== false)
						{
							$sql_ary += array(
								'user_email'		=> $update_email,
								'user_email_hash'	=> crc32($update_email) . strlen($update_email)
							);

							add_log('user', $user_id, 'LOG_USER_UPDATE_EMAIL', $user_row['username'], $user_row['user_email'], $update_email);
						}

						if ($update_password)
						{
							$sql_ary += array(
								'user_password'		=> phpbb_hash($data['new_password']),
								'user_passchg'		=> time(),
								'user_pass_convert'	=> 0,
							);

							$user->reset_login_keys($user_id);
							add_log('user', $user_id, 'LOG_USER_NEW_PASSWORD', $user_row['username']);
						}

						if (sizeof($sql_ary))
						{
							$sql = 'UPDATE ' . USERS_TABLE . '
								SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
								WHERE user_id = ' . $user_id;
							$db->sql_query($sql);
						}

						if ($update_username)
						{
							user_update_name($user_row['username'], $update_username);
						}

						// Let the users permissions being updated
						$auth->acl_clear_prefetch($user_id);

						add_log('admin', 'LOG_USER_USER_UPDATE', $data['username']);

						trigger_error($user->lang['USER_OVERVIEW_UPDATED'] . adm_back_link($this->u_action . '&amp;u=' . $user_id));
					}

					// Replace "error" strings with their real, localised form
					$error = preg_replace('#^([A-Z_]+)$#e', "(!empty(\$user->lang['\\1'])) ? \$user->lang['\\1'] : '\\1'", $error);
				}

				if ($user_id == $user->data['user_id'])
				{
					$quick_tool_ary = array('delsig' => 'DEL_SIG', 'delavatar' => 'DEL_AVATAR', 'moveposts' => 'MOVE_POSTS', 'delposts' => 'DEL_POSTS', 'delattach' => 'DEL_ATTACH');
				}
				else
				{
					$quick_tool_ary = array();

					if ($user_row['user_type'] != USER_FOUNDER)
					{
						$quick_tool_ary += array('banuser' => 'BAN_USER', 'banemail' => 'BAN_EMAIL', 'banip' => 'BAN_IP');
					}

					if ($user_row['user_type'] != USER_FOUNDER && $user_row['user_type'] != USER_IGNORE)
					{
						$quick_tool_ary += array('active' => (($user_row['user_type'] == USER_INACTIVE) ? 'ACTIVATE' : 'DEACTIVATE'));
					}

					$quick_tool_ary += array('delsig' => 'DEL_SIG', 'delavatar' => 'DEL_AVATAR', 'moveposts' => 'MOVE_POSTS', 'delposts' => 'DEL_POSTS', 'delattach' => 'DEL_ATTACH');

					if ($config['email_enable'] && ($user_row['user_type'] == USER_NORMAL || $user_row['user_type'] == USER_INACTIVE))
					{
						$quick_tool_ary['reactivate'] = 'FORCE';
					}
				}

				$s_action_options = '<option class="sep" value="">' . $user->lang['SELECT_OPTION'] . '</option>';
				foreach ($quick_tool_ary as $value => $lang)
				{
					$s_action_options .= '<option value="' . $value . '">' . $user->lang['USER_ADMIN_' . $lang] . '</option>';
				}

				if ($config['load_onlinetrack'])
				{
					$sql = 'SELECT MAX(session_time) AS session_time, MIN(session_viewonline) AS session_viewonline
						FROM ' . SESSIONS_TABLE . "
						WHERE session_user_id = $user_id";
					$result = $db->sql_query($sql);
					$row = $db->sql_fetchrow($result);
					$db->sql_freeresult($result);

					$user_row['session_time'] = (isset($row['session_time'])) ? $row['session_time'] : 0;
					$user_row['session_viewonline'] = (isset($row['session_viewonline'])) ? $row['session_viewonline'] : 0;
					unset($row);
				}

				$last_visit = (!empty($user_row['session_time'])) ? $user_row['session_time'] : $user_row['user_lastvisit'];

				$inactive_reason = '';
				if ($user_row['user_type'] == USER_INACTIVE)
				{
					$inactive_reason = $user->lang['INACTIVE_REASON_UNKNOWN'];

					switch ($user_row['user_inactive_reason'])
					{
						case INACTIVE_REGISTER:
							$inactive_reason = $user->lang['INACTIVE_REASON_REGISTER'];
						break;

						case INACTIVE_PROFILE:
							$inactive_reason = $user->lang['INACTIVE_REASON_PROFILE'];
						break;

						case INACTIVE_MANUAL:
							$inactive_reason = $user->lang['INACTIVE_REASON_MANUAL'];
						break;

						case INACTIVE_REMIND:
							$inactive_reason = $user->lang['INACTIVE_REASON_REMIND'];
						break;
					}
				}

				$template->assign_vars(array(
					'L_NAME_CHARS_EXPLAIN'		=> sprintf($user->lang[$config['allow_name_chars'] . '_EXPLAIN'], $config['min_name_chars'], $config['max_name_chars']),
					'L_CHANGE_PASSWORD_EXPLAIN'	=> sprintf($user->lang[$config['pass_complex'] . '_EXPLAIN'], $config['min_pass_chars'], $config['max_pass_chars']),
					'S_FOUNDER'					=> ($user->data['user_type'] == USER_FOUNDER) ? true : false,

					'S_OVERVIEW'		=> true,
					'S_USER_IP'			=> ($user_row['user_ip']) ? true : false,
					'S_USER_FOUNDER'	=> ($user_row['user_type'] == USER_FOUNDER) ? true : false,
					'S_ACTION_OPTIONS'	=> $s_action_options,
					'S_OWN_ACCOUNT'		=> ($user_id == $user->data['user_id']) ? true : false,
					'S_USER_INACTIVE'	=> ($user_row['user_type'] == USER_INACTIVE) ? true : false,

					'U_SHOW_IP'		=> $this->u_action . "&amp;u=$user_id&amp;ip=" . (($ip == 'ip') ? 'hostname' : 'ip'),
					'U_WHOIS'		=> $this->u_action . "&amp;action=whois&amp;user_ip={$user_row['user_ip']}",

					'U_SWITCH_PERMISSIONS'	=> ($auth->acl_get('a_switchperm') && $user->data['user_id'] != $user_row['user_id']) ? append_sid("{$phpbb_root_path}ucp.$phpEx", "mode=switch_perm&amp;u={$user_row['user_id']}") : '',

					'USER'				=> $user_row['username'],
					'USER_REGISTERED'	=> $user->format_date($user_row['user_regdate']),
					'REGISTERED_IP'		=> ($ip == 'hostname') ? gethostbyaddr($user_row['user_ip']) : $user_row['user_ip'],
					'USER_LASTACTIVE'	=> ($last_visit) ? $user->format_date($last_visit) : ' - ',
					'USER_EMAIL'		=> $user_row['user_email'],
					'USER_WARNINGS'		=> $user_row['user_warnings'],
					'USER_POSTS'		=> $user_row['user_posts'],
					'USER_INACTIVE_REASON'	=> $inactive_reason,
				));

			break;

			case 'feedback':

				$user->add_lang('mcp');

				// Set up general vars
				$start		= request_var('start', 0);
				$deletemark = (isset($_POST['delmarked'])) ? true : false;
				$deleteall	= (isset($_POST['delall'])) ? true : false;
				$marked		= request_var('mark', array(0));
				$message	= utf8_normalize_nfc(request_var('message', '', true));

				// Sort keys
				$sort_days	= request_var('st', 0);
				$sort_key	= request_var('sk', 't');
				$sort_dir	= request_var('sd', 'd');

				// Delete entries if requested and able
				if (($deletemark || $deleteall) && $auth->acl_get('a_clearlogs'))
				{
					if (!check_form_key($form_name))
					{
						trigger_error($user->lang['FORM_INVALID'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
					}

					$where_sql = '';
					if ($deletemark && $marked)
					{
						$sql_in = array();
						foreach ($marked as $mark)
						{
							$sql_in[] = $mark;
						}
						$where_sql = ' AND ' . $db->sql_in_set('log_id', $sql_in);
						unset($sql_in);
					}

					if ($where_sql || $deleteall)
					{
						$sql = 'DELETE FROM ' . LOG_TABLE . '
							WHERE log_type = ' . LOG_USERS . "
							$where_sql";
						$db->sql_query($sql);

						add_log('admin', 'LOG_CLEAR_USER', $user_row['username']);
					}
				}

				if ($submit && $message)
				{
					if (!check_form_key($form_name))
					{
						trigger_error($user->lang['FORM_INVALID'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
					}

					add_log('admin', 'LOG_USER_FEEDBACK', $user_row['username']);
					add_log('mod', 0, 0, 'LOG_USER_FEEDBACK', $user_row['username']);
					add_log('user', $user_id, 'LOG_USER_GENERAL', $message);

					trigger_error($user->lang['USER_FEEDBACK_ADDED'] . adm_back_link($this->u_action . '&amp;u=' . $user_id));
				}

				// Sorting
				$limit_days = array(0 => $user->lang['ALL_ENTRIES'], 1 => $user->lang['1_DAY'], 7 => $user->lang['7_DAYS'], 14 => $user->lang['2_WEEKS'], 30 => $user->lang['1_MONTH'], 90 => $user->lang['3_MONTHS'], 180 => $user->lang['6_MONTHS'], 365 => $user->lang['1_YEAR']);
				$sort_by_text = array('u' => $user->lang['SORT_USERNAME'], 't' => $user->lang['SORT_DATE'], 'i' => $user->lang['SORT_IP'], 'o' => $user->lang['SORT_ACTION']);
				$sort_by_sql = array('u' => 'u.username_clean', 't' => 'l.log_time', 'i' => 'l.log_ip', 'o' => 'l.log_operation');

				$s_limit_days = $s_sort_key = $s_sort_dir = $u_sort_param = '';
				gen_sort_selects($limit_days, $sort_by_text, $sort_days, $sort_key, $sort_dir, $s_limit_days, $s_sort_key, $s_sort_dir, $u_sort_param);

				// Define where and sort sql for use in displaying logs
				$sql_where = ($sort_days) ? (time() - ($sort_days * 86400)) : 0;
				$sql_sort = $sort_by_sql[$sort_key] . ' ' . (($sort_dir == 'd') ? 'DESC' : 'ASC');

				// Grab log data
				$log_data = array();
				$log_count = 0;
				view_log('user', $log_data, $log_count, $config['topics_per_page'], $start, 0, 0, $user_id, $sql_where, $sql_sort);

				$template->assign_vars(array(
					'S_FEEDBACK'	=> true,
					'S_ON_PAGE'		=> on_page($log_count, $config['topics_per_page'], $start),
					'PAGINATION'	=> generate_pagination($this->u_action . "&amp;u=$user_id&amp;$u_sort_param", $log_count, $config['topics_per_page'], $start, true),

					'S_LIMIT_DAYS'	=> $s_limit_days,
					'S_SORT_KEY'	=> $s_sort_key,
					'S_SORT_DIR'	=> $s_sort_dir,
					'S_CLEARLOGS'	=> $auth->acl_get('a_clearlogs'))
				);

				foreach ($log_data as $row)
				{
					$template->assign_block_vars('log', array(
						'USERNAME'		=> $row['username_full'],
						'IP'			=> $row['ip'],
						'DATE'			=> $user->format_date($row['time']),
						'ACTION'		=> nl2br($row['action']),
						'ID'			=> $row['id'])
					);
				}

			break;

			case 'profile':

				include($phpbb_root_path . 'includes/functions_user.' . $phpEx);
				include($phpbb_root_path . 'includes/functions_profile_fields.' . $phpEx);

				$cp = new custom_profile();

				$cp_data = $cp_error = array();

				$sql = 'SELECT lang_id
					FROM ' . LANG_TABLE . "
					WHERE lang_iso = '" . $db->sql_escape($user->data['user_lang']) . "'";
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				$user_row['iso_lang_id'] = $row['lang_id'];

				$data = array(
					'icq'			=> request_var('icq', $user_row['user_icq']),
					'aim'			=> request_var('aim', $user_row['user_aim']),
					'msn'			=> request_var('msn', $user_row['user_msnm']),
					'yim'			=> request_var('yim', $user_row['user_yim']),
					'jabber'		=> utf8_normalize_nfc(request_var('jabber', $user_row['user_jabber'], true)),
					'website'		=> request_var('website', $user_row['user_website']),
					'location'		=> utf8_normalize_nfc(request_var('location', $user_row['user_from'], true)),
					'occupation'	=> utf8_normalize_nfc(request_var('occupation', $user_row['user_occ'], true)),
					'interests'		=> utf8_normalize_nfc(request_var('interests', $user_row['user_interests'], true)),
					'bday_day'		=> 0,
					'bday_month'	=> 0,
					'bday_year'		=> 0,
				);

				if ($user_row['user_birthday'])
				{
					list($data['bday_day'], $data['bday_month'], $data['bday_year']) = explode('-', $user_row['user_birthday']);
				}

				$data['bday_day']		= request_var('bday_day', $data['bday_day']);
				$data['bday_month']		= request_var('bday_month', $data['bday_month']);
				$data['bday_year']		= request_var('bday_year', $data['bday_year']);
				$data['user_birthday']	= sprintf('%2d-%2d-%4d', $data['bday_day'], $data['bday_month'], $data['bday_year']);


				if ($submit)
				{
					$error = validate_data($data, array(
						'icq'			=> array(
							array('string', true, 3, 15),
							array('match', true, '#^[0-9]+$#i')),
						'aim'			=> array('string', true, 3, 255),
						'msn'			=> array('string', true, 5, 255),
						'jabber'		=> array(
							array('string', true, 5, 255),
							array('jabber')),
						'yim'			=> array('string', true, 5, 255),
						'website'		=> array(
							array('string', true, 12, 255),
							array('match', true, '#^http[s]?://(.*?\.)*?[a-z0-9\-]+\.[a-z]{2,4}#i')),
						'location'		=> array('string', true, 2, 255),
						'occupation'	=> array('string', true, 2, 500),
						'interests'		=> array('string', true, 2, 500),
						'bday_day'		=> array('num', true, 1, 31),
						'bday_month'	=> array('num', true, 1, 12),
						'bday_year'		=> array('num', true, 1901, gmdate('Y', time())),
						'user_birthday'	=> array('date', true),
					));

					// validate custom profile fields
					$cp->submit_cp_field('profile', $user_row['iso_lang_id'], $cp_data, $cp_error);

					if (sizeof($cp_error))
					{
						$error = array_merge($error, $cp_error);
					}
					if (!check_form_key($form_name))
					{
						$error[] = 'FORM_INVALID';
					}

					if (!sizeof($error))
					{
						$sql_ary = array(
							'user_icq'		=> $data['icq'],
							'user_aim'		=> $data['aim'],
							'user_msnm'		=> $data['msn'],
							'user_yim'		=> $data['yim'],
							'user_jabber'	=> $data['jabber'],
							'user_website'	=> $data['website'],
							'user_from'		=> $data['location'],
							'user_occ'		=> $data['occupation'],
							'user_interests'=> $data['interests'],
							'user_birthday'	=> $data['user_birthday'],
						);

						$sql = 'UPDATE ' . USERS_TABLE . '
							SET ' . $db->sql_build_array('UPDATE', $sql_ary) . "
							WHERE user_id = $user_id";
						$db->sql_query($sql);

						// Update Custom Fields
						if (sizeof($cp_data))
						{
							switch ($db->sql_layer)
							{
								case 'oracle':
								case 'firebird':
								case 'postgres':
									$right_delim = $left_delim = '"';
								break;

								case 'sqlite':
								case 'mssql':
								case 'mssql_odbc':
									$right_delim = ']';
									$left_delim = '[';
								break;

								case 'mysql':
								case 'mysql4':
								case 'mysqli':
									$right_delim = $left_delim = '`';
								break;
							}

							foreach ($cp_data as $key => $value)
							{
								$cp_data[$left_delim . $key . $right_delim] = $value;
								unset($cp_data[$key]);
							}

							$sql = 'UPDATE ' . PROFILE_FIELDS_DATA_TABLE . '
								SET ' . $db->sql_build_array('UPDATE', $cp_data) . "
								WHERE user_id = $user_id";
							$db->sql_query($sql);

							if (!$db->sql_affectedrows())
							{
								$cp_data['user_id'] = (int) $user_id;

								$db->sql_return_on_error(true);

								$sql = 'INSERT INTO ' . PROFILE_FIELDS_DATA_TABLE . ' ' . $db->sql_build_array('INSERT', $cp_data);
								$db->sql_query($sql);

								$db->sql_return_on_error(false);
							}
						}

						trigger_error($user->lang['USER_PROFILE_UPDATED'] . adm_back_link($this->u_action . '&amp;u=' . $user_id));
					}

					// Replace "error" strings with their real, localised form
					$error = preg_replace('#^([A-Z_]+)$#e', "(!empty(\$user->lang['\\1'])) ? \$user->lang['\\1'] : '\\1'", $error);
				}

				$s_birthday_day_options = '<option value="0"' . ((!$data['bday_day']) ? ' selected="selected"' : '') . '>--</option>';
				for ($i = 1; $i < 32; $i++)
				{
					$selected = ($i == $data['bday_day']) ? ' selected="selected"' : '';
					$s_birthday_day_options .= "<option value=\"$i\"$selected>$i</option>";
				}

				$s_birthday_month_options = '<option value="0"' . ((!$data['bday_month']) ? ' selected="selected"' : '') . '>--</option>';
				for ($i = 1; $i < 13; $i++)
				{
					$selected = ($i == $data['bday_month']) ? ' selected="selected"' : '';
					$s_birthday_month_options .= "<option value=\"$i\"$selected>$i</option>";
				}
				$s_birthday_year_options = '';

				$now = getdate();
				$s_birthday_year_options = '<option value="0"' . ((!$data['bday_year']) ? ' selected="selected"' : '') . '>--</option>';
				for ($i = $now['year'] - 100; $i < $now['year']; $i++)
				{
					$selected = ($i == $data['bday_year']) ? ' selected="selected"' : '';
					$s_birthday_year_options .= "<option value=\"$i\"$selected>$i</option>";
				}
				unset($now);

				$template->assign_vars(array(
					'ICQ'			=> $data['icq'],
					'YIM'			=> $data['yim'],
					'AIM'			=> $data['aim'],
					'MSN'			=> $data['msn'],
					'JABBER'		=> $data['jabber'],
					'WEBSITE'		=> $data['website'],
					'LOCATION'		=> $data['location'],
					'OCCUPATION'	=> $data['occupation'],
					'INTERESTS'		=> $data['interests'],

					'S_BIRTHDAY_DAY_OPTIONS'	=> $s_birthday_day_options,
					'S_BIRTHDAY_MONTH_OPTIONS'	=> $s_birthday_month_options,
					'S_BIRTHDAY_YEAR_OPTIONS'	=> $s_birthday_year_options,

					'S_PROFILE'		=> true)
				);

				// Get additional profile fields and assign them to the template block var 'profile_fields'
				$user->get_profile_fields($user_id);

				$cp->generate_profile_fields('profile', $user_row['iso_lang_id']);

			break;

			case 'prefs':

				include($phpbb_root_path . 'includes/functions_user.' . $phpEx);

				$data = array(
					'dateformat'		=> utf8_normalize_nfc(request_var('dateformat', $user_row['user_dateformat'], true)),
					'lang'				=> basename(request_var('lang', $user_row['user_lang'])),
					'tz'				=> request_var('tz', (float) $user_row['user_timezone']),
					'style'				=> request_var('style', $user_row['user_style']),
					'dst'				=> request_var('dst', $user_row['user_dst']),
					'viewemail'			=> request_var('viewemail', $user_row['user_allow_viewemail']),
					'massemail'			=> request_var('massemail', $user_row['user_allow_massemail']),
					'hideonline'		=> request_var('hideonline', !$user_row['user_allow_viewonline']),
					'notifymethod'		=> request_var('notifymethod', $user_row['user_notify_type']),
					'notifypm'			=> request_var('notifypm', $user_row['user_notify_pm']),
					'popuppm'			=> request_var('popuppm', $this->optionget($user_row, 'popuppm')),
					'allowpm'			=> request_var('allowpm', $user_row['user_allow_pm']),

					'topic_sk'			=> request_var('topic_sk', ($user_row['user_topic_sortby_type']) ? $user_row['user_topic_sortby_type'] : 't'),
					'topic_sd'			=> request_var('topic_sd', ($user_row['user_topic_sortby_dir']) ? $user_row['user_topic_sortby_dir'] : 'd'),
					'topic_st'			=> request_var('topic_st', ($user_row['user_topic_show_days']) ? $user_row['user_topic_show_days'] : 0),

					'post_sk'			=> request_var('post_sk', ($user_row['user_post_sortby_type']) ? $user_row['user_post_sortby_type'] : 't'),
					'post_sd'			=> request_var('post_sd', ($user_row['user_post_sortby_dir']) ? $user_row['user_post_sortby_dir'] : 'a'),
					'post_st'			=> request_var('post_st', ($user_row['user_post_show_days']) ? $user_row['user_post_show_days'] : 0),

					'view_images'		=> request_var('view_images', $this->optionget($user_row, 'viewimg')),
					'view_flash'		=> request_var('view_flash', $this->optionget($user_row, 'viewflash')),
					'view_smilies'		=> request_var('view_smilies', $this->optionget($user_row, 'viewsmilies')),
					'view_sigs'			=> request_var('view_sigs', $this->optionget($user_row, 'viewsigs')),
					'view_avatars'		=> request_var('view_avatars', $this->optionget($user_row, 'viewavatars')),
					'view_wordcensor'	=> request_var('view_wordcensor', $this->optionget($user_row, 'viewcensors')),

					'bbcode'	=> request_var('bbcode', $this->optionget($user_row, 'bbcode')),
					'smilies'	=> request_var('smilies', $this->optionget($user_row, 'smilies')),
					'sig'		=> request_var('sig', $this->optionget($user_row, 'attachsig')),
					'notify'	=> request_var('notify', $user_row['user_notify']),
				);

				if ($submit)
				{
					$error = validate_data($data, array(
						'dateformat'	=> array('string', false, 1, 30),
						'lang'			=> array('match', false, '#^[a-z_\-]{2,}$#i'),
						'tz'			=> array('num', false, -14, 14),

						'topic_sk'		=> array('string', false, 1, 1),
						'topic_sd'		=> array('string', false, 1, 1),
						'post_sk'		=> array('string', false, 1, 1),
						'post_sd'		=> array('string', false, 1, 1),
					));

					if (!check_form_key($form_name))
					{
						$error[] = 'FORM_INVALID';
					}

					if (!sizeof($error))
					{
						$this->optionset($user_row, 'popuppm', $data['popuppm']);
						$this->optionset($user_row, 'viewimg', $data['view_images']);
						$this->optionset($user_row, 'viewflash', $data['view_flash']);
						$this->optionset($user_row, 'viewsmilies', $data['view_smilies']);
						$this->optionset($user_row, 'viewsigs', $data['view_sigs']);
						$this->optionset($user_row, 'viewavatars', $data['view_avatars']);
						$this->optionset($user_row, 'viewcensors', $data['view_wordcensor']);
						$this->optionset($user_row, 'bbcode', $data['bbcode']);
						$this->optionset($user_row, 'smilies', $data['smilies']);
						$this->optionset($user_row, 'attachsig', $data['sig']);

						$sql_ary = array(
							'user_options'			=> $user_row['user_options'],

							'user_allow_pm'			=> $data['allowpm'],
							'user_allow_viewemail'	=> $data['viewemail'],
							'user_allow_massemail'	=> $data['massemail'],
							'user_allow_viewonline'	=> !$data['hideonline'],
							'user_notify_type'		=> $data['notifymethod'],
							'user_notify_pm'		=> $data['notifypm'],

							'user_dst'				=> $data['dst'],
							'user_dateformat'		=> $data['dateformat'],
							'user_lang'				=> $data['lang'],
							'user_timezone'			=> $data['tz'],
							'user_style'			=> $data['style'],

							'user_topic_sortby_type'	=> $data['topic_sk'],
							'user_post_sortby_type'		=> $data['post_sk'],
							'user_topic_sortby_dir'		=> $data['topic_sd'],
							'user_post_sortby_dir'		=> $data['post_sd'],

							'user_topic_show_days'	=> $data['topic_st'],
							'user_post_show_days'	=> $data['post_st'],

							'user_notify'	=> $data['notify'],
						);

						$sql = 'UPDATE ' . USERS_TABLE . '
							SET ' . $db->sql_build_array('UPDATE', $sql_ary) . "
							WHERE user_id = $user_id";
						$db->sql_query($sql);

						trigger_error($user->lang['USER_PREFS_UPDATED'] . adm_back_link($this->u_action . '&amp;u=' . $user_id));
					}

					// Replace "error" strings with their real, localised form
					$error = preg_replace('#^([A-Z_]+)$#e', "(!empty(\$user->lang['\\1'])) ? \$user->lang['\\1'] : '\\1'", $error);
				}

				$dateformat_options = '';
				foreach ($user->lang['dateformats'] as $format => $null)
				{
					$dateformat_options .= '<option value="' . $format . '"' . (($format == $data['dateformat']) ? ' selected="selected"' : '') . '>';
					$dateformat_options .= $user->format_date(time(), $format, false) . ((strpos($format, '|') !== false) ? $user->lang['VARIANT_DATE_SEPARATOR'] . $user->format_date(time(), $format, true) : '');
					$dateformat_options .= '</option>';
				}

				$s_custom = false;

				$dateformat_options .= '<option value="custom"';
				if (!isset($user->lang['dateformats'][$data['dateformat']]))
				{
					$dateformat_options .= ' selected="selected"';
					$s_custom = true;
				}
				$dateformat_options .= '>' . $user->lang['CUSTOM_DATEFORMAT'] . '</option>';

				$sort_dir_text = array('a' => $user->lang['ASCENDING'], 'd' => $user->lang['DESCENDING']);

				// Topic ordering options
				$limit_topic_days = array(0 => $user->lang['ALL_TOPICS'], 1 => $user->lang['1_DAY'], 7 => $user->lang['7_DAYS'], 14 => $user->lang['2_WEEKS'], 30 => $user->lang['1_MONTH'], 90 => $user->lang['3_MONTHS'], 180 => $user->lang['6_MONTHS'], 365 => $user->lang['1_YEAR']);
				$sort_by_topic_text = array('a' => $user->lang['AUTHOR'], 't' => $user->lang['POST_TIME'], 'r' => $user->lang['REPLIES'], 's' => $user->lang['SUBJECT'], 'v' => $user->lang['VIEWS']);

				// Post ordering options
				$limit_post_days = array(0 => $user->lang['ALL_POSTS'], 1 => $user->lang['1_DAY'], 7 => $user->lang['7_DAYS'], 14 => $user->lang['2_WEEKS'], 30 => $user->lang['1_MONTH'], 90 => $user->lang['3_MONTHS'], 180 => $user->lang['6_MONTHS'], 365 => $user->lang['1_YEAR']);
				$sort_by_post_text = array('a' => $user->lang['AUTHOR'], 't' => $user->lang['POST_TIME'], 's' => $user->lang['SUBJECT']);

				$_options = array('topic', 'post');
				foreach ($_options as $sort_option)
				{
					${'s_limit_' . $sort_option . '_days'} = '<select name="' . $sort_option . '_st">';
					foreach (${'limit_' . $sort_option . '_days'} as $day => $text)
					{
						$selected = ($data[$sort_option . '_st'] == $day) ? ' selected="selected"' : '';
						${'s_limit_' . $sort_option . '_days'} .= '<option value="' . $day . '"' . $selected . '>' . $text . '</option>';
					}
					${'s_limit_' . $sort_option . '_days'} .= '</select>';

					${'s_sort_' . $sort_option . '_key'} = '<select name="' . $sort_option . '_sk">';
					foreach (${'sort_by_' . $sort_option . '_text'} as $key => $text)
					{
						$selected = ($data[$sort_option . '_sk'] == $key) ? ' selected="selected"' : '';
						${'s_sort_' . $sort_option . '_key'} .= '<option value="' . $key . '"' . $selected . '>' . $text . '</option>';
					}
					${'s_sort_' . $sort_option . '_key'} .= '</select>';

					${'s_sort_' . $sort_option . '_dir'} = '<select name="' . $sort_option . '_sd">';
					foreach ($sort_dir_text as $key => $value)
					{
						$selected = ($data[$sort_option . '_sd'] == $key) ? ' selected="selected"' : '';
						${'s_sort_' . $sort_option . '_dir'} .= '<option value="' . $key . '"' . $selected . '>' . $value . '</option>';
					}
					${'s_sort_' . $sort_option . '_dir'} .= '</select>';
				}

				$template->assign_vars(array(
					'S_PREFS'			=> true,
					'S_JABBER_DISABLED'	=> ($config['jab_enable'] && $user_row['user_jabber'] && @extension_loaded('xml')) ? false : true,

					'VIEW_EMAIL'		=> $data['viewemail'],
					'MASS_EMAIL'		=> $data['massemail'],
					'ALLOW_PM'			=> $data['allowpm'],
					'HIDE_ONLINE'		=> $data['hideonline'],
					'NOTIFY_EMAIL'		=> ($data['notifymethod'] == NOTIFY_EMAIL) ? true : false,
					'NOTIFY_IM'			=> ($data['notifymethod'] == NOTIFY_IM) ? true : false,
					'NOTIFY_BOTH'		=> ($data['notifymethod'] == NOTIFY_BOTH) ? true : false,
					'NOTIFY_PM'			=> $data['notifypm'],
					'POPUP_PM'			=> $data['popuppm'],
					'DST'				=> $data['dst'],
					'BBCODE'			=> $data['bbcode'],
					'SMILIES'			=> $data['smilies'],
					'ATTACH_SIG'		=> $data['sig'],
					'NOTIFY'			=> $data['notify'],
					'VIEW_IMAGES'		=> $data['view_images'],
					'VIEW_FLASH'		=> $data['view_flash'],
					'VIEW_SMILIES'		=> $data['view_smilies'],
					'VIEW_SIGS'			=> $data['view_sigs'],
					'VIEW_AVATARS'		=> $data['view_avatars'],
					'VIEW_WORDCENSOR'	=> $data['view_wordcensor'],

					'S_TOPIC_SORT_DAYS'		=> $s_limit_topic_days,
					'S_TOPIC_SORT_KEY'		=> $s_sort_topic_key,
					'S_TOPIC_SORT_DIR'		=> $s_sort_topic_dir,
					'S_POST_SORT_DAYS'		=> $s_limit_post_days,
					'S_POST_SORT_KEY'		=> $s_sort_post_key,
					'S_POST_SORT_DIR'		=> $s_sort_post_dir,

					'DATE_FORMAT'			=> $data['dateformat'],
					'S_DATEFORMAT_OPTIONS'	=> $dateformat_options,
					'S_CUSTOM_DATEFORMAT'	=> $s_custom,
					'DEFAULT_DATEFORMAT'	=> $config['default_dateformat'],
					'A_DEFAULT_DATEFORMAT'	=> addslashes($config['default_dateformat']),

					'S_LANG_OPTIONS'	=> language_select($data['lang']),
					'S_STYLE_OPTIONS'	=> style_select($data['style']),
					'S_TZ_OPTIONS'		=> tz_select($data['tz'], true),
					)
				);

			break;

			case 'avatar':

				include($phpbb_root_path . 'includes/functions_display.' . $phpEx);
				include($phpbb_root_path . 'includes/functions_user.' . $phpEx);

				$can_upload = (file_exists($phpbb_root_path . $config['avatar_path']) && @is_writable($phpbb_root_path . $config['avatar_path']) && $file_uploads) ? true : false;

				if ($submit)
				{

					if (!check_form_key($form_name))
					{
							trigger_error($user->lang['FORM_INVALID'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
					}

					if (avatar_process_user($error, $user_row))
					{
						trigger_error($user->lang['USER_AVATAR_UPDATED'] . adm_back_link($this->u_action . '&amp;u=' . $user_row['user_id']));
					}

					// Replace "error" strings with their real, localised form
					$error = preg_replace('#^([A-Z_]+)$#e', "(!empty(\$user->lang['\\1'])) ? \$user->lang['\\1'] : '\\1'", $error);
				}

				// Generate users avatar
				$avatar_img = ($user_row['user_avatar']) ? get_user_avatar($user_row['user_avatar'], $user_row['user_avatar_type'], $user_row['user_avatar_width'], $user_row['user_avatar_height']) : '<img src="' . $phpbb_admin_path . 'images/no_avatar.gif" alt="" />';

				$display_gallery = (isset($_POST['display_gallery'])) ? true : false;
				$avatar_select = basename(request_var('avatar_select', ''));
				$category = basename(request_var('category', ''));

				if ($config['allow_avatar_local'] && $display_gallery)
				{
					avatar_gallery($category, $avatar_select, 4);
				}

				$template->assign_vars(array(
					'S_AVATAR'			=> true,
					'S_CAN_UPLOAD'		=> ($can_upload && $config['allow_avatar_upload']) ? true : false,
					'S_ALLOW_REMOTE'	=> ($config['allow_avatar_remote']) ? true : false,
					'S_DISPLAY_GALLERY'	=> ($config['allow_avatar_local'] && !$display_gallery) ? true : false,
					'S_IN_GALLERY'		=> ($config['allow_avatar_local'] && $display_gallery) ? true : false,

					'AVATAR_IMAGE'			=> $avatar_img,
					'AVATAR_MAX_FILESIZE'	=> $config['avatar_filesize'],
					'USER_AVATAR_WIDTH'		=> $user_row['user_avatar_width'],
					'USER_AVATAR_HEIGHT'	=> $user_row['user_avatar_height'],

					'L_AVATAR_EXPLAIN'	=> sprintf($user->lang['AVATAR_EXPLAIN'], $config['avatar_max_width'], $config['avatar_max_height'], round($config['avatar_filesize'] / 1024)))
				);

			break;

			case 'rank':

				if ($submit)
				{
					if (!check_form_key($form_name))
					{
						trigger_error($user->lang['FORM_INVALID'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
					}

					$rank_id = request_var('user_rank', 0);

					$sql = 'UPDATE ' . USERS_TABLE . "
						SET user_rank = $rank_id
						WHERE user_id = $user_id";
					$db->sql_query($sql);

					trigger_error($user->lang['USER_RANK_UPDATED'] . adm_back_link($this->u_action . '&amp;u=' . $user_id));
				}

				$sql = 'SELECT *
					FROM ' . RANKS_TABLE . '
					WHERE rank_special = 1
					ORDER BY rank_title';
				$result = $db->sql_query($sql);

				$s_rank_options = '<option value="0"' . ((!$user_row['user_rank']) ? ' selected="selected"' : '') . '>' . $user->lang['NO_SPECIAL_RANK'] . '</option>';

				while ($row = $db->sql_fetchrow($result))
				{
					$selected = ($user_row['user_rank'] && $row['rank_id'] == $user_row['user_rank']) ? ' selected="selected"' : '';
					$s_rank_options .= '<option value="' . $row['rank_id'] . '"' . $selected . '>' . $row['rank_title'] . '</option>';
				}
				$db->sql_freeresult($result);

				$template->assign_vars(array(
					'S_RANK'			=> true,
					'S_RANK_OPTIONS'	=> $s_rank_options)
				);

			break;

			case 'sig':

				include_once($phpbb_root_path . 'includes/functions_posting.' . $phpEx);
				include_once($phpbb_root_path . 'includes/functions_display.' . $phpEx);

				$enable_bbcode	= ($config['allow_sig_bbcode']) ? ((request_var('disable_bbcode', !$user->optionget('bbcode'))) ? false : true) : false;
				$enable_smilies	= ($config['allow_sig_smilies']) ? ((request_var('disable_smilies', !$user->optionget('smilies'))) ? false : true) : false;
				$enable_urls	= ($config['allow_sig_links']) ? ((request_var('disable_magic_url', false)) ? false : true) : false;
				$signature		= utf8_normalize_nfc(request_var('signature', (string) $user_row['user_sig'], true));

				$preview		= (isset($_POST['preview'])) ? true : false;

				if ($submit || $preview)
				{
					include_once($phpbb_root_path . 'includes/message_parser.' . $phpEx);

					$message_parser = new parse_message($signature);

					// Allowing Quote BBCode
					$message_parser->parse($enable_bbcode, $enable_urls, $enable_smilies, $config['allow_sig_img'], $config['allow_sig_flash'], true, $config['allow_sig_links'], true, 'sig');

					if (sizeof($message_parser->warn_msg))
					{
						$error[] = implode('<br />', $message_parser->warn_msg);
					}

					if (!check_form_key($form_name))
					{
						$error = 'FORM_INVALID';
					}

					if (!sizeof($error) && $submit)
					{
						$sql_ary = array(
							'user_sig'					=> (string) $message_parser->message,
							'user_sig_bbcode_uid'		=> (string) $message_parser->bbcode_uid,
							'user_sig_bbcode_bitfield'	=> (string) $message_parser->bbcode_bitfield
						);

						$sql = 'UPDATE ' . USERS_TABLE . '
							SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
							WHERE user_id = ' . $user_id;
						$db->sql_query($sql);

						trigger_error($user->lang['USER_SIG_UPDATED'] . adm_back_link($this->u_action . '&amp;u=' . $user_id));
					}

					// Replace "error" strings with their real, localised form
					$error = preg_replace('#^([A-Z_]+)$#e', "(!empty(\$user->lang['\\1'])) ? \$user->lang['\\1'] : '\\1'", $error);
				}

				$signature_preview = '';

				if ($preview)
				{
					// Now parse it for displaying
					$signature_preview = $message_parser->format_display($enable_bbcode, $enable_urls, $enable_smilies, false);
					unset($message_parser);
				}

				decode_message($signature, $user_row['user_sig_bbcode_uid']);

				$template->assign_vars(array(
					'S_SIGNATURE'		=> true,

					'SIGNATURE'			=> $signature,
					'SIGNATURE_PREVIEW'	=> $signature_preview,

					'S_BBCODE_CHECKED'		=> (!$enable_bbcode) ? ' checked="checked"' : '',
					'S_SMILIES_CHECKED'		=> (!$enable_smilies) ? ' checked="checked"' : '',
					'S_MAGIC_URL_CHECKED'	=> (!$enable_urls) ? ' checked="checked"' : '',

					'BBCODE_STATUS'			=> ($config['allow_sig_bbcode']) ? sprintf($user->lang['BBCODE_IS_ON'], '<a href="' . append_sid("{$phpbb_root_path}faq.$phpEx", 'mode=bbcode') . '">', '</a>') : sprintf($user->lang['BBCODE_IS_OFF'], '<a href="' . append_sid("{$phpbb_root_path}faq.$phpEx", 'mode=bbcode') . '">', '</a>'),
					'SMILIES_STATUS'		=> ($config['allow_sig_smilies']) ? $user->lang['SMILIES_ARE_ON'] : $user->lang['SMILIES_ARE_OFF'],
					'IMG_STATUS'			=> ($config['allow_sig_img']) ? $user->lang['IMAGES_ARE_ON'] : $user->lang['IMAGES_ARE_OFF'],
					'FLASH_STATUS'			=> ($config['allow_sig_flash']) ? $user->lang['FLASH_IS_ON'] : $user->lang['FLASH_IS_OFF'],
					'URL_STATUS'			=> ($config['allow_sig_links']) ? $user->lang['URL_IS_ON'] : $user->lang['URL_IS_OFF'],

					'L_SIGNATURE_EXPLAIN'	=> sprintf($user->lang['SIGNATURE_EXPLAIN'], $config['max_sig_chars']),

					'S_BBCODE_ALLOWED'		=> $config['allow_sig_bbcode'],
					'S_SMILIES_ALLOWED'		=> $config['allow_sig_smilies'],
					'S_BBCODE_IMG'			=> ($config['allow_sig_img']) ? true : false,
					'S_BBCODE_FLASH'		=> ($config['allow_sig_flash']) ? true : false,
					'S_LINKS_ALLOWED'		=> ($config['allow_sig_links']) ? true : false)
				);

				// Assigning custom bbcodes
				display_custom_bbcodes();

			break;

			case 'attach':

				$start		= request_var('start', 0);
				$deletemark = (isset($_POST['delmarked'])) ? true : false;
				$marked		= request_var('mark', array(0));

				// Sort keys
				$sort_key	= request_var('sk', 'a');
				$sort_dir	= request_var('sd', 'd');

				if ($deletemark && sizeof($marked))
				{
					$sql = 'SELECT attach_id
						FROM ' . ATTACHMENTS_TABLE . '
						WHERE poster_id = ' . $user_id . '
							AND is_orphan = 0
							AND ' . $db->sql_in_set('attach_id', $marked);
					$result = $db->sql_query($sql);

					$marked = array();
					while ($row = $db->sql_fetchrow($result))
					{
						$marked[] = $row['attach_id'];
					}
					$db->sql_freeresult($result);
				}

				if ($deletemark && sizeof($marked))
				{
					if (confirm_box(true))
					{
						$sql = 'SELECT real_filename
							FROM ' . ATTACHMENTS_TABLE . '
							WHERE ' . $db->sql_in_set('attach_id', $marked);
						$result = $db->sql_query($sql);

						$log_attachments = array();
						while ($row = $db->sql_fetchrow($result))
						{
							$log_attachments[] = $row['real_filename'];
						}
						$db->sql_freeresult($result);

						delete_attachments('attach', $marked);

						$message = (sizeof($log_attachments) == 1) ? $user->lang['ATTACHMENT_DELETED'] : $user->lang['ATTACHMENTS_DELETED'];

						add_log('admin', 'LOG_ATTACHMENTS_DELETED', implode(', ', $log_attachments));
						trigger_error($message . adm_back_link($this->u_action . '&amp;u=' . $user_id));
					}
					else
					{
						confirm_box(false, $user->lang['CONFIRM_OPERATION'], build_hidden_fields(array(
							'u'				=> $user_id,
							'i'				=> $id,
							'mode'			=> $mode,
							'action'		=> $action,
							'delmarked'		=> true,
							'mark'			=> $marked))
						);
					}
				}

				$sk_text = array('a' => $user->lang['SORT_FILENAME'], 'c' => $user->lang['SORT_EXTENSION'], 'd' => $user->lang['SORT_SIZE'], 'e' => $user->lang['SORT_DOWNLOADS'], 'f' => $user->lang['SORT_POST_TIME'], 'g' => $user->lang['SORT_TOPIC_TITLE']);
				$sk_sql = array('a' => 'a.real_filename', 'c' => 'a.extension', 'd' => 'a.filesize', 'e' => 'a.download_count', 'f' => 'a.filetime', 'g' => 't.topic_title');

				$sd_text = array('a' => $user->lang['ASCENDING'], 'd' => $user->lang['DESCENDING']);

				$s_sort_key = '';
				foreach ($sk_text as $key => $value)
				{
					$selected = ($sort_key == $key) ? ' selected="selected"' : '';
					$s_sort_key .= '<option value="' . $key . '"' . $selected . '>' . $value . '</option>';
				}

				$s_sort_dir = '';
				foreach ($sd_text as $key => $value)
				{
					$selected = ($sort_dir == $key) ? ' selected="selected"' : '';
					$s_sort_dir .= '<option value="' . $key . '"' . $selected . '>' . $value . '</option>';
				}

				if (!isset($sk_sql[$sort_key]))
				{
					$sort_key = 'a';
				}

				$order_by = $sk_sql[$sort_key] . ' ' . (($sort_dir == 'a') ? 'ASC' : 'DESC');

				$sql = 'SELECT COUNT(attach_id) as num_attachments
					FROM ' . ATTACHMENTS_TABLE . "
					WHERE poster_id = $user_id
						AND is_orphan = 0";
				$result = $db->sql_query_limit($sql, 1);
				$num_attachments = (int) $db->sql_fetchfield('num_attachments');
				$db->sql_freeresult($result);

				$sql = 'SELECT a.*, t.topic_title, p.message_subject as message_title
					FROM ' . ATTACHMENTS_TABLE . ' a
						LEFT JOIN ' . TOPICS_TABLE . ' t ON (a.topic_id = t.topic_id
							AND a.in_message = 0)
						LEFT JOIN ' . PRIVMSGS_TABLE . ' p ON (a.post_msg_id = p.msg_id
							AND a.in_message = 1)
					WHERE a.poster_id = ' . $user_id . "
						AND a.is_orphan = 0
					ORDER BY $order_by";
				$result = $db->sql_query_limit($sql, $config['posts_per_page'], $start);

				while ($row = $db->sql_fetchrow($result))
				{
					if ($row['in_message'])
					{
						$view_topic = append_sid("{$phpbb_root_path}ucp.$phpEx", "i=pm&amp;p={$row['post_msg_id']}");
					}
					else
					{
						$view_topic = append_sid("{$phpbb_root_path}viewtopic.$phpEx", "t={$row['topic_id']}&amp;p={$row['post_msg_id']}") . '#p' . $row['post_msg_id'];
					}

					$template->assign_block_vars('attach', array(
						'REAL_FILENAME'		=> $row['real_filename'],
						'COMMENT'			=> nl2br($row['attach_comment']),
						'EXTENSION'			=> $row['extension'],
						'SIZE'				=> get_formatted_filesize($row['filesize']),
						'DOWNLOAD_COUNT'	=> $row['download_count'],
						'POST_TIME'			=> $user->format_date($row['filetime']),
						'TOPIC_TITLE'		=> ($row['in_message']) ? $row['message_title'] : $row['topic_title'],

						'ATTACH_ID'			=> $row['attach_id'],
						'POST_ID'			=> $row['post_msg_id'],
						'TOPIC_ID'			=> $row['topic_id'],

						'S_IN_MESSAGE'		=> $row['in_message'],

						'U_DOWNLOAD'		=> append_sid("{$phpbb_root_path}download/file.$phpEx", 'mode=view&amp;id=' . $row['attach_id']),
						'U_VIEW_TOPIC'		=> $view_topic)
					);
				}
				$db->sql_freeresult($result);

				$template->assign_vars(array(
					'S_ATTACHMENTS'		=> true,
					'S_ON_PAGE'			=> on_page($num_attachments, $config['topics_per_page'], $start),
					'S_SORT_KEY'		=> $s_sort_key,
					'S_SORT_DIR'		=> $s_sort_dir,

					'PAGINATION'		=> generate_pagination($this->u_action . "&amp;u=$user_id&amp;sk=$sort_key&amp;sd=$sort_dir", $num_attachments, $config['topics_per_page'], $start, true))
				);

			break;

			case 'groups':

				include($phpbb_root_path . 'includes/functions_user.' . $phpEx);

				$user->add_lang(array('groups', 'acp/groups'));
				$group_id = request_var('g', 0);

				if ($group_id)
				{
					// Check the founder only entry for this group to make sure everything is well
					$sql = 'SELECT group_founder_manage
						FROM ' . GROUPS_TABLE . '
						WHERE group_id = ' . $group_id;
					$result = $db->sql_query($sql);
					$founder_manage = (int) $db->sql_fetchfield('group_founder_manage');
					$db->sql_freeresult($result);

					if ($user->data['user_type'] != USER_FOUNDER && $founder_manage)
					{
						trigger_error($user->lang['NOT_ALLOWED_MANAGE_GROUP'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
					}
				}
				else
				{
					$founder_manage = 0;
				}

				switch ($action)
				{
					case 'demote':
					case 'promote':
					case 'default':
						if (!$group_id)
						{
							trigger_error($user->lang['NO_GROUP'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
						}
						group_user_attributes($action, $group_id, $user_id);

						if ($action == 'default')
						{
							$user_row['group_id'] = $group_id;
						}
					break;

					case 'delete':

						if (confirm_box(true))
						{
							if (!$group_id)
							{
								trigger_error($user->lang['NO_GROUP'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							if ($error = group_user_del($group_id, $user_id))
							{
								trigger_error($user->lang[$error] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
							}

							$error = array();
						}
						else
						{
							confirm_box(false, $user->lang['CONFIRM_OPERATION'], build_hidden_fields(array(
								'u'				=> $user_id,
								'i'				=> $id,
								'mode'			=> $mode,
								'action'		=> $action,
								'g'				=> $group_id))
							);
						}

					break;
				}

				// Add user to group?
				if ($submit)
				{

					if (!check_form_key($form_name))
					{
						trigger_error($user->lang['FORM_INVALID'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
					}

					if (!$group_id)
					{
						trigger_error($user->lang['NO_GROUP'] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
					}

					// Add user/s to group
					if ($error = group_user_add($group_id, $user_id))
					{
						trigger_error($user->lang[$error] . adm_back_link($this->u_action . '&amp;u=' . $user_id), E_USER_WARNING);
					}

					$error = array();
				}


				$sql = 'SELECT ug.*, g.*
					FROM ' . GROUPS_TABLE . ' g, ' . USER_GROUP_TABLE . " ug
					WHERE ug.user_id = $user_id
						AND g.group_id = ug.group_id
					ORDER BY g.group_type DESC, ug.user_pending ASC, g.group_name";
				$result = $db->sql_query($sql);

				$i = 0;
				$group_data = $id_ary = array();
				while ($row = $db->sql_fetchrow($result))
				{
					$type = ($row['group_type'] == GROUP_SPECIAL) ? 'special' : (($row['user_pending']) ? 'pending' : 'normal');

					$group_data[$type][$i]['group_id']		= $row['group_id'];
					$group_data[$type][$i]['group_name']	= $row['group_name'];
					$group_data[$type][$i]['group_leader']	= ($row['group_leader']) ? 1 : 0;

					$id_ary[] = $row['group_id'];

					$i++;
				}
				$db->sql_freeresult($result);

				// Select box for other groups
				$sql = 'SELECT group_id, group_name, group_type, group_founder_manage
					FROM ' . GROUPS_TABLE . '
					' . ((sizeof($id_ary)) ? 'WHERE ' . $db->sql_in_set('group_id', $id_ary, true) : '') . '
					ORDER BY group_type DESC, group_name ASC';
				$result = $db->sql_query($sql);

				$s_group_options = '';
				while ($row = $db->sql_fetchrow($result))
				{
					if (!$config['coppa_enable'] && $row['group_name'] == 'REGISTERED_COPPA')
					{
						continue;
					}

					// Do not display those groups not allowed to be managed
					if ($user->data['user_type'] != USER_FOUNDER && $row['group_founder_manage'])
					{
						continue;
					}

					$s_group_options .= '<option' . (($row['group_type'] == GROUP_SPECIAL) ? ' class="sep"' : '') . ' value="' . $row['group_id'] . '">' . (($row['group_type'] == GROUP_SPECIAL) ? $user->lang['G_' . $row['group_name']] : $row['group_name']) . '</option>';
				}
				$db->sql_freeresult($result);

				$current_type = '';
				foreach ($group_data as $group_type => $data_ary)
				{
					if ($current_type != $group_type)
					{
						$template->assign_block_vars('group', array(
							'S_NEW_GROUP_TYPE'		=> true,
							'GROUP_TYPE'			=> $user->lang['USER_GROUP_' . strtoupper($group_type)])
						);
					}

					foreach ($data_ary as $data)
					{
						$template->assign_block_vars('group', array(
							'U_EDIT_GROUP'		=> append_sid("{$phpbb_admin_path}index.$phpEx", "i=groups&amp;mode=manage&amp;action=edit&amp;u=$user_id&amp;g={$data['group_id']}&amp;back_link=acp_users_groups"),
							'U_DEFAULT'			=> $this->u_action . "&amp;action=default&amp;u=$user_id&amp;g=" . $data['group_id'],
							'U_DEMOTE_PROMOTE'	=> $this->u_action . '&amp;action=' . (($data['group_leader']) ? 'demote' : 'promote') . "&amp;u=$user_id&amp;g=" . $data['group_id'],
							'U_DELETE'			=> $this->u_action . "&amp;action=delete&amp;u=$user_id&amp;g=" . $data['group_id'],

							'GROUP_NAME'		=> ($group_type == 'special') ? $user->lang['G_' . $data['group_name']] : $data['group_name'],
							'L_DEMOTE_PROMOTE'	=> ($data['group_leader']) ? $user->lang['GROUP_DEMOTE'] : $user->lang['GROUP_PROMOTE'],

							'S_NO_DEFAULT'		=> ($user_row['group_id'] != $data['group_id']) ? true : false,
							'S_SPECIAL_GROUP'	=> ($group_type == 'special') ? true : false,
							)
						);
					}
				}

				$template->assign_vars(array(
					'S_GROUPS'			=> true,
					'S_GROUP_OPTIONS'	=> $s_group_options)
				);

			break;

			case 'perm':

				include_once($phpbb_root_path . 'includes/acp/auth.' . $phpEx);

				$auth_admin = new auth_admin();

				$user->add_lang('acp/permissions');
				add_permission_language();

				$forum_id = request_var('f', 0);

				// Global Permissions
				if (!$forum_id)
				{
					// Select auth options
					$sql = 'SELECT auth_option, is_local, is_global
						FROM ' . ACL_OPTIONS_TABLE . '
						WHERE auth_option ' . $db->sql_like_expression($db->any_char . '_') . '
							AND is_global = 1
						ORDER BY auth_option';
					$result = $db->sql_query($sql);

					$hold_ary = array();

					while ($row = $db->sql_fetchrow($result))
					{
						$hold_ary = $auth_admin->get_mask('view', $user_id, false, false, $row['auth_option'], 'global', ACL_NEVER);
						$auth_admin->display_mask('view', $row['auth_option'], $hold_ary, 'user', false, false);
					}
					$db->sql_freeresult($result);

					unset($hold_ary);
				}
				else
				{
					$sql = 'SELECT auth_option, is_local, is_global
						FROM ' . ACL_OPTIONS_TABLE . "
						WHERE auth_option " . $db->sql_like_expression($db->any_char . '_') . "
							AND is_local = 1
						ORDER BY is_global DESC, auth_option";
					$result = $db->sql_query($sql);

					while ($row = $db->sql_fetchrow($result))
					{
						$hold_ary = $auth_admin->get_mask('view', $user_id, false, $forum_id, $row['auth_option'], 'local', ACL_NEVER);
						$auth_admin->display_mask('view', $row['auth_option'], $hold_ary, 'user', true, false);
					}
					$db->sql_freeresult($result);
				}

				$s_forum_options = '<option value="0"' . ((!$forum_id) ? ' selected="selected"' : '') . '>' . $user->lang['VIEW_GLOBAL_PERMS'] . '</option>';
				$s_forum_options .= make_forum_select($forum_id, false, true, false, false, false);

				$template->assign_vars(array(
					'S_PERMISSIONS'				=> true,

					'S_GLOBAL'					=> (!$forum_id) ? true : false,
					'S_FORUM_OPTIONS'			=> $s_forum_options,

					'U_ACTION'					=> $this->u_action . '&amp;u=' . $user_id,
					'U_USER_PERMISSIONS'		=> append_sid("{$phpbb_admin_path}index.$phpEx" ,'i=permissions&amp;mode=setting_user_global&amp;user_id[]=' . $user_id),
					'U_USER_FORUM_PERMISSIONS'	=> append_sid("{$phpbb_admin_path}index.$phpEx", 'i=permissions&amp;mode=setting_user_local&amp;user_id[]=' . $user_id))
				);

			break;

		}

		// Assign general variables
		$template->assign_vars(array(
			'S_ERROR'			=> (sizeof($error)) ? true : false,
			'ERROR_MSG'			=> (sizeof($error)) ? implode('<br />', $error) : '')
		);
	}

	/**
	* Optionset replacement for this module based on $user->optionset
	*/
	function optionset(&$user_row, $key, $value, $data = false)
	{
		global $user;

		$var = ($data) ? $data : $user_row['user_options'];

		if ($value && !($var & 1 << $user->keyoptions[$key]))
		{
			$var += 1 << $user->keyoptions[$key];
		}
		else if (!$value && ($var & 1 << $user->keyoptions[$key]))
		{
			$var -= 1 << $user->keyoptions[$key];
		}
		else
		{
			return ($data) ? $var : false;
		}

		if (!$data)
		{
			$user_row['user_options'] = $var;
			return true;
		}
		else
		{
			return $var;
		}
	}

	/**
	* Optionget replacement for this module based on $user->optionget
	*/
	function optionget(&$user_row, $key, $data = false)
	{
		global $user;

		$var = ($data) ? $data : $user_row['user_options'];
		return ($var & 1 << $user->keyoptions[$key]) ? true : false;
	}
}

?>