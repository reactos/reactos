<?php
/**
*
* @package acp
* @version $Id: functions_admin.php 8479 2008-03-29 00:22:48Z naderman $
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
* Recalculate Binary Tree
function recalc_btree($sql_id, $sql_table, $module_class = '')
{
	global $db;

	if (!$sql_id || !$sql_table)
	{
		return;
	}

	$sql_where = ($module_class) ? " WHERE module_class = '" . $db->sql_escape($module_class) . "'" : '';

	// Reset to minimum possible left and right id
	$sql = "SELECT MIN(left_id) as min_left_id, MIN(right_id) as min_right_id
		FROM $sql_table
		$sql_where";
	$result = $db->sql_query($sql);
	$row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	$substract = (int) (min($row['min_left_id'], $row['min_right_id']) - 1);

	if ($substract > 0)
	{
		$sql = "UPDATE $sql_table
			SET left_id = left_id - $substract, right_id = right_id - $substract
			$sql_where";
		$db->sql_query($sql);
	}

	$sql = "SELECT $sql_id, parent_id, left_id, right_id
		FROM $sql_table
		$sql_where
		ORDER BY left_id ASC, parent_id ASC, $sql_id ASC";
	$f_result = $db->sql_query($sql);

	while ($item_data = $db->sql_fetchrow($f_result))
	{
		if ($item_data['parent_id'])
		{
			$sql = "SELECT left_id, right_id
				FROM $sql_table
				$sql_where " . (($sql_where) ? 'AND' : 'WHERE') . "
					$sql_id = {$item_data['parent_id']}";
			$result = $db->sql_query($sql);

			if (!$row = $db->sql_fetchrow($result))
			{
				$sql = "UPDATE $sql_table SET parent_id = 0 WHERE $sql_id = " . $item_data[$sql_id];
				$db->sql_query($sql);
			}
			$db->sql_freeresult($result);

			$sql = "UPDATE $sql_table
				SET left_id = left_id + 2, right_id = right_id + 2
				$sql_where " . (($sql_where) ? 'AND' : 'WHERE') . "
					left_id > {$row['right_id']}";
			$db->sql_query($sql);

			$sql = "UPDATE $sql_table
				SET right_id = right_id + 2
				$sql_where " . (($sql_where) ? 'AND' : 'WHERE') . "
					{$row['left_id']} BETWEEN left_id AND right_id";
			$db->sql_query($sql);

			$item_data['left_id'] = $row['right_id'];
			$item_data['right_id'] = $row['right_id'] + 1;
		}
		else
		{
			$sql = "SELECT MAX(right_id) AS right_id
				FROM $sql_table
				$sql_where";
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			$item_data['left_id'] = $row['right_id'] + 1;
			$item_data['right_id'] = $row['right_id'] + 2;
		}
	
		$sql = "UPDATE $sql_table
			SET left_id = {$item_data['left_id']}, right_id = {$item_data['right_id']}
			WHERE $sql_id = " . $item_data[$sql_id];
		$db->sql_query($sql);
	}
	$db->sql_freeresult($f_result);
}
*/

/**
* Simple version of jumpbox, just lists authed forums
*/
function make_forum_select($select_id = false, $ignore_id = false, $ignore_acl = false, $ignore_nonpost = false, $ignore_emptycat = true, $only_acl_post = false, $return_array = false)
{
	global $db, $user, $auth;

	$acl = ($ignore_acl) ? '' : (($only_acl_post) ? 'f_post' : array('f_list', 'a_forum', 'a_forumadd', 'a_forumdel'));

	// This query is identical to the jumpbox one
	$sql = 'SELECT forum_id, forum_name, parent_id, forum_type, left_id, right_id
		FROM ' . FORUMS_TABLE . '
		ORDER BY left_id ASC';
	$result = $db->sql_query($sql, 600);

	$right = 0;
	$padding_store = array('0' => '');
	$padding = '';
	$forum_list = ($return_array) ? array() : '';

	// Sometimes it could happen that forums will be displayed here not be displayed within the index page
	// This is the result of forums not displayed at index, having list permissions and a parent of a forum with no permissions.
	// If this happens, the padding could be "broken"

	while ($row = $db->sql_fetchrow($result))
	{
		if ($row['left_id'] < $right)
		{
			$padding .= '&nbsp; &nbsp;';
			$padding_store[$row['parent_id']] = $padding;
		}
		else if ($row['left_id'] > $right + 1)
		{
			$padding = (isset($padding_store[$row['parent_id']])) ? $padding_store[$row['parent_id']] : '';
		}

		$right = $row['right_id'];
		$disabled = false;

		if ($acl && !$auth->acl_gets($acl, $row['forum_id']))
		{
			// List permission?
			if ($auth->acl_get('f_list', $row['forum_id']))
			{
				$disabled = true;
			}
			else
			{
				continue;
			}
		}

		if (
			((is_array($ignore_id) && in_array($row['forum_id'], $ignore_id)) || $row['forum_id'] == $ignore_id)
			||
			// Non-postable forum with no subforums, don't display
			($row['forum_type'] == FORUM_CAT && ($row['left_id'] + 1 == $row['right_id']) && $ignore_emptycat)
			||
			($row['forum_type'] != FORUM_POST && $ignore_nonpost)
			)
		{
			$disabled = true;
		}

		if ($return_array)
		{
			// Include some more information...
			$selected = (is_array($select_id)) ? ((in_array($row['forum_id'], $select_id)) ? true : false) : (($row['forum_id'] == $select_id) ? true : false);
			$forum_list[$row['forum_id']] = array_merge(array('padding' => $padding, 'selected' => ($selected && !$disabled), 'disabled' => $disabled), $row);
		}
		else
		{
			$selected = (is_array($select_id)) ? ((in_array($row['forum_id'], $select_id)) ? ' selected="selected"' : '') : (($row['forum_id'] == $select_id) ? ' selected="selected"' : '');
			$forum_list .= '<option value="' . $row['forum_id'] . '"' . (($disabled) ? ' disabled="disabled" class="disabled-option"' : $selected) . '>' . $padding . $row['forum_name'] . '</option>';
		}
	}
	$db->sql_freeresult($result);
	unset($padding_store);

	return $forum_list;
}

/**
* Generate size select options
*/
function size_select_options($size_compare)
{
	global $user;

	$size_types_text = array($user->lang['BYTES'], $user->lang['KIB'], $user->lang['MIB']);
	$size_types = array('b', 'kb', 'mb');

	$s_size_options = '';

	for ($i = 0, $size = sizeof($size_types_text); $i < $size; $i++)
	{
		$selected = ($size_compare == $size_types[$i]) ? ' selected="selected"' : '';
		$s_size_options .= '<option value="' . $size_types[$i] . '"' . $selected . '>' . $size_types_text[$i] . '</option>';
	}

	return $s_size_options;
}

/**
* Generate list of groups (option fields without select)
*
* @param int $group_id The default group id to mark as selected
* @param array $exclude_ids The group ids to exclude from the list, false (default) if you whish to exclude no id
* @param int $manage_founder If set to false (default) all groups are returned, if 0 only those groups returned not being managed by founders only, if 1 only those groups returned managed by founders only.
*
* @return string The list of options.
*/
function group_select_options($group_id, $exclude_ids = false, $manage_founder = false)
{
	global $db, $user, $config;

	$exclude_sql = ($exclude_ids !== false && sizeof($exclude_ids)) ? 'WHERE ' . $db->sql_in_set('group_id', array_map('intval', $exclude_ids), true) : '';
	$sql_and = (!$config['coppa_enable']) ? (($exclude_sql) ? ' AND ' : ' WHERE ') . "group_name <> 'REGISTERED_COPPA'" : '';
	$sql_founder = ($manage_founder !== false) ? (($exclude_sql || $sql_and) ? ' AND ' : ' WHERE ') . 'group_founder_manage = ' . (int) $manage_founder : '';

	$sql = 'SELECT group_id, group_name, group_type
		FROM ' . GROUPS_TABLE . "
		$exclude_sql
		$sql_and
		$sql_founder
		ORDER BY group_type DESC, group_name ASC";
	$result = $db->sql_query($sql);

	$s_group_options = '';
	while ($row = $db->sql_fetchrow($result))
	{
		$selected = ($row['group_id'] == $group_id) ? ' selected="selected"' : '';
		$s_group_options .= '<option' . (($row['group_type'] == GROUP_SPECIAL) ? ' class="sep"' : '') . ' value="' . $row['group_id'] . '"' . $selected . '>' . (($row['group_type'] == GROUP_SPECIAL) ? $user->lang['G_' . $row['group_name']] : $row['group_name']) . '</option>';
	}
	$db->sql_freeresult($result);

	return $s_group_options;
}

/**
* Obtain authed forums list
*/
function get_forum_list($acl_list = 'f_list', $id_only = true, $postable_only = false, $no_cache = false)
{
	global $db, $auth;
	static $forum_rows;

	if (!isset($forum_rows))
	{
		// This query is identical to the jumpbox one
		$expire_time = ($no_cache) ? 0 : 600;

		$sql = 'SELECT forum_id, forum_name, parent_id, forum_type, left_id, right_id
			FROM ' . FORUMS_TABLE . '
			ORDER BY left_id ASC';
		$result = $db->sql_query($sql, $expire_time);

		$forum_rows = array();

		$right = $padding = 0;
		$padding_store = array('0' => 0);

		while ($row = $db->sql_fetchrow($result))
		{
			if ($row['left_id'] < $right)
			{
				$padding++;
				$padding_store[$row['parent_id']] = $padding;
			}
			else if ($row['left_id'] > $right + 1)
			{
				// Ok, if the $padding_store for this parent is empty there is something wrong. For now we will skip over it.
				// @todo digging deep to find out "how" this can happen.
				$padding = (isset($padding_store[$row['parent_id']])) ? $padding_store[$row['parent_id']] : $padding;
			}

			$right = $row['right_id'];
			$row['padding'] = $padding;

			$forum_rows[] = $row;
		}
		$db->sql_freeresult($result);
		unset($padding_store);
	}

	$rowset = array();
	foreach ($forum_rows as $row)
	{
		if ($postable_only && $row['forum_type'] != FORUM_POST)
		{
			continue;
		}

		if ($acl_list == '' || ($acl_list != '' && $auth->acl_gets($acl_list, $row['forum_id'])))
		{
			$rowset[] = ($id_only) ? $row['forum_id'] : $row;
		}
	}

	return $rowset;
}

/**
* Get forum branch
*/
function get_forum_branch($forum_id, $type = 'all', $order = 'descending', $include_forum = true)
{
	global $db;

	switch ($type)
	{
		case 'parents':
			$condition = 'f1.left_id BETWEEN f2.left_id AND f2.right_id';
		break;

		case 'children':
			$condition = 'f2.left_id BETWEEN f1.left_id AND f1.right_id';
		break;

		default:
			$condition = 'f2.left_id BETWEEN f1.left_id AND f1.right_id OR f1.left_id BETWEEN f2.left_id AND f2.right_id';
		break;
	}

	$rows = array();

	$sql = 'SELECT f2.*
		FROM ' . FORUMS_TABLE . ' f1
		LEFT JOIN ' . FORUMS_TABLE . " f2 ON ($condition)
		WHERE f1.forum_id = $forum_id
		ORDER BY f2.left_id " . (($order == 'descending') ? 'ASC' : 'DESC');
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		if (!$include_forum && $row['forum_id'] == $forum_id)
		{
			continue;
		}

		$rows[] = $row;
	}
	$db->sql_freeresult($result);

	return $rows;
}

