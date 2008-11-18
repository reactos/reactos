<?php
/**
*
* @package acp
* @version $Id: acp_icons.php 8479 2008-03-29 00:22:48Z naderman $
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
* @todo [smilies] check regular expressions for special char replacements (stored specialchared in db)
* @package acp
*/
class acp_icons
{
	var $u_action;

	function main($id, $mode)
	{
		global $db, $user, $auth, $template, $cache;
		global $config, $phpbb_root_path, $phpbb_admin_path, $phpEx;

		$user->add_lang('acp/posting');

		// Set up general vars
		$action = request_var('action', '');
		$action = (isset($_POST['add'])) ? 'add' : $action;
		$action = (isset($_POST['edit'])) ? 'edit' : $action;
		$action = (isset($_POST['import'])) ? 'import' : $action;
		$icon_id = request_var('id', 0);

		$mode = ($mode == 'smilies') ? 'smilies' : 'icons';

		$this->tpl_name = 'acp_icons';

		// What are we working on?
		switch ($mode)
		{
			case 'smilies':
				$table = SMILIES_TABLE;
				$lang = 'SMILIES';
				$fields = 'smiley';
				$img_path = $config['smilies_path'];
			break;

			case 'icons':
				$table = ICONS_TABLE;
				$lang = 'ICONS';
				$fields = 'icons';
				$img_path = $config['icons_path'];
			break;
		}

		$this->page_title = 'ACP_' . $lang;

		// Clear some arrays
		$_images = $_paks = array();
		$notice = '';

		// Grab file list of paks and images
		if ($action == 'edit' || $action == 'add' || $action == 'import')
		{
			$imglist = filelist($phpbb_root_path . $img_path, '');

			foreach ($imglist as $path => $img_ary)
			{
				foreach ($img_ary as $img)
				{
					$img_size = getimagesize($phpbb_root_path . $img_path . '/' . $path . $img);

					if (!$img_size[0] || !$img_size[1] || strlen($img) > 255)
					{
						continue;
					}

					$_images[$path . $img]['file'] = $path . $img;
					$_images[$path . $img]['width'] = $img_size[0];
					$_images[$path . $img]['height'] = $img_size[1];
				}
			}
			unset($imglist);

			if ($dir = @opendir($phpbb_root_path . $img_path))
			{
				while (($file = readdir($dir)) !== false)
				{
					if (is_file($phpbb_root_path . $img_path . '/' . $file) && preg_match('#\.pak$#i', $file))
					{
						$_paks[] = $file;
					}
				}
				closedir($dir);
			}
		}

		// What shall we do today? Oops, I believe that's trademarked ...
		switch ($action)
		{
			case 'edit':
				unset($_images);
				$_images = array();

			// no break;

			case 'add':

				$smilies = $default_row = array();
				$smiley_options = $order_list = $add_order_list = '';

				if ($action == 'add' && $mode == 'smilies')
				{
					$sql = 'SELECT *
						FROM ' . SMILIES_TABLE . '
						ORDER BY smiley_order';
					$result = $db->sql_query($sql);

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
							$selected = false;

							if (!$smiley_options)
							{
								$selected = true;
								$default_row = $row;
							}
							$smiley_options .= '<option value="' . $row['smiley_url'] . '"' . (($selected) ? ' selected="selected"' : '') . '>' . $row['smiley_url'] . '</option>';

							$template->assign_block_vars('smile', array(
								'SMILEY_URL'	=> addslashes($row['smiley_url']),
								'CODE'			=> addslashes($row['code']),
								'EMOTION'		=> addslashes($row['emotion']),
								'WIDTH'			=> $row['smiley_width'],
								'HEIGHT'		=> $row['smiley_height'],
								'ORDER'			=> $row['smiley_order'] + 1,
							));
						}
					}
				}
				
				$sql = "SELECT *
					FROM $table
					ORDER BY {$fields}_order " . (($icon_id || $action == 'add') ? 'DESC' : 'ASC');
				$result = $db->sql_query($sql);
				
