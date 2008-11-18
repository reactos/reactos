<?php

/*
 * Created on August 16, 2007
 *
 * API for MediaWiki 1.8+
 *
 * Copyright (C) 2007 Iker Labarga <Firstname><Lastname>@gmail.com
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
 * A query module to list all external URLs found on a given set of pages.
 *
 * @ingroup API
 */
class ApiEditPage extends ApiBase {

	public function __construct($query, $moduleName) {
		parent :: __construct($query, $moduleName);
	}

	public function execute() {
		global $wgUser;
		$this->getMain()->requestWriteMode();

		$params = $this->extractRequestParams();
		if(is_null($params['title']))
			$this->dieUsageMsg(array('missingparam', 'title'));
		if(is_null($params['text']) && is_null($params['appendtext']) && is_null($params['prependtext']))
			$this->dieUsageMsg(array('missingtext'));
		if(is_null($params['token']))
			$this->dieUsageMsg(array('missingparam', 'token'));
		if(!$wgUser->matchEditToken($params['token']))
			$this->dieUsageMsg(array('sessionfailure'));

		$titleObj = Title::newFromText($params['title']);
		if(!$titleObj)
			$this->dieUsageMsg(array('invalidtitle', $params['title']));

		if($params['createonly'] && $titleObj->exists())
			$this->dieUsageMsg(array('createonly-exists'));
		if($params['nocreate'] && !$titleObj->exists())
			$this->dieUsageMsg(array('nocreate-missing'));

		// Now let's check whether we're even allowed to do this
		$errors = $titleObj->getUserPermissionsErrors('edit', $wgUser);
		if(!$titleObj->exists())
			$errors = array_merge($errors, $titleObj->getUserPermissionsErrors('create', $wgUser));
		if(!empty($errors))
			$this->dieUsageMsg($errors[0]);

		$articleObj = new Article($titleObj);
		$toMD5 = $params['text'];
		if(!is_null($params['appendtext']) || !is_null($params['prependtext']))
		{
			$content = $articleObj->getContent();
			$params['text'] = $params['prependtext'] . $content . $params['appendtext'];
			$toMD5 = $params['prependtext'] . $params['appendtext'];
		}

		# See if the MD5 hash checks out
		if(isset($params['md5']))
			if(md5($toMD5) !== $params['md5'])
				$this->dieUsageMsg(array('hashcheckfailed'));
		
		$ep = new EditPage($articleObj);
		// EditPage wants to parse its stuff from a WebRequest
		// That interface kind of sucks, but it's workable
		$reqArr = array('wpTextbox1' => $params['text'],
				'wpEdittoken' => $params['token'],
				'wpIgnoreBlankSummary' => ''
		);
		if(!is_null($params['summary']))
			$reqArr['wpSummary'] = $params['summary'];
		# Watch out for basetimestamp == ''
		# wfTimestamp() treats it as NOW, almost certainly causing an edit conflict
		if(!is_null($params['basetimestamp']) && $params['basetimestamp'] != '')
			$reqArr['wpEdittime'] = wfTimestamp(TS_MW, $params['basetimestamp']);
		else
			$reqArr['wpEdittime'] = $articleObj->getTimestamp();
		# Fake wpStartime
		$reqArr['wpStarttime'] = $reqArr['wpEdittime'];
		if($params['minor'] || (!$params['notminor'] && $wgUser->getOption('minordefault')))
			$reqArr['wpMinoredit'] = '';
		if($params['recreate'])
			$reqArr['wpRecreate'] = '';
		if(!is_null($params['section']))
		{
			$section = intval($params['section']);
			if($section == 0 && $params['section'] != '0' && $params['section'] != 'new')
				$this->dieUsage("The section parameter must be set to an integer or 'new'", "invalidsection");
			$reqArr['wpSection'] = $params['section'];
		}

		if($params['watch'])
			$watch = true;
		else if($params['unwatch'])
			$watch = false;
		else if($titleObj->userIsWatching())
			$watch = true;
		else if($wgUser->getOption('watchdefault'))
			$watch = true;
		else if($wgUser->getOption('watchcreations') && !$titleObj->exists())
			$watch = true;
		else
			$watch = false;
		if($watch)
			$reqArr['wpWatchthis'] = '';

		$req = new FauxRequest($reqArr, true);
		$ep->importFormData($req);

		# Run hooks
		# Handle CAPTCHA parameters
		global $wgRequest;
		if(isset($params['captchaid']))
			$wgRequest->data['wpCaptchaId'] = $params['captchaid'];
		if(isset($params['captchaword']))
			$wgRequest->data['wpCaptchaWord'] = $params['captchaword'];
		$r = array();
		if(!wfRunHooks('APIEditBeforeSave', array(&$ep, $ep->textbox1, &$r)))
		{
			if(!empty($r))
			{
				$r['result'] = "Failure";
				$this->getResult()->addValue(null, $this->getModuleName(), $r);
				return;
			}
			else
				$this->dieUsageMsg(array('hookaborted'));
		}

		# Do the actual save
		$oldRevId = $articleObj->getRevIdFetched();
		$result = null;
		# *Something* is setting $wgTitle to a title corresponding to "Msg",
		# but that breaks API mode detection through is_null($wgTitle)
		global $wgTitle;
		$wgTitle = null;
		# Fake $wgRequest for some hooks inside EditPage
		# FIXME: This interface SUCKS
		$oldRequest = $wgRequest;
		$wgRequest = $req;

		$retval = $ep->internalAttemptSave($result, $wgUser->isAllowed('bot') && $params['bot']);
		$wgRequest = $oldRequest;
		switch($retval)
		{
			case EditPage::AS_HOOK_ERROR:
			case EditPage::AS_HOOK_ERROR_EXPECTED:
				$this->dieUsageMsg(array('hookaborted'));
			case EditPage::AS_IMAGE_REDIRECT_ANON:
				$this->dieUsageMsg(array('noimageredirect-anon'));
			case EditPage::AS_IMAGE_REDIRECT_LOGGED:
				$this->dieUsageMsg(array('noimageredirect-logged'));
			case EditPage::AS_SPAM_ERROR:
				$this->dieUsageMsg(array('spamdetected', $result['spam']));
			case EditPage::AS_FILTERING:
				$this->dieUsageMsg(array('filtered'));
			case EditPage::AS_BLOCKED_PAGE_FOR_USER:
				$this->dieUsageMsg(array('blockedtext'));
			case EditPage::AS_MAX_ARTICLE_SIZE_EXCEEDED:
			case EditPage::AS_CONTENT_TOO_BIG:
				global $wgMaxArticleSize;
				$this->dieUsageMsg(array('contenttoobig', $wgMaxArticleSize));
			case EditPage::AS_READ_ONLY_PAGE_ANON:
				$this->dieUsageMsg(array('noedit-anon'));
			case EditPage::AS_READ_ONLY_PAGE_LOGGED:
				$this->dieUsageMsg(array('noedit'));
			case EditPage::AS_READ_ONLY_PAGE:
				$this->dieUsageMsg(array('readonlytext'));
			case EditPage::AS_RATE_LIMITED:
				$this->dieUsageMsg(array('actionthrottledtext'));
			case EditPage::AS_ARTICLE_WAS_DELETED:
				$this->dieUsageMsg(array('wasdeleted'));
			case EditPage::AS_NO_CREATE_PERMISSION:
				$this->dieUsageMsg(array('nocreate-loggedin'));
			case EditPage::AS_BLANK_ARTICLE:
				$this->dieUsageMsg(array('blankpage'));
			case EditPage::AS_CONFLICT_DETECTED:
				$this->dieUsageMsg(array('editconflict'));
			#case EditPage::AS_SUMMARY_NEEDED: Can't happen since we set wpIgnoreBlankSummary
			#case EditPage::AS_TEXTBOX_EMPTY: Can't happen since we don't do sections
			case EditPage::AS_END:
				# This usually means some kind of race condition
				# or DB weirdness occurred. Throw an unknown error here.
				$this->dieUsageMsg(array('unknownerror', 'AS_END'));
			case EditPage::AS_SUCCESS_NEW_ARTICLE:
				$r['new'] = '';
			case EditPage::AS_SUCCESS_UPDATE:
				$r['result'] = "Success";
				$r['pageid'] = $titleObj->getArticleID();
				$r['title'] = $titleObj->getPrefixedText();
				$newRevId = $titleObj->getLatestRevId();
				if($newRevId == $oldRevId)
					$r['nochange'] = '';
				else
				{
					$r['oldrevid'] = $oldRevId;
					$r['newrevid'] = $newRevId;
				}
				break;
			default:
				$this->dieUsageMsg(array('unknownerror', $retval));
		}
		$this->getResult()->addValue(null, $this->getModuleName(), $r);
	}