/**
* Get physical file listing
*/
function filelist($rootdir, $dir = '', $type = 'gif|jpg|jpeg|png')
{
	$matches = array();

	// Remove initial / if present
	$rootdir = (substr($rootdir, 0, 1) == '/') ? substr($rootdir, 1) : $rootdir;
	// Add closing / if not present
	$rootdir = ($rootdir && substr($rootdir, -1) != '/') ? $rootdir . '/' : $rootdir;

	// Remove initial / if present
	$dir = (substr($dir, 0, 1) == '/') ? substr($dir, 1) : $dir;
	// Add closing / if not present
	$dir = ($dir && substr($dir, -1) != '/') ? $dir . '/' : $dir;

	if (!is_dir($rootdir . $dir))
	{
		return $matches;
	}

	$dh = @opendir($rootdir . $dir);

	if (!$dh)
	{
		return $matches;
	}

	while (($fname = readdir($dh)) !== false)
	{
		if (is_file("$rootdir$dir$fname"))
		{
			if (filesize("$rootdir$dir$fname") && preg_match('#\.' . $type . '$#i', $fname))
			{
				$matches[$dir][] = $fname;
			}
		}
		else if ($fname[0] != '.' && is_dir("$rootdir$dir$fname"))
		{
			$matches += filelist($rootdir, "$dir$fname", $type);
		}
	}
	closedir($dh);

	return $matches;
}

/**
* Move topic(s)
*/
function move_topics($topic_ids, $forum_id, $auto_sync = true)
{
	global $db;

	if (empty($topic_ids))
	{
		return;
	}

	$forum_ids = array($forum_id);

	if (!is_array($topic_ids))
	{
		$topic_ids = array($topic_ids);
	}

	$sql = 'DELETE FROM ' . TOPICS_TABLE . '
		WHERE ' . $db->sql_in_set('topic_moved_id', $topic_ids) . '
			AND forum_id = ' . $forum_id;
	$db->sql_query($sql);

	if ($auto_sync)
	{
		$sql = 'SELECT DISTINCT forum_id
			FROM ' . TOPICS_TABLE . '
			WHERE ' . $db->sql_in_set('topic_id', $topic_ids);
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$forum_ids[] = $row['forum_id'];
		}
		$db->sql_freeresult($result);
	}

	$table_ary = array(TOPICS_TABLE, POSTS_TABLE, LOG_TABLE, DRAFTS_TABLE, TOPICS_TRACK_TABLE);
	foreach ($table_ary as $table)
	{
		$sql = "UPDATE $table
			SET forum_id = $forum_id
			WHERE " . $db->sql_in_set('topic_id', $topic_ids);
		$db->sql_query($sql);
	}
	unset($table_ary);

	if ($auto_sync)
	{
		sync('forum', 'forum_id', $forum_ids, true, true);
		unset($forum_ids);
	}
}

/**
* Move post(s)
*/
function move_posts($post_ids, $topic_id, $auto_sync = true)
{
	global $db;

	if (!is_array($post_ids))
	{
		$post_ids = array($post_ids);
	}

	$forum_ids = array();
	$topic_ids = array($topic_id);

	$sql = 'SELECT DISTINCT topic_id, forum_id
		FROM ' . POSTS_TABLE . '
		WHERE ' . $db->sql_in_set('post_id', $post_ids);
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		$forum_ids[] = $row['forum_id'];
		$topic_ids[] = $row['topic_id'];
	}
	$db->sql_freeresult($result);

	$sql = 'SELECT forum_id
		FROM ' . TOPICS_TABLE . '
		WHERE topic_id = ' . $topic_id;
	$result = $db->sql_query($sql);
	$forum_row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	if (!$forum_row)
	{
		trigger_error('NO_TOPIC');
	}

	$sql = 'UPDATE ' . POSTS_TABLE . '
		SET forum_id = ' . $forum_row['forum_id'] . ", topic_id = $topic_id
		WHERE " . $db->sql_in_set('post_id', $post_ids);
	$db->sql_query($sql);

	$sql = 'UPDATE ' . ATTACHMENTS_TABLE . "
		SET topic_id = $topic_id, in_message = 0
		WHERE " . $db->sql_in_set('post_msg_id', $post_ids);
	$db->sql_query($sql);

	if ($auto_sync)
	{
		$forum_ids[] = $forum_row['forum_id'];

		sync('topic_reported', 'topic_id', $topic_ids);
		sync('topic_attachment', 'topic_id', $topic_ids);
		sync('topic', 'topic_id', $topic_ids, true);
		sync('forum', 'forum_id', $forum_ids, true, true);
	}

	// Update posted information
	update_posted_info($topic_ids);
}

/**
* Remove topic(s)
*/
function delete_topics($where_type, $where_ids, $auto_sync = true, $post_count_sync = true, $call_delete_posts = true)
{
	global $db, $config;

	$approved_topics = 0;
	$forum_ids = $topic_ids = array();

	if ($where_type === 'range')
	{
		$where_clause = $where_ids;
	}
	else
	{
		$where_ids = (is_array($where_ids)) ? array_unique($where_ids) : array($where_ids);

		if (!sizeof($where_ids))
		{
			return array('topics' => 0, 'posts' => 0);
		}

		$where_clause = $db->sql_in_set($where_type, $where_ids);
	}

	// Making sure that delete_posts does not call delete_topics again...
	$return = array(
		'posts' => ($call_delete_posts) ? delete_posts($where_type, $where_ids, false, true, $post_count_sync, false) : 0,
	);

	$sql = 'SELECT topic_id, forum_id, topic_approved
		FROM ' . TOPICS_TABLE . '
		WHERE ' . $where_clause;
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		$forum_ids[] = $row['forum_id'];
		$topic_ids[] = $row['topic_id'];

		if ($row['topic_approved'])
		{
			$approved_topics++;
		}
	}
	$db->sql_freeresult($result);

	$return['topics'] = sizeof($topic_ids);

	if (!sizeof($topic_ids))
	{
		return $return;
	}

	$db->sql_transaction('begin');

	$table_ary = array(TOPICS_TRACK_TABLE, TOPICS_POSTED_TABLE, POLL_VOTES_TABLE, POLL_OPTIONS_TABLE, TOPICS_WATCH_TABLE, TOPICS_TABLE);

	foreach ($table_ary as $table)
	{
		$sql = "DELETE FROM $table
			WHERE " . $db->sql_in_set('topic_id', $topic_ids);
		$db->sql_query($sql);
	}
	unset($table_ary);

	$moved_topic_ids = array();

	// update the other forums
	$sql = 'SELECT topic_id, forum_id
		FROM ' . TOPICS_TABLE . '
		WHERE ' . $db->sql_in_set('topic_moved_id', $topic_ids);
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		$forum_ids[] = $row['forum_id'];
		$moved_topic_ids[] = $row['topic_id'];
	}
	$db->sql_freeresult($result);

	if (sizeof($moved_topic_ids))
	{
		$sql = 'DELETE FROM ' . TOPICS_TABLE . '
			WHERE ' . $db->sql_in_set('topic_id', $moved_topic_ids);
		$db->sql_query($sql);
	}

	$db->sql_transaction('commit');

	if ($auto_sync)
	{
		sync('forum', 'forum_id', array_unique($forum_ids), true, true);
		sync('topic_reported', $where_type, $where_ids);
	}

	if ($approved_topics)
	{
		set_config('num_topics', $config['num_topics'] - $approved_topics, true);
	}

	return $return;
}

/**
* Remove post(s)
*/
function delete_posts($where_type, $where_ids, $auto_sync = true, $posted_sync = true, $post_count_sync = true, $call_delete_topics = true)
{
	global $db, $config, $phpbb_root_path, $phpEx;

	if ($where_type === 'range')
	{
		$where_clause = $where_ids;
	}
	else
	{
		if (is_array($where_ids))
		{
			$where_ids = array_unique($where_ids);
		}
		else
		{
			$where_ids = array($where_ids);
		}

		if (!sizeof($where_ids))
		{
			return false;
		}

		$where_clause = $db->sql_in_set($where_type, array_map('intval', $where_ids));
	}

	$approved_posts = 0;
	$post_ids = $topic_ids = $forum_ids = $post_counts = $remove_topics = array();

	$sql = 'SELECT post_id, poster_id, post_approved, post_postcount, topic_id, forum_id
		FROM ' . POSTS_TABLE . '
		WHERE ' . $where_clause;
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		$post_ids[] = $row['post_id'];
		$poster_ids[] = $row['poster_id'];
		$topic_ids[] = $row['topic_id'];
		$forum_ids[] = $row['forum_id'];

		if ($row['post_postcount'] && $post_count_sync)
		{
			$post_counts[$row['poster_id']] = (!empty($post_counts[$row['poster_id']])) ? $post_counts[$row['poster_id']] + 1 : 1;
		}

		if ($row['post_approved'])
		{
			$approved_posts++;
		}
	}
	$db->sql_freeresult($result);

	if (!sizeof($post_ids))
	{
		return false;
	}

	$db->sql_transaction('begin');

	$table_ary = array(POSTS_TABLE, REPORTS_TABLE);

	foreach ($table_ary as $table)
	{
		$sql = "DELETE FROM $table
			WHERE " . $db->sql_in_set('post_id', $post_ids);
		$db->sql_query($sql);
	}
	unset($table_ary);

	// Adjust users post counts
	if (sizeof($post_counts) && $post_count_sync)
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

	// Remove topics now having no posts?
	if (sizeof($topic_ids))
	{
		$sql = 'SELECT topic_id
			FROM ' . POSTS_TABLE . '
			WHERE ' . $db->sql_in_set('topic_id', $topic_ids) . '
			GROUP BY topic_id';
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$remove_topics[] = $row['topic_id'];
		}
		$db->sql_freeresult($result);

		// Actually, those not within remove_topics should be removed. ;)
		$remove_topics = array_diff($topic_ids, $remove_topics);
	}

	// Remove the message from the search index
	$search_type = basename($config['search_type']);

	if (!file_exists($phpbb_root_path . 'includes/search/' . $search_type . '.' . $phpEx))
	{
		trigger_error('NO_SUCH_SEARCH_MODULE');
	}

	include_once("{$phpbb_root_path}includes/search/$search_type.$phpEx");

	$error = false;
	$search = new $search_type($error);

	if ($error)
	{
		trigger_error($error);
	}

	$search->index_remove($post_ids, $poster_ids, $forum_ids);

	delete_attachments('post', $post_ids, false);

	$db->sql_transaction('commit');

	// Resync topics_posted table
	if ($posted_sync)
	{
		update_posted_info($topic_ids);
	}

	if ($auto_sync)
	{
		sync('topic_reported', 'topic_id', $topic_ids);
		sync('topic', 'topic_id', $topic_ids, true);
		sync('forum', 'forum_id', $forum_ids, true, true);
	}

	if ($approved_posts)
	{
		set_config('num_posts', $config['num_posts'] - $approved_posts, true);
	}

	// We actually remove topics now to not be inconsistent (the delete_topics function calls this function too)
	if (sizeof($remove_topics) && $call_delete_topics)
	{
		delete_topics('topic_id', $remove_topics, $auto_sync, $post_count_sync, false);
	}

	return sizeof($post_ids);
}

