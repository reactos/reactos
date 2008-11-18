<?php
    /*
    RSDB - ReactOS Support Database
    Copyright (C) 2005-2006  Klemens Friedl <frik85@reactos.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    */

/*
 *	ReactOS Support Database System - RSDB
 *	
 *	(c) by Klemens Friedl <frik85>
 *	
 *	2005 - 2006 
 */


	// To prevent hacking activity:
	if ( !defined('ROST') )
	{
		die(" ");
	}


	// Language detection
	function check_lang($lang)
	{
		if ($lang) {
			if (@preg_match('/^([a-zA-Z]+)(-[a-zA-Z]+)?$/', $lang, $matches)) {
				$checked_lang = @strtolower($matches[1]);
				switch($checked_lang) {
				case 'de':
				case 'en':
				case 'fr':
				case 'ru':
					break;
				default:
					$checked_lang = '';
				}
			}
			else if ($lang == '*') {
				$checked_lang = 'en';
			}
			else {
				$checked_lang = '';
			}
		}
		else {
			$checked_lang = '';
		}
	
		return $checked_lang;
	}
	
	if ($ROST_SET_lang == '' && isset($_COOKIE['roscms_usrset_lang'])) {
		$ROST_SET_lang = $_COOKIE['roscms_usrset_lang'];
		if (substr($ROST_SET_lang, -1) == '/') {
			$ROST_SET_lang = substr($ROST_SET_lang, strlen($ROST_SET_lang) - 1);
		}
		$ROST_SET_lang = check_lang($ROST_SET_lang);
	}
	
	if ($ROST_SET_lang == '') {
		/* After parameter and cookie processing, we still don't have a valid
			   language. So check whether the HTTP Accept-language header can
			   help us. */
		$accept_language = $_SERVER['HTTP_ACCEPT_LANGUAGE'];
		$best_q = 0;
		while (@preg_match('/^\s*([^,]+)((,(.*))|$)/',
						  $accept_language, $matches)) {
			$lang_range = @$matches[1];
			$accept_language = @$matches[4];
			if (@preg_match('/^(([a-zA-Z]+)(-[a-zA-Z]+)?)(;q=([0-1](\.[0-9]{1,3})?))?/',
						   $lang_range, $matches)) {
				$lang = check_lang($matches[1]);
				if ($lang != '') {
					$q = @$matches[5];
					if ($q == "") {
						$q = 1;
					}
					else {
						settype($q, 'float');
					}
					if ($best_q < $q) {
						$ROST_SET_lang = $lang;
						$best_q = $q;
					}
				}
			}
		}
	}
	if ($ROST_SET_lang == '') {
		/* If all else fails, use the default language */
		$ROST_SET_lang = check_lang('*');
	}
	
	$roscms_page_lang = $ROST_SET_lang . '/';
	$ROST_SET_lang_session = $ROST_SET_lang . '/';
	
	
	if (isset($_COOKIE['roscms_usrset_lang']) || isset($_REQUEST['lang'])) {
		/* Delete an existing cookie (if any) which uses the full hostname */
		setcookie('roscms_usrset_lang', '', -3600);
		/* Add cookie using just the domain name */
		require_once('inc/utils.php');
		setcookie('roscms_usrset_lang', $ROST_SET_lang, time() + 5 * 30 * 24 * 3600,
				  '/', cookie_domain());
	}
		
/* This HACK is only to for the first few weeks; when more language files are available, simply delete the following line */
	$ROST_SET_lang="en";
	
	
	require("inc/lang/".$ROST_SET_lang.".php"); // load the language text for 'ROST'

	//ini_set ('session.name', 'roscms');

?>