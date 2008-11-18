<?php
/**
*
* @package ucp
* @version $Id: ucp_pm_options.php 8479 2008-03-29 00:22:48Z naderman $
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
* Execute message options
*/
function message_options($id, $mode, $global_privmsgs_rules, $global_rule_conditions)
{
	global $phpbb_root_path, $phpEx, $user, $template, $auth, $config, $db;

	$redirect_url = append_sid("{$phpbb_root_path}ucp.$phpEx", "i=pm&amp;mode=options");

	add_form_key('ucp_pm_options');
	// Change "full folder" setting - what to do if folder is full
	if (isset($_POST['fullfolder']))
	{
		check_form_key('ucp_pm_options', $config['form_token_lifetime'], $redirect_url);
		$full_action = request_var('full_action', 0);

		$set_folder_id = 0;
		switch ($full_action)
		{
			case 1:
				$set_folder_id = FULL_FOLDER_DELETE;
			break;

			case 2:
				$set_folder_id = request_var('full_move_to', PRIVMSGS_INBOX);
			break;

			case 3:
				$set_folder_id = FULL_FOLDER_HOLD;
			break;

			default:
				$full_action = 0;
			break;
		}

		if ($full_action)
		{
			$sql = 'UPDATE ' . USERS_TABLE . '
				SET user_full_folder = ' . $set_folder_id . '
				WHERE user_id = ' . $user->data['user_id'];
			$db->sql_query($sql);

			$user->data['user_full_folder'] = $set_folder_id;

			$message = $user->lang['FULL_FOLDER_OPTION_CHANGED'] . '<br /><br />' . sprintf($user->lang['RETURN_UCP'], '<a href="' . $redirect_url . '">', '</a>');
			meta_refresh(3, $redirect_url);
			trigger_error($message);
		}
	}
	
	// Add Folder
	if (isset($_POST['addfolder']))
	{
		if (check_form_key('ucp_pm_options'))
		{
			$folder_name = utf8_normalize_nfc(request_var('foldername', '', true));
			$msg = '';

			if ($folder_name)
			{
				$sql = 'SELECT folder_name
					FROM ' . PRIVMSGS_FOLDER_TABLE . "
					WHERE folder_name = '" . $db->sql_escape($folder_name) . "'
						AND user_id = " . $user->data['user_id'];
				$result = $db->sql_query_limit($sql, 1);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if ($row)
				{
					trigger_error(sprintf($user->lang['FOLDER_NAME_EXIST'], $folder_name));
				}

				$sql = 'SELECT COUNT(folder_id) as num_folder
					FROM ' . PRIVMSGS_FOLDER_TABLE . '
						WHERE user_id = ' . $user->data['user_id'];
				$result = $db->sql_query($sql);
				$num_folder = (int) $db->sql_fetchfield('num_folder');
				$db->sql_freeresult($result);

				if ($num_folder >= $config['pm_max_boxes'])
				{
					trigger_error('MAX_FOLDER_REACHED');
				}

				$sql = 'INSERT INTO ' . PRIVMSGS_FOLDER_TABLE . ' ' . $db->sql_build_array('INSERT', array(
					'user_id'		=> (int) $user->data['user_id'],
					'folder_name'	=> $folder_name)
				);
				$db->sql_query($sql);
				$msg = $user->lang['FOLDER_ADDED'];
			}
		}
		else
		{
			$msg = $user->lang['FORM_INVALID'];
		}
		$message = $msg . '<br /><br />' . sprintf($user->lang['RETURN_UCP'], '<a href="' . $redirect_url . '">', '</a>');
		meta_refresh(3, $redirect_url);
		trigger_error($message);
	}

	// Rename folder
	if (isset($_POST['rename_folder']))
	{
		if (check_form_key('ucp_pm_options'))
		{
			$new_folder_name = utf8_normalize_nfc(request_var('new_folder_name', '', true));
			$rename_folder_id= request_var('rename_folder_id', 0);

			if (!$new_folder_name)
			{
				trigger_error('NO_NEW_FOLDER_NAME');
			}

			// Select custom folder
			$sql = 'SELECT folder_name, pm_count
				FROM ' . PRIVMSGS_FOLDER_TABLE . "
				WHERE user_id = {$user->data['user_id']}
					AND folder_id = $rename_folder_id";
			$result = $db->sql_query_limit($sql, 1);
			$folder_row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			if (!$folder_row)
			{
				trigger_error('CANNOT_RENAME_FOLDER');
			}

			$sql = 'UPDATE ' . PRIVMSGS_FOLDER_TABLE . "
				SET folder_name = '" . $db->sql_escape($new_folder_name) . "'
				WHERE folder_id = $rename_folder_id
					AND user_id = {$user->data['user_id']}";
			$db->sql_query($sql);
			$msg = $user->lang['FOLDER_RENAMED'];
		}
		else
		{
			$msg = $user->lang['FORM_INVALID'];
		}

		$message = $msg . '<br /><br />' . sprintf($user->lang['RETURN_UCP'], '<a href="' . $redirect_url . '">', '</a>');

		meta_refresh(3, $redirect_url);
		trigger_error($message);
	}

	// Remove Folder
	if (isset($_POST['remove_folder']))
	{
		$remove_folder_id = request_var('remove_folder_id', 0);

		// Default to "move all messages to inbox"
		$remove_action = request_var('remove_action', 1);
		$move_to = request_var('move_to', PRIVMSGS_INBOX);

		// Move to same folder?
		if ($remove_action == 1 && $remove_folder_id == $move_to)
		{
			trigger_error('CANNOT_MOVE_TO_SAME_FOLDER');
		}

		// Select custom folder
		$sql = 'SELECT folder_name, pm_count
			FROM ' . PRIVMSGS_FOLDER_TABLE . "
			WHERE user_id = {$user->data['user_id']}
				AND folder_id = $remove_folder_id";
		$result = $db->sql_query_limit($sql, 1);
		$folder_row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if (!$folder_row)
		{
			trigger_error('CANNOT_REMOVE_FOLDER');
		}

		$s_hidden_fields = array(
			'remove_folder_id'	=> $remove_folder_id,
			'remove_action'		=> $remove_action,
			'move_to'			=> $move_to,
			'remove_folder'		=> 1
		);

		// Do we need to confirm?
		if (confirm_box(true))
		{
			// Gather message ids
			$sql = 'SELECT msg_id
				FROM ' . PRIVMSGS_TO_TABLE . '
				WHERE user_id = ' . $user->data['user_id'] . "
					AND folder_id = $remove_folder_id";
			$result = $db->sql_query($sql);

			$msg_ids = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$msg_ids[] = (int) $row['msg_id'];
			}
			$db->sql_freeresult($result);

			// First of all, copy all messages to another folder... or delete all messages
			switch ($remove_action)
			{
				// Move Messages
				case 1:
					$num_moved = move_pm($user->data['user_id'], $user->data['message_limit'], $msg_ids, $move_to, $remove_folder_id);
					
					// Something went wrong, only partially moved?
					if ($num_moved != $folder_row['pm_count'])
					{
						trigger_error(sprintf($user->lang['MOVE_PM_ERROR'], $num_moved, $folder_row['pm_count']));
					}
				break;

				// Remove Messages
				case 2:
					delete_pm($user->data['user_id'], $msg_ids, $remove_folder_id);
				break;
			}

			// Remove folder
			$sql = 'DELETE FROM ' . PRIVMSGS_FOLDER_TABLE . "
				WHERE user_id = {$user->data['user_id']}
					AND folder_id = $remove_folder_id";
			$db->sql_query($sql);

			// Check full folder option. If the removed folder has been specified as destination switch back to inbox
			if ($user->data['user_full_folder'] == $remove_folder_id)
			{
				$sql = 'UPDATE ' . USERS_TABLE . '
					SET user_full_folder = ' . PRIVMSGS_INBOX . '
					WHERE user_id = ' . $user->data['user_id'];
				$db->sql_query($sql);

				$user->data['user_full_folder'] = PRIVMSGS_INBOX;
			}

			// Now make sure the folder is not used for rules
			// We assign another folder id (the one the messages got moved to) or assign the INBOX (to not have to remove any rule)
			$sql = 'UPDATE ' . PRIVMSGS_RULES_TABLE . ' SET rule_folder_id = ';
			$sql .= ($remove_action == 1) ? $move_to : PRIVMSGS_INBOX;
			$sql .= ' WHERE rule_folder_id = ' . $remove_folder_id;

			$db->sql_query($sql);

			$meta_info = append_sid("{$phpbb_root_path}ucp.$phpEx", "i=pm&amp;mode=$mode");
			$message = $user->lang['FOLDER_REMOVED'];

			meta_refresh(3, $meta_info);
			$message .= '<br /><br />' . sprintf($user->lang['RETURN_UCP'], '<a href="' . $meta_info . '">', '</a>');
			trigger_error($message);
		}
		else
		{
			confirm_box(false, 'REMOVE_FOLDER', build_hidden_fields($s_hidden_fields));
		}
	}

	// Add Rule
	if (isset($_POST['add_rule']))
	{
		if (check_form_key('ucp_pm_options'))
		{
			$check_option	= request_var('check_option', 0);
			$rule_option	= request_var('rule_option', 0);
			$cond_option	= request_var('cond_option', '');
			$action_option	= explode('|', request_var('action_option', ''));
			$rule_string	= ($cond_option != 'none') ? utf8_normalize_nfc(request_var('rule_string', '', true)) : '';
			$rule_user_id	= ($cond_option != 'none') ? request_var('rule_user_id', 0) : 0;
			$rule_group_id	= ($cond_option != 'none') ? request_var('rule_group_id', 0) : 0;

			$action = (int) $action_option[0];
			$folder_id = (int) $action_option[1];

			if (!$action || !$check_option || !$rule_option || !$cond_option || ($cond_option != 'none' && !$rule_string))
			{
				trigger_error('RULE_NOT_DEFINED');
			}

			if (($cond_option == 'user' && !$rule_user_id) || ($cond_option == 'group' && !$rule_group_id))
			{
				trigger_error('RULE_NOT_DEFINED');
			}

			$rule_ary = array(
				'user_id'			=> $user->data['user_id'],
				'rule_check'		=> $check_option,
				'rule_connection'	=> $rule_option,
				'rule_string'		=> $rule_string,
				'rule_user_id'		=> $rule_user_id,
				'rule_group_id'		=> $rule_group_id,
				'rule_action'		=> $action,
				'rule_folder_id'	=> $folder_id
			);

			$sql = 'SELECT rule_id
				FROM ' . PRIVMSGS_RULES_TABLE . '
				WHERE ' . $db->sql_build_array('SELECT', $rule_ary);
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			if ($row)
			{
				trigger_error('RULE_ALREADY_DEFINED');
			}

			$sql = 'INSERT INTO ' . PRIVMSGS_RULES_TABLE . ' ' . $db->sql_build_array('INSERT', $rule_ary);
			$db->sql_query($sql);

			// Update users message rules
			$sql = 'UPDATE ' . USERS_TABLE . '
				SET user_message_rules = 1
				WHERE user_id = ' . $user->data['user_id'];
			$db->sql_query($sql);

			$msg = $user->lang['RULE_ADDED'];
		}
		else
		{
			$msg = $user->lang['FORM_INVALID'];
		}
		$message = $msg . '<br /><br />' . sprintf($user->lang['RETURN_UCP'], '<a href="' . $redirect_url . '">', '</a>');
		meta_refresh(3, $redirect_url);
		trigger_error($message);
	}

	// Remove Rule
	if (isset($_POST['delete_rule']) && !isset($_POST['cancel']))
	{
		$delete_id = array_keys(request_var('delete_rule', array(0 => 0)));
		$delete_id = (!empty($delete_id[0])) ? $delete_id[0] : 0;

		if (!$delete_id)
		{
			redirect(append_sid("{$phpbb_root_path}ucp.$phpEx", 'i=pm&amp;mode=' . $mode));
		}

		// Do we need to confirm?
		if (confirm_box(true))
		{
			$sql = 'DELETE FROM ' . PRIVMSGS_RULES_TABLE . '
				WHERE user_id = ' . $user->data['user_id'] . "
					AND rule_id = $delete_id";
			$db->sql_query($sql);

			$meta_info = append_sid("{$phpbb_root_path}ucp.$phpEx", 'i=pm&amp;mode=' . $mode);
			$message = $user->lang['RULE_DELETED'];

			// Reset user_message_rules if no more assigned
			$sql = 'SELECT rule_id
				FROM ' . PRIVMSGS_RULES_TABLE . '
				WHERE user_id = ' . $user->data['user_id'];
			$result = $db->sql_query_limit($sql, 1);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			// Update users message rules
			if (!$row)
			{
				$sql = 'UPDATE ' . USERS_TABLE . '
					SET user_message_rules = 0
					WHERE user_id = ' . $user->data['user_id'];
				$db->sql_query($sql);
			}

			meta_refresh(3, $meta_info);
			$message .= '<br /><br />' . sprintf($user->lang['RETURN_UCP'], '<a href="' . $meta_info . '">', '</a>');
			trigger_error($message);
		}
		else
		{
			confirm_box(false, 'DELETE_RULE', build_hidden_fields(array('delete_rule' => array($delete_id => 1))));
		}
	}

	$folder = array();

	$sql = 'SELECT COUNT(msg_id) as num_messages
		FROM ' . PRIVMSGS_TO_TABLE . '
		WHERE user_id = ' . $user->data['user_id'] . '
			AND folder_id = ' . PRIVMSGS_INBOX;
	$result = $db->sql_query($sql);
	$num_messages = (int) $db->sql_fetchfield('num_messages');
	$db->sql_freeresult($result);
	
	$folder[PRIVMSGS_INBOX] = array(
		'folder_name'		=> $user->lang['PM_INBOX'],
		'message_status'	=> sprintf($user->lang['FOLDER_MESSAGE_STATUS'], $num_messages, $user->data['message_limit'])
	);

	$sql = 'SELECT folder_id, folder_name, pm_count
		FROM ' . PRIVMSGS_FOLDER_TABLE . '
			WHERE user_id = ' . $user->data['user_id'];
	$result = $db->sql_query($sql);

	$num_user_folder = 0;
	while ($row = $db->sql_fetchrow($result))
	{
		$num_user_folder++;
		$folder[$row['folder_id']] = array(
			'folder_name'		=> $row['folder_name'],
			'message_status'	=> sprintf($user->lang['FOLDER_MESSAGE_STATUS'], $row['pm_count'], $user->data['message_limit'])
		);
	}
	$db->sql_freeresult($result);

	$s_full_folder_options = $s_to_folder_options = $s_folder_options = '';

	if ($user->data['user_full_folder'] == FULL_FOLDER_NONE)
	{
		// -3 here to let the correct folder id be selected
		$to_folder_id = $config['full_folder_action'] - 3;
	}
	else
	{
		$to_folder_id = $user->data['user_full_folder'];
	}

	foreach ($folder as $folder_id => $folder_ary)
	{
		$s_full_folder_options .= '<option value="' . $folder_id . '"' . (($user->data['user_full_folder'] == $folder_id) ? ' selected="selected"' : '') . '>' . $folder_ary['folder_name'] . ' (' . $folder_ary['message_status'] . ')</option>';
		$s_to_folder_options .= '<option value="' . $folder_id . '"' . (($to_folder_id == $folder_id) ? ' selected="selected"' : '') . '>' . $folder_ary['folder_name'] . ' (' . $folder_ary['message_status'] . ')</option>';

		if ($folder_id != PRIVMSGS_INBOX)
		{
			$s_folder_options .= '<option value="' . $folder_id . '">' . $folder_ary['folder_name'] . ' (' . $folder_ary['message_status'] . ')</option>';
		}
	}

	$s_delete_checked = ($user->data['user_full_folder'] == FULL_FOLDER_DELETE) ? ' checked="checked"' : '';
	$s_hold_checked = ($user->data['user_full_folder'] == FULL_FOLDER_HOLD) ? ' checked="checked"' : '';
	$s_move_checked = ($user->data['user_full_folder'] >= 0) ? ' checked="checked"' : '';

	if ($user->data['user_full_folder'] == FULL_FOLDER_NONE)
	{
		switch ($config['full_folder_action'])
		{
			case 1:
				$s_delete_checked = ' checked="checked"';
			break;

			case 2:
				$s_hold_checked = ' checked="checked"';
			break;
		}
	}

	$template->assign_vars(array(
		'S_FULL_FOLDER_OPTIONS'	=> $s_full_folder_options,
		'S_TO_FOLDER_OPTIONS'	=> $s_to_folder_options,
		'S_FOLDER_OPTIONS'		=> $s_folder_options,
		'S_DELETE_CHECKED'		=> $s_delete_checked,
		'S_HOLD_CHECKED'		=> $s_hold_checked,
		'S_MOVE_CHECKED'		=> $s_move_checked,
		'S_MAX_FOLDER_REACHED'	=> ($num_user_folder >= $config['pm_max_boxes']) ? true : false,
		'S_MAX_FOLDER_ZERO'		=> ($config['pm_max_boxes'] == 0) ? true : false,

		'DEFAULT_ACTION'		=> ($config['full_folder_action'] == 1) ? $user->lang['DELETE_OLDEST_MESSAGES'] : $user->lang['HOLD_NEW_MESSAGES'],

		'U_FIND_USERNAME'		=> append_sid("{$phpbb_root_path}memberlist.$phpEx", 'mode=searchuser&amp;form=ucp&amp;field=rule_string&amp;select_single=true'),
	));

	$rule_lang = $action_lang = $check_lang = array();

	// Build all three language arrays
	preg_replace('#^((RULE|ACTION|CHECK)_([A-Z0-9_]+))$#e', "\${strtolower('\\2') . '_lang'}[constant('\\1')] = \$user->lang['PM_\\2']['\\3']", array_keys(get_defined_constants()));

	/*
		Rule Ordering:
			-> CHECK_* -> RULE_* [IN $global_privmsgs_rules:CHECK_*] -> [IF $rule_conditions[RULE_*] [|text|bool|user|group|own_group]] -> ACTION_*
	*/

	$check_option	= request_var('check_option', 0);
	$rule_option	= request_var('rule_option', 0);
	$cond_option	= request_var('cond_option', '');
	$action_option	= request_var('action_option', '');
	$back = (isset($_REQUEST['back'])) ? request_var('back', array('' => 0)) : array();

	if (sizeof($back))
	{
		if ($action_option)
		{
			$action_option = '';
		}
		else if ($cond_option)
		{
			$cond_option = '';
		}
		else if ($rule_option)
		{
			$rule_option = 0;
		}
		else if ($check_option)
		{
			$check_option = 0;
		}
	}

	if (isset($back['action']) && $cond_option == 'none')
	{
		$back['cond'] = true;
	}

	// Check
	if (!isset($global_privmsgs_rules[$check_option]))
	{
		$check_option = 0;
	}

	define_check_option(($check_option && !isset($back['rule'])) ? true : false, $check_option, $check_lang);

	if ($check_option && !isset($back['rule']))
	{
		define_rule_option(($rule_option && !isset($back['cond'])) ? true : false, $rule_option, $rule_lang, $global_privmsgs_rules[$check_option]);
	}

	if ($rule_option && !isset($back['cond']))
	{
		if (!isset($global_rule_conditions[$rule_option]))
		{
			$cond_option = 'none';
			$template->assign_var('NONE_CONDITION', true);
		}
		else
		{
			define_cond_option(($cond_option && !isset($back['action'])) ? true : false, $cond_option, $rule_option, $global_rule_conditions);
		}
	}

	if ($cond_option && !isset($back['action']))
	{
		define_action_option(false, $action_option, $action_lang, $folder);
	}

	show_defined_rules($user->data['user_id'], $check_lang, $rule_lang, $action_lang, $folder);
}