/**
* Delete Attachments
*
* @param string $mode can be: post|topic|attach|user
* @param mixed $ids can be: post_ids, topic_ids, attach_ids, user_ids
* @param bool $resync set this to false if you are deleting posts or topics
*/
function delete_attachments($mode, $ids, $resync = true)
{
	global $db, $config;

	if (is_array($ids) && sizeof($ids))
	{
		$ids = array_unique($ids);
		$ids = array_map('intval', $ids);
	}
	else
	{
		$ids = array((int) $ids);
	}

	if (!sizeof($ids))
	{
		return false;
	}

	$sql_id = ($mode == 'user') ? 'poster_id' : (($mode == 'post') ? 'post_msg_id' : (($mode == 'topic') ? 'topic_id' : 'attach_id'));

	$post_ids = $topic_ids = $physical = array();

	// Collect post and topics ids for later use
	if ($mode == 'attach' || $mode == 'user' || ($mode == 'topic' && $resync))
	{
		$sql = 'SELECT post_msg_id as post_id, topic_id, physical_filename, thumbnail, filesize
			FROM ' . ATTACHMENTS_TABLE . '
			WHERE ' . $db->sql_in_set($sql_id, $ids);
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$post_ids[] = $row['post_id'];
			$topic_ids[] = $row['topic_id'];
			$physical[] = array('filename' => $row['physical_filename'], 'thumbnail' => $row['thumbnail'], 'filesize' => $row['filesize']);
		}
		$db->sql_freeresult($result);
	}

	if ($mode == 'post')
	{
		$sql = 'SELECT topic_id, physical_filename, thumbnail, filesize
			FROM ' . ATTACHMENTS_TABLE . '
			WHERE ' . $db->sql_in_set('post_msg_id', $ids) . '
				AND in_message = 0';
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$topic_ids[] = $row['topic_id'];
			$physical[] = array('filename' => $row['physical_filename'], 'thumbnail' => $row['thumbnail'], 'filesize' => $row['filesize']);
		}
		$db->sql_freeresult($result);
	}

	// Delete attachments
	$sql = 'DELETE FROM ' . ATTACHMENTS_TABLE . '
		WHERE ' . $db->sql_in_set($sql_id, $ids);
	$db->sql_query($sql);
	$num_deleted = $db->sql_affectedrows();

	if (!$num_deleted)
	{
		return 0;
	}

	// Delete attachments from filesystem
	$space_removed = $files_removed = 0;
	foreach ($physical as $file_ary)
	{
		if (phpbb_unlink($file_ary['filename'], 'file', true))
		{
			$space_removed += $file_ary['filesize'];
			$files_removed++;
		}

		if ($file_ary['thumbnail'])
		{
			phpbb_unlink($file_ary['filename'], 'thumbnail', true);
		}
	}
	set_config('upload_dir_size', $config['upload_dir_size'] - $space_removed, true);
	set_config('num_files', $config['num_files'] - $files_removed, true);

	if ($mode == 'topic' && !$resync)
	{
		return $num_deleted;
	}

	if ($mode == 'post')
	{
		$post_ids = $ids;
	}
	unset($ids);

	$post_ids = array_unique($post_ids);
	$topic_ids = array_unique($topic_ids);

	// Update post indicators
	if (sizeof($post_ids))
	{
		if ($mode == 'post' || $mode == 'topic')
		{
			$sql = 'UPDATE ' . POSTS_TABLE . '
				SET post_attachment = 0
				WHERE ' . $db->sql_in_set('post_id', $post_ids);
			$db->sql_query($sql);
		}

		if ($mode == 'user' || $mode == 'attach')
		{
			$remaining = array();

			$sql = 'SELECT post_msg_id
				FROM ' . ATTACHMENTS_TABLE . '
				WHERE ' . $db->sql_in_set('post_msg_id', $post_ids) . '
					AND in_message = 0';
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$remaining[] = $row['post_msg_id'];		
			}
			$db->sql_freeresult($result);

			$unset_ids = array_diff($post_ids, $remaining);

			if (sizeof($unset_ids))
			{
				$sql = 'UPDATE ' . POSTS_TABLE . '
					SET post_attachment = 0
					WHERE ' . $db->sql_in_set('post_id', $unset_ids);
				$db->sql_query($sql);
			}

			$remaining = array();

			$sql = 'SELECT post_msg_id
				FROM ' . ATTACHMENTS_TABLE . '
				WHERE ' . $db->sql_in_set('post_msg_id', $post_ids) . '
					AND in_message = 1';
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$remaining[] = $row['post_msg_id'];		
			}
			$db->sql_freeresult($result);

			$unset_ids = array_diff($post_ids, $remaining);

			if (sizeof($unset_ids))
			{
				$sql = 'UPDATE ' . PRIVMSGS_TABLE . '
					SET message_attachment = 0
					WHERE ' . $db->sql_in_set('msg_id', $unset_ids);
				$db->sql_query($sql);
			}
		}
	}

	if (sizeof($topic_ids))
	{
		// Update topic indicator
		if ($mode == 'topic')
		{
			$sql = 'UPDATE ' . TOPICS_TABLE . '
				SET topic_attachment = 0
				WHERE ' . $db->sql_in_set('topic_id', $topic_ids);
			$db->sql_query($sql);
		}

		if ($mode == 'post' || $mode == 'user' || $mode == 'attach')
		{
			$remaining = array();

			$sql = 'SELECT topic_id
				FROM ' . ATTACHMENTS_TABLE . '
				WHERE ' . $db->sql_in_set('topic_id', $topic_ids);
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$remaining[] = $row['topic_id'];		
			}
			$db->sql_freeresult($result);

			$unset_ids = array_diff($topic_ids, $remaining);

			if (sizeof($unset_ids))
			{
				$sql = 'UPDATE ' . TOPICS_TABLE . '
					SET topic_attachment = 0
					WHERE ' . $db->sql_in_set('topic_id', $unset_ids);
				$db->sql_query($sql);
			}
		}
	}

	return $num_deleted;
}

/**
* Remove topic shadows
*/
function delete_topic_shadows($max_age, $forum_id = '', $auto_sync = true)
{
	$where = (is_array($forum_id)) ? 'AND ' . $db->sql_in_set('t.forum_id', array_map('intval', $forum_id)) : (($forum_id) ? 'AND t.forum_id = ' . (int) $forum_id : '');

	switch ($db->sql_layer)
	{
		case 'mysql4':
		case 'mysqli':
			$sql = 'DELETE t.*
				FROM ' . TOPICS_TABLE . ' t, ' . TOPICS_TABLE . ' t2
				WHERE t.topic_moved_id = t2.topic_id
					AND t.topic_time < ' . (time() - $max_age)
				. $where;
			$db->sql_query($sql);
		break;
	
		default:
			$sql = 'SELECT t.topic_id
				FROM ' . TOPICS_TABLE . ' t, ' . TOPICS_TABLE . ' t2
				WHERE t.topic_moved_id = t2.topic_id
					AND t.topic_time < ' . (time() - $max_age)
				. $where;
			$result = $db->sql_query($sql);

			$topic_ids = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$topic_ids[] = $row['topic_id'];
			}
			$db->sql_freeresult($result);

			if (sizeof($topic_ids))
			{
				$sql = 'DELETE FROM ' . TOPICS_TABLE . '
					WHERE ' . $db->sql_in_set('topic_id', $topic_ids);
				$db->sql_query($sql);
			}
		break;
	}

	if ($auto_sync)
	{
		$where_type = ($forum_id) ? 'forum_id' : '';
		sync('forum', $where_type, $forum_id, true, true);
	}
}

/**
* Update/Sync posted information for topics
*/
function update_posted_info(&$topic_ids)
{
	global $db, $config;

	if (empty($topic_ids) || !$config['load_db_track'])
	{
		return;
	}

	// First of all, let us remove any posted information for these topics
	$sql = 'DELETE FROM ' . TOPICS_POSTED_TABLE . '
		WHERE ' . $db->sql_in_set('topic_id', $topic_ids);
	$db->sql_query($sql);

	// Now, let us collect the user/topic combos for rebuilding the information
	$sql = 'SELECT poster_id, topic_id
		FROM ' . POSTS_TABLE . '
		WHERE ' . $db->sql_in_set('topic_id', $topic_ids) . '
			AND poster_id <> ' . ANONYMOUS . '
		GROUP BY poster_id, topic_id';
	$result = $db->sql_query($sql);

	$posted = array();
	while ($row = $db->sql_fetchrow($result))
	{
		// Add as key to make them unique (grouping by) and circumvent empty keys on array_unique
		$posted[$row['poster_id']][] = $row['topic_id'];
	}
	$db->sql_freeresult($result);

	// Now add the information...
	$sql_ary = array();
	foreach ($posted as $user_id => $topic_row)
	{
		foreach ($topic_row as $topic_id)
		{
			$sql_ary[] = array(
				'user_id'		=> (int) $user_id,
				'topic_id'		=> (int) $topic_id,
				'topic_posted'	=> 1,
			);
		}
	}
	unset($posted);

	$db->sql_multi_insert(TOPICS_POSTED_TABLE, $sql_ary);
}

/**
* Delete attached file
*/
function phpbb_unlink($filename, $mode = 'file', $entry_removed = false)
{
	global $db, $phpbb_root_path, $config;

	// Because of copying topics or modifications a physical filename could be assigned more than once. If so, do not remove the file itself.
	$sql = 'SELECT COUNT(attach_id) AS num_entries
		FROM ' . ATTACHMENTS_TABLE . "
		WHERE physical_filename = '" . $db->sql_escape(basename($filename)) . "'";
	$result = $db->sql_query($sql);
	$num_entries = (int) $db->sql_fetchfield('num_entries');
	$db->sql_freeresult($result);

	// Do not remove file if at least one additional entry with the same name exist.
	if (($entry_removed && $num_entries > 0) || (!$entry_removed && $num_entries > 1))
	{
		return false;
	}

	$filename = ($mode == 'thumbnail') ? 'thumb_' . basename($filename) : basename($filename);
	return @unlink($phpbb_root_path . $config['upload_path'] . '/' . $filename);
}

