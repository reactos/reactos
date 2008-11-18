<?php
// +-----------------------------------------------------------------------+
// | Copyright (c) 2002-2003, Richard Heyes                                |
// | All rights reserved.                                                  |
// |                                                                       |
// | Redistribution and use in source and binary forms, with or without    |
// | modification, are permitted provided that the following conditions    |
// | are met:                                                              |
// |                                                                       |
// | o Redistributions of source code must retain the above copyright      |
// |   notice, this list of conditions and the following disclaimer.       |
// | o Redistributions in binary form must reproduce the above copyright   |
// |   notice, this list of conditions and the following disclaimer in the |
// |   documentation and/or other materials provided with the distribution.|
// | o The names of the authors may not be used to endorse or promote      |
// |   products derived from this software without specific prior written  |
// |   permission.                                                         |
// |                                                                       |
// | THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   |
// | "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     |
// | LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR |
// | A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  |
// | OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, |
// | SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT      |
// | LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, |
// | DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY |
// | THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT   |
// | (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE |
// | OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  |
// |                                                                       |
// +-----------------------------------------------------------------------+
// | Author: Richard Heyes <richard@phpguru.org>                           |
// +-----------------------------------------------------------------------+
//
// $Id: Request.php,v 1.41 2004/12/10 14:42:57 avb Exp $
//
// HTTP_Request Class
//
// Simple example, (Fetches yahoo.com and displays it):
//
// $a = &new HTTP_Request('http://www.yahoo.com/');
// $a->sendRequest();
// echo $a->getResponseBody();
//

require_once(dirname(__FILE__) . '/../PEAR.php');
require_once(dirname(__FILE__) . '/../Net/Socket.php');
require_once(dirname(__FILE__) . '/../Net/URL.php');

define('HTTP_REQUEST_METHOD_GET',     'GET',     true);
define('HTTP_REQUEST_METHOD_HEAD',    'HEAD',    true);
define('HTTP_REQUEST_METHOD_POST',    'POST',    true);
define('HTTP_REQUEST_METHOD_PUT',     'PUT',     true);
define('HTTP_REQUEST_METHOD_DELETE',  'DELETE',  true);
define('HTTP_REQUEST_METHOD_OPTIONS', 'OPTIONS', true);
define('HTTP_REQUEST_METHOD_TRACE',   'TRACE',   true);

define('HTTP_REQUEST_HTTP_VER_1_0', '1.0', true);
define('HTTP_REQUEST_HTTP_VER_1_1', '1.1', true);

class HTTP_Request {

    /**
    * Instance of Net_URL
    * @var object Net_URL
    */
    var $_url;

    /**
    * Type of request
    * @var string
    */
    var $_method;

    /**
    * HTTP Version
    * @var string
    */
    var $_http;

    /**
    * Request headers
    * @var array
    */
    var $_requestHeaders;

    /**
    * Basic Auth Username
    * @var string
    */
    var $_user;
    
    /**
    * Basic Auth Password
    * @var string
    */
    var $_pass;

    /**
    * Socket object
    * @var object Net_Socket
    */
    var $_sock;
    
    /**
    * Proxy server
    * @var string
    */
    var $_proxy_host;
    
    /**
    * Proxy port
    * @var integer
    */
    var $_proxy_port;
    
    /**
    * Proxy username
    * @var string
    */
    var $_proxy_user;
    
    /**
    * Proxy password
    * @var string
    */
    var $_proxy_pass;

    /**
    * Post data
    * @var mixed
    */
    var $_postData;

   /**
    * Files to post 
    * @var array
    */
    var $_postFiles = array();

    /**
    * Connection timeout.
    * @var float
    */
    var $_timeout;
    
    /**
    * HTTP_Response object
    * @var object HTTP_Response
    */
    var $_response;
    
    /**
    * Whether to allow redirects
    * @var boolean
    */
    var $_allowRedirects;
    
    /**
    * Maximum redirects allowed
    * @var integer
    */
    var $_maxRedirects;
    
    /**
    * Current number of redirects
    * @var integer
    */
    var $_redirects;

