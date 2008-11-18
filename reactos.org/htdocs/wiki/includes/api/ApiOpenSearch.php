<?php

/*
 * Created on Oct 13, 2006
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
	require_once ("ApiBase.php");
}

/**
 * @ingroup API
 */
class ApiOpenSearch extends ApiBase {

	public function __construct($main, $action) {
		parent :: __construct($main, $action);
	}

	public function getCustomPrinter() {
		return $this->getMain()->createPrinterByName('json');
	}

	public function execute() {
		$params = $this->extractRequestParams();
		$search = $params['search'];
		$limit = $params['limit'];
		$namespaces = $params['namespace'];
		
		// Open search results may be stored for a very long time
		$this->getMain()->setCacheMaxAge(1200);

		$srchres = PrefixSearch::titleSearch( $search, $limit, $namespaces );

		// Set top level elements
		$result = $this->getResult();
		$result->addValue(null, 0, $search);
		$result->addValue(null, 1, $srchres);
	}

	public function getAllowedParams() {
		return array (
			'search' => null,
			'limit' => array (
				ApiBase :: PARAM_DFLT => 10,
				ApiBase :: PARAM_TYPE => 'limit',
				ApiBase :: PARAM_MIN => 1,
				ApiBase :: PARAM_MAX => 100,
				ApiBase :: PARAM_MAX2 => 100
			),
			'namespace' => array(
				ApiBase :: PARAM_DFLT => NS_MAIN,
				ApiBase :: PARAM_TYPE => 'namespace',
				ApiBase :: PARAM_ISMULTI => true
			),
		);
	}

	public function getParamDescription() {
		return array (
			'search' => 'Search string',
			'limit' => 'Maximum amount of results to return',
			'namespace' => 'Namespaces to search',
		);
	}

	public function getDescription() {
		return 'This module implements OpenSearch protocol';
	}

	protected function getExamples() {
		return array (
			'api.php?action=opensearch&search=Te'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiOpenSearch.php 35098 2008-05-20 17:13:28Z ialex $';
	}
}