/**
* All-encompasing sync function
*
* Exaples:
* <code>
* sync('topic', 'topic_id', 123);			// resync topic #123
* sync('topic', 'forum_id', array(2, 3));	// resync topics from forum #2 and #3
* sync('topic');							// resync all topics
* sync('topic', 'range', 'topic_id BETWEEN 1 AND 60');	// resync a range of topics/forums (only available for 'topic' and 'forum' modes)
* </code>
*
* Modes:
* - forum				Resync complete forum
* - topic				Resync topics
* - topic_moved			Removes topic shadows that would be in the same forum as the topic they link to
* - topic_approved		Resyncs the topic_approved flag according to the status of the first post
* - post_reported		Resyncs the post_reported flag, relying on actual reports
* - topic_reported		Resyncs the topic_reported flag, relying on post_reported flags
* - post_attachement	Same as post_reported, but with attachment flags
* - topic_attachement	Same as topic_reported, but with attachment flags
*/
function sync($mode, $where_type = '', $where_ids = '', $resync_parents = false, $sync_extra = false)
{
	global $db;

	if (is_array($where_ids))
	{
		$where_ids = array_unique($where_ids);
		$where_ids = array_map('intval', $where_ids);
	}
	else if ($where_type != 'range')
	{
		$where_ids = ($where_ids) ? array((int) $where_ids) : array();
	}

	if ($mode == 'forum' || $mode == 'topic' || $mode == 'topic_approved' || $mode == 'topic_reported' || $mode == 'post_reported')
	{
		if (!$where_type)
		{
			$where_sql = '';
			$where_sql_and = 'WHERE';
		}
		else if ($where_type == 'range')
		{
			// Only check a range of topics/forums. For instance: 'topic_id BETWEEN 1 AND 60'
			$where_sql = 'WHERE (' . $mode[0] . ".$where_ids)";
			$where_sql_and = $where_sql . "\n\tAND";
		}
		else
		{
			// Do not sync the "global forum"
			$where_ids = array_diff($where_ids, array(0));

			if (!sizeof($where_ids))
			{
				// Empty array with IDs. This means that we don't have any work to do. Just return.
				return;
			}

			// Limit the topics/forums we are syncing, use specific topic/forum IDs.
			// $where_type contains the field for the where clause (forum_id, topic_id)
			$where_sql = 'WHERE ' . $db->sql_in_set($mode[0] . '.' . $where_type, $where_ids);
			$where_sql_and = $where_sql . "\n\tAND";
		}
	}
	else
	{
		if (!sizeof($where_ids))
		{
			return;
		}

		// $where_type contains the field for the where clause (forum_id, topic_id)
		$where_sql = 'WHERE ' . $db->sql_in_set($mode[0] . '.' . $where_type, $where_ids);
		$where_sql_and = $where_sql . "\n\tAND";
	}

	switch ($mode)
	{
		case 'topic_moved':
			switch ($db->sql_layer)
			{
				case 'mysql4':
				case 'mysqli':
					$sql = 'DELETE FROM ' . TOPICS_TABLE . '
						USING ' . TOPICS_TABLE . ' t1, ' . TOPICS_TABLE . " t2
						WHERE t1.topic_moved_id = t2.topic_id
							AND t1.forum_id = t2.forum_id";
					$db->sql_query($sql);
				break;
			
				default:
					$sql = 'SELECT t1.topic_id
						FROM ' .TOPICS_TABLE . ' t1, ' . TOPICS_TABLE . " t2
						WHERE t1.topic_moved_id = t2.topic_id
							AND t1.forum_id = t2.forum_id";
					$result = $db->sql_query($sql);

					$topic_id_ary = array();
					while ($row = $db->sql_fetchrow($result))
					{
						$topic_id_ary[] = $row['topic_id'];
					}
					$db->sql_freeresult($result);

					if (!sizeof($topic_id_ary))
					{
						return;
					}

					$sql = 'DELETE FROM ' . TOPICS_TABLE . '
						WHERE ' . $db->sql_in_set('topic_id', $topic_id_ary);
					$db->sql_query($sql);

				break;
			}
		break;

		case 'topic_approved':
			switch ($db->sql_layer)
			{
				case 'mysql4':
				case 'mysqli':
					$sql = 'UPDATE ' . TOPICS_TABLE . ' t, ' . POSTS_TABLE . " p
						SET t.topic_approved = p.post_approved
						$where_sql_and t.topic_first_post_id = p.post_id";
					$db->sql_query($sql);
				break;

				default:
					$sql = 'SELECT t.topic_id, p.post_approved
						FROM ' . TOPICS_TABLE . ' t, ' . POSTS_TABLE . " p
						$where_sql_and p.post_id = t.topic_first_post_id
							AND p.post_approved <> t.topic_approved";
					$result = $db->sql_query($sql);

					$topic_ids = array();
					while ($row = $db->sql_fetchrow($result))
					{
						$topic_ids[] = $row['topic_id'];
					}
					$db->sql_freeresult($result);

					if (!sizeof($topic_ids))
					{
						return;
					}

					$sql = 'UPDATE ' . TOPICS_TABLE . '
						SET topic_approved = 1 - topic_approved
						WHERE ' . $db->sql_in_set('topic_id', $topic_ids);
					$db->sql_query($sql);
				break;
			}
		break;

		case 'post_reported':
			$post_ids = $post_reported = array();

			$sql = 'SELECT p.post_id, p.post_reported
				FROM ' . POSTS_TABLE . " p
				$where_sql
				GROUP BY p.post_id, p.post_reported";
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$post_ids[$row['post_id']] = $row['post_id'];
				if ($row['post_reported'])
				{
					$post_reported[$row['post_id']] = 1;
				}
			}
			$db->sql_freeresult($result);

			$sql = 'SELECT DISTINCT(post_id)
				FROM ' . REPORTS_TABLE . '
				WHERE ' . $db->sql_in_set('post_id', $post_ids) . '
					AND report_closed = 0';
			$result = $db->sql_query($sql);

			$post_ids = array();
			while ($row = $db->sql_fetchrow($result))
			{
				if (!isset($post_reported[$row['post_id']]))
				{
					$post_ids[] = $row['post_id'];
				}
				else
				{
					unset($post_reported[$row['post_id']]);
				}
			}
			$db->sql_freeresult($result);

			// $post_reported should be empty by now, if it's not it contains
			// posts that are falsely flagged as reported
			foreach ($post_reported as $post_id => $void)
			{
				$post_ids[] = $post_id;
			}

			if (sizeof($post_ids))
			{
				$sql = 'UPDATE ' . POSTS_TABLE . '
					SET post_reported = 1 - post_reported
					WHERE ' . $db->sql_in_set('post_id', $post_ids);
				$db->sql_query($sql);
			}
		break;

		case 'topic_reported':
			if ($sync_extra)
			{
				sync('post_reported', $where_type, $where_ids);
			}

			$topic_ids = $topic_reported = array();

			$sql = 'SELECT DISTINCT(t.topic_id)
				FROM ' . POSTS_TABLE . " t
				$where_sql_and t.post_reported = 1";
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$topic_reported[$row['topic_id']] = 1;
			}
			$db->sql_freeresult($result);

			$sql = 'SELECT t.topic_id, t.topic_reported
				FROM ' . TOPICS_TABLE . " t
				$where_sql";
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				if ($row['topic_reported'] ^ isset($topic_reported[$row['topic_id']]))
				{
					$topic_ids[] = $row['topic_id'];
				}
			}
			$db->sql_freeresult($result);

			if (sizeof($topic_ids))
			{
				$sql = 'UPDATE ' . TOPICS_TABLE . '
					SET topic_reported = 1 - topic_reported
					WHERE ' . $db->sql_in_set('topic_id', $topic_ids);
				$db->sql_query($sql);
			}
		break;

		case 'post_attachment':
			$post_ids = $post_attachment = array();

			$sql = 'SELECT p.post_id, p.post_attachment
				FROM ' . POSTS_TABLE . " p
				$where_sql
				GROUP BY p.post_id, p.post_attachment";
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$post_ids[$row['post_id']] = $row['post_id'];
				if ($row['post_attachment'])
				{
					$post_attachment[$row['post_id']] = 1;
				}
			}
			$db->sql_freeresult($result);

			$sql = 'SELECT DISTINCT(post_msg_id)
				FROM ' . ATTACHMENTS_TABLE . '
				WHERE ' . $db->sql_in_set('post_msg_id', $post_ids) . '
					AND in_message = 0';
			$result = $db->sql_query($sql);

			$post_ids = array();
			while ($row = $db->sql_fetchrow($result))
			{
				if (!isset($post_attachment[$row['post_msg_id']]))
				{
					$post_ids[] = $row['post_msg_id'];
				}
				else
				{
					unset($post_attachment[$row['post_msg_id']]);
				}
			}
			$db->sql_freeresult($result);

			// $post_attachment should be empty by now, if it's not it contains
			// posts that are falsely flagged as having attachments
			foreach ($post_attachment as $post_id => $void)
			{
				$post_ids[] = $post_id;
			}

			if (sizeof($post_ids))
			{
				$sql = 'UPDATE ' . POSTS_TABLE . '
					SET post_attachment = 1 - post_attachment
					WHERE ' . $db->sql_in_set('post_id', $post_ids);
				$db->sql_query($sql);
			}
		break;

		case 'topic_attachment':
			if ($sync_extra)
			{
				sync('post_attachment', $where_type, $where_ids);
			}

			$topic_ids = $topic_attachment = array();

			$sql = 'SELECT DISTINCT(t.topic_id)
				FROM ' . POSTS_TABLE . " t
				$where_sql_and t.post_attachment = 1";
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$topic_attachment[$row['topic_id']] = 1;
			}
			$db->sql_freeresult($result);

			$sql = 'SELECT t.topic_id, t.topic_attachment
				FROM ' . TOPICS_TABLE . " t
				$where_sql";
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				if ($row['topic_attachment'] ^ isset($topic_attachment[$row['topic_id']]))
				{
					$topic_ids[] = $row['topic_id'];
				}
			}
			$db->sql_freeresult($result);

			if (sizeof($topic_ids))
			{
				$sql = 'UPDATE ' . TOPICS_TABLE . '
					SET topic_attachment = 1 - topic_attachment
					WHERE ' . $db->sql_in_set('topic_id', $topic_ids);
				$db->sql_query($sql);
			}
		break;

		case 'forum':

			// 1: Get the list of all forums
			$sql = 'SELECT f.*
				FROM ' . FORUMS_TABLE . " f
				$where_sql";
			$result = $db->sql_query($sql);

			$forum_data = $forum_ids = $post_ids = $last_post_id = $post_info = array();
			while ($row = $db->sql_fetchrow($result))
			{
				if ($row['forum_type'] == FORUM_LINK)
				{
					continue;
				}

				$forum_id = (int) $row['forum_id'];
				$forum_ids[$forum_id] = $forum_id;

				$forum_data[$forum_id] = $row;
				if ($sync_extra)
				{
					$forum_data[$forum_id]['posts'] = 0;
					$forum_data[$forum_id]['topics'] = 0;
					$forum_data[$forum_id]['topics_real'] = 0;
				}
				$forum_data[$forum_id]['last_post_id'] = 0;
				$forum_data[$forum_id]['last_post_subject'] = '';
				$forum_data[$forum_id]['last_post_time'] = 0;
				$forum_data[$forum_id]['last_poster_id'] = 0;
				$forum_data[$forum_id]['last_poster_name'] = '';
				$forum_data[$forum_id]['last_poster_colour'] = '';
			}
			$db->sql_freeresult($result);

			if (!sizeof($forum_ids))
			{
				break;
			}

			$forum_ids = array_values($forum_ids);

			// 2: Get topic counts for each forum (optional)
			if ($sync_extra)
			{
				$sql = 'SELECT forum_id, topic_approved, COUNT(topic_id) AS forum_topics
					FROM ' . TOPICS_TABLE . '
					WHERE ' . $db->sql_in_set('forum_id', $forum_ids) . '
					GROUP BY forum_id, topic_approved';
				$result = $db->sql_query($sql);

				while ($row = $db->sql_fetchrow($result))
				{
					$forum_id = (int) $row['forum_id'];
					$forum_data[$forum_id]['topics_real'] += $row['forum_topics'];

					if ($row['topic_approved'])
					{
						$forum_data[$forum_id]['topics'] = $row['forum_topics'];
					}
				}
				$db->sql_freeresult($result);
			}

			// 3: Get post count for each forum (optional)
			if ($sync_extra)
			{
				if (sizeof($forum_ids) == 1)
				{
					$sql = 'SELECT SUM(t.topic_replies + 1) AS forum_posts
						FROM ' . TOPICS_TABLE . ' t
						WHERE ' . $db->sql_in_set('t.forum_id', $forum_ids) . '
							AND t.topic_approved = 1';
				}
				else
				{
					$sql = 'SELECT t.forum_id, SUM(t.topic_replies + 1) AS forum_posts
						FROM ' . TOPICS_TABLE . ' t
						WHERE ' . $db->sql_in_set('t.forum_id', $forum_ids) . '
							AND t.topic_approved = 1
						GROUP BY t.forum_id';
				}

				$result = $db->sql_query($sql);

				while ($row = $db->sql_fetchrow($result))
				{
					$forum_id = (sizeof($forum_ids) == 1) ? (int) $forum_ids[0] : (int) $row['forum_id'];

					$forum_data[$forum_id]['posts'] = (int) $row['forum_posts'];
				}
				$db->sql_freeresult($result);
			}

			// 4: Get last_post_id for each forum
			if (sizeof($forum_ids) == 1)
			{
				$sql = 'SELECT MAX(t.topic_last_post_id) as last_post_id
					FROM ' . TOPICS_TABLE . ' t
					WHERE ' . $db->sql_in_set('t.forum_id', $forum_ids) . '
						AND t.topic_approved = 1';
			}
			else
			{
				$sql = 'SELECT t.forum_id, MAX(t.topic_last_post_id) as last_post_id
					FROM ' . TOPICS_TABLE . ' t
					WHERE ' . $db->sql_in_set('t.forum_id', $forum_ids) . '
						AND t.topic_approved = 1
					GROUP BY t.forum_id';
			}

			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$forum_id = (sizeof($forum_ids) == 1) ? (int) $forum_ids[0] : (int) $row['forum_id'];

				$forum_data[$forum_id]['last_post_id'] = (int) $row['last_post_id'];

				$post_ids[] = $row['last_post_id'];
			}
			$db->sql_freeresult($result);

			// 5: Retrieve last_post infos
			if (sizeof($post_ids))
			{
				$sql = 'SELECT p.post_id, p.poster_id, p.post_subject, p.post_time, p.post_username, u.username, u.user_colour
					FROM ' . POSTS_TABLE . ' p, ' . USERS_TABLE . ' u
					WHERE ' . $db->sql_in_set('p.post_id', $post_ids) . '
						AND p.poster_id = u.user_id';
				$result = $db->sql_query($sql);

				while ($row = $db->sql_fetchrow($result))
				{
					$post_info[$row['post_id']] = $row;
				}
				$db->sql_freeresult($result);

				foreach ($forum_data as $forum_id => $data)
				{
					if ($data['last_post_id'])
					{
						if (isset($post_info[$data['last_post_id']]))
						{
							$forum_data[$forum_id]['last_post_subject'] = $post_info[$data['last_post_id']]['post_subject'];
							$forum_data[$forum_id]['last_post_time'] = $post_info[$data['last_post_id']]['post_time'];
							$forum_data[$forum_id]['last_poster_id'] = $post_info[$data['last_post_id']]['poster_id'];
							$forum_data[$forum_id]['last_poster_name'] = ($post_info[$data['last_post_id']]['poster_id'] != ANONYMOUS) ? $post_info[$data['last_post_id']]['username'] : $post_info[$data['last_post_id']]['post_username'];
							$forum_data[$forum_id]['last_poster_colour'] = $post_info[$data['last_post_id']]['user_colour'];
						}
						else
						{
							// For some reason we did not find the post in the db
							$forum_data[$forum_id]['last_post_id'] = 0;
							$forum_data[$forum_id]['last_post_subject'] = '';
							$forum_data[$forum_id]['last_post_time'] = 0;
							$forum_data[$forum_id]['last_poster_id'] = 0;
							$forum_data[$forum_id]['last_poster_name'] = '';
							$forum_data[$forum_id]['last_poster_colour'] = '';
						}
					}
				}
				unset($post_info);
			}

			// 6: Now do that thing
			$fieldnames = array('last_post_id', 'last_post_subject', 'last_post_time', 'last_poster_id', 'last_poster_name', 'last_poster_colour');

			if ($sync_extra)
			{
				array_push($fieldnames, 'posts', 'topics', 'topics_real');
			}

			foreach ($forum_data as $forum_id => $row)
			{
				$sql_ary = array();

				foreach ($fieldnames as $fieldname)
				{
					if ($row['forum_' . $fieldname] != $row[$fieldname])
					{
						if (preg_match('#(name|colour|subject)$#', $fieldname))
						{
							$sql_ary['forum_' . $fieldname] = (string) $row[$fieldname];
						}
						else
						{
							$sql_ary['forum_' . $fieldname] = (int) $row[$fieldname];
						}
					}
				}

				if (sizeof($sql_ary))
				{
					$sql = 'UPDATE ' . FORUMS_TABLE . '
						SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
						WHERE forum_id = ' . $forum_id;
					$db->sql_query($sql);
				}
			}
		break;

		case 'topic':
			$topic_data = $post_ids = $approved_unapproved_ids = $resync_forums = $delete_topics = $delete_posts = $moved_topics = array();

			$sql = 'SELECT t.topic_id, t.forum_id, t.topic_moved_id, t.topic_approved, ' . (($sync_extra) ? 't.topic_attachment, t.topic_reported, ' : '') . 't.topic_poster, t.topic_time, t.topic_replies, t.topic_replies_real, t.topic_first_post_id, t.topic_first_poster_name, t.topic_first_poster_colour, t.topic_last_post_id, t.topic_last_post_subject, t.topic_last_poster_id, t.topic_last_poster_name, t.topic_last_poster_colour, t.topic_last_post_time
				FROM ' . TOPICS_TABLE . " t
				$where_sql";
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				if ($row['topic_moved_id'])
				{
					$moved_topics[] = $row['topic_id'];
					continue;
				}

				$topic_id = (int) $row['topic_id'];
				$topic_data[$topic_id] = $row;
				$topic_data[$topic_id]['replies_real'] = -1;
				$topic_data[$topic_id]['replies'] = 0;
				$topic_data[$topic_id]['first_post_id'] = 0;
				$topic_data[$topic_id]['last_post_id'] = 0;
				unset($topic_data[$topic_id]['topic_id']);

				// This array holds all topic_ids
				$delete_topics[$topic_id] = '';

				if ($sync_extra)
				{
					$topic_data[$topic_id]['reported'] = 0;
					$topic_data[$topic_id]['attachment'] = 0;
				}
			}
			$db->sql_freeresult($result);

			// Use "t" as table alias because of the $where_sql clause
			// NOTE: 't.post_approved' in the GROUP BY is causing a major slowdown.
			$sql = 'SELECT t.topic_id, t.post_approved, COUNT(t.post_id) AS total_posts, MIN(t.post_id) AS first_post_id, MAX(t.post_id) AS last_post_id
				FROM ' . POSTS_TABLE . " t
				$where_sql
				GROUP BY t.topic_id, t.post_approved";
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$topic_id = (int) $row['topic_id'];

				$row['first_post_id'] = (int) $row['first_post_id'];
				$row['last_post_id'] = (int) $row['last_post_id'];

				if (!isset($topic_data[$topic_id]))
				{
					// Hey, these posts come from a topic that does not exist
					$delete_posts[$topic_id] = '';
				}
				else
				{
					// Unset the corresponding entry in $delete_topics
					// When we'll be done, only topics with no posts will remain
					unset($delete_topics[$topic_id]);

					$topic_data[$topic_id]['replies_real'] += $row['total_posts'];
					$topic_data[$topic_id]['first_post_id'] = (!$topic_data[$topic_id]['first_post_id']) ? $row['first_post_id'] : min($topic_data[$topic_id]['first_post_id'], $row['first_post_id']);

					if ($row['post_approved'] || !$topic_data[$topic_id]['last_post_id'])
					{
						$topic_data[$topic_id]['replies'] = $row['total_posts'] - 1;
						$topic_data[$topic_id]['last_post_id'] = $row['last_post_id'];
					}
				}
			}
			$db->sql_freeresult($result);

			foreach ($topic_data as $topic_id => $row)
			{
				$post_ids[] = $row['first_post_id'];
				if ($row['first_post_id'] != $row['last_post_id'])
				{
					$post_ids[] = $row['last_post_id'];
				}
			}

			// Now we delete empty topics and orphan posts
			if (sizeof($delete_posts))
			{
				delete_posts('topic_id', array_keys($delete_posts), false);
				unset($delete_posts);
			}

			if (!sizeof($topic_data))
			{
				// If we get there, topic ids were invalid or topics did not contain any posts
				delete_topics($where_type, $where_ids, true);
				return;
			}

			if (sizeof($delete_topics))
			{
				$delete_topic_ids = array();
				foreach ($delete_topics as $topic_id => $void)
				{
					unset($topic_data[$topic_id]);
					$delete_topic_ids[] = $topic_id;
				}

				delete_topics('topic_id', $delete_topic_ids, false);
				unset($delete_topics, $delete_topic_ids);
			}

			$sql = 'SELECT p.post_id, p.topic_id, p.post_approved, p.poster_id, p.post_subject, p.post_username, p.post_time, u.username, u.user_colour
				FROM ' . POSTS_TABLE . ' p, ' . USERS_TABLE . ' u
				WHERE ' . $db->sql_in_set('p.post_id', $post_ids) . '
					AND u.user_id = p.poster_id';
			$result = $db->sql_query($sql);

			$post_ids = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$topic_id = intval($row['topic_id']);

				if ($row['post_id'] == $topic_data[$topic_id]['first_post_id'])
				{
					if ($topic_data[$topic_id]['topic_approved'] != $row['post_approved'])
					{
						$approved_unapproved_ids[] = $topic_id;
					}
					$topic_data[$topic_id]['time'] = $row['post_time'];
					$topic_data[$topic_id]['poster'] = $row['poster_id'];
					$topic_data[$topic_id]['first_poster_name'] = ($row['poster_id'] == ANONYMOUS) ? $row['post_username'] : $row['username'];
					$topic_data[$topic_id]['first_poster_colour'] = $row['user_colour'];
				}

				if ($row['post_id'] == $topic_data[$topic_id]['last_post_id'])
				{
					$topic_data[$topic_id]['last_poster_id'] = $row['poster_id'];
					$topic_data[$topic_id]['last_post_subject'] = $row['post_subject'];
					$topic_data[$topic_id]['last_post_time'] = $row['post_time'];
					$topic_data[$topic_id]['last_poster_name'] = ($row['poster_id'] == ANONYMOUS) ? $row['post_username'] : $row['username'];
					$topic_data[$topic_id]['last_poster_colour'] = $row['user_colour'];
				}
			}
			$db->sql_freeresult($result);

			// Make sure shadow topics do link to existing topics
			if (sizeof($moved_topics))
			{
				$delete_topics = array();

				$sql = 'SELECT t1.topic_id, t1.topic_moved_id
					FROM ' . TOPICS_TABLE . ' t1
					LEFT JOIN ' . TOPICS_TABLE . ' t2 ON (t2.topic_id = t1.topic_moved_id)
					WHERE ' . $db->sql_in_set('t1.topic_id', $moved_topics) . '
						AND t2.topic_id IS NULL';
				$result = $db->sql_query($sql);

				while ($row = $db->sql_fetchrow($result))
				{
					$delete_topics[] = $row['topic_id'];
				}
				$db->sql_freeresult($result);

				if (sizeof($delete_topics))
				{
					delete_topics('topic_id', $delete_topics, false);
				}
				unset($delete_topics);

				// Make sure shadow topics having no last post data being updated (this only rarely happens...)
				$sql = 'SELECT topic_id, topic_moved_id, topic_last_post_id, topic_first_post_id
					FROM ' . TOPICS_TABLE . '
					WHERE ' . $db->sql_in_set('topic_id', $moved_topics) . '
						AND topic_last_post_time = 0';
				$result = $db->sql_query($sql);

				$shadow_topic_data = $post_ids = array();
				while ($row = $db->sql_fetchrow($result))
				{
					$shadow_topic_data[$row['topic_moved_id']] = $row;
					$post_ids[] = $row['topic_last_post_id'];
					$post_ids[] = $row['topic_first_post_id'];
				}
				$db->sql_freeresult($result);

				$sync_shadow_topics = array();
				if (sizeof($post_ids))
				{
					$sql = 'SELECT p.post_id, p.topic_id, p.post_approved, p.poster_id, p.post_subject, p.post_username, p.post_time, u.username, u.user_colour
						FROM ' . POSTS_TABLE . ' p, ' . USERS_TABLE . ' u
						WHERE ' . $db->sql_in_set('p.post_id', $post_ids) . '
							AND u.user_id = p.poster_id';
					$result = $db->sql_query($sql);

					$post_ids = array();
					while ($row = $db->sql_fetchrow($result))
					{
						$topic_id = (int) $row['topic_id'];

						// Ok, there should be a shadow topic. If there isn't, then there's something wrong with the db.
						// However, there's not much we can do about it.
						if (!empty($shadow_topic_data[$topic_id]))
						{
							if ($row['post_id'] == $shadow_topic_data[$topic_id]['topic_first_post_id'])
							{
								$orig_topic_id = $shadow_topic_data[$topic_id]['topic_id'];

								if (!isset($sync_shadow_topics[$orig_topic_id]))
								{
									$sync_shadow_topics[$orig_topic_id] = array();
								}

								$sync_shadow_topics[$orig_topic_id]['topic_time'] = $row['post_time'];
								$sync_shadow_topics[$orig_topic_id]['topic_poster'] = $row['poster_id'];
								$sync_shadow_topics[$orig_topic_id]['topic_first_poster_name'] = ($row['poster_id'] == ANONYMOUS) ? $row['post_username'] : $row['username'];
								$sync_shadow_topics[$orig_topic_id]['topic_first_poster_colour'] = $row['user_colour'];
							}

							if ($row['post_id'] == $shadow_topic_data[$topic_id]['topic_last_post_id'])
							{
								$orig_topic_id = $shadow_topic_data[$topic_id]['topic_id'];

								if (!isset($sync_shadow_topics[$orig_topic_id]))
								{
									$sync_shadow_topics[$orig_topic_id] = array();
								}

								$sync_shadow_topics[$orig_topic_id]['topic_last_poster_id'] = $row['poster_id'];
								$sync_shadow_topics[$orig_topic_id]['topic_last_post_subject'] = $row['post_subject'];
								$sync_shadow_topics[$orig_topic_id]['topic_last_post_time'] = $row['post_time'];
								$sync_shadow_topics[$orig_topic_id]['topic_last_poster_name'] = ($row['poster_id'] == ANONYMOUS) ? $row['post_username'] : $row['username'];
								$sync_shadow_topics[$orig_topic_id]['topic_last_poster_colour'] = $row['user_colour'];
							}
						}
					}
					$db->sql_freeresult($result);

					$shadow_topic_data = array();

					// Update the information we collected
					if (sizeof($sync_shadow_topics))
					{
						foreach ($sync_shadow_topics as $sync_topic_id => $sql_ary)
						{
							$sql = 'UPDATE ' . TOPICS_TABLE . '
								SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
								WHERE topic_id = ' . $sync_topic_id;
							$db->sql_query($sql);
						}
					}
				}

				unset($sync_shadow_topics, $shadow_topic_data);
			}

			// approved becomes unapproved, and vice-versa
			if (sizeof($approved_unapproved_ids))
			{
				$sql = 'UPDATE ' . TOPICS_TABLE . '
					SET topic_approved = 1 - topic_approved
					WHERE ' . $db->sql_in_set('topic_id', $approved_unapproved_ids);
				$db->sql_query($sql);
			}
			unset($approved_unapproved_ids);

			// These are fields that will be synchronised
			$fieldnames = array('time', 'replies', 'replies_real', 'poster', 'first_post_id', 'first_poster_name', 'first_poster_colour', 'last_post_id', 'last_post_subject', 'last_post_time', 'last_poster_id', 'last_poster_name', 'last_poster_colour');

			if ($sync_extra)
			{
				// This routine assumes that post_reported values are correct
				// if they are not, use sync('post_reported') first
				$sql = 'SELECT t.topic_id, p.post_id
					FROM ' . TOPICS_TABLE . ' t, ' . POSTS_TABLE . " p
					$where_sql_and p.topic_id = t.topic_id
						AND p.post_reported = 1
					GROUP BY t.topic_id, p.post_id";
				$result = $db->sql_query($sql);

				$fieldnames[] = 'reported';
				while ($row = $db->sql_fetchrow($result))
				{
					$topic_data[intval($row['topic_id'])]['reported'] = 1;
				}
				$db->sql_freeresult($result);

				// This routine assumes that post_attachment values are correct
				// if they are not, use sync('post_attachment') first
				$sql = 'SELECT t.topic_id, p.post_id
					FROM ' . TOPICS_TABLE . ' t, ' . POSTS_TABLE . " p
					$where_sql_and p.topic_id = t.topic_id
						AND p.post_attachment = 1
					GROUP BY t.topic_id, p.post_id";
				$result = $db->sql_query($sql);

				$fieldnames[] = 'attachment';
				while ($row = $db->sql_fetchrow($result))
				{
					$topic_data[intval($row['topic_id'])]['attachment'] = 1;
				}
				$db->sql_freeresult($result);
			}

			foreach ($topic_data as $topic_id => $row)
			{
				$sql_ary = array();

				foreach ($fieldnames as $fieldname)
				{
					if (isset($row[$fieldname]) && isset($row['topic_' . $fieldname]) && $row['topic_' . $fieldname] != $row[$fieldname])
					{
						$sql_ary['topic_' . $fieldname] = $row[$fieldname];
					}
				}

				if (sizeof($sql_ary))
				{
					$sql = 'UPDATE ' . TOPICS_TABLE . '
						SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
						WHERE topic_id = ' . $topic_id;
					$db->sql_query($sql);

					$resync_forums[$row['forum_id']] = $row['forum_id'];
				}
			}
			unset($topic_data);

			// if some topics have been resync'ed then resync parent forums
			// except when we're only syncing a range, we don't want to sync forums during
			// batch processing.
			if ($resync_parents && sizeof($resync_forums) && $where_type != 'range')
			{
				sync('forum', 'forum_id', array_values($resync_forums), true, true);
			}
		break;
	}

	return;
}