   /**
    * Whether to append brackets [] to array variables
    * @var bool
    */
    var $_useBrackets = true;

   /**
    * Attached listeners
    * @var array
    */
    var $_listeners = array();

   /**
    * Whether to save response body in response object property  
    * @var bool
    */
    var $_saveBody = true;

   /**
    * Timeout for reading from socket (array(seconds, microseconds))
    * @var array
    */
    var $_readTimeout = null;

   /**
    * Options to pass to Net_Socket::connect. See stream_context_create
    * @var array
    */
    var $_socketOptions = null;

    /**
    * Constructor
    *
    * Sets up the object
    * @param    string  The url to fetch/access
    * @param    array   Associative array of parameters which can have the following keys:
    * <ul>
    *   <li>method         - Method to use, GET, POST etc (string)</li>
    *   <li>http           - HTTP Version to use, 1.0 or 1.1 (string)</li>
    *   <li>user           - Basic Auth username (string)</li>
    *   <li>pass           - Basic Auth password (string)</li>
    *   <li>proxy_host     - Proxy server host (string)</li>
    *   <li>proxy_port     - Proxy server port (integer)</li>
    *   <li>proxy_user     - Proxy auth username (string)</li>
    *   <li>proxy_pass     - Proxy auth password (string)</li>
    *   <li>timeout        - Connection timeout in seconds (float)</li>
    *   <li>allowRedirects - Whether to follow redirects or not (bool)</li>
    *   <li>maxRedirects   - Max number of redirects to follow (integer)</li>
    *   <li>useBrackets    - Whether to append [] to array variable names (bool)</li>
    *   <li>saveBody       - Whether to save response body in response object property (bool)</li>
    *   <li>readTimeout    - Timeout for reading / writing data over the socket (array (seconds, microseconds))</li>
    *   <li>socketOptions  - Options to pass to Net_Socket object (array)</li>
    * </ul>
    * @access public
    */
    function HTTP_Request($url = '', $params = array())
    {
        $this->_sock           = &new Net_Socket();
        $this->_method         =  HTTP_REQUEST_METHOD_GET;
        $this->_http           =  HTTP_REQUEST_HTTP_VER_1_1;
        $this->_requestHeaders = array();
        $this->_postData       = null;

        $this->_user = null;
        $this->_pass = null;

        $this->_proxy_host = null;
        $this->_proxy_port = null;
        $this->_proxy_user = null;
        $this->_proxy_pass = null;

        $this->_allowRedirects = false;
        $this->_maxRedirects   = 3;
        $this->_redirects      = 0;

        $this->_timeout  = null;
        $this->_response = null;

        foreach ($params as $key => $value) {
            $this->{'_' . $key} = $value;
        }

        if (!empty($url)) {
            $this->setURL($url);
        }

        // Default useragent
        $this->addHeader('User-Agent', 'PEAR HTTP_Request class ( http://pear.php.net/ )');

        // Make sure keepalives dont knobble us
        $this->addHeader('Connection', 'close');

        // Basic authentication
        if (!empty($this->_user)) {
            $this->_requestHeaders['Authorization'] = 'Basic ' . base64_encode($this->_user . ':' . $this->_pass);
        }

        // Use gzip encoding if possible
        // Avoid gzip encoding if using multibyte functions (see #1781)
        if (HTTP_REQUEST_HTTP_VER_1_1 == $this->_http && extension_loaded('zlib') &&
            0 == (2 & ini_get('mbstring.func_overload'))) {

            $this->addHeader('Accept-Encoding', 'gzip');
        }
    }
    
    /**
    * Generates a Host header for HTTP/1.1 requests
    *
    * @access private
    * @return string
    */
    function _generateHostHeader()
    {
        if ($this->_url->port != 80 AND strcasecmp($this->_url->protocol, 'http') == 0) {
            $host = $this->_url->host . ':' . $this->_url->port;

        } elseif ($this->_url->port != 443 AND strcasecmp($this->_url->protocol, 'https') == 0) {
            $host = $this->_url->host . ':' . $this->_url->port;

        } elseif ($this->_url->port == 443 AND strcasecmp($this->_url->protocol, 'https') == 0 AND strpos($this->_url->url, ':443') !== false) {
            $host = $this->_url->host . ':' . $this->_url->port;
        
        } else {
            $host = $this->_url->host;
        }

        return $host;
    }
    
