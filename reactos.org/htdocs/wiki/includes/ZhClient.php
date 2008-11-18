<?php

/**
 * Client for querying zhdaemon
 *
 */
class ZhClient {
	var $mHost, $mPort, $mFP, $mConnected;

	/**
	 * Constructor
	 *
	 * @access private
	 */
	function ZhClient($host, $port) {
		$this->mHost = $host;
		$this->mPort = $port;
		$this->mConnected = $this->connect();
	}

	/**
	 * Check if connection to zhdaemon is successful
	 */
	function isconnected() {
		return $this->mConnected;
	}

	/**
	 * Establish conncetion
	 *
	 * @access private
	 */
	function connect() {
		wfSuppressWarnings();
		$errno = $errstr = '';
		$this->mFP = fsockopen($this->mHost, $this->mPort, $errno, $errstr, 30);
		wfRestoreWarnings();
		if(!$this->mFP) {
			return false;
		}
		return true;
	}

	/**
	 * Query the daemon and return the result
	 *
	 * @access private
	 */
	function query($request) {
		if(!$this->mConnected)
			return false;

		fwrite($this->mFP, $request);

		$result=fgets($this->mFP, 1024);

		list($status, $len) = explode(" ", $result);
		if($status == 'ERROR') {
			//$len is actually the error code...
			print "zhdaemon error $len<br />\n";
			return false;
		}
		$bytesread=0;
		$data='';
		while(!feof($this->mFP) && $bytesread<$len) {
			$str= fread($this->mFP, $len-$bytesread);
			$bytesread += strlen($str);
			$data .= $str;
		}
		//data should be of length $len. otherwise something is wrong
		if(strlen($data) != $len)
			return false;
		return $data;
	}

	/**
	 * Convert the input to a different language variant
	 *
	 * @param $text string: input text
	 * @param $tolang string: language variant
	 * @return string the converted text
	 */
	function convert($text, $tolang) {
		$len = strlen($text);
		$q = "CONV $tolang $len\n$text";
		$result = $this->query($q);
		if(!$result)
			$result = $text;
		return $result;
	}

	/**
	 * Convert the input to all possible variants
	 *
	 * @param $text string: input text
	 * @return array langcode => converted_string
	 */
	function convertToAllVariants($text) {
		$len = strlen($text);
		$q = "CONV ALL $len\n$text";
		$result = $this->query($q);
		if(!$result)
			return false;
		list($infoline, $data) = explode('|', $result, 2);
		$info = explode(";", $infoline);
		$ret = array();
		$i=0;
		foreach($info as $variant) {
			list($code, $len) = explode(' ', $variant);
			$ret[strtolower($code)] = substr($data, $i, $len);
			$i+=$len;
		}
		return $ret;
	}
	/**
	 * Perform word segmentation
	 *
	 * @param $text string: input text
	 * @return string segmented text
	 */
	function segment($text) {
		$len = strlen($text);
		$q = "SEG $len\n$text";
		$result = $this->query($q);
		if(!$result) {// fallback to character based segmentation
			$result = ZhClientFake::segment($text);
		}
		return $result;
	}

	/**
	 * Close the connection
	 */
	function close() {
		fclose($this->mFP);
	}
}
