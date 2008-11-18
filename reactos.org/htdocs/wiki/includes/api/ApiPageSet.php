<?php

/*
 * Created on Sep 24, 2006
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
 * This class contains a list of pages that the client has requested.
 * Initially, when the client passes in titles=, pageids=, or revisions= parameter,
 * an instance of the ApiPageSet class will normalize titles,
 * determine if the pages/revisions exist, and prefetch any additional data page data requested.
 *
 * When generator is used, the result of the generator will become the input for the
 * second instance of this class, and all subsequent actions will go use the second instance
 * for all their work.
 *
 * @ingroup API
 */
class ApiPageSet extends ApiQueryBase {

	private $mAllPages; // [ns][dbkey] => page_id or negative when missing
	private $mTitles, $mGoodTitles, $mMissingTitles, $mInvalidTitles;
	private $mMissingPageIDs, $mRedirectTitles;
	private $mNormalizedTitles, $mInterwikiTitles;
	private $mResolveRedirects, $mPendingRedirectIDs;
	private $mGoodRevIDs, $mMissingRevIDs;
	private $mFakePageId;

	private $mRequestedPageFields;

	public function __construct($query, $resolveRedirects = false) {
		parent :: __construct($query, __CLASS__);

		$this->mAllPages = array ();
		$this->mTitles = array();
		$this->mGoodTitles = array ();
		$this->mMissingTitles = array ();
		$this->mInvalidTitles = array ();
		$this->mMissingPageIDs = array ();
		$this->mRedirectTitles = array ();
		$this->mNormalizedTitles = array ();
		$this->mInterwikiTitles = array ();
		$this->mGoodRevIDs = array();
		$this->mMissingRevIDs = array();

		$this->mRequestedPageFields = array ();
		$this->mResolveRedirects = $resolveRedirects;
		if($resolveRedirects)
			$this->mPendingRedirectIDs = array();

		$this->mFakePageId = -1;
	}

	public function isResolvingRedirects() {
		return $this->mResolveRedirects;
	}

	public function requestField($fieldName) {
		$this->mRequestedPageFields[$fieldName] = null;
	}

	public function getCustomField($fieldName) {
		return $this->mRequestedPageFields[$fieldName];
	}

	/**
	 * Get fields that modules have requested from the page table
	 */
	public function getPageTableFields() {
		// Ensure we get minimum required fields
		$pageFlds = array (
			'page_id' => null,
			'page_namespace' => null,
			'page_title' => null
		);

		// only store non-default fields
		$this->mRequestedPageFields = array_diff_key($this->mRequestedPageFields, $pageFlds);

		if ($this->mResolveRedirects)
			$pageFlds['page_is_redirect'] = null;

		$pageFlds = array_merge($pageFlds, $this->mRequestedPageFields);
		return array_keys($pageFlds);
	}

	/**
	 * Returns an array [ns][dbkey] => page_id for all requested titles.
	 * page_id is a unique negative number in case title was not found.
	 * Invalid titles will also have negative page IDs and will be in namespace 0
	 */
	public function getAllTitlesByNamespace() {
		return $this->mAllPages;
	}

	/**
	 * All Title objects provided.
	 * @return array of Title objects
	 */
	public function getTitles() {
		return $this->mTitles;
	}

	/**
	 * Returns the number of unique pages (not revisions) in the set.
	 */
	public function getTitleCount() {
		return count($this->mTitles);
	}

	/**
	 * Title objects that were found in the database.
	 * @return array page_id (int) => Title (obj)
	 */
	public function getGoodTitles() {
		return $this->mGoodTitles;
	}

	/**
	 * Returns the number of found unique pages (not revisions) in the set.
	 */
	public function getGoodTitleCount() {
		return count($this->mGoodTitles);
	}

	/**
	 * Title objects that were NOT found in the database.
	 * The array's index will be negative for each item
	 * @return array of Title objects
	 */
	public function getMissingTitles() {
		return $this->mMissingTitles;
	}

	/**
	 * Titles that were deemed invalid by Title::newFromText()
	 * The array's index will be unique and negative for each item
	 * @return array of strings (not Title objects)
	 */
	public function getInvalidTitles() {
		return $this->mInvalidTitles;
	}

	/**
	 * Page IDs that were not found in the database
	 * @return array of page IDs
	 */
	public function getMissingPageIDs() {
		return $this->mMissingPageIDs;
	}

	/**
	 * Get a list of redirects when doing redirect resolution
	 * @return array prefixed_title (string) => prefixed_title (string)
	 */
	public function getRedirectTitles() {
		return $this->mRedirectTitles;
	}