    /**
    * Resets the object to its initial state (DEPRECATED).
    * Takes the same parameters as the constructor.
    *
    * @param  string $url    The url to be requested
    * @param  array  $params Associative array of parameters
    *                        (see constructor for details)
    * @access public
    * @deprecated deprecated since 1.2, call the constructor if this is necessary
    */
    function reset($url, $params = array())
    {
        $this->HTTP_Request($url, $params);
    }

    /**
    * Sets the URL to be requested
    *
    * @param  string The url to be requested
    * @access public
    */
    function setURL($url)
    {
        $this->_url = &new Net_URL($url, $this->_useBrackets);

        if (!empty($this->_url->user) || !empty($this->_url->pass)) {
            $this->setBasicAuth($this->_url->user, $this->_url->pass);
        }

        if (HTTP_REQUEST_HTTP_VER_1_1 == $this->_http) {
            $this->addHeader('Host', $this->_generateHostHeader());
        }
    }
    
    /**
    * Sets a proxy to be used
    *
    * @param string     Proxy host
    * @param int        Proxy port
    * @param string     Proxy username
    * @param string     Proxy password
    * @access public
    */
    function setProxy($host, $port = 8080, $user = null, $pass = null)
    {
        $this->_proxy_host = $host;
        $this->_proxy_port = $port;
        $this->_proxy_user = $user;
        $this->_proxy_pass = $pass;

        if (!empty($user)) {
            $this->addHeader('Proxy-Authorization', 'Basic ' . base64_encode($user . ':' . $pass));
        }
    }

    /**
    * Sets basic authentication parameters
    *
    * @param string     Username
    * @param string     Password
    */
    function setBasicAuth($user, $pass)
    {
        $this->_user = $user;
        $this->_pass = $pass;

        $this->addHeader('Authorization', 'Basic ' . base64_encode($user . ':' . $pass));
    }

    /**
    * Sets the method to be used, GET, POST etc.
    *
    * @param string     Method to use. Use the defined constants for this
    * @access public
    */
    function setMethod($method)
    {
        $this->_method = $method;
    }

    /**
    * Sets the HTTP version to use, 1.0 or 1.1
    *
    * @param string     Version to use. Use the defined constants for this
    * @access public
    */
    function setHttpVer($http)
    {
        $this->_http = $http;
    }

    /**
    * Adds a request header
    *
    * @param string     Header name
    * @param string     Header value
    * @access public
    */
    function addHeader($name, $value)
    {
        $this->_requestHeaders[$name] = $value;
    }

    /**
    * Removes a request header
    *
    * @param string     Header name to remove
    * @access public
    */
    function removeHeader($name)
    {
        if (isset($this->_requestHeaders[$name])) {
            unset($this->_requestHeaders[$name]);
        }
    }

    /**
    * Adds a querystring parameter
    *
    * @param string     Querystring parameter name
    * @param string     Querystring parameter value
    * @param bool       Whether the value is already urlencoded or not, default = not
    * @access public
    */
    function addQueryString($name, $value, $preencoded = false)
    {
        $this->_url->addQueryString($name, $value, $preencoded);
    }    
    
    /**
    * Sets the querystring to literally what you supply
    *
    * @param string     The querystring data. Should be of the format foo=bar&x=y etc
    * @param bool       Whether data is already urlencoded or not, default = already encoded
    * @access public
    */
    function addRawQueryString($querystring, $preencoded = true)
    {
        $this->_url->addRawQueryString($querystring, $preencoded);
    }

    /**
    * Adds postdata items
    *
    * @param string     Post data name
    * @param string     Post data value
    * @param bool       Whether data is already urlencoded or not, default = not
    * @access public
    */
    function addPostData($name, $value, $preencoded = false)
    {
        if ($preencoded) {
            $this->_postData[$name] = $value;
        } else {
            $this->_postData[$name] = $this->_arrayMapRecursive('urlencode', $value);
        }
    }