				$data = array();
				$after = false;
				$display = 0;
				$order_lists = array('', '');
				$add_order_lists = array('', '');
				$display_count = 0;
				
				while ($row = $db->sql_fetchrow($result))
				{
					if ($action == 'add')
					{
						unset($_images[$row[$fields . '_url']]);
					}


					if ($row[$fields . '_id'] == $icon_id)
					{
						$after = true;
						$display = $row['display_on_posting'];
						$data[$row[$fields . '_url']] = $row;
					}
					else
					{
						if ($action == 'edit' && !$icon_id)
						{
							$data[$row[$fields . '_url']] = $row;
						}

						$selected = '';
						if (!empty($after))
						{
							$selected = ' selected="selected"';
							$after = false;
						}
						if ($row['display_on_posting'])
						{
							$display_count++;
						}
						$after_txt = ($mode == 'smilies') ? $row['code'] : $row['icons_url'];
						$order_lists[$row['display_on_posting']] = '<option value="' . ($row[$fields . '_order'] + 1) . '"' . $selected . '>' . sprintf($user->lang['AFTER_' . $lang], ' -&gt; ' . $after_txt) . '</option>' . $order_lists[$row['display_on_posting']];

						if (!empty($default_row))
						{
							$add_order_lists[$row['display_on_posting']] = '<option value="' . ($row[$fields . '_order'] + 1) . '"' . (($row[$fields . '_id'] == $default_row['smiley_id']) ? ' selected="selected"' : '') . '>' . sprintf($user->lang['AFTER_' . $lang], ' -&gt; ' . $after_txt) . '</option>' . $add_order_lists[$row['display_on_posting']];
						}
					}
				}
				$db->sql_freeresult($result);

				$order_list = '<option value="1"' . ((!isset($after)) ? ' selected="selected"' : '') . '>' . $user->lang['FIRST'] . '</option>';
				$add_order_list = '<option value="1">' . $user->lang['FIRST'] . '</option>';

				if ($action == 'add')
				{
					$data = $_images;
				}

				$colspan = (($mode == 'smilies') ? '7' : '5');
				$colspan += ($icon_id) ? 1 : 0;
				$colspan += ($action == 'add') ? 2 : 0;
				
				$template->assign_vars(array(
					'S_EDIT'		=> true,
					'S_SMILIES'		=> ($mode == 'smilies') ? true : false,
					'S_ADD'			=> ($action == 'add') ? true : false,
					
					'S_ORDER_LIST_DISPLAY'		=> $order_list . $order_lists[1],
					'S_ORDER_LIST_UNDISPLAY'	=> $order_list . $order_lists[0],
					'S_ORDER_LIST_DISPLAY_COUNT'	=> $display_count + 1,

					'L_TITLE'		=> $user->lang['ACP_' . $lang],
					'L_EXPLAIN'		=> $user->lang['ACP_' . $lang . '_EXPLAIN'],
					'L_CONFIG'		=> $user->lang[$lang . '_CONFIG'],
					'L_URL'			=> $user->lang[$lang . '_URL'],
					'L_LOCATION'	=> $user->lang[$lang . '_LOCATION'],
					'L_WIDTH'		=> $user->lang[$lang . '_WIDTH'],
					'L_HEIGHT'		=> $user->lang[$lang . '_HEIGHT'],
					'L_ORDER'		=> $user->lang[$lang . '_ORDER'],
					'L_NO_ICONS'	=> $user->lang['NO_' . $lang . '_' . strtoupper($action)],

					'COLSPAN'		=> $colspan,
					'ID'			=> $icon_id,

					'U_BACK'		=> $this->u_action,
					'U_ACTION'		=> $this->u_action . '&amp;action=' . (($action == 'add') ? 'create' : 'modify'),
				));

