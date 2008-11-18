<?php
/**
*
* @package phpBB3
* @version $Id: functions_posting.php 8479 2008-03-29 00:22:48Z naderman $
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
* Fill smiley templates (or just the variables) with smilies, either in a window or inline
*/
function generate_smilies($mode, $forum_id)
{
	global $auth, $db, $user, $config, $template;
	global $phpEx, $phpbb_root_path;

	if ($mode == 'window')
	{
		if ($forum_id)
		{
			$sql = 'SELECT forum_style
				FROM ' . FORUMS_TABLE . "
				WHERE forum_id = $forum_id";
			$result = $db->sql_query_limit($sql, 1);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			$user->setup('posting', (int) $row['forum_style']);
		}
		else
		{
			$user->setup('posting');
		}

		page_header($user->lang['SMILIES']);

		$template->set_filenames(array(
			'body' => 'posting_smilies.html')
		);
	}

	$display_link = false;
	if ($mode == 'inline')
	{
		$sql = 'SELECT smiley_id
			FROM ' . SMILIES_TABLE . '
			WHERE display_on_posting = 0';
		$result = $db->sql_query_limit($sql, 1, 0, 3600);

		if ($row = $db->sql_fetchrow($result))
		{
			$display_link = true;
		}
		$db->sql_freeresult($result);
	}

	$last_url = '';

	$sql = 'SELECT *
		FROM ' . SMILIES_TABLE .
		(($mode == 'inline') ? ' WHERE display_on_posting = 1 ' : '') . '
		ORDER BY smiley_order';
	$result = $db->sql_query($sql, 3600);

	$smilies = array();
	while ($row = $db->sql_fetchrow($result))
	{
		if (empty($smilies[$row['smiley_url']]))
		{
			$smilies[$row['smiley_url']] = $row;
		}
	}
	$db->sql_freeresult($result);

	if (sizeof($smilies))
	{
		foreach ($smilies as $row)
		{
			$template->assign_block_vars('smiley', array(
				'SMILEY_CODE'	=> $row['code'],
				'A_SMILEY_CODE'	=> addslashes($row['code']),
				'SMILEY_IMG'	=> $phpbb_root_path . $config['smilies_path'] . '/' . $row['smiley_url'],
				'SMILEY_WIDTH'	=> $row['smiley_width'],
				'SMILEY_HEIGHT'	=> $row['smiley_height'],
				'SMILEY_DESC'	=> $row['emotion'])
			);
		}
	}

	if ($mode == 'inline' && $display_link)
	{
		$template->assign_vars(array(
			'S_SHOW_SMILEY_LINK' 	=> true,
			'U_MORE_SMILIES' 		=> append_sid("{$phpbb_root_path}posting.$phpEx", 'mode=smilies&amp;f=' . $forum_id))
		);
	}

	if ($mode == 'window')
	{
		page_footer();
	}
}

/**
* Update last post information
* Should be used instead of sync() if only the last post information are out of sync... faster
*
* @param	string	$type				Can be forum|topic
* @param	mixed	$ids				topic/forum ids
* @param	bool	$return_update_sql	true: SQL query shall be returned, false: execute SQL
*/
function update_post_information($type, $ids, $return_update_sql = false)
{
	global $db;

	if (empty($ids))
	{
		return;
	}
	if (!is_array($ids))
	{
		$ids = array($ids);
	}


	$update_sql = $empty_forums = $not_empty_forums = array();

	if ($type != 'topic')
	{
		$topic_join = ', ' . TOPICS_TABLE . ' t';
		$topic_condition = 'AND t.topic_id = p.topic_id AND t.topic_approved = 1';
	}
	else
	{
		$topic_join = '';
		$topic_condition = '';
	}

	if (sizeof($ids) == 1)
	{
		$sql = 'SELECT MAX(p.post_id) as last_post_id
			FROM ' . POSTS_TABLE . " p $topic_join
			WHERE " . $db->sql_in_set('p.' . $type . '_id', $ids) . "
				$topic_condition
				AND p.post_approved = 1";
	}
	else
	{
		$sql = 'SELECT p.' . $type . '_id, MAX(p.post_id) as last_post_id
			FROM ' . POSTS_TABLE . " p $topic_join
			WHERE " . $db->sql_in_set('p.' . $type . '_id', $ids) . "
				$topic_condition
				AND p.post_approved = 1
			GROUP BY p.{$type}_id";
	}
	$result = $db->sql_query($sql);

	$last_post_ids = array();
	while ($row = $db->sql_fetchrow($result))
	{
		if (sizeof($ids) == 1)
		{
			$row[$type . '_id'] = $ids[0];
		}

		if ($type == 'forum')
		{
			$not_empty_forums[] = $row['forum_id'];

			if (empty($row['last_post_id']))
			{
				$empty_forums[] = $row['forum_id'];
			}
		}

		$last_post_ids[] = $row['last_post_id'];
	}
	$db->sql_freeresult($result);

	if ($type == 'forum')
	{
		$empty_forums = array_merge($empty_forums, array_diff($ids, $not_empty_forums));

		foreach ($empty_forums as $void => $forum_id)
		{
			$update_sql[$forum_id][] = 'forum_last_post_id = 0';
			$update_sql[$forum_id][] = "forum_last_post_subject = ''";
			$update_sql[$forum_id][] = 'forum_last_post_time = 0';
			$update_sql[$forum_id][] = 'forum_last_poster_id = 0';
			$update_sql[$forum_id][] = "forum_last_poster_name = ''";
			$update_sql[$forum_id][] = "forum_last_poster_colour = ''";
		}
	}

	if (sizeof($last_post_ids))
	{
		$sql = 'SELECT p.' . $type . '_id, p.post_id, p.post_subject, p.post_time, p.poster_id, p.post_username, u.user_id, u.username, u.user_colour
			FROM ' . POSTS_TABLE . ' p, ' . USERS_TABLE . ' u
			WHERE p.poster_id = u.user_id
				AND ' . $db->sql_in_set('p.post_id', $last_post_ids);
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$update_sql[$row["{$type}_id"]][] = $type . '_last_post_id = ' . (int) $row['post_id'];
			$update_sql[$row["{$type}_id"]][] = "{$type}_last_post_subject = '" . $db->sql_escape($row['post_subject']) . "'";
			$update_sql[$row["{$type}_id"]][] = $type . '_last_post_time = ' . (int) $row['post_time'];
			$update_sql[$row["{$type}_id"]][] = $type . '_last_poster_id = ' . (int) $row['poster_id'];
			$update_sql[$row["{$type}_id"]][] = "{$type}_last_poster_colour = '" . $db->sql_escape($row['user_colour']) . "'";
			$update_sql[$row["{$type}_id"]][] = "{$type}_last_poster_name = '" . (($row['poster_id'] == ANONYMOUS) ? $db->sql_escape($row['post_username']) : $db->sql_escape($row['username'])) . "'";
		}
		$db->sql_freeresult($result);
	}
	unset($empty_forums, $ids, $last_post_ids);

	if ($return_update_sql || !sizeof($update_sql))
	{
		return $update_sql;
	}

	$table = ($type == 'forum') ? FORUMS_TABLE : TOPICS_TABLE;

	foreach ($update_sql as $update_id => $update_sql_ary)
	{
		$sql = "UPDATE $table
			SET " . implode(', ', $update_sql_ary) . "
			WHERE {$type}_id = $update_id";
		$db->sql_query($sql);
	}

	return;
}

/**
* Generate Topic Icons for display
*/
function posting_gen_topic_icons($mode, $icon_id)
{
	global $phpbb_root_path, $config, $template, $cache;

	// Grab icons
	$icons = $cache->obtain_icons();

	if (!$icon_id)
	{
		$template->assign_var('S_NO_ICON_CHECKED', ' checked="checked"');
	}

	if (sizeof($icons))
	{
		foreach ($icons as $id => $data)
		{
			if ($data['display'])
			{
				$template->assign_block_vars('topic_icon', array(
					'ICON_ID'		=> $id,
					'ICON_IMG'		=> $phpbb_root_path . $config['icons_path'] . '/' . $data['img'],
					'ICON_WIDTH'	=> $data['width'],
					'ICON_HEIGHT'	=> $data['height'],

					'S_CHECKED'			=> ($id == $icon_id) ? true : false,
					'S_ICON_CHECKED'	=> ($id == $icon_id) ? ' checked="checked"' : '')
				);
			}
		}

		return true;
	}

	return false;
}

/**
* Build topic types able to be selected
*/
function posting_gen_topic_types($forum_id, $cur_topic_type = POST_NORMAL)
{
	global $auth, $user, $template, $topic_type;

	$toggle = false;

	$topic_types = array(
		'sticky'	=> array('const' => POST_STICKY, 'lang' => 'POST_STICKY'),
		'announce'	=> array('const' => POST_ANNOUNCE, 'lang' => 'POST_ANNOUNCEMENT'),
		'global'	=> array('const' => POST_GLOBAL, 'lang' => 'POST_GLOBAL')
	);

	$topic_type_array = array();

	foreach ($topic_types as $auth_key => $topic_value)
	{
		// We do not have a special post global announcement permission
		$auth_key = ($auth_key == 'global') ? 'announce' : $auth_key;

		if ($auth->acl_get('f_' . $auth_key, $forum_id))
		{
			$toggle = true;

			$topic_type_array[] = array(
				'VALUE'			=> $topic_value['const'],
				'S_CHECKED'		=> ($cur_topic_type == $topic_value['const'] || ($forum_id == 0 && $topic_value['const'] == POST_GLOBAL)) ? ' checked="checked"' : '',
				'L_TOPIC_TYPE'	=> $user->lang[$topic_value['lang']]
			);
		}
	}

	if ($toggle)
	{
		$topic_type_array = array_merge(array(0 => array(
			'VALUE'			=> POST_NORMAL,
			'S_CHECKED'		=> ($topic_type == POST_NORMAL) ? ' checked="checked"' : '',
			'L_TOPIC_TYPE'	=> $user->lang['POST_NORMAL'])),

			$topic_type_array
		);

		foreach ($topic_type_array as $array)
		{
			$template->assign_block_vars('topic_type', $array);
		}

		$template->assign_vars(array(
			'S_TOPIC_TYPE_STICKY'	=> ($auth->acl_get('f_sticky', $forum_id)),
			'S_TOPIC_TYPE_ANNOUNCE'	=> ($auth->acl_get('f_announce', $forum_id)))
		);
	}

	return $toggle;
}

//
// Attachment related functions
//