/**
* Defining check option for message rules
*/
function define_check_option($hardcoded, $check_option, $check_lang)
{
	global $template;

	$s_check_options = '';
	if (!$hardcoded)
	{
		foreach ($check_lang as $value => $lang)
		{
			$s_check_options .= '<option value="' . $value . '"' . (($value == $check_option) ? ' selected="selected"' : '') . '>' . $lang . '</option>';
		}
	}

	$template->assign_vars(array(
		'S_CHECK_DEFINED'	=> true,
		'S_CHECK_SELECT'	=> ($hardcoded) ? false : true,
		'CHECK_CURRENT'		=> isset($check_lang[$check_option]) ? $check_lang[$check_option] : '',
		'S_CHECK_OPTIONS'	=> $s_check_options,
		'CHECK_OPTION'		=> $check_option)
	);
}

/**
* Defining action option for message rules
*/
function define_action_option($hardcoded, $action_option, $action_lang, $folder)
{
	global $db, $template, $user;

	$l_action = $s_action_options = '';
	if ($hardcoded)
	{
		$option = explode('|', $action_option);
		$action = (int) $option[0];
		$folder_id = (int) $option[1];

		$l_action = $action_lang[$action];
		if ($action == ACTION_PLACE_INTO_FOLDER)
		{
			$l_action .= ' -> ' . $folder[$folder_id]['folder_name'];
		}
	}
	else
	{
		foreach ($action_lang as $action => $lang)
		{
			if ($action == ACTION_PLACE_INTO_FOLDER)
			{
				foreach ($folder as $folder_id => $folder_ary)
				{
					$s_action_options .= '<option value="' . $action . '|' . $folder_id . '"' . (($action_option == $action . '|' . $folder_id) ? ' selected="selected"' : '') . '>' . $lang . ' -> ' . $folder_ary['folder_name'] . '</option>';
				}
			}
			else
			{
				$s_action_options .= '<option value="' . $action . '|0"' . (($action_option == $action . '|0') ? ' selected="selected"' : '') . '>' . $lang . '</option>';
			}
		}
	}

	$template->assign_vars(array(
		'S_ACTION_DEFINED'	=> true,
		'S_ACTION_SELECT'	=> ($hardcoded) ? false : true,
		'ACTION_CURRENT'	=> $l_action,
		'S_ACTION_OPTIONS'	=> $s_action_options,
		'ACTION_OPTION'		=> $action_option)
	);
}