				foreach ($data as $img => $img_row)
				{
					$template->assign_block_vars('items', array(
						'IMG'		=> $img,
						'A_IMG'		=> addslashes($img),
						'IMG_SRC'	=> $phpbb_root_path . $img_path . '/' . $img,

						'CODE'		=> ($mode == 'smilies' && isset($img_row['code'])) ? $img_row['code'] : '',
						'EMOTION'	=> ($mode == 'smilies' && isset($img_row['emotion'])) ? $img_row['emotion'] : '',

						'S_ID'				=> (isset($img_row[$fields . '_id'])) ? true : false,
						'ID'				=> (isset($img_row[$fields . '_id'])) ? $img_row[$fields . '_id'] : 0,
						'WIDTH'				=> (!empty($img_row[$fields .'_width'])) ? $img_row[$fields .'_width'] : $img_row['width'],
						'HEIGHT'			=> (!empty($img_row[$fields .'_height'])) ? $img_row[$fields .'_height'] : $img_row['height'],
						'POSTING_CHECKED'	=> (!empty($img_row['display_on_posting']) || $action == 'add') ? ' checked="checked"' : '',
					));
				}

				// Ok, another row for adding an addition code for a pre-existing image...
				if ($action == 'add' && $mode == 'smilies' && sizeof($smilies))
				{
					$template->assign_vars(array(
						'S_ADD_CODE'		=> true,

						'S_IMG_OPTIONS'		=> $smiley_options,
						
						'S_ADD_ORDER_LIST_DISPLAY'		=> $add_order_list . $add_order_lists[1],
						'S_ADD_ORDER_LIST_UNDISPLAY'	=> $add_order_list . $add_order_lists[0],
						
						'IMG_SRC'			=> $phpbb_root_path . $img_path . '/' . $default_row['smiley_url'],
						'IMG_PATH'			=> $img_path,
						'PHPBB_ROOT_PATH'	=> $phpbb_root_path,

						'CODE'				=> $default_row['code'],
						'EMOTION'			=> $default_row['emotion'],

						'WIDTH'				=> $default_row['smiley_width'],
						'HEIGHT'			=> $default_row['smiley_height'],
					));
				}

				return;
	
			break;

			case 'create':
			case 'modify':

				// Get items to create/modify
				$images = (isset($_POST['image'])) ? array_keys(request_var('image', array('' => 0))) : array();
				
				// Now really get the items
				$image_id		= (isset($_POST['id'])) ? request_var('id', array('' => 0)) : array();
				$image_order	= (isset($_POST['order'])) ? request_var('order', array('' => 0)) : array();
				$image_width	= (isset($_POST['width'])) ? request_var('width', array('' => 0)) : array();
				$image_height	= (isset($_POST['height'])) ? request_var('height', array('' => 0)) : array();
				$image_add		= (isset($_POST['add_img'])) ? request_var('add_img', array('' => 0)) : array();
				$image_emotion	= utf8_normalize_nfc(request_var('emotion', array('' => ''), true));
				$image_code		= utf8_normalize_nfc(request_var('code', array('' => ''), true));
				$image_display_on_posting = (isset($_POST['display_on_posting'])) ? request_var('display_on_posting', array('' => 0)) : array();

				// Ok, add the relevant bits if we are adding new codes to existing emoticons...
				if (!empty($_POST['add_additional_code']))
				{
					$add_image			= request_var('add_image', '');
					$add_code			= utf8_normalize_nfc(request_var('add_code', '', true));
					$add_emotion		= utf8_normalize_nfc(request_var('add_emotion', '', true));

					if ($add_image && $add_emotion && $add_code)
					{
						$images[] = $add_image;
						$image_add[$add_image] = true;

						$image_code[$add_image] = $add_code;
						$image_emotion[$add_image] = $add_emotion;
						$image_width[$add_image] = request_var('add_width', 0);
						$image_height[$add_image] = request_var('add_height', 0);

						if (!empty($_POST['add_display_on_posting']))
						{
							$image_display_on_posting[$add_image] = 1;
						}

						$image_order[$add_image] = request_var('add_order', 0);
					}
				}