/**
* Prune function
*/
function prune($forum_id, $prune_mode, $prune_date, $prune_flags = 0, $auto_sync = true)
{
	global $db;

	if (!is_array($forum_id))
	{
		$forum_id = array($forum_id);
	}

	if (!sizeof($forum_id))
	{
		return;
	}

	$sql_and = '';

	if (!($prune_flags & FORUM_FLAG_PRUNE_ANNOUNCE))
	{
		$sql_and .= ' AND topic_type <> ' . POST_ANNOUNCE;
	}

	if (!($prune_flags & FORUM_FLAG_PRUNE_STICKY))
	{
		$sql_and .= ' AND topic_type <> ' . POST_STICKY;
	}

	if ($prune_mode == 'posted')
	{
		$sql_and .= " AND topic_last_post_time < $prune_date";
	}

	if ($prune_mode == 'viewed')
	{
		$sql_and .= " AND topic_last_view_time < $prune_date";
	}

	$sql = 'SELECT topic_id
		FROM ' . TOPICS_TABLE . '
		WHERE ' . $db->sql_in_set('forum_id', $forum_id) . "
			AND poll_start = 0
			$sql_and";
	$result = $db->sql_query($sql);

	$topic_list = array();
	while ($row = $db->sql_fetchrow($result))
	{
		$topic_list[] = $row['topic_id'];
	}
	$db->sql_freeresult($result);

	if ($prune_flags & FORUM_FLAG_PRUNE_POLL)
	{
		$sql = 'SELECT topic_id
			FROM ' . TOPICS_TABLE . '
			WHERE ' . $db->sql_in_set('forum_id', $forum_id) . "
				AND poll_start > 0
				AND poll_last_vote < $prune_date
				$sql_and";
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$topic_list[] = $row['topic_id'];
		}
		$db->sql_freeresult($result);

		$topic_list = array_unique($topic_list);
	}

	return delete_topics('topic_id', $topic_list, $auto_sync, false);
}

