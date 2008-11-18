<?php

/*
 * Created on July 30, 2007
 *
 * API for MediaWiki 1.8+
 *
 * Copyright (C) 2007 Yuri Astrakhan <Firstname><Lastname>@gmail.com
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
	require_once ('ApiQueryBase.php');
}

/**
 * Query module to get information about the currently logged-in user
 *
 * @ingroup API
 */
class ApiQueryUserInfo extends ApiQueryBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'ui');
	}

	public function execute() {
		$params = $this->extractRequestParams();
		$result = $this->getResult();
		$r = array();

		if (!is_null($params['prop'])) {
			$this->prop = array_flip($params['prop']);
		} else {
			$this->prop = array();
		}
		$r = $this->getCurrentUserInfo();
		$result->addValue("query", $this->getModuleName(), $r);
	}

	protected function getCurrentUserInfo() {
		global $wgUser;
		$result = $this->getResult();
		$vals = array();
		$vals['id'] = $wgUser->getId();
		$vals['name'] = $wgUser->getName();

		if($wgUser->isAnon())
			$vals['anon'] = '';
		if (isset($this->prop['blockinfo'])) {
			if ($wgUser->isBlocked()) {
				$vals['blockedby'] = User::whoIs($wgUser->blockedBy());
				$vals['blockreason'] = $wgUser->blockedFor();
			}
		}
		if (isset($this->prop['hasmsg']) && $wgUser->getNewtalk()) {
			$vals['messages'] = '';
		}
		if (isset($this->prop['groups'])) {
			$vals['groups'] = $wgUser->getGroups();
			$result->setIndexedTagName($vals['groups'], 'g');	// even if empty
		}
		if (isset($this->prop['rights'])) {
			$vals['rights'] = $wgUser->getRights();
			$result->setIndexedTagName($vals['rights'], 'r');	// even if empty
		}
		if (isset($this->prop['options'])) {
			$vals['options'] = (is_null($wgUser->mOptions) ? User::getDefaultOptions() : $wgUser->mOptions);
		}
		if (isset($this->prop['editcount'])) {
			$vals['editcount'] = $wgUser->getEditCount();
		}
		if (isset($this->prop['ratelimits'])) {
			$vals['ratelimits'] = $this->getRateLimits();
		}
		return $vals;
	}

	protected function getRateLimits()
	{
		global $wgUser, $wgRateLimits;
		if(!$wgUser->isPingLimitable())
			return array(); // No limits

		// Find out which categories we belong to
		$categories = array();
		if($wgUser->isAnon())
			$categories[] = 'anon';
		else
			$categories[] = 'user';
		if($wgUser->isNewBie())
		{
			$categories[] = 'ip';
			$categories[] = 'subnet';
			if(!$wgUser->isAnon())
				$categories[] = 'newbie';
		}

		// Now get the actual limits
		$retval = array();
		foreach($wgRateLimits as $action => $limits)
			foreach($categories as $cat)
				if(isset($limits[$cat]) && !is_null($limits[$cat]))
				{
					$retval[$action][$cat]['hits'] = $limits[$cat][0];
					$retval[$action][$cat]['seconds'] = $limits[$cat][1];
				}
		return $retval;
	}

	public function getAllowedParams() {
		return array (
			'prop' => array (
				ApiBase :: PARAM_DFLT => NULL,
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_TYPE => array (
					'blockinfo',
					'hasmsg',
					'groups',
					'rights',
					'options',
					'editcount',
					'ratelimits'
				)
			)
		);
	}

	public function getParamDescription() {
		return array (
			'prop' => array(
				'What pieces of information to include',
				'  blockinfo  - tags if the current user is blocked, by whom, and for what reason',
				'  hasmsg     - adds a tag "message" if the current user has pending messages',
				'  groups     - lists all the groups the current user belongs to',
				'  rights     - lists of all rights the current user has',
				'  options    - lists all preferences the current user has set',
				'  editcount  - adds the current user\'s edit count',
				'  ratelimits - lists all rate limits applying to the current user'
			)
		);
	}

	public function getDescription() {
		return 'Get information about the current user';
	}

	protected function getExamples() {
		return array (
			'api.php?action=query&meta=userinfo',
			'api.php?action=query&meta=userinfo&uiprop=blockinfo|groups|rights|hasmsg',
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryUserInfo.php 35186 2008-05-22 16:39:43Z brion $';
	}
}