				$icons_updated = 0;
				$errors = array();
				foreach ($images as $image)
				{
					if ($mode == 'smilies' && ($image_emotion[$image] == '' || $image_code[$image] == ''))
					{
						$errors[$image] = 'SMILIE_NO_' . (($image_emotion[$image] == '') ? 'EMOTION' : 'CODE');
					}
					else if ($action == 'create' && !isset($image_add[$image]))
					{
						// skip images where add wasn't checked
					}
					else
					{
						if ($image_width[$image] == 0 || $image_height[$image] == 0)
						{
							$img_size = getimagesize($phpbb_root_path . $img_path . '/' . $image);
							$image_width[$image] = $img_size[0];
							$image_height[$image] = $img_size[1];
						}

						$img_sql = array(
							$fields . '_url'		=> $image,
							$fields . '_width'		=> $image_width[$image],
							$fields . '_height'		=> $image_height[$image],
							'display_on_posting'	=> (isset($image_display_on_posting[$image])) ? 1 : 0,
						);

						if ($mode == 'smilies')
						{
							$img_sql = array_merge($img_sql, array(
								'emotion'	=> $image_emotion[$image],
								'code'		=> $image_code[$image])
							);
						}

						// Image_order holds the 'new' order value
						if (!empty($image_order[$image]))
						{
							$img_sql = array_merge($img_sql, array(
								$fields . '_order'	=>	$image_order[$image])
							);

							// Since we always add 'after' an item, we just need to increase all following + the current by one
							$sql = "UPDATE $table
								SET {$fields}_order = {$fields}_order + 1
								WHERE {$fields}_order >= {$image_order[$image]}";
							$db->sql_query($sql);

							// If we adjust the order, we need to adjust all other orders too - they became inaccurate...
							foreach ($image_order as $_image => $_order)
							{
								if ($_image == $image)
								{
									continue;
								}

								if ($_order >= $image_order[$image])
								{
									$image_order[$_image]++;
								}
							}
						}

						if ($action == 'modify'  && !empty($image_id[$image]))
						{
							$sql = "UPDATE $table
								SET " . $db->sql_build_array('UPDATE', $img_sql) . "
								WHERE {$fields}_id = " . $image_id[$image];
							$db->sql_query($sql);
							$icons_updated++;
						}
						else if ($action !== 'modify')
						{
							$sql = "INSERT INTO $table " . $db->sql_build_array('INSERT', $img_sql);
							$db->sql_query($sql);
							$icons_updated++;
						}
						
 					}
				}
				
				$cache->destroy('_icons');
				$cache->destroy('sql', $table);
				
				$level = E_USER_NOTICE;
				switch ($icons_updated)
				{
					case 0:
						$suc_lang = "{$lang}_NONE";
						$level = E_USER_WARNING;
						break;
						
					case 1:
						$suc_lang = "{$lang}_ONE";
						break;
						
					default:
						$suc_lang = $lang;
				}
				$errormsgs = '<br />';
				foreach ($errors as $img => $error)
				{
					$errormsgs .= '<br />' . sprintf($user->lang[$error], $img);
				}
				if ($action == 'modify')
				{
					trigger_error($user->lang[$suc_lang . '_EDITED'] . $errormsgs . adm_back_link($this->u_action), $level);
				}
				else
				{
					trigger_error($user->lang[$suc_lang . '_ADDED'] . $errormsgs .adm_back_link($this->u_action), $level);
				}

			break;

			case 'import':

				$pak = request_var('pak', '');
				$current = request_var('current', '');

