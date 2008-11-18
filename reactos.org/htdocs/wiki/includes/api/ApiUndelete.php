<?php

/*
 * Created on Jul 3, 2007
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
class ApiUndelete extends ApiBase {

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

		if(!$wgUser->isAllowed('undelete'))
			$this->dieUsageMsg(array('permdenied-undelete'));
		if($wgUser->isBlocked())
			$this->dieUsageMsg(array('blockedtext'));
		if(wfReadOnly())
			$this->dieUsageMsg(array('readonlytext'));
		if(!$wgUser->matchEditToken($params['token']))
			$this->dieUsageMsg(array('sessionfailure'));

		$titleObj = Title::newFromText($params['title']);
		if(!$titleObj)
			$this->dieUsageMsg(array('invalidtitle', $params['title']));

		// Convert timestamps
		if(!isset($params['timestamps']))
			$params['timestamps'] = array();
		if(!is_array($params['timestamps']))
			$params['timestamps'] = array($params['timestamps']);
		foreach($params['timestamps'] as $i => $ts)
			$params['timestamps'][$i] = wfTimestamp(TS_MW, $ts);

		$pa = new PageArchive($titleObj);
		$dbw = wfGetDb(DB_MASTER);
		$dbw->begin();
		$retval = $pa->undelete((isset($params['timestamps']) ? $params['timestamps'] : array()), $params['reason']);
		if(!is_array($retval))
			$this->dieUsageMsg(array('cannotundelete'));

		if($retval[1])
			wfRunHooks( 'FileUndeleteComplete', 
				array($titleObj, array(), $wgUser, $params['reason']) );

		$info['title'] = $titleObj->getPrefixedText();
		$info['revisions'] = $retval[0];
		$info['fileversions'] = $retval[1];
		$info['reason'] = $retval[2];
		$this->getResult()->addValue(null, $this->getModuleName(), $info);
	}

	public function mustBePosted() { return true; }

	public function getAllowedParams() {
		return array (
			'title' => null,
			'token' => null,
			'reason' => "",
			'timestamps' => array(
				ApiBase :: PARAM_ISMULTI => true
			)
		);
	}

	public function getParamDescription() {
		return array (
			'title' => 'Title of the page you want to restore.',
			'token' => 'An undelete token previously retrieved through list=deletedrevs',
			'reason' => 'Reason for restoring (optional)',
			'timestamps' => 'Timestamps of the revisions to restore. If not set, all revisions will be restored.'
		);
	}

	public function getDescription() {
		return array(
			'Restore certain revisions of a deleted page. A list of deleted revisions (including timestamps) can be',
			'retrieved through list=deletedrevs'
		);
	}

	protected function getExamples() {
		return array (
			'api.php?action=undelete&title=Main%20Page&token=123ABC&reason=Restoring%20main%20page',
			'api.php?action=undelete&title=Main%20Page&token=123ABC&timestamps=20070703220045|20070702194856'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiUndelete.php 35348 2008-05-26 10:51:31Z catrope $';
	}
}
