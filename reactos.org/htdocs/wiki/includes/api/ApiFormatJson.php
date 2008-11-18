<?php

/*
 * Created on Sep 19, 2006
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
	require_once ('ApiFormatBase.php');
}

/**
 * @ingroup API
 */
class ApiFormatJson extends ApiFormatBase {

	private $mIsRaw;

	public function __construct($main, $format) {
		parent :: __construct($main, $format);
		$this->mIsRaw = ($format === 'rawfm');
	}

	public function getMimeType() {
		return 'application/json';
	}

	public function getNeedsRawData() {
		return $this->mIsRaw;
	}

	public function execute() {
		$prefix = $suffix = "";

		$params = $this->extractRequestParams();
		$callback = $params['callback'];
		if(!is_null($callback)) {
			$prefix = preg_replace("/[^][.\\'\\\"_A-Za-z0-9]/", "", $callback ) . "(";
			$suffix = ")";
		}

		if (!function_exists('json_encode') || $this->getIsHtml()) {
			$json = new Services_JSON();
			$this->printText($prefix . $json->encode($this->getResultData(), $this->getIsHtml()) . $suffix);
		} else {
			$this->printText($prefix . json_encode($this->getResultData()) . $suffix);
		}
	}

	public function getAllowedParams() {
		return array (
			'callback' => null
		);
	}

	public function getParamDescription() {
		return array (
			'callback' => 'If specified, wraps the output into a given function call. For safety, all user-specific data will be restricted.',
		);
	}

	public function getDescription() {
		if ($this->mIsRaw)
			return 'Output data with the debuging elements in JSON format' . parent :: getDescription();
		else
			return 'Output data in JSON format' . parent :: getDescription();
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiFormatJson.php 35098 2008-05-20 17:13:28Z ialex $';
	}
}
