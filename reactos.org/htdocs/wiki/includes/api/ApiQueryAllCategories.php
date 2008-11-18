<?php

/*
 * Created on December 12, 2007
 *
 * API for MediaWiki 1.8+
 *
 * Copyright (C) 2007 Roan Kattouw <Firstname>.<Lastname>@home.nl
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
 * Query module to enumerate all categories, even the ones that don't have
 * category pages.
 *
 * @ingroup API
 */
class ApiQueryAllCategories extends ApiQueryGeneratorBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'ac');
	}

	public function execute() {
		$this->run();
	}

	public function executeGenerator($resultPageSet) {
		$this->run($resultPageSet);
	}

	private function run($resultPageSet = null) {

		$db = $this->getDB();
		$params = $this->extractRequestParams();

		$this->addTables('category');
		$this->addFields('cat_title');

		if (!is_null($params['from']))
			$this->addWhere('cat_title>=' . $db->addQuotes($this->titleToKey($params['from'])));
		if (isset ($params['prefix']))
			$this->addWhere("cat_title LIKE '" . $db->escapeLike($this->titleToKey($params['prefix'])) . "%'");

		$this->addOption('LIMIT', $params['limit']+1);
		$this->addOption('ORDER BY', 'cat_title' . ($params['dir'] == 'descending' ? ' DESC' : ''));

		$prop = array_flip($params['prop']);
		$this->addFieldsIf( array( 'cat_pages', 'cat_subcats', 'cat_files' ), isset($prop['size']) );
		$this->addFieldsIf( 'cat_hidden', isset($prop['hidden']) );

		$res = $this->select(__METHOD__);

		$pages = array();
		$categories = array();
		$result = $this->getResult();
		$count = 0;
		while ($row = $db->fetchObject($res)) {
			if (++ $count > $params['limit']) {
				// We've reached the one extra which shows that there are additional cats to be had. Stop here...
				// TODO: Security issue - if the user has no right to view next title, it will still be shown
				$this->setContinueEnumParameter('from', $this->keyToTitle($row->cat_title));
				break;
			}

			// Normalize titles
			$titleObj = Title::makeTitle(NS_CATEGORY, $row->cat_title);
			if(!is_null($resultPageSet))
				$pages[] = $titleObj->getPrefixedText();
			else {
				$item = array();
				$result->setContent( $item, $titleObj->getText() );
				if( isset( $prop['size'] ) ) {
					$item['size'] = $row->cat_pages;
					$item['pages'] = $row->cat_pages - $row->cat_subcats - $row->cat_files;
					$item['files'] = $row->cat_files;
					$item['subcats'] = $row->cat_subcats;
				}
				if( isset( $prop['hidden'] ) && $row->cat_hidden )
					$item['hidden'] = '';
				$categories[] = $item;
			}
		}
		$db->freeResult($res);

		if (is_null($resultPageSet)) {
			$result->setIndexedTagName($categories, 'c');
			$result->addValue('query', $this->getModuleName(), $categories);
		} else {
			$resultPageSet->populateFromTitles($pages);
		}
	}

	public function getAllowedParams() {
		return array (
			'from' => null,
			'prefix' => null,
			'dir' => array(
				ApiBase :: PARAM_DFLT => 'ascending',
				ApiBase :: PARAM_TYPE => array(
					'ascending',
					'descending'
				),
			),
			'limit' => array (
				ApiBase :: PARAM_DFLT => 10,
				ApiBase :: PARAM_TYPE => 'limit',
				ApiBase :: PARAM_MIN => 1,
				ApiBase :: PARAM_MAX => ApiBase :: LIMIT_BIG1,
				ApiBase :: PARAM_MAX2 => ApiBase :: LIMIT_BIG2
			),
			'prop' => array (
				ApiBase :: PARAM_TYPE => array( 'size', 'hidden' ),
				ApiBase :: PARAM_DFLT => '',
				ApiBase :: PARAM_ISMULTI => true
			),
		);
	}

	public function getParamDescription() {
		return array (
			'from' => 'The category to start enumerating from.',
			'prefix' => 'Search for all category titles that begin with this value.',
			'dir' => 'Direction to sort in.',
			'limit' => 'How many categories to return.',
			'prop' => 'Which properties to get',
		);
	}

	public function getDescription() {
		return 'Enumerate all categories';
	}

	protected function getExamples() {
		return array (
			'api.php?action=query&list=allcategories&acprop=size',
			'api.php?action=query&generator=allcategories&gacprefix=List&prop=info',
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryAllCategories.php 36790 2008-06-29 22:26:23Z catrope $';
	}
}
