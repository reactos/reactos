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
 * A query module to enumerate categories the set of pages belong to.
 *
 * @ingroup API
 */
class ApiQueryCategories extends ApiQueryGeneratorBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'cl');
	}

	public function execute() {
		$this->run();
	}

	public function executeGenerator($resultPageSet) {
		$this->run($resultPageSet);
	}

	private function run($resultPageSet = null) {

		if ($this->getPageSet()->getGoodTitleCount() == 0)
			return;	// nothing to do

		$params = $this->extractRequestParams();
		$prop = $params['prop'];

		$this->addFields(array (
			'cl_from',
			'cl_to'
		));

		$fld_sortkey = $fld_timestamp = false;
		if (!is_null($prop)) {
			foreach($prop as $p) {
				switch ($p) {
					case 'sortkey':
						$this->addFields('cl_sortkey');
						$fld_sortkey = true;
						break;
					case 'timestamp':
						$this->addFields('cl_timestamp');
						$fld_timestamp = true;
						break;
					default :
						ApiBase :: dieDebug(__METHOD__, "Unknown prop=$p");
				}
			}
		}

		$this->addTables('categorylinks');
		$this->addWhereFld('cl_from', array_keys($this->getPageSet()->getGoodTitles()));
		if(!is_null($params['continue'])) {
			$cont = explode('|', $params['continue']);
			if(count($cont) != 2)
				$this->dieUsage("Invalid continue param. You should pass the " .
					"original value returned by the previous query", "_badcontinue");
			$clfrom = intval($cont[0]);
			$clto = $this->getDb()->strencode($this->titleToKey($cont[1]));
			$this->addWhere("cl_from > $clfrom OR ".
					"(cl_from = $clfrom AND ".
					"cl_to >= '$clto')");
		}
		# Don't order by cl_from if it's constant in the WHERE clause
		if(count($this->getPageSet()->getGoodTitles()) == 1)
			$this->addOption('ORDER BY', 'cl_to');
		else
			$this->addOption('ORDER BY', "cl_from, cl_to");

		$db = $this->getDB();
		$res = $this->select(__METHOD__);

		if (is_null($resultPageSet)) {

			$data = array();
			$lastId = 0;	// database has no ID 0
			$count = 0;
			while ($row = $db->fetchObject($res)) {
				if (++$count > $params['limit']) {
					// We've reached the one extra which shows that
					// there are additional pages to be had. Stop here...
					$this->setContinueEnumParameter('continue', $row->cl_from .
							'|' . $this->keyToTitle($row->cl_to));
					break;
				}
				if ($lastId != $row->cl_from) {
					if($lastId != 0) {
						$this->addPageSubItems($lastId, $data);
						$data = array();
					}
					$lastId = $row->cl_from;
				}

				$title = Title :: makeTitle(NS_CATEGORY, $row->cl_to);

				$vals = array();
				ApiQueryBase :: addTitleInfo($vals, $title);
				if ($fld_sortkey)
					$vals['sortkey'] = $row->cl_sortkey;
				if ($fld_timestamp)
					$vals['timestamp'] = $row->cl_timestamp;

				$data[] = $vals;
			}

			if($lastId != 0) {
				$this->addPageSubItems($lastId, $data);
			}

		} else {

			$titles = array();
			while ($row = $db->fetchObject($res)) {
				if (++$count > $params['limit']) {
					// We've reached the one extra which shows that
					// there are additional pages to be had. Stop here...
					$this->setContinueEnumParameter('continue', $row->cl_from .
							'|' . $this->keyToTitle($row->cl_to));
					break;
				}

				$titles[] = Title :: makeTitle(NS_CATEGORY, $row->cl_to);
			}
			$resultPageSet->populateFromTitles($titles);
		}

		$db->freeResult($res);
	}

	public function getAllowedParams() {
		return array (
			'prop' => array (
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_TYPE => array (
					'sortkey',
					'timestamp',
				)
			),
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

	public function getParamDescription() {
		return array (
			'prop' => 'Which additional properties to get for each category.',
			'limit' => 'How many categories to return',
			'continue' => 'When more results are available, use this to continue',
		);
	}

	public function getDescription() {
		return 'List all categories the page(s) belong to';
	}

	protected function getExamples() {
		return array (
				"Get a list of categories [[Albert Einstein]] belongs to:",
				"  api.php?action=query&prop=categories&titles=Albert%20Einstein",
				"Get information about all categories used in the [[Albert Einstein]]:",
				"  api.php?action=query&generator=categories&titles=Albert%20Einstein&prop=info"
			);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryCategories.php 37909 2008-07-22 13:26:15Z catrope $';
	}
}
