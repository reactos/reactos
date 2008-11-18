<?php
/**
*
* @package acp
* @version $Id: acp_forums.php 8479 2008-03-29 00:22:48Z naderman $
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
class acp_forums
{
	var $u_action;
	var $parent_id = 0;

	function main($id, $mode)
	{
		global $db, $user, $auth, $template, $cache;
		global $config, $phpbb_admin_path, $phpbb_root_path, $phpEx;

		$user->add_lang('acp/forums');
		$this->tpl_name = 'acp_forums';
		$this->page_title = 'ACP_MANAGE_FORUMS';

		$form_key = 'acp_forums';
		add_form_key($form_key);

		$action		= request_var('action', '');
		$update		= (isset($_POST['update'])) ? true : false;
		$forum_id	= request_var('f', 0);

		$this->parent_id	= request_var('parent_id', 0);
		$forum_data = $errors = array();
		if ($update && !check_form_key($form_key))
		{
			$update = false;
			$errors[] = $user->lang['FORM_INVALID'];
		}

		// Check additional permissions
		switch ($action)
		{
			case 'progress_bar':
				$start = request_var('start', 0);
				$total = request_var('total', 0);

				$this->display_progress_bar($start, $total);
				exit_handler();
			break;

			case 'delete':

				if (!$auth->acl_get('a_forumdel'))
				{
					trigger_error($user->lang['NO_PERMISSION_FORUM_DELETE'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}

			break;

			case 'add':

				if (!$auth->acl_get('a_forumadd'))
				{
					trigger_error($user->lang['NO_PERMISSION_FORUM_ADD'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}
			
			break;
		}

		// Major routines
		if ($update)
		{
			switch ($action)
			{
				case 'delete':
					$action_subforums	= request_var('action_subforums', '');
					$subforums_to_id	= request_var('subforums_to_id', 0);
					$action_posts		= request_var('action_posts', '');
					$posts_to_id		= request_var('posts_to_id', 0);

					$errors = $this->delete_forum($forum_id, $action_posts, $action_subforums, $posts_to_id, $subforums_to_id);

					if (sizeof($errors))
					{
						break;
					}

					$auth->acl_clear_prefetch();
					$cache->destroy('sql', FORUMS_TABLE);

					trigger_error($user->lang['FORUM_DELETED'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id));
	
				break;

				case 'edit':
					$forum_data = array(
						'forum_id'		=>	$forum_id
					);

				// No break here

				case 'add':

					$forum_data += array(
						'parent_id'				=> request_var('forum_parent_id', $this->parent_id),
						'forum_type'			=> request_var('forum_type', FORUM_POST),
						'type_action'			=> request_var('type_action', ''),
						'forum_status'			=> request_var('forum_status', ITEM_UNLOCKED),
						'forum_parents'			=> '',
						'forum_name'			=> utf8_normalize_nfc(request_var('forum_name', '', true)),
						'forum_link'			=> request_var('forum_link', ''),
						'forum_link_track'		=> request_var('forum_link_track', false),
						'forum_desc'			=> utf8_normalize_nfc(request_var('forum_desc', '', true)),
						'forum_desc_uid'		=> '',
						'forum_desc_options'	=> 7,
						'forum_desc_bitfield'	=> '',
						'forum_rules'			=> utf8_normalize_nfc(request_var('forum_rules', '', true)),
						'forum_rules_uid'		=> '',
						'forum_rules_options'	=> 7,
						'forum_rules_bitfield'	=> '',
						'forum_rules_link'		=> request_var('forum_rules_link', ''),
						'forum_image'			=> request_var('forum_image', ''),
						'forum_style'			=> request_var('forum_style', 0),
						'display_subforum_list'	=> request_var('display_subforum_list', false),
						'display_on_index'		=> request_var('display_on_index', false),
						'forum_topics_per_page'	=> request_var('topics_per_page', 0),
						'enable_indexing'		=> request_var('enable_indexing', true),
						'enable_icons'			=> request_var('enable_icons', false),
						'enable_prune'			=> request_var('enable_prune', false),
						'enable_post_review'	=> request_var('enable_post_review', true),
						'prune_days'			=> request_var('prune_days', 7),
						'prune_viewed'			=> request_var('prune_viewed', 7),
						'prune_freq'			=> request_var('prune_freq', 1),
						'prune_old_polls'		=> request_var('prune_old_polls', false),
						'prune_announce'		=> request_var('prune_announce', false),
						'prune_sticky'			=> request_var('prune_sticky', false),
						'forum_password'		=> request_var('forum_password', '', true),
						'forum_password_confirm'=> request_var('forum_password_confirm', '', true),
						'forum_password_unset'	=> request_var('forum_password_unset', false),
					);

					// Use link_display_on_index setting if forum type is link
					if ($forum_data['forum_type'] == FORUM_LINK)
					{
						$forum_data['display_on_index'] = request_var('link_display_on_index', false);

						// Linked forums are not able to be locked...
						$forum_data['forum_status'] = ITEM_UNLOCKED;
					}

					$forum_data['show_active'] = ($forum_data['forum_type'] == FORUM_POST) ? request_var('display_recent', false) : request_var('display_active', false);

					// Get data for forum rules if specified...
					if ($forum_data['forum_rules'])
					{
						generate_text_for_storage($forum_data['forum_rules'], $forum_data['forum_rules_uid'], $forum_data['forum_rules_bitfield'], $forum_data['forum_rules_options'], request_var('rules_parse_bbcode', false), request_var('rules_parse_urls', false), request_var('rules_parse_smilies', false));
					}

					// Get data for forum description if specified
					if ($forum_data['forum_desc'])
					{
						generate_text_for_storage($forum_data['forum_desc'], $forum_data['forum_desc_uid'], $forum_data['forum_desc_bitfield'], $forum_data['forum_desc_options'], request_var('desc_parse_bbcode', false), request_var('desc_parse_urls', false), request_var('desc_parse_smilies', false));
					}

					$errors = $this->update_forum_data($forum_data);

					if (!sizeof($errors))
					{
						$forum_perm_from = request_var('forum_perm_from', 0);

						// Copy permissions?
						if ($forum_perm_from && !empty($forum_perm_from) && $forum_perm_from != $forum_data['forum_id'] &&
							(($action != 'edit') || empty($forum_id) || ($auth->acl_get('a_fauth') && $auth->acl_get('a_authusers') && $auth->acl_get('a_authgroups') && $auth->acl_get('a_mauth'))))
						{
							// if we edit a forum delete current permissions first
							if ($action == 'edit')
							{
								$sql = 'DELETE FROM ' . ACL_USERS_TABLE . '
									WHERE forum_id = ' . (int) $forum_data['forum_id'];
								$db->sql_query($sql);
	
								$sql = 'DELETE FROM ' . ACL_GROUPS_TABLE . '
									WHERE forum_id = ' . (int) $forum_data['forum_id'];
								$db->sql_query($sql);
							}

							// From the mysql documentation:
							// Prior to MySQL 4.0.14, the target table of the INSERT statement cannot appear in the FROM clause of the SELECT part of the query. This limitation is lifted in 4.0.14.
							// Due to this we stay on the safe side if we do the insertion "the manual way"

							// Copy permisisons from/to the acl users table (only forum_id gets changed)
							$sql = 'SELECT user_id, auth_option_id, auth_role_id, auth_setting
								FROM ' . ACL_USERS_TABLE . '
								WHERE forum_id = ' . $forum_perm_from;
							$result = $db->sql_query($sql);

							$users_sql_ary = array();
							while ($row = $db->sql_fetchrow($result))
							{
								$users_sql_ary[] = array(
									'user_id'			=> (int) $row['user_id'],
									'forum_id'			=> (int) $forum_data['forum_id'],
									'auth_option_id'	=> (int) $row['auth_option_id'],
									'auth_role_id'		=> (int) $row['auth_role_id'],
									'auth_setting'		=> (int) $row['auth_setting']
								);
							}
							$db->sql_freeresult($result);

							// Copy permisisons from/to the acl groups table (only forum_id gets changed)
							$sql = 'SELECT group_id, auth_option_id, auth_role_id, auth_setting
								FROM ' . ACL_GROUPS_TABLE . '
								WHERE forum_id = ' . $forum_perm_from;
							$result = $db->sql_query($sql);

							$groups_sql_ary = array();
							while ($row = $db->sql_fetchrow($result))
							{
								$groups_sql_ary[] = array(
									'group_id'			=> (int) $row['group_id'],
									'forum_id'			=> (int) $forum_data['forum_id'],
									'auth_option_id'	=> (int) $row['auth_option_id'],
									'auth_role_id'		=> (int) $row['auth_role_id'],
									'auth_setting'		=> (int) $row['auth_setting']
								);
							}
							$db->sql_freeresult($result);

							// Now insert the data
							$db->sql_multi_insert(ACL_USERS_TABLE, $users_sql_ary);
							$db->sql_multi_insert(ACL_GROUPS_TABLE, $groups_sql_ary);
							cache_moderators();
						}

						$auth->acl_clear_prefetch();
						$cache->destroy('sql', FORUMS_TABLE);
	
						$acl_url = '&amp;mode=setting_forum_local&amp;forum_id[]=' . $forum_data['forum_id'];

						$message = ($action == 'add') ? $user->lang['FORUM_CREATED'] : $user->lang['FORUM_UPDATED'];

						// Redirect to permissions
						if ($auth->acl_get('a_fauth'))
						{
							$message .= '<br /><br />' . sprintf($user->lang['REDIRECT_ACL'], '<a href="' . append_sid("{$phpbb_admin_path}index.$phpEx", 'i=permissions' . $acl_url) . '">', '</a>');
						}

						// redirect directly to permission settings screen if authed
						if ($action == 'add' && !$forum_perm_from && $auth->acl_get('a_fauth'))
						{
							meta_refresh(4, append_sid("{$phpbb_admin_path}index.$phpEx", 'i=permissions' . $acl_url));
						}

						trigger_error($message . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id));
					}

				break;
			}
		}

		switch ($action)
		{
			case 'move_up':
			case 'move_down':

				if (!$forum_id)
				{
					trigger_error($user->lang['NO_FORUM'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}

				$sql = 'SELECT *
					FROM ' . FORUMS_TABLE . "
					WHERE forum_id = $forum_id";
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if (!$row)
				{
					trigger_error($user->lang['NO_FORUM'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}

				$move_forum_name = $this->move_forum_by($row, $action, 1);

				if ($move_forum_name !== false)
				{
					add_log('admin', 'LOG_FORUM_' . strtoupper($action), $row['forum_name'], $move_forum_name);
					$cache->destroy('sql', FORUMS_TABLE);
				}

			break;

			case 'sync':
				if (!$forum_id)
				{
					trigger_error($user->lang['NO_FORUM'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}

				@set_time_limit(0);

				$sql = 'SELECT forum_name, forum_topics_real
					FROM ' . FORUMS_TABLE . "
					WHERE forum_id = $forum_id";
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if (!$row)
				{
					trigger_error($user->lang['NO_FORUM'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}

				if ($row['forum_topics_real'])
				{
					$sql = 'SELECT MIN(topic_id) as min_topic_id, MAX(topic_id) as max_topic_id
						FROM ' . TOPICS_TABLE . '
						WHERE forum_id = ' . $forum_id;
					$result = $db->sql_query($sql);
					$row2 = $db->sql_fetchrow($result);
					$db->sql_freeresult($result);

					// Typecast to int if there is no data available
					$row2['min_topic_id'] = (int) $row2['min_topic_id'];
					$row2['max_topic_id'] = (int) $row2['max_topic_id'];

					$start = request_var('start', $row2['min_topic_id']);

					$batch_size = 2000;
					$end = $start + $batch_size;

					// Sync all topics in batch mode...
					sync('topic_approved', 'range', 'topic_id BETWEEN ' . $start . ' AND ' . $end, true, false);
					sync('topic', 'range', 'topic_id BETWEEN ' . $start . ' AND ' . $end, true, true);

					if ($end < $row2['max_topic_id'])
					{
						// We really need to find a way of showing statistics... no progress here
						$sql = 'SELECT COUNT(topic_id) as num_topics
							FROM ' . TOPICS_TABLE . '
							WHERE forum_id = ' . $forum_id . '
								AND topic_id BETWEEN ' . $start . ' AND ' . $end;
						$result = $db->sql_query($sql);
						$topics_done = request_var('topics_done', 0) + (int) $db->sql_fetchfield('num_topics');
						$db->sql_freeresult($result);

						$start += $batch_size;

						$url = $this->u_action . "&amp;parent_id={$this->parent_id}&amp;f=$forum_id&amp;action=sync&amp;start=$start&amp;topics_done=$topics_done&amp;total={$row['forum_topics_real']}";

						meta_refresh(0, $url);

						$template->assign_vars(array(
							'U_PROGRESS_BAR'		=> $this->u_action . "&amp;action=progress_bar&amp;start=$topics_done&amp;total={$row['forum_topics_real']}",
							'UA_PROGRESS_BAR'		=> addslashes($this->u_action . "&amp;action=progress_bar&amp;start=$topics_done&amp;total={$row['forum_topics_real']}"),
							'S_CONTINUE_SYNC'		=> true,
							'L_PROGRESS_EXPLAIN'	=> sprintf($user->lang['SYNC_IN_PROGRESS_EXPLAIN'], $topics_done, $row['forum_topics_real']))
						);

						return;
					}
				}

				$url = $this->u_action . "&amp;parent_id={$this->parent_id}&amp;f=$forum_id&amp;action=sync_forum";
				meta_refresh(0, $url);

				$template->assign_vars(array(
					'U_PROGRESS_BAR'		=> $this->u_action . '&amp;action=progress_bar',
					'UA_PROGRESS_BAR'		=> addslashes($this->u_action . '&amp;action=progress_bar'),
					'S_CONTINUE_SYNC'		=> true,
					'L_PROGRESS_EXPLAIN'	=> sprintf($user->lang['SYNC_IN_PROGRESS_EXPLAIN'], 0, $row['forum_topics_real']))
				);

				return;

			break;

			case 'sync_forum':

				$sql = 'SELECT forum_name, forum_type
					FROM ' . FORUMS_TABLE . "
					WHERE forum_id = $forum_id";
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if (!$row)
				{
					trigger_error($user->lang['NO_FORUM'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}

				sync('forum', 'forum_id', $forum_id, false, true);

				add_log('admin', 'LOG_FORUM_SYNC', $row['forum_name']);
				$cache->destroy('sql', FORUMS_TABLE);

				$template->assign_var('L_FORUM_RESYNCED', sprintf($user->lang['FORUM_RESYNCED'], $row['forum_name']));

			break;

			case 'add':
			case 'edit':

				if ($update)
				{
					$forum_data['forum_flags'] = 0;
					$forum_data['forum_flags'] += (request_var('forum_link_track', false)) ? FORUM_FLAG_LINK_TRACK : 0;
					$forum_data['forum_flags'] += (request_var('prune_old_polls', false)) ? FORUM_FLAG_PRUNE_POLL : 0;
					$forum_data['forum_flags'] += (request_var('prune_announce', false)) ? FORUM_FLAG_PRUNE_ANNOUNCE : 0;
					$forum_data['forum_flags'] += (request_var('prune_sticky', false)) ? FORUM_FLAG_PRUNE_STICKY : 0;
					$forum_data['forum_flags'] += ($forum_data['show_active']) ? FORUM_FLAG_ACTIVE_TOPICS : 0;
					$forum_data['forum_flags'] += (request_var('enable_post_review', true)) ? FORUM_FLAG_POST_REVIEW : 0;
				}

				// Show form to create/modify a forum
				if ($action == 'edit')
				{
					$this->page_title = 'EDIT_FORUM';
					$row = $this->get_forum_info($forum_id);
					$old_forum_type = $row['forum_type'];

					if (!$update)
					{
						$forum_data = $row;
					}
					else
					{
						$forum_data['left_id'] = $row['left_id'];
						$forum_data['right_id'] = $row['right_id'];
					}

					// Make sure no direct child forums are able to be selected as parents.
					$exclude_forums = array();
					foreach (get_forum_branch($forum_id, 'children') as $row)
					{
						$exclude_forums[] = $row['forum_id'];
					}

					$parents_list = make_forum_select($forum_data['parent_id'], $exclude_forums, false, false, false);

					$forum_data['forum_password_confirm'] = $forum_data['forum_password'];
				}
				else
				{
					$this->page_title = 'CREATE_FORUM';

					$forum_id = $this->parent_id;
					$parents_list = make_forum_select($this->parent_id, false, false, false, false);

					// Fill forum data with default values
					if (!$update)
					{
						$forum_data = array(
							'parent_id'				=> $this->parent_id,
							'forum_type'			=> FORUM_POST,
							'forum_status'			=> ITEM_UNLOCKED,
							'forum_name'			=> utf8_normalize_nfc(request_var('forum_name', '', true)),
							'forum_link'			=> '',
							'forum_link_track'		=> false,
							'forum_desc'			=> '',
							'forum_rules'			=> '',
							'forum_rules_link'		=> '',
							'forum_image'			=> '',
							'forum_style'			=> 0,
							'display_subforum_list'	=> true,
							'display_on_index'		=> false,
							'forum_topics_per_page'	=> 0,
							'enable_indexing'		=> true,
							'enable_icons'			=> false,
							'enable_prune'			=> false,
							'prune_days'			=> 7,
							'prune_viewed'			=> 7,
							'prune_freq'			=> 1,
							'forum_flags'			=> FORUM_FLAG_POST_REVIEW,
							'forum_password'		=> '',
							'forum_password_confirm'=> '',
						);
					}
				}

				$forum_rules_data = array(
					'text'			=> $forum_data['forum_rules'],
					'allow_bbcode'	=> true,
					'allow_smilies'	=> true,
					'allow_urls'	=> true
				);

				$forum_desc_data = array(
					'text'			=> $forum_data['forum_desc'],
					'allow_bbcode'	=> true,
					'allow_smilies'	=> true,
					'allow_urls'	=> true
				);

				$forum_rules_preview = '';

				// Parse rules if specified
				if ($forum_data['forum_rules'])
				{
					if (!isset($forum_data['forum_rules_uid']))
					{
						// Before we are able to display the preview and plane text, we need to parse our request_var()'d value...
						$forum_data['forum_rules_uid'] = '';
						$forum_data['forum_rules_bitfield'] = '';
						$forum_data['forum_rules_options'] = 0;

						generate_text_for_storage($forum_data['forum_rules'], $forum_data['forum_rules_uid'], $forum_data['forum_rules_bitfield'], $forum_data['forum_rules_options'], request_var('rules_allow_bbcode', false), request_var('rules_allow_urls', false), request_var('rules_allow_smilies', false));
					}

					// Generate preview content
					$forum_rules_preview = generate_text_for_display($forum_data['forum_rules'], $forum_data['forum_rules_uid'], $forum_data['forum_rules_bitfield'], $forum_data['forum_rules_options']);

					// decode...
					$forum_rules_data = generate_text_for_edit($forum_data['forum_rules'], $forum_data['forum_rules_uid'], $forum_data['forum_rules_options']);
				}

				// Parse desciption if specified
				if ($forum_data['forum_desc'])
				{
					if (!isset($forum_data['forum_desc_uid']))
					{
						// Before we are able to display the preview and plane text, we need to parse our request_var()'d value...
						$forum_data['forum_desc_uid'] = '';
						$forum_data['forum_desc_bitfield'] = '';
						$forum_data['forum_desc_options'] = 0;

						generate_text_for_storage($forum_data['forum_desc'], $forum_data['forum_desc_uid'], $forum_data['forum_desc_bitfield'], $forum_data['forum_desc_options'], request_var('desc_allow_bbcode', false), request_var('desc_allow_urls', false), request_var('desc_allow_smilies', false));
					}

					// decode...
					$forum_desc_data = generate_text_for_edit($forum_data['forum_desc'], $forum_data['forum_desc_uid'], $forum_data['forum_desc_options']);
				}

				$forum_type_options = '';
				$forum_type_ary = array(FORUM_CAT => 'CAT', FORUM_POST => 'FORUM', FORUM_LINK => 'LINK');
		
				foreach ($forum_type_ary as $value => $lang)
				{
					$forum_type_options .= '<option value="' . $value . '"' . (($value == $forum_data['forum_type']) ? ' selected="selected"' : '') . '>' . $user->lang['TYPE_' . $lang] . '</option>';
				}

				$styles_list = style_select($forum_data['forum_style'], true);

				$statuslist = '<option value="' . ITEM_UNLOCKED . '"' . (($forum_data['forum_status'] == ITEM_UNLOCKED) ? ' selected="selected"' : '') . '>' . $user->lang['UNLOCKED'] . '</option><option value="' . ITEM_LOCKED . '"' . (($forum_data['forum_status'] == ITEM_LOCKED) ? ' selected="selected"' : '') . '>' . $user->lang['LOCKED'] . '</option>';

				$sql = 'SELECT forum_id
					FROM ' . FORUMS_TABLE . '
					WHERE forum_type = ' . FORUM_POST . "
						AND forum_id <> $forum_id";
				$result = $db->sql_query($sql);

				if ($db->sql_fetchrow($result))
				{
					$template->assign_vars(array(
						'S_MOVE_FORUM_OPTIONS'		=> make_forum_select($forum_data['parent_id'], $forum_id, false, true, false))
					);
				}
				$db->sql_freeresult($result);

				// Subforum move options
				if ($action == 'edit' && $forum_data['forum_type'] == FORUM_CAT)
				{
					$subforums_id = array();
					$subforums = get_forum_branch($forum_id, 'children');

					foreach ($subforums as $row)
					{
						$subforums_id[] = $row['forum_id'];
					}

					$forums_list = make_forum_select($forum_data['parent_id'], $subforums_id);

					$sql = 'SELECT forum_id
						FROM ' . FORUMS_TABLE . '
						WHERE forum_type = ' . FORUM_POST . "
							AND forum_id <> $forum_id";
					$result = $db->sql_query($sql);

					if ($db->sql_fetchrow($result))
					{
						$template->assign_vars(array(
							'S_MOVE_FORUM_OPTIONS'		=> make_forum_select($forum_data['parent_id'], $subforums_id)) // , false, true, false???
						);
					}
					$db->sql_freeresult($result);

					$template->assign_vars(array(
						'S_HAS_SUBFORUMS'		=> ($forum_data['right_id'] - $forum_data['left_id'] > 1) ? true : false,
						'S_FORUMS_LIST'			=> $forums_list)
					);
				}

				$s_show_display_on_index = false;

				if ($forum_data['parent_id'] > 0)
				{
					// if this forum is a subforum put the "display on index" checkbox
					if ($parent_info = $this->get_forum_info($forum_data['parent_id']))
					{
						if ($parent_info['parent_id'] > 0 || $parent_info['forum_type'] == FORUM_CAT)
						{
							$s_show_display_on_index = true;
						}
					}
				}
				
				if (strlen($forum_data['forum_password']) == 32)
				{
					$errors[] = $user->lang['FORUM_PASSWORD_OLD'];
				}

				$template->assign_vars(array(
					'S_EDIT_FORUM'		=> true,
					'S_ERROR'			=> (sizeof($errors)) ? true : false,
					'S_PARENT_ID'		=> $this->parent_id,
					'S_FORUM_PARENT_ID'	=> $forum_data['parent_id'],
					'S_ADD_ACTION'		=> ($action == 'add') ? true : false,

					'U_BACK'		=> $this->u_action . '&amp;parent_id=' . $this->parent_id,
					'U_EDIT_ACTION'	=> $this->u_action . "&amp;parent_id={$this->parent_id}&amp;action=$action&amp;f=$forum_id",

					'L_COPY_PERMISSIONS_EXPLAIN'	=> $user->lang['COPY_PERMISSIONS_' . strtoupper($action) . '_EXPLAIN'],
					'L_TITLE'						=> $user->lang[$this->page_title],
					'ERROR_MSG'						=> (sizeof($errors)) ? implode('<br />', $errors) : '',

					'FORUM_NAME'				=> $forum_data['forum_name'],
					'FORUM_DATA_LINK'			=> $forum_data['forum_link'],
					'FORUM_IMAGE'				=> $forum_data['forum_image'],
					'FORUM_IMAGE_SRC'			=> ($forum_data['forum_image']) ? $phpbb_root_path . $forum_data['forum_image'] : '',
					'FORUM_POST'				=> FORUM_POST,
					'FORUM_LINK'				=> FORUM_LINK,
					'FORUM_CAT'					=> FORUM_CAT,
					'PRUNE_FREQ'				=> $forum_data['prune_freq'],
					'PRUNE_DAYS'				=> $forum_data['prune_days'],
					'PRUNE_VIEWED'				=> $forum_data['prune_viewed'],
					'TOPICS_PER_PAGE'			=> $forum_data['forum_topics_per_page'],
					'FORUM_RULES_LINK'			=> $forum_data['forum_rules_link'],
					'FORUM_RULES'				=> $forum_data['forum_rules'],
					'FORUM_RULES_PREVIEW'		=> $forum_rules_preview,
					'FORUM_RULES_PLAIN'			=> $forum_rules_data['text'],
					'S_BBCODE_CHECKED'			=> ($forum_rules_data['allow_bbcode']) ? true : false,
					'S_SMILIES_CHECKED'			=> ($forum_rules_data['allow_smilies']) ? true : false,
					'S_URLS_CHECKED'			=> ($forum_rules_data['allow_urls']) ? true : false,
					'S_FORUM_PASSWORD_SET'		=> (empty($forum_data['forum_password'])) ? false : true,

					'FORUM_DESC'				=> $forum_desc_data['text'],
					'S_DESC_BBCODE_CHECKED'		=> ($forum_desc_data['allow_bbcode']) ? true : false,
					'S_DESC_SMILIES_CHECKED'	=> ($forum_desc_data['allow_smilies']) ? true : false,
					'S_DESC_URLS_CHECKED'		=> ($forum_desc_data['allow_urls']) ? true : false,

					'S_FORUM_TYPE_OPTIONS'		=> $forum_type_options,
					'S_STATUS_OPTIONS'			=> $statuslist,
					'S_PARENT_OPTIONS'			=> $parents_list,
					'S_STYLES_OPTIONS'			=> $styles_list,
					'S_FORUM_OPTIONS'			=> make_forum_select(($action == 'add') ? $forum_data['parent_id'] : false, ($action == 'edit') ? $forum_data['forum_id'] : false, false, false, false),
					'S_SHOW_DISPLAY_ON_INDEX'	=> $s_show_display_on_index,
					'S_FORUM_POST'				=> ($forum_data['forum_type'] == FORUM_POST) ? true : false,
					'S_FORUM_ORIG_POST'			=> (isset($old_forum_type) && $old_forum_type == FORUM_POST) ? true : false,
					'S_FORUM_ORIG_CAT'			=> (isset($old_forum_type) && $old_forum_type == FORUM_CAT) ? true : false,
					'S_FORUM_ORIG_LINK'			=> (isset($old_forum_type) && $old_forum_type == FORUM_LINK) ? true : false,
					'S_FORUM_LINK'				=> ($forum_data['forum_type'] == FORUM_LINK) ? true : false,
					'S_FORUM_CAT'				=> ($forum_data['forum_type'] == FORUM_CAT) ? true : false,
					'S_ENABLE_INDEXING'			=> ($forum_data['enable_indexing']) ? true : false,
					'S_TOPIC_ICONS'				=> ($forum_data['enable_icons']) ? true : false,
					'S_DISPLAY_SUBFORUM_LIST'	=> ($forum_data['display_subforum_list']) ? true : false,
					'S_DISPLAY_ON_INDEX'		=> ($forum_data['display_on_index']) ? true : false,
					'S_PRUNE_ENABLE'			=> ($forum_data['enable_prune']) ? true : false,
					'S_FORUM_LINK_TRACK'		=> ($forum_data['forum_flags'] & FORUM_FLAG_LINK_TRACK) ? true : false,
					'S_PRUNE_OLD_POLLS'			=> ($forum_data['forum_flags'] & FORUM_FLAG_PRUNE_POLL) ? true : false,
					'S_PRUNE_ANNOUNCE'			=> ($forum_data['forum_flags'] & FORUM_FLAG_PRUNE_ANNOUNCE) ? true : false,
					'S_PRUNE_STICKY'			=> ($forum_data['forum_flags'] & FORUM_FLAG_PRUNE_STICKY) ? true : false,
					'S_DISPLAY_ACTIVE_TOPICS'	=> ($forum_data['forum_flags'] & FORUM_FLAG_ACTIVE_TOPICS) ? true : false,
					'S_ENABLE_POST_REVIEW'		=> ($forum_data['forum_flags'] & FORUM_FLAG_POST_REVIEW) ? true : false,
					'S_CAN_COPY_PERMISSIONS'	=> ($action != 'edit' || empty($forum_id) || ($auth->acl_get('a_fauth') && $auth->acl_get('a_authusers') && $auth->acl_get('a_authgroups') && $auth->acl_get('a_mauth'))) ? true : false,
				));

				return;

			break;

			case 'delete':

				if (!$forum_id)
				{
					trigger_error($user->lang['NO_FORUM'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}

				$forum_data = $this->get_forum_info($forum_id);

				$subforums_id = array();
				$subforums = get_forum_branch($forum_id, 'children');

				foreach ($subforums as $row)
				{
					$subforums_id[] = $row['forum_id'];
				}

				$forums_list = make_forum_select($forum_data['parent_id'], $subforums_id);

				$sql = 'SELECT forum_id
					FROM ' . FORUMS_TABLE . '
					WHERE forum_type = ' . FORUM_POST . "
						AND forum_id <> $forum_id";
				$result = $db->sql_query($sql);

				if ($db->sql_fetchrow($result))
				{
					$template->assign_vars(array(
						'S_MOVE_FORUM_OPTIONS'		=> make_forum_select($forum_data['parent_id'], $subforums_id, false, true)) // , false, true, false???
					);
				}
				$db->sql_freeresult($result);

				$parent_id = ($this->parent_id == $forum_id) ? 0 : $this->parent_id;

				$template->assign_vars(array(
					'S_DELETE_FORUM'		=> true,
					'U_ACTION'				=> $this->u_action . "&amp;parent_id={$parent_id}&amp;action=delete&amp;f=$forum_id",
					'U_BACK'				=> $this->u_action . '&amp;parent_id=' . $this->parent_id,

					'FORUM_NAME'			=> $forum_data['forum_name'],
					'S_FORUM_POST'			=> ($forum_data['forum_type'] == FORUM_POST) ? true : false,
					'S_FORUM_LINK'			=> ($forum_data['forum_type'] == FORUM_LINK) ? true : false,
					'S_HAS_SUBFORUMS'		=> ($forum_data['right_id'] - $forum_data['left_id'] > 1) ? true : false,
					'S_FORUMS_LIST'			=> $forums_list,
					'S_ERROR'				=> (sizeof($errors)) ? true : false,
					'ERROR_MSG'				=> (sizeof($errors)) ? implode('<br />', $errors) : '')
				);

				return;
			break;
		}

		// Default management page
		if (!$this->parent_id)
		{
			$navigation = $user->lang['FORUM_INDEX'];
		}
		else
		{
			$navigation = '<a href="' . $this->u_action . '">' . $user->lang['FORUM_INDEX'] . '</a>';

			$forums_nav = get_forum_branch($this->parent_id, 'parents', 'descending');
			foreach ($forums_nav as $row)
			{
				if ($row['forum_id'] == $this->parent_id)
				{
					$navigation .= ' -&gt; ' . $row['forum_name'];
				}
				else
				{
					$navigation .= ' -&gt; <a href="' . $this->u_action . '&amp;parent_id=' . $row['forum_id'] . '">' . $row['forum_name'] . '</a>';
				}
			}
		}

		// Jumpbox
		$forum_box = make_forum_select($this->parent_id, false, false, false, false); //make_forum_select($this->parent_id);

		if ($action == 'sync' || $action == 'sync_forum')
		{
			$template->assign_var('S_RESYNCED', true);
		}

		$sql = 'SELECT *
			FROM ' . FORUMS_TABLE . "
			WHERE parent_id = $this->parent_id
			ORDER BY left_id";
		$result = $db->sql_query($sql);

		if ($row = $db->sql_fetchrow($result))
		{
			do
			{
				$forum_type = $row['forum_type'];

				if ($row['forum_status'] == ITEM_LOCKED)
				{
					$folder_image = '<img src="images/icon_folder_lock.gif" alt="' . $user->lang['LOCKED'] . '" />';
				}
				else
				{
					switch ($forum_type)
					{
						case FORUM_LINK:
							$folder_image = '<img src="images/icon_folder_link.gif" alt="' . $user->lang['LINK'] . '" />';
						break;

						default:
							$folder_image = ($row['left_id'] + 1 != $row['right_id']) ? '<img src="images/icon_subfolder.gif" alt="' . $user->lang['SUBFORUM'] . '" />' : '<img src="images/icon_folder.gif" alt="' . $user->lang['FOLDER'] . '" />';
						break;
					}
				}

				$url = $this->u_action . "&amp;parent_id=$this->parent_id&amp;f={$row['forum_id']}";

				$forum_title = ($forum_type != FORUM_LINK) ? '<a href="' . $this->u_action . '&amp;parent_id=' . $row['forum_id'] . '">' : '';
				$forum_title .= $row['forum_name'];
				$forum_title .= ($forum_type != FORUM_LINK) ? '</a>' : '';

				$template->assign_block_vars('forums', array(
					'FOLDER_IMAGE'		=> $folder_image,
					'FORUM_IMAGE'		=> ($row['forum_image']) ? '<img src="' . $phpbb_root_path . $row['forum_image'] . '" alt="" />' : '',
					'FORUM_IMAGE_SRC'	=> ($row['forum_image']) ? $phpbb_root_path . $row['forum_image'] : '',
					'FORUM_NAME'		=> $row['forum_name'],
					'FORUM_DESCRIPTION'	=> generate_text_for_display($row['forum_desc'], $row['forum_desc_uid'], $row['forum_desc_bitfield'], $row['forum_desc_options']),
					'FORUM_TOPICS'		=> $row['forum_topics'],
					'FORUM_POSTS'		=> $row['forum_posts'],

					'S_FORUM_LINK'		=> ($forum_type == FORUM_LINK) ? true : false,
					'S_FORUM_POST'		=> ($forum_type == FORUM_POST) ? true : false,

					'U_FORUM'			=> $this->u_action . '&amp;parent_id=' . $row['forum_id'],
					'U_MOVE_UP'			=> $url . '&amp;action=move_up',
					'U_MOVE_DOWN'		=> $url . '&amp;action=move_down',
					'U_EDIT'			=> $url . '&amp;action=edit',
					'U_DELETE'			=> $url . '&amp;action=delete',
					'U_SYNC'			=> $url . '&amp;action=sync')
				);
			}
			while ($row = $db->sql_fetchrow($result));
		}
		else if ($this->parent_id)
		{
			$row = $this->get_forum_info($this->parent_id);

			$url = $this->u_action . '&amp;parent_id=' . $this->parent_id . '&amp;f=' . $row['forum_id'];

			$template->assign_vars(array(
				'S_NO_FORUMS'		=> true,

				'U_EDIT'			=> $url . '&amp;action=edit',
				'U_DELETE'			=> $url . '&amp;action=delete',
				'U_SYNC'			=> $url . '&amp;action=sync')
			);
		}
		$db->sql_freeresult($result);

		$template->assign_vars(array(
			'ERROR_MSG'		=> (sizeof($errors)) ? implode('<br />', $errors) : '',
			'NAVIGATION'	=> $navigation,
			'FORUM_BOX'		=> $forum_box,
			'U_SEL_ACTION'	=> $this->u_action,
			'U_ACTION'		=> $this->u_action . '&amp;parent_id=' . $this->parent_id,

			'U_PROGRESS_BAR'	=> $this->u_action . '&amp;action=progress_bar',
			'UA_PROGRESS_BAR'	=> addslashes($this->u_action . '&amp;action=progress_bar'),
		));
	}

	/**
	* Get forum details
	*/
	function get_forum_info($forum_id)
	{
		global $db;

		$sql = 'SELECT *
			FROM ' . FORUMS_TABLE . "
			WHERE forum_id = $forum_id";
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if (!$row)
		{
			trigger_error("Forum #$forum_id does not exist", E_USER_ERROR);
		}

		return $row;
	}

	/**
	* Update forum data
	*/
	function update_forum_data(&$forum_data)
	{
		global $db, $user, $cache;

		$errors = array();

		if (!$forum_data['forum_name'])
		{
			$errors[] = $user->lang['FORUM_NAME_EMPTY'];
		}

		if (utf8_strlen($forum_data['forum_desc']) > 4000)
		{
			$errors[] = $user->lang['FORUM_DESC_TOO_LONG'];
		}

		if (utf8_strlen($forum_data['forum_rules']) > 4000)
		{
			$errors[] = $user->lang['FORUM_RULES_TOO_LONG'];
		}

		if ($forum_data['forum_password'] || $forum_data['forum_password_confirm'])
		{
			if ($forum_data['forum_password'] != $forum_data['forum_password_confirm'])
			{
				$forum_data['forum_password'] = $forum_data['forum_password_confirm'] = '';
				$errors[] = $user->lang['FORUM_PASSWORD_MISMATCH'];
			}
		}

		if ($forum_data['prune_days'] < 0 || $forum_data['prune_viewed'] < 0 || $forum_data['prune_freq'] < 0)
		{
			$forum_data['prune_days'] = $forum_data['prune_viewed'] = $forum_data['prune_freq'] = 0;
			$errors[] = $user->lang['FORUM_DATA_NEGATIVE'];
		}
		
		$range_test_ary = array(
			array('lang' => 'FORUM_TOPICS_PAGE', 'value' => $forum_data['forum_topics_per_page'], 'column_type' => 'TINT:0'),
		);
		validate_range($range_test_ary, $errors);



		// Set forum flags
		// 1 = link tracking
		// 2 = prune old polls
		// 4 = prune announcements
		// 8 = prune stickies
		// 16 = show active topics
		// 32 = enable post review
		$forum_data['forum_flags'] = 0;
		$forum_data['forum_flags'] += ($forum_data['forum_link_track']) ? FORUM_FLAG_LINK_TRACK : 0;
		$forum_data['forum_flags'] += ($forum_data['prune_old_polls']) ? FORUM_FLAG_PRUNE_POLL : 0;
		$forum_data['forum_flags'] += ($forum_data['prune_announce']) ? FORUM_FLAG_PRUNE_ANNOUNCE : 0;
		$forum_data['forum_flags'] += ($forum_data['prune_sticky']) ? FORUM_FLAG_PRUNE_STICKY : 0;
		$forum_data['forum_flags'] += ($forum_data['show_active']) ? FORUM_FLAG_ACTIVE_TOPICS : 0;
		$forum_data['forum_flags'] += ($forum_data['enable_post_review']) ? FORUM_FLAG_POST_REVIEW : 0;

		// Unset data that are not database fields
		$forum_data_sql = $forum_data;

		unset($forum_data_sql['forum_link_track']);
		unset($forum_data_sql['prune_old_polls']);
		unset($forum_data_sql['prune_announce']);
		unset($forum_data_sql['prune_sticky']);
		unset($forum_data_sql['show_active']);
		unset($forum_data_sql['enable_post_review']);
		unset($forum_data_sql['forum_password_confirm']);

		// What are we going to do tonight Brain? The same thing we do everynight,
		// try to take over the world ... or decide whether to continue update
		// and if so, whether it's a new forum/cat/link or an existing one
		if (sizeof($errors))
		{
			return $errors;
		}

		// As we don't know the old password, it's kinda tricky to detect changes
		if ($forum_data_sql['forum_password_unset'])
		{
			$forum_data_sql['forum_password'] = '';
		}
		else if (empty($forum_data_sql['forum_password']))
		{
			unset($forum_data_sql['forum_password']);
		}
		else
		{
			$forum_data_sql['forum_password'] = phpbb_hash($forum_data_sql['forum_password']);
		}
		unset($forum_data_sql['forum_password_unset']);
		
		if (!isset($forum_data_sql['forum_id']))
		{
			// no forum_id means we're creating a new forum
			unset($forum_data_sql['type_action']);

			if ($forum_data_sql['parent_id'])
			{
				$sql = 'SELECT left_id, right_id, forum_type
					FROM ' . FORUMS_TABLE . '
					WHERE forum_id = ' . $forum_data_sql['parent_id'];
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if (!$row)
				{
					trigger_error($user->lang['PARENT_NOT_EXIST'] . adm_back_link($this->u_action . '&amp;' . $this->parent_id), E_USER_WARNING);
				}

				if ($row['forum_type'] == FORUM_LINK)
				{
					$errors[] = $user->lang['PARENT_IS_LINK_FORUM'];
					return $errors;
				}

				$sql = 'UPDATE ' . FORUMS_TABLE . '
					SET left_id = left_id + 2, right_id = right_id + 2
					WHERE left_id > ' . $row['right_id'];
				$db->sql_query($sql);

				$sql = 'UPDATE ' . FORUMS_TABLE . '
					SET right_id = right_id + 2
					WHERE ' . $row['left_id'] . ' BETWEEN left_id AND right_id';
				$db->sql_query($sql);

				$forum_data_sql['left_id'] = $row['right_id'];
				$forum_data_sql['right_id'] = $row['right_id'] + 1;
			}
			else
			{
				$sql = 'SELECT MAX(right_id) AS right_id
					FROM ' . FORUMS_TABLE;
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				$forum_data_sql['left_id'] = $row['right_id'] + 1;
				$forum_data_sql['right_id'] = $row['right_id'] + 2;
			}

			$sql = 'INSERT INTO ' . FORUMS_TABLE . ' ' . $db->sql_build_array('INSERT', $forum_data_sql);
			$db->sql_query($sql);

			$forum_data['forum_id'] = $db->sql_nextid();

			add_log('admin', 'LOG_FORUM_ADD', $forum_data['forum_name']);
		}
		else
		{
			$row = $this->get_forum_info($forum_data_sql['forum_id']);

			if ($row['forum_type'] == FORUM_POST && $row['forum_type'] != $forum_data_sql['forum_type'])
			{
				// Has subforums and want to change into a link?
				if ($row['right_id'] - $row['left_id'] > 1 && $forum_data_sql['forum_type'] == FORUM_LINK)
				{
					$errors[] = $user->lang['FORUM_WITH_SUBFORUMS_NOT_TO_LINK'];
					return $errors;
				}

				// we're turning a postable forum into a non-postable forum
				if ($forum_data_sql['type_action'] == 'move')
				{
					$to_forum_id = request_var('to_forum_id', 0);

					if ($to_forum_id)
					{
						$errors = $this->move_forum_content($forum_data_sql['forum_id'], $to_forum_id);
					}
					else
					{
						return array($user->lang['NO_DESTINATION_FORUM']);
					}
				}
				else if ($forum_data_sql['type_action'] == 'delete')
				{
					$errors = $this->delete_forum_content($forum_data_sql['forum_id']);
				}
				else
				{
					return array($user->lang['NO_FORUM_ACTION']);
				}

				$forum_data_sql['forum_posts'] = $forum_data_sql['forum_topics'] = $forum_data_sql['forum_topics_real'] = $forum_data_sql['forum_last_post_id'] = $forum_data_sql['forum_last_poster_id'] = $forum_data_sql['forum_last_post_time'] = 0;
				$forum_data_sql['forum_last_poster_name'] = $forum_data_sql['forum_last_poster_colour'] = '';
			}
			else if ($row['forum_type'] == FORUM_CAT && $forum_data_sql['forum_type'] == FORUM_LINK)
			{
				// Has subforums?
				if ($row['right_id'] - $row['left_id'] > 1)
				{
					// We are turning a category into a link - but need to decide what to do with the subforums.
					$action_subforums = request_var('action_subforums', '');
					$subforums_to_id = request_var('subforums_to_id', 0);

					if ($action_subforums == 'delete')
					{
						$rows = get_forum_branch($row['forum_id'], 'children', 'descending', false);

						foreach ($rows as $_row)
						{
							// Do not remove the forum id we are about to change. ;)
							if ($_row['forum_id'] == $row['forum_id'])
							{
								continue;
							}

							$forum_ids[] = $_row['forum_id'];
							$errors = array_merge($errors, $this->delete_forum_content($_row['forum_id']));
						}

						if (sizeof($errors))
						{
							return $errors;
						}

						if (sizeof($forum_ids))
						{
							$sql = 'DELETE FROM ' . FORUMS_TABLE . '
								WHERE ' . $db->sql_in_set('forum_id', $forum_ids);
							$db->sql_query($sql);

							$sql = 'DELETE FROM ' . ACL_GROUPS_TABLE . '
								WHERE ' . $db->sql_in_set('forum_id', $forum_ids);
							$db->sql_query($sql);

							$sql = 'DELETE FROM ' . ACL_USERS_TABLE . '
								WHERE ' . $db->sql_in_set('forum_id', $forum_ids);
							$db->sql_query($sql);

							// Delete forum ids from extension groups table
							$sql = 'SELECT group_id, allowed_forums
								FROM ' . EXTENSION_GROUPS_TABLE;
							$result = $db->sql_query($sql);

							while ($_row = $db->sql_fetchrow($result))
							{
								if (!$_row['allowed_forums'])
								{
									continue;
								}

								$allowed_forums = unserialize(trim($_row['allowed_forums']));
								$allowed_forums = array_diff($allowed_forums, $forum_ids);

								$sql = 'UPDATE ' . EXTENSION_GROUPS_TABLE . "
									SET allowed_forums = '" . ((sizeof($allowed_forums)) ? serialize($allowed_forums) : '') . "'
									WHERE group_id = {$_row['group_id']}";
								$db->sql_query($sql);
							}
							$db->sql_freeresult($result);

							$cache->destroy('_extensions');
						}
					}
					else if ($action_subforums == 'move')
					{
						if (!$subforums_to_id)
						{
							return array($user->lang['NO_DESTINATION_FORUM']);
						}

						$sql = 'SELECT forum_name
							FROM ' . FORUMS_TABLE . '
							WHERE forum_id = ' . $subforums_to_id;
						$result = $db->sql_query($sql);
						$_row = $db->sql_fetchrow($result);
						$db->sql_freeresult($result);

						if (!$_row)
						{
							return array($user->lang['NO_FORUM']);
						}

						$subforums_to_name = $_row['forum_name'];

						$sql = 'SELECT forum_id
							FROM ' . FORUMS_TABLE . "
							WHERE parent_id = {$row['forum_id']}";
						$result = $db->sql_query($sql);

						while ($_row = $db->sql_fetchrow($result))
						{
							$this->move_forum($_row['forum_id'], $subforums_to_id);
						}
						$db->sql_freeresult($result);

						$sql = 'UPDATE ' . FORUMS_TABLE . "
							SET parent_id = $subforums_to_id
							WHERE parent_id = {$row['forum_id']}";
						$db->sql_query($sql);
					}

					// Adjust the left/right id
					$sql = 'UPDATE ' . FORUMS_TABLE . '
						SET right_id = left_id + 1
						WHERE forum_id = ' . $row['forum_id'];
					$db->sql_query($sql);
				}
			}
			else if ($row['forum_type'] == FORUM_CAT && $forum_data_sql['forum_type'] == FORUM_POST)
			{
				// Changing a category to a forum? Reset the data (you can't post directly in a cat, you must use a forum)
				$forum_data_sql['forum_posts'] = 0;
				$forum_data_sql['forum_topics'] = 0;
				$forum_data_sql['forum_topics_real'] = 0;
				$forum_data_sql['forum_last_post_id'] = 0;
				$forum_data_sql['forum_last_post_subject'] = '';
				$forum_data_sql['forum_last_post_time'] = 0;
				$forum_data_sql['forum_last_poster_id'] = 0;
				$forum_data_sql['forum_last_poster_name'] = '';
				$forum_data_sql['forum_last_poster_colour'] = '';
			}

			if (sizeof($errors))
			{
				return $errors;
			}

			if ($row['parent_id'] != $forum_data_sql['parent_id'])
			{
				$errors = $this->move_forum($forum_data_sql['forum_id'], $forum_data_sql['parent_id']);
			}

			if (sizeof($errors))
			{
				return $errors;
			}

			unset($forum_data_sql['type_action']);

			if ($row['forum_name'] != $forum_data_sql['forum_name'])
			{
				// the forum name has changed, clear the parents list of all forums (for safety)
				$sql = 'UPDATE ' . FORUMS_TABLE . "
					SET forum_parents = ''";
				$db->sql_query($sql);
			}

			// Setting the forum id to the forum id is not really received well by some dbs. ;)
			$forum_id = $forum_data_sql['forum_id'];
			unset($forum_data_sql['forum_id']);

			$sql = 'UPDATE ' . FORUMS_TABLE . '
				SET ' . $db->sql_build_array('UPDATE', $forum_data_sql) . '
				WHERE forum_id = ' . $forum_id;
			$db->sql_query($sql);

			// Add it back
			$forum_data['forum_id'] = $forum_id;

			add_log('admin', 'LOG_FORUM_EDIT', $forum_data['forum_name']);
		}

		return $errors;
	}

	/**
	* Move forum
	*/
	function move_forum($from_id, $to_id)
	{
		global $db, $user;

		$to_data = $moved_ids = $errors = array();

		// Check if we want to move to a parent with link type
		if ($to_id > 0)
		{
			$to_data = $this->get_forum_info($to_id);

			if ($to_data['forum_type'] == FORUM_LINK)
			{
				$errors[] = $user->lang['PARENT_IS_LINK_FORUM'];
				return $errors;
			}
		}

		$moved_forums = get_forum_branch($from_id, 'children', 'descending');
		$from_data = $moved_forums[0];
		$diff = sizeof($moved_forums) * 2;

		$moved_ids = array();
		for ($i = 0; $i < sizeof($moved_forums); ++$i)
		{
			$moved_ids[] = $moved_forums[$i]['forum_id'];
		}

		// Resync parents
		$sql = 'UPDATE ' . FORUMS_TABLE . "
			SET right_id = right_id - $diff, forum_parents = ''
			WHERE left_id < " . $from_data['right_id'] . "
				AND right_id > " . $from_data['right_id'];
		$db->sql_query($sql);

		// Resync righthand side of tree
		$sql = 'UPDATE ' . FORUMS_TABLE . "
			SET left_id = left_id - $diff, right_id = right_id - $diff, forum_parents = ''
			WHERE left_id > " . $from_data['right_id'];
		$db->sql_query($sql);

		if ($to_id > 0)
		{
			// Retrieve $to_data again, it may have been changed...
			$to_data = $this->get_forum_info($to_id);

			// Resync new parents
			$sql = 'UPDATE ' . FORUMS_TABLE . "
				SET right_id = right_id + $diff, forum_parents = ''
				WHERE " . $to_data['right_id'] . ' BETWEEN left_id AND right_id
					AND ' . $db->sql_in_set('forum_id', $moved_ids, true);
			$db->sql_query($sql);

			// Resync the righthand side of the tree
			$sql = 'UPDATE ' . FORUMS_TABLE . "
				SET left_id = left_id + $diff, right_id = right_id + $diff, forum_parents = ''
				WHERE left_id > " . $to_data['right_id'] . '
					AND ' . $db->sql_in_set('forum_id', $moved_ids, true);
			$db->sql_query($sql);

			// Resync moved branch
			$to_data['right_id'] += $diff;

			if ($to_data['right_id'] > $from_data['right_id'])
			{
				$diff = '+ ' . ($to_data['right_id'] - $from_data['right_id'] - 1);
			}
			else
			{
				$diff = '- ' . abs($to_data['right_id'] - $from_data['right_id'] - 1);
			}
		}
		else
		{
			$sql = 'SELECT MAX(right_id) AS right_id
				FROM ' . FORUMS_TABLE . '
				WHERE ' . $db->sql_in_set('forum_id', $moved_ids, true);
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			$diff = '+ ' . ($row['right_id'] - $from_data['left_id'] + 1);
		}

		$sql = 'UPDATE ' . FORUMS_TABLE . "
			SET left_id = left_id $diff, right_id = right_id $diff, forum_parents = ''
			WHERE " . $db->sql_in_set('forum_id', $moved_ids);
		$db->sql_query($sql);

		return $errors;
	}

	/**
	* Move forum content from one to another forum
	*/
	function move_forum_content($from_id, $to_id, $sync = true)
	{
		global $db;

		$table_ary = array(LOG_TABLE, POSTS_TABLE, TOPICS_TABLE, DRAFTS_TABLE, TOPICS_TRACK_TABLE);

		foreach ($table_ary as $table)
		{
			$sql = "UPDATE $table
				SET forum_id = $to_id
				WHERE forum_id = $from_id";
			$db->sql_query($sql);
		}
		unset($table_ary);

		$table_ary = array(FORUMS_ACCESS_TABLE, FORUMS_TRACK_TABLE, FORUMS_WATCH_TABLE, MODERATOR_CACHE_TABLE);

		foreach ($table_ary as $table)
		{
			$sql = "DELETE FROM $table
				WHERE forum_id = $from_id";
			$db->sql_query($sql);
		}

		if ($sync)
		{
			// Delete ghost topics that link back to the same forum then resync counters
			sync('topic_moved');
			sync('forum', 'forum_id', $to_id, false, true);
		}

		return array();
	}

	/**
	* Remove complete forum
	*/
	function delete_forum($forum_id, $action_posts = 'delete', $action_subforums = 'delete', $posts_to_id = 0, $subforums_to_id = 0)
	{
		global $db, $user, $cache;

		$forum_data = $this->get_forum_info($forum_id);

		$errors = array();
		$log_action_posts = $log_action_forums = $posts_to_name = $subforums_to_name = '';
		$forum_ids = array($forum_id);

		if ($action_posts == 'delete')
		{
			$log_action_posts = 'POSTS';
			$errors = array_merge($errors, $this->delete_forum_content($forum_id));
		}
		else if ($action_posts == 'move')
		{
			if (!$posts_to_id)
			{
				$errors[] = $user->lang['NO_DESTINATION_FORUM'];
			}
			else
			{
				$log_action_posts = 'MOVE_POSTS';

				$sql = 'SELECT forum_name
					FROM ' . FORUMS_TABLE . '
					WHERE forum_id = ' . $posts_to_id;
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if (!$row)
				{
					$errors[] = $user->lang['NO_FORUM'];
				}
				else
				{
					$posts_to_name = $row['forum_name'];
					$errors = array_merge($errors, $this->move_forum_content($forum_id, $posts_to_id));
				}
			}
		}

		if (sizeof($errors))
		{
			return $errors;
		}

		if ($action_subforums == 'delete')
		{
			$log_action_forums = 'FORUMS';
			$rows = get_forum_branch($forum_id, 'children', 'descending', false);

			foreach ($rows as $row)
			{
				$forum_ids[] = $row['forum_id'];
				$errors = array_merge($errors, $this->delete_forum_content($row['forum_id']));
			}

			if (sizeof($errors))
			{
				return $errors;
			}

			$diff = sizeof($forum_ids) * 2;

			$sql = 'DELETE FROM ' . FORUMS_TABLE . '
				WHERE ' . $db->sql_in_set('forum_id', $forum_ids);
			$db->sql_query($sql);

			$sql = 'DELETE FROM ' . ACL_GROUPS_TABLE . '
				WHERE ' . $db->sql_in_set('forum_id', $forum_ids);
			$db->sql_query($sql);

			$sql = 'DELETE FROM ' . ACL_USERS_TABLE . '
				WHERE ' . $db->sql_in_set('forum_id', $forum_ids);
			$db->sql_query($sql);
		}
		else if ($action_subforums == 'move')
		{
			if (!$subforums_to_id)
			{
				$errors[] = $user->lang['NO_DESTINATION_FORUM'];
			}
			else
			{
				$log_action_forums = 'MOVE_FORUMS';

				$sql = 'SELECT forum_name
					FROM ' . FORUMS_TABLE . '
					WHERE forum_id = ' . $subforums_to_id;
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if (!$row)
				{
					$errors[] = $user->lang['NO_FORUM'];
				}
				else
				{
					$subforums_to_name = $row['forum_name'];

					$sql = 'SELECT forum_id
						FROM ' . FORUMS_TABLE . "
						WHERE parent_id = $forum_id";
					$result = $db->sql_query($sql);

					while ($row = $db->sql_fetchrow($result))
					{
						$this->move_forum($row['forum_id'], $subforums_to_id);
					}
					$db->sql_freeresult($result);

					// Grab new forum data for correct tree updating later
					$forum_data = $this->get_forum_info($forum_id);

					$sql = 'UPDATE ' . FORUMS_TABLE . "
						SET parent_id = $subforums_to_id
						WHERE parent_id = $forum_id";
					$db->sql_query($sql);

					$diff = 2;
					$sql = 'DELETE FROM ' . FORUMS_TABLE . "
						WHERE forum_id = $forum_id";
					$db->sql_query($sql);

					$sql = 'DELETE FROM ' . ACL_GROUPS_TABLE . "
						WHERE forum_id = $forum_id";
					$db->sql_query($sql);

					$sql = 'DELETE FROM ' . ACL_USERS_TABLE . "
						WHERE forum_id = $forum_id";
					$db->sql_query($sql);
				}
			}

			if (sizeof($errors))
			{
				return $errors;
			}
		}
		else
		{
			$diff = 2;
			$sql = 'DELETE FROM ' . FORUMS_TABLE . "
				WHERE forum_id = $forum_id";
			$db->sql_query($sql);

			$sql = 'DELETE FROM ' . ACL_GROUPS_TABLE . "
				WHERE forum_id = $forum_id";
			$db->sql_query($sql);

			$sql = 'DELETE FROM ' . ACL_USERS_TABLE . "
				WHERE forum_id = $forum_id";
			$db->sql_query($sql);
		}

		// Resync tree
		$sql = 'UPDATE ' . FORUMS_TABLE . "
			SET right_id = right_id - $diff
			WHERE left_id < {$forum_data['right_id']} AND right_id > {$forum_data['right_id']}";
		$db->sql_query($sql);

		$sql = 'UPDATE ' . FORUMS_TABLE . "
			SET left_id = left_id - $diff, right_id = right_id - $diff
			WHERE left_id > {$forum_data['right_id']}";
		$db->sql_query($sql);

		// Delete forum ids from extension groups table
		$sql = 'SELECT group_id, allowed_forums
			FROM ' . EXTENSION_GROUPS_TABLE;
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			if (!$row['allowed_forums'])
			{
				continue;
			}

			$allowed_forums = unserialize(trim($row['allowed_forums']));
			$allowed_forums = array_diff($allowed_forums, $forum_ids);

			$sql = 'UPDATE ' . EXTENSION_GROUPS_TABLE . "
				SET allowed_forums = '" . ((sizeof($allowed_forums)) ? serialize($allowed_forums) : '') . "'
				WHERE group_id = {$row['group_id']}";
			$db->sql_query($sql);
		}
		$db->sql_freeresult($result);

		$cache->destroy('_extensions');

		$log_action = implode('_', array($log_action_posts, $log_action_forums));

		switch ($log_action)
		{
			case 'MOVE_POSTS_MOVE_FORUMS':
				add_log('admin', 'LOG_FORUM_DEL_MOVE_POSTS_MOVE_FORUMS', $posts_to_name, $subforums_to_name, $forum_data['forum_name']);
			break;

			case 'MOVE_POSTS_FORUMS':
				add_log('admin', 'LOG_FORUM_DEL_MOVE_POSTS_FORUMS', $posts_to_name, $forum_data['forum_name']);
			break;

			case 'POSTS_MOVE_FORUMS':
				add_log('admin', 'LOG_FORUM_DEL_POSTS_MOVE_FORUMS', $subforums_to_name, $forum_data['forum_name']);
			break;

			case '_MOVE_FORUMS':
				add_log('admin', 'LOG_FORUM_DEL_MOVE_FORUMS', $subforums_to_name, $forum_data['forum_name']);
			break;

			case 'MOVE_POSTS_':
				add_log('admin', 'LOG_FORUM_DEL_MOVE_POSTS', $posts_to_name, $forum_data['forum_name']);
			break;

			case 'POSTS_FORUMS':
				add_log('admin', 'LOG_FORUM_DEL_POSTS_FORUMS', $forum_data['forum_name']);
			break;

			case '_FORUMS':
				add_log('admin', 'LOG_FORUM_DEL_FORUMS', $forum_data['forum_name']);
			break;

			case 'POSTS_':
				add_log('admin', 'LOG_FORUM_DEL_POSTS', $forum_data['forum_name']);
			break;

			default:
				add_log('admin', 'LOG_FORUM_DEL_FORUM', $forum_data['forum_name']);
			break;
		}

		return $errors;
	}

	/**
	* Delete forum content
	*/
	function delete_forum_content($forum_id)
	{
		global $db, $config, $phpbb_root_path, $phpEx;

		include_once($phpbb_root_path . 'includes/functions_posting.' . $phpEx);

		$db->sql_transaction('begin');

		// Select then delete all attachments
		$sql = 'SELECT a.topic_id
			FROM ' . POSTS_TABLE . ' p, ' . ATTACHMENTS_TABLE . " a
			WHERE p.forum_id = $forum_id
				AND a.in_message = 0
				AND a.topic_id = p.topic_id";
		$result = $db->sql_query($sql);	

		$topic_ids = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$topic_ids[] = $row['topic_id'];
		}
		$db->sql_freeresult($result);

		delete_attachments('topic', $topic_ids, false);

		// Before we remove anything we make sure we are able to adjust the post counts later. ;)
		$sql = 'SELECT poster_id
			FROM ' . POSTS_TABLE . '
			WHERE forum_id = ' . $forum_id . '
				AND post_postcount = 1';
		$result = $db->sql_query($sql);

		$post_counts = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$post_counts[$row['poster_id']] = (!empty($post_counts[$row['poster_id']])) ? $post_counts[$row['poster_id']] + 1 : 1;
		}
		$db->sql_freeresult($result);

		switch ($db->sql_layer)
		{
			case 'mysql4':
			case 'mysqli':

				// Delete everything else and thank MySQL for offering multi-table deletion
				$tables_ary = array(
					SEARCH_WORDMATCH_TABLE	=> 'post_id',
					REPORTS_TABLE			=> 'post_id',
					WARNINGS_TABLE			=> 'post_id',
					BOOKMARKS_TABLE			=> 'topic_id',
					TOPICS_WATCH_TABLE		=> 'topic_id',
					TOPICS_POSTED_TABLE		=> 'topic_id',
					POLL_OPTIONS_TABLE		=> 'topic_id',
					POLL_VOTES_TABLE		=> 'topic_id',
				);

				$sql = 'DELETE ' . POSTS_TABLE;
				$sql_using = "\nFROM " . POSTS_TABLE;
				$sql_where = "\nWHERE " . POSTS_TABLE . ".forum_id = $forum_id\n";

				foreach ($tables_ary as $table => $field)
				{
					$sql .= ", $table ";
					$sql_using .= ", $table ";
					$sql_where .= "\nAND $table.$field = " . POSTS_TABLE . ".$field";
				}

				$db->sql_query($sql . $sql_using . $sql_where);

			break;

			default:
			
				// Delete everything else and curse your DB for not offering multi-table deletion
				$tables_ary = array(
					'post_id'	=>	array(
						SEARCH_WORDMATCH_TABLE,
						REPORTS_TABLE,
						WARNINGS_TABLE,
					),

					'topic_id'	=>	array(
						BOOKMARKS_TABLE,
						TOPICS_WATCH_TABLE,
						TOPICS_POSTED_TABLE,
						POLL_OPTIONS_TABLE,
						POLL_VOTES_TABLE,
					)
				);

				foreach ($tables_ary as $field => $tables)
				{
					$start = 0;

					do
					{
						$sql = "SELECT $field
							FROM " . POSTS_TABLE . '
							WHERE forum_id = ' . $forum_id;
						$result = $db->sql_query_limit($sql, 500, $start);

						$ids = array();
						while ($row = $db->sql_fetchrow($result))
						{
							$ids[] = $row[$field];
						}
						$db->sql_freeresult($result);

						if (sizeof($ids))
						{
							$start += sizeof($ids);

							foreach ($tables as $table)
							{
								$db->sql_query("DELETE FROM $table WHERE " . $db->sql_in_set($field, $ids));
							}
						}
					}
					while ($row);
				}
				unset($ids);

			break;
		}

		$table_ary = array(FORUMS_ACCESS_TABLE, FORUMS_TRACK_TABLE, FORUMS_WATCH_TABLE, LOG_TABLE, MODERATOR_CACHE_TABLE, POSTS_TABLE, TOPICS_TABLE, TOPICS_TRACK_TABLE);

		foreach ($table_ary as $table)
		{
			$db->sql_query("DELETE FROM $table WHERE forum_id = $forum_id");
		}

		// Set forum ids to 0
		$table_ary = array(DRAFTS_TABLE);

		foreach ($table_ary as $table)
		{
			$db->sql_query("UPDATE $table SET forum_id = 0 WHERE forum_id = $forum_id");
		}

		// Adjust users post counts
		if (sizeof($post_counts))
		{
			foreach ($post_counts as $poster_id => $substract)
			{
				$sql = 'UPDATE ' . USERS_TABLE . '
					SET user_posts = 0
					WHERE user_id = ' . $poster_id . '
					AND user_posts < ' . $substract;
				$db->sql_query($sql);
				$sql = 'UPDATE ' . USERS_TABLE . '
					SET user_posts = user_posts - ' . $substract . '
					WHERE user_id = ' . $poster_id . '
					AND user_posts >= ' . $substract;
				$db->sql_query($sql);
			}
		}

		$db->sql_transaction('commit');

		// Make sure the overall post/topic count is correct...
		$sql = 'SELECT COUNT(post_id) AS stat
			FROM ' . POSTS_TABLE . '
			WHERE post_approved = 1';
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		set_config('num_posts', (int) $row['stat'], true);

		$sql = 'SELECT COUNT(topic_id) AS stat
			FROM ' . TOPICS_TABLE . '
			WHERE topic_approved = 1';
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		set_config('num_topics', (int) $row['stat'], true);

		$sql = 'SELECT COUNT(attach_id) as stat
			FROM ' . ATTACHMENTS_TABLE;
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		set_config('num_files', (int) $row['stat'], true);

		$sql = 'SELECT SUM(filesize) as stat
			FROM ' . ATTACHMENTS_TABLE;
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		set_config('upload_dir_size', (int) $row['stat'], true);

		return array();
	}

	/**
	* Move forum position by $steps up/down
	*/
	function move_forum_by($forum_row, $action = 'move_up', $steps = 1)
	{
		global $db;

		/**
		* Fetch all the siblings between the module's current spot
		* and where we want to move it to. If there are less than $steps
		* siblings between the current spot and the target then the
		* module will move as far as possible
		*/
		$sql = 'SELECT forum_id, forum_name, left_id, right_id
			FROM ' . FORUMS_TABLE . "
			WHERE parent_id = {$forum_row['parent_id']}
				AND " . (($action == 'move_up') ? "right_id < {$forum_row['right_id']} ORDER BY right_id DESC" : "left_id > {$forum_row['left_id']} ORDER BY left_id ASC");
		$result = $db->sql_query_limit($sql, $steps);

		$target = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$target = $row;
		}
		$db->sql_freeresult($result);

		if (!sizeof($target))
		{
			// The forum is already on top or bottom
			return false;
		}

		/**
		* $left_id and $right_id define the scope of the nodes that are affected by the move.
		* $diff_up and $diff_down are the values to substract or add to each node's left_id
		* and right_id in order to move them up or down.
		* $move_up_left and $move_up_right define the scope of the nodes that are moving
		* up. Other nodes in the scope of ($left_id, $right_id) are considered to move down.
		*/
		if ($action == 'move_up')
		{
			$left_id = $target['left_id'];
			$right_id = $forum_row['right_id'];

			$diff_up = $forum_row['left_id'] - $target['left_id'];
			$diff_down = $forum_row['right_id'] + 1 - $forum_row['left_id'];

			$move_up_left = $forum_row['left_id'];
			$move_up_right = $forum_row['right_id'];
		}
		else
		{
			$left_id = $forum_row['left_id'];
			$right_id = $target['right_id'];

			$diff_up = $forum_row['right_id'] + 1 - $forum_row['left_id'];
			$diff_down = $target['right_id'] - $forum_row['right_id'];

			$move_up_left = $forum_row['right_id'] + 1;
			$move_up_right = $target['right_id'];
		}

		// Now do the dirty job
		$sql = 'UPDATE ' . FORUMS_TABLE . "
			SET left_id = left_id + CASE
				WHEN left_id BETWEEN {$move_up_left} AND {$move_up_right} THEN -{$diff_up}
				ELSE {$diff_down}
			END,
			right_id = right_id + CASE
				WHEN right_id BETWEEN {$move_up_left} AND {$move_up_right} THEN -{$diff_up}
				ELSE {$diff_down}
			END,
			forum_parents = ''
			WHERE
				left_id BETWEEN {$left_id} AND {$right_id}
				AND right_id BETWEEN {$left_id} AND {$right_id}";
		$db->sql_query($sql);

		return $target['forum_name'];
	}

	/**
	* Display progress bar for syncinc forums
	*/
	function display_progress_bar($start, $total)
	{
		global $template, $user;

		adm_page_header($user->lang['SYNC_IN_PROGRESS']);

		$template->set_filenames(array(
			'body'	=> 'progress_bar.html')
		);

		$template->assign_vars(array(
			'L_PROGRESS'			=> $user->lang['SYNC_IN_PROGRESS'],
			'L_PROGRESS_EXPLAIN'	=> ($start && $total) ? sprintf($user->lang['SYNC_IN_PROGRESS_EXPLAIN'], $start, $total) : $user->lang['SYNC_IN_PROGRESS'])
		);

		adm_page_footer();
	}
}

?>