<?php
/**
*
* @package acp
* @version $Id: acp_ranks.php 8479 2008-03-29 00:22:48Z naderman $
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
class acp_ranks
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
		$action = (isset($_POST['save'])) ? 'save' : $action;
		$rank_id = request_var('id', 0);

		$this->tpl_name = 'acp_ranks';
		$this->page_title = 'ACP_MANAGE_RANKS';

		$form_name = 'acp_prune';
		add_form_key($form_name);

		switch ($action)
		{
			case 'save':

				if (!check_form_key($form_name))
				{
					trigger_error($user->lang['FORM_INVALID']. adm_back_link($this->u_action), E_USER_WARNING);
				}
				$rank_title = utf8_normalize_nfc(request_var('title', '', true));
				$special_rank = request_var('special_rank', 0);
				$min_posts = ($special_rank) ? 0 : request_var('min_posts', 0);
				$rank_image = request_var('rank_image', '');

				// The rank image has to be a jpg, gif or png
				if ($rank_image != '' && !preg_match('#(\.gif|\.png|\.jpg|\.jpeg)$#i', $rank_image))
				{
					$rank_image = '';
				}

				if (!$rank_title)
				{
					trigger_error($user->lang['NO_RANK_TITLE'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				$sql_ary = array(
					'rank_title'		=> $rank_title,
					'rank_special'		=> $special_rank,
					'rank_min'			=> $min_posts,
					'rank_image'		=> htmlspecialchars_decode($rank_image)
				);
				
				if ($rank_id)
				{
					$sql = 'UPDATE ' . RANKS_TABLE . ' SET ' . $db->sql_build_array('UPDATE', $sql_ary) . " WHERE rank_id = $rank_id";
					$message = $user->lang['RANK_UPDATED'];

					add_log('admin', 'LOG_RANK_UPDATED', $rank_title);
				}
				else
				{
					$sql = 'INSERT INTO ' . RANKS_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary);
					$message = $user->lang['RANK_ADDED'];

					add_log('admin', 'LOG_RANK_ADDED', $rank_title);
				}
				$db->sql_query($sql);

				$cache->destroy('_ranks');

				trigger_error($message . adm_back_link($this->u_action));

			break;

			case 'delete':

				if (!$rank_id)
				{
					trigger_error($user->lang['MUST_SELECT_RANK'] . adm_back_link($this->u_action), E_USER_WARNING);
				}

				if (confirm_box(true))
				{
					$sql = 'SELECT rank_title
						FROM ' . RANKS_TABLE . '
						WHERE rank_id = ' . $rank_id;
					$result = $db->sql_query($sql);
					$rank_title = (string) $db->sql_fetchfield('rank_title');
					$db->sql_freeresult($result);

					$sql = 'DELETE FROM ' . RANKS_TABLE . "
						WHERE rank_id = $rank_id";
					$db->sql_query($sql);

					$sql = 'UPDATE ' . USERS_TABLE . "
						SET user_rank = 0
						WHERE user_rank = $rank_id";
					$db->sql_query($sql);

					$cache->destroy('_ranks');

					add_log('admin', 'LOG_RANK_REMOVED', $rank_title);
				}
				else
				{
					confirm_box(false, $user->lang['CONFIRM_OPERATION'], build_hidden_fields(array(
						'i'			=> $id,
						'mode'		=> $mode,
						'rank_id'	=> $rank_id,
						'action'	=> 'delete',
					)));
				}

			break;

			case 'edit':
			case 'add':

				$data = $ranks = $existing_imgs = array();
				
				$sql = 'SELECT *
					FROM ' . RANKS_TABLE . '
					ORDER BY rank_min ASC, rank_special ASC';
				$result = $db->sql_query($sql);

				while ($row = $db->sql_fetchrow($result))
				{
					$existing_imgs[] = $row['rank_image'];

					if ($action == 'edit' && $rank_id == $row['rank_id'])
					{
						$ranks = $row;
					}
				}
				$db->sql_freeresult($result);

				$imglist = filelist($phpbb_root_path . $config['ranks_path'], '');
				$edit_img = $filename_list = '';

				foreach ($imglist as $path => $img_ary)
				{
					sort($img_ary);

					foreach ($img_ary as $img)
					{
						$img = $path . $img;

						if (!in_array($img, $existing_imgs) || $action == 'edit')
						{
							if ($ranks && $img == $ranks['rank_image'])
							{
								$selected = ' selected="selected"';
								$edit_img = $img;
							}
							else
							{
								$selected = '';
							}

							if (strlen($img) > 255)
							{
								continue;
							}

							$filename_list .= '<option value="' . htmlspecialchars($img) . '"' . $selected . '>' . $img . '</option>';
						}
					}
				}

				$filename_list = '<option value=""' . (($edit_img == '') ? ' selected="selected"' : '') . '>----------</option>' . $filename_list;
				unset($existing_imgs, $imglist);

				$template->assign_vars(array(
					'S_EDIT'			=> true,
					'U_BACK'			=> $this->u_action,
					'RANKS_PATH'		=> $phpbb_root_path . $config['ranks_path'],
					'U_ACTION'			=> $this->u_action . '&amp;id=' . $rank_id,

					'RANK_TITLE'		=> (isset($ranks['rank_title'])) ? $ranks['rank_title'] : '',
					'S_FILENAME_LIST'	=> $filename_list,
					'RANK_IMAGE'		=> ($edit_img) ? $phpbb_root_path . $config['ranks_path'] . '/' . $edit_img : $phpbb_admin_path . 'images/spacer.gif',
					'S_SPECIAL_RANK'	=> (!isset($ranks['rank_special']) || $ranks['rank_special']) ? true : false,
					'MIN_POSTS'			=> (isset($ranks['rank_min']) && !$ranks['rank_special']) ? $ranks['rank_min'] : 0)
				);
						

				return;

			break;
		}
	
		$template->assign_vars(array(
			'U_ACTION'		=> $this->u_action)
		);

		$sql = 'SELECT *
			FROM ' . RANKS_TABLE . '
			ORDER BY rank_special DESC, rank_min ASC, rank_title ASC';
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$template->assign_block_vars('ranks', array(
				'S_RANK_IMAGE'		=> ($row['rank_image']) ? true : false,
				'S_SPECIAL_RANK'	=> ($row['rank_special']) ? true : false,

				'RANK_IMAGE'		=> $phpbb_root_path . $config['ranks_path'] . '/' . $row['rank_image'],
				'RANK_TITLE'		=> $row['rank_title'],
				'MIN_POSTS'			=> $row['rank_min'],

				'U_EDIT'			=> $this->u_action . '&amp;action=edit&amp;id=' . $row['rank_id'],
				'U_DELETE'			=> $this->u_action . '&amp;action=delete&amp;id=' . $row['rank_id'])
			);	
		}
		$db->sql_freeresult($result);

	}
}

?>