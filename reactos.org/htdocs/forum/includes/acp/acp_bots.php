<?php
/**
*
* @package acp
* @version $Id: acp_bots.php 8479 2008-03-29 00:22:48Z naderman $
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
class acp_bots
{
	var $u_action;

	function main($id, $mode)
	{
		global $config, $db, $user, $auth, $template, $cache;
		global $phpbb_root_path, $phpbb_admin_path, $phpEx, $table_prefix;

		$action = request_var('action', '');
		$submit = (isset($_POST['submit'])) ? true : false;
		$mark	= request_var('mark', array(0));
		$bot_id	= request_var('id', 0);

		if (isset($_POST['add']))
		{
			$action = 'add';
		}

		$error = array();

		$user->add_lang('acp/bots');
		$this->tpl_name = 'acp_bots';
		$this->page_title = 'ACP_BOTS';
		$form_key = 'acp_bots';
		add_form_key($form_key);

		if ($submit && !check_form_key($form_key))
		{
			$error[] = $user->lang['FORM_INVALID'];
		}

		// User wants to do something, how inconsiderate of them!
		switch ($action)
		{
			case 'activate':
				if ($bot_id || sizeof($mark))
				{
					$sql_id = ($bot_id) ? " = $bot_id" : ' IN (' . implode(', ', $mark) . ')';

					$sql = 'UPDATE ' . BOTS_TABLE . "
						SET bot_active = 1
						WHERE bot_id $sql_id";
					$db->sql_query($sql);
				}

				$cache->destroy('_bots');
			break;

			case 'deactivate':
				if ($bot_id || sizeof($mark))
				{
					$sql_id = ($bot_id) ? " = $bot_id" : ' IN (' . implode(', ', $mark) . ')';

					$sql = 'UPDATE ' . BOTS_TABLE . "
						SET bot_active = 0
						WHERE bot_id $sql_id";
					$db->sql_query($sql);
				}

				$cache->destroy('_bots');
			break;

			case 'delete':
				if ($bot_id || sizeof($mark))
				{
					if (confirm_box(true))
					{
						// We need to delete the relevant user, usergroup and bot entries ...
						$sql_id = ($bot_id) ? " = $bot_id" : ' IN (' . implode(', ', $mark) . ')';

						$sql = 'SELECT bot_name, user_id
							FROM ' . BOTS_TABLE . "
							WHERE bot_id $sql_id";
						$result = $db->sql_query($sql);

						$user_id_ary = $bot_name_ary = array();
						while ($row = $db->sql_fetchrow($result))
						{
							$user_id_ary[] = (int) $row['user_id'];
							$bot_name_ary[] = $row['bot_name'];
						}
						$db->sql_freeresult($result);

						$db->sql_transaction('begin');

						$sql = 'DELETE FROM ' . BOTS_TABLE . "
							WHERE bot_id $sql_id";
						$db->sql_query($sql);

						if (sizeof($user_id_ary))
						{
							$_tables = array(USERS_TABLE, USER_GROUP_TABLE);
							foreach ($_tables as $table)
							{
								$sql = "DELETE FROM $table
									WHERE " . $db->sql_in_set('user_id', $user_id_ary);
								$db->sql_query($sql);
							}
						}

						$db->sql_transaction('commit');

						$cache->destroy('_bots');

						add_log('admin', 'LOG_BOT_DELETE', implode(', ', $bot_name_ary));
						trigger_error($user->lang['BOT_DELETED'] . adm_back_link($this->u_action));
					}
					else
					{
						confirm_box(false, $user->lang['CONFIRM_OPERATION'], build_hidden_fields(array(
							'mark'		=> $mark,
							'id'		=> $bot_id,
							'mode'		=> $mode,
							'action'	=> $action))
						);
					}
				}
			break;

			case 'edit':
			case 'add':
				include_once($phpbb_root_path . 'includes/functions_user.' . $phpEx);

				$bot_row = array(
					'bot_name'		=> utf8_normalize_nfc(request_var('bot_name', '', true)),
					'bot_agent'		=> request_var('bot_agent', ''),
					'bot_ip'		=> request_var('bot_ip', ''),
					'bot_active'	=> request_var('bot_active', true),
					'bot_lang'		=> request_var('bot_lang', $config['default_lang']),
					'bot_style'		=> request_var('bot_style' , $config['default_style']),
				);

				if ($submit)
				{
					if (!$bot_row['bot_agent'] && !$bot_row['bot_ip'])
					{
						$error[] = $user->lang['ERR_BOT_NO_MATCHES'];
					}
			
					if ($bot_row['bot_ip'] && !preg_match('#^[\d\.,:]+$#', $bot_row['bot_ip']))
					{
						if (!$ip_list = gethostbynamel($bot_row['bot_ip']))
						{
							$error[] = $user->lang['ERR_BOT_NO_IP'];
						}
						else
						{
							$bot_row['bot_ip'] = implode(',', $ip_list);
						}
					}
					$bot_row['bot_ip'] = str_replace(' ', '', $bot_row['bot_ip']);

					// Make sure the admin is not adding a bot with an user agent similar to his one
					if ($bot_row['bot_agent'] && substr($user->data['session_browser'], 0, 149) === substr($bot_row['bot_agent'], 0, 149))
					{
						$error[] = $user->lang['ERR_BOT_AGENT_MATCHES_UA'];
					}
					
					$bot_name = false;
					if ($bot_id)
					{
						$sql = 'SELECT u.username_clean
							FROM ' . BOTS_TABLE . ' b, ' . USERS_TABLE . " u
							WHERE b.bot_id = $bot_id
								AND u.user_id = b.user_id";
						$result = $db->sql_query($sql);
						$row = $db->sql_fetchrow($result);
						$db->sql_freeresult($result);

						if (!$bot_row)
						{
							$error[] = $user->lang['NO_BOT'];
						}
						else
						{
							$bot_name = $row['username_clean'];
						}
					}
					if (!$this->validate_botname($bot_row['bot_name'], $bot_name))
					{
						$error[] = $user->lang['BOT_NAME_TAKEN'];
					}
					
					if (!sizeof($error))
					{
						// New bot? Create a new user and group entry
						if ($action == 'add')
						{
							$sql = 'SELECT group_id, group_colour
								FROM ' . GROUPS_TABLE . "
								WHERE group_name = 'BOTS'
									AND group_type = " . GROUP_SPECIAL;
							$result = $db->sql_query($sql);
							$group_row = $db->sql_fetchrow($result);
							$db->sql_freeresult($result);

							if (!$group_row)
							{
								trigger_error($user->lang['NO_BOT_GROUP'] . adm_back_link($this->u_action . "&amp;id=$bot_id&amp;action=$action"), E_USER_WARNING);
							}
						

							$user_id = user_add(array(
								'user_type'				=> (int) USER_IGNORE,
								'group_id'				=> (int) $group_row['group_id'],
								'username'				=> (string) $bot_row['bot_name'],
								'user_regdate'			=> time(),
								'user_password'			=> '',
								'user_colour'			=> (string) $group_row['group_colour'],
								'user_email'			=> '',
								'user_lang'				=> (string) $bot_row['bot_lang'],
								'user_style'			=> (int) $bot_row['bot_style'],
								'user_allow_massemail'	=> 0,
							));
	
							$sql = 'INSERT INTO ' . BOTS_TABLE . ' ' . $db->sql_build_array('INSERT', array(
								'user_id'		=> (int) $user_id,
								'bot_name'		=> (string) $bot_row['bot_name'],
								'bot_active'	=> (int) $bot_row['bot_active'],
								'bot_agent'		=> (string) $bot_row['bot_agent'],
								'bot_ip'		=> (string) $bot_row['bot_ip'])
							);
							$db->sql_query($sql);
	
							$log = 'ADDED';
						}
						else if ($bot_id)
						{
							$sql = 'SELECT user_id, bot_name
								FROM ' . BOTS_TABLE . "
								WHERE bot_id = $bot_id";
							$result = $db->sql_query($sql);
							$row = $db->sql_fetchrow($result);
							$db->sql_freeresult($result);

							if (!$row)
							{
								trigger_error($user->lang['NO_BOT'] . adm_back_link($this->u_action . "&amp;id=$bot_id&amp;action=$action"), E_USER_WARNING);
							}

							$sql_ary = array(
								'user_style'	=> (int) $bot_row['bot_style'],
								'user_lang'		=> (string) $bot_row['bot_lang'],
							);

							if ($bot_row['bot_name'] !== $row['bot_name'])
							{
								$sql_ary['username'] = (string) $bot_row['bot_name'];
								$sql_ary['username_clean'] = (string) utf8_clean_string($bot_row['bot_name']);
							}

							$sql = 'UPDATE ' . USERS_TABLE . ' SET ' . $db->sql_build_array('UPDATE', $sql_ary) . " WHERE user_id = {$row['user_id']}";
							$db->sql_query($sql);

							$sql = 'UPDATE ' . BOTS_TABLE . ' SET ' . $db->sql_build_array('UPDATE', array(
								'bot_name'		=> (string) $bot_row['bot_name'],
								'bot_active'	=> (int) $bot_row['bot_active'],
								'bot_agent'		=> (string) $bot_row['bot_agent'],
								'bot_ip'		=> (string) $bot_row['bot_ip'])
							) . " WHERE bot_id = $bot_id";
							$db->sql_query($sql);

							// Updated username?
							if ($bot_row['bot_name'] !== $row['bot_name'])
							{
								user_update_name($row['bot_name'], $bot_row['bot_name']);
							}

							$log = 'UPDATED';
						}
						
						$cache->destroy('_bots');
						
						add_log('admin', 'LOG_BOT_' . $log, $bot_row['bot_name']);
						trigger_error($user->lang['BOT_' . $log] . adm_back_link($this->u_action));
					
					}
				}
				else if ($bot_id)
				{
					$sql = 'SELECT b.*, u.user_lang, u.user_style
						FROM ' . BOTS_TABLE . ' b, ' . USERS_TABLE . " u
						WHERE b.bot_id = $bot_id
							AND u.user_id = b.user_id";
					$result = $db->sql_query($sql);
					$bot_row = $db->sql_fetchrow($result);
					$db->sql_freeresult($result);

					if (!$bot_row)
					{
						trigger_error($user->lang['NO_BOT'] . adm_back_link($this->u_action . "&amp;id=$bot_id&amp;action=$action"), E_USER_WARNING);
					}

					$bot_row['bot_lang'] = $bot_row['user_lang'];
					$bot_row['bot_style'] = $bot_row['user_style'];
					unset($bot_row['user_lang'], $bot_row['user_style']);
				}

				$s_active_options = '';
				$_options = array('0' => 'NO', '1' => 'YES');
				foreach ($_options as $value => $lang)
				{
					$selected = ($bot_row['bot_active'] == $value) ? ' selected="selected"' : '';
					$s_active_options .= '<option value="' . $value . '"' . $selected . '>' . $user->lang[$lang] . '</option>';
				}

				$style_select = style_select($bot_row['bot_style'], true);
				$lang_select = language_select($bot_row['bot_lang']);

				$l_title = ($action == 'edit') ? 'EDIT' : 'ADD';

				$template->assign_vars(array(
					'L_TITLE'		=> $user->lang['BOT_' . $l_title],
					'U_ACTION'		=> $this->u_action . "&amp;id=$bot_id&amp;action=$action",
					'U_BACK'		=> $this->u_action,
					'ERROR_MSG'		=> (sizeof($error)) ? implode('<br />', $error) : '',
					
					'BOT_NAME'		=> $bot_row['bot_name'],
					'BOT_IP'		=> $bot_row['bot_ip'],
					'BOT_AGENT'		=> $bot_row['bot_agent'],
					
					'S_EDIT_BOT'		=> true,
					'S_ACTIVE_OPTIONS'	=> $s_active_options,
					'S_STYLE_OPTIONS'	=> $style_select,
					'S_LANG_OPTIONS'	=> $lang_select,
					'S_ERROR'			=> (sizeof($error)) ? true : false,
					)
				);

				return;

			break;
		}

		$s_options = '';
		$_options = array('activate' => 'BOT_ACTIVATE', 'deactivate' => 'BOT_DEACTIVATE', 'delete' => 'DELETE');
		foreach ($_options as $value => $lang)
		{
			$s_options .= '<option value="' . $value . '">' . $user->lang[$lang] . '</option>';
		}

		$template->assign_vars(array(
			'U_ACTION'		=> $this->u_action,
			'S_BOT_OPTIONS'	=> $s_options)
		);

		$sql = 'SELECT b.bot_id, b.bot_name, b.bot_active, u.user_lastvisit
			FROM ' . BOTS_TABLE . ' b, ' . USERS_TABLE . ' u
			WHERE u.user_id = b.user_id
			ORDER BY u.user_lastvisit DESC, b.bot_name ASC';
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$active_lang = (!$row['bot_active']) ? 'BOT_ACTIVATE' : 'BOT_DEACTIVATE';
			$active_value = (!$row['bot_active']) ? 'activate' : 'deactivate';

			$template->assign_block_vars('bots', array(
				'BOT_NAME'		=> $row['bot_name'],
				'BOT_ID'		=> $row['bot_id'],
				'LAST_VISIT'	=> ($row['user_lastvisit']) ? $user->format_date($row['user_lastvisit']) : $user->lang['BOT_NEVER'],

				'U_ACTIVATE_DEACTIVATE'	=> $this->u_action . "&amp;id={$row['bot_id']}&amp;action=$active_value",
				'L_ACTIVATE_DEACTIVATE'	=> $user->lang[$active_lang],
				'U_EDIT'				=> $this->u_action . "&amp;id={$row['bot_id']}&amp;action=edit",
				'U_DELETE'				=> $this->u_action . "&amp;id={$row['bot_id']}&amp;action=delete")
			);
		}
		$db->sql_freeresult($result);
	}
	
	/**
	* Validate bot name against username table
	*/
	function validate_botname($newname, $oldname = false)
	{
		global $db;

		if ($oldname && utf8_clean_string($newname) === $oldname)
		{
			return true;
		}

		// Admins might want to use names otherwise forbidden, thus we only check for duplicates.
		$sql = 'SELECT username
			FROM ' . USERS_TABLE . "
			WHERE username_clean = '" . $db->sql_escape(utf8_clean_string($newname)) . "'";
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);
		
		return ($row) ? false : true;
	}
}

?>