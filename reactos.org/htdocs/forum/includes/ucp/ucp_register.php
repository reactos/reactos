<?php
/**
*
* @package ucp
* @version $Id: ucp_register.php 8479 2008-03-29 00:22:48Z naderman $
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
* ucp_register
* Board registration
* @package ucp
*/
class ucp_register
{
	var $u_action;

	function main($id, $mode)
	{
		global $config, $db, $user, $auth, $template, $phpbb_root_path, $phpEx;

		//
		if ($config['require_activation'] == USER_ACTIVATION_DISABLE)
		{
			trigger_error('UCP_REGISTER_DISABLE');
		}

		include($phpbb_root_path . 'includes/functions_profile_fields.' . $phpEx);

		$confirm_id		= request_var('confirm_id', '');
		$coppa			= (isset($_REQUEST['coppa'])) ? ((!empty($_REQUEST['coppa'])) ? 1 : 0) : false;
		$agreed			= (!empty($_POST['agreed'])) ? 1 : 0;
		$submit			= (isset($_POST['submit'])) ? true : false;
		$change_lang	= request_var('change_lang', '');
		$user_lang		= request_var('lang', $user->lang_name);
		
		if ($agreed)
		{
			add_form_key('ucp_register');
		}
		else
		{
			add_form_key('ucp_register_terms');
		}


		if ($change_lang || $user_lang != $config['default_lang'])
		{
			$use_lang = ($change_lang) ? basename($change_lang) : basename($user_lang);

			if (file_exists($phpbb_root_path . 'language/' . $use_lang . '/'))
			{
				if ($change_lang)
				{
					$submit = false;

					// Setting back agreed to let the user view the agreement in his/her language
					$agreed = (empty($_GET['change_lang'])) ? 0 : $agreed;
				}

				$user->lang_name = $lang = $use_lang;
				$user->lang_path = $phpbb_root_path . 'language/' . $lang . '/';
				$user->lang = array();
				$user->add_lang(array('common', 'ucp'));
			}
			else
			{
				$change_lang = '';
				$user_lang = $user->lang_name;
			}
		}

		$cp = new custom_profile();

		$error = $cp_data = $cp_error = array();


		if (!$agreed || ($coppa === false && $config['coppa_enable']) || ($coppa && !$config['coppa_enable']))
		{
			$add_lang = ($change_lang) ? '&amp;change_lang=' . urlencode($change_lang) : '';
			$add_coppa = ($coppa !== false) ? '&amp;coppa=' . $coppa : '';

			$s_hidden_fields = ($confirm_id) ? array('confirm_id' => $confirm_id) : array();

			// If we change the language, we want to pass on some more possible parameter.
			if ($change_lang)
			{
				// We do not include the password
				$s_hidden_fields = array_merge($s_hidden_fields, array(
					'username'			=> utf8_normalize_nfc(request_var('username', '', true)),
					'email'				=> strtolower(request_var('email', '')),
					'email_confirm'		=> strtolower(request_var('email_confirm', '')),
					'confirm_code'		=> request_var('confirm_code', ''),
					'confirm_id'		=> request_var('confirm_id', ''),
					'lang'				=> $user->lang_name,
					'tz'				=> request_var('tz', (float) $config['board_timezone']),
				));
			}

			if ($coppa === false && $config['coppa_enable'])
			{
				$now = getdate();
				$coppa_birthday = $user->format_date(mktime($now['hours'] + $user->data['user_dst'], $now['minutes'], $now['seconds'], $now['mon'], $now['mday'] - 1, $now['year'] - 13), $user->lang['DATE_FORMAT']);
				unset($now);

				$template->assign_vars(array(
					'L_COPPA_NO'		=> sprintf($user->lang['UCP_COPPA_BEFORE'], $coppa_birthday),
					'L_COPPA_YES'		=> sprintf($user->lang['UCP_COPPA_ON_AFTER'], $coppa_birthday),

					'U_COPPA_NO'		=> append_sid("{$phpbb_root_path}ucp.$phpEx", 'mode=register&amp;coppa=0' . $add_lang),
					'U_COPPA_YES'		=> append_sid("{$phpbb_root_path}ucp.$phpEx", 'mode=register&amp;coppa=1' . $add_lang),

					'S_SHOW_COPPA'		=> true,
					'S_HIDDEN_FIELDS'	=> build_hidden_fields($s_hidden_fields),
					'S_UCP_ACTION'		=> append_sid("{$phpbb_root_path}ucp.$phpEx", 'mode=register' . $add_lang),
				));
			}
			else
			{
				$template->assign_vars(array(
					'L_TERMS_OF_USE'	=> sprintf($user->lang['TERMS_OF_USE_CONTENT'], $config['sitename'], generate_board_url()),

					'S_SHOW_COPPA'		=> false,
					'S_REGISTRATION'	=> true,
					'S_HIDDEN_FIELDS'	=> build_hidden_fields($s_hidden_fields),
					'S_UCP_ACTION'		=> append_sid("{$phpbb_root_path}ucp.$phpEx", 'mode=register' . $add_lang . $add_coppa),
					)
				);
			}

			$this->tpl_name = 'ucp_agreement';
			return;
		}


		// Try to manually determine the timezone and adjust the dst if the server date/time complies with the default setting +/- 1
		$timezone = date('Z') / 3600;
		$is_dst = date('I');

		if ($config['board_timezone'] == $timezone || $config['board_timezone'] == ($timezone - 1))
		{
			$timezone = ($is_dst) ? $timezone - 1 : $timezone;

			if (!isset($user->lang['tz_zones'][(string) $timezone]))
			{
				$timezone = $config['board_timezone'];
			}
		}
		else
		{
			$is_dst = $config['board_dst'];
			$timezone = $config['board_timezone'];
		}

		$data = array(
			'username'			=> utf8_normalize_nfc(request_var('username', '', true)),
			'new_password'		=> request_var('new_password', '', true),
			'password_confirm'	=> request_var('password_confirm', '', true),
			'email'				=> strtolower(request_var('email', '')),
			'email_confirm'		=> strtolower(request_var('email_confirm', '')),
			'confirm_code'		=> request_var('confirm_code', ''),
			'lang'				=> basename(request_var('lang', $user->lang_name)),
			'tz'				=> request_var('tz', (float) $timezone),
		);

		// Check and initialize some variables if needed
		if ($submit)
		{
			$error = validate_data($data, array(
				'username'			=> array(
					array('string', false, $config['min_name_chars'], $config['max_name_chars']),
					array('username', '')),
				'new_password'		=> array(
					array('string', false, $config['min_pass_chars'], $config['max_pass_chars']),
					array('password')),
				'password_confirm'	=> array('string', false, $config['min_pass_chars'], $config['max_pass_chars']),
				'email'				=> array(
					array('string', false, 6, 60),
					array('email')),
				'email_confirm'		=> array('string', false, 6, 60),
				'confirm_code'		=> array('string', !$config['enable_confirm'], 5, 8),
				'tz'				=> array('num', false, -14, 14),
				'lang'				=> array('match', false, '#^[a-z_\-]{2,}$#i'),
			));
			if (!check_form_key('ucp_register'))
			{
				$error[] = $user->lang['FORM_INVALID'];
			}
			// Replace "error" strings with their real, localised form
			$error = preg_replace('#^([A-Z_]+)$#e', "(!empty(\$user->lang['\\1'])) ? \$user->lang['\\1'] : '\\1'", $error);

			// DNSBL check
			if ($config['check_dnsbl'])
			{
				if (($dnsbl = $user->check_dnsbl('register')) !== false)
				{
					$error[] = sprintf($user->lang['IP_BLACKLISTED'], $user->ip, $dnsbl[1]);
				}
			}

			// validate custom profile fields
			$cp->submit_cp_field('register', $user->get_iso_lang_id(), $cp_data, $error);

			// Visual Confirmation handling
			$wrong_confirm = false;
			if ($config['enable_confirm'])
			{
				if (!$confirm_id)
				{
					$error[] = $user->lang['CONFIRM_CODE_WRONG'];
					$wrong_confirm = true;
				}
				else
				{
					$sql = 'SELECT code
						FROM ' . CONFIRM_TABLE . "
						WHERE confirm_id = '" . $db->sql_escape($confirm_id) . "'
							AND session_id = '" . $db->sql_escape($user->session_id) . "'
							AND confirm_type = " . CONFIRM_REG;
					$result = $db->sql_query($sql);
					$row = $db->sql_fetchrow($result);
					$db->sql_freeresult($result);

					if ($row)
					{
						if (strcasecmp($row['code'], $data['confirm_code']) === 0)
						{
							$sql = 'DELETE FROM ' . CONFIRM_TABLE . "
								WHERE confirm_id = '" . $db->sql_escape($confirm_id) . "'
									AND session_id = '" . $db->sql_escape($user->session_id) . "'
									AND confirm_type = " . CONFIRM_REG;
							$db->sql_query($sql);
						}
						else
						{
							$error[] = $user->lang['CONFIRM_CODE_WRONG'];
							$wrong_confirm = true;
						}
					}
					else
					{
						$error[] = $user->lang['CONFIRM_CODE_WRONG'];
						$wrong_confirm = true;
					}
				}
			}

			if (!sizeof($error))
			{
				if ($data['new_password'] != $data['password_confirm'])
				{
					$error[] = $user->lang['NEW_PASSWORD_ERROR'];
				}

				if ($data['email'] != $data['email_confirm'])
				{
					$error[] = $user->lang['NEW_EMAIL_ERROR'];
				}
			}

			if (!sizeof($error))
			{
				$server_url = generate_board_url();

				// Which group by default?
				$group_name = ($coppa) ? 'REGISTERED_COPPA' : 'REGISTERED';

				$sql = 'SELECT group_id
					FROM ' . GROUPS_TABLE . "
					WHERE group_name = '" . $db->sql_escape($group_name) . "'
						AND group_type = " . GROUP_SPECIAL;
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if (!$row)
				{
					trigger_error('NO_GROUP');
				}

				$group_id = $row['group_id'];

				if (($coppa ||
					$config['require_activation'] == USER_ACTIVATION_SELF ||
					$config['require_activation'] == USER_ACTIVATION_ADMIN) && $config['email_enable'])
				{
					$user_actkey = gen_rand_string(10);
					$key_len = 54 - (strlen($server_url));
					$key_len = ($key_len < 6) ? 6 : $key_len;
					$user_actkey = substr($user_actkey, 0, $key_len);

					$user_type = USER_INACTIVE;
					$user_inactive_reason = INACTIVE_REGISTER;
					$user_inactive_time = time();
				}
				else
				{
					$user_type = USER_NORMAL;
					$user_actkey = '';
					$user_inactive_reason = 0;
					$user_inactive_time = 0;
				}

				$user_row = array(
					'username'				=> $data['username'],
					'user_password'			=> phpbb_hash($data['new_password']),
					'user_email'			=> $data['email'],
					'group_id'				=> (int) $group_id,
					'user_timezone'			=> (float) $data['tz'],
					'user_dst'				=> $is_dst,
					'user_lang'				=> $data['lang'],
					'user_type'				=> $user_type,
					'user_actkey'			=> $user_actkey,
					'user_ip'				=> $user->ip,
					'user_regdate'			=> time(),
					'user_inactive_reason'	=> $user_inactive_reason,
					'user_inactive_time'	=> $user_inactive_time,
				);

				// Register user...
				$user_id = user_add($user_row, $cp_data);

				// This should not happen, because the required variables are listed above...
				if ($user_id === false)
				{
					trigger_error('NO_USER', E_USER_ERROR);
				}

				if ($coppa && $config['email_enable'])
				{
					$message = $user->lang['ACCOUNT_COPPA'];
					$email_template = 'coppa_welcome_inactive';
				}
				else if ($config['require_activation'] == USER_ACTIVATION_SELF && $config['email_enable'])
				{
					$message = $user->lang['ACCOUNT_INACTIVE'];
					$email_template = 'user_welcome_inactive';
				}
				else if ($config['require_activation'] == USER_ACTIVATION_ADMIN && $config['email_enable'])
				{
					$message = $user->lang['ACCOUNT_INACTIVE_ADMIN'];
					$email_template = 'admin_welcome_inactive';
				}
				else
				{
					$message = $user->lang['ACCOUNT_ADDED'];
					$email_template = 'user_welcome';
				}

				if ($config['email_enable'])
				{
					include_once($phpbb_root_path . 'includes/functions_messenger.' . $phpEx);

					$messenger = new messenger(false);

					$messenger->template($email_template, $data['lang']);

					$messenger->to($data['email'], $data['username']);

					$messenger->headers('X-AntiAbuse: Board servername - ' . $config['server_name']);
					$messenger->headers('X-AntiAbuse: User_id - ' . $user->data['user_id']);
					$messenger->headers('X-AntiAbuse: Username - ' . $user->data['username']);
					$messenger->headers('X-AntiAbuse: User IP - ' . $user->ip);

					$messenger->assign_vars(array(
						'WELCOME_MSG'	=> htmlspecialchars_decode(sprintf($user->lang['WELCOME_SUBJECT'], $config['sitename'])),
						'USERNAME'		=> htmlspecialchars_decode($data['username']),
						'PASSWORD'		=> htmlspecialchars_decode($data['new_password']),
						'U_ACTIVATE'	=> "$server_url/ucp.$phpEx?mode=activate&u=$user_id&k=$user_actkey")
					);

					if ($coppa)
					{
						$messenger->assign_vars(array(
							'FAX_INFO'		=> $config['coppa_fax'],
							'MAIL_INFO'		=> $config['coppa_mail'],
							'EMAIL_ADDRESS'	=> $data['email'])
						);
					}

					$messenger->send(NOTIFY_EMAIL);

					if ($config['require_activation'] == USER_ACTIVATION_ADMIN)
					{
						// Grab an array of user_id's with a_user permissions ... these users can activate a user
						$admin_ary = $auth->acl_get_list(false, 'a_user', false);
						$admin_ary = (!empty($admin_ary[0]['a_user'])) ? $admin_ary[0]['a_user'] : array();

						// Also include founders
						$where_sql = ' WHERE user_type = ' . USER_FOUNDER;

						if (sizeof($admin_ary))
						{
							$where_sql .= ' OR ' . $db->sql_in_set('user_id', $admin_ary);
						}

						$sql = 'SELECT user_id, username, user_email, user_lang, user_jabber, user_notify_type
							FROM ' . USERS_TABLE . ' ' .
							$where_sql;
						$result = $db->sql_query($sql);

						while ($row = $db->sql_fetchrow($result))
						{
							$messenger->template('admin_activate', $row['user_lang']);
							$messenger->to($row['user_email'], $row['username']);
							$messenger->im($row['user_jabber'], $row['username']);

							$messenger->assign_vars(array(
								'USERNAME'			=> htmlspecialchars_decode($data['username']),
								'U_USER_DETAILS'	=> "$server_url/memberlist.$phpEx?mode=viewprofile&u=$user_id",
								'U_ACTIVATE'		=> "$server_url/ucp.$phpEx?mode=activate&u=$user_id&k=$user_actkey")
							);

							$messenger->send($row['user_notify_type']);
						}
						$db->sql_freeresult($result);
					}
				}

				$message = $message . '<br /><br />' . sprintf($user->lang['RETURN_INDEX'], '<a href="' . append_sid("{$phpbb_root_path}index.$phpEx") . '">', '</a>');
				trigger_error($message);
			}
		}

		$s_hidden_fields = array(
			'agreed'		=> 'true',
			'change_lang'	=> 0,
		);

		if ($config['coppa_enable'])
		{
			$s_hidden_fields['coppa'] = $coppa;
		}
		$s_hidden_fields = build_hidden_fields($s_hidden_fields);

		$confirm_image = '';

		// Visual Confirmation - Show images

		if ($config['enable_confirm'])
		{
			if ($change_lang)
			{
				$str = '&amp;change_lang=' . $change_lang;
				$sql = 'SELECT code
						FROM ' . CONFIRM_TABLE . "
						WHERE confirm_id = '" . $db->sql_escape($confirm_id) . "'
							AND session_id = '" . $db->sql_escape($user->session_id) . "'
							AND confirm_type = " . CONFIRM_REG;
				$result = $db->sql_query($sql);
				if (!$row = $db->sql_fetchrow($result))
				{
					$confirm_id = '';
				}
				$db->sql_freeresult($result);
			}
			else
			{
				$str = '';
			}
			if (!$change_lang || !$confirm_id)
			{
				$user->confirm_gc(CONFIRM_REG);
					
				$sql = 'SELECT COUNT(session_id) AS attempts
					FROM ' . CONFIRM_TABLE . "
					WHERE session_id = '" . $db->sql_escape($user->session_id) . "'
						AND confirm_type = " . CONFIRM_REG;
				$result = $db->sql_query($sql);
				$attempts = (int) $db->sql_fetchfield('attempts');
				$db->sql_freeresult($result);

				if ($config['max_reg_attempts'] && $attempts > $config['max_reg_attempts'])
				{
					trigger_error('TOO_MANY_REGISTERS');
				}

				$code = gen_rand_string(mt_rand(5, 8));
				$confirm_id = md5(unique_id($user->ip));
				$seed = hexdec(substr(unique_id(), 4, 10));

				// compute $seed % 0x7fffffff
				$seed -= 0x7fffffff * floor($seed / 0x7fffffff);

				$sql = 'INSERT INTO ' . CONFIRM_TABLE . ' ' . $db->sql_build_array('INSERT', array(
					'confirm_id'	=> (string) $confirm_id,
					'session_id'	=> (string) $user->session_id,
					'confirm_type'	=> (int) CONFIRM_REG,
					'code'			=> (string) $code,
					'seed'			=> (int) $seed)
				);
				$db->sql_query($sql);
			}
			$confirm_image = '<img src="' . append_sid("{$phpbb_root_path}ucp.$phpEx", 'mode=confirm&amp;id=' . $confirm_id . '&amp;type=' . CONFIRM_REG . $str) . '" alt="" title="" />';
			$s_hidden_fields .= '<input type="hidden" name="confirm_id" value="' . $confirm_id . '" />';
		}

		//
		$l_reg_cond = '';
		switch ($config['require_activation'])
		{
			case USER_ACTIVATION_SELF:
				$l_reg_cond = $user->lang['UCP_EMAIL_ACTIVATE'];
			break;

			case USER_ACTIVATION_ADMIN:
				$l_reg_cond = $user->lang['UCP_ADMIN_ACTIVATE'];
			break;
		}

		$template->assign_vars(array(
			'ERROR'				=> (sizeof($error)) ? implode('<br />', $error) : '',
			'USERNAME'			=> $data['username'],
			'PASSWORD'			=> $data['new_password'],
			'PASSWORD_CONFIRM'	=> $data['password_confirm'],
			'EMAIL'				=> $data['email'],
			'EMAIL_CONFIRM'		=> $data['email_confirm'],
			'CONFIRM_IMG'		=> $confirm_image,

			'L_CONFIRM_EXPLAIN'			=> sprintf($user->lang['CONFIRM_EXPLAIN'], '<a href="mailto:' . htmlspecialchars($config['board_contact']) . '">', '</a>'),
			'L_REG_COND'				=> $l_reg_cond,
			'L_USERNAME_EXPLAIN'		=> sprintf($user->lang[$config['allow_name_chars'] . '_EXPLAIN'], $config['min_name_chars'], $config['max_name_chars']),
			'L_PASSWORD_EXPLAIN'		=> sprintf($user->lang[$config['pass_complex'] . '_EXPLAIN'], $config['min_pass_chars'], $config['max_pass_chars']),

			'S_LANG_OPTIONS'	=> language_select($data['lang']),
			'S_TZ_OPTIONS'		=> tz_select($data['tz']),
			'S_CONFIRM_CODE'	=> ($config['enable_confirm']) ? true : false,
			'S_COPPA'			=> $coppa,
			'S_HIDDEN_FIELDS'	=> $s_hidden_fields,
			'S_UCP_ACTION'		=> append_sid("{$phpbb_root_path}ucp.$phpEx", 'mode=register'),
			)
		);

		//
		$user->profile_fields = array();

		// Generate profile fields -> Template Block Variable profile_fields
		$cp->generate_profile_fields('register', $user->get_iso_lang_id());

		//
		$this->tpl_name = 'ucp_register';
		$this->page_title = 'UCP_REGISTRATION';
	}
}

?>