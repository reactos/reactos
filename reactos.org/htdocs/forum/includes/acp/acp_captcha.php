<?php
/**
*
* @package acp
* @version $Id: acp_captcha.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
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
class acp_captcha
{
	var $u_action;

	function main($id, $mode)
	{
		global $db, $user, $auth, $template;
		global $config, $phpbb_root_path, $phpbb_admin_path, $phpEx;

		$user->add_lang('acp/board');

		
		$captcha_vars = array(
			'captcha_gd_x_grid'				=> 'CAPTCHA_GD_X_GRID',
			'captcha_gd_y_grid'				=> 'CAPTCHA_GD_Y_GRID',
			'captcha_gd_foreground_noise'	=> 'CAPTCHA_GD_FOREGROUND_NOISE',
			'captcha_gd'					=> 'CAPTCHA_GD_PREVIEWED'
		);

		if (isset($_GET['demo']))
		{
			$captcha_vars = array_keys($captcha_vars);
			foreach ($captcha_vars as $captcha_var)
			{
				$config[$captcha_var] = (isset($_REQUEST[$captcha_var])) ? request_var($captcha_var, 0) : $config[$captcha_var];
			}
			if ($config['captcha_gd'])
			{
				include($phpbb_root_path . 'includes/captcha/captcha_gd.' . $phpEx);
			}
			else
			{
				include($phpbb_root_path . 'includes/captcha/captcha_non_gd.' . $phpEx);
			}
			$captcha = new captcha();
			$captcha->execute(gen_rand_string(mt_rand(5, 8)), time());
			exit_handler();
		}

		$config_vars = array(
			'enable_confirm'		=> 'REG_ENABLE',
			'enable_post_confirm'	=> 'POST_ENABLE',
			'captcha_gd'			=> 'CAPTCHA_GD',
		);

		$this->tpl_name = 'acp_captcha';
		$this->page_title = 'ACP_VC_SETTINGS';
		$form_key = 'acp_captcha';
		add_form_key($form_key);

		$submit = request_var('submit', '');

		if ($submit && check_form_key($form_key))
		{
			$config_vars = array_keys($config_vars);
			foreach ($config_vars as $config_var)
			{
				set_config($config_var, request_var($config_var, ''));
			}
			$captcha_vars = array_keys($captcha_vars);
			foreach ($captcha_vars as $captcha_var)
			{
				set_config($captcha_var, request_var($captcha_var, 0));
			}
			trigger_error($user->lang['CONFIG_UPDATED'] . adm_back_link($this->u_action));
		}
		else if ($submit)
		{
				trigger_error($user->lang['FORM_INVALID'] . adm_back_link($this->u_action));
		}
		else
		{
			
			$preview_image_src = append_sid(append_sid("{$phpbb_admin_path}index.$phpEx", "i=$id&amp;demo=demo"));
			if (@extension_loaded('gd'))
			{
				$template->assign_var('GD', true);
			}
			foreach ($config_vars as $config_var => $template_var)
			{
				$template->assign_var($template_var, (isset($_REQUEST[$config_var])) ? request_var($config_var, '') : $config[$config_var]) ;
			}
			foreach ($captcha_vars as $captcha_var => $template_var)
			{
				$var = (isset($_REQUEST[$captcha_var])) ? request_var($captcha_var, 0) : $config[$captcha_var];
				$template->assign_var($template_var, $var);
				$preview_image_src .= "&amp;$captcha_var=" . $var;
			}
			$template->assign_vars(array(
				'CAPTCHA_PREVIEW'	=> $preview_image_src,
				'PREVIEW'			=> isset($_POST['preview']),
			));
			
		}
	}
}

?>