   /**
    * Recursively applies the callback function to the value
    * 
    * @param    mixed   Callback function
    * @param    mixed   Value to process
    * @access   private
    * @return   mixed   Processed value
    */
    function _arrayMapRecursive($callback, $value)
    {
        if (!is_array($value)) {
            return call_user_func($callback, $value);
        } else {
            $map = array();
            foreach ($value as $k => $v) {
                $map[$k] = $this->_arrayMapRecursive($callback, $v);
            }
            return $map;
        }
    }

   /**
    * Adds a file to upload
    * 
    * This also changes content-type to 'multipart/form-data' for proper upload
    * 
    * @access public
    * @param  string    name of file-upload field
    * @param  mixed     file name(s)
    * @param  mixed     content-type(s) of file(s) being uploaded
    * @return bool      true on success
    * @throws PEAR_Error
    */
    function addFile($inputName, $fileName, $contentType = 'application/octet-stream')
    {
        if (!is_array($fileName) && !is_readable($fileName)) {
            return PEAR::raiseError("File '{$fileName}' is not readable");
        } elseif (is_array($fileName)) {
            foreach ($fileName as $name) {
                if (!is_readable($name)) {
                    return PEAR::raiseError("File '{$name}' is not readable");
                }
            }
        }
        $this->addHeader('Content-Type', 'multipart/form-data');
        $this->_postFiles[$inputName] = array(
            'name' => $fileName,
            'type' => $contentType
        );
        return true;
    }

    /**
    * Adds raw postdata
    *
    * @param string     The data
    * @param bool       Whether data is preencoded or not, default = already encoded
    * @access public
    */
    function addRawPostData($postdata, $preencoded = true)
    {
        $this->_postData = $preencoded ? $postdata : urlencode($postdata);
    }

    /**
    * Clears any postdata that has been added (DEPRECATED). 
    * 
    * Useful for multiple request scenarios.
    *
    * @access public
    * @deprecated deprecated since 1.2
    */
    function clearPostData()
    {
        $this->_postData = null;
    }

    /**
    * Appends a cookie to "Cookie:" header
    * 
    * @param string $name cookie name
    * @param string $value cookie value
    * @access public
    */
    function addCookie($name, $value)
    {
        $cookies = isset($this->_requestHeaders['Cookie']) ? $this->_requestHeaders['Cookie']. '; ' : '';
        $this->addHeader('Cookie', $cookies . $name . '=' . $value);
    }
    
    /**
    * Clears any cookies that have been added (DEPRECATED). 
    * 
    * Useful for multiple request scenarios
    *
    * @access public
    * @deprecated deprecated since 1.2
    */
    function clearCookies()
    {
        $this->removeHeader('Cookie');
    }

