<?php
/**
*
* @package acp
* @version $Id: acp_inactive.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2006 phpBB Group
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
class acp_inactive
{
	var $u_action;
	var $p_master;

	function acp_inactive(&$p_master)
	{
		$this->p_master = &$p_master;
	}

	function main($id, $mode)
	{
		global $config, $db, $user, $auth, $template;
		global $phpbb_root_path, $phpbb_admin_path, $phpEx, $table_prefix;

		include($phpbb_root_path . 'includes/functions_user.' . $phpEx);

		$user->add_lang('memberlist');

		$action = request_var('action', '');
		$mark	= (isset($_REQUEST['mark'])) ? request_var('mark', array(0)) : array();
		$start	= request_var('start', 0);
		$submit = isset($_POST['submit']);

		// Sort keys
		$sort_days	= request_var('st', 0);
		$sort_key	= request_var('sk', 'i');
		$sort_dir	= request_var('sd', 'd');

		$form_key = 'acp_inactive';
		add_form_key($form_key);

		if ($submit && sizeof($mark))
		{
			if ($action !== 'delete' && !check_form_key($form_key))
			{
				trigger_error($user->lang['FORM_INVALID'] . adm_back_link($this->u_action), E_USER_WARNING);
			}

			switch ($action)
			{
				case 'activate':
				case 'delete':

					$sql = 'SELECT user_id, username
						FROM ' . USERS_TABLE . '
						WHERE ' . $db->sql_in_set('user_id', $mark);
					$result = $db->sql_query($sql);
				
					$user_affected = array();
					while ($row = $db->sql_fetchrow($result))
					{
						$user_affected[$row['user_id']] = $row['username'];
					}
					$db->sql_freeresult($result);

					if ($action == 'activate')
					{
						if ($config['require_activation'] == USER_ACTIVATION_ADMIN)
						{
							// Get those 'being activated'...
							$sql = 'SELECT user_id, username, user_email, user_lang
								FROM ' . USERS_TABLE . '
								WHERE ' . $db->sql_in_set('user_id', $mark) . '
									AND user_type = ' . USER_INACTIVE;
							$result = $db->sql_query($sql);

							$inactive_users = array();
							while ($row = $db->sql_fetchrow($result))
							{
								$inactive_users[] = $row;
							}
							$db->sql_freeresult($result);
						}

						user_active_flip('activate', $mark);

						if ($config['require_activation'] == USER_ACTIVATION_ADMIN && !empty($inactive_users))
						{
							include_once($phpbb_root_path . 'includes/functions_messenger.' . $phpEx);

							$messenger = new messenger();

							foreach ($inactive_users as $row)
							{
								$messenger->template('admin_welcome_activated', $row['user_lang']);

								$messenger->to($row['user_email'], $row['username']);

								$messenger->headers('X-AntiAbuse: Board servername - ' . $config['server_name']);
								$messenger->headers('X-AntiAbuse: User_id - ' . $user->data['user_id']);
								$messenger->headers('X-AntiAbuse: Username - ' . $user->data['username']);

								$messenger->assign_vars(array(
									'USERNAME'	=> htmlspecialchars_decode($row['username']))
								);

								$messenger->send(NOTIFY_EMAIL);
							}

							$messenger->save_queue();
						}
					}
					else if ($action == 'delete')
					{
						if (confirm_box(true))
						{
							if (!$auth->acl_get('a_userdel'))
							{
								trigger_error($user->lang['NO_AUTH_OPERATION'] . adm_back_link($this->u_action), E_USER_WARNING);
							}

							foreach ($mark as $user_id)
							{
								user_delete('retain', $user_id, $user_affected[$user_id]);
							}

							add_log('admin', 'LOG_INACTIVE_' . strtoupper($action), implode(', ', $user_affected));
						}
						else
						{
							$s_hidden_fields = array(
								'mode'			=> $mode,
								'action'		=> $action,
								'mark'			=> $mark,
								'submit'		=> 1,
								'start'			=> $start,
							);
							confirm_box(false, $user->lang['CONFIRM_OPERATION'], build_hidden_fields($s_hidden_fields));
						}
					}

				break;

				case 'remind':
					if (empty($config['email_enable']))
					{
						trigger_error($user->lang['EMAIL_DISABLED'] . adm_back_link($this->u_action), E_USER_WARNING);
					}

					$sql = 'SELECT user_id, username, user_email, user_lang, user_jabber, user_notify_type, user_regdate, user_actkey
						FROM ' . USERS_TABLE . '
						WHERE ' . $db->sql_in_set('user_id', $mark);
					$result = $db->sql_query($sql);

					if ($row = $db->sql_fetchrow($result))
					{
						// Send the messages
						include_once($phpbb_root_path . 'includes/functions_messenger.' . $phpEx);

						$messenger = new messenger();
						$usernames = array();

						do
						{
							$messenger->template('user_remind_inactive', $row['user_lang']);

							$messenger->to($row['user_email'], $row['username']);
							$messenger->im($row['user_jabber'], $row['username']);

							$messenger->assign_vars(array(
								'USERNAME'		=> htmlspecialchars_decode($row['username']),
								'REGISTER_DATE'	=> $user->format_date($row['user_regdate']),
								'U_ACTIVATE'	=> generate_board_url() . "/ucp.$phpEx?mode=activate&u=" . $row['user_id'] . '&k=' . $row['user_actkey'])
							);

							$messenger->send($row['user_notify_type']);

							$usernames[] = $row['username'];
						}
						while ($row = $db->sql_fetchrow($result));

						$messenger->save_queue();

						add_log('admin', 'LOG_INACTIVE_REMIND', implode(', ', $usernames));
						unset($usernames);
					}
					$db->sql_freeresult($result);
		
				break;
			}
		}

		// Sorting
		$limit_days = array(0 => $user->lang['ALL_ENTRIES'], 1 => $user->lang['1_DAY'], 7 => $user->lang['7_DAYS'], 14 => $user->lang['2_WEEKS'], 30 => $user->lang['1_MONTH'], 90 => $user->lang['3_MONTHS'], 180 => $user->lang['6_MONTHS'], 365 => $user->lang['1_YEAR']);
		$sort_by_text = array('i' => $user->lang['SORT_INACTIVE'], 'j' => $user->lang['SORT_REG_DATE'], 'l' => $user->lang['SORT_LAST_VISIT'], 'r' => $user->lang['SORT_REASON'], 'u' => $user->lang['SORT_USERNAME']);
		$sort_by_sql = array('i' => 'user_inactive_time', 'j' => 'user_regdate', 'l' => 'user_lastvisit', 'r' => 'user_inactive_reason', 'u' => 'username_clean');

		$s_limit_days = $s_sort_key = $s_sort_dir = $u_sort_param = '';
		gen_sort_selects($limit_days, $sort_by_text, $sort_days, $sort_key, $sort_dir, $s_limit_days, $s_sort_key, $s_sort_dir, $u_sort_param);

		// Define where and sort sql for use in displaying logs
		$sql_where = ($sort_days) ? (time() - ($sort_days * 86400)) : 0;
		$sql_sort = $sort_by_sql[$sort_key] . ' ' . (($sort_dir == 'd') ? 'DESC' : 'ASC');

		$inactive = array();
		$inactive_count = 0;

		$start = view_inactive_users($inactive, $inactive_count, $config['topics_per_page'], $start, $sql_where, $sql_sort);

		foreach ($inactive as $row)
		{
			$template->assign_block_vars('inactive', array(
				'INACTIVE_DATE'	=> $user->format_date($row['user_inactive_time']),
				'JOINED'		=> $user->format_date($row['user_regdate']),
				'LAST_VISIT'	=> (!$row['user_lastvisit']) ? ' - ' : $user->format_date($row['user_lastvisit']),
				'REASON'		=> $row['inactive_reason'],
				'USER_ID'		=> $row['user_id'],
				'USERNAME'		=> $row['username'],
				'U_USER_ADMIN'	=> append_sid("{$phpbb_admin_path}index.$phpEx", "i=users&amp;mode=overview&amp;u={$row['user_id']}"))
			);
		}

		$option_ary = array('activate' => 'ACTIVATE', 'delete' => 'DELETE');
		if ($config['email_enable'])
		{
			$option_ary += array('remind' => 'REMIND');
		}

		$template->assign_vars(array(
			'S_INACTIVE_USERS'		=> true,
			'S_INACTIVE_OPTIONS'	=> build_select($option_ary),

			'S_LIMIT_DAYS'	=> $s_limit_days,
			'S_SORT_KEY'	=> $s_sort_key,
			'S_SORT_DIR'	=> $s_sort_dir,
			'S_ON_PAGE'		=> on_page($inactive_count, $config['topics_per_page'], $start),
			'PAGINATION'	=> generate_pagination($this->u_action . "&amp;$u_sort_param", $inactive_count, $config['topics_per_page'], $start, true),
			
			'U_ACTION'		=> $this->u_action . '&amp;start=' . $start,
		));

		$this->tpl_name = 'acp_inactive';
		$this->page_title = 'ACP_INACTIVE_USERS';
	}
}

?>