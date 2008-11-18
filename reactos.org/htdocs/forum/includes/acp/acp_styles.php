<?php
/**
*
* @package acp
* @version $Id: acp_styles.php 8479 2008-03-29 00:22:48Z naderman $
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
class acp_styles
{
	var $u_action;

	var $style_cfg;
	var $template_cfg;
	var $theme_cfg;
	var $imageset_cfg;
	var $imageset_keys;

	function main($id, $mode)
	{
		global $db, $user, $auth, $template, $cache;
		global $config, $phpbb_root_path, $phpbb_admin_path, $phpEx;

		// Hardcoded template bitfield to add for new templates
		$bitfield = new bitfield();
		$bitfield->set(0);
		$bitfield->set(3);
		$bitfield->set(8);
		$bitfield->set(9);
		$bitfield->set(11);
		$bitfield->set(12);
		define('TEMPLATE_BITFIELD', $bitfield->get_base64());
		unset($bitfield);

		$user->add_lang('acp/styles');

		$this->tpl_name = 'acp_styles';
		$this->page_title = 'ACP_CAT_STYLES';

		$action = request_var('action', '');
		$action = (isset($_POST['add'])) ? 'add' : $action;
		$style_id = request_var('id', 0);

		// Fill the configuration variables
		$this->style_cfg = $this->template_cfg = $this->theme_cfg = $this->imageset_cfg = '
#
# phpBB {MODE} configuration file
#
# @package phpBB3
# @copyright (c) 2005 phpBB Group
# @license http://opensource.org/licenses/gpl-license.php GNU Public License
#
#
# At the left is the name, please do not change this
# At the right the value is entered
# For on/off options the valid values are on, off, 1, 0, true and false
#
# Values get trimmed, if you want to add a space in front or at the end of
# the value, then enclose the value with single or double quotes.
# Single and double quotes do not need to be escaped.
#
#

# General Information about this {MODE}
name = {NAME}
copyright = {COPYRIGHT}
version = {VERSION}
';

		$this->theme_cfg .= '
# Some configuration options

#
# You have to turn this option on if you want to use the
# path template variables ({T_IMAGESET_PATH} for example) within
# your css file.
# This is mostly the case if you want to use language specific
# images within your css file.
#
parse_css_file = {PARSE_CSS_FILE}
';

		$this->imageset_keys = array(
			'logos' => array(
				'site_logo',
			),
			'buttons'	=> array(
				'icon_back_top', 'icon_contact_aim', 'icon_contact_email', 'icon_contact_icq', 'icon_contact_jabber', 'icon_contact_msnm', 'icon_contact_pm', 'icon_contact_yahoo', 'icon_contact_www', 'icon_post_delete', 'icon_post_edit', 'icon_post_info', 'icon_post_quote', 'icon_post_report', 'icon_user_online', 'icon_user_offline', 'icon_user_profile', 'icon_user_search', 'icon_user_warn', 'button_pm_forward', 'button_pm_new', 'button_pm_reply', 'button_topic_locked', 'button_topic_new', 'button_topic_reply',
			),
			'icons'		=> array(
				'icon_post_target', 'icon_post_target_unread', 'icon_topic_attach', 'icon_topic_latest', 'icon_topic_newest', 'icon_topic_reported', 'icon_topic_unapproved', 'icon_friend', 'icon_foe',
			),
			'forums'	=> array(
				'forum_link', 'forum_read', 'forum_read_locked', 'forum_read_subforum', 'forum_unread', 'forum_unread_locked', 'forum_unread_subforum', 'subforum_read', 'subforum_unread'
			),
			'folders'	=> array(
				'topic_moved', 'topic_read', 'topic_read_mine', 'topic_read_hot', 'topic_read_hot_mine', 'topic_read_locked', 'topic_read_locked_mine', 'topic_unread', 'topic_unread_mine', 'topic_unread_hot', 'topic_unread_hot_mine', 'topic_unread_locked', 'topic_unread_locked_mine', 'sticky_read', 'sticky_read_mine', 'sticky_read_locked', 'sticky_read_locked_mine', 'sticky_unread', 'sticky_unread_mine', 'sticky_unread_locked', 'sticky_unread_locked_mine', 'announce_read', 'announce_read_mine', 'announce_read_locked', 'announce_read_locked_mine', 'announce_unread', 'announce_unread_mine', 'announce_unread_locked', 'announce_unread_locked_mine', 'global_read', 'global_read_mine', 'global_read_locked', 'global_read_locked_mine', 'global_unread', 'global_unread_mine', 'global_unread_locked', 'global_unread_locked_mine', 'pm_read', 'pm_unread',
			),
			'polls'		=> array(
				'poll_left', 'poll_center', 'poll_right',
			),
			'ui'		=> array(
				'upload_bar',
			),
			'user'		=> array(
				'user_icon1', 'user_icon2', 'user_icon3', 'user_icon4', 'user_icon5', 'user_icon6', 'user_icon7', 'user_icon8', 'user_icon9', 'user_icon10',
			),
		);

		// Execute overall actions
		switch ($action)
		{
			case 'delete':
				if ($style_id)
				{
					$this->remove($mode, $style_id);
					return;
				}
			break;

			case 'export':
				if ($style_id)
				{
					$this->export($mode, $style_id);
					return;
				}
			break;

			case 'install':
				$this->install($mode);
				return;
			break;

			case 'add':
				$this->add($mode);
				return;
			break;

			case 'details':
				if ($style_id)
				{
					$this->details($mode, $style_id);
					return;
				}
			break;

			case 'edit':
				if ($style_id)
				{
					switch ($mode)
					{
						case 'imageset':
							return $this->edit_imageset($style_id);
						case 'template':
							return $this->edit_template($style_id);
						case 'theme':
							return $this->edit_theme($style_id);
					}
				}
			break;

			case 'cache':
				if ($style_id)
				{
					switch ($mode)
					{
						case 'template':
							return $this->template_cache($style_id);
					}
				}
			break;
		}

		switch ($mode)
		{
			case 'style':

				switch ($action)
				{
					case 'activate':
					case 'deactivate':

						if ($style_id == $config['default_style'])
						{
							trigger_error($user->lang['DEACTIVATE_DEFAULT'] . adm_back_link($this->u_action), E_USER_WARNING);
						}

						$sql = 'UPDATE ' . STYLES_TABLE . '
							SET style_active = ' . (($action == 'activate') ? 1 : 0) . '
							WHERE style_id = ' . $style_id;
						$db->sql_query($sql);

						// Set style to default for any member using deactivated style
						if ($action == 'deactivate')
						{
							$sql = 'UPDATE ' . USERS_TABLE . '
								SET user_style = ' . $config['default_style'] . "
								WHERE user_style = $style_id";
							$db->sql_query($sql);

							$sql = 'UPDATE ' . FORUMS_TABLE . '
								SET forum_style = 0
								WHERE forum_style = ' . $style_id;
							$db->sql_query($sql);
						}
					break;
				}

				$this->frontend('style', array('details'), array('export', 'delete'));
			break;

			case 'template':

				switch ($action)
				{
					// Refresh template data stored in db and clear cache
					case 'refresh':

						$sql = 'SELECT *
							FROM ' . STYLES_TEMPLATE_TABLE . "
							WHERE template_id = $style_id";
						$result = $db->sql_query($sql);
						$template_row = $db->sql_fetchrow($result);
						$db->sql_freeresult($result);

						if (!$template_row)
						{
							trigger_error($user->lang['NO_TEMPLATE'] . adm_back_link($this->u_action), E_USER_WARNING);
						}

						if (confirm_box(true))
						{
							$template_refreshed = '';

							// Only refresh database if the template is stored in the database
							if ($template_row['template_storedb'] && file_exists("{$phpbb_root_path}styles/{$template_row['template_path']}/template/"))
							{
								$filelist = array('' => array());

								$sql = 'SELECT template_filename, template_mtime
									FROM ' . STYLES_TEMPLATE_DATA_TABLE . "
									WHERE template_id = $style_id";
								$result = $db->sql_query($sql);

								while ($row = $db->sql_fetchrow($result))
								{
//									if (@filemtime("{$phpbb_root_path}styles/{$template_row['template_path']}/template/" . $row['template_filename']) > $row['template_mtime'])
//									{
										// get folder info from the filename
										if (($slash_pos = strrpos($row['template_filename'], '/')) === false)
										{
											$filelist[''][] = $row['template_filename'];
										}
										else
										{
											$filelist[substr($row['template_filename'], 0, $slash_pos + 1)][] = substr($row['template_filename'], $slash_pos + 1, strlen($row['template_filename']) - $slash_pos - 1);
										}
//									}
								}
								$db->sql_freeresult($result);

								$this->store_templates('update', $style_id, $template_row['template_path'], $filelist);
								unset($filelist);

								$template_refreshed = $user->lang['TEMPLATE_REFRESHED'] . '<br />';
								add_log('admin', 'LOG_TEMPLATE_REFRESHED', $template_row['template_name']);
							}

							$this->clear_template_cache($template_row);

							trigger_error($template_refreshed . $user->lang['TEMPLATE_CACHE_CLEARED'] . adm_back_link($this->u_action));
						}
						else
						{
							confirm_box(false, ($template_row['template_storedb']) ? $user->lang['CONFIRM_TEMPLATE_REFRESH'] : $user->lang['CONFIRM_TEMPLATE_CLEAR_CACHE'], build_hidden_fields(array(
								'i'			=> $id,
								'mode'		=> $mode,
								'action'	=> $action,
								'id'		=> $style_id
							)));
						}

					break;
				}

				$this->frontend('template', array('edit', 'cache', 'details'), array('refresh', 'export', 'delete'));
			break;

			case 'theme':

				switch ($action)
				{
					// Refresh theme data stored in the database
					case 'refresh':

						$sql = 'SELECT *
							FROM ' . STYLES_THEME_TABLE . "
							WHERE theme_id = $style_id";
						$result = $db->sql_query($sql);
						$theme_row = $db->sql_fetchrow($result);
						$db->sql_freeresult($result);

						if (!$theme_row)
						{
							trigger_error($user->lang['NO_THEME'] . adm_back_link($this->u_action), E_USER_WARNING);
						}

						if (!$theme_row['theme_storedb'])
						{
							trigger_error($user->lang['THEME_ERR_REFRESH_FS'] . adm_back_link($this->u_action), E_USER_WARNING);
						}

						if (confirm_box(true))
						{
							if ($theme_row['theme_storedb'] && file_exists("{$phpbb_root_path}styles/{$theme_row['theme_path']}/theme/stylesheet.css"))
							{
								// Save CSS contents
								$sql_ary = array(
									'theme_mtime'	=> (int) filemtime("{$phpbb_root_path}styles/{$theme_row['theme_path']}/theme/stylesheet.css"),
									'theme_data'	=> $this->db_theme_data($theme_row)
								);

								$sql = 'UPDATE ' . STYLES_THEME_TABLE . ' SET ' . $db->sql_build_array('UPDATE', $sql_ary) . "
									WHERE theme_id = $style_id";
								$db->sql_query($sql);

								$cache->destroy('sql', STYLES_THEME_TABLE);

								add_log('admin', 'LOG_THEME_REFRESHED', $theme_row['theme_name']);
								trigger_error($user->lang['THEME_REFRESHED'] . adm_back_link($this->u_action));
							}
						}
						else
						{
							confirm_box(false, $user->lang['CONFIRM_THEME_REFRESH'], build_hidden_fields(array(
								'i'			=> $id,
								'mode'		=> $mode,
								'action'	=> $action,
								'id'		=> $style_id
							)));
						}
					break;
				}

				$this->frontend('theme', array('edit', 'details'), array('refresh', 'export', 'delete'));
			break;

			case 'imageset':

				switch ($action)
				{
					case 'refresh':

						$sql = 'SELECT *
							FROM ' . STYLES_IMAGESET_TABLE . "
							WHERE imageset_id = $style_id";
						$result = $db->sql_query($sql);
						$imageset_row = $db->sql_fetchrow($result);
						$db->sql_freeresult($result);

						if (!$imageset_row)
						{
							trigger_error($user->lang['NO_IMAGESET'] . adm_back_link($this->u_action), E_USER_WARNING);
						}

						if (confirm_box(true))
						{
							$sql_ary = array();

							$imageset_definitions = array();
							foreach ($this->imageset_keys as $topic => $key_array)
							{
								$imageset_definitions = array_merge($imageset_definitions, $key_array);
							}

							$cfg_data_imageset = parse_cfg_file("{$phpbb_root_path}styles/{$imageset_row['imageset_path']}/imageset/imageset.cfg");

							$db->sql_transaction('begin');

							$sql = 'DELETE FROM ' . STYLES_IMAGESET_DATA_TABLE . '
								WHERE imageset_id = ' . $style_id;
							$result = $db->sql_query($sql);

							foreach ($cfg_data_imageset as $image_name => $value)
							{
								if (strpos($value, '*') !== false)
								{
									if (substr($value, -1, 1) === '*')
									{
										list($image_filename, $image_height) = explode('*', $value);
										$image_width = 0;
									}
									else
									{
										list($image_filename, $image_height, $image_width) = explode('*', $value);
									}
								}
								else
								{
									$image_filename = $value;
									$image_height = $image_width = 0;
								}

								if (strpos($image_name, 'img_') === 0 && $image_filename)
								{
									$image_name = substr($image_name, 4);
									if (in_array($image_name, $imageset_definitions))
									{
										$sql_ary[] = array(
											'image_name'		=> (string) $image_name,
											'image_filename'	=> (string) $image_filename,
											'image_height'		=> (int) $image_height,
											'image_width'		=> (int) $image_width,
											'imageset_id'		=> (int) $style_id,
											'image_lang'		=> '',
										);
									}
								}
							}

							$sql = 'SELECT lang_dir
								FROM ' . LANG_TABLE;
							$result = $db->sql_query($sql);

							while ($row = $db->sql_fetchrow($result))
							{
								if (@file_exists("{$phpbb_root_path}styles/{$imageset_row['imageset_path']}/imageset/{$row['lang_dir']}/imageset.cfg"))
								{
									$cfg_data_imageset_data = parse_cfg_file("{$phpbb_root_path}styles/{$imageset_row['imageset_path']}/imageset/{$row['lang_dir']}/imageset.cfg");
									foreach ($cfg_data_imageset_data as $image_name => $value)
									{
										if (strpos($value, '*') !== false)
										{
											if (substr($value, -1, 1) === '*')
											{
												list($image_filename, $image_height) = explode('*', $value);
												$image_width = 0;
											}
											else
											{
												list($image_filename, $image_height, $image_width) = explode('*', $value);
											}
										}
										else
										{
											$image_filename = $value;
											$image_height = $image_width = 0;
										}

										if (strpos($image_name, 'img_') === 0 && $image_filename)
										{
											$image_name = substr($image_name, 4);
											if (in_array($image_name, $imageset_definitions))
											{
												$sql_ary[] = array(
													'image_name'		=> (string) $image_name,
													'image_filename'	=> (string) $image_filename,
													'image_height'		=> (int) $image_height,
													'image_width'		=> (int) $image_width,
													'imageset_id'		=> (int) $style_id,
													'image_lang'		=> (string) $row['lang_dir'],
												);
											}
										}
									}
								}
							}
							$db->sql_freeresult($result);

							$db->sql_multi_insert(STYLES_IMAGESET_DATA_TABLE, $sql_ary);

							$db->sql_transaction('commit');

							$cache->destroy('sql', STYLES_IMAGESET_DATA_TABLE);

							add_log('admin', 'LOG_IMAGESET_REFRESHED', $imageset_row['imageset_name']);
							trigger_error($user->lang['IMAGESET_REFRESHED'] . adm_back_link($this->u_action));
						}
						else
						{
							confirm_box(false, $user->lang['CONFIRM_IMAGESET_REFRESH'], build_hidden_fields(array(
								'i'			=> $id,
								'mode'		=> $mode,
								'action'	=> $action,
								'id'		=> $style_id
							)));
						}
					break;
				}

				$this->frontend('imageset', array('edit', 'details'), array('refresh', 'export', 'delete'));
			break;
		}
	}

	/**
	* Build Frontend with supplied options
	*/
	function frontend($mode, $options, $actions)
	{
		global $user, $template, $db, $config, $phpbb_root_path, $phpEx;

		$sql_from = '';
		$style_count = array();

		switch ($mode)
		{
			case 'style':
				$sql_from = STYLES_TABLE;

				$sql = 'SELECT user_style, COUNT(user_style) AS style_count
					FROM ' . USERS_TABLE . '
					GROUP BY user_style';
				$result = $db->sql_query($sql);

				while ($row = $db->sql_fetchrow($result))
				{
					$style_count[$row['user_style']] = $row['style_count'];
				}
				$db->sql_freeresult($result);

			break;

			case 'template':
				$sql_from = STYLES_TEMPLATE_TABLE;
			break;

			case 'theme':
				$sql_from = STYLES_THEME_TABLE;
			break;

			case 'imageset':
				$sql_from = STYLES_IMAGESET_TABLE;
			break;
		}

		$l_prefix = strtoupper($mode);

		$this->page_title = 'ACP_' . $l_prefix . 'S';

		$template->assign_vars(array(
			'S_FRONTEND'		=> true,
			'S_STYLE'			=> ($mode == 'style') ? true : false,

			'L_TITLE'			=> $user->lang[$this->page_title],
			'L_EXPLAIN'			=> $user->lang[$this->page_title . '_EXPLAIN'],
			'L_NAME'			=> $user->lang[$l_prefix . '_NAME'],
			'L_INSTALLED'		=> $user->lang['INSTALLED_' . $l_prefix],
			'L_UNINSTALLED'		=> $user->lang['UNINSTALLED_' . $l_prefix],
			'L_NO_UNINSTALLED'	=> $user->lang['NO_UNINSTALLED_' . $l_prefix],
			'L_CREATE'			=> $user->lang['CREATE_' . $l_prefix],

			'U_ACTION'			=> $this->u_action,
			)
		);

		$sql = "SELECT *
			FROM $sql_from";
		$result = $db->sql_query($sql);

		$installed = array();

		$basis_options = '<option class="sep" value="">' . $user->lang['OPTIONAL_BASIS'] . '</option>';
		while ($row = $db->sql_fetchrow($result))
		{
			$installed[] = $row[$mode . '_name'];
			$basis_options .= '<option value="' . $row[$mode . '_id'] . '">' . $row[$mode . '_name'] . '</option>';

			$stylevis = ($mode == 'style' && !$row['style_active']) ? 'activate' : 'deactivate';

			$s_options = array();
			foreach ($options as $option)
			{
				$s_options[] = '<a href="' . $this->u_action . "&amp;action=$option&amp;id=" . $row[$mode . '_id'] . '">' . $user->lang[strtoupper($option)] . '</a>';
			}

			$s_actions = array();
			foreach ($actions as $option)
			{
				$s_actions[] = '<a href="' . $this->u_action . "&amp;action=$option&amp;id=" . $row[$mode . '_id'] . '">' . $user->lang[strtoupper($option)] . '</a>';
			}

			$template->assign_block_vars('installed', array(
				'S_DEFAULT_STYLE'		=> ($mode == 'style' && $row['style_id'] == $config['default_style']) ? true : false,
				'U_EDIT'				=> $this->u_action . '&amp;action=' . (($mode == 'style') ? 'details' : 'edit') . '&amp;id=' . $row[$mode . '_id'],
				'U_STYLE_ACT_DEACT'		=> $this->u_action . '&amp;action=' . $stylevis . '&amp;id=' . $row[$mode . '_id'],
				'L_STYLE_ACT_DEACT'		=> $user->lang['STYLE_' . strtoupper($stylevis)],
				'S_OPTIONS'				=> implode(' | ', $s_options),
				'S_ACTIONS'				=> implode(' | ', $s_actions),
				'U_PREVIEW'				=> ($mode == 'style') ? append_sid("{$phpbb_root_path}index.$phpEx", "$mode=" . $row[$mode . '_id']) : '',

				'NAME'					=> $row[$mode . '_name'],
				'STYLE_COUNT'			=> ($mode == 'style' && isset($style_count[$row['style_id']])) ? $style_count[$row['style_id']] : 0,
				)
			);
		}
		$db->sql_freeresult($result);

		// Grab uninstalled items
		$new_ary = $cfg = array();

		$dp = @opendir("{$phpbb_root_path}styles");

		if ($dp)
		{
			while (($file = readdir($dp)) !== false)
			{
				$subpath = ($mode != 'style') ? "$mode/" : '';
				if ($file[0] != '.' && file_exists("{$phpbb_root_path}styles/$file/$subpath$mode.cfg"))
				{
					if ($cfg = file("{$phpbb_root_path}styles/$file/$subpath$mode.cfg"))
					{
						$items = parse_cfg_file('', $cfg);
						$name = (isset($items['name'])) ? trim($items['name']) : false;

						if ($name && !in_array($name, $installed))
						{
							$new_ary[] = array(
								'path'		=> $file,
								'name'		=> $name,
								'copyright'	=> $items['copyright'],
							);
						}
					}
				}
			}
			closedir($dp);
		}

		unset($installed);

		if (sizeof($new_ary))
		{
			foreach ($new_ary as $cfg)
			{
				$template->assign_block_vars('uninstalled', array(
					'NAME'			=> $cfg['name'],
					'COPYRIGHT'		=> $cfg['copyright'],
					'U_INSTALL'		=> $this->u_action . '&amp;action=install&amp;path=' . urlencode($cfg['path']))
				);
			}
		}
		unset($new_ary);

		$template->assign_vars(array(
			'S_BASIS_OPTIONS'		=> $basis_options)
		);

	}

	/**
	* Provides a template editor which allows saving changes to template files on the filesystem or in the database.
	*
	* @param int $template_id specifies which template set is being edited
	*/
	function edit_template($template_id)
	{
		global $phpbb_root_path, $phpEx, $config, $db, $cache, $user, $template, $safe_mode;

		$this->page_title = 'EDIT_TEMPLATE';

		$filelist = $filelist_cats = array();

		$template_data	= utf8_normalize_nfc(request_var('template_data', '', true));
		$template_data	= htmlspecialchars_decode($template_data);
		$template_file	= utf8_normalize_nfc(request_var('template_file', '', true));
		$text_rows		= max(5, min(999, request_var('text_rows', 20)));
		$save_changes	= (isset($_POST['save'])) ? true : false;

		// make sure template_file path doesn't go upwards
		$template_file = str_replace('..', '.', $template_file);
		
		// Retrieve some information about the template
		$sql = 'SELECT template_storedb, template_path, template_name
			FROM ' . STYLES_TEMPLATE_TABLE . "
			WHERE template_id = $template_id";
		$result = $db->sql_query($sql);
		$template_info = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if (!$template_info)
		{
			trigger_error($user->lang['NO_TEMPLATE'] . adm_back_link($this->u_action), E_USER_WARNING);
		}

		// save changes to the template if the user submitted any
		if ($save_changes && $template_file)
		{
			// Get the filesystem location of the current file
			$file = "{$phpbb_root_path}styles/{$template_info['template_path']}/template/$template_file";
			$additional = '';

			// If the template is stored on the filesystem try to write the file else store it in the database
			if (!$safe_mode && !$template_info['template_storedb'] && file_exists($file) && @is_writable($file))
			{
				if (!($fp = @fopen($file, 'wb')))
				{
					trigger_error($user->lang['NO_TEMPLATE'] . adm_back_link($this->u_action), E_USER_WARNING);
				}
				fwrite($fp, $template_data);
				fclose($fp);
			}
			else
			{
				$db->sql_transaction('begin');

				// If it's not stored in the db yet, then update the template setting and store all template files in the db
				if (!$template_info['template_storedb'])
				{
					$sql = 'UPDATE ' . STYLES_TEMPLATE_TABLE . '
						SET template_storedb = 1
						WHERE template_id = ' . $template_id;
					$db->sql_query($sql);

					$filelist = filelist("{$phpbb_root_path}styles/{$template_info['template_path']}/template", '', 'html');
					$this->store_templates('insert', $template_id, $template_info['template_path'], $filelist);

					add_log('admin', 'LOG_TEMPLATE_EDIT_DETAILS', $template_info['template_name']);
					$additional .= '<br />' . $user->lang['EDIT_TEMPLATE_STORED_DB'];
				}

				// Update the template_data table entry for this template file
				$sql = 'UPDATE ' . STYLES_TEMPLATE_DATA_TABLE . "
					SET template_data = '" . $db->sql_escape($template_data) . "', template_mtime = " . time() . "
					WHERE template_id = $template_id
						AND template_filename = '" . $db->sql_escape($template_file) . "'";
				$db->sql_query($sql);

				$db->sql_transaction('commit');
			}

			// destroy the cached version of the template (filename without extension)
			$this->clear_template_cache($template_info, array(substr($template_file, 0, -5)));

			$cache->destroy('sql', STYLES_TABLE);

			add_log('admin', 'LOG_TEMPLATE_EDIT', $template_info['template_name'], $template_file);
			trigger_error($user->lang['TEMPLATE_FILE_UPDATED'] . $additional . adm_back_link($this->u_action . "&amp;action=edit&amp;id=$template_id&amp;text_rows=$text_rows&amp;template_file=$template_file"));
		}

		// Generate a category array containing template filenames
		if (!$template_info['template_storedb'])
		{
			$template_path = "{$phpbb_root_path}styles/{$template_info['template_path']}/template";

			$filelist = filelist($template_path, '', 'html');
			$filelist[''] = array_diff($filelist[''], array('bbcode.html'));

			if ($template_file)
			{
				if (!file_exists($template_path . "/$template_file") || !($template_data = file_get_contents($template_path . "/$template_file")))
				{
					trigger_error($user->lang['NO_TEMPLATE'] . adm_back_link($this->u_action), E_USER_WARNING);
				}
			}
		}
		else
		{
			$sql = 'SELECT *
				FROM ' . STYLES_TEMPLATE_DATA_TABLE . "
				WHERE template_id = $template_id";
			$result = $db->sql_query($sql);

			$filelist = array('' => array());
			while ($row = $db->sql_fetchrow($result))
			{
				$file_info = pathinfo($row['template_filename']);

				if (($file_info['basename'] != 'bbcode') && ($file_info['extension'] == 'html'))
				{
					if (($file_info['dirname'] == '.') || empty($file_info['dirname']))
					{
						$filelist[''][] = $row['template_filename'];
					}
					else
					{
						$filelist[$file_info['dirname'] . '/'][] = $file_info['basename'];
					}
				}

				if ($row['template_filename'] == $template_file)
				{
					$template_data = $row['template_data'];
				}
			}
			$db->sql_freeresult($result);
			unset($file_info);
		}

		// Now create the categories
		$filelist_cats[''] = array();
		foreach ($filelist as $pathfile => $file_ary)
		{
			// Use the directory name as category name
			if (!empty($pathfile))
			{
				$filelist_cats[$pathfile] = array();
				foreach ($file_ary as $file)
				{
					$filelist_cats[$pathfile][$pathfile . $file] = $file;
				}
			}
			// or if it's in the main category use the word before the first underscore to group files
			else
			{
				$cats = array();
				foreach ($file_ary as $file)
				{
					$cats[] = substr($file, 0, strpos($file, '_'));
					$filelist_cats[substr($file, 0, strpos($file, '_'))][$file] = $file;
				}

				$cats = array_values(array_unique($cats));

				// we don't need any single element categories so put them into the misc '' category
				for ($i = 0, $n = sizeof($cats); $i < $n; $i++)
				{
					if (sizeof($filelist_cats[$cats[$i]]) == 1 && $cats[$i] !== '')
					{
						$filelist_cats[''][key($filelist_cats[$cats[$i]])] = current($filelist_cats[$cats[$i]]);
						unset($filelist_cats[$cats[$i]]);
					}
				}
				unset($cats);
			}
		}
		unset($filelist);

		// Generate list of categorised template files
		$tpl_options = '';
		ksort($filelist_cats);
		foreach ($filelist_cats as $category => $tpl_ary)
		{
			ksort($tpl_ary);

			if (!empty($category))
			{
				$tpl_options .= '<option class="sep" value="">' . $category . '</option>';
			}

			foreach ($tpl_ary as $filename => $file)
			{
				$selected = ($template_file == $filename) ? ' selected="selected"' : '';
				$tpl_options .= '<option value="' . $filename . '"' . $selected . '>' . $file . '</option>';
			}
		}

		$template->assign_vars(array(
			'S_EDIT_TEMPLATE'	=> true,
			'S_HIDDEN_FIELDS'	=> build_hidden_fields(array('template_file' => $template_file)),
			'S_TEMPLATES'		=> $tpl_options,

			'U_ACTION'			=> $this->u_action . "&amp;action=edit&amp;id=$template_id&amp;text_rows=$text_rows",
			'U_BACK'			=> $this->u_action,

			'L_EDIT'			=> $user->lang['EDIT_TEMPLATE'],
			'L_EDIT_EXPLAIN'	=> $user->lang['EDIT_TEMPLATE_EXPLAIN'],
			'L_EDITOR'			=> $user->lang['TEMPLATE_EDITOR'],
			'L_EDITOR_HEIGHT'	=> $user->lang['TEMPLATE_EDITOR_HEIGHT'],
			'L_FILE'			=> $user->lang['TEMPLATE_FILE'],
			'L_SELECT'			=> $user->lang['SELECT_TEMPLATE'],
			'L_SELECTED'		=> $user->lang['SELECTED_TEMPLATE'],
			'L_SELECTED_FILE'	=> $user->lang['SELECTED_TEMPLATE_FILE'],

			'SELECTED_TEMPLATE'	=> $template_info['template_name'],
			'TEMPLATE_FILE'		=> $template_file,
			'TEMPLATE_DATA'		=> utf8_htmlspecialchars($template_data),
			'TEXT_ROWS'			=> $text_rows)
		);
	}

	/**
	* Allows the admin to view cached versions of template files and clear single template cache files
	*
	* @param int $template_id specifies which template's cache is shown
	*/
	function template_cache($template_id)
	{
		global $phpbb_root_path, $phpEx, $config, $db, $cache, $user, $template;

		$source		= str_replace('/', '.', request_var('source', ''));
		$file_ary	= array_diff(request_var('delete', array('')), array(''));
		$submit		= isset($_POST['submit']) ? true : false;

		$sql = 'SELECT *
			FROM ' . STYLES_TEMPLATE_TABLE . "
			WHERE template_id = $template_id";
		$result = $db->sql_query($sql);
		$template_row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if (!$template_row)
		{
			trigger_error($user->lang['NO_TEMPLATE'] . adm_back_link($this->u_action), E_USER_WARNING);
		}

		// User wants to delete one or more files ...
		if ($submit && $file_ary)
		{
			$this->clear_template_cache($template_row, $file_ary);
			trigger_error($user->lang['TEMPLATE_CACHE_CLEARED'] . adm_back_link($this->u_action . "&amp;action=cache&amp;id=$template_id"));
		}

		$cache_prefix = 'tpl_' . $template_row['template_path'];

		// Someone wants to see the cached source ... so we'll highlight it,
		// add line numbers and indent it appropriately. This could be nasty
		// on larger source files ...
		if ($source && file_exists("{$phpbb_root_path}cache/{$cache_prefix}_$source.html.$phpEx"))
		{
			adm_page_header($user->lang['TEMPLATE_CACHE']);

			$template->set_filenames(array(
				'body'	=> 'viewsource.html')
			);

			$template->assign_vars(array(
				'FILENAME'	=> str_replace('.', '/', $source) . '.html')
			);

			$code = str_replace(array("\r\n", "\r"), array("\n", "\n"), file_get_contents("{$phpbb_root_path}cache/{$cache_prefix}_$source.html.$phpEx"));

			$conf = array('highlight.bg', 'highlight.comment', 'highlight.default', 'highlight.html', 'highlight.keyword', 'highlight.string');
			foreach ($conf as $ini_var)
			{
				@ini_set($ini_var, str_replace('highlight.', 'syntax', $ini_var));
			}

			$marker = 'MARKER' . time();
			$code = highlight_string(str_replace("\n", $marker, $code), true);
			$code = str_replace($marker, "\n", $code);
			$str_from = array('<span style="color: ', '<font color="syntax', '</font>', '<code>', '</code>','[', ']', '.', ':');
			$str_to = array('<span class="', '<span class="syntax', '</span>', '', '', '&#91;', '&#93;', '&#46;', '&#58;');

			$code = str_replace($str_from, $str_to, $code);
			$code = preg_replace('#^(<span class="[a-z_]+">)\n?(.*?)\n?(</span>)$#ism', '$1$2$3', $code);
			$code = substr($code, strlen('<span class="syntaxhtml">'));
			$code = substr($code, 0, -1 * strlen('</ span>'));
			$code = explode("\n", $code);

			foreach ($code as $key => $line)
			{
				$template->assign_block_vars('source', array(
					'LINENUM'	=> $key + 1,
					'LINE'		=> preg_replace('#([^ ;])&nbsp;([^ &])#', '$1 $2', $line))
				);
				unset($code[$key]);
			}

			adm_page_footer();
		}

		$filemtime = array();
		if ($template_row['template_storedb'])
		{
			$sql = 'SELECT template_filename, template_mtime
				FROM ' . STYLES_TEMPLATE_DATA_TABLE . "
				WHERE template_id = $template_id";
			$result = $db->sql_query($sql);

			$filemtime = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$filemtime[$row['template_filename']] = $row['template_mtime'];
			}
			$db->sql_freeresult($result);
		}

		// Get a list of cached template files and then retrieve additional information about them
		$file_ary = $this->template_cache_filelist($template_row['template_path']);

		foreach ($file_ary as $file)
		{
			$file 		= str_replace('/', '.', $file);
			
			// perform some dirty guessing to get the path right.
			// We assume that three dots in a row were '../'
			$tpl_file 	= str_replace('.', '/', $file);
			$tpl_file 	= str_replace('///', '../', $tpl_file);
			
			$filename = "{$cache_prefix}_$file.html.$phpEx";

			if (!file_exists("{$phpbb_root_path}cache/$filename"))
			{
				continue;
			}

			$template->assign_block_vars('file', array(
				'U_VIEWSOURCE'	=> $this->u_action . "&amp;action=cache&amp;id=$template_id&amp;source=$file",

				'CACHED'		=> $user->format_date(filemtime("{$phpbb_root_path}cache/$filename")),
				'FILENAME'		=> $file,
				'FILESIZE'		=> sprintf('%.1f ' . $user->lang['KIB'], filesize("{$phpbb_root_path}cache/$filename") / 1024),
				'MODIFIED'		=> $user->format_date((!$template_row['template_storedb']) ? filemtime("{$phpbb_root_path}styles/{$template_row['template_path']}/template/$tpl_file.html") : $filemtime[$file . '.html']))
			);
		}
		unset($filemtime);

		$template->assign_vars(array(
			'S_CACHE'			=> true,
			'S_TEMPLATE'		=> true,

			'U_ACTION'			=> $this->u_action . "&amp;action=cache&amp;id=$template_id",
			'U_BACK'			=> $this->u_action)
		);
	}

	/**
	* Provides a css editor and a basic easier to use stylesheet editing tool for less experienced (or lazy) users
	*
	* @param int $theme_id specifies which theme is being edited
	*/
	function edit_theme($theme_id)
	{
		global $phpbb_root_path, $phpEx, $config, $db, $cache, $user, $template, $safe_mode;

		$this->page_title = 'EDIT_THEME';

		$filelist = $filelist_cats = array();

		$theme_data		= utf8_normalize_nfc(request_var('template_data', '', true));
		$theme_data		= htmlspecialchars_decode($theme_data);
		$theme_file		= utf8_normalize_nfc(request_var('template_file', '', true));
		$text_rows		= max(5, min(999, request_var('text_rows', 20)));
		$save_changes	= (isset($_POST['save'])) ? true : false;

		// make sure theme_file path doesn't go upwards
		$theme_file = str_replace('..', '.', $theme_file);
		
		// Retrieve some information about the theme
		$sql = 'SELECT theme_storedb, theme_path, theme_name, theme_data
			FROM ' . STYLES_THEME_TABLE . "
			WHERE theme_id = $theme_id";
		$result = $db->sql_query($sql);

		if (!($theme_info = $db->sql_fetchrow($result)))
		{
			trigger_error($user->lang['NO_THEME'] . adm_back_link($this->u_action), E_USER_WARNING);
		}
		$db->sql_freeresult($result);

		// save changes to the theme if the user submitted any
		if ($save_changes)
		{
			// Get the filesystem location of the current file
			$file = "{$phpbb_root_path}styles/{$theme_info['theme_path']}/theme/$theme_file";
			$additional = '';
			$message = $user->lang['THEME_UPDATED'];

			// If the theme is stored on the filesystem try to write the file else store it in the database
			if (!$safe_mode && !$theme_info['theme_storedb'] && file_exists($file) && @is_writable($file))
			{
				if (!($fp = @fopen($file, 'wb')))
				{
					trigger_error($user->lang['NO_THEME'] . adm_back_link($this->u_action), E_USER_WARNING);
				}
				fwrite($fp, $theme_data);
				fclose($fp);
			}
			else
			{
				// Write stylesheet to db
				$sql_ary = array(
					'theme_mtime'		=> time(),
					'theme_storedb'		=> 1,
					'theme_data'		=> $this->db_theme_data($theme_info, $theme_data),
				);
				$sql = 'UPDATE ' . STYLES_THEME_TABLE . '
					SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
					WHERE theme_id = ' . $theme_id;
				$db->sql_query($sql);

				$cache->destroy('sql', STYLES_THEME_TABLE);

				// notify the user if the theme was not stored in the db before his modification
				if (!$theme_info['theme_storedb'])
				{
					add_log('admin', 'LOG_THEME_EDIT_DETAILS', $theme_info['theme_name']);
					$message .= '<br />' . $user->lang['EDIT_THEME_STORED_DB'];
				}
			}
			$cache->destroy('sql', STYLES_THEME_TABLE);
			add_log('admin', (!$theme_info['theme_storedb']) ? 'LOG_THEME_EDIT_FILE' : 'LOG_THEME_EDIT', $theme_info['theme_name'], (!$theme_info['theme_storedb']) ? $theme_file : '');

			trigger_error($message . adm_back_link($this->u_action . "&amp;action=edit&amp;id=$theme_id&amp;template_file=$theme_file&amp;text_rows=$text_rows"));
		}

		// Generate a category array containing theme filenames
		if (!$theme_info['theme_storedb'])
		{
			$theme_path = "{$phpbb_root_path}styles/{$theme_info['theme_path']}/theme";

			$filelist = filelist($theme_path, '', 'css');

			if ($theme_file)
			{
				if (!file_exists($theme_path . "/$theme_file") || !($theme_data = file_get_contents($theme_path . "/$theme_file")))
				{
					trigger_error($user->lang['NO_THEME'] . adm_back_link($this->u_action), E_USER_WARNING);
				}
			}
		}
		else
		{
			$theme_data = &$theme_info['theme_data'];
		}

		// Now create the categories
		$filelist_cats[''] = array();
		foreach ($filelist as $pathfile => $file_ary)
		{
			// Use the directory name as category name
			if (!empty($pathfile))
			{
				$filelist_cats[$pathfile] = array();
				foreach ($file_ary as $file)
				{
					$filelist_cats[$pathfile][$pathfile . $file] = $file;
				}
			}
			// or if it's in the main category use the word before the first underscore to group files
			else
			{
				$cats = array();
				foreach ($file_ary as $file)
				{
					$cats[] = substr($file, 0, strpos($file, '_'));
					$filelist_cats[substr($file, 0, strpos($file, '_'))][$file] = $file;
				}

				$cats = array_values(array_unique($cats));

				// we don't need any single element categories so put them into the misc '' category
				for ($i = 0, $n = sizeof($cats); $i < $n; $i++)
				{
					if (sizeof($filelist_cats[$cats[$i]]) == 1 && $cats[$i] !== '')
					{
						$filelist_cats[''][key($filelist_cats[$cats[$i]])] = current($filelist_cats[$cats[$i]]);
						unset($filelist_cats[$cats[$i]]);
					}
				}
				unset($cats);
			}
		}
		unset($filelist);

		// Generate list of categorised theme files
		$tpl_options = '';
		ksort($filelist_cats);
		foreach ($filelist_cats as $category => $tpl_ary)
		{
			ksort($tpl_ary);

			if (!empty($category))
			{
				$tpl_options .= '<option class="sep" value="">' . $category . '</option>';
			}

			foreach ($tpl_ary as $filename => $file)
			{
				$selected = ($theme_file == $filename) ? ' selected="selected"' : '';
				$tpl_options .= '<option value="' . $filename . '"' . $selected . '>' . $file . '</option>';
			}
		}

		$template->assign_vars(array(
			'S_EDIT_THEME'		=> true,
			'S_HIDDEN_FIELDS'	=> build_hidden_fields(array('template_file' => $theme_file)),
			'S_THEME_IN_DB'		=> $theme_info['theme_storedb'],
			'S_TEMPLATES'		=> $tpl_options,

			'U_ACTION'			=> $this->u_action . "&amp;action=edit&amp;id=$theme_id&amp;text_rows=$text_rows",
			'U_BACK'			=> $this->u_action,

			'L_EDIT'			=> $user->lang['EDIT_THEME'],
			'L_EDIT_EXPLAIN'	=> $user->lang['EDIT_THEME_EXPLAIN'],
			'L_EDITOR'			=> $user->lang['THEME_EDITOR'],
			'L_EDITOR_HEIGHT'	=> $user->lang['THEME_EDITOR_HEIGHT'],
			'L_FILE'			=> $user->lang['THEME_FILE'],
			'L_SELECT'			=> $user->lang['SELECT_THEME'],
			'L_SELECTED'		=> $user->lang['SELECTED_THEME'],
			'L_SELECTED_FILE'	=> $user->lang['SELECTED_THEME_FILE'],

			'SELECTED_TEMPLATE'	=> $theme_info['theme_name'],
			'TEMPLATE_FILE'		=> $theme_file,
			'TEMPLATE_DATA'		=> utf8_htmlspecialchars($theme_data),
			'TEXT_ROWS'			=> $text_rows)
		);
	}


	/**
	* Edit imagesets
	*
	* @param int $imageset_id specifies which imageset is being edited
	*/
	function edit_imageset($imageset_id)
	{
		global $db, $user, $phpbb_root_path, $cache, $template;

		$this->page_title = 'EDIT_IMAGESET';

		$update		= (isset($_POST['update'])) ? true : false;

		$imgname	= request_var('imgname', '');
		$imgpath	= request_var('imgpath', '');
		$imgsize	= request_var('imgsize', false);
		$imgwidth	= request_var('imgwidth', 0);
		$imgheight	= request_var('imgheight', 0);
		
		$imgname	= preg_replace('#[^a-z0-9\-+_]#i', '', $imgname);
		$imgpath	= str_replace('..', '.', $imgpath);

		if ($imageset_id)
		{
			$sql = 'SELECT imageset_path, imageset_name
				FROM ' . STYLES_IMAGESET_TABLE . "
				WHERE imageset_id = $imageset_id";
			$result = $db->sql_query($sql);
			$imageset_row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			$imageset_path		= $imageset_row['imageset_path'];
			$imageset_name		= $imageset_row['imageset_name'];

			$sql_extra = '';
			if (strpos($imgname, '-') !== false)
			{
				list($imgname, $imgnamelang) = explode('-', $imgname);
				$sql_extra = " AND image_lang IN ('" . $db->sql_escape($imgnamelang) . "', '')";
			}

			$sql = 'SELECT image_filename, image_width, image_height, image_lang, image_id
				FROM ' . STYLES_IMAGESET_DATA_TABLE . "
				WHERE imageset_id = $imageset_id
					AND image_name = '" . $db->sql_escape($imgname) . "'$sql_extra";
			$result = $db->sql_query($sql);
			$imageset_data_row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			$image_filename	= $imageset_data_row['image_filename'];
			$image_width	= $imageset_data_row['image_width'];
			$image_height	= $imageset_data_row['image_height'];
			$image_lang		= $imageset_data_row['image_lang'];
			$image_id		= $imageset_data_row['image_id'];

			if (!$imageset_row)
			{
				trigger_error($user->lang['NO_IMAGESET'] . adm_back_link($this->u_action), E_USER_WARNING);
			}

			// Check to see whether the selected image exists in the table
			$valid_name = ($update) ? false : true;

			foreach ($this->imageset_keys as $category => $img_ary)
			{
				if (in_array($imgname, $img_ary))
				{
					$valid_name = true;
					break;
				}
			}

			if ($update && isset($_POST['imgpath']))
			{
				if ($valid_name)
				{
					// If imgwidth and imgheight are non-zero grab the actual size
					// from the image itself ... we ignore width settings for the poll center image
					$imgwidth	= request_var('imgwidth', 0);
					$imgheight	= request_var('imgheight', 0);
					$imglang = '';

					if ($imgpath && !file_exists("{$phpbb_root_path}styles/$imageset_path/imageset/$imgpath"))
					{
						trigger_error($user->lang['NO_IMAGE_ERROR'] . adm_back_link($this->u_action), E_USER_WARNING);
					}

					if ($imgsize && $imgpath)
					{
						if (!$imgwidth || !$imgheight)
						{
							list($imgwidth_file, $imgheight_file) = getimagesize("{$phpbb_root_path}styles/$imageset_path/imageset/$imgpath");
							$imgwidth = ($imgwidth) ? $imgwidth : $imgwidth_file;
							$imgheight = ($imgheight) ? $imgheight : $imgheight_file;
						}
						$imgwidth	= ($imgname != 'poll_center') ? (int) $imgwidth : 0;
						$imgheight	= (int) $imgheight;
					}


					if (strpos($imgpath, '/') !== false)
					{
						list($imglang, $imgfilename) = explode('/', $imgpath);
					}
					else
					{
						$imgfilename = $imgpath;
					}

					$sql_ary = array(
						'image_filename'	=> (string) $imgfilename,
						'image_width'		=> (int) $imgwidth,
						'image_height'		=> (int) $imgheight,
						'image_lang'		=> (string) $imglang,
					);

					// already exists
					if ($imageset_data_row)
					{
						$sql = 'UPDATE ' . STYLES_IMAGESET_DATA_TABLE . '
							SET ' . $db->sql_build_array('UPDATE', $sql_ary) . "
							WHERE image_id = $image_id";
						$db->sql_query($sql);
					}
					// does not exist
					else if (!$imageset_data_row)
					{
						$sql_ary['image_name']	= $imgname;
						$sql_ary['imageset_id']	= (int) $imageset_id;
						$db->sql_query('INSERT INTO ' . STYLES_IMAGESET_DATA_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary));
					}

					$cache->destroy('sql', STYLES_IMAGESET_DATA_TABLE);

					add_log('admin', 'LOG_IMAGESET_EDIT', $imageset_name);

					$template->assign_var('SUCCESS', true);

					$image_filename = $imgfilename;
					$image_width	= $imgwidth;
					$image_height	= $imgheight;
					$image_lang		= $imglang;
				}
			}
		}

		$imglang = '';
		$imagesetlist = array('nolang' => array(), 'lang' => array());
		$langs = array();

		$dir = "{$phpbb_root_path}styles/$imageset_path/imageset";
		$dp = @opendir($dir);

		if ($dp)
		{
			while (($file = readdir($dp)) !== false)
			{
				if ($file[0] != '.' && strtoupper($file) != 'CVS' && !is_file($dir . '/' . $file) && !is_link($dir . '/' . $file))
				{
					$langs[] = $file;
				}
				else if (preg_match('#\.(?:gif|jpg|png)$#', $file))
				{
					$imagesetlist['nolang'][] = $file;
				}
			}

			if ($sql_extra)
			{
				$dp2 = @opendir("$dir/$imgnamelang");

				if ($dp2)
				{
					while (($file2 = readdir($dp2)) !== false)
					{
						if (preg_match('#\.(?:gif|jpg|png)$#', $file2))
						{
							$imagesetlist['lang'][] = "$imgnamelang/$file2";
						}
					}
					closedir($dp2);
				}
			}
			closedir($dp);
		}

		// Generate list of image options
		$img_options = '';
		foreach ($this->imageset_keys as $category => $img_ary)
		{
			$template->assign_block_vars('category', array(
				'NAME'			=> $user->lang['IMG_CAT_' . strtoupper($category)]
			));

			foreach ($img_ary as $img)
			{
				if ($category == 'buttons')
				{
					foreach ($langs as $language)
					{
						$template->assign_block_vars('category.images', array(
							'SELECTED'			=> ($img == $imgname && $language == $imgnamelang),
							'VALUE'				=> $img . '-' . $language,
							'TEXT'				=> $user->lang['IMG_' . strtoupper($img)] . ' [ ' . $language . ' ]'
						));
					}
				}
				else
				{
					$template->assign_block_vars('category.images', array(
						'SELECTED'			=> ($img == $imgname),
						'VALUE'				=> $img,
						'TEXT'				=> (($category == 'custom') ? $img : $user->lang['IMG_' . strtoupper($img)])
					));
				}
			}
		}

		// Make sure the list of possible images is sorted alphabetically
		sort($imagesetlist['lang']);
		sort($imagesetlist['nolang']);

		$image_found = false;
		$img_val = '';
		foreach ($imagesetlist as $type => $img_ary)
		{
			if ($type !== 'lang' || $sql_extra)
			{
				$template->assign_block_vars('imagesetlist', array(
					'TYPE'	=> ($type == 'lang')
				));
			}

			foreach ($img_ary as $img)
			{
				$imgtext = preg_replace('/^([^\/]+\/)/', '', $img);
				$selected = (!empty($imgname) && strpos($image_filename, $imgtext) !== false);
				if ($selected)
				{
					$image_found = true;
					$img_val = htmlspecialchars($img);
				}
				$template->assign_block_vars('imagesetlist.images', array(
					'SELECTED'			=> $selected,
					'TEXT'				=> $imgtext,
					'VALUE'				=> htmlspecialchars($img)
				));
			}
		}

		$imgsize_bool = (!empty($imgname) && $image_width && $image_height) ? true : false;
		$image_request = '../styles/' . $imageset_path . '/imageset/' . ($image_lang ? $imgnamelang . '/' : '') . $image_filename;

		$template->assign_vars(array(
			'S_EDIT_IMAGESET'	=> true,
			'L_TITLE'			=> $user->lang[$this->page_title],
			'L_EXPLAIN'			=> $user->lang[$this->page_title . '_EXPLAIN'],
			'IMAGE_OPTIONS'		=> $img_options,
			'IMAGE_SIZE'		=> $image_width,
			'IMAGE_HEIGHT'		=> $image_height,
			'IMAGE_REQUEST'		=> (empty($image_filename)) ? 'images/no_image.png' : $image_request,
			'U_ACTION'			=> $this->u_action . "&amp;action=edit&amp;id=$imageset_id",
			'U_BACK'			=> $this->u_action,
			'NAME'				=> $imageset_name,
			'A_NAME'			=> addslashes($imageset_name),
			'ERROR'				=> !$valid_name,
			'IMG_SRC'			=> ($image_found) ? '../styles/' . $imageset_path . '/imageset/' . $img_val : 'images/no_image.png',
			'IMAGE_SELECT'		=> $image_found
		));
	}

	/**
	* Remove style/template/theme/imageset
	*/
	function remove($mode, $style_id)
	{
		global $db, $template, $user, $phpbb_root_path, $cache, $config;

		$new_id = request_var('new_id', 0);
		$update = (isset($_POST['update'])) ? true : false;
		$sql_where = '';

		switch ($mode)
		{
			case 'style':
				$sql_from = STYLES_TABLE;
				$sql_select = 'style_name';
				$sql_where = 'AND style_active = 1';
			break;

			case 'template':
				$sql_from = STYLES_TEMPLATE_TABLE;
				$sql_select = 'template_name, template_path, template_storedb';
			break;

			case 'theme':
				$sql_from = STYLES_THEME_TABLE;
				$sql_select = 'theme_name, theme_path, theme_storedb';
			break;

			case 'imageset':
				$sql_from = STYLES_IMAGESET_TABLE;
				$sql_select = 'imageset_name, imageset_path';
			break;
		}

		$l_prefix = strtoupper($mode);

		$sql = "SELECT $sql_select
			FROM $sql_from
			WHERE {$mode}_id = $style_id";
		$result = $db->sql_query($sql);
		$style_row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if (!$style_row)
		{
			trigger_error($user->lang['NO_' . $l_prefix] . adm_back_link($this->u_action), E_USER_WARNING);
		}

		$sql = "SELECT {$mode}_id, {$mode}_name
			FROM $sql_from
			WHERE {$mode}_id <> $style_id
			$sql_where
			ORDER BY {$mode}_name ASC";
		$result = $db->sql_query($sql);

		$s_options = '';

		if ($row = $db->sql_fetchrow($result))
		{
			do
			{
				$s_options .= '<option value="' . $row[$mode . '_id'] . '">' . $row[$mode . '_name'] . '</option>';
			}
			while ($row = $db->sql_fetchrow($result));
		}
		else
		{
			trigger_error($user->lang['ONLY_' . $l_prefix] . adm_back_link($this->u_action), E_USER_WARNING);
		}
		$db->sql_freeresult($result);

		if ($update)
		{
			$sql = "DELETE FROM $sql_from
				WHERE {$mode}_id = $style_id";
			$db->sql_query($sql);

			if ($mode == 'style')
			{
				$sql = 'UPDATE ' . USERS_TABLE . "
					SET user_style = $new_id
					WHERE user_style = $style_id";
				$db->sql_query($sql);

				$sql = 'UPDATE ' . FORUMS_TABLE . "
					SET forum_style = $new_id
					WHERE forum_style = $style_id";
				$db->sql_query($sql);

				if ($style_id == $config['default_style'])
				{
					set_config('default_style', $new_id);
				}
			}
			else
			{
				if ($mode == 'imageset')
				{
					$sql = 'DELETE FROM ' . STYLES_IMAGESET_DATA_TABLE . "
						WHERE imageset_id = $style_id";
					$db->sql_query($sql);
				}
				$sql = 'UPDATE ' . STYLES_TABLE . "
					SET {$mode}_id = $new_id
					WHERE {$mode}_id = $style_id";
				$db->sql_query($sql);
			}

			$cache->destroy('sql', STYLES_TABLE);

			add_log('admin', 'LOG_' . $l_prefix . '_DELETE', $style_row[$mode . '_name']);
			$message = ($mode != 'style') ? $l_prefix . '_DELETED_FS' : $l_prefix . '_DELETED';
			trigger_error($user->lang[$message] . adm_back_link($this->u_action));
		}

		$this->page_title = 'DELETE_' . $l_prefix;

		$template->assign_vars(array(
			'S_DELETE'			=> true,
			'S_REPLACE_OPTIONS'	=> $s_options,

			'L_TITLE'			=> $user->lang[$this->page_title],
			'L_EXPLAIN'			=> $user->lang[$this->page_title . '_EXPLAIN'],
			'L_NAME'			=> $user->lang[$l_prefix . '_NAME'],
			'L_REPLACE'			=> $user->lang['REPLACE_' . $l_prefix],
			'L_REPLACE_EXPLAIN'	=> $user->lang['REPLACE_' . $l_prefix . '_EXPLAIN'],

			'U_ACTION'		=> $this->u_action . "&amp;action=delete&amp;id=$style_id",
			'U_BACK'		=> $this->u_action,

			'NAME'			=> $style_row[$mode . '_name'],
			)
		);
	}

	/**
	* Export style or style elements
	*/
	function export($mode, $style_id)
	{
		global $db, $template, $user, $phpbb_root_path, $cache, $phpEx, $config;

		$update = (isset($_POST['update'])) ? true : false;

		$inc_template = request_var('inc_template', 0);
		$inc_theme = request_var('inc_theme', 0);
		$inc_imageset = request_var('inc_imageset', 0);
		$store = request_var('store', 0);
		$format = request_var('format', '');

		$error = array();
		$methods = array('tar');

		$available_methods = array('tar.gz' => 'zlib', 'tar.bz2' => 'bz2', 'zip' => 'zlib');
		foreach ($available_methods as $type => $module)
		{
			if (!@extension_loaded($module))
			{
				continue;
			}

			$methods[] = $type;
		}

		if (!in_array($format, $methods))
		{
			$format = 'tar';
		}

		switch ($mode)
		{
			case 'style':
				if ($update && ($inc_template + $inc_theme + $inc_imageset) < 1)
				{
					$error[] = $user->lang['STYLE_ERR_MORE_ELEMENTS'];
				}

				$name = 'style_name';

				$sql_select = 's.style_id, s.style_name, s.style_copyright';
				$sql_select .= ($inc_template) ? ', t.*' : ', t.template_name';
				$sql_select .= ($inc_theme) ? ', c.*' : ', c.theme_name';
				$sql_select .= ($inc_imageset) ? ', i.*' : ', i.imageset_name';
				$sql_from = STYLES_TABLE . ' s, ' . STYLES_TEMPLATE_TABLE . ' t, ' . STYLES_THEME_TABLE . ' c, ' . STYLES_IMAGESET_TABLE . ' i';
				$sql_where = "s.style_id = $style_id AND t.template_id = s.template_id AND c.theme_id = s.theme_id AND i.imageset_id = s.imageset_id";

				$l_prefix = 'STYLE';
			break;

			case 'template':
				$name = 'template_name';

				$sql_select = '*';
				$sql_from = STYLES_TEMPLATE_TABLE;
				$sql_where = "template_id = $style_id";

				$l_prefix = 'TEMPLATE';
			break;

			case 'theme':
				$name = 'theme_name';

				$sql_select = '*';
				$sql_from = STYLES_THEME_TABLE;
				$sql_where = "theme_id = $style_id";

				$l_prefix = 'THEME';
			break;

			case 'imageset':
				$name = 'imageset_name';

				$sql_select = '*';
				$sql_from = STYLES_IMAGESET_TABLE;
				$sql_where = "imageset_id = $style_id";

				$l_prefix = 'IMAGESET';
			break;
		}

		if ($update && !sizeof($error))
		{
			$sql = "SELECT $sql_select
				FROM $sql_from
				WHERE $sql_where";
			$result = $db->sql_query($sql);
			$style_row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			if (!$style_row)
			{
				trigger_error($user->lang['NO_' . $l_prefix] . adm_back_link($this->u_action), E_USER_WARNING);
			}

			$var_ary = array('style_id', 'style_name', 'style_copyright', 'template_id', 'template_name', 'template_path', 'template_copyright', 'template_storedb', 'bbcode_bitfield', 'theme_id', 'theme_name', 'theme_path', 'theme_copyright', 'theme_storedb', 'theme_mtime', 'theme_data', 'imageset_id', 'imageset_name', 'imageset_path', 'imageset_copyright');

			foreach ($var_ary as $var)
			{
				if (!isset($style_row[$var]))
				{
					$style_row[$var] = '';
				}
			}

			$files = $data = array();

			if ($mode == 'style')
			{
				$style_cfg = str_replace(array('{MODE}', '{NAME}', '{COPYRIGHT}', '{VERSION}'), array($mode, $style_row['style_name'], $style_row['style_copyright'], $config['version']), $this->style_cfg);

				$style_cfg .= (!$inc_template) ? "\nrequired_template = {$style_row['template_name']}" : '';
				$style_cfg .= (!$inc_theme) ? "\nrequired_theme = {$style_row['theme_name']}" : '';
				$style_cfg .= (!$inc_imageset) ? "\nrequired_imageset = {$style_row['imageset_name']}" : '';

				$data[] = array(
					'src'		=> $style_cfg,
					'prefix'	=> 'style.cfg'
				);

				unset($style_cfg);
			}

			// Export template core code
			if ($mode == 'template' || $inc_template)
			{
				$template_cfg = str_replace(array('{MODE}', '{NAME}', '{COPYRIGHT}', '{VERSION}'), array($mode, $style_row['template_name'], $style_row['template_copyright'], $config['version']), $this->template_cfg);
				$template_cfg .= "\nbbcode_bitfield = {$style_row['bbcode_bitfield']}";

				$data[] = array(
					'src'		=> $template_cfg,
					'prefix'	=> 'template/template.cfg'
				);

				// This is potentially nasty memory-wise ...
				if (!$style_row['template_storedb'])
				{
					$files[] = array(
						'src'		=> "styles/{$style_row['template_path']}/template/",
						'prefix-'	=> "styles/{$style_row['template_path']}/",
						'prefix+'	=> false,
						'exclude'	=> 'template.cfg'
					);
				}
				else
				{
					$sql = 'SELECT template_filename, template_data
						FROM ' . STYLES_TEMPLATE_DATA_TABLE . "
						WHERE template_id = {$style_row['template_id']}";
					$result = $db->sql_query($sql);

					while ($row = $db->sql_fetchrow($result))
					{
						$data[] = array(
							'src' => $row['template_data'],
							'prefix' => 'template/' . $row['template_filename']
						);
					}
					$db->sql_freeresult($result);
				}
				unset($template_cfg);
			}

			// Export theme core code
			if ($mode == 'theme' || $inc_theme)
			{
				$theme_cfg = str_replace(array('{MODE}', '{NAME}', '{COPYRIGHT}', '{VERSION}'), array($mode, $style_row['theme_name'], $style_row['theme_copyright'], $config['version']), $this->theme_cfg);

				// Read old cfg file
				$items = $cache->obtain_cfg_items($style_row);
				$items = $items['theme'];

				if (!isset($items['parse_css_file']))
				{
					$items['parse_css_file'] = 'off';
				}

				$theme_cfg = str_replace(array('{PARSE_CSS_FILE}'), array($items['parse_css_file']), $theme_cfg);

				$files[] = array(
					'src'		=> "styles/{$style_row['theme_path']}/theme/",
					'prefix-'	=> "styles/{$style_row['theme_path']}/",
					'prefix+'	=> false,
					'exclude'	=> ($style_row['theme_storedb']) ? 'stylesheet.css,theme.cfg' : 'theme.cfg'
				);

				$data[] = array(
					'src'		=> $theme_cfg,
					'prefix'	=> 'theme/theme.cfg'
				);

				if ($style_row['theme_storedb'])
				{
					$data[] = array(
						'src'		=> $style_row['theme_data'],
						'prefix'	=> 'theme/stylesheet.css'
					);
				}

				unset($items, $theme_cfg);
			}

			// Export imageset core code
			if ($mode == 'imageset' || $inc_imageset)
			{
				$imageset_cfg = str_replace(array('{MODE}', '{NAME}', '{COPYRIGHT}', '{VERSION}'), array($mode, $style_row['imageset_name'], $style_row['imageset_copyright'], $config['version']), $this->imageset_cfg);

				$imageset_main = array();

				$sql = 'SELECT image_filename, image_name, image_height, image_width
					FROM ' . STYLES_IMAGESET_DATA_TABLE . "
					WHERE imageset_id = $style_id
						AND image_lang = ''";
				$result = $db->sql_query($sql);
				while ($row = $db->sql_fetchrow($result))
				{
					$imageset_main[$row['image_name']] = $row['image_filename'] . ($row['image_height'] ? '*' . $row['image_height']: '') . ($row['image_width'] ? '*' . $row['image_width']: '');
				}
				$db->sql_freeresult($result);

				foreach ($this->imageset_keys as $topic => $key_array)
				{
					foreach ($key_array as $key)
					{
						if (isset($imageset_main[$key]))
						{
							$imageset_cfg .= "\nimg_" . $key . ' = ' . str_replace("styles/{$style_row['imageset_path']}/imageset/", '{PATH}', $imageset_main[$key]);
						}
					}
				}

				$files[] = array(
					'src'		=> "styles/{$style_row['imageset_path']}/imageset/",
					'prefix-'	=> "styles/{$style_row['imageset_path']}/",
					'prefix+'	=> false,
					'exclude'	=> 'imageset.cfg'
				);

				$data[] = array(
					'src'		=> trim($imageset_cfg),
					'prefix'	=> 'imageset/imageset.cfg'
				);

				end($data);

				$imageset_root = "{$phpbb_root_path}styles/{$style_row['imageset_path']}/imageset/";

				if ($dh = @opendir($imageset_root))
				{
					while (($fname = readdir($dh)) !== false)
					{
						if ($fname[0] != '.' && $fname != 'CVS' && is_dir("$imageset_root$fname"))
						{
							$files[key($files)]['exclude'] .= ',' . $fname . '/imageset.cfg';
						}
					}
					closedir($dh);
				}

				$imageset_lang = array();

				$sql = 'SELECT image_filename, image_name, image_height, image_width, image_lang
					FROM ' . STYLES_IMAGESET_DATA_TABLE . "
					WHERE imageset_id = $style_id
						AND image_lang <> ''";
				$result = $db->sql_query($sql);
				while ($row = $db->sql_fetchrow($result))
				{
					$imageset_lang[$row['image_lang']][$row['image_name']] = $row['image_filename'] . ($row['image_height'] ? '*' . $row['image_height']: '') . ($row['image_width'] ? '*' . $row['image_width']: '');
				}
				$db->sql_freeresult($result);

				foreach ($imageset_lang as $lang => $imageset_localized)
				{
					$imageset_cfg = str_replace(array('{MODE}', '{NAME}', '{COPYRIGHT}', '{VERSION}'), array($mode, $style_row['imageset_name'], $style_row['imageset_copyright'], $config['version']), $this->imageset_cfg);

					foreach ($this->imageset_keys as $topic => $key_array)
					{
						foreach ($key_array as $key)
						{
							if (isset($imageset_localized[$key]))
							{
								$imageset_cfg .= "\nimg_" . $key . ' = ' . str_replace("styles/{$style_row['imageset_path']}/imageset/", '{PATH}', $imageset_localized[$key]);
							}
						}
					}

					$data[] = array(
						'src'		=> trim($imageset_cfg),
						'prefix'	=> 'imageset/' . $lang . '/imageset.cfg'
					);
				}

				unset($imageset_cfg);
			}

			switch ($format)
			{
				case 'tar':
					$ext = '.tar';
					$mimetype = 'x-tar';
					$compress = 'compress_tar';
				break;

				case 'zip':
					$ext = '.zip';
					$mimetype = 'zip';
				break;

				case 'tar.gz':
					$ext = '.tar.gz';
					$mimetype = 'x-gzip';
				break;

				case 'tar.bz2':
					$ext = '.tar.bz2';
					$mimetype = 'x-bzip2';
				break;

				default:
					$error[] = $user->lang[$l_prefix . '_ERR_ARCHIVE'];
			}

			if (!sizeof($error))
			{
				include($phpbb_root_path . 'includes/functions_compress.' . $phpEx);

				if ($mode == 'style')
				{
					$path = preg_replace('#[^\w-]+#', '_', $style_row['style_name']);
				}
				else
				{
					$path = $style_row[$mode . '_path'];
				}

				if ($format == 'zip')
				{
					$compress = new compress_zip('w', $phpbb_root_path . "store/$path$ext");
				}
				else
				{
					$compress = new compress_tar('w', $phpbb_root_path . "store/$path$ext", $ext);
				}

				if (sizeof($files))
				{
					foreach ($files as $file_ary)
					{
						$compress->add_file($file_ary['src'], $file_ary['prefix-'], $file_ary['prefix+'], $file_ary['exclude']);
					}
				}

				if (sizeof($data))
				{
					foreach ($data as $data_ary)
					{
						$compress->add_data($data_ary['src'], $data_ary['prefix']);
					}
				}

				$compress->close();

				add_log('admin', 'LOG_' . $l_prefix . '_EXPORT', $style_row[$mode . '_name']);

				if (!$store)
				{
					$compress->download($path);
					@unlink("{$phpbb_root_path}store/$path$ext");
					exit;
				}

				trigger_error(sprintf($user->lang[$l_prefix . '_EXPORTED'], "store/$path$ext") . adm_back_link($this->u_action));
			}
		}

		$sql = "SELECT {$mode}_id, {$mode}_name
			FROM " . (($mode == 'style') ? STYLES_TABLE : $sql_from) . "
			WHERE {$mode}_id = $style_id";
		$result = $db->sql_query($sql);
		$style_row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if (!$style_row)
		{
			trigger_error($user->lang['NO_' . $l_prefix] . adm_back_link($this->u_action), E_USER_WARNING);
		}

		$this->page_title = $l_prefix . '_EXPORT';

		$format_buttons = '';
		foreach ($methods as $method)
		{
			$format_buttons .= '<label><input type="radio"' . ((!$format_buttons) ? ' id="format"' : '') . ' class="radio" value="' . $method . '" name="format"' . (($method == $format) ? ' checked="checked"' : '') . ' /> ' . $method . '</label>';
		}

		$template->assign_vars(array(
			'S_EXPORT'		=> true,
			'S_ERROR_MSG'	=> (sizeof($error)) ? true : false,
			'S_STYLE'		=> ($mode == 'style') ? true : false,

			'L_TITLE'		=> $user->lang[$this->page_title],
			'L_EXPLAIN'		=> $user->lang[$this->page_title . '_EXPLAIN'],
			'L_NAME'		=> $user->lang[$l_prefix . '_NAME'],

			'U_ACTION'		=> $this->u_action . '&amp;action=export&amp;id=' . $style_id,
			'U_BACK'		=> $this->u_action,

			'ERROR_MSG'			=> (sizeof($error)) ? implode('<br />', $error) : '',
			'NAME'				=> $style_row[$mode . '_name'],
			'FORMAT_BUTTONS'	=> $format_buttons)
		);
	}

	/**
	* Display details
	*/
	function details($mode, $style_id)
	{
		global $template, $db, $config, $user, $safe_mode, $cache, $phpbb_root_path;

		$update = (isset($_POST['update'])) ? true : false;
		$l_type = strtoupper($mode);

		$error = array();
		$element_ary = array('template' => STYLES_TEMPLATE_TABLE, 'theme' => STYLES_THEME_TABLE, 'imageset' => STYLES_IMAGESET_TABLE);

		switch ($mode)
		{
			case 'style':
				$sql_from = STYLES_TABLE;
			break;

			case 'template':
				$sql_from = STYLES_TEMPLATE_TABLE;
			break;

			case 'theme':
				$sql_from = STYLES_THEME_TABLE;
			break;

			case 'imageset':
				$sql_from = STYLES_IMAGESET_TABLE;
			break;
		}

		$sql = "SELECT *
			FROM $sql_from
			WHERE {$mode}_id = $style_id";
		$result = $db->sql_query($sql);
		$style_row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if (!$style_row)
		{
			trigger_error($user->lang['NO_' . $l_type] . adm_back_link($this->u_action), E_USER_WARNING);
		}

		$style_row['style_default'] = ($mode == 'style' && $config['default_style'] == $style_id) ? 1 : 0;

		if ($update)
		{
			$name = utf8_normalize_nfc(request_var('name', '', true));
			$copyright = utf8_normalize_nfc(request_var('copyright', '', true));

			$template_id = request_var('template_id', 0);
			$theme_id = request_var('theme_id', 0);
			$imageset_id = request_var('imageset_id', 0);

			$style_active = request_var('style_active', 0);
			$style_default = request_var('style_default', 0);
			$store_db = request_var('store_db', 0);

			if ($mode == 'style' && (!$template_id || !$theme_id || !$imageset_id))
			{
				$error[] = $user->lang['STYLE_ERR_NO_IDS'];
			}

			if ($mode == 'style' && $style_row['style_active'] && !$style_active && $config['default_style'] == $style_id)
			{
				$error[] = $user->lang['DEACTIVATE_DEFAULT'];
			}

			if (!$name)
			{
				$error[] = $user->lang[$l_type . '_ERR_STYLE_NAME'];
			}

			if ($mode === 'theme' || $mode === 'template')
			{
				// a rather elaborate check we have to do here once to avoid trouble later
				$check = "{$phpbb_root_path}styles/" . $style_row["{$mode}_path"] . (($mode === 'theme') ? '/theme/stylesheet.css' : '/template');
				if (($style_row["{$mode}_storedb"] != $store_db) && !$store_db && ($safe_mode || !@is_writable($check)))
				{
					$error[] = $user->lang['EDIT_' . strtoupper($mode) . '_STORED_DB'];
					$store_db = 1;
				}

				// themes which have to be parsed have to go into db
				if ($mode == 'theme')
				{
					$cfg = parse_cfg_file("{$phpbb_root_path}styles/" . $style_row["{$mode}_path"] . "/theme/theme.cfg");

					if (isset($cfg['parse_css_file']) && $cfg['parse_css_file'] && !$store_db)
					{
						$error[] = $user->lang['EDIT_THEME_STORE_PARSED'];
						$store_db = 1;
					}
				}
			}
			
			if (!sizeof($error))
			{
				// Check length settings
				if (utf8_strlen($name) > 30)
				{
					$error[] = $user->lang[$l_type . '_ERR_NAME_LONG'];
				}

				if (utf8_strlen($copyright) > 60)
				{
					$error[] = $user->lang[$l_type . '_ERR_COPY_LONG'];
				}
			}
		}

		if ($update && sizeof($error))
		{
			$style_row = array_merge($style_row, array(
				'template_id'			=> $template_id,
				'theme_id'				=> $theme_id,
				'imageset_id'			=> $imageset_id,
				'style_active'			=> $style_active,
				$mode . '_storedb'		=> $store_db,
				$mode . '_name'			=> $name,
				$mode . '_copyright'	=> $copyright)
			);
		}

		// User has submitted form and no errors have occurred
		if ($update && !sizeof($error))
		{
			$sql_ary = array(
				$mode . '_name'			=> $name,
				$mode . '_copyright'	=> $copyright
			);

			switch ($mode)
			{
				case 'style':

					$sql_ary += array(
						'template_id'		=> (int) $template_id,
						'theme_id'			=> (int) $theme_id,
						'imageset_id'		=> (int) $imageset_id,
						'style_active'		=> (int) $style_active,
					);
				break;

				case 'imageset':
				break;

				case 'theme':

					if ($style_row['theme_storedb'] != $store_db)
					{
						$theme_data = '';

						if (!$style_row['theme_storedb'])
						{
							$theme_data = $this->db_theme_data($style_row);
						}
						else if (!$store_db && !$safe_mode && @is_writable("{$phpbb_root_path}styles/{$style_row['theme_path']}/theme/stylesheet.css"))
						{
							$store_db = 1;
							$theme_data = $style_row['theme_data'];

							if ($fp = @fopen("{$phpbb_root_path}styles/{$style_row['theme_path']}/theme/stylesheet.css", 'wb'))
							{
								$store_db = (@fwrite($fp, str_replace("styles/{$style_row['theme_path']}/theme/", './', $theme_data))) ? 0 : 1;
							}
							fclose($fp);
						}

						$sql_ary += array(
							'theme_mtime'	=> ($store_db) ? filemtime("{$phpbb_root_path}styles/{$style_row['theme_path']}/theme/stylesheet.css") : 0,
							'theme_storedb'	=> $store_db,
							'theme_data'	=> ($store_db) ? $theme_data : '',
						);
					}
				break;

				case 'template':

					if ($style_row['template_storedb'] != $store_db)
					{
						if (!$store_db && !$safe_mode && @is_writable("{$phpbb_root_path}styles/{$style_row['template_path']}/template"))
						{
							$sql = 'SELECT *
								FROM ' . STYLES_TEMPLATE_DATA_TABLE . "
								WHERE template_id = $style_id";
							$result = $db->sql_query($sql);

							while ($row = $db->sql_fetchrow($result))
							{
								if (!($fp = @fopen("{$phpbb_root_path}styles/{$style_row['template_path']}/template/" . $row['template_filename'], 'wb')))
								{
									$store_db = 1;
									$error[] = $user->lang['EDIT_TEMPLATE_STORED_DB'];
									break;
								}

								fwrite($fp, $row['template_data']);
								fclose($fp);
							}
							$db->sql_freeresult($result);

							if (!$store_db)
							{
								$sql = 'DELETE FROM ' . STYLES_TEMPLATE_DATA_TABLE . "
									WHERE template_id = $style_id";
								$db->sql_query($sql);
							}
						}
						else if ($store_db)
						{
							$filelist = filelist("{$phpbb_root_path}styles/{$style_row['template_path']}/template", '', 'html');
							$this->store_templates('insert', $style_id, $style_row['template_path'], $filelist);
						}
						else
						{
							// We no longer store within the db, but are also not able to update the file structure
							// Since the admin want to switch this, we adhere to his decision. But we also need to remove the cache
							$sql = 'DELETE FROM ' . STYLES_TEMPLATE_DATA_TABLE . "
								WHERE template_id = $style_id";
							$db->sql_query($sql);
						}

						$sql_ary += array(
							'template_storedb'	=> $store_db,
						);
					}
				break;
			}

			if (sizeof($sql_ary))
			{
				$sql = "UPDATE $sql_from
					SET " . $db->sql_build_array('UPDATE', $sql_ary) . "
					WHERE {$mode}_id = $style_id";
				$db->sql_query($sql);

				// Making this the default style?
				if ($mode == 'style' && $style_default)
				{
					set_config('default_style', $style_id);
				}
			}

			$cache->destroy('sql', STYLES_TABLE);

			add_log('admin', 'LOG_' . $l_type . '_EDIT_DETAILS', $name);
			if (sizeof($error))
			{
				trigger_error(implode('<br />', $error) . adm_back_link($this->u_action), E_USER_WARNING);
			}
			else
			{
				trigger_error($user->lang[$l_type . '_DETAILS_UPDATED'] . adm_back_link($this->u_action));
			}
		}

		if ($mode == 'style')
		{
			foreach ($element_ary as $element => $table)
			{
				$sql = "SELECT {$element}_id, {$element}_name
					FROM $table
					ORDER BY {$element}_id ASC";
				$result = $db->sql_query($sql);

				${$element . '_options'} = '';
				while ($row = $db->sql_fetchrow($result))
				{
					$selected = ($row[$element . '_id'] == $style_row[$element . '_id']) ? ' selected="selected"' : '';
					${$element . '_options'} .= '<option value="' . $row[$element . '_id'] . '"' . $selected . '>' . $row[$element . '_name'] . '</option>';
				}
				$db->sql_freeresult($result);
			}
		}

		$this->page_title = 'EDIT_DETAILS_' . $l_type;

		$template->assign_vars(array(
			'S_DETAILS'				=> true,
			'S_ERROR_MSG'			=> (sizeof($error)) ? true : false,
			'S_STYLE'				=> ($mode == 'style') ? true : false,
			'S_TEMPLATE'			=> ($mode == 'template') ? true : false,
			'S_THEME'				=> ($mode == 'theme') ? true : false,
			'S_IMAGESET'			=> ($mode == 'imageset') ? true : false,
			'S_STORE_DB'			=> (isset($style_row[$mode . '_storedb'])) ? $style_row[$mode . '_storedb'] : 0,
			'S_STYLE_ACTIVE'		=> (isset($style_row['style_active'])) ? $style_row['style_active'] : 0,
			'S_STYLE_DEFAULT'		=> (isset($style_row['style_default'])) ? $style_row['style_default'] : 0,

			'S_TEMPLATE_OPTIONS'	=> ($mode == 'style') ? $template_options : '',
			'S_THEME_OPTIONS'		=> ($mode == 'style') ? $theme_options : '',
			'S_IMAGESET_OPTIONS'	=> ($mode == 'style') ? $imageset_options : '',

			'U_ACTION'		=> $this->u_action . '&amp;action=details&amp;id=' . $style_id,
			'U_BACK'		=> $this->u_action,

			'L_TITLE'				=> $user->lang[$this->page_title],
			'L_EXPLAIN'				=> $user->lang[$this->page_title . '_EXPLAIN'],
			'L_NAME'				=> $user->lang[$l_type . '_NAME'],
			'L_LOCATION'			=> ($mode == 'template' || $mode == 'theme') ? $user->lang[$l_type . '_LOCATION'] : '',
			'L_LOCATION_EXPLAIN'	=> ($mode == 'template' || $mode == 'theme') ? $user->lang[$l_type . '_LOCATION_EXPLAIN'] : '',

			'ERROR_MSG'		=> (sizeof($error)) ? implode('<br />', $error) : '',
			'NAME'			=> $style_row[$mode . '_name'],
			'COPYRIGHT'		=> $style_row[$mode . '_copyright'],
			)
		);
	}

	/**
	* Load css file contents
	*/
	function load_css_file($path, $filename)
	{
		global $phpbb_root_path;

		$file = "{$phpbb_root_path}styles/$path/theme/$filename";

		if (file_exists($file) && ($content = file_get_contents($file)))
		{
			$content = trim($content);
		}
		else
		{
			$content = '';
		}

		return $content;
	}

	/**
	* Returns a string containing the value that should be used for the theme_data column in the theme database table.
	* Includes contents of files loaded via @import
	*
	* @param array $theme_row is an associative array containing the theme's current database entry
	* @param mixed $stylesheet can either be the new content for the stylesheet or false to load from the standard file
	* @param string $root_path should only be used in case you want to use a different root path than "{$phpbb_root_path}styles/{$theme_row['theme_path']}"
	*
	* @return string Stylesheet data for theme_data column in the theme table
	*/
	function db_theme_data($theme_row, $stylesheet = false, $root_path = '')
	{
		global $phpbb_root_path;

		if (!$root_path)
		{
			$root_path = $phpbb_root_path . 'styles/' . $theme_row['theme_path'];
		}

		if (!$stylesheet)
		{
			$stylesheet = '';
			if (file_exists($root_path . '/theme/stylesheet.css'))
			{
				$stylesheet = file_get_contents($root_path . '/theme/stylesheet.css');
			}
		}

		// Match CSS imports
		$matches = array();
		preg_match_all('/@import url\(["\'](.*)["\']\);/i', $stylesheet, $matches);

		if (sizeof($matches))
		{
			foreach ($matches[0] as $idx => $match)
			{
				$stylesheet = str_replace($match, acp_styles::load_css_file($theme_row['theme_path'], $matches[1][$idx]), $stylesheet);
			}
		}

		// adjust paths
		return str_replace('./', 'styles/' . $theme_row['theme_path'] . '/theme/', $stylesheet);
	}

	/**
	* Store template files into db
	*/
	function store_templates($mode, $style_id, $template_path, $filelist)
	{
		global $phpbb_root_path, $phpEx, $db;

		$template_path = $template_path . '/template/';
		$includes = array();
		foreach ($filelist as $pathfile => $file_ary)
		{
			foreach ($file_ary as $file)
			{
				if (!($fp = @fopen("{$phpbb_root_path}styles/$template_path$pathfile$file", 'r')))
				{
					trigger_error("Could not open {$phpbb_root_path}styles/$template_path$pathfile$file", E_USER_ERROR);
				}
				$template_data = fread($fp, filesize("{$phpbb_root_path}styles/$template_path$pathfile$file"));
				fclose($fp);

				if (preg_match_all('#<!-- INCLUDE (.*?\.html) -->#is', $template_data, $matches))
				{
					foreach ($matches[1] as $match)
					{
						$includes[trim($match)][] = $file;
					}
				}
			}
		}

		foreach ($filelist as $pathfile => $file_ary)
		{
			foreach ($file_ary as $file)
			{
				// Skip index.
				if (strpos($file, 'index.') === 0)
				{
					continue;
				}

				// We could do this using extended inserts ... but that could be one
				// heck of a lot of data ...
				$sql_ary = array(
					'template_id'			=> (int) $style_id,
					'template_filename'		=> "$pathfile$file",
					'template_included'		=> (isset($includes[$file])) ? implode(':', $includes[$file]) . ':' : '',
					'template_mtime'		=> (int) filemtime("{$phpbb_root_path}styles/$template_path$pathfile$file"),
					'template_data'			=> (string) file_get_contents("{$phpbb_root_path}styles/$template_path$pathfile$file"),
				);

				if ($mode == 'insert')
				{
					$sql = 'INSERT INTO ' . STYLES_TEMPLATE_DATA_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary);
				}
				else
				{
					$sql = 'UPDATE ' . STYLES_TEMPLATE_DATA_TABLE . ' SET ' . $db->sql_build_array('UPDATE', $sql_ary) . "
						WHERE template_id = $style_id
							AND template_filename = '" . $db->sql_escape("$pathfile$file") . "'";
				}
				$db->sql_query($sql);
			}
		}
	}

	/**
	* Returns an array containing all template filenames for one template that are currently cached.
	*
	* @param string $template_path contains the name of the template's folder in /styles/
	*
	* @return array of filenames that exist in /styles/$template_path/template/ (without extension!)
	*/
	function template_cache_filelist($template_path)
	{
		global $phpbb_root_path, $phpEx, $user;

		$cache_prefix = 'tpl_' . $template_path;

		if (!($dp = @opendir("{$phpbb_root_path}cache")))
		{
			trigger_error($user->lang['TEMPLATE_ERR_CACHE_READ'] . adm_back_link($this->u_action), E_USER_WARNING);
		}

		$file_ary = array();
		while ($file = readdir($dp))
		{
			if ($file[0] == '.')
			{
				continue;
			}

			if (is_file($phpbb_root_path . 'cache/' . $file) && (strpos($file, $cache_prefix) === 0))
			{
				$file_ary[] = str_replace('.', '/', preg_replace('#^' . preg_quote($cache_prefix, '#') . '_(.*?)\.html\.' . $phpEx . '$#i', '\1', $file));
			}
		}
		closedir($dp);

		return $file_ary;
	}

	/**
	* Destroys cached versions of template files
	*
	* @param array $template_row contains the template's row in the STYLES_TEMPLATE_TABLE database table
	* @param mixed $file_ary is optional and may contain an array of template file names which should be refreshed in the cache.
	*	The file names should be the original template file names and not the cache file names.
	*/
	function clear_template_cache($template_row, $file_ary = false)
	{
		global $phpbb_root_path, $phpEx, $user;

		$cache_prefix = 'tpl_' . $template_row['template_path'];

		if (!$file_ary || !is_array($file_ary))
		{
			$file_ary = $this->template_cache_filelist($template_row['template_path']);
			$log_file_list = $user->lang['ALL_FILES'];
		}
		else
		{
			$log_file_list = implode(', ', $file_ary);
		}

		foreach ($file_ary as $file)
		{
			$file = str_replace('/', '.', $file);

			$file = "{$phpbb_root_path}cache/{$cache_prefix}_$file.html.$phpEx";
			if (file_exists($file) && is_file($file))
			{
				@unlink($file);
			}
		}
		unset($file_ary);

		add_log('admin', 'LOG_TEMPLATE_CACHE_CLEARED', $template_row['template_name'], $log_file_list);
	}

	/**
	* Install Style/Template/Theme/Imageset
	*/
	function install($mode)
	{
		global $phpbb_root_path, $phpEx, $config, $db, $cache, $user, $template;

		$l_type = strtoupper($mode);

		$error = $installcfg = $style_row = array();
		$root_path = $cfg_file = '';
		$element_ary = array('template' => STYLES_TEMPLATE_TABLE, 'theme' => STYLES_THEME_TABLE, 'imageset' => STYLES_IMAGESET_TABLE);

		$install_path = request_var('path', '');
		$update = (isset($_POST['update'])) ? true : false;

		// Installing, obtain cfg file contents
		if ($install_path)
		{
			$root_path = $phpbb_root_path . 'styles/' . $install_path . '/';
			$cfg_file = ($mode == 'style') ? "$root_path$mode.cfg" : "$root_path$mode/$mode.cfg";

			if (!file_exists($cfg_file))
			{
				$error[] = $user->lang[$l_type . '_ERR_NOT_' . $l_type];
			}
			else
			{
				$installcfg = parse_cfg_file($cfg_file);
			}
		}

		// Installing
		if (sizeof($installcfg))
		{
			$name		= $installcfg['name'];
			$copyright	= $installcfg['copyright'];
			$version	= $installcfg['version'];

			$style_row = array(
				$mode . '_id'			=> 0,
				$mode . '_name'			=> '',
				$mode . '_copyright'	=> ''
			);

			switch ($mode)
			{
				case 'style':

					$style_row = array(
						'style_id'			=> 0,
						'style_name'		=> $installcfg['name'],
						'style_copyright'	=> $installcfg['copyright']
					);

					$reqd_template = (isset($installcfg['required_template'])) ? $installcfg['required_template'] : false;
					$reqd_theme = (isset($installcfg['required_theme'])) ? $installcfg['required_theme'] : false;
					$reqd_imageset = (isset($installcfg['required_imageset'])) ? $installcfg['required_imageset'] : false;

					// Check to see if each element is already installed, if it is grab the id
					foreach ($element_ary as $element => $table)
					{
						$style_row = array_merge($style_row, array(
							$element . '_id'			=> 0,
							$element . '_name'			=> '',
							$element . '_copyright'		=> '')
						);

			 			$this->test_installed($element, $error, (${'reqd_' . $element}) ? $phpbb_root_path . 'styles/' . $reqd_template . '/' : $root_path, ${'reqd_' . $element}, $style_row[$element . '_id'], $style_row[$element . '_name'], $style_row[$element . '_copyright']);

						if (!$style_row[$element . '_name'])
						{
							$style_row[$element . '_name'] = $reqd_template;
						}
					}

				break;

				case 'template':
					$this->test_installed('template', $error, $root_path, false, $style_row['template_id'], $style_row['template_name'], $style_row['template_copyright']);
				break;

				case 'theme':
					$this->test_installed('theme', $error, $root_path, false, $style_row['theme_id'], $style_row['theme_name'], $style_row['theme_copyright']);
				break;

				case 'imageset':
					$this->test_installed('imageset', $error, $root_path, false, $style_row['imageset_id'], $style_row['imageset_name'], $style_row['imageset_copyright']);
				break;
			}
		}
		else
		{
			trigger_error($user->lang['NO_' . $l_type] . adm_back_link($this->u_action), E_USER_WARNING);
		}

		$style_row['store_db'] = request_var('store_db', 0);
		$style_row['style_active'] = request_var('style_active', 1);
		$style_row['style_default'] = request_var('style_default', 0);

		// User has submitted form and no errors have occurred
		if ($update && !sizeof($error))
		{
			if ($mode == 'style')
			{
				foreach ($element_ary as $element => $table)
				{
					${$element . '_root_path'} = (${'reqd_' . $element}) ? $phpbb_root_path . 'styles/' . ${'reqd_' . $element} . '/' : false;
					${$element . '_path'} = (${'reqd_' . $element}) ? ${'reqd_' . $element} : false;
				}
				$this->install_style($error, 'install', $root_path, $style_row['style_id'], $style_row['style_name'], $install_path, $style_row['style_copyright'], $style_row['style_active'], $style_row['style_default'], $style_row, $template_root_path, $template_path, $theme_root_path, $theme_path, $imageset_root_path, $imageset_path);
			}
			else
			{
				$style_row['store_db'] = $this->install_element($mode, $error, 'install', $root_path, $style_row[$mode . '_id'], $style_row[$mode . '_name'], $install_path, $style_row[$mode . '_copyright'], $style_row['store_db']);
			}

			if (!sizeof($error))
			{
				$cache->destroy('sql', STYLES_TABLE);

				$message = ($style_row['store_db']) ? '_ADDED_DB' : '_ADDED';
				trigger_error($user->lang[$l_type . $message] . adm_back_link($this->u_action));
			}
		}

		$this->page_title = 'INSTALL_' . $l_type;

		$template->assign_vars(array(
			'S_DETAILS'			=> true,
			'S_INSTALL'			=> true,
			'S_ERROR_MSG'		=> (sizeof($error)) ? true : false,
			'S_STYLE'			=> ($mode == 'style') ? true : false,
			'S_TEMPLATE'		=> ($mode == 'template') ? true : false,
			'S_THEME'			=> ($mode == 'theme') ? true : false,

			'S_STORE_DB'			=> (isset($style_row[$mode . '_storedb'])) ? $style_row[$mode . '_storedb'] : 0,
			'S_STYLE_ACTIVE'		=> (isset($style_row['style_active'])) ? $style_row['style_active'] : 0,
			'S_STYLE_DEFAULT'		=> (isset($style_row['style_default'])) ? $style_row['style_default'] : 0,

			'U_ACTION'			=> $this->u_action . "&amp;action=install&amp;path=" . urlencode($install_path),
			'U_BACK'			=> $this->u_action,

			'L_TITLE'				=> $user->lang[$this->page_title],
			'L_EXPLAIN'				=> $user->lang[$this->page_title . '_EXPLAIN'],
			'L_NAME'				=> $user->lang[$l_type . '_NAME'],
			'L_LOCATION'			=> ($mode == 'template' || $mode == 'theme') ? $user->lang[$l_type . '_LOCATION'] : '',
			'L_LOCATION_EXPLAIN'	=> ($mode == 'template' || $mode == 'theme') ? $user->lang[$l_type . '_LOCATION_EXPLAIN'] : '',

			'ERROR_MSG'			=> (sizeof($error)) ? implode('<br />', $error) : '',
			'NAME'				=> $style_row[$mode . '_name'],
			'COPYRIGHT'			=> $style_row[$mode . '_copyright'],
			'TEMPLATE_NAME'		=> ($mode == 'style') ? $style_row['template_name'] : '',
			'THEME_NAME'		=> ($mode == 'style') ? $style_row['theme_name'] : '',
			'IMAGESET_NAME'		=> ($mode == 'style') ? $style_row['imageset_name'] : '')
		);
	}

	/**
	* Add new style
	*/
	function add($mode)
	{
		global $phpbb_root_path, $phpEx, $config, $db, $cache, $user, $template;

		$l_type = strtoupper($mode);
		$element_ary = array('template' => STYLES_TEMPLATE_TABLE, 'theme' => STYLES_THEME_TABLE, 'imageset' => STYLES_IMAGESET_TABLE);
		$error = array();

		$style_row = array(
			$mode . '_name'			=> utf8_normalize_nfc(request_var('name', '', true)),
			$mode . '_copyright'	=> utf8_normalize_nfc(request_var('copyright', '', true)),
			'template_id'			=> 0,
			'theme_id'				=> 0,
			'imageset_id'			=> 0,
			'store_db'				=> request_var('store_db', 0),
			'style_active'			=> request_var('style_active', 1),
			'style_default'			=> request_var('style_default', 0),
		);

		$basis = request_var('basis', 0);
		$update = (isset($_POST['update'])) ? true : false;

		if ($basis)
		{
			switch ($mode)
			{
				case 'style':
					$sql_select = 'template_id, theme_id, imageset_id';
					$sql_from = STYLES_TABLE;
				break;

				case 'template':
					$sql_select = 'template_id';
					$sql_from = STYLES_TEMPLATE_TABLE;
				break;

				case 'theme':
					$sql_select = 'theme_id';
					$sql_from = STYLES_THEME_TABLE;
				break;

				case 'imageset':
					$sql_select = 'imageset_id';
					$sql_from = STYLES_IMAGESET_TABLE;
				break;
			}

			$sql = "SELECT $sql_select
				FROM $sql_from
				WHERE {$mode}_id = $basis";
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			if (!$row)
			{
				$error[] = $user->lang['NO_' . $l_type];
			}

			if (!sizeof($error))
			{
				$style_row['template_id']	= (isset($row['template_id'])) ? $row['template_id'] : $style_row['template_id'];
				$style_row['theme_id']		= (isset($row['theme_id'])) ? $row['theme_id'] : $style_row['theme_id'];
				$style_row['imageset_id']	= (isset($row['imageset_id'])) ? $row['imageset_id'] : $style_row['imageset_id'];
			}
		}

		if ($update)
		{
			$style_row['template_id'] = request_var('template_id', $style_row['template_id']);
			$style_row['theme_id'] = request_var('theme_id', $style_row['theme_id']);
			$style_row['imageset_id'] = request_var('imageset_id', $style_row['imageset_id']);

			if ($mode == 'style' && (!$style_row['template_id'] || !$style_row['theme_id'] || !$style_row['imageset_id']))
			{
				$error[] = $user->lang['STYLE_ERR_NO_IDS'];
			}
		}

		// User has submitted form and no errors have occurred
		if ($update && !sizeof($error))
		{
			if ($mode == 'style')
			{
				$style_row['style_id'] = 0;

				$this->install_style($error, 'add', '', $style_row['style_id'], $style_row['style_name'], '', $style_row['style_copyright'], $style_row['style_active'], $style_row['style_default'], $style_row);
			}

			if (!sizeof($error))
			{
				$cache->destroy('sql', STYLES_TABLE);

				$message = ($style_row['store_db']) ? '_ADDED_DB' : '_ADDED';
				trigger_error($user->lang[$l_type . $message] . adm_back_link($this->u_action));
			}
		}

		if ($mode == 'style')
		{
			foreach ($element_ary as $element => $table)
			{
				$sql = "SELECT {$element}_id, {$element}_name
					FROM $table
					ORDER BY {$element}_id ASC";
				$result = $db->sql_query($sql);

				${$element . '_options'} = '';
				while ($row = $db->sql_fetchrow($result))
				{
					$selected = ($row[$element . '_id'] == $style_row[$element . '_id']) ? ' selected="selected"' : '';
					${$element . '_options'} .= '<option value="' . $row[$element . '_id'] . '"' . $selected . '>' . $row[$element . '_name'] . '</option>';
				}
				$db->sql_freeresult($result);
			}
		}

		$this->page_title = 'ADD_' . $l_type;

		$template->assign_vars(array(
			'S_DETAILS'			=> true,
			'S_ADD'				=> true,
			'S_ERROR_MSG'		=> (sizeof($error)) ? true : false,
			'S_STYLE'			=> ($mode == 'style') ? true : false,
			'S_TEMPLATE'		=> ($mode == 'template') ? true : false,
			'S_THEME'			=> ($mode == 'theme') ? true : false,
			'S_BASIS'			=> ($basis) ? true : false,

			'S_STORE_DB'			=> (isset($style_row['storedb'])) ? $style_row['storedb'] : 0,
			'S_STYLE_ACTIVE'		=> (isset($style_row['style_active'])) ? $style_row['style_active'] : 0,
			'S_STYLE_DEFAULT'		=> (isset($style_row['style_default'])) ? $style_row['style_default'] : 0,
			'S_TEMPLATE_OPTIONS'	=> ($mode == 'style') ? $template_options : '',
			'S_THEME_OPTIONS'		=> ($mode == 'style') ? $theme_options : '',
			'S_IMAGESET_OPTIONS'	=> ($mode == 'style') ? $imageset_options : '',

			'U_ACTION'			=> $this->u_action . '&amp;action=add&amp;basis=' . $basis,
			'U_BACK'			=> $this->u_action,

			'L_TITLE'				=> $user->lang[$this->page_title],
			'L_EXPLAIN'				=> $user->lang[$this->page_title . '_EXPLAIN'],
			'L_NAME'				=> $user->lang[$l_type . '_NAME'],
			'L_LOCATION'			=> ($mode == 'template' || $mode == 'theme') ? $user->lang[$l_type . '_LOCATION'] : '',
			'L_LOCATION_EXPLAIN'	=> ($mode == 'template' || $mode == 'theme') ? $user->lang[$l_type . '_LOCATION_EXPLAIN'] : '',

			'ERROR_MSG'			=> (sizeof($error)) ? implode('<br />', $error) : '',
			'NAME'				=> $style_row[$mode . '_name'],
			'COPYRIGHT'			=> $style_row[$mode . '_copyright'])
		);

	}

	/**

					$reqd_template = (isset($installcfg['required_template'])) ? $installcfg['required_template'] : false;
					$reqd_theme = (isset($installcfg['required_theme'])) ? $installcfg['required_theme'] : false;
					$reqd_imageset = (isset($installcfg['required_imageset'])) ? $installcfg['required_imageset'] : false;

					// Check to see if each element is already installed, if it is grab the id
					foreach ($element_ary as $element => $table)
					{
						$style_row = array_merge($style_row, array(
							$element . '_id'			=> 0,
							$element . '_name'			=> '',
							$element . '_copyright'		=> '')
						);

			 			$this->test_installed($element, $error, $root_path, ${'reqd_' . $element}, $style_row[$element . '_id'], $style_row[$element . '_name'], $style_row[$element . '_copyright']);
	* Is this element installed? If not, grab its cfg details
	*/
	function test_installed($element, &$error, $root_path, $reqd_name, &$id, &$name, &$copyright)
	{
		global $db, $user;

		switch ($element)
		{
			case 'template':
				$sql_from = STYLES_TEMPLATE_TABLE;
			break;

			case 'theme':
				$sql_from = STYLES_THEME_TABLE;
			break;

			case 'imageset':
				$sql_from = STYLES_IMAGESET_TABLE;
			break;
		}

		$l_element = strtoupper($element);

		$chk_name = ($reqd_name !== false) ? $reqd_name : $name;

		$sql = "SELECT {$element}_id, {$element}_name
			FROM $sql_from
			WHERE {$element}_name = '" . $db->sql_escape($chk_name) . "'";
		$result = $db->sql_query($sql);

		if ($row = $db->sql_fetchrow($result))
		{
			$name = $row[$element . '_name'];
			$id = $row[$element . '_id'];
		}
		else
		{
			if (!($cfg = @file("$root_path$element/$element.cfg")))
			{
				$error[] = sprintf($user->lang['REQUIRES_' . $l_element], $reqd_name);
				return false;
			}

			$cfg = parse_cfg_file("$root_path$element/$element.cfg", $cfg);

			$name = $cfg['name'];
			$copyright = $cfg['copyright'];
			$id = 0;

			unset($cfg);
		}
		$db->sql_freeresult($result);
	}

	/**
	* Install/Add style
	*/
	function install_style(&$error, $action, $root_path, &$id, $name, $path, $copyright, $active, $default, &$style_row, $template_root_path = false, $template_path = false, $theme_root_path = false, $theme_path = false, $imageset_root_path = false, $imageset_path = false)
	{
		global $config, $db, $user;

		$element_ary = array('template', 'theme', 'imageset');

		if (!$name)
		{
			$error[] = $user->lang['STYLE_ERR_STYLE_NAME'];
		}

		// Check length settings
		if (utf8_strlen($name) > 30)
		{
			$error[] = $user->lang['STYLE_ERR_NAME_LONG'];
		}

		if (utf8_strlen($copyright) > 60)
		{
			$error[] = $user->lang['STYLE_ERR_COPY_LONG'];
		}

		// Check if the name already exist
		$sql = 'SELECT style_id
			FROM ' . STYLES_TABLE . "
			WHERE style_name = '" . $db->sql_escape($name) . "'";
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if ($row)
		{
			$error[] = $user->lang['STYLE_ERR_NAME_EXIST'];
		}

		if (sizeof($error))
		{
			return false;
		}

		foreach ($element_ary as $element)
		{
			// Zero id value ... need to install element ... run usual checks
			// and do the install if necessary
			if (!$style_row[$element . '_id'])
			{
				$this->install_element($element, $error, $action, (${$element . '_root_path'}) ? ${$element . '_root_path'} : $root_path, $style_row[$element . '_id'], $style_row[$element . '_name'], (${$element . '_path'}) ? ${$element . '_path'} : $path, $style_row[$element . '_copyright']);
			}
		}

		if (!$style_row['template_id'] || !$style_row['theme_id'] || !$style_row['imageset_id'])
		{
			$error[] = $user->lang['STYLE_ERR_NO_IDS'];
		}

		if (sizeof($error))
		{
			return false;
		}

		$db->sql_transaction('begin');

		$sql_ary = array(
			'style_name'		=> $name,
			'style_copyright'	=> $copyright,
			'style_active'		=> (int) $active,
			'template_id'		=> (int) $style_row['template_id'],
			'theme_id'			=> (int) $style_row['theme_id'],
			'imageset_id'		=> (int) $style_row['imageset_id'],
		);

		$sql = 'INSERT INTO ' . STYLES_TABLE . '
			' . $db->sql_build_array('INSERT', $sql_ary);
		$db->sql_query($sql);

		$id = $db->sql_nextid();

		if ($default)
		{
			$sql = 'UPDATE ' . USERS_TABLE . "
				SET user_style = $id
				WHERE user_style = " . $config['default_style'];
			$db->sql_query($sql);

			set_config('default_style', $id);
		}

		$db->sql_transaction('commit');

		add_log('admin', 'LOG_STYLE_ADD', $name);
	}

	/**
	* Install/add an element, doing various checks as we go
	*/
	function install_element($mode, &$error, $action, $root_path, &$id, $name, $path, $copyright, $store_db = 0)
	{
		global $phpbb_root_path, $db, $user;

		switch ($mode)
		{
			case 'template':
				$sql_from = STYLES_TEMPLATE_TABLE;
			break;

			case 'theme':
				$sql_from = STYLES_THEME_TABLE;
			break;

			case 'imageset':
				$sql_from = STYLES_IMAGESET_TABLE;
			break;
		}

		$l_type = strtoupper($mode);

		if (!$name)
		{
			$error[] = $user->lang[$l_type . '_ERR_STYLE_NAME'];
		}

		// Check length settings
		if (utf8_strlen($name) > 30)
		{
			$error[] = $user->lang[$l_type . '_ERR_NAME_LONG'];
		}

		if (utf8_strlen($copyright) > 60)
		{
			$error[] = $user->lang[$l_type . '_ERR_COPY_LONG'];
		}

		// Check if the name already exist
		$sql = "SELECT {$mode}_id
			FROM $sql_from
			WHERE {$mode}_name = '" . $db->sql_escape($name) . "'";
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if ($row)
		{
			// If it exist, we just use the style on installation
			if ($action == 'install')
			{
				$id = $row[$mode . '_id'];
				return false;
			}

			$error[] = $user->lang[$l_type . '_ERR_NAME_EXIST'];
		}

		if (sizeof($error))
		{
			return false;
		}

		$sql_ary = array(
			$mode . '_name'			=> $name,
			$mode . '_copyright'	=> $copyright,
			$mode . '_path'			=> $path,
		);

		switch ($mode)
		{
			case 'template':
				// We check if the template author defined a different bitfield
				$cfg_data = parse_cfg_file("$root_path$mode/template.cfg");

				if (!empty($cfg_data['template_bitfield']))
				{
					$sql_ary['bbcode_bitfield'] = $cfg_data['template_bitfield'];
				}
				else
				{
					$sql_ary['bbcode_bitfield'] = TEMPLATE_BITFIELD;
				}

				// We set a pre-defined bitfield here which we may use further in 3.2
				$sql_ary += array(
					'template_storedb'	=> $store_db
				);
			break;

			case 'theme':
				// We are only interested in the theme configuration for now
				$theme_cfg = parse_cfg_file("{$phpbb_root_path}styles/$path/theme/theme.cfg");

				if (isset($theme_cfg['parse_css_file']) && $theme_cfg['parse_css_file'])
				{
					$store_db = 1;
				}

				$sql_ary += array(
					'theme_storedb'	=> $store_db,
					'theme_data'	=> ($store_db) ? $this->db_theme_data($sql_ary, false, $root_path) : '',
					'theme_mtime'	=> (int) filemtime("{$phpbb_root_path}styles/$path/theme/stylesheet.css")
				);
			break;

			// all the heavy lifting is done later
			case 'imageset':
			break;
		}

		$db->sql_transaction('begin');

		$sql = "INSERT INTO $sql_from
			" . $db->sql_build_array('INSERT', $sql_ary);
		$db->sql_query($sql);

		$id = $db->sql_nextid();

		if ($mode == 'template' && $store_db)
		{
			$filelist = filelist("{$root_path}template", '', 'html');
			$this->store_templates('insert', $id, $path, $filelist);
		}
		else if ($mode == 'imageset')
		{
			$cfg_data = parse_cfg_file("$root_path$mode/imageset.cfg");

			$imageset_definitions = array();
			foreach ($this->imageset_keys as $topic => $key_array)
			{
				$imageset_definitions = array_merge($imageset_definitions, $key_array);
			}

			foreach ($cfg_data as $key => $value)
			{
				if (strpos($value, '*') !== false)
				{
					if (substr($value, -1, 1) === '*')
					{
						list($image_filename, $image_height) = explode('*', $value);
						$image_width = 0;
					}
					else
					{
						list($image_filename, $image_height, $image_width) = explode('*', $value);
					}
				}
				else
				{
					$image_filename = $value;
					$image_height = $image_width = 0;
				}

				if (strpos($key, 'img_') === 0 && $image_filename)
				{
					$key = substr($key, 4);
					if (in_array($key, $imageset_definitions))
					{
						$sql_ary = array(
							'image_name'		=> $key,
							'image_filename'	=> str_replace('{PATH}', "styles/$path/imageset/", trim($image_filename)),
							'image_height'		=> (int) $image_height,
							'image_width'		=> (int) $image_width,
							'imageset_id'		=> (int) $id,
							'image_lang'		=> '',
						);
						$db->sql_query('INSERT INTO ' . STYLES_IMAGESET_DATA_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary));
					}
				}
			}
			unset($cfg_data);

			$sql = 'SELECT lang_dir
				FROM ' . LANG_TABLE;
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				if (@file_exists("$root_path$mode/{$row['lang_dir']}/imageset.cfg"))
				{
					$cfg_data_imageset_data = parse_cfg_file("$root_path$mode/{$row['lang_dir']}/imageset.cfg");
					foreach ($cfg_data_imageset_data as $image_name => $value)
					{
						if (strpos($value, '*') !== false)
						{
							if (substr($value, -1, 1) === '*')
							{
								list($image_filename, $image_height) = explode('*', $value);
								$image_width = 0;
							}
							else
							{
								list($image_filename, $image_height, $image_width) = explode('*', $value);
							}
						}
						else
						{
							$image_filename = $value;
							$image_height = $image_width = 0;
						}

						if (strpos($image_name, 'img_') === 0 && $image_filename)
						{
							$image_name = substr($image_name, 4);
							if (in_array($image_name, $imageset_definitions))
							{
								$sql_ary = array(
									'image_name'		=> $image_name,
									'image_filename'	=> $image_filename,
									'image_height'		=> (int) $image_height,
									'image_width'		=> (int) $image_width,
									'imageset_id'		=> (int) $id,
									'image_lang'		=> $row['lang_dir'],
								);
								$db->sql_query('INSERT INTO ' . STYLES_IMAGESET_DATA_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary));
							}
						}
					}
					unset($cfg_data_imageset_data);
				}
			}
			$db->sql_freeresult($result);
		}

		$db->sql_transaction('commit');

		$log = ($store_db) ? 'LOG_' . $l_type . '_ADD_DB' : 'LOG_' . $l_type . '_ADD_FS';
		add_log('admin', $log, $name);

		// Return store_db in case it had to be altered
		return $store_db;
	}

}

?>