/**
* Upload Attachment - filedata is generated here
* Uses upload class
*/
function upload_attachment($form_name, $forum_id, $local = false, $local_storage = '', $is_message = false, $local_filedata = false)
{
	global $auth, $user, $config, $db, $cache;
	global $phpbb_root_path, $phpEx;

	$filedata = array(
		'error'	=> array()
	);

	include_once($phpbb_root_path . 'includes/functions_upload.' . $phpEx);
	$upload = new fileupload();

	if (!$local)
	{
		$filedata['post_attach'] = ($upload->is_valid($form_name)) ? true : false;
	}
	else
	{
		$filedata['post_attach'] = true;
	}

	if (!$filedata['post_attach'])
	{
		$filedata['error'][] = $user->lang['NO_UPLOAD_FORM_FOUND'];
		return $filedata;
	}

	$extensions = $cache->obtain_attach_extensions((($is_message) ? false : (int) $forum_id));
	$upload->set_allowed_extensions(array_keys($extensions['_allowed_']));

	$file = ($local) ? $upload->local_upload($local_storage, $local_filedata) : $upload->form_upload($form_name);

	if ($file->init_error)
	{
		$filedata['post_attach'] = false;
		return $filedata;
	}

	$cat_id = (isset($extensions[$file->get('extension')]['display_cat'])) ? $extensions[$file->get('extension')]['display_cat'] : ATTACHMENT_CATEGORY_NONE;

	// Make sure the image category only holds valid images...
	if ($cat_id == ATTACHMENT_CATEGORY_IMAGE && !$file->is_image())
	{
		$file->remove();

		// If this error occurs a user tried to exploit an IE Bug by renaming extensions
		// Since the image category is displaying content inline we need to catch this.
		trigger_error($user->lang['ATTACHED_IMAGE_NOT_IMAGE']);
	}

	// Do we have to create a thumbnail?
	$filedata['thumbnail'] = ($cat_id == ATTACHMENT_CATEGORY_IMAGE && $config['img_create_thumbnail']) ? 1 : 0;

	// Check Image Size, if it is an image
	if (!$auth->acl_get('a_') && !$auth->acl_get('m_', $forum_id) && $cat_id == ATTACHMENT_CATEGORY_IMAGE)
	{
		$file->upload->set_allowed_dimensions(0, 0, $config['img_max_width'], $config['img_max_height']);
	}

	// Admins and mods are allowed to exceed the allowed filesize
	if (!$auth->acl_get('a_') && !$auth->acl_get('m_', $forum_id))
	{
		if (!empty($extensions[$file->get('extension')]['max_filesize']))
		{
			$allowed_filesize = $extensions[$file->get('extension')]['max_filesize'];
		}
		else
		{
			$allowed_filesize = ($is_message) ? $config['max_filesize_pm'] : $config['max_filesize'];
		}

		$file->upload->set_max_filesize($allowed_filesize);
	}

	$file->clean_filename('unique', $user->data['user_id'] . '_');

	// Are we uploading an image *and* this image being within the image category? Only then perform additional image checks.
	$no_image = ($cat_id == ATTACHMENT_CATEGORY_IMAGE) ? false : true;

	$file->move_file($config['upload_path'], false, $no_image);

	if (sizeof($file->error))
	{
		$file->remove();
		$filedata['error'] = array_merge($filedata['error'], $file->error);
		$filedata['post_attach'] = false;

		return $filedata;
	}

	$filedata['filesize'] = $file->get('filesize');
	$filedata['mimetype'] = $file->get('mimetype');
	$filedata['extension'] = $file->get('extension');
	$filedata['physical_filename'] = $file->get('realname');
	$filedata['real_filename'] = $file->get('uploadname');
	$filedata['filetime'] = time();

	// Check our complete quota
	if ($config['attachment_quota'])
	{
		if ($config['upload_dir_size'] + $file->get('filesize') > $config['attachment_quota'])
		{
			$filedata['error'][] = $user->lang['ATTACH_QUOTA_REACHED'];
			$filedata['post_attach'] = false;

			$file->remove();

			return $filedata;
		}
	}

	// Check free disk space
	if ($free_space = @disk_free_space($phpbb_root_path . $config['upload_path']))
	{
		if ($free_space <= $file->get('filesize'))
		{
			$filedata['error'][] = $user->lang['ATTACH_QUOTA_REACHED'];
			$filedata['post_attach'] = false;

			$file->remove();

			return $filedata;
		}
	}

	// Create Thumbnail
	if ($filedata['thumbnail'])
	{
		$source = $file->get('destination_file');
		$destination = $file->get('destination_path') . '/thumb_' . $file->get('realname');

		if (!create_thumbnail($source, $destination, $file->get('mimetype')))
		{
			$filedata['thumbnail'] = 0;
		}
	}

	return $filedata;
}

/**
* Calculate the needed size for Thumbnail
*/
function get_img_size_format($width, $height)
{
	global $config;

	// Maximum Width the Image can take
	$max_width = ($config['img_max_thumb_width']) ? $config['img_max_thumb_width'] : 400;

	if ($width > $height)
	{
		return array(
			round($width * ($max_width / $width)),
			round($height * ($max_width / $width))
		);
	}
	else
	{
		return array(
			round($width * ($max_width / $height)),
			round($height * ($max_width / $height))
		);
	}
}

/**
* Return supported image types
*/
function get_supported_image_types($type = false)
{
	if (@extension_loaded('gd'))
	{
		$format = imagetypes();
		$new_type = 0;

		if ($type !== false)
		{
			switch ($type)
			{
				// GIF
				case 1:
					$new_type = ($format & IMG_GIF) ? IMG_GIF : false;
				break;

				// JPG, JPC, JP2
				case 2:
				case 9:
				case 10:
				case 11:
				case 12:
					$new_type = ($format & IMG_JPG) ? IMG_JPG : false;
				break;

				// PNG
				case 3:
					$new_type = ($format & IMG_PNG) ? IMG_PNG : false;
				break;

				// BMP, WBMP
				case 6:
				case 15:
					$new_type = ($format & IMG_WBMP) ? IMG_WBMP : false;
				break;
			}
		}
		else
		{
			$new_type = array();
			$go_through_types = array(IMG_GIF, IMG_JPG, IMG_PNG, IMG_WBMP);

			foreach ($go_through_types as $check_type)
			{
				if ($format & $check_type)
				{
					$new_type[] = $check_type;
				}
			}
		}

		return array(
			'gd'		=> ($new_type) ? true : false,
			'format'	=> $new_type,
			'version'	=> (function_exists('imagecreatetruecolor')) ? 2 : 1
		);
	}

	return array('gd' => false);
}

/**
* Create Thumbnail
*/
function create_thumbnail($source, $destination, $mimetype)
{
	global $config;

	$min_filesize = (int) $config['img_min_thumb_filesize'];
	$img_filesize = (file_exists($source)) ? @filesize($source) : false;

	if (!$img_filesize || $img_filesize <= $min_filesize)
	{
		return false;
	}

	$dimension = @getimagesize($source);

	if ($dimension === false)
	{
		return false;
	}

	list($width, $height, $type, ) = $dimension;

	if (empty($width) || empty($height))
	{
		return false;
	}

	list($new_width, $new_height) = get_img_size_format($width, $height);

	// Do not create a thumbnail if the resulting width/height is bigger than the original one
	if ($new_width > $width && $new_height > $height)
	{
		return false;
	}

	$used_imagick = false;

	// Only use imagemagick if defined and the passthru function not disabled
	if ($config['img_imagick'] && function_exists('passthru'))
	{
		if (substr($config['img_imagick'], -1) !== '/')
		{
			$config['img_imagick'] .= '/';
		}

		@passthru(escapeshellcmd($config['img_imagick']) . 'convert' . ((defined('PHP_OS') && preg_match('#^win#i', PHP_OS)) ? '.exe' : '') . ' -quality 85 -antialias -sample ' . $new_width . 'x' . $new_height . ' "' . str_replace('\\', '/', $source) . '" +profile "*" "' . str_replace('\\', '/', $destination) . '"');

		if (file_exists($destination))
		{
			$used_imagick = true;
		}
	}

	if (!$used_imagick)
	{
		$type = get_supported_image_types($type);

		if ($type['gd'])
		{
			// If the type is not supported, we are not able to create a thumbnail
			if ($type['format'] === false)
			{
				return false;
			}

			switch ($type['format'])
			{
				case IMG_GIF:
					$image = @imagecreatefromgif($source);
				break;

				case IMG_JPG:
					$image = @imagecreatefromjpeg($source);
				break;

				case IMG_PNG:
					$image = @imagecreatefrompng($source);
				break;

				case IMG_WBMP:
					$image = @imagecreatefromwbmp($source);
				break;
			}

			if ($type['version'] == 1)
			{
				$new_image = imagecreate($new_width, $new_height);

				if ($new_image === false)
				{
					return false;
				}

				imagecopyresized($new_image, $image, 0, 0, 0, 0, $new_width, $new_height, $width, $height);
			}
			else
			{
				$new_image = imagecreatetruecolor($new_width, $new_height);

				if ($new_image === false)
				{
					return false;
				}

				imagecopyresampled($new_image, $image, 0, 0, 0, 0, $new_width, $new_height, $width, $height);
			}

			// If we are in safe mode create the destination file prior to using the gd functions to circumvent a PHP bug
			if (@ini_get('safe_mode') || @strtolower(ini_get('safe_mode')) == 'on')
			{
				@touch($destination);
			}

			switch ($type['format'])
			{
				case IMG_GIF:
					imagegif($new_image, $destination);
				break;

				case IMG_JPG:
					imagejpeg($new_image, $destination, 90);
				break;

				case IMG_PNG:
					imagepng($new_image, $destination);
				break;

				case IMG_WBMP:
					imagewbmp($new_image, $destination);
				break;
			}

			imagedestroy($new_image);
		}
		else
		{
			return false;
		}
	}

	if (!file_exists($destination))
	{
		return false;
	}

	@chmod($destination, 0666);

	return true;
}

/**
* Assign Inline attachments (build option fields)
*/
function posting_gen_inline_attachments(&$attachment_data)
{
	global $template;

	if (sizeof($attachment_data))
	{
		$s_inline_attachment_options = '';

		foreach ($attachment_data as $i => $attachment)
		{
			$s_inline_attachment_options .= '<option value="' . $i . '">' . basename($attachment['real_filename']) . '</option>';
		}

		$template->assign_var('S_INLINE_ATTACHMENT_OPTIONS', $s_inline_attachment_options);

		return true;
	}

	return false;
}

/**
* Generate inline attachment entry
*/
function posting_gen_attachment_entry($attachment_data, &$filename_data)
{
	global $template, $config, $phpbb_root_path, $phpEx, $user;

	$template->assign_vars(array(
		'S_SHOW_ATTACH_BOX'	=> true)
	);

	if (sizeof($attachment_data))
	{
		$template->assign_vars(array(
			'S_HAS_ATTACHMENTS'	=> true)
		);

		// We display the posted attachments within the desired order.
		($config['display_order']) ? krsort($attachment_data) : ksort($attachment_data);

		foreach ($attachment_data as $count => $attach_row)
		{
			$hidden = '';
			$attach_row['real_filename'] = basename($attach_row['real_filename']);

			foreach ($attach_row as $key => $value)
			{
				$hidden .= '<input type="hidden" name="attachment_data[' . $count . '][' . $key . ']" value="' . $value . '" />';
			}

			$download_link = append_sid("{$phpbb_root_path}download/file.$phpEx", 'mode=view&amp;id=' . (int) $attach_row['attach_id'], true, ($attach_row['is_orphan']) ? $user->session_id : false);

			$template->assign_block_vars('attach_row', array(
				'FILENAME'			=> basename($attach_row['real_filename']),
				'A_FILENAME'		=> addslashes(basename($attach_row['real_filename'])),
				'FILE_COMMENT'		=> $attach_row['attach_comment'],
				'ATTACH_ID'			=> $attach_row['attach_id'],
				'S_IS_ORPHAN'		=> $attach_row['is_orphan'],
				'ASSOC_INDEX'		=> $count,

				'U_VIEW_ATTACHMENT'	=> $download_link,
				'S_HIDDEN'			=> $hidden)
			);
		}
	}

	$template->assign_vars(array(
		'FILE_COMMENT'	=> $filename_data['filecomment'],
		'FILESIZE'		=> $config['max_filesize'])
	);

	return sizeof($attachment_data);
}

//
// General Post functions
//