    /**
    * Sends the request
    *
    * @access public
    * @param  bool   Whether to store response body in Response object property,
    *                set this to false if downloading a LARGE file and using a Listener
    * @return mixed  PEAR error on error, true otherwise
    */
    function sendRequest($saveBody = true)
    {
        if (!is_a($this->_url, 'Net_URL')) {
            return PEAR::raiseError('No URL given.');
        }

        $host = isset($this->_proxy_host) ? $this->_proxy_host : $this->_url->host;
        $port = isset($this->_proxy_port) ? $this->_proxy_port : $this->_url->port;

        // 4.3.0 supports SSL connections using OpenSSL. The function test determines
        // we running on at least 4.3.0
        if (strcasecmp($this->_url->protocol, 'https') == 0 AND function_exists('file_get_contents') AND extension_loaded('openssl')) {
            if (isset($this->_proxy_host)) {
                return PEAR::raiseError('HTTPS proxies are not supported.');
            }
            $host = 'ssl://' . $host;
        }

        // If this is a second request, we may get away without
        // re-connecting if they're on the same server
        if (PEAR::isError($err = $this->_sock->connect($host, $port, null, $this->_timeout, $this->_socketOptions)) ||
            PEAR::isError($err = $this->_sock->write($this->_buildRequest()))) {

            return $err;
        }
        if (!empty($this->_readTimeout)) {
            $this->_sock->setTimeout($this->_readTimeout[0], $this->_readTimeout[1]);
        }

        $this->_notify('sentRequest');

        // Read the response
        $this->_response = &new HTTP_Response($this->_sock, $this->_listeners);
        if (PEAR::isError($err = $this->_response->process($this->_saveBody && $saveBody)) ) {
            return $err;
        }

        // Check for redirection
        // Bugfix (PEAR) bug #18, 6 oct 2003 by Dave Mertens (headers are also stored lowercase, so we're gonna use them here)
        // some non RFC2616 compliant servers (scripts) are returning lowercase headers ('location: xxx')
        if (    $this->_allowRedirects
            AND $this->_redirects <= $this->_maxRedirects
            AND $this->getResponseCode() > 300
            AND $this->getResponseCode() < 399
            AND !empty($this->_response->_headers['location'])) {

            
            $redirect = $this->_response->_headers['location'];

            // Absolute URL
            if (preg_match('/^https?:\/\//i', $redirect)) {
                $this->_url = &new Net_URL($redirect);
                $this->addHeader('Host', $this->_generateHostHeader());
            // Absolute path
            } elseif ($redirect{0} == '/') {
                $this->_url->path = $redirect;
            
            // Relative path
            } elseif (substr($redirect, 0, 3) == '../' OR substr($redirect, 0, 2) == './') {
                if (substr($this->_url->path, -1) == '/') {
                    $redirect = $this->_url->path . $redirect;
                } else {
                    $redirect = dirname($this->_url->path) . '/' . $redirect;
                }
                $redirect = Net_URL::resolvePath($redirect);
                $this->_url->path = $redirect;
                
            // Filename, no path
            } else {
                if (substr($this->_url->path, -1) == '/') {
                    $redirect = $this->_url->path . $redirect;
                } else {
                    $redirect = dirname($this->_url->path) . '/' . $redirect;
                }
                $this->_url->path = $redirect;
            }

            $this->_redirects++;
            return $this->sendRequest($saveBody);

        // Too many redirects
        } elseif ($this->_allowRedirects AND $this->_redirects > $this->_maxRedirects) {
            return PEAR::raiseError('Too many redirects');
        }

        $this->_sock->disconnect();

        return true;
    }

    /**
    * Returns the response code
    *
    * @access public
    * @return mixed     Response code, false if not set
    */
    function getResponseCode()
    {
        return isset($this->_response->_code) ? $this->_response->_code : false;
    }

    /**
    * Returns either the named header or all if no name given
    *
    * @access public
    * @param string     The header name to return, do not set to get all headers
    * @return mixed     either the value of $headername (false if header is not present)
    *                   or an array of all headers
    */
    function getResponseHeader($headername = null)
    {
        if (!isset($headername)) {
            return isset($this->_response->_headers)? $this->_response->_headers: array();
        } else {
            return isset($this->_response->_headers[$headername]) ? $this->_response->_headers[$headername] : false;
        }
    }

    /**
    * Returns the body of the response
    *
    * @access public
    * @return mixed     response body, false if not set
    */
    function getResponseBody()
    {
        return isset($this->_response->_body) ? $this->_response->_body : false;
    }

    /**
    * Returns cookies set in response
    * 
    * @access public
    * @return mixed     array of response cookies, false if none are present
    */
    function getResponseCookies()
    {
        return isset($this->_response->_cookies) ? $this->_response->_cookies : false;
    }

