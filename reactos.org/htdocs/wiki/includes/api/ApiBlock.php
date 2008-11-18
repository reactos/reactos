<?php

/*
 * Created on Sep 4, 2007
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
* API module that facilitates the blocking of users. Requires API write mode
* to be enabled.
*
 * @ingroup API
 */
class ApiBlock extends ApiBase {

	/**
	 * Std ctor.
	 */
	public function __construct($main, $action) {
		parent :: __construct($main, $action);
	}

	/**
	 * Blocks the user specified in the parameters for the given expiry, with the
	 * given reason, and with all other settings provided in the params. If the block
	 * succeeds, produces a result containing the details of the block and notice
	 * of success. If it fails, the result will specify the nature of the error.
	 */
	public function execute() {
		global $wgUser;
		$this->getMain()->requestWriteMode();
		$params = $this->extractRequestParams();

		if($params['gettoken'])
		{
			$res['blocktoken'] = $wgUser->editToken();
			$this->getResult()->addValue(null, $this->getModuleName(), $res);
			return;
		}

		if(is_null($params['user']))
			$this->dieUsageMsg(array('missingparam', 'user'));
		if(is_null($params['token']))
			$this->dieUsageMsg(array('missingparam', 'token'));
		if(!$wgUser->matchEditToken($params['token']))
			$this->dieUsageMsg(array('sessionfailure'));
		if(!$wgUser->isAllowed('block'))
			$this->dieUsageMsg(array('cantblock'));
		if($params['hidename'] && !$wgUser->isAllowed('hideuser'))
			$this->dieUsageMsg(array('canthide'));
		if($params['noemail'] && !$wgUser->isAllowed('blockemail'))
			$this->dieUsageMsg(array('cantblock-email'));
		if(wfReadOnly())
			$this->dieUsageMsg(array('readonlytext'));

		$form = new IPBlockForm('');
		$form->BlockAddress = $params['user'];
		$form->BlockReason = (is_null($params['reason']) ? '' : $params['reason']);
		$form->BlockReasonList = 'other';
		$form->BlockExpiry = ($params['expiry'] == 'never' ? 'infinite' : $params['expiry']);
		$form->BlockOther = '';
		$form->BlockAnonOnly = $params['anononly'];
		$form->BlockCreateAccount = $params['nocreate'];
		$form->BlockEnableAutoBlock = $params['autoblock'];
		$form->BlockEmail = $params['noemail'];
		$form->BlockHideName = $params['hidename'];

		$userID = $expiry = null;
		$retval = $form->doBlock($userID, $expiry);
		if(!empty($retval))
			// We don't care about multiple errors, just report one of them
			$this->dieUsageMsg($retval);

		$res['user'] = $params['user'];
		$res['userID'] = $userID;
		$res['expiry'] = ($expiry == Block::infinity() ? 'infinite' : wfTimestamp(TS_ISO_8601, $expiry));
		$res['reason'] = $params['reason'];
		if($params['anononly'])
			$res['anononly'] = '';
		if($params['nocreate'])
			$res['nocreate'] = '';
		if($params['autoblock'])
			$res['autoblock'] = '';
		if($params['noemail'])
			$res['noemail'] = '';
		if($params['hidename'])
			$res['hidename'] = '';

		$this->getResult()->addValue(null, $this->getModuleName(), $res);
	}

	public function mustBePosted() { return true; }

	public function getAllowedParams() {
		return array (
			'user' => null,
			'token' => null,
			'gettoken' => false,
			'expiry' => 'never',
			'reason' => null,
			'anononly' => false,
			'nocreate' => false,
			'autoblock' => false,
			'noemail' => false,
			'hidename' => false,
		);
	}

	public function getParamDescription() {
		return array (
			'user' => 'Username, IP address or IP range you want to block',
			'token' => 'A block token previously obtained through the gettoken parameter',
			'gettoken' => 'If set, a block token will be returned, and no other action will be taken',
			'expiry' => 'Relative expiry time, e.g. \'5 months\' or \'2 weeks\'. If set to \'infinite\', \'indefinite\' or \'never\', the block will never expire.',
			'reason' => 'Reason for block (optional)',
			'anononly' => 'Block anonymous users only (i.e. disable anonymous edits for this IP)',
			'nocreate' => 'Prevent account creation',
			'autoblock' => 'Automatically block the last used IP address, and any subsequent IP addresses they try to login from',
			'noemail' => 'Prevent user from sending e-mail through the wiki. (Requires the "blockemail" right.)',
			'hidename' => 'Hide the username from the block log. (Requires the "hideuser" right.)'
		);
	}

	public function getDescription() {
		return array(
			'Block a user.'
		);
	}

	protected function getExamples() {
		return array (
			'api.php?action=block&user=123.5.5.12&expiry=3%20days&reason=First%20strike',
			'api.php?action=block&user=Vandal&expiry=never&reason=Vandalism&nocreate&autoblock&noemail'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiBlock.php 35388 2008-05-27 10:18:28Z catrope $';
	}
}
