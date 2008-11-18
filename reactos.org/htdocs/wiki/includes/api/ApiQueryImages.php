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
 * This query adds an <images> subelement to all pages with the list of images embedded into those pages.
 *
 * @ingroup API
 */
class ApiQueryImages extends ApiQueryGeneratorBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'im');
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
		$this->addFields(array (
			'il_from',
			'il_to'
		));

		$this->addTables('imagelinks');
		$this->addWhereFld('il_from', array_keys($this->getPageSet()->getGoodTitles()));
		if(!is_null($params['continue'])) {
			$cont = explode('|', $params['continue']);
			if(count($cont) != 2)
				$this->dieUsage("Invalid continue param. You should pass the " .
					"original value returned by the previous query", "_badcontinue");
			$ilfrom = intval($cont[0]);
			$ilto = $this->getDb()->strencode($this->titleToKey($cont[1]));
			$this->addWhere("il_from > $ilfrom OR ".
					"(il_from = $ilfrom AND ".
					"il_to >= '$ilto')");
		}
		# Don't order by il_from if it's constant in the WHERE clause
		if(count($this->getPageSet()->getGoodTitles()) == 1)
			$this->addOption('ORDER BY', 'il_to');
		else
			$this->addOption('ORDER BY', 'il_from, il_to');
		$this->addOption('LIMIT', $params['limit'] + 1);

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
					$this->setContinueEnumParameter('continue', $row->il_from .
							'|' . $this->keyToTitle($row->il_to));
					break;
				}
				if ($lastId != $row->il_from) {
					if($lastId != 0) {
						$this->addPageSubItems($lastId, $data);
						$data = array();
					}
					$lastId = $row->il_from;
				}

				$vals = array();
				ApiQueryBase :: addTitleInfo($vals, Title :: makeTitle(NS_IMAGE, $row->il_to));
				$data[] = $vals;
			}

			if($lastId != 0) {
				$this->addPageSubItems($lastId, $data);
			}

		} else {

			$titles = array();
			$count = 0;
			while ($row = $db->fetchObject($res)) {
				if (++$count > $params['limit']) {
					// We've reached the one extra which shows that
					// there are additional pages to be had. Stop here...
					$this->setContinueEnumParameter('continue', $row->il_from .
							'|' . $this->keyToTitle($row->il_to));
					break;
				}
				$titles[] = Title :: makeTitle(NS_IMAGE, $row->il_to);
			}
			$resultPageSet->populateFromTitles($titles);
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
			'limit' => 'How many images to return',
			'continue' => 'When more results are available, use this to continue',
		);
	}

	public function getDescription() {
		return 'Returns all images contained on the given page(s)';
	}

	protected function getExamples() {
		return array (
				"Get a list of images used in the [[Main Page]]:",
				"  api.php?action=query&prop=images&titles=Main%20Page",
				"Get information about all images used in the [[Main Page]]:",
				"  api.php?action=query&generator=images&titles=Main%20Page&prop=info"
			);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryImages.php 37535 2008-07-10 21:20:43Z catrope $';
	}
}
