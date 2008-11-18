<?php

/*
 * Created on Oct 16, 2006
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
 * This is a three-in-one module to query:
 *   * backlinks  - links pointing to the given page,
 *   * embeddedin - what pages transclude the given page within themselves,
 *   * imageusage - what pages use the given image
 *
 * @ingroup API
 */
class ApiQueryBacklinks extends ApiQueryGeneratorBase {

	private $params, $rootTitle, $contRedirs, $contLevel, $contTitle, $contID, $redirID;

	// output element name, database column field prefix, database table
	private $backlinksSettings = array (
		'backlinks' => array (
			'code' => 'bl',
			'prefix' => 'pl',
			'linktbl' => 'pagelinks'
		),
		'embeddedin' => array (
			'code' => 'ei',
			'prefix' => 'tl',
			'linktbl' => 'templatelinks'
		),
		'imageusage' => array (
			'code' => 'iu',
			'prefix' => 'il',
			'linktbl' => 'imagelinks'
		)
	);

	public function __construct($query, $moduleName) {
		$code = $prefix = $linktbl = null;
		extract($this->backlinksSettings[$moduleName]);

		parent :: __construct($query, $moduleName, $code);
		$this->bl_ns = $prefix . '_namespace';
		$this->bl_from = $prefix . '_from';
		$this->bl_table = $linktbl;
		$this->bl_code = $code;

		$this->hasNS = $moduleName !== 'imageusage';
		if ($this->hasNS) {
			$this->bl_title = $prefix . '_title';
			$this->bl_sort = "{$this->bl_ns}, {$this->bl_title}, {$this->bl_from}";
			$this->bl_fields = array (
				$this->bl_ns,
				$this->bl_title
			);
		} else {
			$this->bl_title = $prefix . '_to';
			$this->bl_sort = "{$this->bl_title}, {$this->bl_from}";
			$this->bl_fields = array (
				$this->bl_title
			);
		}
	}

	public function execute() {
		$this->run();
	}

	public function executeGenerator($resultPageSet) {
		$this->run($resultPageSet);
	}

	private function prepareFirstQuery($resultPageSet = null) {
		/* SELECT page_id, page_title, page_namespace, page_is_redirect
		 * FROM pagelinks, page WHERE pl_from=page_id
		 * AND pl_title='Foo' AND pl_namespace=0
		 * LIMIT 11 ORDER BY pl_from
		 */
		$db = $this->getDb();
		$this->addTables(array('page', $this->bl_table));
		$this->addWhere("{$this->bl_from}=page_id");
		if(is_null($resultPageSet))
			$this->addFields(array('page_id', 'page_title', 'page_namespace'));
		else
			$this->addFields($resultPageSet->getPageTableFields());
		$this->addFields('page_is_redirect');
		$this->addWhereFld($this->bl_title, $this->rootTitle->getDbKey());
		if($this->hasNS)
			$this->addWhereFld($this->bl_ns, $this->rootTitle->getNamespace());
		$this->addWhereFld('page_namespace', $this->params['namespace']);
		if(!is_null($this->contID))
			$this->addWhere("page_id>={$this->contID}");
		if($this->params['filterredir'] == 'redirects')
			$this->addWhereFld('page_is_redirect', 1);
		if($this->params['filterredir'] == 'nonredirects')
			$this->addWhereFld('page_is_redirect', 0);
		$this->addOption('LIMIT', $this->params['limit'] + 1);
		$this->addOption('ORDER BY', $this->bl_from);
	}

	private function prepareSecondQuery($resultPageSet = null) {
		/* SELECT page_id, page_title, page_namespace, page_is_redirect, pl_title, pl_namespace
		 * FROM pagelinks, page WHERE pl_from=page_id
		 * AND (pl_title='Foo' AND pl_namespace=0) OR (pl_title='Bar' AND pl_namespace=1)
		 * LIMIT 11 ORDER BY pl_namespace, pl_title, pl_from
		 */
		$db = $this->getDb();
		$this->addTables(array('page', $this->bl_table));
		$this->addWhere("{$this->bl_from}=page_id");
		if(is_null($resultPageSet))
			$this->addFields(array('page_id', 'page_title', 'page_namespace', 'page_is_redirect'));
		else
			$this->addFields($resultPageSet->getPageTableFields());
		$this->addFields($this->bl_title);
		if($this->hasNS)
			$this->addFields($this->bl_ns);
		$titleWhere = '';
		foreach($this->redirTitles as $t)
			$titleWhere .= ($titleWhere != '' ? " OR " : '') .
					"({$this->bl_title} = ".$db->addQuotes($t->getDBKey()).
					($this->hasNS ? " AND {$this->bl_ns} = '{$t->getNamespace()}'" : "") .
					")";
		$this->addWhere($titleWhere);
		$this->addWhereFld('page_namespace', $this->params['namespace']);
		if(!is_null($this->redirID))
			$this->addWhere("page_id>={$this->redirID}");
		if($this->params['filterredir'] == 'redirects')
			$this->addWhereFld('page_is_redirect', 1);
		if($this->params['filterredir'] == 'nonredirects')
			$this->addWhereFld('page_is_redirect', 0);
		$this->addOption('LIMIT', $this->params['limit'] + 1);
		$this->addOption('ORDER BY', $this->bl_sort);
	}

