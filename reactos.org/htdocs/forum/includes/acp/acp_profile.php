<?php
/**
*
* @package acp
* @version $Id: acp_profile.php 8479 2008-03-29 00:22:48Z naderman $
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
class acp_profile
{
	var $u_action;

	var $edit_lang_id;
	var $lang_defs;

	function main($id, $mode)
	{
		global $config, $db, $user, $auth, $template, $cache;
		global $phpbb_root_path, $phpbb_admin_path, $phpEx, $table_prefix;

		include($phpbb_root_path . 'includes/functions_posting.' . $phpEx);
		include($phpbb_root_path . 'includes/functions_user.' . $phpEx);
		include($phpbb_root_path . 'includes/functions_profile_fields.' . $phpEx);

		$user->add_lang(array('ucp', 'acp/profile'));
		$this->tpl_name = 'acp_profile';
		$this->page_title = 'ACP_CUSTOM_PROFILE_FIELDS';

		$action = (isset($_POST['create'])) ? 'create' : request_var('action', '');

		$error = array();
		$s_hidden_fields = '';

		// Define some default values for each field type
		$default_values = array(
			FIELD_STRING	=> array('field_length' => 10, 'field_minlen' => 0, 'field_maxlen' => 20, 'field_validation' => '.*', 'field_novalue' => '', 'field_default_value' => ''),
			FIELD_TEXT		=> array('field_length' => '5|80', 'field_minlen' => 0, 'field_maxlen' => 1000, 'field_validation' => '.*', 'field_novalue' => '', 'field_default_value' => ''),
			FIELD_INT		=> array('field_length' => 5, 'field_minlen' => 0, 'field_maxlen' => 100, 'field_validation' => '', 'field_novalue' => 0, 'field_default_value' => 0),
			FIELD_DATE		=> array('field_length' => 10, 'field_minlen' => 10, 'field_maxlen' => 10, 'field_validation' => '', 'field_novalue' => ' 0- 0-   0', 'field_default_value' => ' 0- 0-   0'),
			FIELD_BOOL		=> array('field_length' => 1, 'field_minlen' => 0, 'field_maxlen' => 0, 'field_validation' => '', 'field_novalue' => 0, 'field_default_value' => 0),
			FIELD_DROPDOWN	=> array('field_length' => 0, 'field_minlen' => 0, 'field_maxlen' => 5, 'field_validation' => '', 'field_novalue' => 0, 'field_default_value' => 0),
		);

		$cp = new custom_profile_admin();

		// Build Language array
		// Based on this, we decide which elements need to be edited later and which language items are missing
		$this->lang_defs = array();

		$sql = 'SELECT lang_id, lang_iso
			FROM ' . LANG_TABLE . '
			ORDER BY lang_english_name';
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			// Make some arrays with all available languages
			$this->lang_defs['id'][$row['lang_id']] = $row['lang_iso'];
			$this->lang_defs['iso'][$row['lang_iso']] = $row['lang_id'];
		}
		$db->sql_freeresult($result);

		$sql = 'SELECT field_id, lang_id
			FROM ' . PROFILE_LANG_TABLE . '
			ORDER BY lang_id';
		$result = $db->sql_query($sql);
	
		while ($row = $db->sql_fetchrow($result))
		{
			// Which languages are available for each item
			$this->lang_defs['entry'][$row['field_id']][] = $row['lang_id'];
		}
		$db->sql_freeresult($result);

		// Have some fields been defined?
		if (isset($this->lang_defs['entry']))
		{
			foreach ($this->lang_defs['entry'] as $field_id => $field_ary)
			{
				// Fill an array with the languages that are missing for each field
				$this->lang_defs['diff'][$field_id] = array_diff(array_values($this->lang_defs['iso']), $field_ary);
			}
		}

		switch ($action)
		{
			case 'delete':
				$field_id = request_var('field_id', 0);

				if (!$field_id)
				{
					trigger_error($user->lang['NO_FIELD_ID'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				if (confirm_box(true))
				{
					$sql = 'SELECT field_ident
						FROM ' . PROFILE_FIELDS_TABLE . "
						WHERE field_id = $field_id";
					$result = $db->sql_query($sql);
					$field_ident = (string) $db->sql_fetchfield('field_ident');
					$db->sql_freeresult($result);

					$db->sql_transaction('begin');

					$db->sql_query('DELETE FROM ' . PROFILE_FIELDS_TABLE . " WHERE field_id = $field_id");
					$db->sql_query('DELETE FROM ' . PROFILE_FIELDS_LANG_TABLE . " WHERE field_id = $field_id");
					$db->sql_query('DELETE FROM ' . PROFILE_LANG_TABLE . " WHERE field_id = $field_id");

					switch ($db->sql_layer)
					{
						case 'sqlite':
							$sql = "SELECT sql
								FROM sqlite_master
								WHERE type = 'table'
									AND name = '" . PROFILE_FIELDS_DATA_TABLE . "'
								ORDER BY type DESC, name;";
							$result = $db->sql_query($sql);
							$row = $db->sql_fetchrow($result);
							$db->sql_freeresult($result);

							// Create a temp table and populate it, destroy the existing one
							$db->sql_query(preg_replace('#CREATE\s+TABLE\s+"?' . PROFILE_FIELDS_DATA_TABLE . '"?#i', 'CREATE TEMPORARY TABLE ' . PROFILE_FIELDS_DATA_TABLE . '_temp', $row['sql']));
							$db->sql_query('INSERT INTO ' . PROFILE_FIELDS_DATA_TABLE . '_temp SELECT * FROM ' . PROFILE_FIELDS_DATA_TABLE);
							$db->sql_query('DROP TABLE ' . PROFILE_FIELDS_DATA_TABLE);

							preg_match('#\((.*)\)#s', $row['sql'], $matches);

							$new_table_cols = trim($matches[1]);
							$old_table_cols = preg_split('/,(?=[\\sa-z])/im', $new_table_cols);
							$column_list = array();

							foreach ($old_table_cols as $declaration)
							{
								$entities = preg_split('#\s+#', trim($declaration));

								if ($entities[0] == 'PRIMARY')
								{
									continue;
								}

								if ($entities[0] !== 'pf_' . $field_ident)
								{
									$column_list[] = $entities[0];
								}
							}

							$columns = implode(',', $column_list);

							$new_table_cols = preg_replace('/' . 'pf_' . $field_ident . '[^,]+,/', '', $new_table_cols);

							// create a new table and fill it up. destroy the temp one
							$db->sql_query('CREATE TABLE ' . PROFILE_FIELDS_DATA_TABLE . ' (' . $new_table_cols . ');');
							$db->sql_query('INSERT INTO ' . PROFILE_FIELDS_DATA_TABLE . ' (' . $columns . ') SELECT ' . $columns . ' FROM ' . PROFILE_FIELDS_DATA_TABLE . '_temp;');
							$db->sql_query('DROP TABLE ' . PROFILE_FIELDS_DATA_TABLE . '_temp');
						break;

						default:
							$db->sql_query('ALTER TABLE ' . PROFILE_FIELDS_DATA_TABLE . " DROP COLUMN pf_$field_ident");
					}

					$order = 0;

					$sql = 'SELECT *
						FROM ' . PROFILE_FIELDS_TABLE . '
						ORDER BY field_order';
					$result = $db->sql_query($sql);

					while ($row = $db->sql_fetchrow($result))
					{
						$order++;
						if ($row['field_order'] != $order)
						{
							$sql = 'UPDATE ' . PROFILE_FIELDS_TABLE . "
								SET field_order = $order
								WHERE field_id = {$row['field_id']}";
							$db->sql_query($sql);
						}
					}
					$db->sql_freeresult($result);

					$db->sql_transaction('commit');

					add_log('admin', 'LOG_PROFILE_FIELD_REMOVED', $field_ident);
					trigger_error($user->lang['REMOVED_PROFILE_FIELD'] . adm_back_link($this->u_action));
				}
				else
				{
					confirm_box(false, 'DELETE_PROFILE_FIELD', build_hidden_fields(array(
						'i'			=> $id,
						'mode'		=> $mode,
						'action'	=> $action,
						'field_id'	=> $field_id,
					)));
				}
	
			break;

			case 'activate':
				$field_id = request_var('field_id', 0);

				if (!$field_id)
				{
					trigger_error($user->lang['NO_FIELD_ID'] . adm_back_link($this->u_action), E_USER_WARNING);
				}
	
				$sql = 'SELECT lang_id
					FROM ' . LANG_TABLE . "
					WHERE lang_iso = '" . $db->sql_escape($config['default_lang']) . "'";
				$result = $db->sql_query($sql);
				$default_lang_id = (int) $db->sql_fetchfield('lang_id');
				$db->sql_freeresult($result);

				if (!in_array($default_lang_id, $this->lang_defs['entry'][$field_id]))
				{
					trigger_error($user->lang['DEFAULT_LANGUAGE_NOT_FILLED'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				$sql = 'UPDATE ' . PROFILE_FIELDS_TABLE . "
					SET field_active = 1
					WHERE field_id = $field_id";
				$db->sql_query($sql);

				$sql = 'SELECT field_ident
					FROM ' . PROFILE_FIELDS_TABLE . "
					WHERE field_id = $field_id";
				$result = $db->sql_query($sql);
				$field_ident = (string) $db->sql_fetchfield('field_ident');
				$db->sql_freeresult($result);

				add_log('admin', 'LOG_PROFILE_FIELD_ACTIVATE', $field_ident);
				trigger_error($user->lang['PROFILE_FIELD_ACTIVATED'] . adm_back_link($this->u_action));

			break;

			case 'deactivate':
				$field_id = request_var('field_id', 0);

				if (!$field_id)
				{
					trigger_error($user->lang['NO_FIELD_ID'] . adm_back_link($this->u_action), E_USER_WARNING);
				}
	
				$sql = 'UPDATE ' . PROFILE_FIELDS_TABLE . "
					SET field_active = 0
					WHERE field_id = $field_id";
				$db->sql_query($sql);

				$sql = 'SELECT field_ident
					FROM ' . PROFILE_FIELDS_TABLE . "
					WHERE field_id = $field_id";
				$result = $db->sql_query($sql);
				$field_ident = (string) $db->sql_fetchfield('field_ident');
				$db->sql_freeresult($result);

				add_log('admin', 'LOG_PROFILE_FIELD_DEACTIVATE', $field_ident);
				trigger_error($user->lang['PROFILE_FIELD_DEACTIVATED'] . adm_back_link($this->u_action));

			break;

			case 'move_up':
			case 'move_down':
				$field_order = request_var('order', 0);
				$order_total = $field_order * 2 + (($action == 'move_up') ? -1 : 1);

				$sql = 'UPDATE ' . PROFILE_FIELDS_TABLE . "
					SET field_order = $order_total - field_order
					WHERE field_order IN ($field_order, " . (($action == 'move_up') ? $field_order - 1 : $field_order + 1) . ')';
				$db->sql_query($sql);

			break;

			case 'create':
			case 'edit':
		
				$field_id = request_var('field_id', 0);
				$step = request_var('step', 1);

				$submit = (isset($_REQUEST['next']) || isset($_REQUEST['prev'])) ? true : false;
				$save = (isset($_REQUEST['save'])) ? true : false;

				// The language id of default language
				$this->edit_lang_id = $this->lang_defs['iso'][$config['default_lang']];

				// We are editing... we need to grab basic things
				if ($action == 'edit')
				{
					if (!$field_id)
					{
						trigger_error($user->lang['NO_FIELD_ID'] . adm_back_link($this->u_action), E_USER_WARNING);
					}

					$sql = 'SELECT l.*, f.*
						FROM ' . PROFILE_LANG_TABLE . ' l, ' . PROFILE_FIELDS_TABLE . ' f
						WHERE l.lang_id = ' . $this->edit_lang_id . "
							AND f.field_id = $field_id
							AND l.field_id = f.field_id";
					$result = $db->sql_query($sql);
					$field_row = $db->sql_fetchrow($result);
					$db->sql_freeresult($result);

					if (!$field_row)
					{
						// Some admin changed the default language?
						$sql = 'SELECT l.*, f.*
							FROM ' . PROFILE_LANG_TABLE . ' l, ' . PROFILE_FIELDS_TABLE . ' f
							WHERE l.lang_id <> ' . $this->edit_lang_id . "
							AND f.field_id = $field_id
							AND l.field_id = f.field_id";
						$result = $db->sql_query($sql);
						$field_row = $db->sql_fetchrow($result);
						$db->sql_freeresult($result);

						if (!$field_row)
						{
							trigger_error($user->lang['FIELD_NOT_FOUND'] . adm_back_link($this->u_action), E_USER_WARNING);
						}

						$this->edit_lang_id = $field_row['lang_id'];
					}
					$field_type = $field_row['field_type'];

					// Get language entries
					$sql = 'SELECT *
						FROM ' . PROFILE_FIELDS_LANG_TABLE . '
						WHERE lang_id = ' . $this->edit_lang_id . "
							AND field_id = $field_id
						ORDER BY option_id ASC";
					$result = $db->sql_query($sql);

					$lang_options = array();
					while ($row = $db->sql_fetchrow($result))
					{
						$lang_options[$row['option_id']] = $row['lang_value'];
					}
					$db->sql_freeresult($result);

					$s_hidden_fields = '<input type="hidden" name="field_id" value="' . $field_id . '" />';
				}
				else
				{
					// We are adding a new field, define basic params
					$lang_options = $field_row = array();
		
					$field_type = request_var('field_type', 0);
			
					if (!$field_type)
					{
						trigger_error($user->lang['NO_FIELD_TYPE'] . adm_back_link($this->u_action), E_USER_WARNING);
					}

					$field_row = array_merge($default_values[$field_type], array(
						'field_ident'		=> utf8_clean_string(request_var('field_ident', '', true)),
						'field_required'	=> 0,
						'field_hide'		=> 0,
						'field_no_view'		=> 0,
						'field_show_on_reg'	=> 0,
						'lang_name'			=> utf8_normalize_nfc(request_var('field_ident', '', true)),
						'lang_explain'		=> '',
						'lang_default_value'=> '')
					);

					$s_hidden_fields = '<input type="hidden" name="field_type" value="' . $field_type . '" />';
				}

				// $exclude contains the data we gather in each step
				$exclude = array(
					1	=> array('field_ident', 'lang_name', 'lang_explain', 'field_option', 'field_no_view'),
					2	=> array('field_length', 'field_maxlen', 'field_minlen', 'field_validation', 'field_novalue', 'field_default_value'),
					3	=> array('l_lang_name', 'l_lang_explain', 'l_lang_default_value', 'l_lang_options')
				);

				// Text-based fields require the lang_default_value to be excluded
				if ($field_type == FIELD_STRING || $field_type == FIELD_TEXT)
				{
					$exclude[1][] = 'lang_default_value';
				}

				// option-specific fields require lang_options to be excluded
				if ($field_type == FIELD_BOOL || $field_type == FIELD_DROPDOWN)
				{
					$exclude[1][] = 'lang_options';
				}

				$cp->vars['field_ident']		= ($action == 'create' && $step == 1) ? utf8_clean_string(request_var('field_ident', $field_row['field_ident'], true)) : request_var('field_ident', $field_row['field_ident']);
				$cp->vars['lang_name']			= utf8_normalize_nfc(request_var('lang_name', $field_row['lang_name'], true));
				$cp->vars['lang_explain']		= utf8_normalize_nfc(request_var('lang_explain', $field_row['lang_explain'], true));
				$cp->vars['lang_default_value']	= utf8_normalize_nfc(request_var('lang_default_value', $field_row['lang_default_value'], true));

				// Field option...
				if (isset($_REQUEST['field_option']))
				{
					$field_option = request_var('field_option', '');

					$cp->vars['field_required'] = ($field_option == 'field_required') ? 1 : 0;
					$cp->vars['field_show_on_reg'] = ($field_option == 'field_show_on_reg') ? 1 : 0;
					$cp->vars['field_hide'] = ($field_option == 'field_hide') ? 1 : 0;
				}
				else
				{
					$cp->vars['field_required'] = $field_row['field_required'];
					$cp->vars['field_show_on_reg'] = $field_row['field_show_on_reg'];
					$cp->vars['field_hide'] = $field_row['field_hide'];

					$field_option = ($field_row['field_required']) ? 'field_required' : (($field_row['field_show_on_reg']) ? 'field_show_on_reg' : (($field_row['field_hide']) ? 'field_hide' : ''));
				}

				$cp->vars['field_no_view'] = request_var('field_no_view', $field_row['field_no_view']);

				// A boolean field expects an array as the lang options
				if ($field_type == FIELD_BOOL)
				{
					$options = utf8_normalize_nfc(request_var('lang_options', array(''), true));
				}
				else
				{
					$options = utf8_normalize_nfc(request_var('lang_options', '', true));
				}

				// If the user has submitted a form with options (i.e. dropdown field)
				if ($options)
				{
					$exploded_options = (is_array($options)) ? $options : explode("\n", $options);

					if (sizeof($exploded_options) == sizeof($lang_options) || $action == 'create')
					{
						// The number of options in the field is equal to the number of options already in the database
						// Or we are creating a new dropdown list.
						$cp->vars['lang_options'] = $exploded_options;
					}
					else if ($action == 'edit')
					{
						// Changing the number of options? (We remove and re-create the option fields)
						$cp->vars['lang_options'] = $exploded_options;
					}
				}
				else
				{
					$cp->vars['lang_options'] = $lang_options;
				}

				// step 2
				foreach ($exclude[2] as $key)
				{
					$var = utf8_normalize_nfc(request_var($key, $field_row[$key], true));

					// Manipulate the intended variables a little bit if needed
					if ($field_type == FIELD_DROPDOWN && $key == 'field_maxlen')
					{
						// Get the number of options if this key is 'field_maxlen'
						$var = sizeof(explode("\n", utf8_normalize_nfc(request_var('lang_options', '', true))));
					}
					else if ($field_type == FIELD_TEXT && $key == 'field_length')
					{
						if (isset($_REQUEST['rows']))
						{
							$cp->vars['rows'] = request_var('rows', 0);
							$cp->vars['columns'] = request_var('columns', 0);
							$var = $cp->vars['rows'] . '|' . $cp->vars['columns'];
						}
						else
						{
							$row_col = explode('|', $var);
							$cp->vars['rows'] = $row_col[0];
							$cp->vars['columns'] = $row_col[1];
						}
					}
					else if ($field_type == FIELD_DATE && $key == 'field_default_value')
					{
						$always_now = request_var('always_now', -1);
						
						if ($always_now == 1 || ($always_now === -1 && $var == 'now'))
						{
							$now = getdate();

							$cp->vars['field_default_value_day'] = $now['mday'];
							$cp->vars['field_default_value_month'] = $now['mon'];
							$cp->vars['field_default_value_year'] = $now['year'];
							$var = $_POST['field_default_value'] = 'now';
						}
						else
						{
							if (isset($_REQUEST['field_default_value_day']))
							{
								$cp->vars['field_default_value_day'] = request_var('field_default_value_day', 0);
								$cp->vars['field_default_value_month'] = request_var('field_default_value_month', 0);
								$cp->vars['field_default_value_year'] = request_var('field_default_value_year', 0);
								$var = $_POST['field_default_value'] = sprintf('%2d-%2d-%4d', $cp->vars['field_default_value_day'], $cp->vars['field_default_value_month'], $cp->vars['field_default_value_year']);
							}
							else
							{
								list($cp->vars['field_default_value_day'], $cp->vars['field_default_value_month'], $cp->vars['field_default_value_year']) = explode('-', $var);
							}
						}	
					}
					/* else if ($field_type == FIELD_BOOL && $key == 'field_default_value')
					{
						// Get the number of options if this key is 'field_maxlen'
						$var = request_var('field_default_value', 0);
					}*/

					$cp->vars[$key] = $var;
				}

				// step 3 - all arrays
				if ($action == 'edit')
				{
					// Get language entries
					$sql = 'SELECT *
						FROM ' . PROFILE_FIELDS_LANG_TABLE . '
						WHERE lang_id <> ' . $this->edit_lang_id . "
							AND field_id = $field_id
						ORDER BY option_id ASC";
					$result = $db->sql_query($sql);

					$l_lang_options = array();
					while ($row = $db->sql_fetchrow($result))
					{
						$l_lang_options[$row['lang_id']][$row['option_id']] = $row['lang_value'];
					}
					$db->sql_freeresult($result);

		
					$sql = 'SELECT lang_id, lang_name, lang_explain, lang_default_value
						FROM ' . PROFILE_LANG_TABLE . '
						WHERE lang_id <> ' . $this->edit_lang_id . "
							AND field_id = $field_id
						ORDER BY lang_id ASC";
					$result = $db->sql_query($sql);

					$l_lang_name = $l_lang_explain = $l_lang_default_value = array();
					while ($row = $db->sql_fetchrow($result))
					{
						$l_lang_name[$row['lang_id']] = $row['lang_name'];
						$l_lang_explain[$row['lang_id']] = $row['lang_explain'];
						$l_lang_default_value[$row['lang_id']] = $row['lang_default_value'];
					}
					$db->sql_freeresult($result);
				}
		
				foreach ($exclude[3] as $key)
				{
					$cp->vars[$key] = utf8_normalize_nfc(request_var($key, array(0 => ''), true));

					if (!$cp->vars[$key] && $action == 'edit')
					{
						$cp->vars[$key] = $$key;
					}
					else if ($key == 'l_lang_options' && $field_type == FIELD_BOOL)
					{
						$cp->vars[$key] = utf8_normalize_nfc(request_var($key, array(0 => array('')), true));
					}
					else if ($key == 'l_lang_options' && is_array($cp->vars[$key]))
					{
						foreach ($cp->vars[$key] as $lang_id => $options)
						{
							$cp->vars[$key][$lang_id] = explode("\n", $options);
						}
						
					}
				}

				// Check for general issues in every step
				if ($submit) //  && $step == 1
				{
					// Check values for step 1
					if ($cp->vars['field_ident'] == '')
					{
						$error[] = $user->lang['EMPTY_FIELD_IDENT'];
					}

					if (!preg_match('/^[a-z_]+$/', $cp->vars['field_ident']))
					{
						$error[] = $user->lang['INVALID_CHARS_FIELD_IDENT'];
					}

					if (strlen($cp->vars['field_ident']) > 17)
					{
						$error[] = $user->lang['INVALID_FIELD_IDENT_LEN'];
					}

					if ($cp->vars['lang_name'] == '')
					{
						$error[] = $user->lang['EMPTY_USER_FIELD_NAME'];
					}

					if ($field_type == FIELD_DROPDOWN && !sizeof($cp->vars['lang_options']))
					{
						$error[] = $user->lang['NO_FIELD_ENTRIES'];
					}

					if ($field_type == FIELD_BOOL && (empty($cp->vars['lang_options'][0]) || empty($cp->vars['lang_options'][1])))
					{
						$error[] = $user->lang['NO_FIELD_ENTRIES'];
					}

					// Check for already existing field ident
					if ($action != 'edit')
					{
						$sql = 'SELECT field_ident
							FROM ' . PROFILE_FIELDS_TABLE . "
							WHERE field_ident = '" . $db->sql_escape($cp->vars['field_ident']) . "'";
						$result = $db->sql_query($sql);
						$row = $db->sql_fetchrow($result);
						$db->sql_freeresult($result);

						if ($row)
						{
							$error[] = $user->lang['FIELD_IDENT_ALREADY_EXIST'];
						}
					}
				}

				$step = (isset($_REQUEST['next'])) ? $step + 1 : ((isset($_REQUEST['prev'])) ? $step - 1 : $step);

				if (sizeof($error))
				{
					$step--;
					$submit = false;
				}

				// Build up the specific hidden fields
				foreach ($exclude as $num => $key_ary)
				{
					if ($num == $step)
					{
						continue;
					}

					$_new_key_ary = array();

					foreach ($key_ary as $key)
					{
						if ($field_type == FIELD_TEXT && $key == 'field_length' && isset($_REQUEST['rows']))
						{
							$cp->vars['rows'] = request_var('rows', 0);
							$cp->vars['columns'] = request_var('columns', 0);
							$_new_key_ary[$key] = $cp->vars['rows'] . '|' . $cp->vars['columns'];
						}
						else if ($field_type == FIELD_DATE && $key == 'field_default_value')
						{
							$always_now = request_var('always_now', 0);

							if ($always_now)
							{
								$_new_key_ary[$key] = 'now';
							}
							else if (isset($_REQUEST['field_default_value_day']))
							{
								$cp->vars['field_default_value_day'] = request_var('field_default_value_day', 0);
								$cp->vars['field_default_value_month'] = request_var('field_default_value_month', 0);
								$cp->vars['field_default_value_year'] = request_var('field_default_value_year', 0);
								$_new_key_ary[$key]  = sprintf('%2d-%2d-%4d', $cp->vars['field_default_value_day'], $cp->vars['field_default_value_month'], $cp->vars['field_default_value_year']);
							}
						}
						else if ($field_type == FIELD_BOOL && $key == 'l_lang_options' && isset($_REQUEST['l_lang_options']))
						{
							$_new_key_ary[$key] = utf8_normalize_nfc(request_var($key, array(array('')), true));
						}
						else
						{
							if (!isset($_REQUEST[$key]))
							{
								$var = false;
							}
							else if ($key == 'field_ident' && isset($cp->vars[$key]))
							{
								$_new_key_ary[$key]= $cp->vars[$key];
							}
							else
							{
								$_new_key_ary[$key] = (is_array($_REQUEST[$key])) ? utf8_normalize_nfc(request_var($key, array(''), true)) : utf8_normalize_nfc(request_var($key, '', true));
							}
						}
					}

					$s_hidden_fields .= build_hidden_fields($_new_key_ary);
				}

				if (!sizeof($error))
				{
					if ($step == 3 && (sizeof($this->lang_defs['iso']) == 1 || $save))
					{
						$this->save_profile_field($cp, $field_type, $action);
					}
					else if ($action == 'edit' && $save)
					{
						$this->save_profile_field($cp, $field_type, $action);
					}
				}

				$template->assign_vars(array(
					'S_EDIT'			=> true,
					'S_EDIT_MODE'		=> ($action == 'edit') ? true : false,
					'ERROR_MSG'			=> (sizeof($error)) ? implode('<br />', $error) : '',

					'L_TITLE'			=> $user->lang['STEP_' . $step . '_TITLE_' . strtoupper($action)],
					'L_EXPLAIN'			=> $user->lang['STEP_' . $step . '_EXPLAIN_' . strtoupper($action)],
					
					'U_ACTION'			=> $this->u_action . "&amp;action=$action&amp;step=$step",
					'U_BACK'			=> $this->u_action)
				);

				// Now go through the steps
				switch ($step)
				{
					// Create basic options - only small differences between field types
					case 1:
	
						// Build common create options
						$template->assign_vars(array(
							'S_STEP_ONE'		=> true,
							'S_FIELD_REQUIRED'	=> ($cp->vars['field_required']) ? true : false,
							'S_SHOW_ON_REG'		=> ($cp->vars['field_show_on_reg']) ? true : false,
							'S_FIELD_HIDE'		=> ($cp->vars['field_hide']) ? true : false,
							'S_FIELD_NO_VIEW'	=> ($cp->vars['field_no_view']) ? true : false,

							'L_LANG_SPECIFIC'	=> sprintf($user->lang['LANG_SPECIFIC_OPTIONS'], $config['default_lang']),
							'FIELD_TYPE'		=> $user->lang['FIELD_' . strtoupper($cp->profile_types[$field_type])],
							'FIELD_IDENT'		=> $cp->vars['field_ident'],
							'LANG_NAME'			=> $cp->vars['lang_name'],
							'LANG_EXPLAIN'		=> $cp->vars['lang_explain'])
						);

						// String and Text needs to set default values here...
						if ($field_type == FIELD_STRING || $field_type == FIELD_TEXT)
						{
							$template->assign_vars(array(
								'S_TEXT'		=> ($field_type == FIELD_TEXT) ? true : false,
								'S_STRING'		=> ($field_type == FIELD_STRING) ? true : false,

								'L_DEFAULT_VALUE_EXPLAIN'	=> $user->lang[strtoupper($cp->profile_types[$field_type]) . '_DEFAULT_VALUE_EXPLAIN'],
								'LANG_DEFAULT_VALUE'		=> $cp->vars['lang_default_value'])
							);
						}
			
						if ($field_type == FIELD_BOOL || $field_type == FIELD_DROPDOWN)
						{
							// Initialize these array elements if we are creating a new field
							if (!sizeof($cp->vars['lang_options']))
							{
								if ($field_type == FIELD_BOOL)
								{
									// No options have been defined for a boolean field.
									$cp->vars['lang_options'][0] = '';
									$cp->vars['lang_options'][1] = '';
								}
								else
								{
									// No options have been defined for the dropdown menu
									$cp->vars['lang_options'] = array();
								}
							}

							$template->assign_vars(array(
								'S_BOOL'		=> ($field_type == FIELD_BOOL) ? true : false,
								'S_DROPDOWN'	=> ($field_type == FIELD_DROPDOWN) ? true : false,

								'L_LANG_OPTIONS_EXPLAIN'	=> $user->lang[strtoupper($cp->profile_types[$field_type]) . '_ENTRIES_EXPLAIN'],
								'LANG_OPTIONS'				=> ($field_type == FIELD_DROPDOWN) ? implode("\n", $cp->vars['lang_options']) : '',
								'FIRST_LANG_OPTION'			=> ($field_type == FIELD_BOOL) ? $cp->vars['lang_options'][0] : '',
								'SECOND_LANG_OPTION'		=> ($field_type == FIELD_BOOL) ? $cp->vars['lang_options'][1] : '')
							);
						}
					
					break;

					case 2:
						
						$template->assign_vars(array(
							'S_STEP_TWO'		=> true,
							'L_NEXT_STEP'			=> (sizeof($this->lang_defs['iso']) == 1) ? $user->lang['SAVE'] : $user->lang['PROFILE_LANG_OPTIONS'])
						);

						// Build options based on profile type
						$function = 'get_' . $cp->profile_types[$field_type] . '_options';
						$options = $cp->$function();

						foreach ($options as $num => $option_ary)
						{
							$template->assign_block_vars('option', $option_ary);
						}

					break;

					// Define remaining language variables
					case 3:

						$template->assign_var('S_STEP_THREE', true);
						$options = $this->build_language_options($cp, $field_type, $action);

						foreach ($options as $lang_id => $lang_ary)
						{
							$template->assign_block_vars('options', array(
								'LANGUAGE'		=> sprintf($user->lang[(($lang_id == $this->edit_lang_id) ? 'DEFAULT_' : '') . 'ISO_LANGUAGE'], $lang_ary['lang_iso']))
							);

							foreach ($lang_ary['fields'] as $field_ident => $field_ary)
							{
								$template->assign_block_vars('options.field', array(
									'L_TITLE'		=> $field_ary['TITLE'],
									'L_EXPLAIN'		=> (isset($field_ary['EXPLAIN'])) ? $field_ary['EXPLAIN'] : '',
									'FIELD'			=> $field_ary['FIELD'])
								);
							}
						}
		
					break;
				}

				$template->assign_vars(array(
					'S_HIDDEN_FIELDS'	=> $s_hidden_fields)
				);

				return;

			break;
		}
		
		$sql = 'SELECT *
			FROM ' . PROFILE_FIELDS_TABLE . '
			ORDER BY field_order';
		$result = $db->sql_query($sql);

		$s_one_need_edit = false;
		while ($row = $db->sql_fetchrow($result))
		{
			$active_lang = (!$row['field_active']) ? 'ACTIVATE' : 'DEACTIVATE';
			$active_value = (!$row['field_active']) ? 'activate' : 'deactivate';
			$id = $row['field_id'];

			$s_need_edit = (sizeof($this->lang_defs['diff'][$row['field_id']])) ? true : false;

			if ($s_need_edit)
			{
				$s_one_need_edit = true;
			}

			$template->assign_block_vars('fields', array(
				'FIELD_IDENT'		=> $row['field_ident'],
				'FIELD_TYPE'		=> $user->lang['FIELD_' . strtoupper($cp->profile_types[$row['field_type']])],

				'L_ACTIVATE_DEACTIVATE'		=> $user->lang[$active_lang],
				'U_ACTIVATE_DEACTIVATE'		=> $this->u_action . "&amp;action=$active_value&amp;field_id=$id",
				'U_EDIT'					=> $this->u_action . "&amp;action=edit&amp;field_id=$id",
				'U_TRANSLATE'				=> $this->u_action . "&amp;action=edit&amp;field_id=$id&amp;step=3",
				'U_DELETE'					=> $this->u_action . "&amp;action=delete&amp;field_id=$id",
				'U_MOVE_UP'					=> $this->u_action . "&amp;action=move_up&amp;order={$row['field_order']}",
				'U_MOVE_DOWN'				=> $this->u_action . "&amp;action=move_down&amp;order={$row['field_order']}",

				'S_NEED_EDIT'				=> $s_need_edit)
			);
		}
		$db->sql_freeresult($result);

		// At least one option field needs editing?
		if ($s_one_need_edit)
		{
			$template->assign_var('S_NEED_EDIT', true);
		}

		$s_select_type = '';
		foreach ($cp->profile_types as $key => $value)
		{
			$s_select_type .= '<option value="' . $key . '">' . $user->lang['FIELD_' . strtoupper($value)] . '</option>';
		}

		$template->assign_vars(array(
			'U_ACTION'			=> $this->u_action,
			'S_TYPE_OPTIONS'	=> $s_select_type)
		);
	}

	/**
	* Build all Language specific options
	*/
	function build_language_options(&$cp, $field_type, $action = 'create')
	{
		global $user, $config, $db;

		$default_lang_id = (!empty($this->edit_lang_id)) ? $this->edit_lang_id : $this->lang_defs['iso'][$config['default_lang']];

		$sql = 'SELECT lang_id, lang_iso
			FROM ' . LANG_TABLE . '
			WHERE lang_id <> ' . (int) $default_lang_id . '
			ORDER BY lang_english_name';
		$result = $db->sql_query($sql);

		$languages = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$languages[$row['lang_id']] = $row['lang_iso'];
		}
		$db->sql_freeresult($result);
		
		$options = array();
		$options['lang_name'] = 'string';
		if ($cp->vars['lang_explain'])
		{
			$options['lang_explain'] = 'text';
		}
	
		switch ($field_type)
		{
			case FIELD_BOOL:
				$options['lang_options'] = 'two_options';
			break;

			case FIELD_DROPDOWN:
				$options['lang_options'] = 'optionfield';
			break;
			
			case FIELD_TEXT:
			case FIELD_STRING:
				if ($cp->vars['lang_default_value'])
				{
					$options['lang_default_value'] = ($field_type == FIELD_STRING) ? 'string' : 'text';
				}
			break;
		}
	
		$lang_options = array();

		foreach ($options as $field => $field_type)
		{
			$lang_options[1]['lang_iso'] = $this->lang_defs['id'][$default_lang_id];
			$lang_options[1]['fields'][$field] = array(
				'TITLE'		=> $user->lang['CP_' . strtoupper($field)],
				'FIELD'		=> '<dd>' . ((is_array($cp->vars[$field])) ? implode('<br />', $cp->vars[$field]) : bbcode_nl2br($cp->vars[$field])) . '</dd>'
			);

			if (isset($user->lang['CP_' . strtoupper($field) . '_EXPLAIN']))
			{
				$lang_options[1]['fields'][$field]['EXPLAIN'] = $user->lang['CP_' . strtoupper($field) . '_EXPLAIN'];
			}
		}

		foreach ($languages as $lang_id => $lang_iso)
		{
			$lang_options[$lang_id]['lang_iso'] = $lang_iso;
			foreach ($options as $field => $field_type)
			{
				$value = ($action == 'create') ? utf8_normalize_nfc(request_var('l_' . $field, array(0 => ''), true)) : $cp->vars['l_' . $field];
				if ($field == 'lang_options')
				{
					$var = (!isset($cp->vars['l_lang_options'][$lang_id]) || !is_array($cp->vars['l_lang_options'][$lang_id])) ? $cp->vars['lang_options'] : $cp->vars['l_lang_options'][$lang_id];
					
					switch ($field_type)
					{
						case 'two_options':

							$lang_options[$lang_id]['fields'][$field] = array(
								'TITLE'		=> $user->lang['CP_' . strtoupper($field)],
								'FIELD'		=> '
											<dd><input class="medium" name="l_' . $field . '[' . $lang_id . '][]" value="' . ((isset($value[$lang_id][0])) ? $value[$lang_id][0] : $var[0]) . '" /> ' . $user->lang['FIRST_OPTION'] . '</dd>
											<dd><input class="medium" name="l_' . $field . '[' . $lang_id . '][]" value="' . ((isset($value[$lang_id][1])) ? $value[$lang_id][1] : $var[1]) . '" /> ' . $user->lang['SECOND_OPTION'] . '</dd>'
							);
						break;

						case 'optionfield':
							$value = ((isset($value[$lang_id])) ? ((is_array($value[$lang_id])) ?  implode("\n", $value[$lang_id]) : $value[$lang_id]) : implode("\n", $var));
							$lang_options[$lang_id]['fields'][$field] = array(
								'TITLE'		=> $user->lang['CP_' . strtoupper($field)],
								'FIELD'		=> '<dd><textarea name="l_' . $field . '[' . $lang_id . ']" rows="7" cols="80">' . $value . '</textarea></dd>'
							);
						break;
					}
				
					if (isset($user->lang['CP_' . strtoupper($field) . '_EXPLAIN']))
					{
						$lang_options[$lang_id]['fields'][$field]['EXPLAIN'] = $user->lang['CP_' . strtoupper($field) . '_EXPLAIN'];
					}
				}
				else
				{
					$var = ($action == 'create' || !is_array($cp->vars[$field])) ? $cp->vars[$field] : $cp->vars[$field][$lang_id];

					$lang_options[$lang_id]['fields'][$field] = array(
						'TITLE'		=> $user->lang['CP_' . strtoupper($field)],
						'FIELD'		=> ($field_type == 'string') ? '<dd><input class="medium" type="text" name="l_' . $field . '[' . $lang_id . ']" value="' . ((isset($value[$lang_id])) ? $value[$lang_id] : $var) . '" /></dd>' : '<dd><textarea name="l_' . $field . '[' . $lang_id . ']" rows="3" cols="80">' . ((isset($value[$lang_id])) ? $value[$lang_id] : $var) . '</textarea></dd>'
					);
			
					if (isset($user->lang['CP_' . strtoupper($field) . '_EXPLAIN']))
					{
						$lang_options[$lang_id]['fields'][$field]['EXPLAIN'] = $user->lang['CP_' . strtoupper($field) . '_EXPLAIN'];
					}
				}
			}
		}

		return $lang_options;
	}

	/**
	* Save Profile Field
	*/
	function save_profile_field(&$cp, $field_type, $action = 'create')
	{
		global $db, $config, $user;

		$field_id = request_var('field_id', 0);

		// Collect all information, if something is going wrong, abort the operation
		$profile_sql = $profile_lang = $empty_lang = $profile_lang_fields = array();

		$default_lang_id = (!empty($this->edit_lang_id)) ? $this->edit_lang_id : $this->lang_defs['iso'][$config['default_lang']];

		if ($action == 'create')
		{
			$sql = 'SELECT MAX(field_order) as max_field_order
				FROM ' . PROFILE_FIELDS_TABLE;
			$result = $db->sql_query($sql);
			$new_field_order = (int) $db->sql_fetchfield('max_field_order');
			$db->sql_freeresult($result);

			$field_ident = $cp->vars['field_ident'];
		}

		// Save the field
		$profile_fields = array(
			'field_length'			=> $cp->vars['field_length'],
			'field_minlen'			=> $cp->vars['field_minlen'],
			'field_maxlen'			=> $cp->vars['field_maxlen'],
			'field_novalue'			=> $cp->vars['field_novalue'],
			'field_default_value'	=> $cp->vars['field_default_value'],
			'field_validation'		=> $cp->vars['field_validation'],
			'field_required'		=> $cp->vars['field_required'],
			'field_show_on_reg'		=> $cp->vars['field_show_on_reg'],
			'field_hide'			=> $cp->vars['field_hide'],
			'field_no_view'			=> $cp->vars['field_no_view']
		);

		if ($action == 'create')
		{
			$profile_fields += array(
				'field_type'		=> $field_type,
				'field_ident'		=> $field_ident,
				'field_name'		=> $field_ident,
				'field_order'		=> $new_field_order + 1,
				'field_active'		=> 1
			);

			$sql = 'INSERT INTO ' . PROFILE_FIELDS_TABLE . ' ' . $db->sql_build_array('INSERT', $profile_fields);
			$db->sql_query($sql);

			$field_id = $db->sql_nextid();
		}
		else
		{
			$sql = 'UPDATE ' . PROFILE_FIELDS_TABLE . '
				SET ' . $db->sql_build_array('UPDATE', $profile_fields) . "
				WHERE field_id = $field_id";
			$db->sql_query($sql);
		}
		
		if ($action == 'create')
		{
			$field_ident = 'pf_' . $field_ident;
			$profile_sql[] = $this->add_field_ident($field_ident, $field_type);
		}

		$sql_ary = array(
			'lang_name'				=> $cp->vars['lang_name'],
			'lang_explain'			=> $cp->vars['lang_explain'],
			'lang_default_value'	=> $cp->vars['lang_default_value']
		);

		if ($action == 'create')
		{
			$sql_ary['field_id'] = $field_id;
			$sql_ary['lang_id'] = $default_lang_id;
		
			$profile_sql[] = 'INSERT INTO ' . PROFILE_LANG_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary);
		}
		else
		{
			$this->update_insert(PROFILE_LANG_TABLE, $sql_ary, array('field_id' => $field_id, 'lang_id' => $default_lang_id));
		}

		if (is_array($cp->vars['l_lang_name']) && sizeof($cp->vars['l_lang_name']))
		{
			foreach ($cp->vars['l_lang_name'] as $lang_id => $data)
			{
				if (($cp->vars['lang_name'] != '' && $cp->vars['l_lang_name'][$lang_id] == '')
					|| ($cp->vars['lang_explain'] != '' && $cp->vars['l_lang_explain'][$lang_id] == '')
					|| ($cp->vars['lang_default_value'] != '' && $cp->vars['l_lang_default_value'][$lang_id] == ''))
				{
					$empty_lang[$lang_id] = true;
					break;
				}

				if (!isset($empty_lang[$lang_id]))
				{
					$profile_lang[] = array(
						'field_id'		=> $field_id,
						'lang_id'		=> $lang_id,
						'lang_name'		=> $cp->vars['l_lang_name'][$lang_id],
						'lang_explain'	=> (isset($cp->vars['l_lang_explain'][$lang_id])) ? $cp->vars['l_lang_explain'][$lang_id] : '',
						'lang_default_value'	=> (isset($cp->vars['l_lang_default_value'][$lang_id])) ? $cp->vars['l_lang_default_value'][$lang_id] : ''
					);
				}
			}

			foreach ($empty_lang as $lang_id => $NULL)
			{
				$sql = 'DELETE FROM ' . PROFILE_LANG_TABLE . "
					WHERE field_id = $field_id
					AND lang_id = " . (int) $lang_id;
				$db->sql_query($sql);
			}
		}

		// These are always arrays because the key is the language id...
		$cp->vars['l_lang_name']			= utf8_normalize_nfc(request_var('l_lang_name', array(0 => ''), true));
		$cp->vars['l_lang_explain']			= utf8_normalize_nfc(request_var('l_lang_explain', array(0 => ''), true));
		$cp->vars['l_lang_default_value']	= utf8_normalize_nfc(request_var('l_lang_default_value', array(0 => ''), true));

		if ($field_type != FIELD_BOOL)
		{
			$cp->vars['l_lang_options']			= utf8_normalize_nfc(request_var('l_lang_options', array(0 => ''), true));
		}
		else
		{
			/**
			* @todo check if this line is correct...
			$cp->vars['l_lang_default_value']	= request_var('l_lang_default_value', array(0 => array('')), true);
			*/
			$cp->vars['l_lang_options']	= utf8_normalize_nfc(request_var('l_lang_options', array(0 => array('')), true));
		}

		if ($cp->vars['lang_options'])
		{
			if (!is_array($cp->vars['lang_options']))
			{
				$cp->vars['lang_options'] = explode("\n", $cp->vars['lang_options']);
			}

			if ($action != 'create')
			{
				$sql = 'DELETE FROM ' . PROFILE_FIELDS_LANG_TABLE . "
					WHERE field_id = $field_id
						AND lang_id = " . (int) $default_lang_id;
				$db->sql_query($sql);
			}
			
			foreach ($cp->vars['lang_options'] as $option_id => $value)
			{
				$sql_ary = array(
					'field_type'	=> (int) $field_type,
					'lang_value'	=> $value
				);

				if ($action == 'create')
				{
					$sql_ary['field_id'] = $field_id;
					$sql_ary['lang_id'] = $default_lang_id;
					$sql_ary['option_id'] = (int) $option_id;

					$profile_sql[] = 'INSERT INTO ' . PROFILE_FIELDS_LANG_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary);
				}
				else
				{
					$this->update_insert(PROFILE_FIELDS_LANG_TABLE, $sql_ary, array(
						'field_id'	=> $field_id,
						'lang_id'	=> (int) $default_lang_id,
						'option_id'	=> (int) $option_id)
					);
				}
			}
		}

		if (is_array($cp->vars['l_lang_options']) && sizeof($cp->vars['l_lang_options']))
		{
			$empty_lang = array();

			foreach ($cp->vars['l_lang_options'] as $lang_id => $lang_ary)
			{
				if (!is_array($lang_ary))
				{
					$lang_ary = explode("\n", $lang_ary);
				}

				if (sizeof($lang_ary) != sizeof($cp->vars['lang_options']))
				{
					$empty_lang[$lang_id] = true;
				}

				if (!isset($empty_lang[$lang_id]))
				{
					if ($action != 'create')
					{
						$sql = 'DELETE FROM ' . PROFILE_FIELDS_LANG_TABLE . "
							WHERE field_id = $field_id
							AND lang_id = " . (int) $lang_id;
						$db->sql_query($sql);
					}

					foreach ($lang_ary as $option_id => $value)
					{
						$profile_lang_fields[] = array(
							'field_id'		=> (int) $field_id,
							'lang_id'		=> (int) $lang_id,
							'option_id'		=> (int) $option_id,
							'field_type'	=> (int) $field_type,
							'lang_value'	=> $value
						);
					}
				}
			}

			foreach ($empty_lang as $lang_id => $NULL)
			{
				$sql = 'DELETE FROM ' . PROFILE_FIELDS_LANG_TABLE . "
					WHERE field_id = $field_id
					AND lang_id = " . (int) $lang_id;
				$db->sql_query($sql);
			}
		}

		foreach ($profile_lang as $sql)
		{
			if ($action == 'create')
			{
				$profile_sql[] = 'INSERT INTO ' . PROFILE_LANG_TABLE . ' ' . $db->sql_build_array('INSERT', $sql);
			}
			else
			{
				$lang_id = $sql['lang_id'];
				unset($sql['lang_id'], $sql['field_id']);

				$this->update_insert(PROFILE_LANG_TABLE, $sql, array('lang_id' => (int) $lang_id, 'field_id' => $field_id));
			}
		}

		if (sizeof($profile_lang_fields))
		{
			foreach ($profile_lang_fields as $sql)
			{
				if ($action == 'create')
				{
					$profile_sql[] = 'INSERT INTO ' . PROFILE_FIELDS_LANG_TABLE . ' ' . $db->sql_build_array('INSERT', $sql);
				}
				else
				{
					$lang_id = $sql['lang_id'];
					$option_id = $sql['option_id'];
					unset($sql['lang_id'], $sql['field_id'], $sql['option_id']);
					
					$this->update_insert(PROFILE_FIELDS_LANG_TABLE, $sql, array(
						'lang_id'	=> $lang_id,
						'field_id'	=> $field_id,
						'option_id'	=> $option_id)
					);
				}
			}
		}


		$db->sql_transaction('begin');

		if ($action == 'create')
		{
			foreach ($profile_sql as $sql)
			{
				$db->sql_query($sql);
			}
		}
		
		$db->sql_transaction('commit');

		if ($action == 'edit')
		{
			add_log('admin', 'LOG_PROFILE_FIELD_EDIT', $cp->vars['field_ident'] . ':' . $cp->vars['lang_name']);
			trigger_error($user->lang['CHANGED_PROFILE_FIELD'] . adm_back_link($this->u_action));
		}
		else
		{
			add_log('admin', 'LOG_PROFILE_FIELD_CREATE', substr($field_ident, 3) . ':' . $cp->vars['lang_name']);
			trigger_error($user->lang['ADDED_PROFILE_FIELD'] . adm_back_link($this->u_action));
		}
	}

	/**
	* Update, then insert if not successfull
	*/
	function update_insert($table, $sql_ary, $where_fields)
	{
		global $db;

		$where_sql = array();
		$check_key = '';

		foreach ($where_fields as $key => $value)
		{
			$check_key = (!$check_key) ? $key : $check_key;
			$where_sql[] = $key . ' = ' . ((is_string($value)) ? "'" . $db->sql_escape($value) . "'" : (int) $value);
		}

		if (!sizeof($where_sql))
		{
			return;
		}

		$sql = "SELECT $check_key
			FROM $table
			WHERE " . implode(' AND ', $where_sql);
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if (!$row)
		{
			$sql_ary = array_merge($where_fields, $sql_ary);
			
			if (sizeof($sql_ary))
			{
				$db->sql_query("INSERT INTO $table " . $db->sql_build_array('INSERT', $sql_ary));
			}
		}
		else
		{
			if (sizeof($sql_ary))
			{
				$sql = "UPDATE $table SET " . $db->sql_build_array('UPDATE', $sql_ary) . '
					WHERE ' . implode(' AND ', $where_sql);
				$db->sql_query($sql);
			}
		}
	}

	/**
	* Return sql statement for adding a new field ident (profile field) to the profile fields data table
	*/
	function add_field_ident($field_ident, $field_type)
	{
		global $db;

		switch ($db->sql_layer)
		{
			case 'mysql':
			case 'mysql4':
			case 'mysqli':

				// We are defining the biggest common value, because of the possibility to edit the min/max values of each field.
				$sql = 'ALTER TABLE ' . PROFILE_FIELDS_DATA_TABLE . " ADD `$field_ident` ";

				switch ($field_type)
				{
					case FIELD_STRING:
						$sql .= ' VARCHAR(255) ';
					break;

					case FIELD_DATE:
						$sql .= 'VARCHAR(10) ';
					break;

					case FIELD_TEXT:
						$sql .= "TEXT";
		//						ADD {$field_ident}_bbcode_uid VARCHAR(5) NOT NULL,
		//						ADD {$field_ident}_bbcode_bitfield INT(11) UNSIGNED";
					break;

					case FIELD_BOOL:
						$sql .= 'TINYINT(2) ';
					break;
				
					case FIELD_DROPDOWN:
						$sql .= 'MEDIUMINT(8) ';
					break;

					case FIELD_INT:
						$sql .= 'BIGINT(20) ';
					break;
				}

			break;

			case 'sqlite':

				switch ($field_type)
				{
					case FIELD_STRING:
						$type = ' VARCHAR(255) ';
					break;

					case FIELD_DATE:
						$type = 'VARCHAR(10) ';
					break;

					case FIELD_TEXT:
						$type = "TEXT(65535)";
		//						ADD {$field_ident}_bbcode_uid VARCHAR(5) NOT NULL,
		//						ADD {$field_ident}_bbcode_bitfield INT(11) UNSIGNED";
					break;

					case FIELD_BOOL:
						$type = 'TINYINT(2) ';
					break;

					case FIELD_DROPDOWN:
						$type = 'MEDIUMINT(8) ';
					break;

					case FIELD_INT:
						$type = 'BIGINT(20) ';
					break;
				}

				// We are defining the biggest common value, because of the possibility to edit the min/max values of each field.
				if (version_compare(sqlite_libversion(), '3.0') == -1)
				{
					$sql = "SELECT sql
						FROM sqlite_master
						WHERE type = 'table'
							AND name = '" . PROFILE_FIELDS_DATA_TABLE . "'
						ORDER BY type DESC, name;";
					$result = $db->sql_query($sql);
					$row = $db->sql_fetchrow($result);
					$db->sql_freeresult($result);

					// Create a temp table and populate it, destroy the existing one
					$db->sql_query(preg_replace('#CREATE\s+TABLE\s+"?' . PROFILE_FIELDS_DATA_TABLE . '"?#i', 'CREATE TEMPORARY TABLE ' . PROFILE_FIELDS_DATA_TABLE . '_temp', $row['sql']));
					$db->sql_query('INSERT INTO ' . PROFILE_FIELDS_DATA_TABLE . '_temp SELECT * FROM ' . PROFILE_FIELDS_DATA_TABLE);
					$db->sql_query('DROP TABLE ' . PROFILE_FIELDS_DATA_TABLE);

					preg_match('#\((.*)\)#s', $row['sql'], $matches);

					$new_table_cols = trim($matches[1]);
					$old_table_cols = explode(',', $new_table_cols);
					$column_list = array();

					foreach ($old_table_cols as $declaration)
					{
						$entities = preg_split('#\s+#', trim($declaration));
						if ($entities[0] == 'PRIMARY')
						{
							continue;
						}
						$column_list[] = $entities[0];
					}

					$columns = implode(',', $column_list);

					$new_table_cols = $field_ident . ' ' . $type . ',' . $new_table_cols;

					// create a new table and fill it up. destroy the temp one
					$db->sql_query('CREATE TABLE ' . PROFILE_FIELDS_DATA_TABLE . ' (' . $new_table_cols . ');');
					$db->sql_query('INSERT INTO ' . PROFILE_FIELDS_DATA_TABLE . ' (' . $columns . ') SELECT ' . $columns . ' FROM ' . PROFILE_FIELDS_DATA_TABLE . '_temp;');
					$db->sql_query('DROP TABLE ' . PROFILE_FIELDS_DATA_TABLE . '_temp');
				}
				else
				{
					$sql = 'ALTER TABLE ' . PROFILE_FIELDS_DATA_TABLE . " ADD $field_ident [$type]";
				}

			break;

			case 'mssql':
			case 'mssql_odbc':

				// We are defining the biggest common value, because of the possibility to edit the min/max values of each field.
				$sql = 'ALTER TABLE [' . PROFILE_FIELDS_DATA_TABLE . "] ADD [$field_ident] ";

				switch ($field_type)
				{
					case FIELD_STRING:
						$sql .= ' [VARCHAR] (255) ';
					break;

					case FIELD_DATE:
						$sql .= '[VARCHAR] (10) ';
					break;

					case FIELD_TEXT:
						$sql .= "[TEXT]";
		//						ADD {$field_ident}_bbcode_uid [VARCHAR] (5) NOT NULL,
		//						ADD {$field_ident}_bbcode_bitfield [INT] UNSIGNED";
					break;

					case FIELD_BOOL:
					case FIELD_DROPDOWN:
						$sql .= '[INT] ';
					break;

					case FIELD_INT:
						$sql .= '[FLOAT] ';
					break;
				}

			break;

			case 'postgres':

				// We are defining the biggest common value, because of the possibility to edit the min/max values of each field.
				$sql = 'ALTER TABLE ' . PROFILE_FIELDS_DATA_TABLE . " ADD COLUMN \"$field_ident\" ";

				switch ($field_type)
				{
					case FIELD_STRING:
						$sql .= ' VARCHAR(255) ';
					break;

					case FIELD_DATE:
						$sql .= 'VARCHAR(10) ';
					break;

					case FIELD_TEXT:
						$sql .= "TEXT";
		//						ADD {$field_ident}_bbcode_uid VARCHAR(5) NOT NULL,
		//						ADD {$field_ident}_bbcode_bitfield INT4 UNSIGNED";
					break;

					case FIELD_BOOL:
						$sql .= 'INT2 ';
					break;

					case FIELD_DROPDOWN:
						$sql .= 'INT4 ';
					break;

					case FIELD_INT:
						$sql .= 'INT8 ';
					break;
				}

			break;

			case 'firebird':

				// We are defining the biggest common value, because of the possibility to edit the min/max values of each field.
				$sql = 'ALTER TABLE ' . PROFILE_FIELDS_DATA_TABLE . " ADD \"$field_ident\" ";

				switch ($field_type)
				{
					case FIELD_STRING:
						$sql .= ' VARCHAR(255) ';
					break;

					case FIELD_DATE:
						$sql .= 'VARCHAR(10) ';
					break;

					case FIELD_TEXT:
						$sql .= "BLOB SUB_TYPE TEXT";
		//						ADD {$field_ident}_bbcode_uid VARCHAR(5) NOT NULL,
		//						ADD {$field_ident}_bbcode_bitfield INTEGER UNSIGNED";
					break;

					case FIELD_BOOL:
					case FIELD_DROPDOWN:
						$sql .= 'INTEGER ';
					break;

					case FIELD_INT:
						$sql .= 'DOUBLE PRECISION ';
					break;
				}

			break;

			case 'oracle':

				// We are defining the biggest common value, because of the possibility to edit the min/max values of each field.
				$sql = 'ALTER TABLE ' . PROFILE_FIELDS_DATA_TABLE . " ADD $field_ident ";

				switch ($field_type)
				{
					case FIELD_STRING:
						$sql .= ' VARCHAR2(255) ';
					break;

					case FIELD_DATE:
						$sql .= 'VARCHAR2(10) ';
					break;

					case FIELD_TEXT:
						$sql .= "CLOB";
		//						ADD {$field_ident}_bbcode_uid VARCHAR2(5) NOT NULL,
		//						ADD {$field_ident}_bbcode_bitfield NUMBER(11) UNSIGNED";
					break;

					case FIELD_BOOL:
						$sql .= 'NUMBER(2) ';
					break;

					case FIELD_DROPDOWN:
						$sql .= 'NUMBER(8) ';
					break;

					case FIELD_INT:
						$sql .= 'NUMBER(20) ';
					break;
				}

			break;
		}

		return $sql;
	}
}

?>