    /**
    * Builds the request string
    *
    * @access private
    * @return string The request string
    */
    function _buildRequest()
    {
        $separator = ini_get('arg_separator.output');
        ini_set('arg_separator.output', '&');
        $querystring = ($querystring = $this->_url->getQueryString()) ? '?' . $querystring : '';
        ini_set('arg_separator.output', $separator);

        $host = isset($this->_proxy_host) ? $this->_url->protocol . '://' . $this->_url->host : '';
        $port = (isset($this->_proxy_host) AND $this->_url->port != 80) ? ':' . $this->_url->port : '';
        $path = (empty($this->_url->path)? '/': $this->_url->path) . $querystring;
        $url  = $host . $port . $path;

        $request = $this->_method . ' ' . $url . ' HTTP/' . $this->_http . "\r\n";

        if (HTTP_REQUEST_METHOD_POST != $this->_method && HTTP_REQUEST_METHOD_PUT != $this->_method) {
            $this->removeHeader('Content-Type');
        } else {
            if (empty($this->_requestHeaders['Content-Type'])) {
                // Add default content-type
                $this->addHeader('Content-Type', 'application/x-www-form-urlencoded');
            } elseif ('multipart/form-data' == $this->_requestHeaders['Content-Type']) {
                $boundary = 'HTTP_Request_' . md5(uniqid('request') . microtime());
                $this->addHeader('Content-Type', 'multipart/form-data; boundary=' . $boundary);
            }
        }

        // Request Headers
        if (!empty($this->_requestHeaders)) {
            foreach ($this->_requestHeaders as $name => $value) {
                $request .= $name . ': ' . $value . "\r\n";
            }
        }

        // No post data or wrong method, so simply add a final CRLF
        if ((HTTP_REQUEST_METHOD_POST != $this->_method && HTTP_REQUEST_METHOD_PUT != $this->_method) ||
            (empty($this->_postData) && empty($this->_postFiles))) {

            $request .= "\r\n";
        // Post data if it's an array
        } elseif ((!empty($this->_postData) && is_array($this->_postData)) || !empty($this->_postFiles)) {
            // "normal" POST request
            if (!isset($boundary)) {
                $postdata = implode('&', array_map(
                    create_function('$a', 'return $a[0] . \'=\' . $a[1];'), 
                    $this->_flattenArray('', $this->_postData)
                ));

            // multipart request, probably with file uploads
            } else {
                $postdata = '';
                if (!empty($this->_postData)) {
                    $flatData = $this->_flattenArray('', $this->_postData);
                    foreach ($flatData as $item) {
                        $postdata .= '--' . $boundary . "\r\n";
                        $postdata .= 'Content-Disposition: form-data; name="' . $item[0] . '"';
                        $postdata .= "\r\n\r\n" . urldecode($item[1]) . "\r\n";
                    }
                }
                foreach ($this->_postFiles as $name => $value) {
                    if (is_array($value['name'])) {
                        $varname       = $name . ($this->_useBrackets? '[]': '');
                    } else {
                        $varname       = $name;
                        $value['name'] = array($value['name']);
                    }
                    foreach ($value['name'] as $key => $filename) {
                        $fp   = fopen($filename, 'r');
                        $data = fread($fp, filesize($filename));
                        fclose($fp);
                        $basename = basename($filename);
                        $type     = is_array($value['type'])? @$value['type'][$key]: $value['type'];

                        $postdata .= '--' . $boundary . "\r\n";
                        $postdata .= 'Content-Disposition: form-data; name="' . $varname . '"; filename="' . $basename . '"';
                        $postdata .= "\r\nContent-Type: " . $type;
                        $postdata .= "\r\n\r\n" . $data . "\r\n";
                    }
                }
                $postdata .= '--' . $boundary . "\r\n";
            }
            $request .= 'Content-Length: ' . strlen($postdata) . "\r\n\r\n";
            $request .= $postdata;

        // Post data if it's raw
        } elseif(!empty($this->_postData)) {
            $request .= 'Content-Length: ' . strlen($this->_postData) . "\r\n\r\n";
            $request .= $this->_postData;
        }
        
        return $request;
    }

   /**
    * Helper function to change the (probably multidimensional) associative array
    * into the simple one.
    *
    * @param    string  name for item
    * @param    mixed   item's values
    * @return   array   array with the following items: array('item name', 'item value');
    */
    function _flattenArray($name, $values)
    {
        if (!is_array($values)) {
            return array(array($name, $values));
        } else {
            $ret = array();
            foreach ($values as $k => $v) {
                if (empty($name)) {
                    $newName = $k;
                } elseif ($this->_useBrackets) {
                    $newName = $name . '[' . $k . ']';
                } else {
                    $newName = $name;
                }
                $ret = array_merge($ret, $this->_flattenArray($newName, $v));
            }
            return $ret;
        }
    }


