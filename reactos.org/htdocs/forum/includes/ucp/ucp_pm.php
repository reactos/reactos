<?php
/**
* @package ucp
* @version $Id: ucp_pm.php 8479 2008-03-29 00:22:48Z naderman $
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
* Private Message Class
*
* $_REQUEST['folder'] display folder with the id used
* $_REQUEST['folder'] inbox|outbox|sentbox display folder with the associated name
*
*	Display Messages (default to inbox) - mode=view
*	Display single message - mode=view&p=[msg_id] or &p=[msg_id] (short linkage)
*
*	if the folder id with (&f=[folder_id]) is used when displaying messages, one query will be saved. If it is not used, phpBB needs to grab
*	the folder id first in order to display the input boxes and folder names and such things. ;) phpBB always checks this against the database to make
*	sure the user is able to view the message.
*
*	Composing Messages (mode=compose):
*		To specific user (u=[user_id])
*		To specific group (g=[group_id])
*		Quoting a post (action=quotepost&p=[post_id])
*		Quoting a PM (action=quote&p=[msg_id])
*		Forwarding a PM (action=forward&p=[msg_id])
*
* @package ucp
*/
class ucp_pm
{
	var $u_action;

	function main($id, $mode)
	{
		global $user, $template, $phpbb_root_path, $auth, $phpEx, $db, $config;

		if (!$user->data['is_registered'])
		{
			trigger_error('NO_MESSAGE');
		}

		// Is PM disabled?
		if (!$config['allow_privmsg'])
		{
			trigger_error('PM_DISABLED');
		}

		$user->add_lang('posting');
		$template->assign_var('S_PRIVMSGS', true);

		// Folder directly specified?
		$folder_specified = request_var('folder', '');

		if (!in_array($folder_specified, array('inbox', 'outbox', 'sentbox')))
		{
			$folder_specified = (int) $folder_specified;
		}
		else
		{
			$folder_specified = ($folder_specified == 'inbox') ? PRIVMSGS_INBOX : (($folder_specified == 'outbox') ? PRIVMSGS_OUTBOX : PRIVMSGS_SENTBOX);
		}

		if (!$folder_specified)
		{
			$mode = (!$mode) ? request_var('mode', 'view') : $mode;
		}
		else
		{
			$mode = 'view';
		}

		include($phpbb_root_path . 'includes/functions_privmsgs.' . $phpEx);

		switch ($mode)
		{
			// New private messages popup
			case 'popup':

				$l_new_message = '';
				if ($user->data['is_registered'])
				{
					if ($user->data['user_new_privmsg'])
					{
						$l_new_message = ($user->data['user_new_privmsg'] == 1) ? $user->lang['YOU_NEW_PM'] : $user->lang['YOU_NEW_PMS'];
					}
					else
					{
						$l_new_message = $user->lang['YOU_NO_NEW_PM'];
					}
				}

				$template->assign_vars(array(
					'MESSAGE'			=> $l_new_message,
					'S_NOT_LOGGED_IN'	=> ($user->data['user_id'] == ANONYMOUS) ? true : false,
					'CLICK_TO_VIEW'		=> sprintf($user->lang['CLICK_VIEW_PRIVMSG'], '<a href="' . append_sid("{$phpbb_root_path}ucp.$phpEx", 'i=pm&amp;folder=inbox') . '" onclick="jump_to_inbox(this.href); return false;">', '</a>'),
					'U_INBOX'			=> append_sid("{$phpbb_root_path}ucp.$phpEx", 'i=pm&amp;folder=inbox'),
					'UA_INBOX'			=> append_sid("{$phpbb_root_path}ucp.$phpEx", 'i=pm&folder=inbox', false))
				);

				$tpl_file = 'ucp_pm_popup';
			break;

			// Compose message
			case 'compose':
				$action = request_var('action', 'post');

				get_folder($user->data['user_id']);

				if (!$auth->acl_get('u_sendpm'))
				{
					trigger_error('NO_AUTH_SEND_MESSAGE');
				}

				include($phpbb_root_path . 'includes/ucp/ucp_pm_compose.' . $phpEx);
				compose_pm($id, $mode, $action);

				$tpl_file = 'posting_body';
			break;

			case 'options':
				$sql = 'SELECT group_message_limit
					FROM ' . GROUPS_TABLE . '
					WHERE group_id = ' . $user->data['group_id'];
				$result = $db->sql_query($sql, 3600);
				$message_limit = (int) $db->sql_fetchfield('group_message_limit');
				$db->sql_freeresult($result);

				$user->data['message_limit'] = (!$message_limit) ? $config['pm_max_msgs'] : $message_limit;

				get_folder($user->data['user_id']);

				include($phpbb_root_path . 'includes/ucp/ucp_pm_options.' . $phpEx);
				message_options($id, $mode, $global_privmsgs_rules, $global_rule_conditions);

				$tpl_file = 'ucp_pm_options';
			break;

			case 'drafts':

				get_folder($user->data['user_id']);
				$this->p_name = 'pm';

				// Call another module... please do not try this at home... Hoochie Coochie Man
				include($phpbb_root_path . 'includes/ucp/ucp_main.' . $phpEx);

				$module = new ucp_main($this);
				$module->u_action = $this->u_action;
				$module->main($id, $mode);

				$this->tpl_name = $module->tpl_name;
				$this->page_title = 'UCP_PM_DRAFTS';

				unset($module);
				return;

			break;

			case 'view':

				$sql = 'SELECT group_message_limit
					FROM ' . GROUPS_TABLE . '
					WHERE group_id = ' . $user->data['group_id'];
				$result = $db->sql_query($sql, 3600);
				$message_limit = (int) $db->sql_fetchfield('group_message_limit');
				$db->sql_freeresult($result);

				$user->data['message_limit'] = (!$message_limit) ? $config['pm_max_msgs'] : $message_limit;

				if ($folder_specified)
				{
					$folder_id = $folder_specified;
					$action = 'view_folder';
				}
				else
				{
					$folder_id = request_var('f', PRIVMSGS_NO_BOX);
					$action = request_var('action', 'view_folder');
				}

				$msg_id = request_var('p', 0);
				$view	= request_var('view', '');

				// View message if specified
				if ($msg_id)
				{
					$action = 'view_message';
				}

				if (!$auth->acl_get('u_readpm'))
				{
					trigger_error('NO_AUTH_READ_MESSAGE');
				}

				// Do not allow hold messages to be seen
				if ($folder_id == PRIVMSGS_HOLD_BOX)
				{
					trigger_error('NO_AUTH_READ_HOLD_MESSAGE');
				}


				// First Handle Mark actions and moving messages
				$submit_mark	= (isset($_POST['submit_mark'])) ? true : false;
				$move_pm		= (isset($_POST['move_pm'])) ? true : false;
				$mark_option	= request_var('mark_option', '');
				$dest_folder	= request_var('dest_folder', PRIVMSGS_NO_BOX);

				// Is moving PM triggered through mark options?
				if (!in_array($mark_option, array('mark_important', 'delete_marked')) && $submit_mark)
				{
					$move_pm = true;
					$dest_folder = (int) $mark_option;
					$submit_mark = false;
				}

				// Move PM
				if ($move_pm)
				{
					$move_msg_ids	= (isset($_POST['marked_msg_id'])) ? request_var('marked_msg_id', array(0)) : array();
					$cur_folder_id	= request_var('cur_folder_id', PRIVMSGS_NO_BOX);

					if (move_pm($user->data['user_id'], $user->data['message_limit'], $move_msg_ids, $dest_folder, $cur_folder_id))
					{
						// Return to folder view if single message moved
						if ($action == 'view_message')
						{
							$msg_id		= 0;
							$folder_id	= request_var('cur_folder_id', PRIVMSGS_NO_BOX);
							$action		= 'view_folder';
						}
					}
				}

				// Message Mark Options
				if ($submit_mark)
				{
					handle_mark_actions($user->data['user_id'], $mark_option);
				}

				// If new messages arrived, place them into the appropriate folder
				$num_not_moved = $num_removed = 0;
				$release = request_var('release', 0);

				if ($user->data['user_new_privmsg'] && $action == 'view_folder')
				{
					$return = place_pm_into_folder($global_privmsgs_rules, $release);
					$num_not_moved = $return['not_moved'];
					$num_removed = $return['removed'];
				}

				if (!$msg_id && $folder_id == PRIVMSGS_NO_BOX)
				{
					$folder_id = PRIVMSGS_INBOX;
				}
				else if ($msg_id && $folder_id == PRIVMSGS_NO_BOX)
				{
					$sql = 'SELECT folder_id
						FROM ' . PRIVMSGS_TO_TABLE . "
						WHERE msg_id = $msg_id
							AND folder_id <> " . PRIVMSGS_NO_BOX . '
							AND user_id = ' . $user->data['user_id'];
					$result = $db->sql_query($sql);
					$row = $db->sql_fetchrow($result);
					$db->sql_freeresult($result);

					if (!$row)
					{
						trigger_error('NO_MESSAGE');
					}
					$folder_id = (int) $row['folder_id'];
				}

				$message_row = array();
				if ($action == 'view_message' && $msg_id)
				{
					// Get Message user want to see
					if ($view == 'next' || $view == 'previous')
					{
						$sql_condition = ($view == 'next') ? '>' : '<';
						$sql_ordering = ($view == 'next') ? 'ASC' : 'DESC';

						$sql = 'SELECT t.msg_id
							FROM ' . PRIVMSGS_TO_TABLE . ' t, ' . PRIVMSGS_TABLE . ' p, ' . PRIVMSGS_TABLE . " p2
							WHERE p2.msg_id = $msg_id
								AND t.folder_id = $folder_id
								AND t.user_id = " . $user->data['user_id'] . "
								AND t.msg_id = p.msg_id
								AND p.message_time $sql_condition p2.message_time
							ORDER BY p.message_time $sql_ordering";
						$result = $db->sql_query_limit($sql, 1);
						$row = $db->sql_fetchrow($result);
						$db->sql_freeresult($result);

						if (!$row)
						{
							$message = ($view == 'next') ? 'NO_NEWER_PM' : 'NO_OLDER_PM';
							trigger_error($message);
						}
						else
						{
							$msg_id = $row['msg_id'];
						}
					}

					$sql = 'SELECT t.*, p.*, u.*
						FROM ' . PRIVMSGS_TO_TABLE . ' t, ' . PRIVMSGS_TABLE . ' p, ' . USERS_TABLE . ' u
						WHERE t.user_id = ' . $user->data['user_id'] . "
							AND p.author_id = u.user_id
							AND t.folder_id = $folder_id
							AND t.msg_id = p.msg_id
							AND p.msg_id = $msg_id";
					$result = $db->sql_query($sql);
					$message_row = $db->sql_fetchrow($result);
					$db->sql_freeresult($result);

					if (!$message_row)
					{
						trigger_error('NO_MESSAGE');
					}

					// Update unread status
					update_unread_status($message_row['pm_unread'], $message_row['msg_id'], $user->data['user_id'], $folder_id);
				}

				$folder = get_folder($user->data['user_id'], $folder_id);

				$s_folder_options = $s_to_folder_options = '';
				foreach ($folder as $f_id => $folder_ary)
				{
					$option = '<option' . ((!in_array($f_id, array(PRIVMSGS_INBOX, PRIVMSGS_OUTBOX, PRIVMSGS_SENTBOX))) ? ' class="sep"' : '') . ' value="' . $f_id . '"' . (($f_id == $folder_id) ? ' selected="selected"' : '') . '>' . $folder_ary['folder_name'] . (($folder_ary['unread_messages']) ? ' [' . $folder_ary['unread_messages'] . '] ' : '') . '</option>';

					$s_to_folder_options .= ($f_id != PRIVMSGS_OUTBOX && $f_id != PRIVMSGS_SENTBOX) ? $option : '';
					$s_folder_options .= $option;
				}
				clean_sentbox($folder[PRIVMSGS_SENTBOX]['num_messages']);

				// Header for message view - folder and so on
				$folder_status = get_folder_status($folder_id, $folder);

				$template->assign_vars(array(
					'CUR_FOLDER_ID'			=> $folder_id,
					'CUR_FOLDER_NAME'		=> $folder_status['folder_name'],
					'NUM_NOT_MOVED'			=> $num_not_moved,
					'NUM_REMOVED'			=> $num_removed,
					'RELEASE_MESSAGE_INFO'	=> sprintf($user->lang['RELEASE_MESSAGES'], '<a href="' . $this->u_action . '&amp;folder=' . $folder_id . '&amp;release=1">', '</a>'),
					'NOT_MOVED_MESSAGES'	=> ($num_not_moved == 1) ? $user->lang['NOT_MOVED_MESSAGE'] : sprintf($user->lang['NOT_MOVED_MESSAGES'], $num_not_moved),
					'RULE_REMOVED_MESSAGES'	=> ($num_removed == 1) ? $user->lang['RULE_REMOVED_MESSAGE'] : sprintf($user->lang['RULE_REMOVED_MESSAGES'], $num_removed),

					'S_FOLDER_OPTIONS'		=> $s_folder_options,
					'S_TO_FOLDER_OPTIONS'	=> $s_to_folder_options,
					'S_FOLDER_ACTION'		=> $this->u_action . '&amp;action=view_folder',
					'S_PM_ACTION'			=> $this->u_action . '&amp;action=' . $action,

					'U_INBOX'				=> $this->u_action . '&amp;folder=inbox',
					'U_OUTBOX'				=> $this->u_action . '&amp;folder=outbox',
					'U_SENTBOX'				=> $this->u_action . '&amp;folder=sentbox',
					'U_CREATE_FOLDER'		=> $this->u_action . '&amp;mode=options',
					'U_CURRENT_FOLDER'		=> $this->u_action . '&amp;folder=' . $folder_id,

					'S_IN_INBOX'			=> ($folder_id == PRIVMSGS_INBOX) ? true : false,
					'S_IN_OUTBOX'			=> ($folder_id == PRIVMSGS_OUTBOX) ? true : false,
					'S_IN_SENTBOX'			=> ($folder_id == PRIVMSGS_SENTBOX) ? true : false,

					'FOLDER_STATUS'				=> $folder_status['message'],
					'FOLDER_MAX_MESSAGES'		=> $folder_status['max'],
					'FOLDER_CUR_MESSAGES'		=> $folder_status['cur'],
					'FOLDER_REMAINING_MESSAGES'	=> $folder_status['remaining'],
					'FOLDER_PERCENT'			=> $folder_status['percent'])
				);

				if ($action == 'view_folder')
				{
					include($phpbb_root_path . 'includes/ucp/ucp_pm_viewfolder.' . $phpEx);
					view_folder($id, $mode, $folder_id, $folder);

					$tpl_file = 'ucp_pm_viewfolder';
				}
				else if ($action == 'view_message')
				{
					$template->assign_vars(array(
						'S_VIEW_MESSAGE'	=> true,
						'MSG_ID'			=> $msg_id)
					);

					if (!$msg_id)
					{
						trigger_error('NO_MESSAGE');
					}

					include($phpbb_root_path . 'includes/ucp/ucp_pm_viewmessage.' . $phpEx);
					view_message($id, $mode, $folder_id, $msg_id, $folder, $message_row);

					$tpl_file = ($view == 'print') ? 'ucp_pm_viewmessage_print' : 'ucp_pm_viewmessage';
				}

			break;

			default:
				trigger_error('NO_ACTION_MODE', E_USER_ERROR);
			break;
		}

		$template->assign_vars(array(
			'L_TITLE'			=> $user->lang['UCP_PM_' . strtoupper($mode)],
			'S_UCP_ACTION'		=> $this->u_action . ((isset($action)) ? "&amp;action=$action" : ''))
		);

		// Set desired template
		$this->tpl_name = $tpl_file;
		$this->page_title = 'UCP_PM_' . strtoupper($mode);
	}
}

?>