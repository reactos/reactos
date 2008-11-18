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
 * A query module to list all external URLs found on a given set of pages.
 *
 * @ingroup API
 */
class ApiQueryExternalLinks extends ApiQueryBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'el');
	}

	public function execute() {
		if ( $this->getPageSet()->getGoodTitleCount() == 0 )
			return;

		$params = $this->extractRequestParams();
		$this->addFields(array (
			'el_from',
			'el_to'
		));

		$this->addTables('externallinks');
		$this->addWhereFld('el_from', array_keys($this->getPageSet()->getGoodTitles()));
		# Don't order by el_from if it's constant in the WHERE clause
		if(count($this->getPageSet()->getGoodTitles()) != 1)
			$this->addOption('ORDER BY', 'el_from');
		$this->addOption('LIMIT', $params['limit'] + 1);
		if(!is_null($params['offset']))
			$this->addOption('OFFSET', $params['offset']);

		$db = $this->getDB();
		$res = $this->select(__METHOD__);

		$data = array();
		$lastId = 0;	// database has no ID 0
		$count = 0;
		while ($row = $db->fetchObject($res)) {
			if (++$count > $params['limit']) {
				// We've reached the one extra which shows that
				// there are additional pages to be had. Stop here...
				$this->setContinueEnumParameter('offset', @$params['offset'] + $params['limit']);
				break;
			}
			if ($lastId != $row->el_from) {
				if($lastId != 0) {
					$this->addPageSubItems($lastId, $data);
					$data = array();
				}
				$lastId = $row->el_from;
			}

			$entry = array();
			ApiResult :: setContent($entry, $row->el_to);
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
				'offset' => null,
		);
	}

	public function getParamDescription () {
		return array(
			'limit' => 'How many links to return',
			'offset' => 'When more results are available, use this to continue',
		);
	}

	public function getDescription() {
		return 'Returns all external urls (not interwikies) from the given page(s)';
	}

	protected function getExamples() {
		return array (
				"Get a list of external links on the [[Main Page]]:",
				"  api.php?action=query&prop=extlinks&titles=Main%20Page",
			);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryExternalLinks.php 37270 2008-07-07 17:32:22Z catrope $';
	}
}
