<?php
/**
*
* @package acp
* @version $Id: acp_language.php 8479 2008-03-29 00:22:48Z naderman $
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
class acp_language
{
	var $u_action;
	var $main_files;
	var $language_header = '';
	var $lang_header = '';

	var $language_file = '';
	var $language_directory = '';

	function main($id, $mode)
	{
		global $config, $db, $user, $auth, $template, $cache;
		global $phpbb_root_path, $phpbb_admin_path, $phpEx, $table_prefix;
		global $safe_mode, $file_uploads;

		include_once($phpbb_root_path . 'includes/functions_user.' . $phpEx);

		$this->default_variables();

		// Check and set some common vars

		$action		= (isset($_POST['update_details'])) ? 'update_details' : '';
		$action		= (isset($_POST['download_file'])) ? 'download_file' : $action;
		$action		= (isset($_POST['upload_file'])) ? 'upload_file' : $action;
		$action		= (isset($_POST['upload_data'])) ? 'upload_data' : $action;
		$action		= (isset($_POST['submit_file'])) ? 'submit_file' : $action;
		$action		= (isset($_POST['remove_store'])) ? 'details' : $action;

		$submit = (empty($action) && !isset($_POST['update']) && !isset($_POST['test_connection'])) ? false : true;
		$action = (empty($action)) ? request_var('action', '') : $action;

		$form_name = 'acp_lang';
		add_form_key('acp_lang');

		$lang_id = request_var('id', 0);
		if (isset($_POST['missing_file']))
		{
			$missing_file = request_var('missing_file', array('' => 0));
			list($_REQUEST['language_file'], ) = array_keys($missing_file);
		}

		$selected_lang_file = request_var('language_file', '|common.' . $phpEx);

		list($this->language_directory, $this->language_file) = explode('|', $selected_lang_file);

		$this->language_directory = basename($this->language_directory);
		$this->language_file = basename($this->language_file);

		$user->add_lang('acp/language');
		$this->tpl_name = 'acp_language';
		$this->page_title = 'ACP_LANGUAGE_PACKS';

		if ($submit && $action == 'upload_data' && request_var('test_connection', ''))
		{
			$test_connection = false;
			$action = 'upload_file';
			$method = request_var('method', '');

			include_once($phpbb_root_path . 'includes/functions_transfer.' . $phpEx);

			switch ($method)
			{
				case 'ftp':
					$transfer = new ftp(request_var('host', ''), request_var('username', ''), request_var('password', ''), request_var('root_path', ''), request_var('port', ''), request_var('timeout', ''));
				break;

				case 'ftp_fsock':
					$transfer = new ftp_fsock(request_var('host', ''), request_var('username', ''), request_var('password', ''), request_var('root_path', ''), request_var('port', ''), request_var('timeout', ''));
				break;

				default:
					trigger_error($user->lang['INVALID_UPLOAD_METHOD'], E_USER_ERROR);
				break;
			}

			$test_connection = $transfer->open_session();
			$transfer->close_session();
		}

		switch ($action)
		{
			case 'upload_file':

				include_once($phpbb_root_path . 'includes/functions_transfer.' . $phpEx);

				$method = request_var('method', '');

				if (!class_exists($method))
				{
					trigger_error('Method does not exist.', E_USER_ERROR);
				}

				$requested_data = call_user_func(array($method, 'data'));
				foreach ($requested_data as $data => $default)
				{
					$template->assign_block_vars('data', array(
						'DATA'		=> $data,
						'NAME'		=> $user->lang[strtoupper($method . '_' . $data)],
						'EXPLAIN'	=> $user->lang[strtoupper($method . '_' . $data) . '_EXPLAIN'],
						'DEFAULT'	=> (!empty($_REQUEST[$data])) ? request_var($data, '') : $default
					));
				}

				$hidden_data = build_hidden_fields(array(
					'file'			=> $this->language_file,
					'dir'			=> $this->language_directory,
					'language_file'	=> $selected_lang_file,
					'method'		=> $method)
				);

				$hidden_data .= build_hidden_fields(array('entry' => $_POST['entry']), true, STRIP);

				$template->assign_vars(array(
					'S_UPLOAD'	=> true,
					'NAME'		=> $method,
					'U_ACTION'	=> $this->u_action . "&amp;id=$lang_id&amp;action=upload_data",
					'U_BACK'	=> $this->u_action . "&amp;id=$lang_id&amp;action=details&amp;language_file=" . urlencode($selected_lang_file),
					'HIDDEN'	=> $hidden_data,

					'S_CONNECTION_SUCCESS'		=> (request_var('test_connection', '') && $test_connection === true) ? true : false,
					'S_CONNECTION_FAILED'		=> (request_var('test_connection', '') && $test_connection !== true) ? true : false
				));
			break;

			case 'update_details':

				if (!$submit || !check_form_key($form_name))
				{
					trigger_error($user->lang['FORM_INVALID']. adm_back_link($this->u_action), E_USER_WARNING);
				}

				if (!$lang_id)
				{
					trigger_error($user->lang['NO_LANG_ID'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				$sql = 'SELECT *
					FROM ' . LANG_TABLE . "
					WHERE lang_id = $lang_id";
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				$sql_ary	= array(
					'lang_english_name'		=> request_var('lang_english_name', $row['lang_english_name']),
					'lang_local_name'		=> utf8_normalize_nfc(request_var('lang_local_name', $row['lang_local_name'], true)),
					'lang_author'			=> utf8_normalize_nfc(request_var('lang_author', $row['lang_author'], true)),
				);

				$db->sql_query('UPDATE ' . LANG_TABLE . '
					SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
					WHERE lang_id = ' . $lang_id);

				add_log('admin', 'LOG_LANGUAGE_PACK_UPDATED', $sql_ary['lang_english_name']);

				trigger_error($user->lang['LANGUAGE_DETAILS_UPDATED'] . adm_back_link($this->u_action));
			break;

			case 'submit_file':
			case 'download_file':
			case 'upload_data':
				
				if (!$submit || !check_form_key($form_name))
				{
					trigger_error($user->lang['FORM_INVALID']. adm_back_link($this->u_action), E_USER_WARNING);
				}

				if (!$lang_id || empty($_POST['entry']))
				{
					trigger_error($user->lang['NO_LANG_ID'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				if ($this->language_directory != 'email' && !is_array($_POST['entry']))
				{
					trigger_error($user->lang['NO_LANG_ID'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				if (!$this->language_file || (!$this->language_directory && !in_array($this->language_file, $this->main_files)))
				{
					trigger_error($user->lang['NO_FILE_SELECTED'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				$sql = 'SELECT *
					FROM ' . LANG_TABLE . "
					WHERE lang_id = $lang_id";
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if (!$row)
				{
					trigger_error($user->lang['NO_LANG_ID'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				// Before we attempt to write anything let's check if the admin really chose a correct filename
				switch ($this->language_directory)
				{
					case 'email':
						// Get email templates
						$email_files = filelist($phpbb_root_path . 'language/' . $row['lang_iso'], 'email', 'txt');
						$email_files = $email_files['email/'];

						if (!in_array($this->language_file, $email_files))
						{
							trigger_error($user->lang['WRONG_LANGUAGE_FILE'] . adm_back_link($this->u_action . '&amp;action=details&amp;id=' . $lang_id), E_USER_WARNING);
						}
					break;

					case 'acp':
						// Get acp files
						$acp_files = filelist($phpbb_root_path . 'language/' . $row['lang_iso'], 'acp', $phpEx);
						$acp_files = $acp_files['acp/'];

						if (!in_array($this->language_file, $acp_files))
						{
							trigger_error($user->lang['WRONG_LANGUAGE_FILE'] . adm_back_link($this->u_action . '&amp;action=details&amp;id=' . $lang_id), E_USER_WARNING);
						}
					break;

					case 'mods':
						// Get mod files
						$mods_files = filelist($phpbb_root_path . 'language/' . $row['lang_iso'], 'mods', $phpEx);
						$mods_files = (isset($mods_files['mods/'])) ? $mods_files['mods/'] : array();

						if (!in_array($this->language_file, $mods_files))
						{
							trigger_error($user->lang['WRONG_LANGUAGE_FILE'] . adm_back_link($this->u_action . '&amp;action=details&amp;id=' . $lang_id), E_USER_WARNING);
						}
					break;

					default:
						if (!in_array($this->language_file, $this->main_files))
						{
							trigger_error($user->lang['WRONG_LANGUAGE_FILE'] . adm_back_link($this->u_action . '&amp;action=details&amp;id=' . $lang_id), E_USER_WARNING);
						}
					break;
				}

				if (!$safe_mode)
				{
					$mkdir_ary = array('language', 'language/' . $row['lang_iso']);
					
					if ($this->language_directory)
					{
						$mkdir_ary[] = 'language/' . $row['lang_iso'] . '/' . $this->language_directory;
					}
				
					foreach ($mkdir_ary as $dir)
					{
						$dir = $phpbb_root_path . 'store/' . $dir;
			
						if (!is_dir($dir))
						{
							if (!@mkdir($dir, 0777))
							{
								trigger_error("Could not create directory $dir", E_USER_ERROR);
							}
							@chmod($dir, 0777);
						}
					}
				}

				// Get target filename for storage folder
				$filename = $this->get_filename($row['lang_iso'], $this->language_directory, $this->language_file, true, true);
				$fp = @fopen($phpbb_root_path . $filename, 'wb');

				if (!$fp)
				{
					trigger_error(sprintf($user->lang['UNABLE_TO_WRITE_FILE'], $filename) . adm_back_link($this->u_action . '&amp;id=' . $lang_id . '&amp;action=details&amp;language_file=' . urlencode($selected_lang_file)), E_USER_WARNING);
				}

				if ($this->language_directory == 'email')
				{
					// Email Template
					$entry = $this->prepare_lang_entry($_POST['entry'], false);
					fwrite($fp, $entry);
				}
				else
				{
					$name = (($this->language_directory) ? $this->language_directory . '_' : '') . $this->language_file;
					$header = str_replace(array('{FILENAME}', '{LANG_NAME}', '{CHANGED}', '{AUTHOR}'), array($name, $row['lang_english_name'], date('Y-m-d', time()), $row['lang_author']), $this->language_file_header);

					if (strpos($this->language_file, 'help_') === 0)
					{
						// Help File
						$header .= '$help = array(' . "\n";
						fwrite($fp, $header);

						foreach ($_POST['entry'] as $key => $value)
						{
							if (!is_array($value))
							{
								continue;
							}

							$entry = "\tarray(\n";
							
							foreach ($value as $_key => $_value)
							{
								$entry .= "\t\t" . (int) $_key . "\t=> '" . $this->prepare_lang_entry($_value) . "',\n";
							}

							$entry .= "\t),\n";
							fwrite($fp, $entry);
						}

						$footer = ");\n\n?>";
						fwrite($fp, $footer);
					}
					else
					{
						// Language File
						$header .= $this->lang_header;
						fwrite($fp, $header);

						foreach ($_POST['entry'] as $key => $value)
						{
							$entry = $this->format_lang_array($key, $value);
							fwrite($fp, $entry);
						}

						$footer = "));\n\n?>";
						fwrite($fp, $footer);
					}
				}

				fclose($fp);

				if ($action == 'download_file')
				{
					header('Pragma: no-cache');
					header('Content-Type: application/octetstream; name="' . $this->language_file . '"');
					header('Content-disposition: attachment; filename=' . $this->language_file);

					$fp = @fopen($phpbb_root_path . $filename, 'rb');
					while ($buffer = fread($fp, 1024))
					{
						echo $buffer;
					}
					fclose($fp);

					add_log('admin', 'LOG_LANGUAGE_FILE_SUBMITTED', $this->language_file);

					exit;
				}
				else if ($action == 'upload_data')
				{
					$sql = 'SELECT lang_iso
						FROM ' . LANG_TABLE . "
						WHERE lang_id = $lang_id";
					$result = $db->sql_query($sql);
					$row = $db->sql_fetchrow($result);
					$db->sql_freeresult($result);

					$file = request_var('file', '');
					$dir = request_var('dir', '');

					$selected_lang_file = $dir . '|' . $file;

					$old_file = '/' . $this->get_filename($row['lang_iso'], $dir, $file, false, true);
					$lang_path = 'language/' . $row['lang_iso'] . '/' . (($dir) ? $dir . '/' : '');

					include_once($phpbb_root_path . 'includes/functions_transfer.' . $phpEx);
					$method = request_var('method', '');

					if ($method != 'ftp' && $method != 'ftp_fsock')
					{
						trigger_error($user->lang['INVALID_UPLOAD_METHOD'], E_USER_ERROR);
					}

					$transfer = new $method(request_var('host', ''), request_var('username', ''), request_var('password', ''), request_var('root_path', ''), request_var('port', ''), request_var('timeout', ''));

					if (($result = $transfer->open_session()) !== true)
					{
						trigger_error($user->lang[$result] . adm_back_link($this->u_action . '&amp;action=details&amp;id=' . $lang_id . '&amp;language_file=' . urlencode($selected_lang_file)), E_USER_WARNING);
					}

					$transfer->rename($lang_path . $file, $lang_path . $file . '.bak');
					$result = $transfer->copy_file('store/' . $lang_path . $file, $lang_path . $file);

					if ($result === false)
					{
						// If failed, try to rename again and print error out...
						$transfer->delete_file($lang_path . $file);
						$transfer->rename($lang_path . $file . '.bak', $lang_path . $file);

						trigger_error($user->lang['UPLOAD_FAILED'] . adm_back_link($this->u_action . '&amp;action=details&amp;id=' . $lang_id . '&amp;language_file=' . urlencode($selected_lang_file)), E_USER_WARNING);
					}

					$transfer->close_session();

					// Remove from storage folder
					if (file_exists($phpbb_root_path . 'store/' . $lang_path . $file))
					{
						@unlink($phpbb_root_path . 'store/' . $lang_path . $file);
					}

					add_log('admin', 'LOG_LANGUAGE_FILE_REPLACED', $file);

					trigger_error($user->lang['UPLOAD_COMPLETED'] . adm_back_link($this->u_action . '&amp;action=details&amp;id=' . $lang_id . '&amp;language_file=' . urlencode($selected_lang_file)));
				}

				add_log('admin', 'LOG_LANGUAGE_FILE_SUBMITTED', $this->language_file);
				$action = 'details';

			// no break;

			case 'details':

				if (!$lang_id)
				{
					trigger_error($user->lang['NO_LANG_ID'] . adm_back_link($this->u_action), E_USER_WARNING);
				}
				
				$this->page_title = 'LANGUAGE_PACK_DETAILS';

				$sql = 'SELECT *
					FROM ' . LANG_TABLE . '
					WHERE lang_id = ' . $lang_id;
				$result = $db->sql_query($sql);
				$lang_entries = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);
				
				$lang_iso = $lang_entries['lang_iso'];
				$missing_vars = $missing_files = array();

				// Get email templates
				$email_files = filelist($phpbb_root_path . 'language/' . $config['default_lang'], 'email', 'txt');
				$email_files = $email_files['email/'];

				// Get acp files
				$acp_files = filelist($phpbb_root_path . 'language/' . $config['default_lang'], 'acp', $phpEx);
				$acp_files = $acp_files['acp/'];

				// Get mod files
				$mods_files = filelist($phpbb_root_path . 'language/' . $config['default_lang'], 'mods', $phpEx);
				$mods_files = (isset($mods_files['mods/'])) ? $mods_files['mods/'] : array();

				// Check if our current filename matches the files
				switch ($this->language_directory)
				{
					case 'email':
						if (!in_array($this->language_file, $email_files))
						{
							trigger_error($user->lang['WRONG_LANGUAGE_FILE'] . adm_back_link($this->u_action . '&amp;action=details&amp;id=' . $lang_id), E_USER_WARNING);
						}
					break;

					case 'acp':
						if (!in_array($this->language_file, $acp_files))
						{
							trigger_error($user->lang['WRONG_LANGUAGE_FILE'] . adm_back_link($this->u_action . '&amp;action=details&amp;id=' . $lang_id), E_USER_WARNING);
						}
					break;

					case 'mods':
						if (!in_array($this->language_file, $mods_files))
						{
							trigger_error($user->lang['WRONG_LANGUAGE_FILE'] . adm_back_link($this->u_action . '&amp;action=details&amp;id=' . $lang_id), E_USER_WARNING);
						}
					break;

					default:
						if (!in_array($this->language_file, $this->main_files))
						{
							trigger_error($user->lang['WRONG_LANGUAGE_FILE'] . adm_back_link($this->u_action . '&amp;action=details&amp;id=' . $lang_id), E_USER_WARNING);
						}
				}
				
				if (isset($_POST['remove_store']))
				{
					$store_filename = $this->get_filename($lang_iso, $this->language_directory, $this->language_file, true, true);

					if (file_exists($phpbb_root_path . $store_filename))
					{
						@unlink($phpbb_root_path . $store_filename);
					}
				}

				include_once($phpbb_root_path . 'includes/functions_transfer.' . $phpEx);

				$methods = transfer::methods();

				foreach ($methods as $method)
				{
					$template->assign_block_vars('buttons', array(
						'VALUE' => $method
					));
				}

				$template->assign_vars(array(
					'S_DETAILS'			=> true,
					'U_ACTION'			=> $this->u_action . "&amp;action=details&amp;id=$lang_id",
					'U_BACK'			=> $this->u_action,
					'LANG_LOCAL_NAME'	=> $lang_entries['lang_local_name'],
					'LANG_ENGLISH_NAME'	=> $lang_entries['lang_english_name'],
					'LANG_ISO'			=> $lang_entries['lang_iso'],
					'LANG_AUTHOR'		=> $lang_entries['lang_author'],
					'ALLOW_UPLOAD'		=> sizeof($methods)
					)
				);

				// If current lang is different from the default lang, then first try to grab missing/additional vars
				if ($lang_iso != $config['default_lang'])
				{
					$is_missing_var = false;

					foreach ($this->main_files as $file)
					{
						if (file_exists($phpbb_root_path . $this->get_filename($lang_iso, '', $file)))
						{
							$missing_vars[$file] = $this->compare_language_files($config['default_lang'], $lang_iso, '', $file);
							
							if (sizeof($missing_vars[$file]))
							{
								$is_missing_var = true;
							}
						}
						else
						{
							$missing_files[] = $this->get_filename($lang_iso, '', $file);
						}
					}

					// Now go through acp/mods directories
					foreach ($acp_files as $file)
					{
						if (file_exists($phpbb_root_path . $this->get_filename($lang_iso, 'acp', $file)))
						{
							$missing_vars['acp/' . $file] = $this->compare_language_files($config['default_lang'], $lang_iso, 'acp', $file);
							
							if (sizeof($missing_vars['acp/' . $file]))
							{
								$is_missing_var = true;
							}
						}
						else
						{
							$missing_files[] = $this->get_filename($lang_iso, 'acp', $file);
						}
					}

					if (sizeof($mods_files))
					{
						foreach ($mods_files as $file)
						{
							if (file_exists($phpbb_root_path . $this->get_filename($lang_iso, 'mods', $file)))
							{
								$missing_vars['mods/' . $file] = $this->compare_language_files($config['default_lang'], $lang_iso, 'mods', $file);
								
								if (sizeof($missing_vars['mods/' . $file]))
								{
									$is_missing_var = true;
								}
							}
							else
							{
								$missing_files[] = $this->get_filename($lang_iso, 'mods', $file);
							}
						}
					}
				
					// More missing files... for example email templates?
					foreach ($email_files as $file)
					{
						if (!file_exists($phpbb_root_path . $this->get_filename($lang_iso, 'email', $file)))
						{
							$missing_files[] = $this->get_filename($lang_iso, 'email', $file);
						}
					}

					if (sizeof($missing_files))
					{
						$template->assign_vars(array(
							'S_MISSING_FILES'		=> true,
							'L_MISSING_FILES'		=> sprintf($user->lang['THOSE_MISSING_LANG_FILES'], $lang_entries['lang_local_name']),
							'MISSING_FILES'			=> implode('<br />', $missing_files))
						);
					}

					if ($is_missing_var)
					{
						$template->assign_vars(array(
							'S_MISSING_VARS'			=> true,
							'L_MISSING_VARS_EXPLAIN'	=> sprintf($user->lang['THOSE_MISSING_LANG_VARIABLES'], $lang_entries['lang_local_name']),
							'U_MISSING_ACTION'			=> $this->u_action . "&amp;action=$action&amp;id=$lang_id")
						);

						foreach ($missing_vars as $file => $vars)
						{
							if (!sizeof($vars))
							{
								continue;
							}

							$template->assign_block_vars('missing', array(
								'FILE'			=> $file,
								'TPL'			=> $this->print_language_entries($vars, '', false),
								'KEY'			=> (strpos($file, '/') === false) ? '|' . $file : str_replace('/', '|', $file))
							);
						}
					}
				}

				// Main language files
				$s_lang_options = '<option value="|common.' . $phpEx . '" class="sep">' . $user->lang['LANGUAGE_FILES'] . '</option>';
				foreach ($this->main_files as $file)
				{
					if (strpos($file, 'help_') === 0)
					{
						continue;
					}

					$prefix = (file_exists($phpbb_root_path . $this->get_filename($lang_iso, '', $file, true, true))) ? '* ' : '';

					$selected = (!$this->language_directory && $this->language_file == $file) ? ' selected="selected"' : '';
					$s_lang_options .= '<option value="|' . $file . '"' . $selected . '>' . $prefix . $file . '</option>';
				}

				// Help Files
				$s_lang_options .= '<option value="|common.' . $phpEx . '" class="sep">' . $user->lang['HELP_FILES'] . '</option>';
				foreach ($this->main_files as $file)
				{
					if (strpos($file, 'help_') !== 0)
					{
						continue;
					}

					$prefix = (file_exists($phpbb_root_path . $this->get_filename($lang_iso, '', $file, true, true))) ? '* ' : '';

					$selected = (!$this->language_directory && $this->language_file == $file) ? ' selected="selected"' : '';
					$s_lang_options .= '<option value="|' . $file . '"' . $selected . '>' . $prefix . $file . '</option>';
				}

				// Now every other language directory
				$check_files = array('email', 'acp', 'mods');

				foreach ($check_files as $check)
				{
					if (!sizeof(${$check . '_files'}))
					{
						continue;
					}

					$s_lang_options .= '<option value="|common.' . $phpEx . '" class="sep">' . $user->lang[strtoupper($check) . '_FILES'] . '</option>';

					foreach (${$check . '_files'} as $file)
					{
						$prefix = (file_exists($phpbb_root_path . $this->get_filename($lang_iso, $check, $file, true, true))) ? '* ' : '';

						$selected = ($this->language_directory == $check && $this->language_file == $file) ? ' selected="selected"' : '';
						$s_lang_options .= '<option value="' . $check . '|' . $file . '"' . $selected . '>' . $prefix . $file . '</option>';
					}
				}

				// Get Language Entries - if saved within store folder, we take this one (with the option to remove it)
				$lang = array();

				$is_email_file = ($this->language_directory == 'email') ? true : false;
				$is_help_file = (strpos($this->language_file, 'help_') === 0) ? true : false;

				$file_from_store = (file_exists($phpbb_root_path . $this->get_filename($lang_iso, $this->language_directory, $this->language_file, true, true))) ? true : false;
				$no_store_filename = $this->get_filename($lang_iso, $this->language_directory, $this->language_file);

				if (!$file_from_store && !file_exists($phpbb_root_path . $no_store_filename))
				{
					$print_message = sprintf($user->lang['MISSING_LANGUAGE_FILE'], $no_store_filename);
				}
				else
				{
					if ($is_email_file)
					{
						$lang = file_get_contents($phpbb_root_path . $this->get_filename($lang_iso, $this->language_directory, $this->language_file, $file_from_store));
					}
					else
					{
						$help = array();
						include($phpbb_root_path . $this->get_filename($lang_iso, $this->language_directory, $this->language_file, $file_from_store));

						if ($is_help_file)
						{
							$lang = $help;
							unset($help);
						}
					}

					$print_message = (($this->language_directory) ? $this->language_directory . '/' : '') . $this->language_file;
				}

				// Normal language pack entries
				$template->assign_vars(array(
					'U_ENTRY_ACTION'		=> $this->u_action . "&amp;action=details&amp;id=$lang_id#entries",
					'S_EMAIL_FILE'			=> $is_email_file,
					'S_FROM_STORE'			=> $file_from_store,
					'S_LANG_OPTIONS'		=> $s_lang_options,
					'PRINT_MESSAGE'			=> $print_message,
					)
				);

				if (!$is_email_file)
				{
					$tpl = '';
					$name = (($this->language_directory) ? $this->language_directory . '/' : '') . $this->language_file;

					if (isset($missing_vars[$name]) && sizeof($missing_vars[$name]))
					{
						$tpl .= $this->print_language_entries($missing_vars[$name], '* ');
					}

					$tpl .= $this->print_language_entries($lang);

					$template->assign_var('TPL', $tpl);
					unset($tpl);
				}
				else
				{
					$template->assign_vars(array(
						'LANG'		=> $lang)
					);

					unset($lang);
				}

				return;

			break;

			case 'delete':

				if (!$lang_id)
				{
					trigger_error($user->lang['NO_LANG_ID'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				$sql = 'SELECT *
					FROM ' . LANG_TABLE . '
					WHERE lang_id = ' . $lang_id;
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if ($row['lang_iso'] == $config['default_lang'])
				{
					trigger_error($user->lang['NO_REMOVE_DEFAULT_LANG'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				$db->sql_query('DELETE FROM ' . LANG_TABLE . ' WHERE lang_id = ' . $lang_id);

				$sql = 'UPDATE ' . USERS_TABLE . "
					SET user_lang = '" . $db->sql_escape($config['default_lang']) . "'
					WHERE user_lang = '" . $db->sql_escape($row['lang_iso']) . "'";
				$db->sql_query($sql);

				// We also need to remove the translated entries for custom profile fields - we want clean tables, don't we?
				$sql = 'DELETE FROM ' . PROFILE_LANG_TABLE . ' WHERE lang_id = ' . $lang_id;
				$db->sql_query($sql);

				$sql = 'DELETE FROM ' . PROFILE_FIELDS_LANG_TABLE . ' WHERE lang_id = ' . $lang_id;
				$db->sql_query($sql);

				$sql = 'DELETE FROM ' . STYLES_IMAGESET_DATA_TABLE . " WHERE image_lang = '" . $db->sql_escape($row['lang_iso']) . "'";
				$result = $db->sql_query($sql);

				$cache->destroy('sql', STYLES_IMAGESET_DATA_TABLE);

				add_log('admin', 'LOG_LANGUAGE_PACK_DELETED', $row['lang_english_name']);

				trigger_error(sprintf($user->lang['LANGUAGE_PACK_DELETED'], $row['lang_english_name']) . adm_back_link($this->u_action));
			break;

			case 'install':
				$lang_iso = request_var('iso', '');
				$lang_iso = basename($lang_iso);

				if (!$lang_iso || !file_exists("{$phpbb_root_path}language/$lang_iso/iso.txt"))
				{
					trigger_error($user->lang['LANGUAGE_PACK_NOT_EXIST'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				$file = file("{$phpbb_root_path}language/$lang_iso/iso.txt");

				$lang_pack = array(
					'iso'		=> $lang_iso,
					'name'		=> trim(htmlspecialchars($file[0])),
					'local_name'=> trim(htmlspecialchars($file[1], ENT_COMPAT, 'UTF-8')),
					'author'	=> trim(htmlspecialchars($file[2], ENT_COMPAT, 'UTF-8'))
				);
				unset($file);

				$sql = 'SELECT lang_iso
					FROM ' . LANG_TABLE . "
					WHERE lang_iso = '" . $db->sql_escape($lang_iso) . "'";
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if ($row)
				{
					trigger_error($user->lang['LANGUAGE_PACK_ALREADY_INSTALLED'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				if (!$lang_pack['name'] || !$lang_pack['local_name'])
				{
					trigger_error($user->lang['INVALID_LANGUAGE_PACK'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				// Add language pack
				$sql_ary = array(
					'lang_iso'			=> $lang_pack['iso'],
					'lang_dir'			=> $lang_pack['iso'],
					'lang_english_name'	=> $lang_pack['name'],
					'lang_local_name'	=> $lang_pack['local_name'],
					'lang_author'		=> $lang_pack['author']
				);

				$db->sql_query('INSERT INTO ' . LANG_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary));
				$lang_id = $db->sql_nextid();

				$valid_localized = array(
					'icon_back_top', 'icon_contact_aim', 'icon_contact_email', 'icon_contact_icq', 'icon_contact_jabber', 'icon_contact_msnm', 'icon_contact_pm', 'icon_contact_yahoo', 'icon_contact_www', 'icon_post_delete', 'icon_post_edit', 'icon_post_info', 'icon_post_quote', 'icon_post_report', 'icon_user_online', 'icon_user_offline', 'icon_user_profile', 'icon_user_search', 'icon_user_warn', 'button_pm_forward', 'button_pm_new', 'button_pm_reply', 'button_topic_locked', 'button_topic_new', 'button_topic_reply',
				);

				$sql_ary = array();

				$sql = 'SELECT *
					FROM ' . STYLES_IMAGESET_TABLE;
				$result = $db->sql_query($sql);
				while ($imageset_row = $db->sql_fetchrow($result))
				{
					if (@file_exists("{$phpbb_root_path}styles/{$imageset_row['imageset_path']}/imageset/{$lang_pack['iso']}/imageset.cfg"))
					{
						$cfg_data_imageset_data = parse_cfg_file("{$phpbb_root_path}styles/{$imageset_row['imageset_path']}/imageset/{$lang_pack['iso']}/imageset.cfg");
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
								if (in_array($image_name, $valid_localized))
								{
									$sql_ary[] = array(
										'image_name'		=> (string) $image_name,
										'image_filename'	=> (string) $image_filename,
										'image_height'		=> (int) $image_height,
										'image_width'		=> (int) $image_width,
										'imageset_id'		=> (int) $imageset_row['imageset_id'],
										'image_lang'		=> (string) $lang_pack['iso'],
									);
								}
							}
						}
					}
				}
				$db->sql_freeresult($result);

				if (sizeof($sql_ary))
				{
					$db->sql_multi_insert(STYLES_IMAGESET_DATA_TABLE, $sql_ary);
					$cache->destroy('sql', STYLES_IMAGESET_DATA_TABLE);
				}

				// Now let's copy the default language entries for custom profile fields for this new language - makes admin's life easier.
				$sql = 'SELECT lang_id
					FROM ' . LANG_TABLE . "
					WHERE lang_iso = '" . $db->sql_escape($config['default_lang']) . "'";
				$result = $db->sql_query($sql);
				$default_lang_id = (int) $db->sql_fetchfield('lang_id');
				$db->sql_freeresult($result);

				// From the mysql documentation:
				// Prior to MySQL 4.0.14, the target table of the INSERT statement cannot appear in the FROM clause of the SELECT part of the query. This limitation is lifted in 4.0.14.
				// Due to this we stay on the safe side if we do the insertion "the manual way"

				$sql = 'SELECT field_id, lang_name, lang_explain, lang_default_value
					FROM ' . PROFILE_LANG_TABLE . '
					WHERE lang_id = ' . $default_lang_id;
				$result = $db->sql_query($sql);

				while ($row = $db->sql_fetchrow($result))
				{
					$row['lang_id'] = $lang_id;
					$db->sql_query('INSERT INTO ' . PROFILE_LANG_TABLE . ' ' . $db->sql_build_array('INSERT', $row));
				}
				$db->sql_freeresult($result);

				$sql = 'SELECT field_id, option_id, field_type, lang_value
					FROM ' . PROFILE_FIELDS_LANG_TABLE . '
					WHERE lang_id = ' . $default_lang_id;
				$result = $db->sql_query($sql);

				while ($row = $db->sql_fetchrow($result))
				{
					$row['lang_id'] = $lang_id;
					$db->sql_query('INSERT INTO ' . PROFILE_FIELDS_LANG_TABLE . ' ' . $db->sql_build_array('INSERT', $row));
				}
				$db->sql_freeresult($result);

				add_log('admin', 'LOG_LANGUAGE_PACK_INSTALLED', $lang_pack['name']);

				trigger_error(sprintf($user->lang['LANGUAGE_PACK_INSTALLED'], $lang_pack['name']) . adm_back_link($this->u_action));

			break;

			case 'download':

				if (!$lang_id)
				{
					trigger_error($user->lang['NO_LANG_ID'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				$sql = 'SELECT *
					FROM ' . LANG_TABLE . '
					WHERE lang_id = ' . $lang_id;
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				$use_method = request_var('use_method', '');
				$methods = array('.tar');

				$available_methods = array('.tar.gz' => 'zlib', '.tar.bz2' => 'bz2', '.zip' => 'zlib');
				foreach ($available_methods as $type => $module)
				{
					if (!@extension_loaded($module))
					{
						continue;
					}

					$methods[] = $type;
				}

				// Let the user decide in which format he wants to have the pack
				if (!$use_method)
				{
					$this->page_title = 'SELECT_DOWNLOAD_FORMAT';

					$radio_buttons = '';
					foreach ($methods as $method)
					{
						$radio_buttons .= '<label><input type="radio"' . ((!$radio_buttons) ? ' id="use_method"' : '') . ' class="radio" value="' . $method . '" name="use_method" /> ' . $method . '</label>';
					}

					$template->assign_vars(array(
						'S_SELECT_METHOD'		=> true,
						'U_BACK'				=> $this->u_action,
						'U_ACTION'				=> $this->u_action . "&amp;action=$action&amp;id=$lang_id",
						'RADIO_BUTTONS'			=> $radio_buttons)
					);

					return;
				}

				if (!in_array($use_method, $methods))
				{
					$use_method = '.tar';
				}

				include_once($phpbb_root_path . 'includes/functions_compress.' . $phpEx);

				if ($use_method == '.zip')
				{
					$compress = new compress_zip('w', $phpbb_root_path . 'store/lang_' . $row['lang_iso'] . $use_method);
				}
				else
				{
					$compress = new compress_tar('w', $phpbb_root_path . 'store/lang_' . $row['lang_iso'] . $use_method, $use_method);
				}

				// Get email templates
				$email_templates = filelist($phpbb_root_path . 'language/' . $row['lang_iso'], 'email', 'txt');
				$email_templates = $email_templates['email/'];

				// Get acp files
				$acp_files = filelist($phpbb_root_path . 'language/' . $row['lang_iso'], 'acp', $phpEx);
				$acp_files = $acp_files['acp/'];

				// Get mod files
				$mod_files = filelist($phpbb_root_path . 'language/' . $row['lang_iso'], 'mods', $phpEx);
				$mod_files = (isset($mod_files['mods/'])) ? $mod_files['mods/'] : array();

				// Add main files
				$this->add_to_archive($compress, $this->main_files, $row['lang_iso']);

				// Add search files if they exist...
				if (file_exists($phpbb_root_path . 'language/' . $row['lang_iso'] . '/search_ignore_words.' . $phpEx))
				{
					$this->add_to_archive($compress, array("search_ignore_words.$phpEx"), $row['lang_iso']);
				}

				if (file_exists($phpbb_root_path . 'language/' . $row['lang_iso'] . '/search_synonyms.' . $phpEx))
				{
					$this->add_to_archive($compress, array("search_synonyms.$phpEx"), $row['lang_iso']);
				}

				// Write files in folders
				$this->add_to_archive($compress, $email_templates, $row['lang_iso'], 'email');
				$this->add_to_archive($compress, $acp_files, $row['lang_iso'], 'acp');
				$this->add_to_archive($compress, $mod_files, $row['lang_iso'], 'mods');

				// Write ISO File
				$iso_src = htmlspecialchars_decode($row['lang_english_name']) . "\n";
				$iso_src .= htmlspecialchars_decode($row['lang_local_name']) . "\n";
				$iso_src .= htmlspecialchars_decode($row['lang_author']);
				$compress->add_data($iso_src, 'language/' . $row['lang_iso'] . '/iso.txt');

				// index.html files
				$compress->add_data('', 'language/' . $row['lang_iso'] . '/index.html');
				$compress->add_data('', 'language/' . $row['lang_iso'] . '/email/index.html');
				$compress->add_data('', 'language/' . $row['lang_iso'] . '/acp/index.html');
				
				if (sizeof($mod_files))
				{
					$compress->add_data('', 'language/' . $row['lang_iso'] . '/mods/index.html');
				}

				$compress->close();

				$compress->download('lang_' . $row['lang_iso']);
				@unlink($phpbb_root_path . 'store/lang_' . $row['lang_iso'] . $use_method);

				exit;

			break;
		}

		$sql = 'SELECT user_lang, COUNT(user_lang) AS lang_count
			FROM ' . USERS_TABLE . '
			GROUP BY user_lang';
		$result = $db->sql_query($sql);

		$lang_count = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$lang_count[$row['user_lang']] = $row['lang_count'];
		}
		$db->sql_freeresult($result);

		$sql = 'SELECT *
			FROM ' . LANG_TABLE . '
			ORDER BY lang_english_name';
		$result = $db->sql_query($sql);

		$installed = array();

		while ($row = $db->sql_fetchrow($result))
		{
			$installed[] = $row['lang_iso'];
			$tagstyle = ($row['lang_iso'] == $config['default_lang']) ? '*' : '';

			$template->assign_block_vars('lang', array(
				'U_DETAILS'			=> $this->u_action . "&amp;action=details&amp;id={$row['lang_id']}",
				'U_DOWNLOAD'		=> $this->u_action . "&amp;action=download&amp;id={$row['lang_id']}",
				'U_DELETE'			=> $this->u_action . "&amp;action=delete&amp;id={$row['lang_id']}",

				'ENGLISH_NAME'		=> $row['lang_english_name'],
				'TAG'				=> $tagstyle,
				'LOCAL_NAME'		=> $row['lang_local_name'],
				'ISO'				=> $row['lang_iso'],
				'USED_BY'			=> (isset($lang_count[$row['lang_iso']])) ? $lang_count[$row['lang_iso']] : 0,
			));
		}
		$db->sql_freeresult($result);

		$new_ary = $iso = array();
		$dp = @opendir("{$phpbb_root_path}language");

		if ($dp)
		{
			while (($file = readdir($dp)) !== false)
			{
				if ($file[0] != '.' && file_exists("{$phpbb_root_path}language/$file/iso.txt"))
				{
					if (!in_array($file, $installed))
					{
						if ($iso = file("{$phpbb_root_path}language/$file/iso.txt"))
						{
							if (sizeof($iso) == 3)
							{
								$new_ary[$file] = array(
									'iso'		=> $file,
									'name'		=> trim($iso[0]),
									'local_name'=> trim($iso[1]),
									'author'	=> trim($iso[2])
								);
							}
						}
					}
				}
			}
			closedir($dp);
		}

		unset($installed);

		if (sizeof($new_ary))
		{
			foreach ($new_ary as $iso => $lang_ary)
			{
				$template->assign_block_vars('notinst', array(
					'ISO'			=> htmlspecialchars($lang_ary['iso']),
					'LOCAL_NAME'	=> htmlspecialchars($lang_ary['local_name'], ENT_COMPAT, 'UTF-8'),
					'NAME'			=> htmlspecialchars($lang_ary['name'], ENT_COMPAT, 'UTF-8'),
					'U_INSTALL'		=> $this->u_action . '&amp;action=install&amp;iso=' . urlencode($lang_ary['iso']))
				);
			}
		}

		unset($new_ary);
	}


	/**
	* Set default language variables/header
	*/
	function default_variables()
	{
		global $phpEx;

		$this->language_file_header = '<?php
/**
*
* {FILENAME} [{LANG_NAME}]
*
* @package language
* @version $' . 'Id: ' . '$
* @copyright (c) ' . date('Y') . ' phpBB Group
* @author {CHANGED} - {AUTHOR}
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* DO NOT CHANGE
*/
if (!defined(\'IN_PHPBB\'))
{
	exit;
}

if (empty($lang) || !is_array($lang))
{
	$lang = array();
}

// DEVELOPERS PLEASE NOTE
//
// All language files should use UTF-8 as their encoding and the files must not contain a BOM.
//
// Placeholders can now contain order information, e.g. instead of
// \'Page %s of %s\' you can (and should) write \'Page %1$s of %2$s\', this allows
// translators to re-order the output of data while ensuring it remains correct
//
// You do not need this where single placeholders are used, e.g. \'Message %d\' is fine
// equally where a string contains only two placeholders which are used to wrap text
// in a url you again do not need to specify an order e.g., \'Click %sHERE%s\' is fine
';

		$this->lang_header = '
$lang = array_merge($lang, array(
';

		// Language files in language root directory
		$this->main_files = array("common.$phpEx", "groups.$phpEx", "install.$phpEx", "mcp.$phpEx", "memberlist.$phpEx", "posting.$phpEx", "search.$phpEx", "ucp.$phpEx", "viewforum.$phpEx", "viewtopic.$phpEx", "help_bbcode.$phpEx", "help_faq.$phpEx");
	}

	/**
	* Get filename/location of language file
	*/
	function get_filename($lang_iso, $directory, $filename, $check_store = false, $only_return_filename = false)
	{
		global $phpbb_root_path, $safe_mode;
		
		$check_filename = "language/$lang_iso/" . (($directory) ? $directory . '/' : '') . $filename;

		if ($check_store)
		{
			$check_store_filename = ($safe_mode) ? "store/langfile_{$lang_iso}" . (($directory) ? '_' . $directory : '') . "_{$filename}" : "store/language/$lang_iso/" . (($directory) ? $directory . '/' : '') . $filename;

			if (!$only_return_filename && file_exists($phpbb_root_path . $check_store_filename))
			{
				return $check_store_filename;
			}
			else if ($only_return_filename)
			{
				return $check_store_filename;
			}
		}

		return $check_filename;
	}

	/**
	* Add files to archive
	*/
	function add_to_archive(&$compress, $filelist, $lang_iso, $directory = '')
	{
		global $phpbb_root_path;

		foreach ($filelist as $file)
		{
			// Get source filename
			$source = $this->get_filename($lang_iso, $directory, $file, true);
			$destination = 'language/' . $lang_iso . '/' . (($directory) ? $directory . '/' : '') . $file;

			// Add file to archive
			$compress->add_custom_file($phpbb_root_path . $source, $destination);
		}
	}

	/**
	* Little helper to add some hardcoded template bits
	*/
	function add_input_field()
	{
		$keys = func_get_args();

		$non_static		= array_shift($keys);
		$value			= array_shift($keys);

		if (!$non_static)
		{
			return '<strong>' . htmlspecialchars($value, ENT_COMPAT, 'UTF-8') . '</strong>';
		}

		// If more then 270 characters, then we present a textarea, else an input field
		$textarea = (utf8_strlen($value) > 270) ? true : false;
		$tpl = '';

		$tpl .= ($textarea) ? '<textarea name="' : '<input type="text" name="';
		$tpl .= 'entry[' . implode('][', array_map('utf8_htmlspecialchars', $keys)) . ']"';

		$tpl .= ($textarea) ? ' cols="80" rows="5" class="langvalue">' : ' class="langvalue" value="';
		$tpl .= htmlspecialchars($value, ENT_COMPAT, 'UTF-8');
		$tpl .= ($textarea) ? '</textarea>' : '" />';

		return $tpl;
	}

	/**
	* Print language entries
	*/
	function print_language_entries(&$lang_ary, $key_prefix = '', $input_field = true)
	{
		$tpl = '';

		foreach ($lang_ary as $key => $value)
		{
			if (is_array($value))
			{
				// Write key
				$tpl .= '
				<tr>
					<td class="row3" colspan="2">' . htmlspecialchars($key_prefix, ENT_COMPAT, 'UTF-8') . '<strong>' . htmlspecialchars($key, ENT_COMPAT, 'UTF-8') . '</strong></td>
				</tr>';

				foreach ($value as $_key => $_value)
				{
					if (is_array($_value))
					{
						// Write key
						$tpl .= '
							<tr>
								<td class="row3" colspan="2">' . htmlspecialchars($key_prefix, ENT_COMPAT, 'UTF-8') . '&nbsp; &nbsp;<strong>' . htmlspecialchars($_key, ENT_COMPAT, 'UTF-8') . '</strong></td>
							</tr>';

						foreach ($_value as $__key => $__value)
						{
							// Write key
							$tpl .= '
								<tr>
									<td class="row1" style="white-space: nowrap;">' . htmlspecialchars($key_prefix, ENT_COMPAT, 'UTF-8') . '<strong>' . htmlspecialchars($__key, ENT_COMPAT, 'UTF-8') . '</strong></td>
									<td class="row2">';

							$tpl .= $this->add_input_field($input_field, $__value, $key, $_key, $__key);

							$tpl .= '</td>
								</tr>';
						}
					}
					else
					{
						// Write key
						$tpl .= '
							<tr>
								<td class="row1" style="white-space: nowrap;">' . htmlspecialchars($key_prefix, ENT_COMPAT, 'UTF-8') . '<strong>' . htmlspecialchars($_key, ENT_COMPAT, 'UTF-8') . '</strong></td>
								<td class="row2">';

						$tpl .= $this->add_input_field($input_field, $_value, $key, $_key);

						$tpl .= '</td>
							</tr>';
					}
				}

				$tpl .= '
				<tr>
					<td class="spacer" colspan="2">&nbsp;</td>
				</tr>';
			}
			else
			{
				// Write key
				$tpl .= '
				<tr>
					<td class="row1" style="white-space: nowrap;">' . htmlspecialchars($key_prefix, ENT_COMPAT, 'UTF-8') . '<strong>' . htmlspecialchars($key, ENT_COMPAT, 'UTF-8') . '</strong></td>
					<td class="row2">';

				$tpl .= $this->add_input_field($input_field, $value, $key);

				$tpl .= '</td>
					</tr>';
			}
		}

		return $tpl;
	}

	/**
	* Compare two language files
	*/
	function compare_language_files($source_lang, $dest_lang, $directory, $file)
	{
		global $phpbb_root_path, $phpEx;

		$return_ary = array();

		$lang = array();
		include("{$phpbb_root_path}language/{$source_lang}/" . (($directory) ? $directory . '/' : '') . $file);
		$lang_entry_src = $lang;

		$lang = array();

		if (!file_exists($phpbb_root_path . $this->get_filename($dest_lang, $directory, $file, true)))
		{
			return array();
		}

		include($phpbb_root_path . $this->get_filename($dest_lang, $directory, $file, true));

		$lang_entry_dst = $lang;

		unset($lang);

		$diff_array_keys = array_diff(array_keys($lang_entry_src), array_keys($lang_entry_dst));
		unset($lang_entry_dst);

		foreach ($diff_array_keys as $key)
		{
			$return_ary[$key] = $lang_entry_src[$key];
		}

		unset($lang_entry_src);

		return $return_ary;
	}

	/**
	* Return language string value for storage
	*/
	function prepare_lang_entry($text, $store = true)
	{
		$text = (STRIP) ? stripslashes($text) : $text;

		// Adjust for storage...
		if ($store)
		{
			$text = str_replace("'", "\\'", str_replace('\\', '\\\\', $text));
		}

		return $text;
	}

	/**
	* Format language array for storage
	*/
	function format_lang_array($key, $value, $tabs = "\t")
	{
		$entry = '';

		if (!is_array($value))
		{
			$entry .= "{$tabs}'" . $this->prepare_lang_entry($key) . "'\t=> '" . $this->prepare_lang_entry($value) . "',\n";
		}
		else
		{
			$_tabs = $tabs . "\t";
			$entry .= "\n{$tabs}'" . $this->prepare_lang_entry($key) . "'\t=> array(\n";

			foreach ($value as $_key => $_value)
			{
				$entry .= $this->format_lang_array($_key, $_value, $_tabs);
			}

			$entry .= "{$tabs}),\n\n";
		}

		return $entry;
	}
}

?>