<?php

/*
 * Created on Oct 22, 2006
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
class ApiFormatWddx extends ApiFormatBase {

	public function __construct($main, $format) {
		parent :: __construct($main, $format);
	}

	public function getMimeType() {
		return 'text/xml';
	}

	public function execute() {
		if (function_exists('wddx_serialize_value')) {
			$this->printText(wddx_serialize_value($this->getResultData()));
		} else {
			$this->printText('<?xml version="1.0" encoding="utf-8"?>');
			$this->printText('<wddxPacket version="1.0"><header/><data>');
			$this->slowWddxPrinter($this->getResultData());
			$this->printText('</data></wddxPacket>');
		}
	}

	/**
	* Recursivelly go through the object and output its data in WDDX format.
	*/
	function slowWddxPrinter($elemValue) {
		switch (gettype($elemValue)) {
			case 'array' :
				$this->printText('<struct>');
				foreach ($elemValue as $subElemName => $subElemValue) {
					$this->printText(wfElement('var', array (
						'name' => $subElemName
					), null));
					$this->slowWddxPrinter($subElemValue);
					$this->printText('</var>');
				}
				$this->printText('</struct>');
				break;
			case 'integer' :
			case 'double' :
				$this->printText(wfElement('number', null, $elemValue));
				break;
			case 'string' :
				$this->printText(wfElement('string', null, $elemValue));
				break;
			default :
				ApiBase :: dieDebug(__METHOD__, 'Unknown type ' . gettype($elemValue));
		}
	}

	public function getDescription() {
		return 'Output data in WDDX format' . parent :: getDescription();
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiFormatWddx.php 35098 2008-05-20 17:13:28Z ialex $';
	}
}