   /**
    * Adds a Listener to the list of listeners that are notified of
    * the object's events
    * 
    * @param    object   HTTP_Request_Listener instance to attach
    * @return   boolean  whether the listener was successfully attached
    * @access   public
    */
    function attach(&$listener)
    {
        if (!is_a($listener, 'HTTP_Request_Listener')) {
            return false;
        }
        $this->_listeners[$listener->getId()] =& $listener;
        return true;
    }


   /**
    * Removes a Listener from the list of listeners 
    * 
    * @param    object   HTTP_Request_Listener instance to detach
    * @return   boolean  whether the listener was successfully detached
    * @access   public
    */
    function detach(&$listener)
    {
        if (!is_a($listener, 'HTTP_Request_Listener') || 
            !isset($this->_listeners[$listener->getId()])) {
            return false;
        }
        unset($this->_listeners[$listener->getId()]);
        return true;
    }


   /**
    * Notifies all registered listeners of an event.
    * 
    * Events sent by HTTP_Request object
    * 'sentRequest': after the request was sent
    * Events sent by HTTP_Response object
    * 'gotHeaders': after receiving response headers (headers are passed in $data)
    * 'tick': on receiving a part of response body (the part is passed in $data)
    * 'gzTick': on receiving a gzip-encoded part of response body (ditto)
    * 'gotBody': after receiving the response body (passes the decoded body in $data if it was gzipped)
    * 
    * @param    string  Event name
    * @param    mixed   Additional data
    * @access   private
    */
    function _notify($event, $data = null)
    {
        foreach (array_keys($this->_listeners) as $id) {
            $this->_listeners[$id]->update($this, $event, $data);
        }
    }
}


/**
* Response class to complement the Request class
*/
class HTTP_Response
{
    /**
    * Socket object
    * @var object
    */
    var $_sock;

    /**
    * Protocol
    * @var string
    */
    var $_protocol;
    
    /**
    * Return code
    * @var string
    */
    var $_code;
    
    /**
    * Response headers
    * @var array
    */
    var $_headers;

    /**
    * Cookies set in response  
    * @var array
    */
    var $_cookies;

    /**
    * Response body
    * @var string
    */
    var $_body = '';

   /**
    * Used by _readChunked(): remaining length of the current chunk
    * @var string
    */
    var $_chunkLength = 0;

   /**
    * Attached listeners
    * @var array
    */
    var $_listeners = array();

    /**
    * Constructor
    *
    * @param  object Net_Socket     socket to read the response from
    * @param  array                 listeners attached to request
    * @return mixed PEAR Error on error, true otherwise
    */
    function HTTP_Response(&$sock, &$listeners)
    {
        $this->_sock      =& $sock;
        $this->_listeners =& $listeners;
    }


   /**
    * Processes a HTTP response
    * 
    * This extracts response code, headers, cookies and decodes body if it 
    * was encoded in some way
    *
    * @access public
    * @param  bool      Whether to store response body in object property, set
    *                   this to false if downloading a LARGE file and using a Listener.
    *                   This is assumed to be true if body is gzip-encoded.
    * @throws PEAR_Error
    * @return mixed     true on success, PEAR_Error in case of malformed response
    */
    function process($saveBody = true)
    {
        do {
            $line = $this->_sock->readLine();
            if (sscanf($line, 'HTTP/%s %s', $http_version, $returncode) != 2) {
                return PEAR::raiseError('Malformed response.');
            } else {
                $this->_protocol = 'HTTP/' . $http_version;
                $this->_code     = intval($returncode);
            }
            while ('' !== ($header = $this->_sock->readLine())) {
                $this->_processHeader($header);
            }
        } while (100 == $this->_code);

        $this->_notify('gotHeaders', $this->_headers);

        // If response body is present, read it and decode
        $chunked = isset($this->_headers['transfer-encoding']) && ('chunked' == $this->_headers['transfer-encoding']);
        $gzipped = isset($this->_headers['content-encoding']) && ('gzip' == $this->_headers['content-encoding']);
        $hasBody = false;
        while (!$this->_sock->eof()) {
            if ($chunked) {
                $data = $this->_readChunked();
            } else {
                $data = $this->_sock->read(4096);
            }
            if ('' != $data) {
                $hasBody = true;
                if ($saveBody || $gzipped) {
                    $this->_body .= $data;
                }
                $this->_notify($gzipped? 'gzTick': 'tick', $data);
            }
        }
        if ($hasBody) {
            // Uncompress the body if needed
            if ($gzipped) {
                $this->_body = gzinflate(substr($this->_body, 10));
                $this->_notify('gotBody', $this->_body);
            } else {
                $this->_notify('gotBody');
            }
        }
        return true;
    }


