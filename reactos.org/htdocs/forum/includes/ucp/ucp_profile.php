<?php
/**
*
* @package ucp
* @version $Id: ucp_profile.php 8479 2008-03-29 00:22:48Z naderman $
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
* ucp_profile
* Changing profile settings
*
* @todo what about pertaining user_sig_options?
* @package ucp
*/
class ucp_profile
{
	var $u_action;

	function main($id, $mode)
	{
		global $config, $db, $user, $auth, $template, $phpbb_root_path, $phpEx;

		$user->add_lang('posting');

		$preview	= (!empty($_POST['preview'])) ? true : false;
		$submit		= (!empty($_POST['submit'])) ? true : false;
		$delete		= (!empty($_POST['delete'])) ? true : false;
		$error = $data = array();
		$s_hidden_fields = '';

		switch ($mode)
		{
			case 'reg_details':
				die('You can modify your account settings using the <a href="/roscms/index.php/my/edit">myReactOS Settings</a> page.<br /><br />Please tell an Administrator to disable this page in the phpBB Administration Panel.');

			case 'profile_info':

				include($phpbb_root_path . 'includes/functions_profile_fields.' . $phpEx);

				$cp = new custom_profile();

				$cp_data = $cp_error = array();

				$data = array(
					'icq'			=> request_var('icq', $user->data['user_icq']),
					'aim'			=> request_var('aim', $user->data['user_aim']),
					'msn'			=> request_var('msn', $user->data['user_msnm']),
					'yim'			=> request_var('yim', $user->data['user_yim']),
					'jabber'		=> utf8_normalize_nfc(request_var('jabber', $user->data['user_jabber'], true)),
					'website'		=> request_var('website', $user->data['user_website']),
					'location'		=> utf8_normalize_nfc(request_var('location', $user->data['user_from'], true)),
					'occupation'	=> utf8_normalize_nfc(request_var('occupation', $user->data['user_occ'], true)),
					'interests'		=> utf8_normalize_nfc(request_var('interests', $user->data['user_interests'], true)),
				);

				if ($config['allow_birthdays'])
				{
					$data['bday_day'] = $data['bday_month'] = $data['bday_year'] = 0;

					if ($user->data['user_birthday'])
					{
						list($data['bday_day'], $data['bday_month'], $data['bday_year']) = explode('-', $user->data['user_birthday']);
					}

					$data['bday_day'] = request_var('bday_day', $data['bday_day']);
					$data['bday_month'] = request_var('bday_month', $data['bday_month']);
					$data['bday_year'] = request_var('bday_year', $data['bday_year']);
					$data['user_birthday'] = sprintf('%2d-%2d-%4d', $data['bday_day'], $data['bday_month'], $data['bday_year']);
				}

				add_form_key('ucp_profile_info');

				if ($submit)
				{
					$validate_array = array(
						'icq'			=> array(
							array('string', true, 3, 15),
							array('match', true, '#^[0-9]+$#i')),
						'aim'			=> array('string', true, 3, 255),
						'msn'			=> array('string', true, 5, 255),
						'jabber'		=> array(
							array('string', true, 5, 255),
							array('jabber')),
						'yim'			=> array('string', true, 5, 255),
						'website'		=> array(
							array('string', true, 12, 255),
							array('match', true, '#^http[s]?://(.*?\.)*?[a-z0-9\-]+\.[a-z]{2,4}#i')),
						'location'		=> array('string', true, 2, 255),
						'occupation'	=> array('string', true, 2, 500),
						'interests'		=> array('string', true, 2, 500),
					);

					if ($config['allow_birthdays'])
					{
						$validate_array = array_merge($validate_array, array(
							'bday_day'		=> array('num', true, 1, 31),
							'bday_month'	=> array('num', true, 1, 12),
							'bday_year'		=> array('num', true, 1901, gmdate('Y', time()) + 50),
							'user_birthday' => array('date', true),
						));
					}

					$error = validate_data($data, $validate_array);

					// validate custom profile fields
					$cp->submit_cp_field('profile', $user->get_iso_lang_id(), $cp_data, $cp_error);

					if (sizeof($cp_error))
					{
						$error = array_merge($error, $cp_error);
					}

					if (!check_form_key('ucp_profile_info'))
					{
						$error[] = 'FORM_INVALID';
					}

					if (!sizeof($error))
					{
						$sql_ary = array(
							'user_icq'		=> $data['icq'],
							'user_aim'		=> $data['aim'],
							'user_msnm'		=> $data['msn'],
							'user_yim'		=> $data['yim'],
							'user_jabber'	=> $data['jabber'],
							'user_website'	=> $data['website'],
							'user_from'		=> $data['location'],
							'user_occ'		=> $data['occupation'],
							'user_interests'=> $data['interests'],
						);

						if ($config['allow_birthdays'])
						{
							$sql_ary['user_birthday'] = $data['user_birthday'];
						}

						$sql = 'UPDATE ' . USERS_TABLE . '
							SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
							WHERE user_id = ' . $user->data['user_id'];
						$db->sql_query($sql);

						// Update Custom Fields
						if (sizeof($cp_data))
						{
							$sql = 'UPDATE ' . PROFILE_FIELDS_DATA_TABLE . '
								SET ' . $db->sql_build_array('UPDATE', $cp_data) . '
								WHERE user_id = ' . $user->data['user_id'];
							$db->sql_query($sql);

							if (!$db->sql_affectedrows())
							{
								$cp_data['user_id'] = (int) $user->data['user_id'];

								$db->sql_return_on_error(true);

								$sql = 'INSERT INTO ' . PROFILE_FIELDS_DATA_TABLE . ' ' . $db->sql_build_array('INSERT', $cp_data);
								$db->sql_query($sql);

								$db->sql_return_on_error(false);
							}
						}

						meta_refresh(3, $this->u_action);
						$message = $user->lang['PROFILE_UPDATED'] . '<br /><br />' . sprintf($user->lang['RETURN_UCP'], '<a href="' . $this->u_action . '">', '</a>');
						trigger_error($message);
					}

					// Replace "error" strings with their real, localised form
					$error = preg_replace('#^([A-Z_]+)$#e', "(!empty(\$user->lang['\\1'])) ? \$user->lang['\\1'] : '\\1'", $error);
				}

				if ($config['allow_birthdays'])
				{
					$s_birthday_day_options = '<option value="0"' . ((!$data['bday_day']) ? ' selected="selected"' : '') . '>--</option>';
					for ($i = 1; $i < 32; $i++)
					{
						$selected = ($i == $data['bday_day']) ? ' selected="selected"' : '';
						$s_birthday_day_options .= "<option value=\"$i\"$selected>$i</option>";
					}

					$s_birthday_month_options = '<option value="0"' . ((!$data['bday_month']) ? ' selected="selected"' : '') . '>--</option>';
					for ($i = 1; $i < 13; $i++)
					{
						$selected = ($i == $data['bday_month']) ? ' selected="selected"' : '';
						$s_birthday_month_options .= "<option value=\"$i\"$selected>$i</option>";
					}
					$s_birthday_year_options = '';

					$now = getdate();
					$s_birthday_year_options = '<option value="0"' . ((!$data['bday_year']) ? ' selected="selected"' : '') . '>--</option>';
					for ($i = $now['year'] - 100; $i < $now['year']; $i++)
					{
						$selected = ($i == $data['bday_year']) ? ' selected="selected"' : '';
						$s_birthday_year_options .= "<option value=\"$i\"$selected>$i</option>";
					}
					unset($now);

					$template->assign_vars(array(
						'S_BIRTHDAY_DAY_OPTIONS'	=> $s_birthday_day_options,
						'S_BIRTHDAY_MONTH_OPTIONS'	=> $s_birthday_month_options,
						'S_BIRTHDAY_YEAR_OPTIONS'	=> $s_birthday_year_options,
						'S_BIRTHDAYS_ENABLED'		=> true,
					));
				}

				$template->assign_vars(array(
					'ERROR'		=> (sizeof($error)) ? implode('<br />', $error) : '',

					'ICQ'		=> $data['icq'],
					'YIM'		=> $data['yim'],
					'AIM'		=> $data['aim'],
					'MSN'		=> $data['msn'],
					'JABBER'	=> $data['jabber'],
					'WEBSITE'	=> $data['website'],
					'LOCATION'	=> $data['location'],
					'OCCUPATION'=> $data['occupation'],
					'INTERESTS'	=> $data['interests'],
				));

				// Get additional profile fields and assign them to the template block var 'profile_fields'
				$user->get_profile_fields($user->data['user_id']);

				$cp->generate_profile_fields('profile', $user->get_iso_lang_id());

			break;

			case 'signature':

				if (!$auth->acl_get('u_sig'))
				{
					trigger_error('NO_AUTH_SIGNATURE');
				}
				
				include($phpbb_root_path . 'includes/functions_posting.' . $phpEx);
				include($phpbb_root_path . 'includes/functions_display.' . $phpEx);

				$enable_bbcode	= ($config['allow_sig_bbcode']) ? ((request_var('disable_bbcode', !$user->optionget('bbcode'))) ? false : true) : false;
				$enable_smilies	= ($config['allow_sig_smilies']) ? ((request_var('disable_smilies', !$user->optionget('smilies'))) ? false : true) : false;
				$enable_urls	= ($config['allow_sig_links']) ? ((request_var('disable_magic_url', false)) ? false : true) : false;

				$signature		= utf8_normalize_nfc(request_var('signature', (string) $user->data['user_sig'], true));

				add_form_key('ucp_sig');

				if ($submit || $preview)
				{
					include($phpbb_root_path . 'includes/message_parser.' . $phpEx);

					if (!sizeof($error))
					{
						$message_parser = new parse_message($signature);

						// Allowing Quote BBCode
						$message_parser->parse($enable_bbcode, $enable_urls, $enable_smilies, $config['allow_sig_img'], $config['allow_sig_flash'], true, $config['allow_sig_links'], true, 'sig');

						if (sizeof($message_parser->warn_msg))
						{
							$error[] = implode('<br />', $message_parser->warn_msg);
						}

						if (!check_form_key('ucp_sig'))
						{
							$error[] = 'FORM_INVALID';
						}

						if (!sizeof($error) && $submit)
						{
							$sql_ary = array(
								'user_sig'					=> (string) $message_parser->message,
								'user_sig_bbcode_uid'		=> (string) $message_parser->bbcode_uid,
								'user_sig_bbcode_bitfield'	=> $message_parser->bbcode_bitfield
							);

							$sql = 'UPDATE ' . USERS_TABLE . '
								SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
								WHERE user_id = ' . $user->data['user_id'];
							$db->sql_query($sql);

							$message = $user->lang['PROFILE_UPDATED'] . '<br /><br />' . sprintf($user->lang['RETURN_UCP'], '<a href="' . $this->u_action . '">', '</a>');
							trigger_error($message);
						}
					}

					// Replace "error" strings with their real, localised form
					$error = preg_replace('#^([A-Z_]+)$#e', "(!empty(\$user->lang['\\1'])) ? \$user->lang['\\1'] : '\\1'", $error);
				}

				$signature_preview = '';
				if ($preview)
				{
					// Now parse it for displaying
					$signature_preview = $message_parser->format_display($enable_bbcode, $enable_urls, $enable_smilies, false);
					unset($message_parser);
				}

				decode_message($signature, $user->data['user_sig_bbcode_uid']);

				$template->assign_vars(array(
					'ERROR'				=> (sizeof($error)) ? implode('<br />', $error) : '',
					'SIGNATURE'			=> $signature,
					'SIGNATURE_PREVIEW'	=> $signature_preview,

					'S_BBCODE_CHECKED' 		=> (!$enable_bbcode) ? ' checked="checked"' : '',
					'S_SMILIES_CHECKED' 	=> (!$enable_smilies) ? ' checked="checked"' : '',
					'S_MAGIC_URL_CHECKED' 	=> (!$enable_urls) ? ' checked="checked"' : '',

					'BBCODE_STATUS'			=> ($config['allow_sig_bbcode']) ? sprintf($user->lang['BBCODE_IS_ON'], '<a href="' . append_sid("{$phpbb_root_path}faq.$phpEx", 'mode=bbcode') . '">', '</a>') : sprintf($user->lang['BBCODE_IS_OFF'], '<a href="' . append_sid("{$phpbb_root_path}faq.$phpEx", 'mode=bbcode') . '">', '</a>'),
					'SMILIES_STATUS'		=> ($config['allow_sig_smilies']) ? $user->lang['SMILIES_ARE_ON'] : $user->lang['SMILIES_ARE_OFF'],
					'IMG_STATUS'			=> ($config['allow_sig_img']) ? $user->lang['IMAGES_ARE_ON'] : $user->lang['IMAGES_ARE_OFF'],
					'FLASH_STATUS'			=> ($config['allow_sig_flash']) ? $user->lang['FLASH_IS_ON'] : $user->lang['FLASH_IS_OFF'],
					'URL_STATUS'			=> ($config['allow_sig_links']) ? $user->lang['URL_IS_ON'] : $user->lang['URL_IS_OFF'],

					'L_SIGNATURE_EXPLAIN'	=> sprintf($user->lang['SIGNATURE_EXPLAIN'], $config['max_sig_chars']),

					'S_BBCODE_ALLOWED'		=> $config['allow_sig_bbcode'],
					'S_SMILIES_ALLOWED'		=> $config['allow_sig_smilies'],
					'S_BBCODE_IMG'			=> ($config['allow_sig_img']) ? true : false,
					'S_BBCODE_FLASH'		=> ($config['allow_sig_flash']) ? true : false,
					'S_LINKS_ALLOWED'		=> ($config['allow_sig_links']) ? true : false)
				);

				// Build custom bbcodes array
				display_custom_bbcodes();

			break;

			case 'avatar':

				include($phpbb_root_path . 'includes/functions_display.' . $phpEx);

				$display_gallery = request_var('display_gallery', '0');
				$avatar_select = basename(request_var('avatar_select', ''));
				$category = basename(request_var('category', ''));

				$can_upload = ($config['allow_avatar_upload'] && file_exists($phpbb_root_path . $config['avatar_path']) && @is_writable($phpbb_root_path . $config['avatar_path']) && $auth->acl_get('u_chgavatar') && (@ini_get('file_uploads') || strtolower(@ini_get('file_uploads')) == 'on')) ? true : false;

				add_form_key('ucp_avatar');

				if ($submit)
				{
					if (check_form_key('ucp_avatar'))
					{
						if (avatar_process_user($error))
						{
							meta_refresh(3, $this->u_action);
							$message = $user->lang['PROFILE_UPDATED'] . '<br /><br />' . sprintf($user->lang['RETURN_UCP'], '<a href="' . $this->u_action . '">', '</a>');
							trigger_error($message);
						}
					}
					else
					{
						$error[] = 'FORM_INVALID';
					}
					// Replace "error" strings with their real, localised form
					$error = preg_replace('#^([A-Z_]+)$#e', "(!empty(\$user->lang['\\1'])) ? \$user->lang['\\1'] : '\\1'", $error);
				}

				$template->assign_vars(array(
					'ERROR'			=> (sizeof($error)) ? implode('<br />', $error) : '',
					'AVATAR'		=> get_user_avatar($user->data['user_avatar'], $user->data['user_avatar_type'], $user->data['user_avatar_width'], $user->data['user_avatar_height']),
					'AVATAR_SIZE'	=> $config['avatar_filesize'],
					
					'U_GALLERY'		=> append_sid("{$phpbb_root_path}ucp.$phpEx", 'i=profile&amp;mode=avatar&amp;display_gallery=1'),
					
					'S_FORM_ENCTYPE'	=> ($can_upload) ? ' enctype="multipart/form-data"' : '',

					'L_AVATAR_EXPLAIN'	=> sprintf($user->lang['AVATAR_EXPLAIN'], $config['avatar_max_width'], $config['avatar_max_height'], $config['avatar_filesize'] / 1024),
				));

				if ($display_gallery && $auth->acl_get('u_chgavatar') && $config['allow_avatar_local'])
				{
					avatar_gallery($category, $avatar_select, 4);
				}
				else
				{
					$avatars_enabled = ($can_upload || ($auth->acl_get('u_chgavatar') && ($config['allow_avatar_local'] || $config['allow_avatar_remote']))) ? true : false;
					
					$template->assign_vars(array(
						'AVATAR_WIDTH'	=> request_var('width', $user->data['user_avatar_width']),
						'AVATAR_HEIGHT'	=> request_var('height', $user->data['user_avatar_height']),

						'S_AVATARS_ENABLED'		=> $avatars_enabled,
						'S_UPLOAD_AVATAR_FILE'	=> $can_upload,
						'S_UPLOAD_AVATAR_URL'	=> $can_upload,
						'S_LINK_AVATAR'			=> ($auth->acl_get('u_chgavatar') && $config['allow_avatar_remote']) ? true : false,
						'S_DISPLAY_GALLERY'		=> ($auth->acl_get('u_chgavatar') && $config['allow_avatar_local']) ? true : false)
					);
				}

			break;
		}

		$template->assign_vars(array(
			'L_TITLE'	=> $user->lang['UCP_PROFILE_' . strtoupper($mode)],

			'S_HIDDEN_FIELDS'	=> $s_hidden_fields,
			'S_UCP_ACTION'		=> $this->u_action)
		);

		// Set desired template
		$this->tpl_name = 'ucp_profile_' . $mode;
		$this->page_title = 'UCP_PROFILE_' . strtoupper($mode);
	}
}

?>