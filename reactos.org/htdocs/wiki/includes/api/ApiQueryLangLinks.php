<?php

/*
 * Created on May 13, 2007
 *
 * API for MediaWiki 1.8+
 *
 * Copyright (C) 2006 Yuri Astrakhan <Firstname><Lastname>@gmail.com
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
	require_once ("ApiQueryBase.php");
}

/**
 * A query module to list all langlinks (links to correspanding foreign language pages).
 *
 * @ingroup API
 */
class ApiQueryLangLinks extends ApiQueryBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'll');
	}

	public function execute() {
		if ( $this->getPageSet()->getGoodTitleCount() == 0 )
			return;

		$params = $this->extractRequestParams();
		$this->addFields(array (
			'll_from',
			'll_lang',
			'll_title'
		));

		$this->addTables('langlinks');
		$this->addWhereFld('ll_from', array_keys($this->getPageSet()->getGoodTitles()));
		if(!is_null($params['continue'])) {
			$cont = explode('|', $params['continue']);
			if(count($cont) != 2)
				$this->dieUsage("Invalid continue param. You should pass the " .
					"original value returned by the previous query", "_badcontinue");
			$llfrom = intval($cont[0]);
			$lllang = $this->getDb()->strencode($cont[1]);
			$this->addWhere("ll_from > $llfrom OR ".
					"(ll_from = $llfrom AND ".
					"ll_lang >= '$lllang')");
		}
		# Don't order by ll_from if it's constant in the WHERE clause
		if(count($this->getPageSet()->getGoodTitles()) == 1)
			$this->addOption('ORDER BY', 'll_lang');
		else
			$this->addOption('ORDER BY', 'll_from, ll_lang');
		$this->addOption('LIMIT', $params['limit'] + 1);
		$res = $this->select(__METHOD__);

		$data = array();
		$lastId = 0;	// database has no ID 0
		$count = 0;
		$db = $this->getDB();
		while ($row = $db->fetchObject($res)) {
			if (++$count > $params['limit']) {
				// We've reached the one extra which shows that
				// there are additional pages to be had. Stop here...
				$this->setContinueEnumParameter('continue', "{$row->ll_from}|{$row->ll_lang}");
				break;
			}
			if ($lastId != $row->ll_from) {
				if($lastId != 0) {
					$this->addPageSubItems($lastId, $data);
					$data = array();
				}
				$lastId = $row->ll_from;
			}

			$entry = array('lang' => $row->ll_lang);
			ApiResult :: setContent($entry, $row->ll_title);
			$data[] = $entry;
		}

		if($lastId != 0) {
			$this->addPageSubItems($lastId, $data);
		}

		$db->freeResult($res);
	}

	public function getAllowedParams() {
		return array(
				'limit' => array(
					ApiBase :: PARAM_DFLT => 10,
					ApiBase :: PARAM_TYPE => 'limit',
					ApiBase :: PARAM_MIN => 1,
					ApiBase :: PARAM_MAX => ApiBase :: LIMIT_BIG1,
					ApiBase :: PARAM_MAX2 => ApiBase :: LIMIT_BIG2
				),
				'continue' => null,
		);
	}

	public function getParamDescription () {
		return array(
			'limit' => 'How many langlinks to return',
			'continue' => 'When more results are available, use this to continue',
		);
	}

	public function getDescription() {
		return 'Returns all interlanguage links from the given page(s)';
	}

	protected function getExamples() {
		return array (
				"Get interlanguage links from the [[Main Page]]:",
				"  api.php?action=query&prop=langlinks&titles=Main%20Page&redirects",
			);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryLangLinks.php 37534 2008-07-10 21:08:37Z brion $';
	}
}
