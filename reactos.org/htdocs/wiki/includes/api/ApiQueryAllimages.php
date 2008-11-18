<?php

/*
 * Created on Mar 16, 2008
 *
 * API for MediaWiki 1.12+
 *
 * Copyright (C) 2008 Vasiliev Victor vasilvv@gmail.com,
 * based on ApiQueryAllpages.php
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
 * Query module to enumerate all available pages.
 *
 * @ingroup API
 */
class ApiQueryAllimages extends ApiQueryGeneratorBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'ai');
	}

	public function execute() {
		$this->run();
	}

	public function executeGenerator($resultPageSet) {
		if ($resultPageSet->isResolvingRedirects())
			$this->dieUsage('Use "gaifilterredir=nonredirects" option instead of "redirects" when using allimages as a generator', 'params');

		$this->run($resultPageSet);
	}

	private function run($resultPageSet = null) {
		$repo = RepoGroup::singleton()->getLocalRepo();
		if ( !$repo instanceof LocalRepo )
			$this->dieUsage('Local file repository does not support querying all images', 'unsupportedrepo');

		$db = $this->getDB();

		$params = $this->extractRequestParams();

		// Image filters
		if (!is_null($params['from']))
			$this->addWhere('img_name>=' . $db->addQuotes($this->titleToKey($params['from'])));
		if (isset ($params['prefix']))
			$this->addWhere("img_name LIKE '" . $db->escapeLike($this->titleToKey($params['prefix'])) . "%'");

		if (isset ($params['minsize'])) {
			$this->addWhere('img_size>=' . intval($params['minsize']));
		}

		if (isset ($params['maxsize'])) {
			$this->addWhere('img_size<=' . intval($params['maxsize']));
		}

		$sha1 = false;
		if( isset( $params['sha1'] ) ) {
			$sha1 = wfBaseConvert( $params['sha1'], 16, 36, 31 );
		} elseif( isset( $params['sha1base36'] ) ) {
			$sha1 = $params['sha1base36'];
		}
		if( $sha1 ) {
			$this->addWhere( 'img_sha1=' . $db->addQuotes( $sha1 ) );
		}

		$this->addTables('image');

		$prop = array_flip($params['prop']);
		$this->addFields( LocalFile::selectFields() );

		$limit = $params['limit'];
		$this->addOption('LIMIT', $limit+1);
		$this->addOption('ORDER BY', 'img_name' .
						($params['dir'] == 'descending' ? ' DESC' : ''));

		$res = $this->select(__METHOD__);

		$data = array ();
		$count = 0;
		$result = $this->getResult();
		while ($row = $db->fetchObject($res)) {
			if (++ $count > $limit) {
				// We've reached the one extra which shows that there are additional pages to be had. Stop here...
				// TODO: Security issue - if the user has no right to view next title, it will still be shown
				$this->setContinueEnumParameter('from', $this->keyToTitle($row->img_name));
				break;
			}

			if (is_null($resultPageSet)) {
				$file = $repo->newFileFromRow( $row );

				$data[] = ApiQueryImageInfo::getInfo( $file, $prop, $result );
			} else {
				$data[] = Title::makeTitle( NS_IMAGE, $row->img_name );
			}
		}
		$db->freeResult($res);

		if (is_null($resultPageSet)) {
			$result = $this->getResult();
			$result->setIndexedTagName($data, 'img');
			$result->addValue('query', $this->getModuleName(), $data);
		} else {
			$resultPageSet->populateFromTitles( $data );
		}
	}

	public function getAllowedParams() {
		return array (
			'from' => null,
			'prefix' => null,
			'minsize' => array (
				ApiBase :: PARAM_TYPE => 'integer',
			),
			'maxsize' => array (
				ApiBase :: PARAM_TYPE => 'integer',
			),
			'limit' => array (
				ApiBase :: PARAM_DFLT => 10,
				ApiBase :: PARAM_TYPE => 'limit',
				ApiBase :: PARAM_MIN => 1,
				ApiBase :: PARAM_MAX => ApiBase :: LIMIT_BIG1,
				ApiBase :: PARAM_MAX2 => ApiBase :: LIMIT_BIG2
			),
			'dir' => array (
				ApiBase :: PARAM_DFLT => 'ascending',
				ApiBase :: PARAM_TYPE => array (
					'ascending',
					'descending'
				)
			),
			'sha1' => null,
			'sha1base36' => null,
			'prop' => array (
				ApiBase :: PARAM_TYPE => array(
					'timestamp',
					'user',
					'comment',
					'url',
					'size',
					'dimensions', // Obsolete
					'mime',
					'sha1',
					'metadata'
				),
				ApiBase :: PARAM_DFLT => 'timestamp|url',
				ApiBase :: PARAM_ISMULTI => true
			)
		);
	}

	public function getParamDescription() {
		return array (
			'from' => 'The image title to start enumerating from.',
			'prefix' => 'Search for all image titles that begin with this value.',
			'dir' => 'The direction in which to list',
			'minsize' => 'Limit to images with at least this many bytes',
			'maxsize' => 'Limit to images with at most this many bytes',
			'limit' => 'How many total images to return.',
			'sha1' => 'SHA1 hash of image',
			'sha1base36' => 'SHA1 hash of image in base 36 (used in MediaWiki)',
			'prop' => 'Which properties to get',
		);
	}

	public function getDescription() {
		return 'Enumerate all images sequentially';
	}

	protected function getExamples() {
		return array (
			'Simple Use',
			' Show a list of images starting at the letter "B"',
			'  api.php?action=query&list=allimages&aifrom=B',
			'Using as Generator',
			' Show info about 4 images starting at the letter "T"',
			'  api.php?action=query&generator=allimages&gailimit=4&gaifrom=T&prop=imageinfo',
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryAllimages.php 37909 2008-07-22 13:26:15Z catrope $';
	}
}