	private function run($resultPageSet = null) {
		$this->params = $this->extractRequestParams(false);
		$this->redirect = isset($this->params['redirect']) && $this->params['redirect'];
		$userMax = ( $this->redirect ? ApiBase::LIMIT_BIG1/2 : ApiBase::LIMIT_BIG1 );
		$botMax  = ( $this->redirect ? ApiBase::LIMIT_BIG2/2 : ApiBase::LIMIT_BIG2 );
		if( $this->params['limit'] == 'max' ) {
			$this->params['limit'] = $this->getMain()->canApiHighLimits() ? $botMax : $userMax;
			$this->getResult()->addValue( 'limits', $this->getModuleName(), $this->params['limit'] );
		}

		$this->processContinue();
		$this->prepareFirstQuery($resultPageSet);

		$db = $this->getDB();
		$res = $this->select(__METHOD__);

		$count = 0;
		$this->data = array ();
		$this->continueStr = null;
		$this->redirTitles = array();
		while ($row = $db->fetchObject($res)) {
			if (++ $count > $this->params['limit']) {
				// We've reached the one extra which shows that there are additional pages to be had. Stop here...
				// Continue string preserved in case the redirect query doesn't pass the limit
				$this->continueStr = $this->getContinueStr($row->page_id);
				break;
			}

			if (is_null($resultPageSet))
				$this->extractRowInfo($row);
			else
			{
				if($row->page_is_redirect)
					$this->redirTitles[] = Title::makeTitle($row->page_namespace, $row->page_title);
				$resultPageSet->processDbRow($row);
			}
		}
		$db->freeResult($res);

		if($this->redirect && !empty($this->redirTitles))
		{
			$this->resetQueryParams();
			$this->prepareSecondQuery($resultPageSet);
			$res = $this->select(__METHOD__);
			$count = 0;
			while($row = $db->fetchObject($res))
			{
				if(++$count > $this->params['limit'])
				{
					// We've reached the one extra which shows that there are additional pages to be had. Stop here...
					// We need to keep the parent page of this redir in
					if($this->hasNS)
						$contTitle = Title::makeTitle($row->{$this->bl_ns}, $row->{$this->bl_title});
					else
						$contTitle = Title::makeTitle(NS_IMAGE, $row->{$this->bl_title});
					$this->continueStr = $this->getContinueRedirStr($contTitle->getArticleID(), $row->page_id);
					break;
				}

				if(is_null($resultPageSet))
					$this->extractRedirRowInfo($row);
				else
					$resultPageSet->processDbRow($row);
			}
			$db->freeResult($res);
		}
		if(!is_null($this->continueStr))
			$this->setContinueEnumParameter('continue', $this->continueStr);

		if (is_null($resultPageSet)) {
			$resultData = array();
			foreach($this->data as $ns => $a)
				foreach($a as $title => $arr)
					$resultData[$arr['pageid']] = $arr;
			$result = $this->getResult();
			$result->setIndexedTagName($resultData, $this->bl_code);
			$result->addValue('query', $this->getModuleName(), $resultData);
		}
	}

	private function extractRowInfo($row) {
		if(!isset($this->data[$row->page_namespace][$row->page_title])) {
			$this->data[$row->page_namespace][$row->page_title]['pageid'] = $row->page_id;
			ApiQueryBase::addTitleInfo($this->data[$row->page_namespace][$row->page_title], Title::makeTitle($row->page_namespace, $row->page_title));
			if($row->page_is_redirect)
			{
				$this->data[$row->page_namespace][$row->page_title]['redirect'] = '';
				$this->redirTitles[] = Title::makeTitle($row->page_namespace, $row->page_title);
			}
		}
	}

	private function extractRedirRowInfo($row)
	{
		$a['pageid'] = $row->page_id;
		ApiQueryBase::addTitleInfo($a, Title::makeTitle($row->page_namespace, $row->page_title));
		if($row->page_is_redirect)
			$a['redirect'] = '';
		$ns = $this->hasNS ? $row->{$this->bl_ns} : NS_IMAGE;
		$this->data[$ns][$row->{$this->bl_title}]['redirlinks'][] = $a;
		$this->getResult()->setIndexedTagName($this->data[$ns][$row->{$this->bl_title}]['redirlinks'], $this->bl_code);
	}