   /**
    * Processes the response header
    *
    * @access private
    * @param  string    HTTP header
    */
    function _processHeader($header)
    {
        list($headername, $headervalue) = explode(':', $header, 2);
        $headername_i = strtolower($headername);
        $headervalue  = ltrim($headervalue);
        
        if ('set-cookie' != $headername_i) {
            $this->_headers[$headername]   = $headervalue;
            $this->_headers[$headername_i] = $headervalue;
        } else {
            $this->_parseCookie($headervalue);
        }
    }


   /**
    * Parse a Set-Cookie header to fill $_cookies array
    *
    * @access private
    * @param  string    value of Set-Cookie header
    */
    function _parseCookie($headervalue)
    {
        $cookie = array(
            'expires' => null,
            'domain'  => null,
            'path'    => null,
            'secure'  => false
        );

        // Only a name=value pair
        if (!strpos($headervalue, ';')) {
            $pos = strpos($headervalue, '=');
            $cookie['name']  = trim(substr($headervalue, 0, $pos));
            $cookie['value'] = trim(substr($headervalue, $pos + 1));

        // Some optional parameters are supplied
        } else {
            $elements = explode(';', $headervalue);
            $pos = strpos($elements[0], '=');
            $cookie['name']  = trim(substr($elements[0], 0, $pos));
            $cookie['value'] = trim(substr($elements[0], $pos + 1));

            for ($i = 1; $i < count($elements); $i++) {
                if (false === strpos($elements[$i], '=')) {
                    $elName  = trim($elements[$i]);
                    $elValue = null;
                } else {
                    list ($elName, $elValue) = array_map('trim', explode('=', $elements[$i]));
                }
                $elName = strtolower($elName);
                if ('secure' == $elName) {
                    $cookie['secure'] = true;
                } elseif ('expires' == $elName) {
                    $cookie['expires'] = str_replace('"', '', $elValue);
                } elseif ('path' == $elName || 'domain' == $elName) {
                    $cookie[$elName] = urldecode($elValue);
                } else {
                    $cookie[$elName] = $elValue;
                }
            }
        }
        $this->_cookies[] = $cookie;
    }


   /**
    * Read a part of response body encoded with chunked Transfer-Encoding
    * 
    * @access private
    * @return string
    */
    function _readChunked()
    {
        // at start of the next chunk?
        if (0 == $this->_chunkLength) {
            $line = $this->_sock->readLine();
            if (preg_match('/^([0-9a-f]+)/i', $line, $matches)) {
                $this->_chunkLength = hexdec($matches[1]); 
                // Chunk with zero length indicates the end
                if (0 == $this->_chunkLength) {
                    $this->_sock->readAll(); // make this an eof()
                    return '';
                }
            } elseif ($this->_sock->eof()) {
                return '';
            }
        }
        $data = $this->_sock->read($this->_chunkLength);
        $this->_chunkLength -= strlen($data);
        if (0 == $this->_chunkLength) {
            $this->_sock->readLine(); // Trailing CRLF
        }
        return $data;
    }


   /**
    * Notifies all registered listeners of an event.
    * 
    * @param    string  Event name
    * @param    mixed   Additional data
    * @access   private
    * @see HTTP_Request::_notify()
    */
    function _notify($event, $data = null)
    {
        foreach (array_keys($this->_listeners) as $id) {
            $this->_listeners[$id]->update($this, $event, $data);
        }
    }
} // End class HTTP_Response
?>
