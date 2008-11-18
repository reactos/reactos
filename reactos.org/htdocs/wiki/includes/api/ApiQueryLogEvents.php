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
	require_once ('ApiQueryBase.php');
}

/**
 * Query action to List the log events, with optional filtering by various parameters.
 *
 * @ingroup API
 */
class ApiQueryLogEvents extends ApiQueryBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'le');
	}

	public function execute() {
		$params = $this->extractRequestParams();
		$db = $this->getDB();

		$prop = $params['prop'];
		$this->fld_ids = in_array('ids', $prop);
		$this->fld_title = in_array('title', $prop);
		$this->fld_type = in_array('type', $prop);
		$this->fld_user = in_array('user', $prop);
		$this->fld_timestamp = in_array('timestamp', $prop);
		$this->fld_comment = in_array('comment', $prop);
		$this->fld_details = in_array('details', $prop);

		list($tbl_logging, $tbl_page, $tbl_user) = $db->tableNamesN('logging', 'page', 'user');

		$hideLogs = LogEventsList::getExcludeClause($db);
		if($hideLogs !== false)
			$this->addWhere($hideLogs);

		// Order is significant here
		$this->addTables(array('user', 'page', 'logging'));
		$this->addJoinConds(array(
			'page' => array('LEFT JOIN',
				array(	'log_namespace=page_namespace',
					'log_title=page_title'))));
		$this->addWhere('user_id=log_user');
		$this->addOption('USE INDEX', array('logging' => 'times')); // default, may change

		$this->addFields(array (
			'log_type',
			'log_action',
			'log_timestamp',
		));

		$this->addFieldsIf('log_id', $this->fld_ids);
		$this->addFieldsIf('page_id', $this->fld_ids);
		$this->addFieldsIf('log_user', $this->fld_user);
		$this->addFieldsIf('user_name', $this->fld_user);
		$this->addFieldsIf('log_namespace', $this->fld_title);
		$this->addFieldsIf('log_title', $this->fld_title);
		$this->addFieldsIf('log_comment', $this->fld_comment);
		$this->addFieldsIf('log_params', $this->fld_details);

		$this->addWhereFld('log_deleted', 0);
		
		if( !is_null($params['type']) ) {
			$this->addWhereFld('log_type', $params['type']);
			$this->addOption('USE INDEX', array('logging' => array('type_time')));
		}
		
		$this->addWhereRange('log_timestamp', $params['dir'], $params['start'], $params['end']);

		$limit = $params['limit'];
		$this->addOption('LIMIT', $limit +1);

		$user = $params['user'];
		if (!is_null($user)) {
			$userid = $db->selectField('user', 'user_id', array (
				'user_name' => $user
			));
			if (!$userid)
				$this->dieUsage("User name $user not found", 'param_user');
			$this->addWhereFld('log_user', $userid);
			$this->addOption('USE INDEX', array('logging' => array('user_time','page_time')));
		}

		$title = $params['title'];
		if (!is_null($title)) {
			$titleObj = Title :: newFromText($title);
			if (is_null($titleObj))
				$this->dieUsage("Bad title value '$title'", 'param_title');
			$this->addWhereFld('log_namespace', $titleObj->getNamespace());
			$this->addWhereFld('log_title', $titleObj->getDBkey());
			$this->addOption('USE INDEX', array('logging' => array('user_time','page_time')));
		}

		$data = array ();
		$count = 0;
		$res = $this->select(__METHOD__);
		while ($row = $db->fetchObject($res)) {
			if (++ $count > $limit) {
				// We've reached the one extra which shows that there are additional pages to be had. Stop here...
				$this->setContinueEnumParameter('start', wfTimestamp(TS_ISO_8601, $row->log_timestamp));
				break;
			}

			$vals = $this->extractRowInfo($row);
			if($vals)
				$data[] = $vals;
		}
		$db->freeResult($res);

		$this->getResult()->setIndexedTagName($data, 'item');
		$this->getResult()->addValue('query', $this->getModuleName(), $data);
	}

	private function extractRowInfo($row) {
		$vals = array();

		if ($this->fld_ids) {
			$vals['logid'] = intval($row->log_id);
			$vals['pageid'] = intval($row->page_id);
		}

		if ($this->fld_title) {
			$title = Title :: makeTitle($row->log_namespace, $row->log_title);
			ApiQueryBase :: addTitleInfo($vals, $title);
		}

		if ($this->fld_type) {
			$vals['type'] = $row->log_type;
			$vals['action'] = $row->log_action;
		}

		if ($this->fld_details && $row->log_params !== '') {
			$params = explode("\n", $row->log_params);
			switch ($row->log_type) {
				case 'move':
					if (isset ($params[0])) {
						$title = Title :: newFromText($params[0]);
						if ($title) {
							$vals2 = array();
							ApiQueryBase :: addTitleInfo($vals2, $title, "new_");
							$vals[$row->log_type] = $vals2;
							$params = null;
						}
					}
					break;
				case 'patrol':
					$vals2 = array();
					list( $vals2['cur'], $vals2['prev'], $vals2['auto'] ) = $params;
					$vals[$row->log_type] = $vals2;
					$params = null;
					break;
				case 'rights':
					$vals2 = array();
					list( $vals2['old'], $vals2['new'] ) = $params;
					$vals[$row->log_type] = $vals2;
					$params = null;
					break;
				case 'block':
					$vals2 = array();
					list( $vals2['duration'], $vals2['flags'] ) = $params;
					$vals[$row->log_type] = $vals2;
					$params = null;
					break;
			}

			if (isset($params)) {
				$this->getResult()->setIndexedTagName($params, 'param');
				$vals = array_merge($vals, $params);
			}
		}

		if ($this->fld_user) {
			$vals['user'] = $row->user_name;
			if(!$row->log_user)
				$vals['anon'] = '';
		}
		if ($this->fld_timestamp) {
			$vals['timestamp'] = wfTimestamp(TS_ISO_8601, $row->log_timestamp);
		}
		if ($this->fld_comment && !empty ($row->log_comment)) {
			$vals['comment'] = $row->log_comment;
		}

		return $vals;
	}


	public function getAllowedParams() {
		global $wgLogTypes;
		return array (
			'prop' => array (
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_DFLT => 'ids|title|type|user|timestamp|comment|details',
				ApiBase :: PARAM_TYPE => array (
					'ids',
					'title',
					'type',
					'user',
					'timestamp',
					'comment',
					'details',
				)
			),
			'type' => array (
				ApiBase :: PARAM_TYPE => $wgLogTypes
			),
			'start' => array (
				ApiBase :: PARAM_TYPE => 'timestamp'
			),
			'end' => array (
				ApiBase :: PARAM_TYPE => 'timestamp'
			),
			'dir' => array (
				ApiBase :: PARAM_DFLT => 'older',
				ApiBase :: PARAM_TYPE => array (
					'newer',
					'older'
				)
			),
			'user' => null,
			'title' => null,
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
			'prop' => 'Which properties to get',
			'type' => 'Filter log entries to only this type(s)',
			'start' => 'The timestamp to start enumerating from.',
			'end' => 'The timestamp to end enumerating.',
			'dir' => 'In which direction to enumerate.',
			'user' => 'Filter entries to those made by the given user.',
			'title' => 'Filter entries to those related to a page.',
			'limit' => 'How many total event entries to return.'
		);
	}

	public function getDescription() {
		return 'Get events from logs.';
	}

	protected function getExamples() {
		return array (
			'api.php?action=query&list=logevents'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryLogEvents.php 35098 2008-05-20 17:13:28Z ialex $';
	}
}