				if ($pak != '')
				{
					$order = 0;

					if (!($pak_ary = @file($phpbb_root_path . $img_path . '/' . $pak)))
					{
						trigger_error($user->lang['PAK_FILE_NOT_READABLE'] . adm_back_link($this->u_action), E_USER_WARNING);
					}

					// Make sure the pak_ary is valid
					foreach ($pak_ary as $pak_entry)
					{
						if (preg_match_all("#'(.*?)', ?#", $pak_entry, $data))
						{
							if ((sizeof($data[1]) != 4 && $mode == 'icons') ||
								((sizeof($data[1]) != 6 || (empty($data[1][4]) || empty($data[1][5]))) && $mode == 'smilies' ))
							{
								trigger_error($user->lang['WRONG_PAK_TYPE'] . adm_back_link($this->u_action), E_USER_WARNING);
							}
						}
						else
						{
							trigger_error($user->lang['WRONG_PAK_TYPE'] . adm_back_link($this->u_action), E_USER_WARNING);
						}
					}


					// The user has already selected a smilies_pak file
					if ($current == 'delete')
					{
						switch ($db->sql_layer)
						{
							case 'sqlite':
							case 'firebird':
								$db->sql_query('DELETE FROM ' . $table);
							break;

							default:
								$db->sql_query('TRUNCATE TABLE ' . $table);
							break;
						}

						switch ($mode)
						{
							case 'smilies':
							break;

							case 'icons':
								// Reset all icon_ids
								$db->sql_query('UPDATE ' . TOPICS_TABLE . ' SET icon_id = 0');
								$db->sql_query('UPDATE ' . POSTS_TABLE . ' SET icon_id = 0');
							break;
						}
					}
					else
					{
						$cur_img = array();

						$field_sql = ($mode == 'smilies') ? 'code' : 'icons_url';

						$sql = "SELECT $field_sql
							FROM $table";
						$result = $db->sql_query($sql);

						while ($row = $db->sql_fetchrow($result))
						{
							++$order;
							$cur_img[$row[$field_sql]] = 1;
						}
						$db->sql_freeresult($result);
					}

					foreach ($pak_ary as $pak_entry)
					{
						$data = array();
						if (preg_match_all("#'(.*?)', ?#", $pak_entry, $data))
						{
							if ((sizeof($data[1]) != 4 && $mode == 'icons') ||
								(sizeof($data[1]) != 6 && $mode == 'smilies'))
							{
								trigger_error($user->lang['WRONG_PAK_TYPE'] . adm_back_link($this->u_action), E_USER_WARNING);
							}

							// Stripslash here because it got addslashed before... (on export)
							$img = stripslashes($data[1][0]);
							$width = stripslashes($data[1][1]);
							$height = stripslashes($data[1][2]);
							$display_on_posting = stripslashes($data[1][3]);

							if (isset($data[1][4]) && isset($data[1][5]))
							{
								$emotion = stripslashes($data[1][4]);
								$code = stripslashes($data[1][5]);
							}

							if ($current == 'replace' &&
								(($mode == 'smilies' && !empty($cur_img[$code])) ||
								($mode == 'icons' && !empty($cur_img[$img]))))
							{
								$replace_sql = ($mode == 'smilies') ? $code : $img;
								$sql = array(
									$fields . '_url'		=> $img,
									$fields . '_height'		=> (int) $height,
									$fields . '_width'		=> (int) $width,
									'display_on_posting'	=> (int) $display_on_posting,
								);

								if ($mode == 'smilies')
								{
									$sql = array_merge($sql, array(
										'emotion'				=> $emotion,
									));
								}

								$sql = "UPDATE $table SET " . $db->sql_build_array('UPDATE', $sql) . "
									WHERE $field_sql = '" . $db->sql_escape($replace_sql) . "'";
								$db->sql_query($sql);
							}
							else
							{
								++$order;

								$sql = array(
									$fields . '_url'	=> $img,
									$fields . '_height'	=> (int) $height,
									$fields . '_width'	=> (int) $width,
									$fields . '_order'	=> (int) $order,
									'display_on_posting'=> (int) $display_on_posting,
								);

								if ($mode == 'smilies')
								{
									$sql = array_merge($sql, array(
										'code'				=> $code,
										'emotion'			=> $emotion,
									));
								}
								$db->sql_query("INSERT INTO $table " . $db->sql_build_array('INSERT', $sql));
							}
						}
					}

					$cache->destroy('_icons');
					$cache->destroy('sql', $table);

					trigger_error($user->lang[$lang . '_IMPORT_SUCCESS'] . adm_back_link($this->u_action));
				}
				else
				{
					$pak_options = '';

					foreach ($_paks as $pak)
					{
						$pak_options .= '<option value="' . $pak . '">' . htmlspecialchars($pak) . '</option>';
					}

					$template->assign_vars(array(
						'S_CHOOSE_PAK'		=> true,
						'S_PAK_OPTIONS'		=> $pak_options,

						'L_TITLE'			=> $user->lang['ACP_' . $lang],
						'L_EXPLAIN'			=> $user->lang['ACP_' . $lang . '_EXPLAIN'],
						'L_NO_PAK_OPTIONS'	=> $user->lang['NO_' . $lang . '_PAK'],
						'L_CURRENT'			=> $user->lang['CURRENT_' . $lang],
						'L_CURRENT_EXPLAIN'	=> $user->lang['CURRENT_' . $lang . '_EXPLAIN'],
						'L_IMPORT_SUBMIT'	=> $user->lang['IMPORT_' . $lang],

						'U_BACK'		=> $this->u_action,
						'U_ACTION'		=> $this->u_action . '&amp;action=import',
						)
					);
				}
			break;