	/**
	 * Get a list of title normalizations - maps the title given
	 * with its normalized version.
	 * @return array raw_prefixed_title (string) => prefixed_title (string)
	 */
	public function getNormalizedTitles() {
		return $this->mNormalizedTitles;
	}

	/**
	 * Get a list of interwiki titles - maps the title given
	 * with to the interwiki prefix.
	 * @return array raw_prefixed_title (string) => interwiki_prefix (string)
	 */
	public function getInterwikiTitles() {
		return $this->mInterwikiTitles;
	}

	/**
	 * Get the list of revision IDs (requested with revids= parameter)
	 * @return array revID (int) => pageID (int)
	 */
	public function getRevisionIDs() {
		return $this->mGoodRevIDs;
	}

	/**
	 * Revision IDs that were not found in the database
	 * @return array of revision IDs
	 */
	public function getMissingRevisionIDs() {
		return $this->mMissingRevIDs;
	}

	/**
	 * Returns the number of revisions (requested with revids= parameter)
	 */
	public function getRevisionCount() {
		return count($this->getRevisionIDs());
	}

	/**
	 * Populate from the request parameters
	 */
	public function execute() {
		$this->profileIn();
		$titles = $pageids = $revids = null;
		extract($this->extractRequestParams());

		// Only one of the titles/pageids/revids is allowed at the same time
		$dataSource = null;
		if (isset ($titles))
			$dataSource = 'titles';
		if (isset ($pageids)) {
			if (isset ($dataSource))
				$this->dieUsage("Cannot use 'pageids' at the same time as '$dataSource'", 'multisource');
			$dataSource = 'pageids';
		}
		if (isset ($revids)) {
			if (isset ($dataSource))
				$this->dieUsage("Cannot use 'revids' at the same time as '$dataSource'", 'multisource');
			$dataSource = 'revids';
		}

		switch ($dataSource) {
			case 'titles' :
				$this->initFromTitles($titles);
				break;
			case 'pageids' :
				$this->initFromPageIds($pageids);
				break;
			case 'revids' :
				if($this->mResolveRedirects)
					$this->dieUsage('revids may not be used with redirect resolution', 'params');
				$this->initFromRevIDs($revids);
				break;
			default :
				// Do nothing - some queries do not need any of the data sources.
				break;
		}
		$this->profileOut();
	}

	/**
	 * Initialize PageSet from a list of Titles
	 */
	public function populateFromTitles($titles) {
		$this->profileIn();
		$this->initFromTitles($titles);
		$this->profileOut();
	}

	/**
	 * Initialize PageSet from a list of Page IDs
	 */
	public function populateFromPageIDs($pageIDs) {
		$this->profileIn();
		$this->initFromPageIds($pageIDs);
		$this->profileOut();
	}

	/**
	 * Initialize PageSet from a rowset returned from the database
	 */
	public function populateFromQueryResult($db, $queryResult) {
		$this->profileIn();
		$this->initFromQueryResult($db, $queryResult);
		$this->profileOut();
	}

	/**
	 * Initialize PageSet from a list of Revision IDs
	 */
	public function populateFromRevisionIDs($revIDs) {
		$this->profileIn();
		$revIDs = array_map('intval', $revIDs); // paranoia
		$this->initFromRevIDs($revIDs);
		$this->profileOut();
	}

	/**
	 * Extract all requested fields from the row received from the database
	 */
	public function processDbRow($row) {

		// Store Title object in various data structures
		$title = Title :: makeTitle($row->page_namespace, $row->page_title);

		$pageId = intval($row->page_id);
		$this->mAllPages[$row->page_namespace][$row->page_title] = $pageId;
		$this->mTitles[] = $title;

		if ($this->mResolveRedirects && $row->page_is_redirect == '1') {
			$this->mPendingRedirectIDs[$pageId] = $title;
		} else {
			$this->mGoodTitles[$pageId] = $title;
		}

		foreach ($this->mRequestedPageFields as $fieldName => & $fieldValues)
			$fieldValues[$pageId] = $row-> $fieldName;
	}

	public function finishPageSetGeneration() {
		$this->profileIn();
		$this->resolvePendingRedirects();
		$this->profileOut();
	}

