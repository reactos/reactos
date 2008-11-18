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
 * This action allows users to get their watchlist items in RSS/Atom formats.
 * When executed, it performs a nested call to the API to get the needed data,
 * and formats it in a proper format.
 *
 * @ingroup API
 */
class ApiFeedWatchlist extends ApiBase {

	public function __construct($main, $action) {
		parent :: __construct($main, $action);
	}

	/**
	 * This module uses a custom feed wrapper printer.
	 */
	public function getCustomPrinter() {
		return new ApiFormatFeedWrapper($this->getMain());
	}

	/**
	 * Make a nested call to the API to request watchlist items in the last $hours.
	 * Wrap the result as an RSS/Atom feed.
	 */
	public function execute() {

		global $wgFeedClasses, $wgFeedLimit, $wgSitename, $wgContLanguageCode;

		try {
			$params = $this->extractRequestParams();

			// limit to the number of hours going from now back
			$endTime = wfTimestamp(TS_MW, time() - intval($params['hours'] * 60 * 60));

			$dbr = wfGetDB( DB_SLAVE );
			// Prepare parameters for nested request
			$fauxReqArr = array (
				'action' => 'query',
				'meta' => 'siteinfo',
				'siprop' => 'general',
				'list' => 'watchlist',
				'wlprop' => 'title|user|comment|timestamp',
				'wldir' => 'older',		// reverse order - from newest to oldest
				'wlend' => $dbr->timestamp($endTime),	// stop at this time
				'wllimit' => (50 > $wgFeedLimit) ? $wgFeedLimit : 50
			);

			// Check for 'allrev' parameter, and if found, show all revisions to each page on wl.
			if ( ! is_null ( $params['allrev'] ) )  $fauxReqArr['wlallrev'] = '';

			// Create the request
			$fauxReq = new FauxRequest ( $fauxReqArr );

			// Execute
			$module = new ApiMain($fauxReq);
			$module->execute();

			// Get data array
			$data = $module->getResultData();

			$feedItems = array ();
			foreach ($data['query']['watchlist'] as $info) {
				$feedItems[] = $this->createFeedItem($info);
			}

			$feedTitle = $wgSitename . ' - ' . wfMsgForContent('watchlist') . ' [' . $wgContLanguageCode . ']';
			$feedUrl = SpecialPage::getTitleFor( 'Watchlist' )->getFullUrl();

			$feed = new $wgFeedClasses[$params['feedformat']] ($feedTitle, htmlspecialchars(wfMsgForContent('watchlist')), $feedUrl);

			ApiFormatFeedWrapper :: setResult($this->getResult(), $feed, $feedItems);

		} catch (Exception $e) {

			// Error results should not be cached
			$this->getMain()->setCacheMaxAge(0);

			$feedTitle = $wgSitename . ' - Error - ' . wfMsgForContent('watchlist') . ' [' . $wgContLanguageCode . ']';
			$feedUrl = SpecialPage::getTitleFor( 'Watchlist' )->getFullUrl();

			$feedFormat = isset($params['feedformat']) ? $params['feedformat'] : 'rss';
			$feed = new $wgFeedClasses[$feedFormat] ($feedTitle, htmlspecialchars(wfMsgForContent('watchlist')), $feedUrl);


			if ($e instanceof UsageException) {
				$errorCode = $e->getCodeString();
			} else {
				// Something is seriously wrong
				$errorCode = 'internal_api_error';
			}

			$errorText = $e->getMessage();
			$feedItems[] = new FeedItem("Error ($errorCode)", $errorText, "", "", "");
			ApiFormatFeedWrapper :: setResult($this->getResult(), $feed, $feedItems);
		}
	}

	private function createFeedItem($info) {
		$titleStr = $info['title'];
		$title = Title :: newFromText($titleStr);
		$titleUrl = $title->getFullUrl();
		$comment = isset( $info['comment'] ) ? $info['comment'] : null;
		$timestamp = $info['timestamp'];
		$user = $info['user'];

		$completeText = "$comment ($user)";

		return new FeedItem($titleStr, $completeText, $titleUrl, $timestamp, $user);
	}

	public function getAllowedParams() {
		global $wgFeedClasses;
		$feedFormatNames = array_keys($wgFeedClasses);
		return array (
			'feedformat' => array (
				ApiBase :: PARAM_DFLT => 'rss',
				ApiBase :: PARAM_TYPE => $feedFormatNames
			),
			'hours' => array (
				ApiBase :: PARAM_DFLT => 24,
				ApiBase :: PARAM_TYPE => 'integer',
				ApiBase :: PARAM_MIN => 1,
				ApiBase :: PARAM_MAX => 72,
			),
			'allrev' => null
		);
	}

	public function getParamDescription() {
		return array (
			'feedformat' => 'The format of the feed',
			'hours'      => 'List pages modified within this many hours from now',
			'allrev'     => 'Include multiple revisions of the same page within given timeframe.'
		);
	}

	public function getDescription() {
		return 'This module returns a watchlist feed';
	}

	protected function getExamples() {
		return array (
			'api.php?action=feedwatchlist'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiFeedWatchlist.php 35098 2008-05-20 17:13:28Z ialex $';
	}
}