			case 'export':

				$this->page_title = 'EXPORT_' . $lang;
				$this->tpl_name = 'message_body';

				$template->assign_vars(array(
					'MESSAGE_TITLE'		=> $user->lang['EXPORT_' . $lang],
					'MESSAGE_TEXT'		=> sprintf($user->lang['EXPORT_' . $lang . '_EXPLAIN'], '<a href="' . $this->u_action . '&amp;action=send">', '</a>'),

					'S_USER_NOTICE'		=> true,
					)
				);

				return;

			break;

			case 'send':

				$sql = "SELECT *
					FROM $table
					ORDER BY {$fields}_order";
				$result = $db->sql_query($sql);

				$pak = '';
				while ($row = $db->sql_fetchrow($result))
				{
					$pak .= "'" . addslashes($row[$fields . '_url']) . "', ";
					$pak .= "'" . addslashes($row[$fields . '_width']) . "', ";
					$pak .= "'" . addslashes($row[$fields . '_height']) . "', ";
					$pak .= "'" . addslashes($row['display_on_posting']) . "', ";

					if ($mode == 'smilies')
					{
						$pak .= "'" . addslashes($row['emotion']) . "', ";
						$pak .= "'" . addslashes($row['code']) . "', ";
					}

					$pak .= "\n";
				}
				$db->sql_freeresult($result);

