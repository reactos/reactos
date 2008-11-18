<?php

/*
 * Created on Monday, January 28, 2008
 *
 * API for MediaWiki 1.8+
 *
 * Copyright (C) 2008 Brent Garber
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
 * Query module to get list of random pages
 *
 * @ingroup API
 */

 class ApiQueryRandom extends ApiQueryGeneratorBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'rn');
	}

	public function execute() {
		$this->run();
	}

	public function executeGenerator($resultPageSet) {
		$this->run($resultPageSet);
	}

	protected function prepareQuery($randstr, $limit, $namespace, &$resultPageSet) {
		$this->resetQueryParams();
		$this->addTables('page');
		$this->addOption('LIMIT', $limit);
		$this->addWhereFld('page_namespace', $namespace);
		$this->addWhereRange('page_random', 'newer', $randstr, null);
		$this->addWhere(array('page_is_redirect' => 0));
		$this->addOption('USE INDEX', 'page_random');
		if(is_null($resultPageSet))
			$this->addFields(array('page_id', 'page_title', 'page_namespace'));
		else
			$this->addFields($resultPageSet->getPageTableFields());
	}

	protected function runQuery(&$data, &$resultPageSet) {
		$db = $this->getDB();
		$res = $this->select(__METHOD__);
		$count = 0;
		while($row = $db->fetchObject($res)) {
			$count++;
			if(is_null($resultPageSet))
			{
				// Prevent duplicates
				if(!in_array($row->page_id, $this->pageIDs))
				{
					$data[] = $this->extractRowInfo($row);
					$this->pageIDs[] = $row->page_id;
				}
			}
			else
				$resultPageSet->processDbRow($row);
		}
		$db->freeResult($res);
		return $count;
	}

	public function run($resultPageSet = null) {
		$params = $this->extractRequestParams();
		$result = $this->getResult();
		$data = array();
		$this->pageIDs = array();
		$this->prepareQuery(wfRandom(), $params['limit'], $params['namespace'], $resultPageSet);
		$count = $this->runQuery($data, $resultPageSet);
		if($count < $params['limit'])
		{
			/* We got too few pages, we probably picked a high value
			 * for page_random. We'll just take the lowest ones, see
			 * also the comment in Title::getRandomTitle()
			 */
			 $this->prepareQuery(0, $params['limit'] - $count, $params['namespace'], $resultPageSet);
			 $this->runQuery($data, $resultPageSet);
		}

		if(is_null($resultPageSet)) {
			$result->setIndexedTagName($data, 'page');
			$result->addValue('query', $this->getModuleName(), $data);
		}
	}

	private function extractRowInfo($row) {
		$title = Title::makeTitle($row->page_namespace, $row->page_title);
		$vals = array();
		$vals['title'] = $title->getPrefixedText();
		$vals['ns'] = $row->page_namespace;
		$vals['id'] = $row->page_id;
		return $vals;
	}

	public function getAllowedParams() {
		return array (
			'namespace' => array(
				ApiBase :: PARAM_TYPE => 'namespace',
				ApiBase :: PARAM_ISMULTI => true
			),
			'limit' => array (
				ApiBase :: PARAM_TYPE => 'limit',
				ApiBase :: PARAM_DFLT => 1,
				ApiBase :: PARAM_MIN => 1,
				ApiBase :: PARAM_MAX => 10,
				ApiBase :: PARAM_MAX2 => 20
			),
		);
	}

	public function getParamDescription() {
		return array (
			'namespace' => 'Return pages in these namespaces only',
			'limit' => 'Limit how many random pages will be returned'
		);
	}

	public function getDescription() {
		return array(	'Get a set of random pages',
				'NOTE: Pages are listed in a fixed sequence, only the starting point is random. This means that if, for example, "Main Page" is the first ',
				'      random page on your list, "List of fictional monkeys" will *always* be second, "List of people on stamps of Vanuatu" third, etc.',
				'NOTE: If the number of pages in the namespace is lower than rnlimit, you will get fewer pages. You will not get the same page twice.'
		);
	}

	protected function getExamples() {
		return 'api.php?action=query&list=random&rnnamespace=0&rnlimit=2';
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryRandom.php overlordq$';
	}
}