	protected function processContinue() {
		if (!is_null($this->params['continue']))
			$this->parseContinueParam();
		else {
			if ( $this->params['title'] !== "" ) {
				$title = Title::newFromText( $this->params['title'] );
				if ( !$title ) {
					$this->dieUsageMsg(array('invalidtitle', $this->params['title']));
				} else {
					$this->rootTitle = $title;
				}
			} else {
				$this->dieUsageMsg(array('missingparam', 'title'));
			}
		}

		// only image titles are allowed for the root in imageinfo mode
		if (!$this->hasNS && $this->rootTitle->getNamespace() !== NS_IMAGE)
			$this->dieUsage("The title for {$this->getModuleName()} query must be an image", 'bad_image_title');
	}

	protected function parseContinueParam() {
		$continueList = explode('|', $this->params['continue']);
		// expected format:
		// ns | key | id1 [| id2]
		// ns+key: root title
		// id1: first-level page ID to continue from
		// id2: second-level page ID to continue from

		// null stuff out now so we know what's set and what isn't
		$this->rootTitle = $this->contID = $this->redirID = null;
		$rootNs = intval($continueList[0]);
		if($rootNs === 0 && $continueList[0] !== '0')
			// Illegal continue parameter
			$this->dieUsage("Invalid continue param. You should pass the original value returned by the previous query", "_badcontinue");
		$this->rootTitle = Title::makeTitleSafe($rootNs, $continueList[1]);
		if(!$this->rootTitle)
			$this->dieUsage("Invalid continue param. You should pass the original value returned by the previous query", "_badcontinue");
		$contID = intval($continueList[2]);
		if($contID === 0 && $continueList[2] !== '0')
			$this->dieUsage("Invalid continue param. You should pass the original value returned by the previous query", "_badcontinue");
		$this->contID = $contID;
		$redirID = intval(@$continueList[3]);
		if($redirID === 0 && @$continueList[3] !== '0')
			// This one isn't required
			return;
		$this->redirID = $redirID;

	}

	protected function getContinueStr($lastPageID) {
		return $this->rootTitle->getNamespace() .
		'|' . $this->rootTitle->getDBkey() .
		'|' . $lastPageID;
	}

	protected function getContinueRedirStr($lastPageID, $lastRedirID) {
		return $this->getContinueStr($lastPageID) . '|' . $lastRedirID;
	}

	public function getAllowedParams() {
		$retval =  array (
			'title' => null,
			'continue' => null,
			'namespace' => array (
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_TYPE => 'namespace'
			),
			'filterredir' => array(
				ApiBase :: PARAM_DFLT => 'all',
				ApiBase :: PARAM_TYPE => array(
					'all',
					'redirects',
					'nonredirects'
				)
			),
			'limit' => array (
				ApiBase :: PARAM_DFLT => 10,
				ApiBase :: PARAM_TYPE => 'limit',
				ApiBase :: PARAM_MIN => 1,
				ApiBase :: PARAM_MAX => ApiBase :: LIMIT_BIG1,
				ApiBase :: PARAM_MAX2 => ApiBase :: LIMIT_BIG2
			)
		);
		if($this->getModuleName() == 'embeddedin')
			return $retval;
		$retval['redirect'] = false;
		return $retval;
	}

	public function getParamDescription() {
		$retval = array (
			'title' => 'Title to search. If null, titles= parameter will be used instead, but will be obsolete soon.',
			'continue' => 'When more results are available, use this to continue.',
			'namespace' => 'The namespace to enumerate.',
			'filterredir' => 'How to filter for redirects'
		);
		if($this->getModuleName() != 'embeddedin')
			return array_merge($retval, array(
				'redirect' => 'If linking page is a redirect, find all pages that link to that redirect as well. Maximum limit is halved.',
				'limit' => "How many total pages to return. If {$this->bl_code}redirect is enabled, limit applies to each level separately."
			));
		return array_merge($retval, array(
			'limit' => "How many total pages to return."
		));
	}

	public function getDescription() {
		switch ($this->getModuleName()) {
			case 'backlinks' :
				return 'Find all pages that link to the given page';
			case 'embeddedin' :
				return 'Find all pages that embed (transclude) the given title';
			case 'imageusage' :
				return 'Find all pages that use the given image title.';
			default :
				ApiBase :: dieDebug(__METHOD__, 'Unknown module name');
		}
	}

	protected function getExamples() {
		static $examples = array (
			'backlinks' => array (
				"api.php?action=query&list=backlinks&bltitle=Main%20Page",
				"api.php?action=query&generator=backlinks&gbltitle=Main%20Page&prop=info"
			),
			'embeddedin' => array (
				"api.php?action=query&list=embeddedin&eititle=Template:Stub",
				"api.php?action=query&generator=embeddedin&geititle=Template:Stub&prop=info"
			),
			'imageusage' => array (
				"api.php?action=query&list=imageusage&iutitle=Image:Albert%20Einstein%20Head.jpg",
				"api.php?action=query&generator=imageusage&giutitle=Image:Albert%20Einstein%20Head.jpg&prop=info"
			)
		);

		return $examples[$this->getModuleName()];
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryBacklinks.php 37504 2008-07-10 14:28:09Z catrope $';
	}
}