/**
* Load Drafts
*/
function load_drafts($topic_id = 0, $forum_id = 0, $id = 0)
{
	global $user, $db, $template, $auth;
	global $phpbb_root_path, $phpEx;

	$topic_ids = $forum_ids = $draft_rows = array();

	// Load those drafts not connected to forums/topics
	// If forum_id == 0 AND topic_id == 0 then this is a PM draft
	if (!$topic_id && !$forum_id)
	{
		$sql_and = ' AND d.forum_id = 0 AND d.topic_id = 0';
	}
	else
	{
		$sql_and = '';
		$sql_and .= ($forum_id) ? ' AND d.forum_id = ' . (int) $forum_id : '';
		$sql_and .= ($topic_id) ? ' AND d.topic_id = ' . (int) $topic_id : '';
	}

	$sql = 'SELECT d.*, f.forum_id, f.forum_name
		FROM ' . DRAFTS_TABLE . ' d
		LEFT JOIN ' . FORUMS_TABLE . ' f ON (f.forum_id = d.forum_id)
			WHERE d.user_id = ' . $user->data['user_id'] . "
			$sql_and
		ORDER BY d.save_time DESC";
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		if ($row['topic_id'])
		{
			$topic_ids[] = (int) $row['topic_id'];
		}
		$draft_rows[] = $row;
	}
	$db->sql_freeresult($result);

	if (!sizeof($draft_rows))
	{
		return;
	}

	$topic_rows = array();
	if (sizeof($topic_ids))
	{
		$sql = 'SELECT topic_id, forum_id, topic_title
			FROM ' . TOPICS_TABLE . '
			WHERE ' . $db->sql_in_set('topic_id', array_unique($topic_ids));
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$topic_rows[$row['topic_id']] = $row;
		}
		$db->sql_freeresult($result);
	}
	unset($topic_ids);

	$template->assign_var('S_SHOW_DRAFTS', true);

	foreach ($draft_rows as $draft)
	{
		$link_topic = $link_forum = $link_pm = false;
		$insert_url = $view_url = $title = '';

		if (isset($topic_rows[$draft['topic_id']])
			&& (
				($topic_rows[$draft['topic_id']]['forum_id'] && $auth->acl_get('f_read', $topic_rows[$draft['topic_id']]['forum_id']))
				||
				(!$topic_rows[$draft['topic_id']]['forum_id'] && $auth->acl_getf_global('f_read'))
			))
		{
			$topic_forum_id = ($topic_rows[$draft['topic_id']]['forum_id']) ? $topic_rows[$draft['topic_id']]['forum_id'] : $forum_id;

			$link_topic = true;
			$view_url = append_sid("{$phpbb_root_path}viewtopic.$phpEx", 'f=' . $topic_forum_id . '&amp;t=' . $draft['topic_id']);
			$title = $topic_rows[$draft['topic_id']]['topic_title'];

			$insert_url = append_sid("{$phpbb_root_path}posting.$phpEx", 'f=' . $topic_forum_id . '&amp;t=' . $draft['topic_id'] . '&amp;mode=reply&amp;d=' . $draft['draft_id']);
		}
		else if ($draft['forum_id'] && $auth->acl_get('f_read', $draft['forum_id']))
		{
			$link_forum = true;
			$view_url = append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $draft['forum_id']);
			$title = $draft['forum_name'];

			$insert_url = append_sid("{$phpbb_root_path}posting.$phpEx", 'f=' . $draft['forum_id'] . '&amp;mode=post&amp;d=' . $draft['draft_id']);
		}
		else
		{
			// Either display as PM draft if forum_id and topic_id are empty or if access to the forums has been denied afterwards...
			$link_pm = true;
			$insert_url = append_sid("{$phpbb_root_path}ucp.$phpEx", "i=$id&amp;mode=compose&amp;d={$draft['draft_id']}");
		}

		$template->assign_block_vars('draftrow', array(
			'DRAFT_ID'		=> $draft['draft_id'],
			'DATE'			=> $user->format_date($draft['save_time']),
			'DRAFT_SUBJECT'	=> $draft['draft_subject'],

			'TITLE'			=> $title,
			'U_VIEW'		=> $view_url,
			'U_INSERT'		=> $insert_url,

			'S_LINK_PM'		=> $link_pm,
			'S_LINK_TOPIC'	=> $link_topic,
			'S_LINK_FORUM'	=> $link_forum)
		);
	}
}

/**
* Topic Review
*/
function topic_review($topic_id, $forum_id, $mode = 'topic_review', $cur_post_id = 0, $show_quote_button = true)
{
	global $user, $auth, $db, $template, $bbcode, $cache;
	global $config, $phpbb_root_path, $phpEx;

	// Go ahead and pull all data for this topic
	$sql = 'SELECT p.post_id
		FROM ' . POSTS_TABLE . ' p' . "
		WHERE p.topic_id = $topic_id
			" . ((!$auth->acl_get('m_approve', $forum_id)) ? 'AND p.post_approved = 1' : '') . '
			' . (($mode == 'post_review') ? " AND p.post_id > $cur_post_id" : '') . '
		ORDER BY p.post_time ';
	$sql .= ($mode == 'post_review') ? 'ASC' : 'DESC';
	$result = $db->sql_query_limit($sql, $config['posts_per_page']);

	$post_list = array();

	while ($row = $db->sql_fetchrow($result))
	{
		$post_list[] = $row['post_id'];
	}

	$db->sql_freeresult($result);

	if (!sizeof($post_list))
	{
		return false;
	}

	$sql = $db->sql_build_query('SELECT', array(
		'SELECT'	=> 'u.username, u.user_id, u.user_colour, p.*',

		'FROM'		=> array(
			USERS_TABLE		=> 'u',
			POSTS_TABLE		=> 'p',
		),

		'WHERE'		=> $db->sql_in_set('p.post_id', $post_list) . '
			AND u.user_id = p.poster_id'
	));

	$result = $db->sql_query($sql);

	$bbcode_bitfield = '';
	$rowset = array();
	$has_attachments = false;
	while ($row = $db->sql_fetchrow($result))
	{
		$rowset[$row['post_id']] = $row;
		$bbcode_bitfield = $bbcode_bitfield | base64_decode($row['bbcode_bitfield']);

		if ($row['post_attachment'])
		{
			$has_attachments = true;
		}
	}
	$db->sql_freeresult($result);

	// Instantiate BBCode class
	if (!isset($bbcode) && $bbcode_bitfield !== '')
	{
		include_once($phpbb_root_path . 'includes/bbcode.' . $phpEx);
		$bbcode = new bbcode(base64_encode($bbcode_bitfield));
	}

	// Grab extensions
	$extensions = $attachments = array();
	if ($has_attachments && $auth->acl_get('u_download') && $auth->acl_get('f_download', $forum_id))
	{
		$extensions = $cache->obtain_attach_extensions($forum_id);

		// Get attachments...
		$sql = 'SELECT *
			FROM ' . ATTACHMENTS_TABLE . '
			WHERE ' . $db->sql_in_set('post_msg_id', $post_list) . '
				AND in_message = 0
			ORDER BY filetime DESC, post_msg_id ASC';
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$attachments[$row['post_msg_id']][] = $row;
		}
		$db->sql_freeresult($result);
	}

	for ($i = 0, $end = sizeof($post_list); $i < $end; ++$i)
	{
		// A non-existing rowset only happens if there was no user present for the entered poster_id
		// This could be a broken posts table.
		if (!isset($rowset[$post_list[$i]]))
		{
			continue;
		}

		$row =& $rowset[$post_list[$i]];

		$poster_id		= $row['user_id'];
		$post_subject	= $row['post_subject'];
		$message		= censor_text($row['post_text']);

		$decoded_message = false;

		if ($show_quote_button && $auth->acl_get('f_reply', $forum_id))
		{
			$decoded_message = $message;
			decode_message($decoded_message, $row['bbcode_uid']);

			$decoded_message = bbcode_nl2br($decoded_message);
		}

		if ($row['bbcode_bitfield'])
		{
			$bbcode->bbcode_second_pass($message, $row['bbcode_uid'], $row['bbcode_bitfield']);
		}

		$message = bbcode_nl2br($message);
		$message = smiley_text($message, !$row['enable_smilies']);

		if (!empty($attachments[$row['post_id']]))
		{
			$update_count = array();
			parse_attachments($forum_id, $message, $attachments[$row['post_id']], $update_count);
		}

		$post_subject = censor_text($post_subject);

		$template->assign_block_vars($mode . '_row', array(
			'POST_AUTHOR_FULL'		=> get_username_string('full', $poster_id, $row['username'], $row['user_colour'], $row['post_username']),
			'POST_AUTHOR_COLOUR'	=> get_username_string('colour', $poster_id, $row['username'], $row['user_colour'], $row['post_username']),
			'POST_AUTHOR'			=> get_username_string('username', $poster_id, $row['username'], $row['user_colour'], $row['post_username']),
			'U_POST_AUTHOR'			=> get_username_string('profile', $poster_id, $row['username'], $row['user_colour'], $row['post_username']),

			'S_HAS_ATTACHMENTS'	=> (!empty($attachments[$row['post_id']])) ? true : false,

			'POST_SUBJECT'		=> $post_subject,
			'MINI_POST_IMG'		=> $user->img('icon_post_target', $user->lang['POST']),
			'POST_DATE'			=> $user->format_date($row['post_time']),
			'MESSAGE'			=> $message,
			'DECODED_MESSAGE'	=> $decoded_message,
			'POST_ID'			=> $row['post_id'],
			'U_MINI_POST'		=> append_sid("{$phpbb_root_path}viewtopic.$phpEx", 'p=' . $row['post_id']) . '#p' . $row['post_id'],
			'U_MCP_DETAILS'		=> ($auth->acl_get('m_info', $forum_id)) ? append_sid("{$phpbb_root_path}mcp.$phpEx", 'i=main&amp;mode=post_details&amp;f=' . $forum_id . '&amp;p=' . $row['post_id'], true, $user->session_id) : '',
			'POSTER_QUOTE'		=> ($show_quote_button && $auth->acl_get('f_reply', $forum_id)) ? addslashes(get_username_string('username', $poster_id, $row['username'], $row['user_colour'], $row['post_username'])) : '')
		);

		// Display not already displayed Attachments for this post, we already parsed them. ;)
		if (!empty($attachments[$row['post_id']]))
		{
			foreach ($attachments[$row['post_id']] as $attachment)
			{
				$template->assign_block_vars($mode . '_row.attachment', array(
					'DISPLAY_ATTACHMENT'	=> $attachment)
				);
			}
		}

		unset($rowset[$i]);
	}

	if ($mode == 'topic_review')
	{
		$template->assign_var('QUOTE_IMG', $user->img('icon_post_quote', $user->lang['REPLY_WITH_QUOTE']));
	}

	return true;
}

