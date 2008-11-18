<?php

/*
 * Created on June 14, 2007
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
 * A query module to enumerate pages that belong to a category.
 *
 * @ingroup API
 */
class ApiQueryCategoryMembers extends ApiQueryGeneratorBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'cm');
	}

	public function execute() {
		$this->run();
	}

	public function executeGenerator($resultPageSet) {
		$this->run($resultPageSet);
	}

	private function run($resultPageSet = null) {

		$params = $this->extractRequestParams();

		if ( !isset($params['title']) || is_null($params['title']) )
			$this->dieUsage("The cmtitle parameter is required", 'notitle');
		$categoryTitle = Title::newFromText($params['title']);

		if ( is_null( $categoryTitle ) || $categoryTitle->getNamespace() != NS_CATEGORY )
			$this->dieUsage("The category name you entered is not valid", 'invalidcategory');

		$prop = array_flip($params['prop']);
		$fld_ids = isset($prop['ids']);
		$fld_title = isset($prop['title']);
		$fld_sortkey = isset($prop['sortkey']);
		$fld_timestamp = isset($prop['timestamp']);

		if (is_null($resultPageSet)) {
			$this->addFields(array('cl_from', 'cl_sortkey', 'page_namespace', 'page_title'));
			$this->addFieldsIf('page_id', $fld_ids);
		} else {
			$this->addFields($resultPageSet->getPageTableFields()); // will include page_ id, ns, title
			$this->addFields(array('cl_from', 'cl_sortkey'));
		}

		$this->addFieldsIf('cl_timestamp', $fld_timestamp || $params['sort'] == 'timestamp');
		$this->addTables(array('page','categorylinks'));	// must be in this order for 'USE INDEX'
									// Not needed after bug 10280 is applied to servers
		if($params['sort'] == 'timestamp')
		{
			$this->addOption('USE INDEX', 'cl_timestamp');
			// cl_timestamp will be added by addWhereRange() later
			$this->addOption('ORDER BY', 'cl_to');
		}
		else
		{
			$dir = ($params['dir'] == 'desc' ? ' DESC' : '');
			$this->addOption('USE INDEX', 'cl_sortkey');
			$this->addOption('ORDER BY', 'cl_to, cl_sortkey' . $dir . ', cl_from' . $dir);
		}

		$this->addWhere('cl_from=page_id');
		$this->setContinuation($params['continue'], $params['dir']);
		$this->addWhereFld('cl_to', $categoryTitle->getDBkey());
		$this->addWhereFld('page_namespace', $params['namespace']);
		if($params['sort'] == 'timestamp')
			$this->addWhereRange('cl_timestamp', ($params['dir'] == 'asc' ? 'newer' : 'older'), $params['start'], $params['end']);

		$limit = $params['limit'];
		$this->addOption('LIMIT', $limit +1);

		$db = $this->getDB();

		$data = array ();
		$count = 0;
		$lastSortKey = null;
		$res = $this->select(__METHOD__);
		while ($row = $db->fetchObject($res)) {
			if (++ $count > $limit) {
				// We've reached the one extra which shows that there are additional pages to be had. Stop here...
				// TODO: Security issue - if the user has no right to view next title, it will still be shown
				if ($params['sort'] == 'timestamp')
					$this->setContinueEnumParameter('start', wfTimestamp(TS_ISO_8601, $row->cl_timestamp));
				else
					$this->setContinueEnumParameter('continue', $this->getContinueStr($row, $lastSortKey));
				break;
			}

			$lastSortKey = $row->cl_sortkey;	// detect duplicate sortkeys

			if (is_null($resultPageSet)) {
				$vals = array();
				if ($fld_ids)
					$vals['pageid'] = intval($row->page_id);
				if ($fld_title) {
					$title = Title :: makeTitle($row->page_namespace, $row->page_title);
					$vals['ns'] = intval($title->getNamespace());
					$vals['title'] = $title->getPrefixedText();
				}
				if ($fld_sortkey)
					$vals['sortkey'] = $row->cl_sortkey;
				if ($fld_timestamp)
					$vals['timestamp'] = wfTimestamp(TS_ISO_8601, $row->cl_timestamp);
				$data[] = $vals;
			} else {
				$resultPageSet->processDbRow($row);
			}
		}
		$db->freeResult($res);

		if (is_null($resultPageSet)) {
			$this->getResult()->setIndexedTagName($data, 'cm');
			$this->getResult()->addValue('query', $this->getModuleName(), $data);
		}
	}

	private function getContinueStr($row, $lastSortKey) {
		$ret = $row->cl_sortkey . '|';
		if ($row->cl_sortkey == $lastSortKey)	// duplicate sort key, add cl_from
			$ret .= $row->cl_from;
		return $ret;
	}

	/**
	 * Add DB WHERE clause to continue previous query based on 'continue' parameter
	 */
	private function setContinuation($continue, $dir) {
		if (is_null($continue))
			return;	// This is not a continuation request

		$continueList = explode('|', $continue);
		$hasError = count($continueList) != 2;
		$from = 0;
		if (!$hasError && strlen($continueList[1]) > 0) {
			$from = intval($continueList[1]);
			$hasError = ($from == 0);
		}

		if ($hasError)
			$this->dieUsage("Invalid continue param. You should pass the original value returned by the previous query", "badcontinue");

		$encSortKey = $this->getDB()->addQuotes($continueList[0]);
		$encFrom = $this->getDB()->addQuotes($from);
		
		$op = ($dir == 'desc' ? '<' : '>');

		if ($from != 0) {
			// Duplicate sort key continue
			$this->addWhere( "cl_sortkey$op$encSortKey OR (cl_sortkey=$encSortKey AND cl_from$op=$encFrom)" );
		} else {
			$this->addWhere( "cl_sortkey$op=$encSortKey" );
		}
	}

	public function getAllowedParams() {
		return array (
			'title' => null,
			'prop' => array (
				ApiBase :: PARAM_DFLT => 'ids|title',
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_TYPE => array (
					'ids',
					'title',
					'sortkey',
					'timestamp',
				)
			),
			'namespace' => array (
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_TYPE => 'namespace',
			),
			'continue' => null,
			'limit' => array (
				ApiBase :: PARAM_TYPE => 'limit',
				ApiBase :: PARAM_DFLT => 10,
				ApiBase :: PARAM_MIN => 1,
				ApiBase :: PARAM_MAX => ApiBase :: LIMIT_BIG1,
				ApiBase :: PARAM_MAX2 => ApiBase :: LIMIT_BIG2
			),
			'sort' => array(
				ApiBase :: PARAM_DFLT => 'sortkey',
				ApiBase :: PARAM_TYPE => array(
					'sortkey',
					'timestamp'
				)
			),
			'dir' => array(
				ApiBase :: PARAM_DFLT => 'asc',
				ApiBase :: PARAM_TYPE => array(
					'asc',
					'desc'
				)
			),
			'start' => array(
				ApiBase :: PARAM_TYPE => 'timestamp'
			),
			'end' => array(
				ApiBase :: PARAM_TYPE => 'timestamp'
			)
		);
	}

	public function getParamDescription() {
		return array (
			'title' => 'Which category to enumerate (required). Must include Category: prefix',
			'prop' => 'What pieces of information to include',
			'namespace' => 'Only include pages in these namespaces',
			'sort' => 'Property to sort by',
			'dir' => 'In which direction to sort',
			'start' => 'Timestamp to start listing from. Can only be used with cmsort=timestamp',
			'end' => 'Timestamp to end listing at. Can only be used with cmsort=timestamp',
			'continue' => 'For large categories, give the value retured from previous query',
			'limit' => 'The maximum number of pages to return.',
		);
	}

	public function getDescription() {
		return 'List all pages in a given category';
	}

	protected function getExamples() {
		return array (
				"Get first 10 pages in [[Category:Physics]]:",
				"  api.php?action=query&list=categorymembers&cmtitle=Category:Physics",
				"Get page info about first 10 pages in [[Category:Physics]]:",
				"  api.php?action=query&generator=categorymembers&gcmtitle=Category:Physics&prop=info",
			);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryCategoryMembers.php 35098 2008-05-20 17:13:28Z ialex $';
	}
}
