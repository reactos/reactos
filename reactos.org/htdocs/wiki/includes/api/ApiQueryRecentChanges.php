<?php

/*
 * Created on Oct 19, 2006
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
 * A query action to enumerate the recent changes that were done to the wiki.
 * Various filters are supported.
 *
 * @ingroup API
 */
class ApiQueryRecentChanges extends ApiQueryBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'rc');
	}

	private $fld_comment = false, $fld_user = false, $fld_flags = false,
			$fld_timestamp = false, $fld_title = false, $fld_ids = false,
			$fld_sizes = false;

	/**
	 * Generates and outputs the result of this query based upon the provided parameters.
	 */
	public function execute() {
		/* Initialize vars */
		$limit = $prop = $namespace = $titles = $show = $type = $dir = $start = $end = null;

		/* Get the parameters of the request. */
		extract($this->extractRequestParams());

		/* Build our basic query. Namely, something along the lines of:
		 * SELECT * FROM recentchanges WHERE rc_timestamp > $start
		 * 		AND rc_timestamp < $end AND rc_namespace = $namespace
		 * 		AND rc_deleted = '0'
		 */
		$db = $this->getDB();
		$this->addTables('recentchanges');
		$this->addOption('USE INDEX', array('recentchanges' => 'rc_timestamp'));
		$this->addWhereRange('rc_timestamp', $dir, $start, $end);
		$this->addWhereFld('rc_namespace', $namespace);
		$this->addWhereFld('rc_deleted', 0);
		if(!empty($titles))
		{
			$lb = new LinkBatch;
			foreach($titles as $t)
			{
				$obj = Title::newFromText($t);
				$lb->addObj($obj);
				if($obj->getNamespace() < 0)
				{
					// LinkBatch refuses these, but we need them anyway
					if(!array_key_exists($obj->getNamespace(), $lb->data))
						$lb->data[$obj->getNamespace()] = array();
					$lb->data[$obj->getNamespace()][$obj->getDbKey()] = 1;
				}
			}
			$where = $lb->constructSet('rc', $this->getDb());
			if($where != '')
				$this->addWhere($where);
		}

		if(!is_null($type))
				$this->addWhereFld('rc_type', $this->parseRCType($type));

		if (!is_null($show)) {
			$show = array_flip($show);

			/* Check for conflicting parameters. */
			if ((isset ($show['minor']) && isset ($show['!minor']))
					|| (isset ($show['bot']) && isset ($show['!bot']))
					|| (isset ($show['anon']) && isset ($show['!anon']))
					|| (isset ($show['redirect']) && isset ($show['!redirect']))
					|| (isset ($show['patrolled']) && isset ($show['!patrolled']))) {

				$this->dieUsage("Incorrect parameter - mutually exclusive values may not be supplied", 'show');
			}
			
			// Check permissions
			global $wgUser;
			if((isset($show['patrolled']) || isset($show['!patrolled'])) && !$wgUser->isAllowed('patrol'))
				$this->dieUsage("You need the patrol right to request the patrolled flag", 'permissiondenied');

			/* Add additional conditions to query depending upon parameters. */
			$this->addWhereIf('rc_minor = 0', isset ($show['!minor']));
			$this->addWhereIf('rc_minor != 0', isset ($show['minor']));
			$this->addWhereIf('rc_bot = 0', isset ($show['!bot']));
			$this->addWhereIf('rc_bot != 0', isset ($show['bot']));
			$this->addWhereIf('rc_user = 0', isset ($show['anon']));
			$this->addWhereIf('rc_user != 0', isset ($show['!anon']));
			$this->addWhereIf('rc_patrolled = 0', isset($show['!patrolled']));
			$this->addWhereIf('rc_patrolled != 0', isset($show['patrolled']));
			$this->addWhereIf('page_is_redirect = 1', isset ($show['redirect']));
			// Don't throw log entries out the window here
			$this->addWhereIf('page_is_redirect = 0 OR page_is_redirect IS NULL', isset ($show['!redirect']));
		}

		/* Add the fields we're concerned with to out query. */
		$this->addFields(array (
			'rc_timestamp',
			'rc_namespace',
			'rc_title',
			'rc_type',
			'rc_moved_to_ns',
			'rc_moved_to_title'
		));

		/* Determine what properties we need to display. */
		if (!is_null($prop)) {
			$prop = array_flip($prop);

			/* Set up internal members based upon params. */
			$this->fld_comment = isset ($prop['comment']);
			$this->fld_user = isset ($prop['user']);
			$this->fld_flags = isset ($prop['flags']);
			$this->fld_timestamp = isset ($prop['timestamp']);
			$this->fld_title = isset ($prop['title']);
			$this->fld_ids = isset ($prop['ids']);
			$this->fld_sizes = isset ($prop['sizes']);
			$this->fld_redirect = isset($prop['redirect']);
			$this->fld_patrolled = isset($prop['patrolled']);

			global $wgUser;
			if($this->fld_patrolled && !$wgUser->isAllowed('patrol'))
				$this->dieUsage("You need the patrol right to request the patrolled flag", 'permissiondenied');

			/* Add fields to our query if they are specified as a needed parameter. */
			$this->addFieldsIf('rc_id', $this->fld_ids);
			$this->addFieldsIf('rc_cur_id', $this->fld_ids);
			$this->addFieldsIf('rc_this_oldid', $this->fld_ids);
			$this->addFieldsIf('rc_last_oldid', $this->fld_ids);
			$this->addFieldsIf('rc_comment', $this->fld_comment);
			$this->addFieldsIf('rc_user', $this->fld_user);
			$this->addFieldsIf('rc_user_text', $this->fld_user);
			$this->addFieldsIf('rc_minor', $this->fld_flags);
			$this->addFieldsIf('rc_bot', $this->fld_flags);
			$this->addFieldsIf('rc_new', $this->fld_flags);
			$this->addFieldsIf('rc_old_len', $this->fld_sizes);
			$this->addFieldsIf('rc_new_len', $this->fld_sizes);
			$this->addFieldsIf('rc_patrolled', $this->fld_patrolled);
			if($this->fld_redirect || isset($show['redirect']) || isset($show['!redirect']))
			{
				$this->addTables('page');
				$this->addJoinConds(array('page' => array('LEFT JOIN', array('rc_namespace=page_namespace', 'rc_title=page_title'))));
				$this->addFields('page_is_redirect');
			}
		}
		/* Specify the limit for our query. It's $limit+1 because we (possibly) need to
		 * generate a "continue" parameter, to allow paging. */
		$this->addOption('LIMIT', $limit +1);

		$data = array ();
		$count = 0;

		/* Perform the actual query. */
		$db = $this->getDB();
		$res = $this->select(__METHOD__);

		/* Iterate through the rows, adding data extracted from them to our query result. */
		while ($row = $db->fetchObject($res)) {
			if (++ $count > $limit) {
				// We've reached the one extra which shows that there are additional pages to be had. Stop here...
				$this->setContinueEnumParameter('start', wfTimestamp(TS_ISO_8601, $row->rc_timestamp));
				break;
			}

			/* Extract the data from a single row. */
			$vals = $this->extractRowInfo($row);

			/* Add that row's data to our final output. */
			if($vals)
				$data[] = $vals;
		}

		$db->freeResult($res);

		/* Format the result */
		$result = $this->getResult();
		$result->setIndexedTagName($data, 'rc');
		$result->addValue('query', $this->getModuleName(), $data);
	}

	/**
	 * Extracts from a single sql row the data needed to describe one recent change.
	 *
	 * @param $row The row from which to extract the data.
	 * @return An array mapping strings (descriptors) to their respective string values.
	 * @access private
	 */
	private function extractRowInfo($row) {
		/* If page was moved somewhere, get the title of the move target. */
		$movedToTitle = false;
		if (!empty($row->rc_moved_to_title))
			$movedToTitle = Title :: makeTitle($row->rc_moved_to_ns, $row->rc_moved_to_title);

		/* Determine the title of the page that has been changed. */
		$title = Title :: makeTitle($row->rc_namespace, $row->rc_title);

		/* Our output data. */
		$vals = array ();

		$type = intval ( $row->rc_type );

		/* Determine what kind of change this was. */
		switch ( $type ) {
		case RC_EDIT:  $vals['type'] = 'edit'; break;
		case RC_NEW:   $vals['type'] = 'new'; break;
		case RC_MOVE:  $vals['type'] = 'move'; break;
		case RC_LOG:   $vals['type'] = 'log'; break;
		case RC_MOVE_OVER_REDIRECT: $vals['type'] = 'move over redirect'; break;
		default: $vals['type'] = $type;
		}

		/* Create a new entry in the result for the title. */
		if ($this->fld_title) {
			ApiQueryBase :: addTitleInfo($vals, $title);
			if ($movedToTitle)
				ApiQueryBase :: addTitleInfo($vals, $movedToTitle, "new_");
		}

		/* Add ids, such as rcid, pageid, revid, and oldid to the change's info. */
		if ($this->fld_ids) {
			$vals['rcid'] = intval($row->rc_id);
			$vals['pageid'] = intval($row->rc_cur_id);
			$vals['revid'] = intval($row->rc_this_oldid);
			$vals['old_revid'] = intval( $row->rc_last_oldid );
		}

		/* Add user data and 'anon' flag, if use is anonymous. */
		if ($this->fld_user) {
			$vals['user'] = $row->rc_user_text;
			if(!$row->rc_user)
				$vals['anon'] = '';
		}

		/* Add flags, such as new, minor, bot. */
		if ($this->fld_flags) {
			if ($row->rc_bot)
				$vals['bot'] = '';
			if ($row->rc_new)
				$vals['new'] = '';
			if ($row->rc_minor)
				$vals['minor'] = '';
		}

		/* Add sizes of each revision. (Only available on 1.10+) */
		if ($this->fld_sizes) {
			$vals['oldlen'] = intval($row->rc_old_len);
			$vals['newlen'] = intval($row->rc_new_len);
		}

		/* Add the timestamp. */
		if ($this->fld_timestamp)
			$vals['timestamp'] = wfTimestamp(TS_ISO_8601, $row->rc_timestamp);

		/* Add edit summary / log summary. */
		if ($this->fld_comment && !empty ($row->rc_comment)) {
			$vals['comment'] = $row->rc_comment;
		}

		if ($this->fld_redirect)
			if($row->page_is_redirect)
				$vals['redirect'] = '';

		/* Add the patrolled flag */
		if ($this->fld_patrolled && $row->rc_patrolled == 1)
			$vals['patrolled'] = '';

		return $vals;
	}

	private function parseRCType($type)
	{
			if(is_array($type))
			{
					$retval = array();
					foreach($type as $t)
							$retval[] = $this->parseRCType($t);
					return $retval;
			}
			switch($type)
			{
					case 'edit': return RC_EDIT;
					case 'new': return RC_NEW;
					case 'log': return RC_LOG;
			}
	}

	public function getAllowedParams() {
		return array (
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
			'namespace' => array (
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_TYPE => 'namespace'
			),
			'titles' => array(
				ApiBase :: PARAM_ISMULTI => true
			),
			'prop' => array (
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_DFLT => 'title|timestamp|ids',
				ApiBase :: PARAM_TYPE => array (
					'user',
					'comment',
					'flags',
					'timestamp',
					'title',
					'ids',
					'sizes',
					'redirect',
					'patrolled'
				)
			),
			'show' => array (
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_TYPE => array (
					'minor',
					'!minor',
					'bot',
					'!bot',
					'anon',
					'!anon',
					'redirect',
					'!redirect',
					'patrolled',
					'!patrolled'
				)
			),
			'limit' => array (
				ApiBase :: PARAM_DFLT => 10,
				ApiBase :: PARAM_TYPE => 'limit',
				ApiBase :: PARAM_MIN => 1,
				ApiBase :: PARAM_MAX => ApiBase :: LIMIT_BIG1,
				ApiBase :: PARAM_MAX2 => ApiBase :: LIMIT_BIG2
			),
			'type' => array (
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_TYPE => array (
					'edit',
					'new',
					'log'
				)
			)
		);
	}

	public function getParamDescription() {
		return array (
			'start' => 'The timestamp to start enumerating from.',
			'end' => 'The timestamp to end enumerating.',
			'dir' => 'In which direction to enumerate.',
			'namespace' => 'Filter log entries to only this namespace(s)',
			'titles' => 'Filter log entries to only these page titles',
			'prop' => 'Include additional pieces of information',
			'show' => array (
				'Show only items that meet this criteria.',
				'For example, to see only minor edits done by logged-in users, set show=minor|!anon'
			),
			'type' => 'Which types of changes to show.',
			'limit' => 'How many total changes to return.'
		);
	}

	public function getDescription() {
		return 'Enumerate recent changes';
	}

	protected function getExamples() {
		return array (
			'api.php?action=query&list=recentchanges'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryRecentChanges.php 37909 2008-07-22 13:26:15Z catrope $';
	}
}