/**
* User Notification
*/
function user_notification($mode, $subject, $topic_title, $forum_name, $forum_id, $topic_id, $post_id)
{
	global $db, $user, $config, $phpbb_root_path, $phpEx, $auth;

	$topic_notification = ($mode == 'reply' || $mode == 'quote') ? true : false;
	$forum_notification = ($mode == 'post') ? true : false;

	if (!$topic_notification && !$forum_notification)
	{
		trigger_error('WRONG_NOTIFICATION_MODE');
	}

	if (($topic_notification && !$config['allow_topic_notify']) || ($forum_notification && !$config['allow_forum_notify']))
	{
		return;
	}

	$topic_title = ($topic_notification) ? $topic_title : $subject;
	$topic_title = censor_text($topic_title);

	// Get banned User ID's
	$sql = 'SELECT ban_userid
		FROM ' . BANLIST_TABLE . '
		WHERE ban_userid <> 0
			AND ban_exclude <> 1';
	$result = $db->sql_query($sql);

	$sql_ignore_users = ANONYMOUS . ', ' . $user->data['user_id'];
	while ($row = $db->sql_fetchrow($result))
	{
		$sql_ignore_users .= ', ' . (int) $row['ban_userid'];
	}
	$db->sql_freeresult($result);

	$notify_rows = array();

	// -- get forum_userids	|| topic_userids
	$sql = 'SELECT u.user_id, u.username, u.user_email, u.user_lang, u.user_notify_type, u.user_jabber
		FROM ' . (($topic_notification) ? TOPICS_WATCH_TABLE : FORUMS_WATCH_TABLE) . ' w, ' . USERS_TABLE . ' u
		WHERE w.' . (($topic_notification) ? 'topic_id' : 'forum_id') . ' = ' . (($topic_notification) ? $topic_id : $forum_id) . "
			AND w.user_id NOT IN ($sql_ignore_users)
			AND w.notify_status = 0
			AND u.user_type IN (" . USER_NORMAL . ', ' . USER_FOUNDER . ')
			AND u.user_id = w.user_id';
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		$notify_rows[$row['user_id']] = array(
			'user_id'		=> $row['user_id'],
			'username'		=> $row['username'],
			'user_email'	=> $row['user_email'],
			'user_jabber'	=> $row['user_jabber'],
			'user_lang'		=> $row['user_lang'],
			'notify_type'	=> ($topic_notification) ? 'topic' : 'forum',
			'template'		=> ($topic_notification) ? 'topic_notify' : 'newtopic_notify',
			'method'		=> $row['user_notify_type'],
			'allowed'		=> false
		);
	}
	$db->sql_freeresult($result);

	// forum notification is sent to those not already receiving topic notifications
	if ($topic_notification)
	{
		if (sizeof($notify_rows))
		{
			$sql_ignore_users .= ', ' . implode(', ', array_keys($notify_rows));
		}

		$sql = 'SELECT u.user_id, u.username, u.user_email, u.user_lang, u.user_notify_type, u.user_jabber
			FROM ' . FORUMS_WATCH_TABLE . ' fw, ' . USERS_TABLE . " u
			WHERE fw.forum_id = $forum_id
				AND fw.user_id NOT IN ($sql_ignore_users)
				AND fw.notify_status = 0
				AND u.user_type IN (" . USER_NORMAL . ', ' . USER_FOUNDER . ')
				AND u.user_id = fw.user_id';
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$notify_rows[$row['user_id']] = array(
				'user_id'		=> $row['user_id'],
				'username'		=> $row['username'],
				'user_email'	=> $row['user_email'],
				'user_jabber'	=> $row['user_jabber'],
				'user_lang'		=> $row['user_lang'],
				'notify_type'	=> 'forum',
				'template'		=> 'forum_notify',
				'method'		=> $row['user_notify_type'],
				'allowed'		=> false
			);
		}
		$db->sql_freeresult($result);
	}

	if (!sizeof($notify_rows))
	{
		return;
	}

	// Make sure users are allowed to read the forum
	foreach ($auth->acl_get_list(array_keys($notify_rows), 'f_read', $forum_id) as $forum_id => $forum_ary)
	{
		foreach ($forum_ary as $auth_option => $user_ary)
		{
			foreach ($user_ary as $user_id)
			{
				$notify_rows[$user_id]['allowed'] = true;
			}
		}
	}


	// Now, we have to do a little step before really sending, we need to distinguish our users a little bit. ;)
	$msg_users = $delete_ids = $update_notification = array();
	foreach ($notify_rows as $user_id => $row)
	{
		if (!$row['allowed'] || !trim($row['user_email']))
		{
			$delete_ids[$row['notify_type']][] = $row['user_id'];
		}
		else
		{
			$msg_users[] = $row;
			$update_notification[$row['notify_type']][] = $row['user_id'];
		}
	}
	unset($notify_rows);

	// Now, we are able to really send out notifications
	if (sizeof($msg_users))
	{
		include_once($phpbb_root_path . 'includes/functions_messenger.' . $phpEx);
		$messenger = new messenger();

		$msg_list_ary = array();
		foreach ($msg_users as $row)
		{
			$pos = (!isset($msg_list_ary[$row['template']])) ? 0 : sizeof($msg_list_ary[$row['template']]);

			$msg_list_ary[$row['template']][$pos]['method']	= $row['method'];
			$msg_list_ary[$row['template']][$pos]['email']	= $row['user_email'];
			$msg_list_ary[$row['template']][$pos]['jabber']	= $row['user_jabber'];
			$msg_list_ary[$row['template']][$pos]['name']	= $row['username'];
			$msg_list_ary[$row['template']][$pos]['lang']	= $row['user_lang'];
		}
		unset($msg_users);

		foreach ($msg_list_ary as $email_template => $email_list)
		{
			foreach ($email_list as $addr)
			{
				$messenger->template($email_template, $addr['lang']);

				$messenger->to($addr['email'], $addr['name']);
				$messenger->im($addr['jabber'], $addr['name']);

				$messenger->assign_vars(array(
					'USERNAME'		=> htmlspecialchars_decode($addr['name']),
					'TOPIC_TITLE'	=> htmlspecialchars_decode($topic_title),
					'FORUM_NAME'	=> htmlspecialchars_decode($forum_name),

					'U_FORUM'				=> generate_board_url() . "/viewforum.$phpEx?f=$forum_id",
					'U_TOPIC'				=> generate_board_url() . "/viewtopic.$phpEx?f=$forum_id&t=$topic_id",
					'U_NEWEST_POST'			=> generate_board_url() . "/viewtopic.$phpEx?f=$forum_id&t=$topic_id&p=$post_id&e=$post_id",
					'U_STOP_WATCHING_TOPIC'	=> generate_board_url() . "/viewtopic.$phpEx?f=$forum_id&t=$topic_id&unwatch=topic",
					'U_STOP_WATCHING_FORUM'	=> generate_board_url() . "/viewforum.$phpEx?f=$forum_id&unwatch=forum",
				));

				$messenger->send($addr['method']);
			}
		}
		unset($msg_list_ary);

		$messenger->save_queue();
	}

	// Handle the DB updates
	$db->sql_transaction('begin');

	if (!empty($update_notification['topic']))
	{
		$sql = 'UPDATE ' . TOPICS_WATCH_TABLE . "
			SET notify_status = 1
			WHERE topic_id = $topic_id
				AND " . $db->sql_in_set('user_id', $update_notification['topic']);
		$db->sql_query($sql);
	}

	if (!empty($update_notification['forum']))
	{
		$sql = 'UPDATE ' . FORUMS_WATCH_TABLE . "
			SET notify_status = 1
			WHERE forum_id = $forum_id
				AND " . $db->sql_in_set('user_id', $update_notification['forum']);
		$db->sql_query($sql);
	}

	// Now delete the user_ids not authorised to receive notifications on this topic/forum
	if (!empty($delete_ids['topic']))
	{
		$sql = 'DELETE FROM ' . TOPICS_WATCH_TABLE . "
			WHERE topic_id = $topic_id
				AND " . $db->sql_in_set('user_id', $delete_ids['topic']);
		$db->sql_query($sql);
	}

	if (!empty($delete_ids['forum']))
	{
		$sql = 'DELETE FROM ' . FORUMS_WATCH_TABLE . "
			WHERE forum_id = $forum_id
				AND " . $db->sql_in_set('user_id', $delete_ids['forum']);
		$db->sql_query($sql);
	}

	$db->sql_transaction('commit');
}

//
// Post handling functions
//

