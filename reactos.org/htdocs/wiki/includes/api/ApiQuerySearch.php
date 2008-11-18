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
 * Query module to perform full text search within wiki titles and content
 *
 * @ingroup API
 */
class ApiQuerySearch extends ApiQueryGeneratorBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'sr');
	}

	public function execute() {
		$this->run();
	}

	public function executeGenerator($resultPageSet) {
		$this->run($resultPageSet);
	}

	private function run($resultPageSet = null) {

		$params = $this->extractRequestParams();

		$limit = $params['limit'];
		$query = $params['search'];
		if (is_null($query) || empty($query))
			$this->dieUsage("empty search string is not allowed", 'param-search');

		$search = SearchEngine::create();
		$search->setLimitOffset( $limit+1, $params['offset'] );
		$search->setNamespaces( $params['namespace'] );
		$search->showRedirects = $params['redirects'];

		if ($params['what'] == 'text')
			$matches = $search->searchText( $query );
		else
			$matches = $search->searchTitle( $query );
		if (is_null($matches))
			$this->dieUsage("{$params['what']} search is disabled",
					"search-{$params['what']}-disabled");

		$data = array ();
		$count = 0;
		while( $result = $matches->next() ) {
			if (++ $count > $limit) {
				// We've reached the one extra which shows that there are additional items to be had. Stop here...
				$this->setContinueEnumParameter('offset', $params['offset'] + $params['limit']);
				break;
			}

			// Silently skip broken titles
			if ($result->isBrokenTitle()) continue;
			
			$title = $result->getTitle();
			if (is_null($resultPageSet)) {
				$data[] = array(
					'ns' => intval($title->getNamespace()),
					'title' => $title->getPrefixedText());
			} else {
				$data[] = $title;
			}
		}

		if (is_null($resultPageSet)) {
			$result = $this->getResult();
			$result->setIndexedTagName($data, 'p');
			$result->addValue('query', $this->getModuleName(), $data);
		} else {
			$resultPageSet->populateFromTitles($data);
		}
	}

	public function getAllowedParams() {
		return array (
			'search' => null,
			'namespace' => array (
				ApiBase :: PARAM_DFLT => 0,
				ApiBase :: PARAM_TYPE => 'namespace',
				ApiBase :: PARAM_ISMULTI => true,
			),
			'what' => array (
				ApiBase :: PARAM_DFLT => 'title',
				ApiBase :: PARAM_TYPE => array (
					'title',
					'text',
				)
			),
			'redirects' => false,
			'offset' => 0,
			'limit' => array (
				ApiBase :: PARAM_DFLT => 10,
				ApiBase :: PARAM_TYPE => 'limit',
				ApiBase :: PARAM_MIN => 1,
				ApiBase :: PARAM_MAX => ApiBase :: LIMIT_BIG1,
				ApiBase :: PARAM_MAX2 => ApiBase :: LIMIT_BIG2
			)
		);
	}

	public function getParamDescription() {
		return array (
			'search' => 'Search for all page titles (or content) that has this value.',
			'namespace' => 'The namespace(s) to enumerate.',
			'what' => 'Search inside the text or titles.',
			'redirects' => 'Include redirect pages in the search.',
			'offset' => 'Use this value to continue paging (return by query)',
			'limit' => 'How many total pages to return.'
		);
	}

	public function getDescription() {
		return 'Perform a full text search';
	}

	protected function getExamples() {
		return array (
			'api.php?action=query&list=search&srsearch=meaning',
			'api.php?action=query&list=search&srwhat=text&srsearch=meaning',
			'api.php?action=query&generator=search&gsrsearch=meaning&prop=info',
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQuerySearch.php 35098 2008-05-20 17:13:28Z ialex $';
	}
}