	public function mustBePosted() {
		return true;
	}

	protected function getDescription() {
		return 'Create and edit pages.';
	}

	protected function getAllowedParams() {
		return array (
			'title' => null,
			'section' => null,
			'text' => null,
			'token' => null,
			'summary' => null,
			'minor' => false,
			'notminor' => false,
			'bot' => false,
			'basetimestamp' => null,
			'recreate' => false,
			'createonly' => false,
			'nocreate' => false,
			'captchaword' => null,
			'captchaid' => null,
			'watch' => false,
			'unwatch' => false,
			'md5' => null,
			'prependtext' => null,
			'appendtext' => null,
		);
	}

	protected function getParamDescription() {
		return array (
			'title' => 'Page title',
			'section' => 'Section number. 0 for the top section, \'new\' for a new section',
			'text' => 'Page content',
			'token' => 'Edit token. You can get one of these through prop=info',
			'summary' => 'Edit summary. Also section title when section=new',
			'minor' => 'Minor edit',
			'notminor' => 'Non-minor edit',
			'bot' => 'Mark this edit as bot',
			'basetimestamp' => array('Timestamp of the base revision (gotten through prop=revisions&rvprop=timestamp).',
						'Used to detect edit conflicts; leave unset to ignore conflicts.'
			),
			'recreate' => 'Override any errors about the article having been deleted in the meantime',
			'createonly' => 'Don\'t edit the page if it exists already',
			'nocreate' => 'Throw an error if the page doesn\'t exist',
			'watch' => 'Add the page to your watchlist',
			'unwatch' => 'Remove the page from your watchlist',
			'captchaid' => 'CAPTCHA ID from previous request',
			'captchaword' => 'Answer to the CAPTCHA',
			'md5' => array(	'The MD5 hash of the text parameter, or the prependtext and appendtext parameters concatenated.',
				 	'If set, the edit won\'t be done unless the hash is correct'),
			'prependtext' => array( 'Add this text to the beginning of the page. Overrides text.',
						'Don\'t use together with section: that won\'t do what you expect.'),
			'appendtext' => 'Add this text to the end of the page. Overrides text',
		);
	}

	protected function getExamples() {
		return array (
			"Edit a page (anonymous user):",
			"    api.php?action=edit&title=Test&summary=test%20summary&text=article%20content&basetimestamp=20070824123454&token=%2B\\"
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiEditPage.php 36309 2008-06-15 20:37:28Z catrope $';
	}
}