/**
* Delete Post
*/
function delete_post($forum_id, $topic_id, $post_id, &$data)
{
	global $db, $user, $auth;
	global $config, $phpEx, $phpbb_root_path;

	// Specify our post mode
	$post_mode = 'delete';
	if (($data['topic_first_post_id'] === $data['topic_last_post_id']) && $data['topic_replies_real'] == 0)
	{
		$post_mode = 'delete_topic';
	}
	else if ($data['topic_first_post_id'] == $post_id)
	{
		$post_mode = 'delete_first_post';
	} 
	else if ($data['topic_last_post_id'] == $post_id)
	{
		$post_mode = 'delete_last_post';
	}
	$sql_data = array();
	$next_post_id = false;

	include_once($phpbb_root_path . 'includes/functions_admin.' . $phpEx);

	$db->sql_transaction('begin');

	// we must make sure to update forums that contain the shadow'd topic
	if ($post_mode == 'delete_topic')
	{
		$shadow_forum_ids = array();

		$sql = 'SELECT forum_id
			FROM ' . TOPICS_TABLE . '
			WHERE ' . $db->sql_in_set('topic_moved_id', $topic_id);
		$result = $db->sql_query($sql);
		while ($row = $db->sql_fetchrow($result))
		{
			if (!isset($shadow_forum_ids[(int) $row['forum_id']]))
			{
				$shadow_forum_ids[(int) $row['forum_id']] = 1;
			}
			else
			{
				$shadow_forum_ids[(int) $row['forum_id']]++;
			}
		}
		$db->sql_freeresult($result);
	}

	if (!delete_posts('post_id', array($post_id), false, false))
	{
		// Try to delete topic, we may had an previous error causing inconsistency
		if ($post_mode == 'delete_topic')
		{
			delete_topics('topic_id', array($topic_id), false);
		}
		trigger_error('ALREADY_DELETED');
	}

	$db->sql_transaction('commit');

	// Collect the necessary information for updating the tables
	$sql_data[FORUMS_TABLE] = '';
	switch ($post_mode)
	{
		case 'delete_topic':

			foreach ($shadow_forum_ids as $updated_forum => $topic_count)
			{
				// counting is fun! we only have to do sizeof($forum_ids) number of queries,
				// even if the topic is moved back to where its shadow lives (we count how many times it is in a forum)
				$db->sql_query('UPDATE ' . FORUMS_TABLE . ' SET forum_topics_real = forum_topics_real - ' . $topic_count . ', forum_topics = forum_topics - ' . $topic_count . ' WHERE forum_id = ' . $updated_forum);
				update_post_information('forum', $updated_forum);
			}

			delete_topics('topic_id', array($topic_id), false);

			if ($data['topic_type'] != POST_GLOBAL)
			{
				$sql_data[FORUMS_TABLE] .= 'forum_topics_real = forum_topics_real - 1';
				$sql_data[FORUMS_TABLE] .= ($data['topic_approved']) ? ', forum_posts = forum_posts - 1, forum_topics = forum_topics - 1' : '';
			}

			$update_sql = update_post_information('forum', $forum_id, true);
			if (sizeof($update_sql))
			{
				$sql_data[FORUMS_TABLE] .= ($sql_data[FORUMS_TABLE]) ? ', ' : '';
				$sql_data[FORUMS_TABLE] .= implode(', ', $update_sql[$forum_id]);
			}
		break;

		case 'delete_first_post':
			$sql = 'SELECT p.post_id, p.poster_id, p.post_username, u.username, u.user_colour
				FROM ' . POSTS_TABLE . ' p, ' . USERS_TABLE . " u
				WHERE p.topic_id = $topic_id
					AND p.poster_id = u.user_id
				ORDER BY p.post_time ASC";
			$result = $db->sql_query_limit($sql, 1);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			if ($data['topic_type'] != POST_GLOBAL)
			{
				$sql_data[FORUMS_TABLE] = ($data['post_approved']) ? 'forum_posts = forum_posts - 1' : '';
			}

			$sql_data[TOPICS_TABLE] = 'topic_first_post_id = ' . intval($row['post_id']) . ", topic_first_poster_colour = '" . $db->sql_escape($row['user_colour']) . "', topic_first_poster_name = '" . (($row['poster_id'] == ANONYMOUS) ? $db->sql_escape($row['post_username']) : $db->sql_escape($row['username'])) . "'";

			// Decrementing topic_replies here is fine because this case only happens if there is more than one post within the topic - basically removing one "reply"
			$sql_data[TOPICS_TABLE] .= ', topic_replies_real = topic_replies_real - 1' . (($data['post_approved']) ? ', topic_replies = topic_replies - 1' : '');

			$next_post_id = (int) $row['post_id'];
		break;

		case 'delete_last_post':
			if ($data['topic_type'] != POST_GLOBAL)
			{
				$sql_data[FORUMS_TABLE] = ($data['post_approved']) ? 'forum_posts = forum_posts - 1' : '';
			}

			$update_sql = update_post_information('forum', $forum_id, true);
			if (sizeof($update_sql))
			{
				$sql_data[FORUMS_TABLE] .= ($sql_data[FORUMS_TABLE]) ? ', ' : '';
				$sql_data[FORUMS_TABLE] .= implode(', ', $update_sql[$forum_id]);
			}

			$sql_data[TOPICS_TABLE] = 'topic_bumped = 0, topic_bumper = 0, topic_replies_real = topic_replies_real - 1' . (($data['post_approved']) ? ', topic_replies = topic_replies - 1' : '');

			$update_sql = update_post_information('topic', $topic_id, true);
			if (sizeof($update_sql))
			{
				$sql_data[TOPICS_TABLE] .= ', ' . implode(', ', $update_sql[$topic_id]);
				$next_post_id = (int) str_replace('topic_last_post_id = ', '', $update_sql[$topic_id][0]);
			}
			else
			{
				$sql = 'SELECT MAX(post_id) as last_post_id
					FROM ' . POSTS_TABLE . "
					WHERE topic_id = $topic_id " .
						((!$auth->acl_get('m_approve', $forum_id)) ? 'AND post_approved = 1' : '');
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				$next_post_id = (int) $row['last_post_id'];
			}
		break;

		case 'delete':
			$sql = 'SELECT post_id
				FROM ' . POSTS_TABLE . "
				WHERE topic_id = $topic_id " .
					((!$auth->acl_get('m_approve', $forum_id)) ? 'AND post_approved = 1' : '') . '
					AND post_time > ' . $data['post_time'] . '
				ORDER BY post_time ASC';
			$result = $db->sql_query_limit($sql, 1);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			if ($data['topic_type'] != POST_GLOBAL)
			{
				$sql_data[FORUMS_TABLE] = ($data['post_approved']) ? 'forum_posts = forum_posts - 1' : '';
			}

			$sql_data[TOPICS_TABLE] = 'topic_replies_real = topic_replies_real - 1' . (($data['post_approved']) ? ', topic_replies = topic_replies - 1' : '');
			$next_post_id = (int) $row['post_id'];
		break;
	}

//	$sql_data[USERS_TABLE] = ($data['post_postcount']) ? 'user_posts = user_posts - 1' : '';

	$db->sql_transaction('begin');

	$where_sql = array(
		FORUMS_TABLE	=> "forum_id = $forum_id",
		TOPICS_TABLE	=> "topic_id = $topic_id",
		USERS_TABLE		=> 'user_id = ' . $data['poster_id']
	);

	foreach ($sql_data as $table => $update_sql)
	{
		if ($update_sql)
		{
			$db->sql_query("UPDATE $table SET $update_sql WHERE " . $where_sql[$table]);
		}
	}

	// Adjust posted info for this user by looking for a post by him/her within this topic...
	if ($post_mode != 'delete_topic' && $config['load_db_track'] && $data['poster_id'] != ANONYMOUS)
	{
		$sql = 'SELECT poster_id
			FROM ' . POSTS_TABLE . '
			WHERE topic_id = ' . $topic_id . '
				AND poster_id = ' . $data['poster_id'];
		$result = $db->sql_query_limit($sql, 1);
		$poster_id = (int) $db->sql_fetchfield('poster_id');
		$db->sql_freeresult($result);

		// The user is not having any more posts within this topic
		if (!$poster_id)
		{
			$sql = 'DELETE FROM ' . TOPICS_POSTED_TABLE . '
				WHERE topic_id = ' . $topic_id . '
					AND user_id = ' . $data['poster_id'];
			$db->sql_query($sql);
		}
	}

	$db->sql_transaction('commit');

	if ($data['post_reported'] && ($post_mode != 'delete_topic'))
	{
		sync('topic_reported', 'topic_id', array($topic_id));
	}

	return $next_post_id;
}