	/**
	 * This method populates internal variables with page information
	 * based on the given array of title strings.
	 *
	 * Steps:
	 * #1 For each title, get data from `page` table
	 * #2 If page was not found in the DB, store it as missing
	 *
	 * Additionally, when resolving redirects:
	 * #3 If no more redirects left, stop.
	 * #4 For each redirect, get its links from `pagelinks` table.
	 * #5 Substitute the original LinkBatch object with the new list
	 * #6 Repeat from step #1
	 */
	private function initFromTitles($titles) {

		// Get validated and normalized title objects
		$linkBatch = $this->processTitlesArray($titles);
		if($linkBatch->isEmpty())
			return;

		$db = $this->getDB();
		$set = $linkBatch->constructSet('page', $db);

		// Get pageIDs data from the `page` table
		$this->profileDBIn();
		$res = $db->select('page', $this->getPageTableFields(), $set, __METHOD__);
		$this->profileDBOut();

		// Hack: get the ns:titles stored in array(ns => array(titles)) format
		$this->initFromQueryResult($db, $res, $linkBatch->data, true);	// process Titles

		// Resolve any found redirects
		$this->resolvePendingRedirects();
	}

	private function initFromPageIds($pageids) {
		if(empty($pageids))
			return;

		$pageids = array_map('intval', $pageids); // paranoia
		$set = array (
			'page_id' => $pageids
		);

		$db = $this->getDB();

		// Get pageIDs data from the `page` table
		$this->profileDBIn();
		$res = $db->select('page', $this->getPageTableFields(), $set, __METHOD__);
		$this->profileDBOut();

		$remaining = array_flip($pageids);
		$this->initFromQueryResult($db, $res, $remaining, false);	// process PageIDs

		// Resolve any found redirects
		$this->resolvePendingRedirects();
	}

	/**
	 * Iterate through the result of the query on 'page' table,
	 * and for each row create and store title object and save any extra fields requested.
	 * @param $db Database
	 * @param $res DB Query result
	 * @param $remaining Array of either pageID or ns/title elements (optional).
	 *        If given, any missing items will go to $mMissingPageIDs and $mMissingTitles
	 * @param $processTitles bool Must be provided together with $remaining.
	 *        If true, treat $remaining as an array of [ns][title]
	 *        If false, treat it as an array of [pageIDs]
	 * @return Array of redirect IDs (only when resolving redirects)
	 */
	private function initFromQueryResult($db, $res, &$remaining = null, $processTitles = null) {
		if (!is_null($remaining) && is_null($processTitles))
			ApiBase :: dieDebug(__METHOD__, 'Missing $processTitles parameter when $remaining is provided');

		while ($row = $db->fetchObject($res)) {

			$pageId = intval($row->page_id);

			// Remove found page from the list of remaining items
			if (isset($remaining)) {
				if ($processTitles)
					unset ($remaining[$row->page_namespace][$row->page_title]);
				else
					unset ($remaining[$pageId]);
			}

			// Store any extra fields requested by modules
			$this->processDbRow($row);
		}
		$db->freeResult($res);

		if(isset($remaining)) {
			// Any items left in the $remaining list are added as missing
			if($processTitles) {
				// The remaining titles in $remaining are non-existant pages
				foreach ($remaining as $ns => $dbkeys) {
					foreach ( $dbkeys as $dbkey => $unused ) {
						$title = Title :: makeTitle($ns, $dbkey);
						$this->mAllPages[$ns][$dbkey] = $this->mFakePageId;
						$this->mMissingTitles[$this->mFakePageId] = $title;
						$this->mFakePageId--;
						$this->mTitles[] = $title;
					}
				}
			}
			else
			{
				// The remaining pageids do not exist
				if(empty($this->mMissingPageIDs))
					$this->mMissingPageIDs = array_keys($remaining);
				else
					$this->mMissingPageIDs = array_merge($this->mMissingPageIDs, array_keys($remaining));
			}
		}
	}

	private function initFromRevIDs($revids) {

		if(empty($revids))
			return;

		$db = $this->getDB();
		$pageids = array();
		$remaining = array_flip($revids);

		$tables = array('revision');
		$fields = array('rev_id','rev_page');
		$where = array('rev_deleted' => 0, 'rev_id' => $revids);

		// Get pageIDs data from the `page` table
		$this->profileDBIn();
		$res = $db->select( $tables, $fields, $where,  __METHOD__ );
		while ( $row = $db->fetchObject( $res ) ) {
			$revid = intval($row->rev_id);
			$pageid = intval($row->rev_page);
			$this->mGoodRevIDs[$revid] = $pageid;
			$pageids[$pageid] = '';
			unset($remaining[$revid]);
		}
		$db->freeResult( $res );
		$this->profileDBOut();

		$this->mMissingRevIDs = array_keys($remaining);

		// Populate all the page information
		if($this->mResolveRedirects)
			ApiBase :: dieDebug(__METHOD__, 'revids may not be used with redirect resolution');
		$this->initFromPageIds(array_keys($pageids));
	}

