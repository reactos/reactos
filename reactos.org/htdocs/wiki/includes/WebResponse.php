<?php
/**
 * Allow programs to request this object from WebRequest::response()
 * and handle all outputting (or lack of outputting) via it.
 */
class WebResponse {

	/** Output a HTTP header */
	function header($string, $replace=true) {
		header($string,$replace);
	}

	/** Set the browser cookie */
	function setcookie($name, $value, $expire) {
		global $wgCookiePath, $wgCookieDomain, $wgCookieSecure;
		setcookie($name,$value,$expire, $wgCookiePath, $wgCookieDomain, $wgCookieSecure);
	}
}