/**
* Submit Post
*/
function submit_post($mode, $subject, $username, $topic_type, &$poll, &$data, $update_message = true)
{
	global $db, $auth, $user, $config, $phpEx, $template, $phpbb_root_path;

	// We do not handle erasing posts here
	if ($mode == 'delete')
	{
		return false;
	}

	$current_time = time();

	if ($mode == 'post')
	{
		$post_mode = 'post';
		$update_message = true;
	}
	else if ($mode != 'edit')
	{
		$post_mode = 'reply';
		$update_message = true;
	}
	else if ($mode == 'edit')
	{
		$post_mode = ($data['topic_replies_real'] == 0) ? 'edit_topic' : (($data['topic_first_post_id'] == $data['post_id']) ? 'edit_first_post' : (($data['topic_last_post_id'] == $data['post_id']) ? 'edit_last_post' : 'edit'));
	}

	// First of all make sure the subject and topic title are having the correct length.
	// To achieve this without cutting off between special chars we convert to an array and then count the elements.
	$subject = truncate_string($subject);
	$data['topic_title'] = truncate_string($data['topic_title']);

	// Collect some basic information about which tables and which rows to update/insert
	$sql_data = $topic_row = array();
	$poster_id = ($mode == 'edit') ? $data['poster_id'] : (int) $user->data['user_id'];

	// Retrieve some additional information if not present
	if ($mode == 'edit' && (!isset($data['post_approved']) || !isset($data['topic_approved']) || $data['post_approved'] === false || $data['topic_approved'] === false))
	{
		$sql = 'SELECT p.post_approved, t.topic_type, t.topic_replies, t.topic_replies_real, t.topic_approved
			FROM ' . TOPICS_TABLE . ' t, ' . POSTS_TABLE . ' p
			WHERE t.topic_id = p.topic_id
				AND p.post_id = ' . $data['post_id'];
		$result = $db->sql_query($sql);
		$topic_row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		$data['topic_approved'] = $topic_row['topic_approved'];
		$data['post_approved'] = $topic_row['post_approved'];
	}

	// Start the transaction here
	$db->sql_transaction('begin');


	// Collect Information
	switch ($post_mode)
	{
		case 'post':
		case 'reply':
			$sql_data[POSTS_TABLE]['sql'] = array(
				'forum_id'			=> ($topic_type == POST_GLOBAL) ? 0 : $data['forum_id'],
				'poster_id'			=> (int) $user->data['user_id'],
				'icon_id'			=> $data['icon_id'],
				'poster_ip'			=> $user->ip,
				'post_time'			=> $current_time,
				'post_approved'		=> (!$auth->acl_get('f_noapprove', $data['forum_id']) && !$auth->acl_get('m_approve', $data['forum_id'])) ? 0 : 1,
				'enable_bbcode'		=> $data['enable_bbcode'],
				'enable_smilies'	=> $data['enable_smilies'],
				'enable_magic_url'	=> $data['enable_urls'],
				'enable_sig'		=> $data['enable_sig'],
				'post_username'		=> (!$user->data['is_registered']) ? $username : '',
				'post_subject'		=> $subject,
				'post_text'			=> $data['message'],
				'post_checksum'		=> $data['message_md5'],
				'post_attachment'	=> (!empty($data['attachment_data'])) ? 1 : 0,
				'bbcode_bitfield'	=> $data['bbcode_bitfield'],
				'bbcode_uid'		=> $data['bbcode_uid'],
				'post_postcount'	=> ($auth->acl_get('f_postcount', $data['forum_id'])) ? 1 : 0,
				'post_edit_locked'	=> $data['post_edit_locked']
			);
		break;

		case 'edit_first_post':
		case 'edit':

		case 'edit_last_post':
		case 'edit_topic':

			// If edit reason is given always display edit info

			// If editing last post then display no edit info
			// If m_edit permission then display no edit info
			// If normal edit display edit info

			// Display edit info if edit reason given or user is editing his post, which is not the last within the topic.
			if ($data['post_edit_reason'] || (!$auth->acl_get('m_edit', $data['forum_id']) && ($post_mode == 'edit' || $post_mode == 'edit_first_post')))
			{
				$data['post_edit_reason']		= truncate_string($data['post_edit_reason'], 255, false);

				$sql_data[POSTS_TABLE]['sql']	= array(
					'post_edit_time'	=> $current_time,
					'post_edit_reason'	=> $data['post_edit_reason'],
					'post_edit_user'	=> (int) $data['post_edit_user'],
				);

				$sql_data[POSTS_TABLE]['stat'][] = 'post_edit_count = post_edit_count + 1';
			}
			else if (!$data['post_edit_reason'] && $mode == 'edit' && $auth->acl_get('m_edit', $data['forum_id']))
			{
				$sql_data[POSTS_TABLE]['sql'] = array(
					'post_edit_reason'	=> '',
				);
			}

			// If the person editing this post is different to the one having posted then we will add a log entry stating the edit
			// Could be simplified by only adding to the log if the edit is not tracked - but this may confuse admins/mods
			if ($user->data['user_id'] != $poster_id)
			{
				$log_subject = ($subject) ? $subject : $data['topic_title'];
				add_log('mod', $data['forum_id'], $data['topic_id'], 'LOG_POST_EDITED', $log_subject, (!empty($username)) ? $username : $user->lang['GUEST']);
			}

			if (!isset($sql_data[POSTS_TABLE]['sql']))
			{
				$sql_data[POSTS_TABLE]['sql'] = array();
			}

			$sql_data[POSTS_TABLE]['sql'] = array_merge($sql_data[POSTS_TABLE]['sql'], array(
				'forum_id'			=> ($topic_type == POST_GLOBAL) ? 0 : $data['forum_id'],
				'poster_id'			=> $data['poster_id'],
				'icon_id'			=> $data['icon_id'],
				'post_approved'		=> (!$auth->acl_get('f_noapprove', $data['forum_id']) && !$auth->acl_get('m_approve', $data['forum_id'])) ? 0 : $data['post_approved'],
				'enable_bbcode'		=> $data['enable_bbcode'],
				'enable_smilies'	=> $data['enable_smilies'],
				'enable_magic_url'	=> $data['enable_urls'],
				'enable_sig'		=> $data['enable_sig'],
				'post_username'		=> ($username && $data['poster_id'] == ANONYMOUS) ? $username : '',
				'post_subject'		=> $subject,
				'post_checksum'		=> $data['message_md5'],
				'post_attachment'	=> (!empty($data['attachment_data'])) ? 1 : 0,
				'bbcode_bitfield'	=> $data['bbcode_bitfield'],
				'bbcode_uid'		=> $data['bbcode_uid'],
				'post_edit_locked'	=> $data['post_edit_locked'])
			);

			if ($update_message)
			{
				$sql_data[POSTS_TABLE]['sql']['post_text'] = $data['message'];
			}

		break;
	}

	$post_approved = $sql_data[POSTS_TABLE]['sql']['post_approved'];
	$topic_row = array();

	// And the topic ladies and gentlemen
	switch ($post_mode)
	{
		case 'post':
			$sql_data[TOPICS_TABLE]['sql'] = array(
				'topic_poster'				=> (int) $user->data['user_id'],
				'topic_time'				=> $current_time,
				'forum_id'					=> ($topic_type == POST_GLOBAL) ? 0 : $data['forum_id'],
				'icon_id'					=> $data['icon_id'],
				'topic_approved'			=> (!$auth->acl_get('f_noapprove', $data['forum_id']) && !$auth->acl_get('m_approve', $data['forum_id'])) ? 0 : 1,
				'topic_title'				=> $subject,
				'topic_first_poster_name'	=> (!$user->data['is_registered'] && $username) ? $username : (($user->data['user_id'] != ANONYMOUS) ? $user->data['username'] : ''),
				'topic_first_poster_colour'	=> $user->data['user_colour'],
				'topic_type'				=> $topic_type,
				'topic_time_limit'			=> ($topic_type == POST_STICKY || $topic_type == POST_ANNOUNCE) ? ($data['topic_time_limit'] * 86400) : 0,
				'topic_attachment'			=> (!empty($data['attachment_data'])) ? 1 : 0,
			);

			if (isset($poll['poll_options']) && !empty($poll['poll_options']))
			{
				$sql_data[TOPICS_TABLE]['sql'] = array_merge($sql_data[TOPICS_TABLE]['sql'], array(
					'poll_title'		=> $poll['poll_title'],
					'poll_start'		=> ($poll['poll_start']) ? $poll['poll_start'] : $current_time,
					'poll_max_options'	=> $poll['poll_max_options'],
					'poll_length'		=> ($poll['poll_length'] * 86400),
					'poll_vote_change'	=> $poll['poll_vote_change'])
				);
			}

			$sql_data[USERS_TABLE]['stat'][] = "user_lastpost_time = $current_time" . (($auth->acl_get('f_postcount', $data['forum_id'])) ? ', user_posts = user_posts + 1' : '');

			if ($topic_type != POST_GLOBAL)
			{
				if ($auth->acl_get('f_noapprove', $data['forum_id']) || $auth->acl_get('m_approve', $data['forum_id']))
				{
					$sql_data[FORUMS_TABLE]['stat'][] = 'forum_posts = forum_posts + 1';
				}
				$sql_data[FORUMS_TABLE]['stat'][] = 'forum_topics_real = forum_topics_real + 1' . (($auth->acl_get('f_noapprove', $data['forum_id']) || $auth->acl_get('m_approve', $data['forum_id'])) ? ', forum_topics = forum_topics + 1' : '');
			}
		break;

		case 'reply':
			$sql_data[TOPICS_TABLE]['stat'][] = 'topic_replies_real = topic_replies_real + 1, topic_bumped = 0, topic_bumper = 0' . (($auth->acl_get('f_noapprove', $data['forum_id']) || $auth->acl_get('m_approve', $data['forum_id'])) ? ', topic_replies = topic_replies + 1' : '') . ((!empty($data['attachment_data']) || (isset($data['topic_attachment']) && $data['topic_attachment'])) ? ', topic_attachment = 1' : '');

			$sql_data[USERS_TABLE]['stat'][] = "user_lastpost_time = $current_time" . (($auth->acl_get('f_postcount', $data['forum_id'])) ? ', user_posts = user_posts + 1' : '');

			if (($auth->acl_get('f_noapprove', $data['forum_id']) || $auth->acl_get('m_approve', $data['forum_id'])) && $topic_type != POST_GLOBAL)
			{
				$sql_data[FORUMS_TABLE]['stat'][] = 'forum_posts = forum_posts + 1';
			}
		break;

		case 'edit_topic':
		case 'edit_first_post':

			$sql_data[TOPICS_TABLE]['sql'] = array(
				'forum_id'					=> ($topic_type == POST_GLOBAL) ? 0 : $data['forum_id'],
				'icon_id'					=> $data['icon_id'],
				'topic_approved'			=> (!$auth->acl_get('f_noapprove', $data['forum_id']) && !$auth->acl_get('m_approve', $data['forum_id'])) ? 0 : $data['topic_approved'],
				'topic_title'				=> $subject,
				'topic_first_poster_name'	=> $username,
				'topic_type'				=> $topic_type,
				'topic_time_limit'			=> ($topic_type == POST_STICKY || $topic_type == POST_ANNOUNCE) ? ($data['topic_time_limit'] * 86400) : 0,
				'poll_title'				=> (isset($poll['poll_options'])) ? $poll['poll_title'] : '',
				'poll_start'				=> (isset($poll['poll_options'])) ? (($poll['poll_start']) ? $poll['poll_start'] : $current_time) : 0,
				'poll_max_options'			=> (isset($poll['poll_options'])) ? $poll['poll_max_options'] : 1,
				'poll_length'				=> (isset($poll['poll_options'])) ? ($poll['poll_length'] * 86400) : 0,
				'poll_vote_change'			=> (isset($poll['poll_vote_change'])) ? $poll['poll_vote_change'] : 0,

				'topic_attachment'			=> (!empty($data['attachment_data'])) ? 1 : (isset($data['topic_attachment']) ? $data['topic_attachment'] : 0),
			);

			// Correctly set back the topic replies and forum posts... only if the topic was approved before and now gets disapproved
			if (!$auth->acl_get('f_noapprove', $data['forum_id']) && !$auth->acl_get('m_approve', $data['forum_id']) && $data['topic_approved'])
			{
				// Do we need to grab some topic informations?
				if (!sizeof($topic_row))
				{
					$sql = 'SELECT topic_type, topic_replies, topic_replies_real, topic_approved
						FROM ' . TOPICS_TABLE . '
						WHERE topic_id = ' . $data['topic_id'];
					$result = $db->sql_query($sql);
					$topic_row = $db->sql_fetchrow($result);
					$db->sql_freeresult($result);
				}

				// If this is the only post remaining we do not need to decrement topic_replies.
				// Also do not decrement if first post - then the topic_replies will not be adjusted if approving the topic again.

				// If this is an edited topic or the first post the topic gets completely disapproved later on...
				$sql_data[FORUMS_TABLE]['stat'][] = 'forum_topics = forum_topics - 1';
				$sql_data[FORUMS_TABLE]['stat'][] = 'forum_posts = forum_posts - ' . ($topic_row['topic_replies'] + 1);

				set_config('num_topics', $config['num_topics'] - 1, true);
				set_config('num_posts', $config['num_posts'] - ($topic_row['topic_replies'] + 1), true);
			}

		break;

		case 'edit':
		case 'edit_last_post':

			// Correctly set back the topic replies and forum posts... but only if the post was approved before.
			if (!$auth->acl_get('f_noapprove', $data['forum_id']) && !$auth->acl_get('m_approve', $data['forum_id']) && $data['post_approved'])
			{
				$sql_data[TOPICS_TABLE]['stat'][] = 'topic_replies = topic_replies - 1';
				$sql_data[FORUMS_TABLE]['stat'][] = 'forum_posts = forum_posts - 1';

				set_config('num_posts', $config['num_posts'] - 1, true);
			}

		break;
	}

	// Submit new topic
	if ($post_mode == 'post')
	{
		$sql = 'INSERT INTO ' . TOPICS_TABLE . ' ' .
			$db->sql_build_array('INSERT', $sql_data[TOPICS_TABLE]['sql']);
		$db->sql_query($sql);

		$data['topic_id'] = $db->sql_nextid();

		$sql_data[POSTS_TABLE]['sql'] = array_merge($sql_data[POSTS_TABLE]['sql'], array(
			'topic_id' => $data['topic_id'])
		);
		unset($sql_data[TOPICS_TABLE]['sql']);
	}

	// Submit new post
	if ($post_mode == 'post' || $post_mode == 'reply')
	{
		if ($post_mode == 'reply')
		{
			$sql_data[POSTS_TABLE]['sql'] = array_merge($sql_data[POSTS_TABLE]['sql'], array(
				'topic_id' => $data['topic_id'])
			);
		}

		$sql = 'INSERT INTO ' . POSTS_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_data[POSTS_TABLE]['sql']);
		$db->sql_query($sql);
		$data['post_id'] = $db->sql_nextid();

		if ($post_mode == 'post')
		{
			$sql_data[TOPICS_TABLE]['sql'] = array(
				'topic_first_post_id'		=> $data['post_id'],
				'topic_last_post_id'		=> $data['post_id'],
				'topic_last_post_time'		=> $current_time,
				'topic_last_poster_id'		=> (int) $user->data['user_id'],
				'topic_last_poster_name'	=> (!$user->data['is_registered'] && $username) ? $username : (($user->data['user_id'] != ANONYMOUS) ? $user->data['username'] : ''),
				'topic_last_poster_colour'	=> $user->data['user_colour'],
			);
		}

		unset($sql_data[POSTS_TABLE]['sql']);
	}

	$make_global = false;

	// Are we globalising or unglobalising?
	if ($post_mode == 'edit_first_post' || $post_mode == 'edit_topic')
	{
		if (!sizeof($topic_row))
		{
			$sql = 'SELECT topic_type, topic_replies, topic_replies_real, topic_approved, topic_last_post_id
				FROM ' . TOPICS_TABLE . '
				WHERE topic_id = ' . $data['topic_id'];
			$result = $db->sql_query($sql);
			$topic_row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);
		}

		// globalise/unglobalise?
		if (($topic_row['topic_type'] != POST_GLOBAL && $topic_type == POST_GLOBAL) || ($topic_row['topic_type'] == POST_GLOBAL && $topic_type != POST_GLOBAL))
		{
			if (!empty($sql_data[FORUMS_TABLE]['stat']) && implode('', $sql_data[FORUMS_TABLE]['stat']))
			{
				$db->sql_query('UPDATE ' . FORUMS_TABLE . ' SET ' . implode(', ', $sql_data[FORUMS_TABLE]['stat']) . ' WHERE forum_id = ' . $data['forum_id']);
			}

			$make_global = true;
			$sql_data[FORUMS_TABLE]['stat'] = array();
		}

		// globalise
		if ($topic_row['topic_type'] != POST_GLOBAL && $topic_type == POST_GLOBAL)
		{
			// Decrement topic/post count
			$sql_data[FORUMS_TABLE]['stat'][] = 'forum_posts = forum_posts - ' . ($topic_row['topic_replies_real'] + 1);
			$sql_data[FORUMS_TABLE]['stat'][] = 'forum_topics_real = forum_topics_real - 1' . (($topic_row['topic_approved']) ? ', forum_topics = forum_topics - 1' : '');

			// Update forum_ids for all posts
			$sql = 'UPDATE ' . POSTS_TABLE . '
				SET forum_id = 0
				WHERE topic_id = ' . $data['topic_id'];
			$db->sql_query($sql);
		}
		// unglobalise
		else if ($topic_row['topic_type'] == POST_GLOBAL && $topic_type != POST_GLOBAL)
		{
			// Increment topic/post count
			$sql_data[FORUMS_TABLE]['stat'][] = 'forum_posts = forum_posts + ' . ($topic_row['topic_replies_real'] + 1);
			$sql_data[FORUMS_TABLE]['stat'][] = 'forum_topics_real = forum_topics_real + 1' . (($topic_row['topic_approved']) ? ', forum_topics = forum_topics + 1' : '');

			// Update forum_ids for all posts
			$sql = 'UPDATE ' . POSTS_TABLE . '
				SET forum_id = ' . $data['forum_id'] . '
				WHERE topic_id = ' . $data['topic_id'];
			$db->sql_query($sql);
		}
	}

	// Update the topics table
	if (isset($sql_data[TOPICS_TABLE]['sql']))
	{
		$sql = 'UPDATE ' . TOPICS_TABLE . '
			SET ' . $db->sql_build_array('UPDATE', $sql_data[TOPICS_TABLE]['sql']) . '
			WHERE topic_id = ' . $data['topic_id'];
		$db->sql_query($sql);
	}

	// Update the posts table
	if (isset($sql_data[POSTS_TABLE]['sql']))
	{
		$sql = 'UPDATE ' . POSTS_TABLE . '
			SET ' . $db->sql_build_array('UPDATE', $sql_data[POSTS_TABLE]['sql']) . '
			WHERE post_id = ' . $data['post_id'];
		$db->sql_query($sql);
	}

	// Update Poll Tables
	if (isset($poll['poll_options']) && !empty($poll['poll_options']))
	{
		$cur_poll_options = array();

		if ($poll['poll_start'] && $mode == 'edit')
		{
			$sql = 'SELECT *
				FROM ' . POLL_OPTIONS_TABLE . '
				WHERE topic_id = ' . $data['topic_id'] . '
				ORDER BY poll_option_id';
			$result = $db->sql_query($sql);

			$cur_poll_options = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$cur_poll_options[] = $row;
			}
			$db->sql_freeresult($result);
		}

		$sql_insert_ary = array();

		for ($i = 0, $size = sizeof($poll['poll_options']); $i < $size; $i++)
		{
			if (strlen(trim($poll['poll_options'][$i])))
			{
				if (empty($cur_poll_options[$i]))
				{
					// If we add options we need to put them to the end to be able to preserve votes...
					$sql_insert_ary[] = array(
						'poll_option_id'	=> (int) sizeof($cur_poll_options) + 1 + sizeof($sql_insert_ary),
						'topic_id'			=> (int) $data['topic_id'],
						'poll_option_text'	=> (string) $poll['poll_options'][$i]
					);
				}
				else if ($poll['poll_options'][$i] != $cur_poll_options[$i])
				{
					$sql = 'UPDATE ' . POLL_OPTIONS_TABLE . "
						SET poll_option_text = '" . $db->sql_escape($poll['poll_options'][$i]) . "'
						WHERE poll_option_id = " . $cur_poll_options[$i]['poll_option_id'] . '
							AND topic_id = ' . $data['topic_id'];
					$db->sql_query($sql);
				}
			}
		}

		$db->sql_multi_insert(POLL_OPTIONS_TABLE, $sql_insert_ary);

		if (sizeof($poll['poll_options']) < sizeof($cur_poll_options))
		{
			$sql = 'DELETE FROM ' . POLL_OPTIONS_TABLE . '
				WHERE poll_option_id > ' . sizeof($poll['poll_options']) . '
					AND topic_id = ' . $data['topic_id'];
			$db->sql_query($sql);
		}

		// If edited, we would need to reset votes (since options can be re-ordered above, you can't be sure if the change is for changing the text or adding an option
		if ($mode == 'edit' && sizeof($poll['poll_options']) != sizeof($cur_poll_options))
		{
			$db->sql_query('DELETE FROM ' . POLL_VOTES_TABLE . ' WHERE topic_id = ' . $data['topic_id']);
			$db->sql_query('UPDATE ' . POLL_OPTIONS_TABLE . ' SET poll_option_total = 0 WHERE topic_id = ' . $data['topic_id']);
		}
	}

	// Submit Attachments
	if (!empty($data['attachment_data']) && $data['post_id'] && in_array($mode, array('post', 'reply', 'quote', 'edit')))
	{
		$space_taken = $files_added = 0;
		$orphan_rows = array();

		foreach ($data['attachment_data'] as $pos => $attach_row)
		{
			$orphan_rows[(int) $attach_row['attach_id']] = array();
		}

		if (sizeof($orphan_rows))
		{
			$sql = 'SELECT attach_id, filesize, physical_filename
				FROM ' . ATTACHMENTS_TABLE . '
				WHERE ' . $db->sql_in_set('attach_id', array_keys($orphan_rows)) . '
					AND is_orphan = 1
					AND poster_id = ' . $user->data['user_id'];
			$result = $db->sql_query($sql);

			$orphan_rows = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$orphan_rows[$row['attach_id']] = $row;
			}
			$db->sql_freeresult($result);
		}

		foreach ($data['attachment_data'] as $pos => $attach_row)
		{
			if ($attach_row['is_orphan'] && !isset($orphan_rows[$attach_row['attach_id']]))
			{
				continue;
			}

			if (!$attach_row['is_orphan'])
			{
				// update entry in db if attachment already stored in db and filespace
				$sql = 'UPDATE ' . ATTACHMENTS_TABLE . "
					SET attach_comment = '" . $db->sql_escape($attach_row['attach_comment']) . "'
					WHERE attach_id = " . (int) $attach_row['attach_id'] . '
						AND is_orphan = 0';
				$db->sql_query($sql);
			}
			else
			{
				// insert attachment into db
				if (!@file_exists($phpbb_root_path . $config['upload_path'] . '/' . basename($orphan_rows[$attach_row['attach_id']]['physical_filename'])))
				{
					continue;
				}

				$space_taken += $orphan_rows[$attach_row['attach_id']]['filesize'];
				$files_added++;

				$attach_sql = array(
					'post_msg_id'		=> $data['post_id'],
					'topic_id'			=> $data['topic_id'],
					'is_orphan'			=> 0,
					'poster_id'			=> $poster_id,
					'attach_comment'	=> $attach_row['attach_comment'],
				);

				$sql = 'UPDATE ' . ATTACHMENTS_TABLE . ' SET ' . $db->sql_build_array('UPDATE', $attach_sql) . '
					WHERE attach_id = ' . $attach_row['attach_id'] . '
						AND is_orphan = 1
						AND poster_id = ' . $user->data['user_id'];
				$db->sql_query($sql);
			}
		}

		if ($space_taken && $files_added)
		{
			set_config('upload_dir_size', $config['upload_dir_size'] + $space_taken, true);
			set_config('num_files', $config['num_files'] + $files_added, true);
		}
	}

	// we need to update the last forum information
	// only applicable if the topic is not global and it is approved
	// we also check to make sure we are not dealing with globaling the latest topic (pretty rare but still needs to be checked)
	if ($topic_type != POST_GLOBAL && !$make_global && ($post_approved || !$data['post_approved']))
	{
		// the last post makes us update the forum table. This can happen if...
		// We make a new topic
		// We reply to a topic
		// We edit the last post in a topic and this post is the latest in the forum (maybe)
		// We edit the only post in the topic
		// We edit the first post in the topic and all the other posts are not approved
		if (($post_mode == 'post' || $post_mode == 'reply') && $post_approved)
		{
			$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_post_id = ' . $data['post_id'];
			$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_post_subject = '" . $db->sql_escape($subject) . "'";
			$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_post_time = ' . $current_time;
			$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_poster_id = ' . (int) $user->data['user_id'];
			$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_poster_name = '" . $db->sql_escape((!$user->data['is_registered'] && $username) ? $username : (($user->data['user_id'] != ANONYMOUS) ? $user->data['username'] : '')) . "'";
			$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_poster_colour = '" . $db->sql_escape($user->data['user_colour']) . "'";
		}
		else if ($post_mode == 'edit_last_post' || $post_mode == 'edit_topic' || ($post_mode == 'edit_first_post' && !$data['topic_replies']))
		{
			// this does not _necessarily_ mean that we must update the info again,
			// it just means that we might have to
			$sql = 'SELECT forum_last_post_id, forum_last_post_subject
				FROM ' . FORUMS_TABLE . '
				WHERE forum_id = ' . (int) $data['forum_id'];
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			// this post is the latest post in the forum, better update
			if ($row['forum_last_post_id'] == $data['post_id'])
			{
				if ($post_approved && $row['forum_last_post_subject'] !== $subject)
				{
					// the only data that can really be changed is the post's subject
					$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_post_subject = \'' . $db->sql_escape($subject) . '\'';
				}
				else if ($data['post_approved'] !== $post_approved)
				{
					// we need a fresh change of socks, everything has become invalidated
					$sql = 'SELECT MAX(topic_last_post_id) as last_post_id
						FROM ' . TOPICS_TABLE . '
						WHERE forum_id = ' . (int) $data['forum_id'] . '
							AND topic_approved = 1';
					$result = $db->sql_query($sql);
					$row = $db->sql_fetchrow($result);
					$db->sql_freeresult($result);

					// any posts left in this forum?
					if (!empty($row['last_post_id']))
					{
						$sql = 'SELECT p.post_id, p.post_subject, p.post_time, p.poster_id, p.post_username, u.user_id, u.username, u.user_colour
							FROM ' . POSTS_TABLE . ' p, ' . USERS_TABLE . ' u
							WHERE p.poster_id = u.user_id
								AND p.post_id = ' . (int) $row['last_post_id'];
						$result = $db->sql_query($sql);
						$row = $db->sql_fetchrow($result);
						$db->sql_freeresult($result);

						// salvation, a post is found! jam it into the forums table
						$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_post_id = ' . (int) $row['post_id'];
						$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_post_subject = '" . $db->sql_escape($row['post_subject']) . "'";
						$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_post_time = ' . (int) $row['post_time'];
						$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_poster_id = ' . (int) $row['poster_id'];
						$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_poster_name = '" . $db->sql_escape(($row['poster_id'] == ANONYMOUS) ? $row['post_username'] : $row['username']) . "'";
						$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_poster_colour = '" . $db->sql_escape($row['user_colour']) . "'";
					}
					else
					{
						// just our luck, the last topic in the forum has just been turned unapproved...
						$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_post_id = 0';
						$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_post_subject = ''";
						$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_post_time = 0';
						$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_poster_id = 0';
						$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_poster_name = ''";
						$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_poster_colour = ''";
					}
				}
			}
		}
	}
	else if ($make_global)
	{
		// somebody decided to be a party pooper, we must recalculate the whole shebang (maybe)
		$sql = 'SELECT forum_last_post_id
			FROM ' . FORUMS_TABLE . '
			WHERE forum_id = ' . (int) $data['forum_id'];
		$result = $db->sql_query($sql);
		$forum_row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		// we made a topic global, go get new data
		if ($topic_row['topic_type'] != POST_GLOBAL && $topic_type == POST_GLOBAL && $forum_row['forum_last_post_id'] == $topic_row['topic_last_post_id'])
		{
			// we need a fresh change of socks, everything has become invalidated
			$sql = 'SELECT MAX(topic_last_post_id) as last_post_id
				FROM ' . TOPICS_TABLE . '
				WHERE forum_id = ' . (int) $data['forum_id'] . '
					AND topic_approved = 1';
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			// any posts left in this forum?
			if (!empty($row['last_post_id']))
			{
				$sql = 'SELECT p.post_id, p.post_subject, p.post_time, p.poster_id, p.post_username, u.user_id, u.username, u.user_colour
					FROM ' . POSTS_TABLE . ' p, ' . USERS_TABLE . ' u
					WHERE p.poster_id = u.user_id
						AND p.post_id = ' . (int) $row['last_post_id'];
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				// salvation, a post is found! jam it into the forums table
				$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_post_id = ' . (int) $row['post_id'];
				$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_post_subject = '" . $db->sql_escape($row['post_subject']) . "'";
				$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_post_time = ' . (int) $row['post_time'];
				$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_poster_id = ' . (int) $row['poster_id'];
				$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_poster_name = '" . $db->sql_escape(($row['poster_id'] == ANONYMOUS) ? $row['post_username'] : $row['username']) . "'";
				$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_poster_colour = '" . $db->sql_escape($row['user_colour']) . "'";
			}
			else
			{
				// just our luck, the last topic in the forum has just been globalized...
				$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_post_id = 0';
				$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_post_subject = ''";
				$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_post_time = 0';
				$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_poster_id = 0';
				$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_poster_name = ''";
				$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_poster_colour = ''";
			}
		}
		else if ($topic_row['topic_type'] == POST_GLOBAL && $topic_type != POST_GLOBAL && $forum_row['forum_last_post_id'] < $topic_row['topic_last_post_id'])
		{
			// this post has a higher id, it is newer
			$sql = 'SELECT p.post_id, p.post_subject, p.post_time, p.poster_id, p.post_username, u.user_id, u.username, u.user_colour
				FROM ' . POSTS_TABLE . ' p, ' . USERS_TABLE . ' u
				WHERE p.poster_id = u.user_id
					AND p.post_id = ' . (int) $topic_row['topic_last_post_id'];
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			// salvation, a post is found! jam it into the forums table
			$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_post_id = ' . (int) $row['post_id'];
			$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_post_subject = '" . $db->sql_escape($row['post_subject']) . "'";
			$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_post_time = ' . (int) $row['post_time'];
			$sql_data[FORUMS_TABLE]['stat'][] = 'forum_last_poster_id = ' . (int) $row['poster_id'];
			$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_poster_name = '" . $db->sql_escape(($row['poster_id'] == ANONYMOUS) ? $row['post_username'] : $row['username']) . "'";
			$sql_data[FORUMS_TABLE]['stat'][] = "forum_last_poster_colour = '" . $db->sql_escape($row['user_colour']) . "'";
		}
	}

	// topic sync time!
	// simply, we update if it is a reply or the last post is edited
	if ($post_approved)
	{
		// reply requires the whole thing
		if ($post_mode == 'reply')
		{
			$sql_data[TOPICS_TABLE]['stat'][] = 'topic_last_post_id = ' . (int) $data['post_id'];
			$sql_data[TOPICS_TABLE]['stat'][] = 'topic_last_poster_id = ' . (int) $user->data['user_id'];
			$sql_data[TOPICS_TABLE]['stat'][] = "topic_last_poster_name = '" . $db->sql_escape((!$user->data['is_registered'] && $username) ? $username : (($user->data['user_id'] != ANONYMOUS) ? $user->data['username'] : '')) . "'";
			$sql_data[TOPICS_TABLE]['stat'][] = "topic_last_poster_colour = '" . (($user->data['user_id'] != ANONYMOUS) ? $db->sql_escape($user->data['user_colour']) : '') . "'";
			$sql_data[TOPICS_TABLE]['stat'][] = "topic_last_post_subject = '" . $db->sql_escape($subject) . "'";
			$sql_data[TOPICS_TABLE]['stat'][] = 'topic_last_post_time = ' . (int) $current_time;
		}
		else if ($post_mode == 'edit_last_post' || $post_mode == 'edit_topic' || ($post_mode == 'edit_first_post' && !$data['topic_replies']))
		{
			// only the subject can be changed from edit
			$sql_data[TOPICS_TABLE]['stat'][] = "topic_last_post_subject = '" . $db->sql_escape($subject) . "'";
		}
	}
	else if (!$data['post_approved'] && ($post_mode == 'edit_last_post' || $post_mode == 'edit_topic' || ($post_mode == 'edit_first_post' && !$data['topic_replies'])))
	{
		// like having the rug pulled from under us
		$sql = 'SELECT MAX(post_id) as last_post_id
			FROM ' . POSTS_TABLE . '
			WHERE topic_id = ' . (int) $data['topic_id'] . '
				AND post_approved = 1';
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		// any posts left in this forum?
		if (!empty($row['last_post_id']))
		{
			$sql = 'SELECT p.post_id, p.post_subject, p.post_time, p.poster_id, p.post_username, u.user_id, u.username, u.user_colour
				FROM ' . POSTS_TABLE . ' p, ' . USERS_TABLE . ' u
				WHERE p.poster_id = u.user_id
					AND p.post_id = ' . (int) $row['last_post_id'];
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			// salvation, a post is found! jam it into the topics table
			$sql_data[TOPICS_TABLE]['stat'][] = 'topic_last_post_id = ' . (int) $row['post_id'];
			$sql_data[TOPICS_TABLE]['stat'][] = "topic_last_post_subject = '" . $db->sql_escape($row['post_subject']) . "'";
			$sql_data[TOPICS_TABLE]['stat'][] = 'topic_last_post_time = ' . (int) $row['post_time'];
			$sql_data[TOPICS_TABLE]['stat'][] = 'topic_last_poster_id = ' . (int) $row['poster_id'];
			$sql_data[TOPICS_TABLE]['stat'][] = "topic_last_poster_name = '" . $db->sql_escape(($row['poster_id'] == ANONYMOUS) ? $row['post_username'] : $row['username']) . "'";
			$sql_data[TOPICS_TABLE]['stat'][] = "topic_last_poster_colour = '" . $db->sql_escape($row['user_colour']) . "'";
		}
	}

	// Update total post count, do not consider moderated posts/topics
	if ($auth->acl_get('f_noapprove', $data['forum_id']) || $auth->acl_get('m_approve', $data['forum_id']))
	{
		if ($post_mode == 'post')
		{
			set_config('num_topics', $config['num_topics'] + 1, true);
			set_config('num_posts', $config['num_posts'] + 1, true);
		}

		if ($post_mode == 'reply')
		{
			set_config('num_posts', $config['num_posts'] + 1, true);
		}
	}

	// Update forum stats
	$where_sql = array(POSTS_TABLE => 'post_id = ' . $data['post_id'], TOPICS_TABLE => 'topic_id = ' . $data['topic_id'], FORUMS_TABLE => 'forum_id = ' . $data['forum_id'], USERS_TABLE => 'user_id = ' . $user->data['user_id']);

	foreach ($sql_data as $table => $update_ary)
	{
		if (isset($update_ary['stat']) && implode('', $update_ary['stat']))
		{
			$sql = "UPDATE $table SET " . implode(', ', $update_ary['stat']) . ' WHERE ' . $where_sql[$table];
			$db->sql_query($sql);
		}
	}

	// Delete topic shadows (if any exist). We do not need a shadow topic for an global announcement
	if ($make_global)
	{
		$sql = 'DELETE FROM ' . TOPICS_TABLE . '
			WHERE topic_moved_id = ' . $data['topic_id'];
		$db->sql_query($sql);
	}

	// Committing the transaction before updating search index
	$db->sql_transaction('commit');

	// Delete draft if post was loaded...
	$draft_id = request_var('draft_loaded', 0);
	if ($draft_id)
	{
		$sql = 'DELETE FROM ' . DRAFTS_TABLE . "
			WHERE draft_id = $draft_id
				AND user_id = {$user->data['user_id']}";
		$db->sql_query($sql);
	}

	// Index message contents
	if ($update_message && $data['enable_indexing'])
	{
		// Select the search method and do some additional checks to ensure it can actually be utilised
		$search_type = basename($config['search_type']);

		if (!file_exists($phpbb_root_path . 'includes/search/' . $search_type . '.' . $phpEx))
		{
			trigger_error('NO_SUCH_SEARCH_MODULE');
		}

		if (!class_exists($search_type))
		{
			include("{$phpbb_root_path}includes/search/$search_type.$phpEx");
		}

		$error = false;
		$search = new $search_type($error);

		if ($error)
		{
			trigger_error($error);
		}

		$search->index($mode, $data['post_id'], $data['message'], $subject, $poster_id, ($topic_type == POST_GLOBAL) ? 0 : $data['forum_id']);
	}

	// Topic Notification, do not change if moderator is changing other users posts...
	if ($user->data['user_id'] == $poster_id)
	{
		if (!$data['notify_set'] && $data['notify'])
		{
			$sql = 'INSERT INTO ' . TOPICS_WATCH_TABLE . ' (user_id, topic_id)
				VALUES (' . $user->data['user_id'] . ', ' . $data['topic_id'] . ')';
			$db->sql_query($sql);
		}
		else if ($data['notify_set'] && !$data['notify'])
		{
			$sql = 'DELETE FROM ' . TOPICS_WATCH_TABLE . '
				WHERE user_id = ' . $user->data['user_id'] . '
					AND topic_id = ' . $data['topic_id'];
			$db->sql_query($sql);
		}
	}

	if ($mode == 'post' || $mode == 'reply' || $mode == 'quote')
	{
		// Mark this topic as posted to
		markread('post', $data['forum_id'], $data['topic_id'], $data['post_time']);
	}

	// Mark this topic as read
	// We do not use post_time here, this is intended (post_time can have a date in the past if editing a message)
	markread('topic', $data['forum_id'], $data['topic_id'], time());

	//
	if ($config['load_db_lastread'] && $user->data['is_registered'])
	{
		$sql = 'SELECT mark_time
			FROM ' . FORUMS_TRACK_TABLE . '
			WHERE user_id = ' . $user->data['user_id'] . '
				AND forum_id = ' . $data['forum_id'];
		$result = $db->sql_query($sql);
		$f_mark_time = (int) $db->sql_fetchfield('mark_time');
		$db->sql_freeresult($result);
	}
	else if ($config['load_anon_lastread'] || $user->data['is_registered'])
	{
		$f_mark_time = false;
	}

	if (($config['load_db_lastread'] && $user->data['is_registered']) || $config['load_anon_lastread'] || $user->data['is_registered'])
	{
		// Update forum info
		$sql = 'SELECT forum_last_post_time
			FROM ' . FORUMS_TABLE . '
			WHERE forum_id = ' . $data['forum_id'];
		$result = $db->sql_query($sql);
		$forum_last_post_time = (int) $db->sql_fetchfield('forum_last_post_time');
		$db->sql_freeresult($result);

		update_forum_tracking_info($data['forum_id'], $forum_last_post_time, $f_mark_time, false);
	}

	// Send Notifications
	if ($mode != 'edit' && $mode != 'delete' && ($auth->acl_get('f_noapprove', $data['forum_id']) || $auth->acl_get('m_approve', $data['forum_id'])))
	{
		user_notification($mode, $subject, $data['topic_title'], $data['forum_name'], $data['forum_id'], $data['topic_id'], $data['post_id']);
	}

	$params = $add_anchor = '';

	if ($auth->acl_get('f_noapprove', $data['forum_id']) || $auth->acl_get('m_approve', $data['forum_id']))
	{
		$params .= '&amp;t=' . $data['topic_id'];

		if ($mode != 'post')
		{
			$params .= '&amp;p=' . $data['post_id'];
			$add_anchor = '#p' . $data['post_id'];
		}
	}
	else if ($mode != 'post' && $post_mode != 'edit_first_post' && $post_mode != 'edit_topic')
	{
		$params .= '&amp;t=' . $data['topic_id'];
	}

	$url = (!$params) ? "{$phpbb_root_path}viewforum.$phpEx" : "{$phpbb_root_path}viewtopic.$phpEx";
	$url = append_sid($url, 'f=' . $data['forum_id'] . $params) . $add_anchor;

	return $url;
}

?>