<?php

/*
 * Created on July 6, 2007
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
 * A query action to get image information and upload history.
 *
 * @ingroup API
 */
class ApiQueryImageInfo extends ApiQueryBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName, 'ii');
	}

	public function execute() {
		$params = $this->extractRequestParams();

		$prop = array_flip($params['prop']);

		if($params['urlheight'] != -1 && $params['urlwidth'] == -1)
			$this->dieUsage("iiurlheight cannot be used without iiurlwidth", 'iiurlwidth');
		
		if ( $params['urlwidth'] != -1 ) {
			$scale = array();
			$scale['width'] = $params['urlwidth'];
			$scale['height'] = $params['urlheight'];
		} else {
			$scale = null;
		}

		$pageIds = $this->getPageSet()->getAllTitlesByNamespace();
		if (!empty($pageIds[NS_IMAGE])) {
			
			$result = $this->getResult();
			$images = RepoGroup::singleton()->findFiles( array_keys( $pageIds[NS_IMAGE] ) );
			foreach ( $images as $img ) {
				$data = array();
				
				// Get information about the current version first
				// Check that the current version is within the start-end boundaries
				if((is_null($params['start']) || $img->getTimestamp() <= $params['start']) &&
						(is_null($params['end']) || $img->getTimestamp() >= $params['end'])) {
					$data[] = self::getInfo( $img, $prop, $result, $scale );
				}

				// Now get the old revisions
				// Get one more to facilitate query-continue functionality
				$count = count($data);
				$oldies = $img->getHistory($params['limit'] - $count + 1, $params['start'], $params['end']);
				foreach($oldies as $oldie) {
					if(++$count > $params['limit']) {
						// We've reached the extra one which shows that there are additional pages to be had. Stop here...
						// Only set a query-continue if there was only one title
						if(count($pageIds[NS_IMAGE]) == 1)
							$this->setContinueEnumParameter('start', $oldie->getTimestamp());
						break;
					}
					$data[] = self::getInfo( $oldie, $prop, $result );
				}

				$pageId = $pageIds[NS_IMAGE][ $img->getOriginalTitle()->getDBkey() ];
				$result->addValue(
					array( 'query', 'pages', intval( $pageId ) ),
					'imagerepository', $img->getRepoName()
				);
				$this->addPageSubItems($pageId, $data);
			}
			
			$missing = array_diff( array_keys( $pageIds[NS_IMAGE] ), array_keys( $images ) );
			foreach ( $missing as $title )
				$result->addValue(
					array( 'query', 'pages', intval( $pageIds[NS_IMAGE][$title] ) ),
					'imagerepository', ''
				);
		}
	}

	/**
	 * Get result information for an image revision
	 * @param File f The image
	 * @return array Result array
	 */
	static function getInfo($file, $prop, $result, $scale = null) {
		$vals = array();
		if( isset( $prop['timestamp'] ) )
			$vals['timestamp'] = wfTimestamp(TS_ISO_8601, $file->getTimestamp());
		if( isset( $prop['user'] ) ) {
			$vals['user'] = $file->getUser();
			if( !$file->getUser( 'id' ) )
				$vals['anon'] = '';
		}
		if( isset( $prop['size'] ) || isset( $prop['dimensions'] ) ) {
			$vals['size'] = intval( $file->getSize() );
			$vals['width'] = intval( $file->getWidth() );
			$vals['height'] = intval( $file->getHeight() );
		}
		if( isset( $prop['url'] ) ) {
			if( !is_null( $scale ) && !$file->isOld() ) {
				$thumb = $file->getThumbnail( $scale['width'], $scale['height'] );
				if( $thumb )
				{
					$vals['thumburl'] = wfExpandUrl( $thumb->getURL() );
					$vals['thumbwidth'] = $thumb->getWidth();
					$vals['thumbheight'] = $thumb->getHeight();
				}
			}
			$vals['url'] = $file->getFullURL();
			$vals['descriptionurl'] = wfExpandUrl( $file->getDescriptionUrl() );
		}
		if( isset( $prop['comment'] ) )
			$vals['comment'] = $file->getDescription();
		if( isset( $prop['sha1'] ) )
			$vals['sha1'] = wfBaseConvert( $file->getSha1(), 36, 16, 40 );
		if( isset( $prop['metadata'] ) ) {
			$metadata = $file->getMetadata();
			$vals['metadata'] = $metadata ? unserialize( $metadata ) : null;
			$result->setIndexedTagName_recursive( $vals['metadata'], 'meta' );
		}
		if( isset( $prop['mime'] ) ) 
			$vals['mime'] = $file->getMimeType();
		
		if( isset( $prop['archivename'] ) && $file->isOld() )
			$vals['archivename'] = $file->getArchiveName();

		return $vals;
	}

	public function getAllowedParams() {
		return array (
			'prop' => array (
				ApiBase :: PARAM_ISMULTI => true,
				ApiBase :: PARAM_DFLT => 'timestamp|user',
				ApiBase :: PARAM_TYPE => array (
					'timestamp',
					'user',
					'comment',
					'url',
					'size',
					'sha1',
					'mime',
					'metadata',
					'archivename'
				)
			),
			'limit' => array(
				ApiBase :: PARAM_TYPE => 'limit',
				ApiBase :: PARAM_DFLT => 1,
				ApiBase :: PARAM_MIN => 1,
				ApiBase :: PARAM_MAX => ApiBase :: LIMIT_BIG1,
				ApiBase :: PARAM_MAX2 => ApiBase :: LIMIT_BIG2
			),
			'start' => array(
				ApiBase :: PARAM_TYPE => 'timestamp'
			),
			'end' => array(
				ApiBase :: PARAM_TYPE => 'timestamp'
			),
			'urlwidth' => array(
				ApiBase :: PARAM_TYPE => 'integer',
				ApiBase :: PARAM_DFLT => -1
			),
			'urlheight' => array(
				ApiBase :: PARAM_TYPE => 'integer',
				ApiBase :: PARAM_DFLT => -1
			)
		);
	}

	public function getParamDescription() {
		return array (
			'prop' => 'What image information to get.',
			'limit' => 'How many image revisions to return',
			'start' => 'Timestamp to start listing from',
			'end' => 'Timestamp to stop listing at',
			'urlwidth' => array('If iiprop=url is set, a URL to an image scaled to this width will be returned.',
					    'Only the current version of the image can be scaled.'),
			'urlheight' => 'Similar to iiurlwidth. Cannot be used without iiurlwidth',
		);
	}

	public function getDescription() {
		return array (
			'Returns image information and upload history'
		);
	}

	protected function getExamples() {
		return array (
			'api.php?action=query&titles=Image:Albert%20Einstein%20Head.jpg&prop=imageinfo',
			'api.php?action=query&titles=Image:Test.jpg&prop=imageinfo&iilimit=50&iiend=20071231235959&iiprop=timestamp|user|url',
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiQueryImageInfo.php 37504 2008-07-10 14:28:09Z catrope $';
	}
}
