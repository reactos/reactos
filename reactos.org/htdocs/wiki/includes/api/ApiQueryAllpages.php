<?php

/*
 * Created on Sep 25, 2006
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
	require_once ('ApiQueryBase.php');
}

/**
 * Query module to enumerate all available pages.
 *
 * @ingroup API
 */
class ApiQueryAllpages extends ApiQueryGeneratorBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'ap');
	}

	public function execute() {
		$this->run();
	}

	public function executeGenerator($resultPageSet) {
		if ($resultPageSet->isResolvingRedirects())
			$this->dieUsage('Use "gapfilterredir=nonredirects" option instead of "redirects" when using allpages as a generator', 'params');

		$this->run($resultPageSet);
	}

	private function run($resultPageSet = null) {

		$db = $this->getDB();

		$params = $this->extractRequestParams();

		// Page filters
		$this->addTables('page');
		if (!$this->addWhereIf('page_is_redirect = 1', $params['filterredir'] === 'redirects'))
			$this->addWhereIf('page_is_redirect = 0', $params['filterredir'] === 'nonredirects');
		$this->addWhereFld('page_namespace', $params['namespace']);
		$dir = ($params['dir'] == 'descending' ? 'older' : 'newer');
		$from = (is_null($params['from']) ? null : $this->titleToKey($params['from']));
		$this->addWhereRange('page_title', $dir, $from, null);
		if (isset ($params['prefix']))
			$this->addWhere("page_title LIKE '" . $db->escapeLike($this->titleToKey($params['prefix'])) . "%'");

		$forceNameTitleIndex = true;
		if (isset ($params['minsize'])) {
			$this->addWhere('page_len>=' . intval($params['minsize']));
			$forceNameTitleIndex = false;
		}

		if (isset ($params['maxsize'])) {
			$this->addWhere('page_len<=' . intval($params['maxsize']));
			$forceNameTitleIndex = false;
		}

		// Page protection filtering
		if (isset ($params['prtype'])) {
			$this->addTables('page_restrictions');
			$this->addWhere('page_id=pr_page');
			$this->addWhere('pr_expiry>' . $db->addQuotes($db->timestamp()));
			$this->addWhereFld('pr_type', $params['prtype']);

			$prlevel = $params['prlevel'];
			if (!is_null($prlevel) && $prlevel != '' && $prlevel != '*')
				$this->addWhereFld('pr_level', $prlevel);

			$this->addOption('DISTINCT');

			$forceNameTitleIndex = false;

		} else if (isset ($params['prlevel'])) {
			$this->dieUsage('prlevel may not be used without prtype', 'params');
		}

		if($params['filterlanglinks'] == 'withoutlanglinks') {
			$this->addTables('langlinks');
			$this->addJoinConds(array('langlinks' => array('LEFT JOIN', 'page_id=ll_from')));
			$this->addWhere('ll_from IS NULL');
			$forceNameTitleIndex = false;
		} else if($params['filterlanglinks'] == 'withlanglinks') {
			$this->addTables('langlinks');
			$this->addWhere('page_id=ll_from');
			$forceNameTitleIndex = false;
		}
		if ($forceNameTitleIndex)
			$this->addOption('USE INDEX', 'name_title');

		if (is_null($resultPageSet)) {
			$this->addFields(array (
				'page_id',
				'page_namespace',
				'page_title'
			));
		} else {
			$this->addFields($resultPageSet->getPageTableFields());
		}

		$limit = $params['limit'];
		$this->addOption('LIMIT', $limit+1);
		$res = $this->select(__METHOD__);

		$data = array ();
		$count = 0;
		while ($row = $db->fetchObject($res)) {
			if (++ $count > $limit) {
				// We've reached the one extra which shows that there are additional pages to be had. Stop here...
				// TODO: Security issue - if the user has no right to view next title, it will still be shown
				$this->setContinueEnumParameter('from', $this->keyToTitle($row->page_title));
				break;
			}

			if (is_null($resultPageSet)) {
				$title = Title :: makeTitle($row->page_namespace, $row->page_title);
				$data[] = array(
					'pageid' => intval($row->page_id),
					'ns' => intval($title->getNamespace()),
					'title' => $title->getPrefixedText());
			} else {
				$resultPageSet->processDbRow($row);
			}
		}
		$db->freeResult($res);

		if (is_null($resultPageSet)) {
			$result = $this->getResult();
			$result->setIndexedTagName($data, 'p');
			$result->addValue('query', $this->getModuleName(), $data);
		}
	}

	public function getAllowedParams() {
		global $wgRestrictionTypes, $wgRestrictionLevels;

		return array (
			'from' => null,
			'prefix' => null,
			'namespace' => array (
				ApiBase :: PARAM_DFLT => 0,
				ApiBase :: PARAM_TYPE => 'namespace',
			),
			'filterredir' => array (
				ApiBase :: PARAM_DFLT => 'all',
				ApiBase :: PARAM_TYPE => array (
					'all',
					'redirects',
					'nonredirects'
				)
			),
			'minsize' => array (
				ApiBase :: PARAM_TYPE => 'integer',
			),
			'maxsize' => array (
				ApiBase :: PARAM_TYPE => 'integer',
			),
			'prtype' => array (
				ApiBase :: PARAM_TYPE => $wgRestrictionTypes,
				ApiBase :: PARAM_ISMULTI => true
			),
			'prlevel' => array (
				ApiBase :: PARAM_TYPE => $wgRestrictionLevels,
				ApiBase :: PARAM_ISMULTI => true
			),
			'limit' => array (
				ApiBase :: PARAM_DFLT => 10,
				ApiBase :: PARAM_TYPE => 'limit',
				ApiBase :: PARAM_MIN => 1,
				ApiBase :: PARAM_MAX => ApiBase :: LIMIT_BIG1,
				ApiBase :: PARAM_MAX2 => ApiBase :: LIMIT_BIG2
			),
			'dir' => array (
				ApiBase :: PARAM_DFLT => 'ascending',
				ApiBase :: PARAM_TYPE => array (
					'ascending',
					'descending'
				)
			),
			'filterlanglinks' => array(
				ApiBase :: PARAM_TYPE => array(
					'withlanglinks',
					'withoutlanglinks',
					'all'
				),
				ApiBase :: PARAM_DFLT => 'all'
			)
		);
	}

	public function getParamDescription() {
		return array (
			'from' => 'The page title to start enumerating from.',
			'prefix' => 'Search for all page titles that begin with this value.',
			'namespace' => 'The namespace to enumerate.',
			'filterredir' => 'Which pages to list.',
			'dir' => 'The direction in which to list',
			'minsize' => 'Limit to pages with at least this many bytes',
			'maxsize' => 'Limit to pages with at most this many bytes',
			'prtype' => 'Limit to protected pages only',
			'prlevel' => 'The protection level (must be used with apprtype= parameter)',
			'filterlanglinks' => 'Filter based on whether a page has langlinks',
			'limit' => 'How many total pages to return.'
		);
	}

	public function getDescription() {
		return 'Enumerate all pages sequentially in a given namespace';
	}

	protected function getExamples() {
		return array (
			'Simple Use',
			' Show a list of pages starting at the letter "B"',
			'  api.php?action=query&list=allpages&apfrom=B',
			'Using as Generator',
			' Show info about 4 pages starting at the letter "T"',
			'  api.php?action=query&generator=allpages&gaplimit=4&gapfrom=T&prop=info',
			' Show content of first 2 non-redirect pages begining at "Re"',
			'  api.php?action=query&generator=allpages&gaplimit=2&gapfilterredir=nonredirects&gapfrom=Re&prop=revisions&rvprop=content'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryAllpages.php 37775 2008-07-17 09:26:01Z brion $';
	}
}
