<?php

/*
 * Created on May 12, 2007
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
 * A query module to list all wiki links on a given set of pages.
 *
 * @ingroup API
 */
class ApiQueryLinks extends ApiQueryGeneratorBase {

	const LINKS = 'links';
	const TEMPLATES = 'templates';

	private $table, $prefix, $description;

	public function __construct($query, $moduleName) {

		switch ($moduleName) {
			case self::LINKS :
				$this->table = 'pagelinks';
				$this->prefix = 'pl';
				$this->description = 'link';
				break;
			case self::TEMPLATES :
				$this->table = 'templatelinks';
				$this->prefix = 'tl';
				$this->description = 'template';
				break;
			default :
				ApiBase :: dieDebug(__METHOD__, 'Unknown module name');
		}

		parent :: __construct($query, $moduleName, $this->prefix);
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
			$this->prefix . '_from pl_from',
			$this->prefix . '_namespace pl_namespace',
			$this->prefix . '_title pl_title'
		));

		$this->addTables($this->table);
		$this->addWhereFld($this->prefix . '_from', array_keys($this->getPageSet()->getGoodTitles()));
		$this->addWhereFld($this->prefix . '_namespace', $params['namespace']);

		if(!is_null($params['continue'])) {
			$cont = explode('|', $params['continue']);
			if(count($cont) != 3)
				$this->dieUsage("Invalid continue param. You should pass the " .
					"original value returned by the previous query", "_badcontinue");
			$plfrom = intval($cont[0]);
			$plns = intval($cont[1]);
			$pltitle = $this->getDb()->strencode($this->titleToKey($cont[2]));
			$this->addWhere("{$this->prefix}_from > $plfrom OR ".
					"({$this->prefix}_from = $plfrom AND ".
					"({$this->prefix}_namespace > $plns OR ".
					"({$this->prefix}_namespace = $plns AND ".
					"{$this->prefix}_title >= '$pltitle')))");
		}

		# Here's some MySQL craziness going on: if you use WHERE foo='bar'
		# and later ORDER BY foo MySQL doesn't notice the ORDER BY is pointless
		# but instead goes and filesorts, because the index for foo was used
		# already. To work around this, we drop constant fields in the WHERE
		# clause from the ORDER BY clause
		$order = array();
		if(count($this->getPageSet()->getGoodTitles()) != 1)
			$order[] = "{$this->prefix}_from";
		if(count($params['namespace']) != 1)
			$order[] = "{$this->prefix}_namespace";
		$order[] = "{$this->prefix}_title";
		$this->addOption('ORDER BY', implode(", ", $order));
		$this->addOption('USE INDEX', "{$this->prefix}_from");
		$this->addOption('LIMIT', $params['limit'] + 1);

		$db = $this->getDB();
		$res = $this->select(__METHOD__);

		if (is_null($resultPageSet)) {

			$data = array();
			$lastId = 0;	// database has no ID 0
			$count = 0;
			while ($row = $db->fetchObject($res)) {
				if(++$count > $params['limit']) {
					// We've reached the one extra which shows that
					// there are additional pages to be had. Stop here...
					$this->setContinueEnumParameter('continue',
						"{$row->pl_from}|{$row->pl_namespace}|" .
						$this->keyToTitle($row->pl_title));
					break;
				}
				if ($lastId != $row->pl_from) {
					if($lastId != 0) {
						$this->addPageSubItems($lastId, $data);
						$data = array();
					}
					$lastId = $row->pl_from;
				}

				$vals = array();
				ApiQueryBase :: addTitleInfo($vals, Title :: makeTitle($row->pl_namespace, $row->pl_title));
				$data[] = $vals;
			}

			if($lastId != 0) {
				$this->addPageSubItems($lastId, $data);
			}

		} else {

			$titles = array();
			$count = 0;
			while ($row = $db->fetchObject($res)) {
				if(++$count > $params['limit']) {
					// We've reached the one extra which shows that
					// there are additional pages to be had. Stop here...
					$this->setContinueEnumParameter('continue',
						"{$row->pl_from}|{$row->pl_namespace}|" .
						$this->keyToTitle($row->pl_title));
					break;
				}
				$titles[] = Title :: makeTitle($row->pl_namespace, $row->pl_title);
			}
			$resultPageSet->populateFromTitles($titles);
		}

		$db->freeResult($res);
	}

	public function getAllowedParams()
	{
		return array(
				'namespace' => array(
					ApiBase :: PARAM_TYPE => 'namespace',
					ApiBase :: PARAM_ISMULTI => true
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

	public function getParamDescription()
	{
		return array(
				'namespace' => "Show {$this->description}s in this namespace(s) only",
				'limit' => "How many {$this->description}s to return",
				'continue' => 'When more results are available, use this to continue',
		);
	}

	public function getDescription() {
		return "Returns all {$this->description}s from the given page(s)";
	}

	protected function getExamples() {
		return array (
				"Get {$this->description}s from the [[Main Page]]:",
				"  api.php?action=query&prop={$this->getModuleName()}&titles=Main%20Page",
				"Get information about the {$this->description} pages in the [[Main Page]]:",
				"  api.php?action=query&generator={$this->getModuleName()}&titles=Main%20Page&prop=info",
				"Get {$this->description}s from the Main Page in the User and Template namespaces:",
				"  api.php?action=query&prop={$this->getModuleName()}&titles=Main%20Page&{$this->prefix}namespace=2|10"
			);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryLinks.php 37909 2008-07-22 13:26:15Z catrope $';
	}
}
