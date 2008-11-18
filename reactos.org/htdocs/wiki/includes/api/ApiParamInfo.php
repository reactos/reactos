<?php

/*
 * Created on Dec 01, 2007
 *
 * API for MediaWiki 1.8+
 *
 * Copyright (C) 2008 Roan Kattouw <Firstname>.<Lastname>@home.nl
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
class ApiParamInfo extends ApiBase {

	public function __construct($main, $action) {
		parent :: __construct($main, $action);
	}

	public function execute() {
		// Get parameters
		$params = $this->extractRequestParams();
		$result = $this->getResult();
		$r = array();
		if(is_array($params['modules']))
		{
			$modArr = $this->getMain()->getModules();
			foreach($params['modules'] as $m)
			{
				if(!isset($modArr[$m]))
				{
					$r['modules'][] = array('name' => $m, 'missing' => '');
					continue;
				}
				$obj = new $modArr[$m]($this->getMain(), $m);
				$a = $this->getClassInfo($obj);
				$a['name'] = $m;
				$r['modules'][] = $a;
			}
			$result->setIndexedTagName($r['modules'], 'module');
		}
		if(is_array($params['querymodules']))
		{
			$queryObj = new ApiQuery($this->getMain(), 'query');
			$qmodArr = $queryObj->getModules();
			foreach($params['querymodules'] as $qm)
			{
				if(!isset($qmodArr[$qm]))
				{
					$r['querymodules'][] = array('name' => $qm, 'missing' => '');
					continue;
				}
				$obj = new $qmodArr[$qm]($this, $qm);
				$a = $this->getClassInfo($obj);
				$a['name'] = $qm;
				$r['querymodules'][] = $a;
			}
			$result->setIndexedTagName($r['querymodules'], 'module');
		}
		$result->addValue(null, $this->getModuleName(), $r);
	}

	function getClassInfo($obj)
	{
		$result = $this->getResult();
		$retval['classname'] = get_class($obj);
		$retval['description'] = (is_array($obj->getDescription()) ? implode("\n", $obj->getDescription()) : $obj->getDescription());
		$retval['prefix'] = $obj->getModulePrefix();
		$allowedParams = $obj->getAllowedParams();
		if(!is_array($allowedParams))
			return $retval;
		$retval['parameters'] = array();
		$paramDesc = $obj->getParamDescription();
		foreach($obj->getAllowedParams() as $n => $p)
		{
			$a = array('name' => $n);
			if(!is_array($p))
			{
				if(is_bool($p))
				{
					$a['type'] = 'bool';
					$a['default'] = ($p ? 'true' : 'false');
				}
				if(is_string($p))
					$a['default'] = $p;
				$retval['parameters'][] = $a;
				continue;
			}

			if(isset($p[ApiBase::PARAM_DFLT]))
				$a['default'] = $p[ApiBase::PARAM_DFLT];
			if(isset($p[ApiBase::PARAM_ISMULTI]))
				if($p[ApiBase::PARAM_ISMULTI])
					$a['multi'] = '';
			if(isset($p[ApiBase::PARAM_TYPE]))
			{
				$a['type'] = $p[ApiBase::PARAM_TYPE];
				if(is_array($a['type']))
					$result->setIndexedTagName($a['type'], 't');
			}
			if(isset($p[ApiBase::PARAM_MAX]))
				$a['max'] = $p[ApiBase::PARAM_MAX];
			if(isset($p[ApiBase::PARAM_MAX2]))
				$a['highmax'] = $p[ApiBase::PARAM_MAX2];
			if(isset($p[ApiBase::PARAM_MIN]))
				$a['min'] = $p[ApiBase::PARAM_MIN];
			if(isset($paramDesc[$n]))
				$a['description'] = (is_array($paramDesc[$n]) ? implode("\n", $paramDesc[$n]) : $paramDesc[$n]);
			$retval['parameters'][] = $a;
		}
		$result->setIndexedTagName($retval['parameters'], 'param');
		return $retval;
	}

	public function getAllowedParams() {
		return array (
			'modules' => array(
				ApiBase :: PARAM_ISMULTI => true
			),
			'querymodules' => array(
				ApiBase :: PARAM_ISMULTI => true
			)
		);
	}

	public function getParamDescription() {
		return array (
			'modules' => 'List of module names (value of the action= parameter)',
			'querymodules' => 'List of query module names (value of prop=, meta= or list= parameter)',
		);
	}

	public function getDescription() {
		return 'Obtain information about certain API parameters';
	}

	protected function getExamples() {
		return array (
			'api.php?action=paraminfo&modules=parse&querymodules=allpages|siteinfo'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiParamInfo.php 35098 2008-05-20 17:13:28Z ialex $';
	}
}
