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
 * This query action adds a list of a specified user's contributions to the output.
 *
 * @ingroup API
 */
class ApiQueryContributions extends ApiQueryBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'uc');
	}

	private $params, $username;
	private $fld_ids = false, $fld_title = false, $fld_timestamp = false,
			$fld_comment = false, $fld_flags = false;

	public function execute() {

		// Parse some parameters
		$this->params = $this->extractRequestParams();

		$prop = array_flip($this->params['prop']);
		$this->fld_ids = isset($prop['ids']);
		$this->fld_title = isset($prop['title']);
		$this->fld_comment = isset($prop['comment']);
		$this->fld_flags = isset($prop['flags']);
		$this->fld_timestamp = isset($prop['timestamp']);

		// TODO: if the query is going only against the revision table, should this be done?
		$this->selectNamedDB('contributions', DB_SLAVE, 'contributions');
		$db = $this->getDB();

		if(isset($this->params['userprefix']))
		{
			$this->prefixMode = true;
			$this->userprefix = $this->params['userprefix'];
		}
		else
		{
			$this->usernames = array();
			if(!is_array($this->params['user']))
				$this->params['user'] = array($this->params['user']);
			foreach($this->params['user'] as $u)
				$this->prepareUsername($u);
			$this->prefixMode = false;
		}
		$this->prepareQuery();

		//Do the actual query.
		$res = $this->select( __METHOD__ );

		//Initialise some variables
		$data = array ();
		$count = 0;
		$limit = $this->params['limit'];

		//Fetch each row
		while ( $row = $db->fetchObject( $res ) ) {
			if (++ $count > $limit) {
				// We've reached the one extra which shows that there are additional pages to be had. Stop here...
				$this->setContinueEnumParameter('start', wfTimestamp(TS_ISO_8601, $row->rev_timestamp));
				break;
			}

			$vals = $this->extractRowInfo($row);
			if ($vals)
				$data[] = $vals;
		}

		//Free the database record so the connection can get on with other stuff
		$db->freeResult($res);

		//And send the whole shebang out as output.
		$this->getResult()->setIndexedTagName($data, 'item');
		$this->getResult()->addValue('query', $this->getModuleName(), $data);
	}

	/**
	 * Validate the 'user' parameter and set the value to compare
	 * against `revision`.`rev_user_text`
	 */
	private function prepareUsername($user) {
		if( $user ) {
			$name = User::isIP( $user )
				? $user
				: User::getCanonicalName( $user, 'valid' );
			if( $name === false ) {
				$this->dieUsage( "User name {$user} is not valid", 'param_user' );
			} else {
				$this->usernames[] = $name;
			}
		} else {
			$this->dieUsage( 'User parameter may not be empty', 'param_user' );
		}
	}

	/**
	 * Prepares the query and returns the limit of rows requested
	 */
	private function prepareQuery() {

		//We're after the revision table, and the corresponding page row for
		//anything we retrieve.
		$this->addTables(array('revision', 'page'));
		$this->addWhere('page_id=rev_page');

		$this->addWhereFld('rev_deleted', 0);
		// We only want pages by the specified users.
		if($this->prefixMode)
			$this->addWhere("rev_user_text LIKE '" . $this->getDb()->escapeLike($this->userprefix) . "%'");
		else
			$this->addWhereFld( 'rev_user_text', $this->usernames );
		// ... and in the specified timeframe.
		// Ensure the same sort order for rev_user_text and rev_timestamp
		// so our query is indexed
		$this->addWhereRange('rev_user_text', $this->params['dir'], null, null);
		$this->addWhereRange('rev_timestamp',
			$this->params['dir'], $this->params['start'], $this->params['end'] );
		$this->addWhereFld('page_namespace', $this->params['namespace']);

		$show = $this->params['show'];
		if (!is_null($show)) {
			$show = array_flip($show);
			if (isset ($show['minor']) && isset ($show['!minor']))
				$this->dieUsage("Incorrect parameter - mutually exclusive values may not be supplied", 'show');

			$this->addWhereIf('rev_minor_edit = 0', isset ($show['!minor']));
			$this->addWhereIf('rev_minor_edit != 0', isset ($show['minor']));
		}
		$this->addOption('LIMIT', $this->params['limit'] + 1);

		// Mandatory fields: timestamp allows request continuation
		// ns+title checks if the user has access rights for this page
		// user_text is necessary if multiple users were specified
		$this->addFields(array(
			'rev_timestamp',
			'page_namespace',
			'page_title',
			'rev_user_text',
			));

		$this->addFieldsIf('rev_page', $this->fld_ids);
		$this->addFieldsIf('rev_id', $this->fld_ids || $this->fld_flags);
		$this->addFieldsIf('page_latest', $this->fld_flags);
		// $this->addFieldsIf('rev_text_id', $this->fld_ids); // Should this field be exposed?
		$this->addFieldsIf('rev_comment', $this->fld_comment);
		$this->addFieldsIf('rev_minor_edit', $this->fld_flags);
		$this->addFieldsIf('page_is_new', $this->fld_flags);
	}

	/**
	 * Extract fields from the database row and append them to a result array
	 */
	private function extractRowInfo($row) {

		$vals = array();

		$vals['user'] = $row->rev_user_text;
		if ($this->fld_ids) {
			$vals['pageid'] = intval($row->rev_page);
			$vals['revid'] = intval($row->rev_id);
			// $vals['textid'] = intval($row->rev_text_id);	// todo: Should this field be exposed?
		}

		if ($this->fld_title)
			ApiQueryBase :: addTitleInfo($vals,
				Title :: makeTitle($row->page_namespace, $row->page_title));

		if ($this->fld_timestamp)
			$vals['timestamp'] = wfTimestamp(TS_ISO_8601, $row->rev_timestamp);

		if ($this->fld_flags) {
			if ($row->page_is_new)
				$vals['new'] = '';
			if ($row->rev_minor_edit)
				$vals['minor'] = '';
			if ($row->page_latest == $row->rev_id)
				$vals['top'] = '';
		}

		if ($this->fld_comment && !empty ($row->rev_comment))
			$vals['comment'] = $row->rev_comment;

		return $vals;
	}

	public function getAllowedParams() {
		return array (
			'limit' => array (
				ApiBase :: PARAM_DFLT => 10,
				ApiBase :: PARAM_TYPE => 'limit',
				ApiBase :: PARAM_MIN => 1,
				ApiBase :: PARAM_MAX => ApiBase :: LIMIT_BIG1,
				ApiBase :: PARAM_MAX2 => ApiBase :: LIMIT_BIG2
			),
			'start' => array (
				ApiBase :: PARAM_TYPE => 'timestamp'
			),
			'end' => array (
				ApiBase :: PARAM_TYPE => 'timestamp'
			),
			'user' => array (
				ApiBase :: PARAM_ISMULTI => true
			),
			'userprefix' => null,
			'dir' => array (
				ApiBase :: PARAM_DFLT => 'older',
				ApiBase :: PARAM_TYPE => array (
					'newer',
					'older'
				)
			),
			'namespace' => array (
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_TYPE => 'namespace'
			),
			'prop' => array (
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_DFLT => 'ids|title|timestamp|flags|comment',
				ApiBase :: PARAM_TYPE => array (
					'ids',
					'title',
					'timestamp',
					'comment',
					'flags'
				)
			),
			'show' => array (
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_TYPE => array (
					'minor',
					'!minor',
				)
			),
		);
	}

	public function getParamDescription() {
		return array (
			'limit' => 'The maximum number of contributions to return.',
			'start' => 'The start timestamp to return from.',
			'end' => 'The end timestamp to return to.',
			'user' => 'The user to retrieve contributions for.',
			'userprefix' => 'Retrieve contibutions for all users whose names begin with this value. Overrides ucuser.',
			'dir' => 'The direction to search (older or newer).',
			'namespace' => 'Only list contributions in these namespaces',
			'prop' => 'Include additional pieces of information',
			'show' => 'Show only items that meet this criteria, e.g. non minor edits only: show=!minor',
		);
	}

	public function getDescription() {
		return 'Get all edits by a user';
	}

	protected function getExamples() {
		return array (
			'api.php?action=query&list=usercontribs&ucuser=YurikBot',
			'api.php?action=query&list=usercontribs&ucuserprefix=217.121.114.',
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryUserContributions.php 37383 2008-07-09 11:44:49Z btongminh $';
	}
}
