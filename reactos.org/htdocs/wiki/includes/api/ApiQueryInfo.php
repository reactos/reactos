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
 * A query module to show basic page information.
 *
 * @ingroup API
 */
class ApiQueryInfo extends ApiQueryBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'in');
	}

	public function requestExtraData($pageSet) {
		$pageSet->requestField('page_restrictions');
		$pageSet->requestField('page_is_redirect');
		$pageSet->requestField('page_is_new');
		$pageSet->requestField('page_counter');
		$pageSet->requestField('page_touched');
		$pageSet->requestField('page_latest');
		$pageSet->requestField('page_len');
	}

	protected function getTokenFunctions() {
		// tokenname => function
		// function prototype is func($pageid, $title)
		// should return token or false
		
		// Don't call the hooks twice
		if(isset($this->tokenFunctions))
			return $this->tokenFunctions;

		// If we're in JSON callback mode, no tokens can be obtained
		if(!is_null($this->getMain()->getRequest()->getVal('callback')))
			return array();

		$this->tokenFunctions = array(
			'edit' => array( 'ApiQueryInfo', 'getEditToken' ),
			'delete' => array( 'ApiQueryInfo', 'getDeleteToken' ),
			'protect' => array( 'ApiQueryInfo', 'getProtectToken' ),
			'move' => array( 'ApiQueryInfo', 'getMoveToken' ),
			'block' => array( 'ApiQueryInfo', 'getBlockToken' ),
			'unblock' => array( 'ApiQueryInfo', 'getUnblockToken' )
		);
		wfRunHooks('APIQueryInfoTokens', array(&$this->tokenFunctions));
		return $this->tokenFunctions;
	}

	public static function getEditToken($pageid, $title)
	{
		// We could check for $title->userCan('edit') here,
		// but that's too expensive for this purpose
		global $wgUser;
		if(!$wgUser->isAllowed('edit'))
			return false;
		
		// The edit token is always the same, let's exploit that
		static $cachedEditToken = null;
		if(!is_null($cachedEditToken))
			return $cachedEditToken;

		$cachedEditToken = $wgUser->editToken();
		return $cachedEditToken;
	}
	
	public static function getDeleteToken($pageid, $title)
	{
		global $wgUser;
		if(!$wgUser->isAllowed('delete'))
			return false;			

		static $cachedDeleteToken = null;
		if(!is_null($cachedDeleteToken))
			return $cachedDeleteToken;

		$cachedDeleteToken = $wgUser->editToken();
		return $cachedDeleteToken;
	}

	public static function getProtectToken($pageid, $title)
	{
		global $wgUser;
		if(!$wgUser->isAllowed('protect'))
			return false;

		static $cachedProtectToken = null;
		if(!is_null($cachedProtectToken))
			return $cachedProtectToken;

		$cachedProtectToken = $wgUser->editToken();
		return $cachedProtectToken;
	}

	public static function getMoveToken($pageid, $title)
	{
		global $wgUser;
		if(!$wgUser->isAllowed('move'))
			return false;

		static $cachedMoveToken = null;
		if(!is_null($cachedMoveToken))
			return $cachedMoveToken;

		$cachedMoveToken = $wgUser->editToken();
		return $cachedMoveToken;
	}

	public static function getBlockToken($pageid, $title)
	{
		global $wgUser;
		if(!$wgUser->isAllowed('block'))
			return false;

		static $cachedBlockToken = null;
		if(!is_null($cachedBlockToken))
			return $cachedBlockToken;

		$cachedBlockToken = $wgUser->editToken();
		return $cachedBlockToken;
	}

	public static function getUnblockToken($pageid, $title)
	{
		// Currently, this is exactly the same as the block token
		return self::getBlockToken($pageid, $title);
	}

	public function execute() {

		global $wgUser;

		$params = $this->extractRequestParams();
		$fld_protection = $fld_talkid = $fld_subjectid = false;
		if(!is_null($params['prop'])) {
			$prop = array_flip($params['prop']);
			$fld_protection = isset($prop['protection']);
			$fld_talkid = isset($prop['talkid']);
			$fld_subjectid = isset($prop['subjectid']);
		}

		$pageSet = $this->getPageSet();
		$titles = $pageSet->getGoodTitles();
		$missing = $pageSet->getMissingTitles();
		$result = $this->getResult();

		$pageRestrictions = $pageSet->getCustomField('page_restrictions');
		$pageIsRedir = $pageSet->getCustomField('page_is_redirect');
		$pageIsNew = $pageSet->getCustomField('page_is_new');
		$pageCounter = $pageSet->getCustomField('page_counter');
		$pageTouched = $pageSet->getCustomField('page_touched');
		$pageLatest = $pageSet->getCustomField('page_latest');
		$pageLength = $pageSet->getCustomField('page_len');

		$db = $this->getDB();
		if ($fld_protection && !empty($titles)) {
			$this->addTables('page_restrictions');
			$this->addFields(array('pr_page', 'pr_type', 'pr_level', 'pr_expiry', 'pr_cascade'));
			$this->addWhereFld('pr_page', array_keys($titles));

			$res = $this->select(__METHOD__);
			while($row = $db->fetchObject($res)) {
				$a = array(
					'type' => $row->pr_type,
					'level' => $row->pr_level,
					'expiry' => Block::decodeExpiry( $row->pr_expiry, TS_ISO_8601 )
				);
				if($row->pr_cascade)
					$a['cascade'] = '';
				$protections[$row->pr_page][] = $a;
			}
			$db->freeResult($res);
			
			$imageIds = array();
			foreach ($titles as $id => $title)
				if ($title->getNamespace() == NS_IMAGE)
					$imageIds[] = $id;
			// To avoid code duplication
			$cascadeTypes = array(
				array(
					'prefix' => 'tl',
					'table' => 'templatelinks',
					'ns' => 'tl_namespace',
					'title' => 'tl_title',
					'ids' => array_diff(array_keys($titles), $imageIds)
				),
				array(
				 	'prefix' => 'il',
				 	'table' => 'imagelinks',
				 	'ns' => NS_IMAGE,
				 	'title' => 'il_to',
				 	'ids' => $imageIds
				)
			);
			
			foreach ($cascadeTypes as $type)
			{
				if (count($type['ids']) != 0) {
					$this->resetQueryParams();
					$this->addTables(array('page_restrictions', $type['table']));
					$this->addTables('page', 'page_source');
					$this->addTables('page', 'page_target');
					$this->addFields(array('pr_type', 'pr_level', 'pr_expiry', 
							'page_target.page_id AS page_target_id',
							'page_source.page_namespace AS page_source_namespace',
							'page_source.page_title AS page_source_title'));
					$this->addWhere(array("{$type['prefix']}_from = pr_page", 
							'page_target.page_namespace = '.$type['ns'], 
							'page_target.page_title = '.$type['title'],
							'page_source.page_id = pr_page'
					));
					$this->addWhereFld('pr_cascade', 1);
					$this->addWhereFld('page_target.page_id', $type['ids']);
				
					$res = $this->select(__METHOD__);
					while($row = $db->fetchObject($res)) {
						$source = Title::makeTitle($row->page_source_namespace, $row->page_source_title);
						$a = array(
							'type' => $row->pr_type,
							'level' => $row->pr_level,
							'expiry' => Block::decodeExpiry( $row->pr_expiry, TS_ISO_8601 ),
							'source' => $source->getPrefixedText()
						);
						$protections[$row->page_target_id][] = $a;
					}
					$db->freeResult($res);
				}
			}
		}

		// We don't need to check for pt stuff if there are no nonexistent titles
		if($fld_protection && !empty($missing))
		{
			$this->resetQueryParams();
			// Construct a custom WHERE clause that matches all titles in $missing
			$lb = new LinkBatch($missing);
			$this->addTables('protected_titles');
			$this->addFields(array('pt_title', 'pt_namespace', 'pt_create_perm', 'pt_expiry'));
			$this->addWhere($lb->constructSet('pt', $db));
			$res = $this->select(__METHOD__);
			$prottitles = array();
			while($row = $db->fetchObject($res)) {
				$prottitles[$row->pt_namespace][$row->pt_title][] = array(
					'type' => 'create',
					'level' => $row->pt_create_perm,
					'expiry' => Block::decodeExpiry($row->pt_expiry, TS_ISO_8601)
				);
			}
			$db->freeResult($res);
			
			$images = array();
			$others = array();
			foreach ($missing as $title)
				if ($title->getNamespace() == NS_IMAGE)
					$images[] = $title->getDbKey();
				else
					$others[] = $title;					
			
			if (count($others) != 0) {
				$lb = new LinkBatch($others);
				$this->resetQueryParams();
				$this->addTables(array('page_restrictions', 'page', 'templatelinks'));
				$this->addFields(array('pr_type', 'pr_level', 'pr_expiry', 
						'page_title', 'page_namespace',
						'tl_title', 'tl_namespace'));
				$this->addWhere($lb->constructSet('tl', $db));
				$this->addWhere('pr_page = page_id');
				$this->addWhere('pr_page = tl_from');
				$this->addWhereFld('pr_cascade', 1);
				
				$res = $this->select(__METHOD__);
				while($row = $db->fetchObject($res)) {
					$source = Title::makeTitle($row->page_namespace, $row->page_title);
					$a = array(
						'type' => $row->pr_type,
						'level' => $row->pr_level,
						'expiry' => Block::decodeExpiry( $row->pr_expiry, TS_ISO_8601 ),
						'source' => $source->getPrefixedText()
					);
					$prottitles[$row->tl_namespace][$row->tl_title][] = $a;
				}
				$db->freeResult($res);
			}
			
			if (count($images) != 0) {
				$this->resetQueryParams();
				$this->addTables(array('page_restrictions', 'page', 'imagelinks'));
				$this->addFields(array('pr_type', 'pr_level', 'pr_expiry', 
						'page_title', 'page_namespace', 'il_to'));
				$this->addWhere('pr_page = page_id');
				$this->addWhere('pr_page = il_from');
				$this->addWhereFld('pr_cascade', 1);
				$this->addWhereFld('il_to', $images);
				
				$res = $this->select(__METHOD__);
				while($row = $db->fetchObject($res)) {
					$source = Title::makeTitle($row->page_namespace, $row->page_title);
					$a = array(
						'type' => $row->pr_type,
						'level' => $row->pr_level,
						'expiry' => Block::decodeExpiry( $row->pr_expiry, TS_ISO_8601 ),
						'source' => $source->getPrefixedText()
					);
					$prottitles[NS_IMAGE][$row->il_to][] = $a;
				}
				$db->freeResult($res);
			}
		}

		// Run the talkid/subjectid query
		if($fld_talkid || $fld_subjectid)
		{
			$talktitles = $subjecttitles =
				$talkids = $subjectids = array();
			$everything = array_merge($titles, $missing);
			foreach($everything as $t)
			{
				if(MWNamespace::isTalk($t->getNamespace()))
				{
					if($fld_subjectid)
						$subjecttitles[] = $t->getSubjectPage();
				}
				else if($fld_talkid)
					$talktitles[] = $t->getTalkPage();
			}
			if(!empty($talktitles) || !empty($subjecttitles))
			{
				// Construct a custom WHERE clause that matches
				// all titles in $talktitles and $subjecttitles
				$lb = new LinkBatch(array_merge($talktitles, $subjecttitles));
				$this->resetQueryParams();
				$this->addTables('page');
				$this->addFields(array('page_title', 'page_namespace', 'page_id'));
				$this->addWhere($lb->constructSet('page', $db));
				$res = $this->select(__METHOD__);
				while($row = $db->fetchObject($res))
				{
					if(MWNamespace::isTalk($row->page_namespace))
						$talkids[MWNamespace::getSubject($row->page_namespace)][$row->page_title] = $row->page_id;
					else
						$subjectids[MWNamespace::getTalk($row->page_namespace)][$row->page_title] = $row->page_id;
				}
			}
		}

		foreach ( $titles as $pageid => $title ) {
			$pageInfo = array (
				'touched' => wfTimestamp(TS_ISO_8601, $pageTouched[$pageid]),
				'lastrevid' => intval($pageLatest[$pageid]),
				'counter' => intval($pageCounter[$pageid]),
				'length' => intval($pageLength[$pageid]),
			);

			if ($pageIsRedir[$pageid])
				$pageInfo['redirect'] = '';

			if ($pageIsNew[$pageid])
				$pageInfo['new'] = '';

			if (!is_null($params['token'])) {
				$tokenFunctions = $this->getTokenFunctions();
				foreach($params['token'] as $t)
				{
					$val = call_user_func($tokenFunctions[$t], $pageid, $title);
					if($val === false)
						$this->setWarning("Action '$t' is not allowed for the current user");
					else
						$pageInfo[$t . 'token'] = $val;
				}
			}

			if($fld_protection) {
				if (isset($protections[$pageid])) {
					$pageInfo['protection'] = $protections[$pageid];
					$result->setIndexedTagName($pageInfo['protection'], 'pr');
				} else {
					# Also check old restrictions
					if( $pageRestrictions[$pageid] ) {
						foreach( explode( ':', trim( $pageRestrictions[$pageid] ) ) as $restrict ) {
							$temp = explode( '=', trim( $restrict ) );
							if(count($temp) == 1) {
								// old old format should be treated as edit/move restriction
								$restriction = trim( $temp[0] );
								$pageInfo['protection'][] = array(
									'type' => 'edit',
									'level' => $restriction,
									'expiry' => 'infinity',
								);
								$pageInfo['protection'][] = array(
									'type' => 'move',
									'level' => $restriction,
									'expiry' => 'infinity',
								);
							} else {
								$restriction = trim( $temp[1] );
								$pageInfo['protection'][] = array(
									'type' => $temp[0],
									'level' => $restriction,
									'expiry' => 'infinity',
								);
							}
						}
						$result->setIndexedTagName($pageInfo['protection'], 'pr');
					} else {
						$pageInfo['protection'] = array();
					}
				}
			}
			if($fld_talkid && isset($talkids[$title->getNamespace()][$title->getDbKey()]))
				$pageInfo['talkid'] = $talkids[$title->getNamespace()][$title->getDbKey()];
			if($fld_subjectid && isset($subjectids[$title->getNamespace()][$title->getDbKey()]))
				$pageInfo['subjectid'] = $subjectids[$title->getNamespace()][$title->getDbKey()];

			$result->addValue(array (
				'query',
				'pages'
			), $pageid, $pageInfo);
		}

		// Get edit/protect tokens and protection data for missing titles if requested
		// Delete and move tokens are N/A for missing titles anyway
		if(!is_null($params['token']) || $fld_protection || $fld_talkid || $fld_subjectid)
		{
			$res = &$result->getData();
			foreach($missing as $pageid => $title) {
				if(!is_null($params['token'])) 
				{
					$tokenFunctions = $this->getTokenFunctions();
					foreach($params['token'] as $t)
					{
						$val = call_user_func($tokenFunctions[$t], $pageid, $title);
						if($val !== false)
							$res['query']['pages'][$pageid][$t . 'token'] = $val;
					}
				}
				if($fld_protection)
				{
					// Apparently the XML formatting code doesn't like array(null)
					// This is painful to fix, so we'll just work around it
					if(isset($prottitles[$title->getNamespace()][$title->getDBkey()]))
						$res['query']['pages'][$pageid]['protection'] = $prottitles[$title->getNamespace()][$title->getDBkey()];
					else
						$res['query']['pages'][$pageid]['protection'] = array();
					$result->setIndexedTagName($res['query']['pages'][$pageid]['protection'], 'pr');
				}
				if($fld_talkid && isset($talkids[$title->getNamespace()][$title->getDbKey()]))
					$res['query']['pages'][$pageid]['talkid'] = $talkids[$title->getNamespace()][$title->getDbKey()];
				if($fld_subjectid && isset($subjectids[$title->getNamespace()][$title->getDbKey()]))
					$res['query']['pages'][$pageid]['subjectid'] = $subjectids[$title->getNamespace()][$title->getDbKey()];
			}
		}
	}

	public function getAllowedParams() {
		return array (
			'prop' => array (
				ApiBase :: PARAM_DFLT => NULL,
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_TYPE => array (
					'protection',
					'talkid',
					'subjectid'
				)),
			'token' => array (
				ApiBase :: PARAM_DFLT => NULL,
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_TYPE => array_keys($this->getTokenFunctions())
			)
		);
	}

	public function getParamDescription() {
		return array (
			'prop' => array (
				'Which additional properties to get:',
				' "protection"   - List the protection level of each page',
				' "talkid"       - The page ID of the talk page for each non-talk page',
				' "subjectid"     - The page ID of the parent page for each talk page'
			),
			'token' => 'Request a token to perform a data-modifying action on a page',
		);
	}


	public function getDescription() {
		return 'Get basic page information such as namespace, title, last touched date, ...';
	}

	protected function getExamples() {
		return array (
			'api.php?action=query&prop=info&titles=Main%20Page',
			'api.php?action=query&prop=info&inprop=protection&titles=Main%20Page'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryInfo.php 37191 2008-07-06 18:43:06Z brion $';
	}
}
