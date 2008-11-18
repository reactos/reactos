<?php

/*
 * Created on Sep 19, 2006
 *
 * API for MediaWiki 1.8+
 *
 * Copyright (C) 2006-2007 Yuri Astrakhan <Firstname><Lastname>@gmail.com,
 * Daniel Cannon (cannon dot danielc at gmail dot com)
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
	require_once ('ApiBase.php');
}

/**
 * Unit to authenticate log-in attempts to the current wiki.
 *
 * @ingroup API
 */
class ApiLogin extends ApiBase {

	/**
	 * Time (in seconds) a user must wait after submitting
	 * a bad login (will be multiplied by the THROTTLE_FACTOR for each bad attempt)
	 */
	const THROTTLE_TIME = 5;

	/**
	 * The factor by which the wait-time in between authentication
	 * attempts is increased every failed attempt.
	 */
	const THROTTLE_FACTOR = 2;

	/**
	 * The maximum number of failed logins after which the wait increase stops.
	 */
	const THOTTLE_MAX_COUNT = 10;

	public function __construct($main, $action) {
		parent :: __construct($main, $action, 'lg');
	}

	/**
	 * Executes the log-in attempt using the parameters passed. If
	 * the log-in succeeeds, it attaches a cookie to the session
	 * and outputs the user id, username, and session token. If a
	 * log-in fails, as the result of a bad password, a nonexistant
	 * user, or any other reason, the host is cached with an expiry
	 * and no log-in attempts will be accepted until that expiry
	 * is reached. The expiry is $this->mLoginThrottle.
	 *
	 * @access public
	 */
	public function execute() {
		$name = $password = $domain = null;
		extract($this->extractRequestParams());

		$result = array ();

		// Make sure noone is trying to guess the password brut-force
		$nextLoginIn = $this->getNextLoginTimeout();
		if ($nextLoginIn > 0) {
			$result['result']  = 'NeedToWait';
			$result['details'] = "Please wait $nextLoginIn seconds before next log-in attempt";
			$result['wait'] = $nextLoginIn;
			$this->getResult()->addValue(null, 'login', $result);
			return;
		}

		$params = new FauxRequest(array (
			'wpName' => $name,
			'wpPassword' => $password,
			'wpDomain' => $domain,
			'wpRemember' => ''
		));

		// Init session if necessary
		if( session_id() == '' ) {
			wfSetupSession();
		}

		$loginForm = new LoginForm($params);
		switch ($loginForm->authenticateUserData()) {
			case LoginForm :: SUCCESS :
				global $wgUser, $wgCookiePrefix;

				$wgUser->setOption('rememberpassword', 1);
				$wgUser->setCookies();

				// Run hooks. FIXME: split back and frontend from this hook.
				// FIXME: This hook should be placed in the backend
				$injected_html = '';
				wfRunHooks('UserLoginComplete', array(&$wgUser, &$injected_html));

				$result['result'] = 'Success';
				$result['lguserid'] = $wgUser->getId();
				$result['lgusername'] = $wgUser->getName();
				$result['lgtoken'] = $wgUser->getToken();
				$result['cookieprefix'] = $wgCookiePrefix;
				$result['sessionid'] = session_id();
				break;

			case LoginForm :: NO_NAME :
				$result['result'] = 'NoName';
				break;
			case LoginForm :: ILLEGAL :
				$result['result'] = 'Illegal';
				break;
			case LoginForm :: WRONG_PLUGIN_PASS :
				$result['result'] = 'WrongPluginPass';
				break;
			case LoginForm :: NOT_EXISTS :
				$result['result'] = 'NotExists';
				break;
			case LoginForm :: WRONG_PASS :
				$result['result'] = 'WrongPass';
				break;
			case LoginForm :: EMPTY_PASS :
				$result['result'] = 'EmptyPass';
				break;
			case LoginForm :: CREATE_BLOCKED :
				$result['result'] = 'CreateBlocked';
				$result['details'] = 'Your IP address is blocked from account creation';
				break;
			default :
				ApiBase :: dieDebug(__METHOD__, 'Unhandled case value');
		}

		if ($result['result'] != 'Success' && !isset( $result['details'] ) ) {
			$delay = $this->cacheBadLogin();
			$result['wait'] = $delay;
			$result['details'] = "Please wait " . $delay . " seconds before next log-in attempt";
		}
		// if we were allowed to try to login, memcache is fine

		$this->getResult()->addValue(null, 'login', $result);
	}