/**
* Function auto_prune(), this function now relies on passed vars
*/
function auto_prune($forum_id, $prune_mode, $prune_flags, $prune_days, $prune_freq)
{
	global $db;

	$sql = 'SELECT forum_name
		FROM ' . FORUMS_TABLE . "
		WHERE forum_id = $forum_id";
	$result = $db->sql_query($sql, 3600);
	$row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	if ($row)
	{
		$prune_date = time() - ($prune_days * 86400);
		$next_prune = time() + ($prune_freq * 86400);

		prune($forum_id, $prune_mode, $prune_date, $prune_flags, true);

		$sql = 'UPDATE ' . FORUMS_TABLE . "
			SET prune_next = $next_prune
			WHERE forum_id = $forum_id";
		$db->sql_query($sql);

		add_log('admin', 'LOG_AUTO_PRUNE', $row['forum_name']);
	}

	return;
}

/**
* remove_comments will strip the sql comment lines out of an uploaded sql file
* specifically for mssql and postgres type files in the install....
*/
function remove_comments(&$output)
{
	$lines = explode("\n", $output);
	$output = '';

	// try to keep mem. use down
	$linecount = sizeof($lines);

	$in_comment = false;
	for ($i = 0; $i < $linecount; $i++)
	{
		if (trim($lines[$i]) == '/*')
		{
			$in_comment = true;
		}

		if (!$in_comment)
		{
			$output .= $lines[$i] . "\n";
		}

		if (trim($lines[$i]) == '*/')
		{
			$in_comment = false;
		}
	}

	unset($lines);
	return $output;
}

/**
* Cache moderators, called whenever permissions are changed via admin_permissions. Changes of username
* and group names must be carried through for the moderators table
*/
function cache_moderators()
{
	global $db, $cache, $auth, $phpbb_root_path, $phpEx;

	// Remove cached sql results
	$cache->destroy('sql', MODERATOR_CACHE_TABLE);

	// Clear table
	switch ($db->sql_layer)
	{
		case 'sqlite':
		case 'firebird':
			$db->sql_query('DELETE FROM ' . MODERATOR_CACHE_TABLE);
		break;

		default:
			$db->sql_query('TRUNCATE TABLE ' . MODERATOR_CACHE_TABLE);
		break;
	}

	// We add moderators who have forum moderator permissions without an explicit ACL_NEVER setting
	$hold_ary = $ug_id_ary = $sql_ary = array();

	// Grab all users having moderative options...
	$hold_ary = $auth->acl_user_raw_data(false, 'm_%', false);

	// Add users?
	if (sizeof($hold_ary))
	{
		// At least one moderative option warrants a display
		$ug_id_ary = array_keys($hold_ary);

		// Remove users who have group memberships with DENY moderator permissions
		$sql = $db->sql_build_query('SELECT', array(
			'SELECT'	=> 'a.forum_id, ug.user_id',

			'FROM'		=> array(
				ACL_OPTIONS_TABLE	=> 'o',
				USER_GROUP_TABLE	=> 'ug',
				ACL_GROUPS_TABLE	=> 'a'
			),

			'LEFT_JOIN'	=> array(
				array(
					'FROM'	=> array(ACL_ROLES_DATA_TABLE => 'r'),
					'ON'	=> 'a.auth_role_id = r.role_id'
				)
			),

			'WHERE'		=> '(o.auth_option_id = a.auth_option_id OR o.auth_option_id = r.auth_option_id)
				AND ((a.auth_setting = ' . ACL_NEVER . ' AND r.auth_setting IS NULL)
					OR r.auth_setting = ' . ACL_NEVER . ')
				AND a.group_id = ug.group_id
				AND ' . $db->sql_in_set('ug.user_id', $ug_id_ary) . "
				AND ug.user_pending = 0
				AND o.auth_option " . $db->sql_like_expression('m_' . $db->any_char),
		));
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			if (isset($hold_ary[$row['user_id']][$row['forum_id']]))
			{
				unset($hold_ary[$row['user_id']][$row['forum_id']]);
			}
		}
		$db->sql_freeresult($result);

		if (sizeof($hold_ary))
		{
			// Get usernames...
			$sql = 'SELECT user_id, username
				FROM ' . USERS_TABLE . '
				WHERE ' . $db->sql_in_set('user_id', array_keys($hold_ary));
			$result = $db->sql_query($sql);

			$usernames_ary = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$usernames_ary[$row['user_id']] = $row['username'];
			}

			foreach ($hold_ary as $user_id => $forum_id_ary)
			{
				// Do not continue if user does not exist
				if (!isset($usernames_ary[$user_id]))
				{
					continue;
				}

				foreach ($forum_id_ary as $forum_id => $auth_ary)
				{
					$sql_ary[] = array(
						'forum_id'		=> (int) $forum_id,
						'user_id'		=> (int) $user_id,
						'username'		=> (string) $usernames_ary[$user_id],
						'group_id'		=> 0,
						'group_name'	=> ''
					);
				}
			}
		}
	}

	// Now to the groups...
	$hold_ary = $auth->acl_group_raw_data(false, 'm_%', false);

	if (sizeof($hold_ary))
	{
		$ug_id_ary = array_keys($hold_ary);

		// Make sure not hidden or special groups are involved...
		$sql = 'SELECT group_name, group_id, group_type
			FROM ' . GROUPS_TABLE . '
			WHERE ' . $db->sql_in_set('group_id', $ug_id_ary);
		$result = $db->sql_query($sql);

		$groupnames_ary = array();
		while ($row = $db->sql_fetchrow($result))
		{
			if ($row['group_type'] == GROUP_HIDDEN || $row['group_type'] == GROUP_SPECIAL)
			{
				unset($hold_ary[$row['group_id']]);
			}

			$groupnames_ary[$row['group_id']] = $row['group_name'];
		}
		$db->sql_freeresult($result);

		foreach ($hold_ary as $group_id => $forum_id_ary)
		{
			// If there is no group, we do not assign it...
			if (!isset($groupnames_ary[$group_id]))
			{
				continue;
			}

			foreach ($forum_id_ary as $forum_id => $auth_ary)
			{
				$flag = false;
				foreach ($auth_ary as $auth_option => $setting)
				{
					// Make sure at least one ACL_YES option is set...
					if ($setting == ACL_YES)
					{
						$flag = true;
						break;
					}
				}

				if (!$flag)
				{
					continue;
				}

				$sql_ary[] = array(
					'forum_id'		=> (int) $forum_id,
					'user_id'		=> 0,
					'username'		=> '',
					'group_id'		=> (int) $group_id,
					'group_name'	=> (string) $groupnames_ary[$group_id]
				);
			}
		}
	}

	$db->sql_multi_insert(MODERATOR_CACHE_TABLE, $sql_ary);
}

