<?php
/**
*
* @package phpBB3
* @version $Id: swatch.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @ignore
*/
define('IN_PHPBB', true);
define('ADMIN_START', true);
$phpbb_root_path = (defined('PHPBB_ROOT_PATH')) ? PHPBB_ROOT_PATH : './../';
$phpEx = substr(strrchr(__FILE__, '.'), 1);
include($phpbb_root_path . 'common.' . $phpEx);

// Start session management
$user->session_begin(false);
$auth->acl($user->data);
$user->setup();

// Set custom template for admin area
$template->set_custom_template($phpbb_root_path . 'adm/style', 'admin');

$template->set_filenames(array(
	'body' => 'colour_swatch.html')
);

$form = request_var('form', '');
$name = request_var('name', '');

// We validate form and name here, only id/class allowed
$form = (!preg_match('/^[a-z0-9_-]+$/i', $form)) ? '' : $form;
$name = (!preg_match('/^[a-z0-9_-]+$/i', $name)) ? '' : $name;

$template->assign_vars(array(
	'OPENER'		=> $form,
	'NAME'			=> $name,
	'T_IMAGES_PATH'	=> "{$phpbb_root_path}images/",

	'S_USER_LANG'			=> $user->lang['USER_LANG'],
	'S_CONTENT_DIRECTION'	=> $user->lang['DIRECTION'],
	'S_CONTENT_ENCODING'	=> 'UTF-8',
));

$template->display('body');

garbage_collection();

?>