	/**
	 * Caches a bad-login attempt associated with the host and with an
	 * expiry of $this->mLoginThrottle. These are cached by a key
	 * separate from that used by the captcha system--as such, logging
	 * in through the standard interface will get you a legal session
	 * and cookies to prove it, but will not remove this entry.
	 *
	 * Returns the number of seconds until next login attempt will be allowed.
	 *
	 * @access private
	 */
	private function cacheBadLogin() {
		global $wgMemc;

		$key = $this->getMemCacheKey();
		$val = $wgMemc->get( $key );

		$val['lastReqTime'] = time();
		if (!isset($val['count'])) {
			$val['count'] = 1;
		} else {
			$val['count'] = 1 + $val['count'];
		}

		$delay = ApiLogin::calculateDelay($val['count']);

		$wgMemc->delete($key);
		// Cache expiration should be the maximum timeout - to prevent a "try and wait" attack
		$wgMemc->add( $key, $val, ApiLogin::calculateDelay(ApiLogin::THOTTLE_MAX_COUNT) );

		return $delay;
	}

	/**
	 * How much time the client must wait before it will be
	 * allowed to try to log-in next.
	 * The return value is 0 if no wait is required.
	 */
	private function getNextLoginTimeout() {
		global $wgMemc;

		$val = $wgMemc->get($this->getMemCacheKey());

		$elapse = (time() - $val['lastReqTime']);  // in seconds
		$canRetryIn = ApiLogin::calculateDelay($val['count']) - $elapse;

		return $canRetryIn < 0 ? 0 : $canRetryIn;
	}

	/**
	 * Based on the number of previously attempted logins, returns
	 * the delay (in seconds) when the next login attempt will be allowed.
	 */
	private static function calculateDelay($count) {
		// Defensive programming
		$count = intval($count);
		$count = $count < 1 ? 1 : $count;
		$count = $count > self::THOTTLE_MAX_COUNT ? self::THOTTLE_MAX_COUNT : $count;

		return self::THROTTLE_TIME + self::THROTTLE_TIME * ($count - 1) * self::THROTTLE_FACTOR;
	}

	/**
	* Internal cache key for badlogin checks. Robbed from the
	* ConfirmEdit extension and modified to use a key unique to the
	* API login.3
	*
	* @return string
	* @access private
	*/
	private function getMemCacheKey() {
		return wfMemcKey( 'apilogin', 'badlogin', 'ip', wfGetIP() );
	}

	public function mustBePosted() { return true; }

	public function getAllowedParams() {
		return array (
			'name' => null,
			'password' => null,
			'domain' => null
		);
	}

	public function getParamDescription() {
		return array (
			'name' => 'User Name',
			'password' => 'Password',
			'domain' => 'Domain (optional)'
		);
	}

	public function getDescription() {
		return array (
			'This module is used to login and get the authentication tokens. ',
			'In the event of a successful log-in, a cookie will be attached',
			'to your session. In the event of a failed log-in, you will not ',
			'be able to attempt another log-in through this method for 5 seconds.',
			'This is to prevent password guessing by automated password crackers.'
		);
	}

	protected function getExamples() {
		return array(
			'api.php?action=login&lgname=user&lgpassword=password'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiLogin.php 35565 2008-05-29 19:23:37Z btongminh $';
	}
}