/**
* Defining rule option for message rules
*/
function define_rule_option($hardcoded, $rule_option, $rule_lang, $check_ary)
{
	global $template;

	$s_rule_options = '';
	if (!$hardcoded)
	{
		foreach ($check_ary as $value => $_check)
		{
			$s_rule_options .= '<option value="' . $value . '"' . (($value == $rule_option) ? ' selected="selected"' : '') . '>' . $rule_lang[$value] . '</option>';
		}
	}

	$template->assign_vars(array(
		'S_RULE_DEFINED'	=> true,
		'S_RULE_SELECT'		=> !$hardcoded,
		'RULE_CURRENT'		=> isset($rule_lang[$rule_option]) ? $rule_lang[$rule_option] : '',
		'S_RULE_OPTIONS'	=> $s_rule_options,
		'RULE_OPTION'		=> $rule_option)
	);
}

/**
* Defining condition option for message rules
*/
function define_cond_option($hardcoded, $cond_option, $rule_option, $global_rule_conditions)
{
	global $db, $template, $auth, $user;
	
	$template->assign_vars(array(
		'S_COND_DEFINED'	=> true,
		'S_COND_SELECT'		=> (!$hardcoded && isset($global_rule_conditions[$rule_option])) ? true : false)
	);

	// Define COND_OPTION
	if (!isset($global_rule_conditions[$rule_option]))
	{
		$template->assign_vars(array(
			'COND_OPTION'	=> 'none',
			'COND_CURRENT'	=> false)
		);
		return;
	}

	// Define Condition
	$condition = $global_rule_conditions[$rule_option];
	$current_value = '';

	switch ($condition)
	{
		case 'text':
			$rule_string = utf8_normalize_nfc(request_var('rule_string', '', true));
			
			$template->assign_vars(array(
				'S_TEXT_CONDITION'	=> true,
				'CURRENT_STRING'	=> $rule_string,
				'CURRENT_USER_ID'	=> 0,
				'CURRENT_GROUP_ID'	=> 0)
			);

			$current_value = $rule_string;
		break;

		case 'user':
			$rule_user_id = request_var('rule_user_id', 0);
			$rule_string = utf8_normalize_nfc(request_var('rule_string', '', true));
			
			if ($rule_string && !$rule_user_id)
			{
				$sql = 'SELECT user_id
					FROM ' . USERS_TABLE . "
					WHERE username_clean = '" . $db->sql_escape(utf8_clean_string($rule_string)) . "'";
				$result = $db->sql_query($sql);
				$rule_user_id = (int) $db->sql_fetchfield('user_id');
				$db->sql_freeresult($result);

				if (!$rule_user_id)
				{
					$rule_string = '';
				}
			}
			else if (!$rule_string && $rule_user_id)
			{
				$sql = 'SELECT username
					FROM ' . USERS_TABLE . "
					WHERE user_id = $rule_user_id";
				$result = $db->sql_query($sql);
				$rule_string = $db->sql_fetchfield('username');
				$db->sql_freeresult($result);

				if (!$rule_string)
				{
					$rule_user_id = 0;
				}
			}

			$template->assign_vars(array(
				'S_USER_CONDITION'	=> true,
				'CURRENT_STRING'	=> $rule_string,
				'CURRENT_USER_ID'	=> $rule_user_id,
				'CURRENT_GROUP_ID'	=> 0)
			);

			$current_value = $rule_string;
		break;

		case 'group':
			$rule_group_id = request_var('rule_group_id', 0);
			$rule_string = utf8_normalize_nfc(request_var('rule_string', '', true));

			$sql = 'SELECT g.group_id, g.group_name, g.group_type
					FROM ' . GROUPS_TABLE . ' g ';

			if (!$auth->acl_gets('a_group', 'a_groupadd', 'a_groupdel'))
			{
				$sql .= 'LEFT JOIN ' . USER_GROUP_TABLE . ' ug
					ON (
						g.group_id = ug.group_id
						AND ug.user_id = ' . $user->data['user_id'] . '
						AND ug.user_pending = 0
					)
					WHERE (ug.user_id = ' . $user->data['user_id'] . ' OR g.group_type <> ' . GROUP_HIDDEN . ')
					AND';
			}
			else
			{
				$sql .= 'WHERE';
			}
			
			$sql .= " (g.group_name NOT IN ('GUESTS', 'BOTS') OR g.group_type <> " . GROUP_SPECIAL . ')	
				ORDER BY g.group_type DESC, g.group_name ASC';
			
			$result = $db->sql_query($sql);

			$s_group_options = '';
			while ($row = $db->sql_fetchrow($result))
			{
				if ($rule_group_id && ($row['group_id'] == $rule_group_id))
				{
					$rule_string = (($row['group_type'] == GROUP_SPECIAL) ? $user->lang['G_' . $row['group_name']] : $row['group_name']);
				}

				$s_class	= ($row['group_type'] == GROUP_SPECIAL) ? ' class="sep"' : '';
				$s_selected	= ($row['group_id'] == $rule_group_id) ? ' selected="selected"' : '';
				
				$s_group_options .= '<option value="' . $row['group_id'] . '"' . $s_class . $s_selected . '>' . (($row['group_type'] == GROUP_SPECIAL) ? $user->lang['G_' . $row['group_name']] : $row['group_name']) . '</option>';
			}
			$db->sql_freeresult($result);

			$template->assign_vars(array(
				'S_GROUP_CONDITION'	=> true,
				'S_GROUP_OPTIONS'	=> $s_group_options,
				'CURRENT_STRING'	=> $rule_string,
				'CURRENT_USER_ID'	=> 0,
				'CURRENT_GROUP_ID'	=> $rule_group_id)
			);

			$current_value = $rule_string;
		break;

		default:
			return;
	}

	$template->assign_vars(array(
		'COND_OPTION'	=> $condition,
		'COND_CURRENT'	=> $current_value)
	);
}

/**
* Display defined message rules
*/
function show_defined_rules($user_id, $check_lang, $rule_lang, $action_lang, $folder)
{
	global $db, $template;

	$sql = 'SELECT *
		FROM ' . PRIVMSGS_RULES_TABLE . '
		WHERE user_id = ' . $user_id . '
		ORDER BY rule_id ASC';
	$result = $db->sql_query($sql);
	
	$count = 0;
	while ($row = $db->sql_fetchrow($result))
	{
		$template->assign_block_vars('rule', array(
			'COUNT'		=> ++$count,
			'RULE_ID'	=> $row['rule_id'],
			'CHECK'		=> $check_lang[$row['rule_check']],
			'RULE'		=> $rule_lang[$row['rule_connection']],
			'STRING'	=> $row['rule_string'],
			'ACTION'	=> $action_lang[$row['rule_action']],
			'FOLDER'	=> ($row['rule_action'] == ACTION_PLACE_INTO_FOLDER) ? $folder[$row['rule_folder_id']]['folder_name'] : '')
		);
	}
	$db->sql_freeresult($result);
}

?>