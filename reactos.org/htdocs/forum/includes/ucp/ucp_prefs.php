<?php
/**
*
* @package ucp
* @version $Id: ucp_prefs.php 8479 2008-03-29 00:22:48Z naderman $
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
* ucp_prefs
* Changing user preferences
* @package ucp
*/
class ucp_prefs
{
	var $u_action;

	function main($id, $mode)
	{
		global $config, $db, $user, $auth, $template, $phpbb_root_path, $phpEx;

		$submit = (isset($_POST['submit'])) ? true : false;
		$error = $data = array();
		$s_hidden_fields = '';

		switch ($mode)
		{
			case 'personal':
				add_form_key('ucp_prefs_personal');
				$data = array(
					'notifymethod'	=> request_var('notifymethod', $user->data['user_notify_type']),
					'dateformat'	=> request_var('dateformat', $user->data['user_dateformat'], true),
					'lang'			=> basename(request_var('lang', $user->data['user_lang'])),
					'style'			=> request_var('style', (int) $user->data['user_style']),
					'tz'			=> request_var('tz', (float) $user->data['user_timezone']),

					'dst'			=> request_var('dst', (bool) $user->data['user_dst']),
					'viewemail'		=> request_var('viewemail', (bool) $user->data['user_allow_viewemail']),
					'massemail'		=> request_var('massemail', (bool) $user->data['user_allow_massemail']),
					'hideonline'	=> request_var('hideonline', (bool) !$user->data['user_allow_viewonline']),
					'notifypm'		=> request_var('notifypm', (bool) $user->data['user_notify_pm']),
					'popuppm'		=> request_var('popuppm', (bool) $user->optionget('popuppm')),
					'allowpm'		=> request_var('allowpm', (bool) $user->data['user_allow_pm']),
				);

				if ($submit)
				{
					$data['style'] = ($config['override_user_style']) ? $config['default_style'] : $data['style'];

					$error = validate_data($data, array(
						'dateformat'	=> array('string', false, 1, 30),
						'lang'			=> array('match', false, '#^[a-z0-9_\-]{2,}$#i'),
						'tz'			=> array('num', false, -14, 14),
					));

					if (!check_form_key('ucp_prefs_personal'))
					{
						$error[] = 'FORM_INVALID';
					}

					if (!sizeof($error))
					{
						$user->optionset('popuppm', $data['popuppm']);

						$sql_ary = array(
							'user_allow_pm'			=> $data['allowpm'],
							'user_allow_viewemail'	=> $data['viewemail'],
							'user_allow_massemail'	=> $data['massemail'],
							'user_allow_viewonline'	=> ($auth->acl_get('u_hideonline')) ? !$data['hideonline'] : $user->data['user_allow_viewonline'],
							'user_notify_type'		=> $data['notifymethod'],
							'user_notify_pm'		=> $data['notifypm'],
							'user_options'			=> $user->data['user_options'],

							'user_dst'				=> $data['dst'],
							'user_dateformat'		=> $data['dateformat'],
							'user_lang'				=> $data['lang'],
							'user_timezone'			=> $data['tz'],
							'user_style'			=> $data['style'],
						);

						$sql = 'UPDATE ' . USERS_TABLE . '
							SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
							WHERE user_id = ' . $user->data['user_id'];
						$db->sql_query($sql);

						meta_refresh(3, $this->u_action);
						$message = $user->lang['PREFERENCES_UPDATED'] . '<br /><br />' . sprintf($user->lang['RETURN_UCP'], '<a href="' . $this->u_action . '">', '</a>');
						trigger_error($message);
					}

					// Replace "error" strings with their real, localised form
					$error = preg_replace('#^([A-Z_]+)$#e', "(!empty(\$user->lang['\\1'])) ? \$user->lang['\\1'] : '\\1'", $error);
				}

				$dateformat_options = '';

				foreach ($user->lang['dateformats'] as $format => $null)
				{
					$dateformat_options .= '<option value="' . $format . '"' . (($format == $data['dateformat']) ? ' selected="selected"' : '') . '>';
					$dateformat_options .= $user->format_date(time(), $format, false) . ((strpos($format, '|') !== false) ? $user->lang['VARIANT_DATE_SEPARATOR'] . $user->format_date(time(), $format, true) : '');
					$dateformat_options .= '</option>';
				}

				$s_custom = false;

				$dateformat_options .= '<option value="custom"';
				if (!isset($user->lang['dateformats'][$data['dateformat']]))
				{
					$dateformat_options .= ' selected="selected"';
					$s_custom = true;
				}
				$dateformat_options .= '>' . $user->lang['CUSTOM_DATEFORMAT'] . '</option>';

				$template->assign_vars(array(
					'ERROR'				=> (sizeof($error)) ? implode('<br />', $error) : '',

					'S_NOTIFY_EMAIL'	=> ($data['notifymethod'] == NOTIFY_EMAIL) ? true : false,
					'S_NOTIFY_IM'		=> ($data['notifymethod'] == NOTIFY_IM) ? true : false,
					'S_NOTIFY_BOTH'		=> ($data['notifymethod'] == NOTIFY_BOTH) ? true : false,
					'S_VIEW_EMAIL'		=> $data['viewemail'],
					'S_MASS_EMAIL'		=> $data['massemail'],
					'S_ALLOW_PM'		=> $data['allowpm'],
					'S_HIDE_ONLINE'		=> $data['hideonline'],
					'S_NOTIFY_PM'		=> $data['notifypm'],
					'S_POPUP_PM'		=> $data['popuppm'],
					'S_DST'				=> $data['dst'],

					'DATE_FORMAT'			=> $data['dateformat'],
					'A_DATE_FORMAT'			=> addslashes($data['dateformat']),
					'S_DATEFORMAT_OPTIONS'	=> $dateformat_options,
					'S_CUSTOM_DATEFORMAT'	=> $s_custom,
					'DEFAULT_DATEFORMAT'	=> $config['default_dateformat'],
					'A_DEFAULT_DATEFORMAT'	=> addslashes($config['default_dateformat']),

					'S_LANG_OPTIONS'		=> language_select($data['lang']),
					'S_STYLE_OPTIONS'		=> ($config['override_user_style']) ? '' : style_select($data['style']),
					'S_TZ_OPTIONS'			=> tz_select($data['tz'], true),
					'S_CAN_HIDE_ONLINE'		=> ($auth->acl_get('u_hideonline')) ? true : false,
					'S_SELECT_NOTIFY'		=> ($config['jab_enable'] && $user->data['user_jabber'] && @extension_loaded('xml')) ? true : false)
				);

			break;

			case 'view':

				add_form_key('ucp_prefs_view');

				$data = array(
					'topic_sk'		=> request_var('topic_sk', (!empty($user->data['user_topic_sortby_type'])) ? $user->data['user_topic_sortby_type'] : 't'),
					'topic_sd'		=> request_var('topic_sd', (!empty($user->data['user_topic_sortby_dir'])) ? $user->data['user_topic_sortby_dir'] : 'd'),
					'topic_st'		=> request_var('topic_st', (!empty($user->data['user_topic_show_days'])) ? $user->data['user_topic_show_days'] : 0),

					'post_sk'		=> request_var('post_sk', (!empty($user->data['user_post_sortby_type'])) ? $user->data['user_post_sortby_type'] : 't'),
					'post_sd'		=> request_var('post_sd', (!empty($user->data['user_post_sortby_dir'])) ? $user->data['user_post_sortby_dir'] : 'a'),
					'post_st'		=> request_var('post_st', (!empty($user->data['user_post_show_days'])) ? $user->data['user_post_show_days'] : 0),

					'images'		=> request_var('images', (bool) $user->optionget('viewimg')),
					'flash'			=> request_var('flash', (bool) $user->optionget('viewflash')),
					'smilies'		=> request_var('smilies', (bool) $user->optionget('viewsmilies')),
					'sigs'			=> request_var('sigs', (bool) $user->optionget('viewsigs')),
					'avatars'		=> request_var('avatars', (bool) $user->optionget('viewavatars')),
					'wordcensor'	=> request_var('wordcensor', (bool) $user->optionget('viewcensors')),
				);

				if ($submit)
				{
					$error = validate_data($data, array(
						'topic_sk'	=> array('string', false, 1, 1),
						'topic_sd'	=> array('string', false, 1, 1),
						'post_sk'	=> array('string', false, 1, 1),
						'post_sd'	=> array('string', false, 1, 1),
					));

					if (!check_form_key('ucp_prefs_view'))
					{
						$error[] = 'FORM_INVALID';
					}

					if (!sizeof($error))
					{
						$user->optionset('viewimg', $data['images']);
						$user->optionset('viewflash', $data['flash']);
						$user->optionset('viewsmilies', $data['smilies']);
						$user->optionset('viewsigs', $data['sigs']);
						$user->optionset('viewavatars', $data['avatars']);

						if ($auth->acl_get('u_chgcensors'))
						{
							$user->optionset('viewcensors', $data['wordcensor']);
						}

						$sql_ary = array(
							'user_options'				=> $user->data['user_options'],
							'user_topic_sortby_type'	=> $data['topic_sk'],
							'user_post_sortby_type'		=> $data['post_sk'],
							'user_topic_sortby_dir'		=> $data['topic_sd'],
							'user_post_sortby_dir'		=> $data['post_sd'],

							'user_topic_show_days'	=> $data['topic_st'],
							'user_post_show_days'	=> $data['post_st'],
						);

						$sql = 'UPDATE ' . USERS_TABLE . '
							SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
							WHERE user_id = ' . $user->data['user_id'];
						$db->sql_query($sql);

						meta_refresh(3, $this->u_action);
						$message = $user->lang['PREFERENCES_UPDATED'] . '<br /><br />' . sprintf($user->lang['RETURN_UCP'], '<a href="' . $this->u_action . '">', '</a>');
						trigger_error($message);
					}

					// Replace "error" strings with their real, localised form
					$error = preg_replace('#^([A-Z_]+)$#e', "(!empty(\$user->lang['\\1'])) ? \$user->lang['\\1'] : '\\1'", $error);
				}

				$sort_dir_text = array('a' => $user->lang['ASCENDING'], 'd' => $user->lang['DESCENDING']);

				// Topic ordering options
				$limit_topic_days = array(0 => $user->lang['ALL_TOPICS'], 1 => $user->lang['1_DAY'], 7 => $user->lang['7_DAYS'], 14 => $user->lang['2_WEEKS'], 30 => $user->lang['1_MONTH'], 90 => $user->lang['3_MONTHS'], 180 => $user->lang['6_MONTHS'], 365 => $user->lang['1_YEAR']);

				$sort_by_topic_text = array('a' => $user->lang['AUTHOR'], 't' => $user->lang['POST_TIME'], 'r' => $user->lang['REPLIES'], 's' => $user->lang['SUBJECT'], 'v' => $user->lang['VIEWS']);
				$sort_by_topic_sql = array('a' => 't.topic_first_poster_name', 't' => 't.topic_last_post_time', 'r' => 't.topic_replies', 's' => 't.topic_title', 'v' => 't.topic_views');

				// Post ordering options
				$limit_post_days = array(0 => $user->lang['ALL_POSTS'], 1 => $user->lang['1_DAY'], 7 => $user->lang['7_DAYS'], 14 => $user->lang['2_WEEKS'], 30 => $user->lang['1_MONTH'], 90 => $user->lang['3_MONTHS'], 180 => $user->lang['6_MONTHS'], 365 => $user->lang['1_YEAR']);

				$sort_by_post_text = array('a' => $user->lang['AUTHOR'], 't' => $user->lang['POST_TIME'], 's' => $user->lang['SUBJECT']);
				$sort_by_post_sql = array('a' => 'u.username_clean', 't' => 'p.post_id', 's' => 'p.post_subject');

				$_options = array('topic', 'post');
				foreach ($_options as $sort_option)
				{
					${'s_limit_' . $sort_option . '_days'} = '<select name="' . $sort_option . '_st">';
					foreach (${'limit_' . $sort_option . '_days'} as $day => $text)
					{
						$selected = ($data[$sort_option . '_st'] == $day) ? ' selected="selected"' : '';
						${'s_limit_' . $sort_option . '_days'} .= '<option value="' . $day . '"' . $selected . '>' . $text . '</option>';
					}
					${'s_limit_' . $sort_option . '_days'} .= '</select>';

					${'s_sort_' . $sort_option . '_key'} = '<select name="' . $sort_option . '_sk">';
					foreach (${'sort_by_' . $sort_option . '_text'} as $key => $text)
					{
						$selected = ($data[$sort_option . '_sk'] == $key) ? ' selected="selected"' : '';
						${'s_sort_' . $sort_option . '_key'} .= '<option value="' . $key . '"' . $selected . '>' . $text . '</option>';
					}
					${'s_sort_' . $sort_option . '_key'} .= '</select>';

					${'s_sort_' . $sort_option . '_dir'} = '<select name="' . $sort_option . '_sd">';
					foreach ($sort_dir_text as $key => $value)
					{
						$selected = ($data[$sort_option . '_sd'] == $key) ? ' selected="selected"' : '';
						${'s_sort_' . $sort_option . '_dir'} .= '<option value="' . $key . '"' . $selected . '>' . $value . '</option>';
					}
					${'s_sort_' . $sort_option . '_dir'} .= '</select>';
				}

				$template->assign_vars(array(
					'ERROR'				=> (sizeof($error)) ? implode('<br />', $error) : '',

					'S_IMAGES'			=> $data['images'],
					'S_FLASH'			=> $data['flash'],
					'S_SMILIES'			=> $data['smilies'],
					'S_SIGS'			=> $data['sigs'],
					'S_AVATARS'			=> $data['avatars'],
					'S_DISABLE_CENSORS'	=> $data['wordcensor'],

					'S_CHANGE_CENSORS'		=> ($auth->acl_get('u_chgcensors')) ? true : false,

					'S_TOPIC_SORT_DAYS'		=> $s_limit_topic_days,
					'S_TOPIC_SORT_KEY'		=> $s_sort_topic_key,
					'S_TOPIC_SORT_DIR'		=> $s_sort_topic_dir,
					'S_POST_SORT_DAYS'		=> $s_limit_post_days,
					'S_POST_SORT_KEY'		=> $s_sort_post_key,
					'S_POST_SORT_DIR'		=> $s_sort_post_dir)
				);

			break;

			case 'post':

				$data = array(
					'bbcode'	=> request_var('bbcode', $user->optionget('bbcode')),
					'smilies'	=> request_var('smilies', $user->optionget('smilies')),
					'sig'		=> request_var('sig', $user->optionget('attachsig')),
					'notify'	=> request_var('notify', $user->data['user_notify']),
				);
				add_form_key('ucp_prefs_post');

				if ($submit)
				{
					if (check_form_key('ucp_prefs_post'))
					{
						$user->optionset('bbcode', $data['bbcode']);
						$user->optionset('smilies', $data['smilies']);
						$user->optionset('attachsig', $data['sig']);

						$sql_ary = array(
							'user_options'	=> $user->data['user_options'],
							'user_notify'	=> $data['notify'],
						);

						$sql = 'UPDATE ' . USERS_TABLE . '
							SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
							WHERE user_id = ' . $user->data['user_id'];
						$db->sql_query($sql);

						$msg = $user->lang['PREFERENCES_UPDATED'];
					}
					else
					{
						$msg = $user->lang['FORM_INVALID'];
					}
					meta_refresh(3, $this->u_action);
					$message = $msg . '<br /><br />' . sprintf($user->lang['RETURN_UCP'], '<a href="' . $this->u_action . '">', '</a>');
					trigger_error($message);
				}

				$template->assign_vars(array(
					'S_BBCODE'	=> $data['bbcode'],
					'S_SMILIES'	=> $data['smilies'],
					'S_SIG'		=> $data['sig'],
					'S_NOTIFY'	=> $data['notify'])
				);
			break;
		}

		$template->assign_vars(array(
			'L_TITLE'			=> $user->lang['UCP_PREFS_' . strtoupper($mode)],

			'S_HIDDEN_FIELDS'	=> $s_hidden_fields,
			'S_UCP_ACTION'		=> $this->u_action)
		);

		$this->tpl_name = 'ucp_prefs_' . $mode;
		$this->page_title = 'UCP_PREFS_' . strtoupper($mode);
	}
}

?>