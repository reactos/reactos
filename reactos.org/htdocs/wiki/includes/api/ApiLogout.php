<?php

/*
 * Created on Jan 4, 2008
 *
 * API for MediaWiki 1.8+
 *
 * Copyright (C) 2008 Yuri Astrakhan <Firstname><Lastname>@gmail.com,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * http://www.gnu.org/copyleft/gpl.html
 */

if (!defined('MEDIAWIKI')) {
	// Eclipse helper - will be ignored in production
	require_once ('ApiBase.php');
}

/**
 * API module to allow users to log out of the wiki. API equivalent of
 * Special:Userlogout.
 *
 * @ingroup API
 */
class ApiLogout extends ApiBase {

	public function __construct($main, $action) {
		parent :: __construct($main, $action);
	}

	public function execute() {
		global $wgUser;
		$wgUser->logout();
		
		// Give extensions to do something after user logout
		$injected_html = '';
		wfRunHooks( 'UserLogoutComplete', array(&$wgUser, &$injected_html) );
	}

	public function getAllowedParams() {
		return array ();
	}

	public function getParamDescription() {
		return array ();
	}

	public function getDescription() {
		return array (
			'This module is used to logout and clear session data'
		);
	}

	protected function getExamples() {
		return array(
			'api.php?action=logout'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiLogout.php 35294 2008-05-24 20:44:49Z btongminh $';
	}
}