				if ($pak != '')
				{
					garbage_collection();

					header('Pragma: public');

					// Send out the Headers
					header('Content-Type: text/x-delimtext; name="' . $mode . '.pak"');
					header('Content-Disposition: inline; filename="' . $mode . '.pak"');
					echo $pak;

					flush();
					exit;
				}
				else
				{
					trigger_error($user->lang['NO_' . strtoupper($fields) . '_EXPORT'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

			break;

			case 'delete':

				if (confirm_box(true))
				{
					$sql = "DELETE FROM $table
						WHERE {$fields}_id = $icon_id";
					$db->sql_query($sql);

					switch ($mode)
					{
						case 'smilies':
						break;

						case 'icons':
							// Reset appropriate icon_ids
							$db->sql_query('UPDATE ' . TOPICS_TABLE . "
								SET icon_id = 0
								WHERE icon_id = $icon_id");

							$db->sql_query('UPDATE ' . POSTS_TABLE . "
								SET icon_id = 0
								WHERE icon_id = $icon_id");
						break;
					}

					$notice = $user->lang[$lang . '_DELETED'];

					$cache->destroy('_icons');
					$cache->destroy('sql', $table);
				}
				else
				{
					confirm_box(false, $user->lang['CONFIRM_OPERATION'], build_hidden_fields(array(
						'i'			=> $id,
						'mode'		=> $mode,
						'id'		=> $icon_id,
						'action'	=> 'delete',
					)));
				}

			break;

			case 'move_up':
			case 'move_down':

				// Get current order id...
				$sql = "SELECT {$fields}_order as current_order
					FROM $table
					WHERE {$fields}_id = $icon_id";
				$result = $db->sql_query($sql);
				$current_order = (int) $db->sql_fetchfield('current_order');
				$db->sql_freeresult($result);

				if ($current_order == 0 && $action == 'move_up')
				{
					break;
				}

				// on move_down, switch position with next order_id...
				// on move_up, switch position with previous order_id...
				$switch_order_id = ($action == 'move_down') ? $current_order + 1 : $current_order - 1;

				//
				$sql = "UPDATE $table
					SET {$fields}_order = $current_order
					WHERE {$fields}_order = $switch_order_id
						AND {$fields}_id <> $icon_id";
				$db->sql_query($sql);

				// Only update the other entry too if the previous entry got updated
				if ($db->sql_affectedrows())
				{
					$sql = "UPDATE $table
						SET {$fields}_order = $switch_order_id
						WHERE {$fields}_order = $current_order
							AND {$fields}_id = $icon_id";
					$db->sql_query($sql);
				}

				$cache->destroy('_icons');
				$cache->destroy('sql', $table);

			break;
		}

		// By default, check that image_order is valid and fix it if necessary
		$sql = "SELECT {$fields}_id AS order_id, {$fields}_order AS fields_order
			FROM $table
			ORDER BY display_on_posting DESC, {$fields}_order";
		$result = $db->sql_query($sql);

		if ($row = $db->sql_fetchrow($result))
		{
			$order = 0;
			do
			{
				++$order;
				if ($row['fields_order'] != $order)
				{
					$db->sql_query("UPDATE $table
						SET {$fields}_order = $order
						WHERE {$fields}_id = " . $row['order_id']);
				}
			}
			while ($row = $db->sql_fetchrow($result));
		}
		$db->sql_freeresult($result);

		$template->assign_vars(array(
			'L_TITLE'			=> $user->lang['ACP_' . $lang],
			'L_EXPLAIN'			=> $user->lang['ACP_' . $lang . '_EXPLAIN'],
			'L_IMPORT'			=> $user->lang['IMPORT_' . $lang],
			'L_EXPORT'			=> $user->lang['EXPORT_' . $lang],
			'L_NOT_DISPLAYED'	=> $user->lang[$lang . '_NOT_DISPLAYED'],
			'L_ICON_ADD'		=> $user->lang['ADD_' . $lang],
			'L_ICON_EDIT'		=> $user->lang['EDIT_' . $lang],

			'NOTICE'			=> $notice,
			'COLSPAN'			=> ($mode == 'smilies') ? 5 : 3,

			'S_SMILIES'			=> ($mode == 'smilies') ? true : false,

			'U_ACTION'			=> $this->u_action,
			'U_IMPORT'			=> $this->u_action . '&amp;action=import',
			'U_EXPORT'			=> $this->u_action . '&amp;action=export',
			)
		);

		$spacer = false;

		$sql = "SELECT *
			FROM $table
			ORDER BY {$fields}_order ASC";
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$alt_text = ($mode == 'smilies') ? $row['code'] : '';

			$template->assign_block_vars('items', array(
				'S_SPACER'		=> (!$spacer && !$row['display_on_posting']) ? true : false,
				'ALT_TEXT'		=> $alt_text,
				'IMG_SRC'		=> $phpbb_root_path . $img_path . '/' . $row[$fields . '_url'],
				'WIDTH'			=> $row[$fields . '_width'],
				'HEIGHT'		=> $row[$fields . '_height'],
				'CODE'			=> (isset($row['code'])) ? $row['code'] : '',
				'EMOTION'		=> (isset($row['emotion'])) ? $row['emotion'] : '',
				'U_EDIT'		=> $this->u_action . '&amp;action=edit&amp;id=' . $row[$fields . '_id'],
				'U_DELETE'		=> $this->u_action . '&amp;action=delete&amp;id=' . $row[$fields . '_id'],
				'U_MOVE_UP'		=> $this->u_action . '&amp;action=move_up&amp;id=' . $row[$fields . '_id'],
				'U_MOVE_DOWN'	=> $this->u_action . '&amp;action=move_down&amp;id=' . $row[$fields . '_id'])
			);

			if (!$spacer && !$row['display_on_posting'])
			{
				$spacer = true;
			}
		}
		$db->sql_freeresult($result);
	}
}

?>