/**
* View log
*/
function view_log($mode, &$log, &$log_count, $limit = 0, $offset = 0, $forum_id = 0, $topic_id = 0, $user_id = 0, $limit_days = 0, $sort_by = 'l.log_time DESC')
{
	global $db, $user, $auth, $phpEx, $phpbb_root_path, $phpbb_admin_path;

	$topic_id_list = $reportee_id_list = $is_auth = $is_mod = array();

	$profile_url = (defined('IN_ADMIN')) ? append_sid("{$phpbb_admin_path}index.$phpEx", 'i=users&amp;mode=overview') : append_sid("{$phpbb_root_path}memberlist.$phpEx", 'mode=viewprofile');

	switch ($mode)
	{
		case 'admin':
			$log_type = LOG_ADMIN;
			$sql_forum = '';
		break;

		case 'mod':
			$log_type = LOG_MOD;

			if ($topic_id)
			{
				$sql_forum = 'AND l.topic_id = ' . intval($topic_id);
			}
			else if (is_array($forum_id))
			{
				$sql_forum = 'AND ' . $db->sql_in_set('l.forum_id', array_map('intval', $forum_id));
			}
			else
			{
				$sql_forum = ($forum_id) ? 'AND l.forum_id = ' . intval($forum_id) : '';
			}
		break;

		case 'user':
			$log_type = LOG_USERS;
			$sql_forum = 'AND l.reportee_id = ' . (int) $user_id;
		break;
		
		case 'users':
			$log_type = LOG_USERS;
			$sql_forum = '';
		break;

		case 'critical':
			$log_type = LOG_CRITICAL;
			$sql_forum = '';
		break;
		
		default:
			return;
	}

	$sql = "SELECT l.*, u.username, u.username_clean, u.user_colour
		FROM " . LOG_TABLE . " l, " . USERS_TABLE . " u
		WHERE l.log_type = $log_type
			AND u.user_id = l.user_id
			" . (($limit_days) ? "AND l.log_time >= $limit_days" : '') . "
			$sql_forum
		ORDER BY $sort_by";
	$result = $db->sql_query_limit($sql, $limit, $offset);

	$i = 0;
	$log = array();
	while ($row = $db->sql_fetchrow($result))
	{
		if ($row['topic_id'])
		{
			$topic_id_list[] = $row['topic_id'];
		}

		if ($row['reportee_id'])
		{
			$reportee_id_list[] = $row['reportee_id'];
		}

		$log[$i] = array(
			'id'				=> $row['log_id'],

			'reportee_id'			=> $row['reportee_id'],
			'reportee_username'		=> '',
			'reportee_username_full'=> '',

			'user_id'			=> $row['user_id'],
			'username'			=> $row['username'],
			'username_full'		=> get_username_string('full', $row['user_id'], $row['username'], $row['user_colour'], false, $profile_url),

			'ip'				=> $row['log_ip'],
			'time'				=> $row['log_time'],
			'forum_id'			=> $row['forum_id'],
			'topic_id'			=> $row['topic_id'],

			'viewforum'			=> ($row['forum_id'] && $auth->acl_get('f_read', $row['forum_id'])) ? append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $row['forum_id']) : false,
			'action'			=> (isset($user->lang[$row['log_operation']])) ? $user->lang[$row['log_operation']] : '{' . ucfirst(str_replace('_', ' ', $row['log_operation'])) . '}',
		);

		if (!empty($row['log_data']))
		{
			$log_data_ary = unserialize($row['log_data']);

			if (isset($user->lang[$row['log_operation']]))
			{
				// We supress the warning about inappropriate number of passed parameters here due to possible changes within LOG strings from one version to another.
				$log[$i]['action'] = @vsprintf($log[$i]['action'], $log_data_ary);

				// If within the admin panel we do not censor text out
				if (defined('IN_ADMIN'))
				{
					$log[$i]['action'] = bbcode_nl2br($log[$i]['action']);
				}
				else
				{
					$log[$i]['action'] = bbcode_nl2br(censor_text($log[$i]['action']));
				}
			}
			else
			{
				$log[$i]['action'] .= '<br />' . implode('', $log_data_ary);
			}

			/* Apply make_clickable... has to be seen if it is for good. :/
			// Seems to be not for the moment, reconsider later...
			$log[$i]['action'] = make_clickable($log[$i]['action']);
			*/
		}

		$i++;
	}
	$db->sql_freeresult($result);

	if (sizeof($topic_id_list))
	{
		$topic_id_list = array_unique($topic_id_list);

		// This query is not really needed if move_topics() updates the forum_id field,
		// although it's also used to determine if the topic still exists in the database
		$sql = 'SELECT topic_id, forum_id
			FROM ' . TOPICS_TABLE . '
			WHERE ' . $db->sql_in_set('topic_id', array_map('intval', $topic_id_list));
		$result = $db->sql_query($sql);

		$default_forum_id = 0;

		while ($row = $db->sql_fetchrow($result))
		{
			if (!$row['forum_id'])
			{
				if ($auth->acl_getf_global('f_read'))
				{
					if (!$default_forum_id)
					{
						$sql = 'SELECT forum_id
							FROM ' . FORUMS_TABLE . '
							WHERE forum_type = ' . FORUM_POST;
						$f_result = $db->sql_query_limit($sql, 1);
						$default_forum_id = (int) $db->sql_fetchfield('forum_id', false, $f_result);
						$db->sql_freeresult($f_result);
					}

					$is_auth[$row['topic_id']] = $default_forum_id;
				}
			}
			else
			{
				if ($auth->acl_get('f_read', $row['forum_id']))
				{
					$is_auth[$row['topic_id']] = $row['forum_id'];
				}
			}

			if ($auth->acl_gets('a_', 'm_', $row['forum_id']))
			{
				$is_mod[$row['topic_id']] = $row['forum_id'];
			}
		}
		$db->sql_freeresult($result);

		foreach ($log as $key => $row)
		{
			$log[$key]['viewtopic'] = (isset($is_auth[$row['topic_id']])) ? append_sid("{$phpbb_root_path}viewtopic.$phpEx", 'f=' . $is_auth[$row['topic_id']] . '&amp;t=' . $row['topic_id']) : false;
			$log[$key]['viewlogs'] = (isset($is_mod[$row['topic_id']])) ? append_sid("{$phpbb_root_path}mcp.$phpEx", 'i=logs&amp;mode=topic_logs&amp;t=' . $row['topic_id'], true, $user->session_id) : false;
		}
	}

	if (sizeof($reportee_id_list))
	{
		$reportee_id_list = array_unique($reportee_id_list);
		$reportee_names_list = array();

		$sql = 'SELECT user_id, username, user_colour
			FROM ' . USERS_TABLE . '
			WHERE ' . $db->sql_in_set('user_id', $reportee_id_list);
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$reportee_names_list[$row['user_id']] = $row;
		}
		$db->sql_freeresult($result);

		foreach ($log as $key => $row)
		{
			if (!isset($reportee_names_list[$row['reportee_id']]))
			{
				continue;
			}

			$log[$key]['reportee_username'] = $reportee_names_list[$row['reportee_id']]['username'];
			$log[$key]['reportee_username_full'] = get_username_string('full', $row['reportee_id'], $reportee_names_list[$row['reportee_id']]['username'], $reportee_names_list[$row['reportee_id']]['user_colour'], false, $profile_url);
		}
	}

	$sql = 'SELECT COUNT(l.log_id) AS total_entries
		FROM ' . LOG_TABLE . " l
		WHERE l.log_type = $log_type
			AND l.log_time >= $limit_days
			$sql_forum";
	$result = $db->sql_query($sql);
	$log_count = (int) $db->sql_fetchfield('total_entries');
	$db->sql_freeresult($result);

	return;
}

/**
* Update foes - remove moderators and administrators from foe lists...
*/
function update_foes($group_id = false, $user_id = false)
{
	global $db, $auth;

	// update foes for some user
	if (is_array($user_id) && sizeof($user_id))
	{
		$sql = 'DELETE FROM ' . ZEBRA_TABLE . '
			WHERE ' . $db->sql_in_set('zebra_id', $user_id) . '
				AND foe = 1';
		$db->sql_query($sql);
		return;
	}

	// update foes for some group
	if (is_array($group_id) && sizeof($group_id))
	{
		// Grab group settings...
		$sql = $db->sql_build_query('SELECT', array(
			'SELECT'	=> 'a.group_id',

			'FROM'		=> array(
				ACL_OPTIONS_TABLE	=> 'ao',
				ACL_GROUPS_TABLE	=> 'a'
			),

			'LEFT_JOIN'	=> array(
				array(
					'FROM'	=> array(ACL_ROLES_DATA_TABLE => 'r'),
					'ON'	=> 'a.auth_role_id = r.role_id'
				),
			),

			'WHERE'		=> '(ao.auth_option_id = a.auth_option_id OR ao.auth_option_id = r.auth_option_id)
				AND ' . $db->sql_in_set('a.group_id', $group_id) . "
				AND ao.auth_option IN ('a_', 'm_')",

			'GROUP_BY'	=> 'a.group_id'
		));
		$result = $db->sql_query($sql);

		$groups = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$groups[] = (int) $row['group_id'];
		}
		$db->sql_freeresult($result);

		if (!sizeof($groups))
		{
			return;
		}

		switch ($db->sql_layer)
		{
			case 'mysqli':
			case 'mysql4':
				$sql = 'DELETE ' . (($db->sql_layer === 'mysqli' || version_compare($db->mysql_version, '4.1', '>=')) ? 'z.*' : ZEBRA_TABLE) . '
					FROM ' . ZEBRA_TABLE . ' z, ' . USER_GROUP_TABLE . ' ug
					WHERE z.zebra_id = ug.user_id
						AND z.foe = 1
						AND ' . $db->sql_in_set('ug.group_id', $groups);
				$db->sql_query($sql);
			break;

			default:
				$sql = 'SELECT user_id
					FROM ' . USER_GROUP_TABLE . '
					WHERE ' . $db->sql_in_set('group_id', $groups);
				$result = $db->sql_query($sql);

				$users = array();
				while ($row = $db->sql_fetchrow($result))
				{
					$users[] = (int) $row['user_id'];
				}
				$db->sql_freeresult($result);

				if (sizeof($users))
				{				
					$sql = 'DELETE FROM ' . ZEBRA_TABLE . '
						WHERE ' . $db->sql_in_set('zebra_id', $users) . '
							AND foe = 1';
					$db->sql_query($sql);
				}
			break;
		}

		return;
	}

	// update foes for everyone
	$perms = array();
	foreach ($auth->acl_get_list(false, array('a_', 'm_'), false) as $forum_id => $forum_ary)
	{
		foreach ($forum_ary as $auth_option => $user_ary)
		{
			$perms = array_merge($perms, $user_ary);
		}
	}

	if (sizeof($perms))
	{
		$sql = 'DELETE FROM ' . ZEBRA_TABLE . '
			WHERE ' . $db->sql_in_set('zebra_id', array_unique($perms)) . '
				AND foe = 1';
		$db->sql_query($sql);
	}
	unset($perms);
}