	private function resolvePendingRedirects() {

		if($this->mResolveRedirects) {
			$db = $this->getDB();
			$pageFlds = $this->getPageTableFields();

			// Repeat until all redirects have been resolved
			// The infinite loop is prevented by keeping all known pages in $this->mAllPages
			while (!empty ($this->mPendingRedirectIDs)) {

				// Resolve redirects by querying the pagelinks table, and repeat the process
				// Create a new linkBatch object for the next pass
				$linkBatch = $this->getRedirectTargets();

				if ($linkBatch->isEmpty())
					break;

				$set = $linkBatch->constructSet('page', $db);
				if(false === $set)
					break;

				// Get pageIDs data from the `page` table
				$this->profileDBIn();
				$res = $db->select('page', $pageFlds, $set, __METHOD__);
				$this->profileDBOut();

				// Hack: get the ns:titles stored in array(ns => array(titles)) format
				$this->initFromQueryResult($db, $res, $linkBatch->data, true);
			}
		}
	}

	private function getRedirectTargets() {
		$lb = new LinkBatch();
		$db = $this->getDB();

		$this->profileDBIn();
		$res = $db->select('redirect', array(
				'rd_from',
				'rd_namespace',
				'rd_title'
			), array('rd_from' => array_keys($this->mPendingRedirectIDs)),
			__METHOD__
		);
		$this->profileDBOut();

		while($row = $db->fetchObject($res))
		{
			$rdfrom = intval($row->rd_from);
			$from = $this->mPendingRedirectIDs[$rdfrom]->getPrefixedText();
			$to = Title::makeTitle($row->rd_namespace, $row->rd_title)->getPrefixedText();
			unset($this->mPendingRedirectIDs[$rdfrom]);
			if(!isset($this->mAllPages[$row->rd_namespace][$row->rd_title]))
				$lb->add($row->rd_namespace, $row->rd_title);
			$this->mRedirectTitles[$from] = $to;
		}
		$db->freeResult($res);
		if(!empty($this->mPendingRedirectIDs))
		{
			# We found pages that aren't in the redirect table
			# Add them
			foreach($this->mPendingRedirectIDs as $id => $title)
			{
				$article = new Article($title);
				$rt = $article->insertRedirect();
				if(!$rt)
					# What the hell. Let's just ignore this
					continue;
				$lb->addObj($rt);
				$this->mRedirectTitles[$title->getPrefixedText()] = $rt->getPrefixedText();
				unset($this->mPendingRedirectIDs[$id]);
			}
		}
		return $lb;
	}

	/**
	 * Given an array of title strings, convert them into Title objects.
	 * Alternativelly, an array of Title objects may be given.
	 * This method validates access rights for the title,
	 * and appends normalization values to the output.
	 *
	 * @return LinkBatch of title objects.
	 */
	private function processTitlesArray($titles) {

		$linkBatch = new LinkBatch();

		foreach ($titles as $title) {

			$titleObj = is_string($title) ? Title :: newFromText($title) : $title;
			if (!$titleObj)
			{
				# Handle invalid titles gracefully
				$this->mAllpages[0][$title] = $this->mFakePageId;
				$this->mInvalidTitles[$this->mFakePageId] = $title;
				$this->mFakePageId--;
				continue; // There's nothing else we can do
			}
			$iw = $titleObj->getInterwiki();
			if (!empty($iw)) {
				// This title is an interwiki link.
				$this->mInterwikiTitles[$titleObj->getPrefixedText()] = $iw;
			} else {

				// Validation
				if ($titleObj->getNamespace() < 0)
					$this->dieUsage("No support for special pages has been implemented", 'unsupportednamespace');

				$linkBatch->addObj($titleObj);
			}

			// Make sure we remember the original title that was given to us
			// This way the caller can correlate new titles with the originally requested,
			// i.e. namespace is localized or capitalization is different
			if (is_string($title) && $title !== $titleObj->getPrefixedText()) {
				$this->mNormalizedTitles[$title] = $titleObj->getPrefixedText();
			}
		}

		return $linkBatch;
	}

	protected function getAllowedParams() {
		return array (
			'titles' => array (
				ApiBase :: PARAM_ISMULTI => true
			),
			'pageids' => array (
				ApiBase :: PARAM_TYPE => 'integer',
				ApiBase :: PARAM_ISMULTI => true
			),
			'revids' => array (
				ApiBase :: PARAM_TYPE => 'integer',
				ApiBase :: PARAM_ISMULTI => true
			)
		);
	}

	protected function getParamDescription() {
		return array (
			'titles' => 'A list of titles to work on',
			'pageids' => 'A list of page IDs to work on',
			'revids' => 'A list of revision IDs to work on'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiPageSet.php 35098 2008-05-20 17:13:28Z ialex $';
	}
}
