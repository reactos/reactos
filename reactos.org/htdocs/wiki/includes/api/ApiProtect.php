<?php

/*
 * Created on Sep 1, 2007
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
 * @ingroup API
 */
class ApiProtect extends ApiBase {

	public function __construct($main, $action) {
		parent :: __construct($main, $action);
	}

	public function execute() {
		global $wgUser;
		$this->getMain()->requestWriteMode();
		$params = $this->extractRequestParams();

		$titleObj = NULL;
		if(!isset($params['title']))
			$this->dieUsageMsg(array('missingparam', 'title'));
		if(!isset($params['token']))
			$this->dieUsageMsg(array('missingparam', 'token'));
		if(!isset($params['protections']) || empty($params['protections']))
			$this->dieUsageMsg(array('missingparam', 'protections'));

		if(!$wgUser->matchEditToken($params['token']))
			$this->dieUsageMsg(array('sessionfailure'));

		$titleObj = Title::newFromText($params['title']);
		if(!$titleObj)
			$this->dieUsageMsg(array('invalidtitle', $params['title']));

		$errors = $titleObj->getUserPermissionsErrors('protect', $wgUser);
		if(!empty($errors))
			// We don't care about multiple errors, just report one of them
			$this->dieUsageMsg(current($errors));

		if(in_array($params['expiry'], array('infinite', 'indefinite', 'never')))
			$expiry = Block::infinity();
		else
		{
			$expiry = strtotime($params['expiry']);
			if($expiry < 0 || $expiry == false)
				$this->dieUsageMsg(array('invalidexpiry'));

			$expiry = wfTimestamp(TS_MW, $expiry);
			if($expiry < wfTimestampNow())
				$this->dieUsageMsg(array('pastexpiry'));
		}

		$protections = array();
		foreach($params['protections'] as $prot)
		{
			$p = explode('=', $prot);
			$protections[$p[0]] = ($p[1] == 'all' ? '' : $p[1]);
			if($titleObj->exists() && $p[0] == 'create')
				$this->dieUsageMsg(array('create-titleexists'));
			if(!$titleObj->exists() && $p[0] != 'create')
				$this->dieUsageMsg(array('missingtitles-createonly'));
		}

		if($titleObj->exists()) {
			$articleObj = new Article($titleObj);
			$ok = $articleObj->updateRestrictions($protections, $params['reason'], $params['cascade'], $expiry);
		} else
			$ok = $titleObj->updateTitleProtection($protections['create'], $params['reason'], $expiry);
		if(!$ok)
			// This is very weird. Maybe the article was deleted or the user was blocked/desysopped in the meantime?
			// Just throw an unknown error in this case, as it's very likely to be a race condition
			$this->dieUsageMsg(array());
		$res = array('title' => $titleObj->getPrefixedText(), 'reason' => $params['reason']);
		if($expiry == Block::infinity())
			$res['expiry'] = 'infinity';
		else
			$res['expiry'] = wfTimestamp(TS_ISO_8601, $expiry);

		if($params['cascade'])
			$res['cascade'] = '';
		$res['protections'] = $protections;
		$this->getResult()->addValue(null, $this->getModuleName(), $res);
	}

	public function mustBePosted() { return true; }

	public function getAllowedParams() {
		return array (
			'title' => null,
			'token' => null,
			'protections' => array(
				ApiBase :: PARAM_ISMULTI => true
			),
			'expiry' => 'infinite',
			'reason' => '',
			'cascade' => false
		);
	}

	public function getParamDescription() {
		return array (
			'title' => 'Title of the page you want to restore.',
			'token' => 'A protect token previously retrieved through prop=info',
			'protections' => 'Pipe-separated list of protection levels, formatted action=group (e.g. edit=sysop)',
			'expiry' => 'Expiry timestamp. If set to \'infinite\', \'indefinite\' or \'never\', the protection will never expire.',
			'reason' => 'Reason for (un)protecting (optional)',
			'cascade' => 'Enable cascading protection (i.e. protect pages included in this page)'
		);
	}

	public function getDescription() {
		return array(
			'Change the protection level of a page.'
		);
	}

	protected function getExamples() {
		return array (
			'api.php?action=protect&title=Main%20Page&token=123ABC&protections=edit=sysop|move=sysop&cascade&expiry=20070901163000',
			'api.php?action=protect&title=Main%20Page&token=123ABC&protections=edit=all|move=all&reason=Lifting%20restrictions'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiProtect.php 35098 2008-05-20 17:13:28Z ialex $';
	}
}