/**
* Lists inactive users
*/
function view_inactive_users(&$users, &$user_count, $limit = 0, $offset = 0, $limit_days = 0, $sort_by = 'user_inactive_time DESC')
{
	global $db, $user;

	$sql = 'SELECT COUNT(user_id) AS user_count
		FROM ' . USERS_TABLE . '
		WHERE user_type = ' . USER_INACTIVE .
		(($limit_days) ? " AND user_inactive_time >= $limit_days" : '');
	$result = $db->sql_query($sql);
	$user_count = (int) $db->sql_fetchfield('user_count');
	$db->sql_freeresult($result);

	if ($offset >= $user_count)
	{
		$offset = ($offset - $limit < 0) ? 0 : $offset - $limit;
	}

	$sql = 'SELECT user_id, username, user_regdate, user_lastvisit, user_inactive_time, user_inactive_reason
		FROM ' . USERS_TABLE . '
		WHERE user_type = ' . USER_INACTIVE .
		(($limit_days) ? " AND user_inactive_time >= $limit_days" : '') . "
		ORDER BY $sort_by";
	$result = $db->sql_query_limit($sql, $limit, $offset);

	while ($row = $db->sql_fetchrow($result))
	{
		$row['inactive_reason'] = $user->lang['INACTIVE_REASON_UNKNOWN'];
		switch ($row['user_inactive_reason'])
		{
			case INACTIVE_REGISTER:
				$row['inactive_reason'] = $user->lang['INACTIVE_REASON_REGISTER'];
			break;

			case INACTIVE_PROFILE:
				$row['inactive_reason'] = $user->lang['INACTIVE_REASON_PROFILE'];
			break;

			case INACTIVE_MANUAL:
				$row['inactive_reason'] = $user->lang['INACTIVE_REASON_MANUAL'];
			break;

			case INACTIVE_REMIND:
				$row['inactive_reason'] = $user->lang['INACTIVE_REASON_REMIND'];
			break;
		}
	
		$users[] = $row;
	}

	return $offset;
}

/**
* Lists warned users
*/
function view_warned_users(&$users, &$user_count, $limit = 0, $offset = 0, $limit_days = 0, $sort_by = 'user_warnings DESC')
{
	global $db;

	$sql = 'SELECT user_id, username, user_colour, user_warnings, user_last_warning
		FROM ' . USERS_TABLE . '
		WHERE user_warnings > 0
		' . (($limit_days) ? "AND user_last_warning >= $limit_days" : '') . "
		ORDER BY $sort_by";
	$result = $db->sql_query_limit($sql, $limit, $offset);
	$users = $db->sql_fetchrowset($result);
	$db->sql_freeresult($result);

	$sql = 'SELECT count(user_id) AS user_count
		FROM ' . USERS_TABLE . '
		WHERE user_warnings > 0
		' . (($limit_days) ? "AND user_last_warning >= $limit_days" : '');
	$result = $db->sql_query($sql);
	$user_count = (int) $db->sql_fetchfield('user_count');
	$db->sql_freeresult($result);

	return;
}

/**
* Get database size
* Currently only mysql and mssql are supported
*/
function get_database_size()
{
	global $db, $user, $table_prefix;

	$database_size = false;

	// This code is heavily influenced by a similar routine in phpMyAdmin 2.2.0
	switch ($db->sql_layer)
	{
		case 'mysql':
		case 'mysql4':
		case 'mysqli':
			$sql = 'SELECT VERSION() AS mysql_version';
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			if ($row)
			{
				$version = $row['mysql_version'];

				if (preg_match('#(3\.23|[45]\.)#', $version))
				{
					$db_name = (preg_match('#^(?:3\.23\.(?:[6-9]|[1-9]{2}))|[45]\.#', $version)) ? "`{$db->dbname}`" : $db->dbname;

					$sql = 'SHOW TABLE STATUS
						FROM ' . $db_name;
					$result = $db->sql_query($sql, 7200);

					$database_size = 0;
					while ($row = $db->sql_fetchrow($result))
					{
						if ((isset($row['Type']) && $row['Type'] != 'MRG_MyISAM') || (isset($row['Engine']) && ($row['Engine'] == 'MyISAM' || $row['Engine'] == 'InnoDB')))
						{
							if ($table_prefix != '')
							{
								if (strpos($row['Name'], $table_prefix) !== false)
								{
									$database_size += $row['Data_length'] + $row['Index_length'];
								}
							}
							else
							{
								$database_size += $row['Data_length'] + $row['Index_length'];
							}
						}
					}
					$db->sql_freeresult($result);
				}
			}
		break;

		case 'firebird':
			global $dbname;

			// if it on the local machine, we can get lucky
			if (file_exists($dbname))
			{
				$database_size = filesize($dbname);
			}

		break;

		case 'sqlite':
			global $dbhost;

			if (file_exists($dbhost))
			{
				$database_size = filesize($dbhost);
			}

		break;

		case 'mssql':
		case 'mssql_odbc':
			$sql = 'SELECT ((SUM(size) * 8.0) * 1024.0) as dbsize
				FROM sysfiles';
			$result = $db->sql_query($sql, 7200);
			$database_size = ($row = $db->sql_fetchrow($result)) ? $row['dbsize'] : false;
			$db->sql_freeresult($result);
		break;

		case 'postgres':
			$sql = "SELECT proname
				FROM pg_proc
				WHERE proname = 'pg_database_size'";
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			if ($row['proname'] == 'pg_database_size')
			{
				$database = $db->dbname;
				if (strpos($database, '.') !== false)
				{
					list($database, ) = explode('.', $database);
				}

				$sql = "SELECT oid
					FROM pg_database
					WHERE datname = '$database'";
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				$oid = $row['oid'];

				$sql = 'SELECT pg_database_size(' . $oid . ') as size';
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				$database_size = $row['size'];
			}
		break;

		case 'oracle':
			$sql = 'SELECT SUM(bytes) as dbsize
				FROM user_segments';
			$result = $db->sql_query($sql, 7200);
			$database_size = ($row = $db->sql_fetchrow($result)) ? $row['dbsize'] : false;
			$db->sql_freeresult($result);
		break;
	}

	$database_size = ($database_size !== false) ? get_formatted_filesize($database_size) : $user->lang['NOT_AVAILABLE'];

	return $database_size;
}

/**
* Retrieve contents from remotely stored file
*/
function get_remote_file($host, $directory, $filename, &$errstr, &$errno, $port = 80, $timeout = 10)
{
	global $user;

	if ($fsock = @fsockopen($host, $port, $errno, $errstr, $timeout))
	{
		@fputs($fsock, "GET $directory/$filename HTTP/1.1\r\n");
		@fputs($fsock, "HOST: $host\r\n");
		@fputs($fsock, "Connection: close\r\n\r\n");
	
		$file_info = '';
		$get_info = false;

		while (!@feof($fsock))
		{
			if ($get_info)
			{
				$file_info .= @fread($fsock, 1024);
			}
			else
			{
				$line = @fgets($fsock, 1024);
				if ($line == "\r\n")
				{
					$get_info = true;
				}
				else if (stripos($line, '404 not found') !== false)
				{
					$errstr = $user->lang['FILE_NOT_FOUND'] . ': ' . $filename;
					return false;
				}
			}
		}
		@fclose($fsock);
	}
	else
	{
		if ($errstr)
		{
			$errstr = utf8_convert_message($errstr);
			return false;
		}
		else
		{
			$errstr = $user->lang['FSOCK_DISABLED'];
			return false;
		}
	}
	
	return $file_info;
}

/**
* Tidy Warnings
* Remove all warnings which have now expired from the database
* The duration of a warning can be defined by the administrator
* This only removes the warning and reduces the associated count,
* it does not remove the user note recording the contents of the warning
*/
function tidy_warnings()
{
	global $db, $config;

	$expire_date = time() - ($config['warnings_expire_days'] * 86400);
	$warning_list = $user_list = array();

	$sql = 'SELECT * FROM ' . WARNINGS_TABLE . "
		WHERE warning_time < $expire_date";
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		$warning_list[] = $row['warning_id'];
		$user_list[$row['user_id']] = isset($user_list[$row['user_id']]) ? ++$user_list[$row['user_id']] : 1;
	}
	$db->sql_freeresult($result);

	if (sizeof($warning_list))
	{
		$db->sql_transaction('begin');

		$sql = 'DELETE FROM ' . WARNINGS_TABLE . '
			WHERE ' . $db->sql_in_set('warning_id', $warning_list);
		$db->sql_query($sql);
	
		foreach ($user_list as $user_id => $value)
		{
			$sql = 'UPDATE ' . USERS_TABLE . " SET user_warnings = user_warnings - $value
				WHERE user_id = $user_id";
			$db->sql_query($sql);
		}

		$db->sql_transaction('commit');
	}

	set_config('warnings_last_gc', time(), true);
}

/**
* Tidy database, doing some maintanance tasks
*/
function tidy_database()
{
	global $db;

	// Here we check permission consistency

	// Sometimes, it can happen permission tables having forums listed which do not exist
	$sql = 'SELECT forum_id
		FROM ' . FORUMS_TABLE;
	$result = $db->sql_query($sql);

	$forum_ids = array(0);
	while ($row = $db->sql_fetchrow($result))
	{
		$forum_ids[] = $row['forum_id'];
	}
	$db->sql_freeresult($result);

	// Delete those rows from the acl tables not having listed the forums above
	$sql = 'DELETE FROM ' . ACL_GROUPS_TABLE . '
		WHERE ' . $db->sql_in_set('forum_id', $forum_ids, true);
	$db->sql_query($sql);

	$sql = 'DELETE FROM ' . ACL_USERS_TABLE . '
		WHERE ' . $db->sql_in_set('forum_id', $forum_ids, true);
	$db->sql_query($sql);

	set_config('database_last_gc', time(), true);
}

/**
* Add permission language - this will make sure custom files will be included
*/
function add_permission_language()
{
	global $user, $phpEx;

	// First of all, our own file. We need to include it as the first file because it presets all relevant variables.
	$user->add_lang('acp/permissions_phpbb');

	$files_to_add = array();

	// Now search in acp and mods folder for permissions_ files.
	foreach (array('acp/', 'mods/') as $path)
	{
		$dh = @opendir($user->lang_path . $path);

		if ($dh)
		{
			while (($file = readdir($dh)) !== false)
			{
				if ($file !== 'permissions_phpbb.' . $phpEx && strpos($file, 'permissions_') === 0 && substr($file, -(strlen($phpEx) + 1)) === '.' . $phpEx)
				{
					$files_to_add[] = $path . substr($file, 0, -(strlen($phpEx) + 1));
				}
			}
			closedir($dh);
		}
	}

	if (!sizeof($files_to_add))
	{
		return false;
	}

	$user->add_lang($files_to_add);
	return true;
}

?>