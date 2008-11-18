<?php

/*
 * Created on Sep 7, 2007
 * API for MediaWiki 1.8+
 *
 * Copyright (C) 2007 Roan Kattouw <Firstname>.<Lastname>@home.nl
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
	require_once ("ApiBase.php");
}

/**
 * API module that facilitates the unblocking of users. Requires API write mode
 * to be enabled.
 *
 * @ingroup API
 */
class ApiUnblock extends ApiBase {

	public function __construct($main, $action) {
		parent :: __construct($main, $action);
	}

	/**
	 * Unblocks the specified user or provides the reason the unblock failed.
	 */
	public function execute() {
		global $wgUser;
		$this->getMain()->requestWriteMode();
		$params = $this->extractRequestParams();

		if($params['gettoken'])
		{
			$res['unblocktoken'] = $wgUser->editToken();
			$this->getResult()->addValue(null, $this->getModuleName(), $res);
			return;
		}

		if(is_null($params['id']) && is_null($params['user']))
			$this->dieUsageMsg(array('unblock-notarget'));
		if(!is_null($params['id']) && !is_null($params['user']))
			$this->dieUsageMsg(array('unblock-idanduser'));
		if(is_null($params['token']))
			$this->dieUsageMsg(array('missingparam', 'token'));
		if(!$wgUser->matchEditToken($params['token']))
			$this->dieUsageMsg(array('sessionfailure'));
		if(!$wgUser->isAllowed('block'))
			$this->dieUsageMsg(array('cantunblock'));
		if(wfReadOnly())
			$this->dieUsageMsg(array('readonlytext'));

		$id = $params['id'];
		$user = $params['user'];
		$reason = (is_null($params['reason']) ? '' : $params['reason']);
		$retval = IPUnblockForm::doUnblock($id, $user, $reason, $range);
		if(!empty($retval))
			$this->dieUsageMsg($retval);

		$res['id'] = $id;
		$res['user'] = $user;
		$res['reason'] = $reason;
		$this->getResult()->addValue(null, $this->getModuleName(), $res);
	}

	public function mustBePosted() { return true; }

	public function getAllowedParams() {
		return array (
			'id' => null,
			'user' => null,
			'token' => null,
			'gettoken' => false,
			'reason' => null,
		);
	}

	public function getParamDescription() {
		return array (
			'id' => 'ID of the block you want to unblock (obtained through list=blocks). Cannot be used together with user',
			'user' => 'Username, IP address or IP range you want to unblock. Cannot be used together with id',
			'token' => 'An unblock token previously obtained through the gettoken parameter',
			'gettoken' => 'If set, an unblock token will be returned, and no other action will be taken',
			'reason' => 'Reason for unblock (optional)',
		);
	}

	public function getDescription() {
		return array(
			'Unblock a user.'
		);
	}

	protected function getExamples() {
		return array (
			'api.php?action=unblock&id=105',
			'api.php?action=unblock&user=Bob&reason=Sorry%20Bob'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiUnblock.php 35098 2008-05-20 17:13:28Z ialex $';
	}
}
