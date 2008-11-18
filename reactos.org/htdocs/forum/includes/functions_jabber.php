<?php
/**
*
* @package phpBB3
* @version $Id: functions_jabber.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2007 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @ignore
*/
if (!defined('IN_PHPBB'))
{
	exit;
}

/**
*
* Jabber class from Flyspray project
*
* @version class.jabber2.php 1488 2007-11-25
* @copyright 2006 Flyspray.org
* @author Florian Schmitz (floele)
*
* Only slightly modified by Acyd Burn
*
* @package phpBB3
*/
class jabber
{
	var $connection = null;
	var $session = array();
	var $timeout = 10;

	var $server;
	var $port;
	var $username;
	var $password;
	var $use_ssl;
	var $resource = 'functions_jabber.phpbb.php';

	var $enable_logging;
	var $log_array;

	var $features = array();

	/**
	*/
	function jabber($server, $port, $username, $password, $use_ssl = false)
	{
		$this->server				= ($server) ? $server : 'localhost';
		$this->port					= ($port) ? $port : 5222;
		$this->username				= $username;
		$this->password				= $password;
		$this->use_ssl				= ($use_ssl && $this->can_use_ssl()) ? true : false;

		// Change port if we use SSL
		if ($this->port == 5222 && $this->use_ssl)
		{
			$this->port = 5223;
		}

		$this->enable_logging		= true;
		$this->log_array			= array();
	}

	/**
	* Able to use the SSL functionality?
	*/
	function can_use_ssl()
	{
		// Will not work with PHP >= 5.2.1 or < 5.2.3RC2 until timeout problem with ssl hasn't been fixed (http://bugs.php.net/41236)
		return ((version_compare(PHP_VERSION, '5.2.1', '<') || version_compare(PHP_VERSION, '5.2.3RC2', '>=')) && @extension_loaded('openssl')) ? true : false;
	}

	/**
	* Able to use TLS?
	*/
	function can_use_tls()
	{
		if (!@extension_loaded('openssl') || !function_exists('stream_socket_enable_crypto') || !function_exists('stream_get_meta_data') || !function_exists('socket_set_blocking') || !function_exists('stream_get_wrappers'))
		{
			return false;
		}

		/**
		* Make sure the encryption stream is supported
		* Also seem to work without the crypto stream if correctly compiled

		$streams = stream_get_wrappers();

		if (!in_array('streams.crypto', $streams))
		{
			return false;
		}
		*/

		return true;
	}

	/**
	* Sets the resource which is used. No validation is done here, only escaping.
	* @param string $name
	* @access public
	*/
	function set_resource($name)
	{
		$this->resource = $name;
	}

	/**
	* Connect
	*/
	function connect()
	{
/*		if (!$this->check_jid($this->username . '@' . $this->server))
		{
			$this->add_to_log('Error: Jabber ID is not valid: ' . $this->username . '@' . $this->server);
			return false;
		}*/

		$this->session['ssl'] = $this->use_ssl;

		if ($this->open_socket($this->server, $this->port, $this->use_ssl))
		{
			$this->send("<?xml version='1.0' encoding='UTF-8' ?" . ">\n");
			$this->send("<stream:stream to='{$this->server}' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' version='1.0'>\n");
		}
		else
		{
			$this->add_to_log('Error: connect() #2');
			return false;
		}

		// Now we listen what the server has to say...and give appropriate responses
		$this->response($this->listen());
		return true;
	}

	/**
	* Disconnect
	*/
	function disconnect()
	{
		if ($this->connected())
		{
			// disconnect gracefully
			if (isset($this->session['sent_presence']))
			{
				$this->send_presence('offline', '', true);
			}

			$this->send('</stream:stream>');
			$this->session = array();
			return fclose($this->connection);
		}

		return false;
	}

	/**
	* Connected?
	*/
	function connected()
	{
		return (is_resource($this->connection) && !feof($this->connection)) ? true : false;
	}


	/**
	* Initiates login (using data from contructor, after calling connect())
	* @access public
	* @return bool
	*/
	function login()
	{
		if (!sizeof($this->features))
		{
			$this->add_to_log('Error: No feature information from server available.');
			return false;
		}

		return $this->response($this->features);
	}

	/**
	* Send data to the Jabber server
	* @param string $xml
	* @access public
	* @return bool
	*/
	function send($xml)
	{
		if ($this->connected())
		{
			$xml = trim($xml);
			$this->add_to_log('SEND: '. $xml);
			return fwrite($this->connection, $xml);
		}
		else
		{
			$this->add_to_log('Error: Could not send, connection lost (flood?).');
			return false;
		}
	}

	/**
	* OpenSocket
	* @param string $server host to connect to
	* @param int $port port number
	* @param bool $use_ssl use ssl or not
	* @access public
	* @return bool
	*/
	function open_socket($server, $port, $use_ssl = false)
	{
		if (@function_exists('dns_get_record'))
		{
			$record = @dns_get_record("_xmpp-client._tcp.$server", DNS_SRV);
			if (!empty($record) && !empty($record[0]['target']))
			{
				$server = $record[0]['target'];
			}
		}

		$server = $use_ssl ? 'ssl://' . $server : $server;

		if ($this->connection = @fsockopen($server, $port, $errorno, $errorstr, $this->timeout))
		{
			socket_set_blocking($this->connection, 0);
			socket_set_timeout($this->connection, 60);

			return true;
		}

		// Apparently an error occured...
		$this->add_to_log('Error: open_socket() - ' . $errorstr);
		return false;
	}

	/**
	* Return log
	*/
	function get_log()
	{
		if ($this->enable_logging && sizeof($this->log_array))
		{
			return implode("<br /><br />", $this->log_array);
		}

		return '';
	}

	/**
	* Add information to log
	*/
	function add_to_log($string)
	{
		if ($this->enable_logging)
		{
			$this->log_array[] = utf8_htmlspecialchars($string);
		}
	}

	/**
	* Listens to the connection until it gets data or the timeout is reached.
	* Thus, it should only be called if data is expected to be received.
	* @access public
	* @return mixed either false for timeout or an array with the received data
	*/
	function listen($timeout = 10, $wait = false)
	{
		if (!$this->connected())
		{
			return false;
		}

		// Wait for a response until timeout is reached
		$start = time();
		$data = '';

		do
		{
			$read = trim(fread($this->connection, 4096));
			$data .= $read;
		}
		while (time() <= $start + $timeout && !feof($this->connection) && ($wait || $data == '' || $read != '' || (substr(rtrim($data), -1) != '>')));

		if ($data != '')
		{
			$this->add_to_log('RECV: '. $data);
			return $this->xmlize($data);
		}
		else
		{
			$this->add_to_log('Timeout, no response from server.');
			return false;
		}
	}

	/**
	* Initiates account registration (based on data used for contructor)
	* @access public
	* @return bool
	*/
	function register()
	{
		if (!isset($this->session['id']) || isset($this->session['jid']))
		{
			$this->add_to_log('Error: Cannot initiate registration.');
			return false;
		}

		$this->send("<iq type='get' id='reg_1'><query xmlns='jabber:iq:register'/></iq>");
		return $this->response($this->listen());
	}

	/**
	* Sets account presence. No additional info required (default is "online" status)
	* @param $message online, offline...
	* @param $type dnd, away, chat, xa or nothing
	* @param $unavailable set this to true if you want to become unavailable
	* @access public
	* @return bool
	*/
	function send_presence($message = '', $type = '', $unavailable = false)
	{
		if (!isset($this->session['jid']))
		{
			$this->add_to_log('ERROR: send_presence() - Cannot set presence at this point, no jid given.');
			return false;
		}

		$type = strtolower($type);
		$type = (in_array($type, array('dnd', 'away', 'chat', 'xa'))) ? '<show>'. $type .'</show>' : '';

		$unavailable = ($unavailable) ? " type='unavailable'" : '';
		$message = ($message) ? '<status>' . utf8_htmlspecialchars($message) .'</status>' : '';

		$this->session['sent_presence'] = !$unavailable;

		return $this->send("<presence$unavailable>" . $type . $message . '</presence>');
	}

	/**
	* This handles all the different XML elements
	* @param array $xml
	* @access public
	* @return bool
	*/
	function response($xml)
	{
		if (!is_array($xml) || !sizeof($xml))
		{
			return false;
		}

		// did we get multiple elements? do one after another
		// array('message' => ..., 'presence' => ...)
		if (sizeof($xml) > 1)
		{
			foreach ($xml as $key => $value)
			{
				$this->response(array($key => $value));
			}
			return;
		}
		else
		{
			// or even multiple elements of the same type?
			// array('message' => array(0 => ..., 1 => ...))
			if (sizeof(reset($xml)) > 1)
			{
				foreach (reset($xml) as $value)
				{
					$this->response(array(key($xml) => array(0 => $value)));
				}
				return;
			}
		}

		switch (key($xml))
		{
			case 'stream:stream':
				// Connection initialised (or after authentication). Not much to do here...

				if (isset($xml['stream:stream'][0]['#']['stream:features']))
				{
					// we already got all info we need
					$this->features = $xml['stream:stream'][0]['#'];
				}
				else
				{
					$this->features = $this->listen();
				}

				$second_time = isset($this->session['id']);
				$this->session['id'] = $xml['stream:stream'][0]['@']['id'];

				if ($second_time)
				{
					// If we are here for the second time after TLS, we need to continue logging in
					$this->login();
					return;
				}

				// go on with authentication?
				if (isset($this->features['stream:features'][0]['#']['bind']) || !empty($this->session['tls']))
				{
					return $this->response($this->features);
				}
			break;

			case 'stream:features':
				// Resource binding after successful authentication
				if (isset($this->session['authenticated']))
				{
					// session required?
					$this->session['sess_required'] = isset($xml['stream:features'][0]['#']['session']);

					$this->send("<iq type='set' id='bind_1'>
						<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>
							<resource>" . utf8_htmlspecialchars($this->resource) . '</resource>
						</bind>
					</iq>');
					return $this->response($this->listen());
				}

				// Let's use TLS if SSL is not enabled and we can actually use it
				if (!$this->session['ssl'] && $this->can_use_tls() && $this->can_use_ssl() && isset($xml['stream:features'][0]['#']['starttls']))
				{
					$this->add_to_log('Switching to TLS.');
					$this->send("<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>\n");
					return $this->response($this->listen());
				}

				// Does the server support SASL authentication?

				// I hope so, because we do (and no other method).
				if (isset($xml['stream:features'][0]['#']['mechanisms'][0]['@']['xmlns']) && $xml['stream:features'][0]['#']['mechanisms'][0]['@']['xmlns'] == 'urn:ietf:params:xml:ns:xmpp-sasl')
				{
					// Now decide on method
					$methods = array();

					foreach ($xml['stream:features'][0]['#']['mechanisms'][0]['#']['mechanism'] as $value)
					{
						$methods[] = $value['#'];
					}

					// we prefer DIGEST-MD5
					// we don't want to use plain authentication (neither does the server usually) if no encryption is in place

					// http://www.xmpp.org/extensions/attic/jep-0078-1.7.html
					// The plaintext mechanism SHOULD NOT be used unless the underlying stream is encrypted (using SSL or TLS)
					// and the client has verified that the server certificate is signed by a trusted certificate authority.

					if (in_array('DIGEST-MD5', $methods))
					{
						$this->send("<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='DIGEST-MD5'/>");
					}
					else if (in_array('PLAIN', $methods) && ($this->session['ssl'] || !empty($this->session['tls'])))
					{
						$this->send("<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='PLAIN'>"
							. base64_encode(chr(0) . $this->username . '@' . $this->server . chr(0) . $this->password) .
							'</auth>');
					}
					else if (in_array('ANONYMOUS', $methods))
					{
						$this->send("<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='ANONYMOUS'/>");
					}
					else
					{
						// not good...
						$this->add_to_log('Error: No authentication method supported.');
						$this->disconnect();
						return false;
					}

					return $this->response($this->listen());
				}
				else
				{
					// ok, this is it. bye.
					$this->add_to_log('Error: Server does not offer SASL authentication.');
					$this->disconnect();
					return false;
				}
			break;

			case 'challenge':
				// continue with authentication...a challenge literally -_-
				$decoded = base64_decode($xml['challenge'][0]['#']);
				$decoded = $this->parse_data($decoded);

				if (!isset($decoded['digest-uri']))
				{
					$decoded['digest-uri'] = 'xmpp/'. $this->server;
				}

				// better generate a cnonce, maybe it's needed
				$str = '';
				mt_srand((double)microtime()*10000000);

				for ($i = 0; $i < 32; $i++)
				{
					$str .= chr(mt_rand(0, 255));
				}
				$decoded['cnonce'] = base64_encode($str);

				// second challenge?
				if (isset($decoded['rspauth']))
				{
					$this->send("<response xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>");
				}
				else
				{
					// Make sure we only use 'auth' for qop (relevant for $this->encrypt_password())
					// If the <response> is choking up on the changed parameter we may need to adjust encrypt_password() directly
					if (isset($decoded['qop']) && $decoded['qop'] != 'auth' && strpos($decoded['qop'], 'auth') !== false)
					{
						$decoded['qop'] = 'auth';
					}

					$response = array(
						'username'	=> $this->username,
						'response'	=> $this->encrypt_password(array_merge($decoded, array('nc' => '00000001'))),
						'charset'	=> 'utf-8',
						'nc'		=> '00000001',
						'qop'		=> 'auth',			// only auth being supported
					);

					foreach (array('nonce', 'digest-uri', 'realm', 'cnonce') as $key)
					{
						if (isset($decoded[$key]))
						{
							$response[$key] = $decoded[$key];
						}
					}

					$this->send("<response xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>" . base64_encode($this->implode_data($response)) . '</response>');
				}

				return $this->response($this->listen());
			break;

			case 'failure':
				$this->add_to_log('Error: Server sent "failure".');
				$this->disconnect();
				return false;
			break;

			case 'proceed':
				// continue switching to TLS
				$meta = stream_get_meta_data($this->connection);
				socket_set_blocking($this->connection, 1);

				if (!stream_socket_enable_crypto($this->connection, true, STREAM_CRYPTO_METHOD_TLS_CLIENT))
				{
					$this->add_to_log('Error: TLS mode change failed.');
					return false;
				}

				socket_set_blocking($this->connection, $meta['blocked']);
				$this->session['tls'] = true;

				// new stream
				$this->send("<?xml version='1.0' encoding='UTF-8' ?" . ">\n");
				$this->send("<stream:stream to='{$this->server}' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' version='1.0'>\n");

				return $this->response($this->listen());
			break;

			case 'success':
				// Yay, authentication successful.
				$this->send("<stream:stream to='{$this->server}' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' version='1.0'>\n");
				$this->session['authenticated'] = true;

				// we have to wait for another response
				return $this->response($this->listen());
			break;

			case 'iq':
				// we are not interested in IQs we did not expect
				if (!isset($xml['iq'][0]['@']['id']))
				{
					return false;
				}

				// multiple possibilities here
				switch ($xml['iq'][0]['@']['id'])
				{
					case 'bind_1':
						$this->session['jid'] = $xml['iq'][0]['#']['bind'][0]['#']['jid'][0]['#'];

						// and (maybe) yet another request to be able to send messages *finally*
						if ($this->session['sess_required'])
						{
							$this->send("<iq to='{$this->server}' type='set' id='sess_1'>
								<session xmlns='urn:ietf:params:xml:ns:xmpp-session'/>
								</iq>");
							return $this->response($this->listen());
						}

						return true;
					break;

					case 'sess_1':
						return true;
					break;

					case 'reg_1':
						$this->send("<iq type='set' id='reg_2'>
								<query xmlns='jabber:iq:register'>
									<username>" . utf8_htmlspecialchars($this->username) . "</username>
									<password>" . utf8_htmlspecialchars($this->password) . "</password>
								</query>
							</iq>");
						return $this->response($this->listen());
					break;

					case 'reg_2':
						// registration end
						if (isset($xml['iq'][0]['#']['error']))
						{
							$this->add_to_log('Warning: Registration failed.');
							return false;
						}
						return true;
					break;

					case 'unreg_1':
						return true;
					break;

					default:
						$this->add_to_log('Notice: Received unexpected IQ.');
						return false;
					break;
				}
			break;

			case 'message':
				// we are only interested in content...
				if (!isset($xml['message'][0]['#']['body']))
				{
					return false;
				}

				$message['body'] = $xml['message'][0]['#']['body'][0]['#'];
				$message['from'] = $xml['message'][0]['@']['from'];

				if (isset($xml['message'][0]['#']['subject']))
				{
					$message['subject'] = $xml['message'][0]['#']['subject'][0]['#'];
				}
				$this->session['messages'][] = $message;
			break;

			default:
				// hm...don't know this response
				$this->add_to_log('Notice: Unknown server response (' . key($xml) . ')');
				return false;
			break;
		}
	}

	function send_message($to, $text, $subject = '', $type = 'normal')
	{
		if (!isset($this->session['jid']))
		{
			return false;
		}

		if (!in_array($type, array('chat', 'normal', 'error', 'groupchat', 'headline')))
		{
			$type = 'normal';
		}

		return $this->send("<message from='" . utf8_htmlspecialchars($this->session['jid']) . "' to='" . utf8_htmlspecialchars($to) . "' type='$type' id='" . uniqid('msg') . "'>
			<subject>" . utf8_htmlspecialchars($subject) . "</subject>
			<body>" . utf8_htmlspecialchars($text) . "</body>
			</message>"
		);
	}

	/**
	* Encrypts a password as in RFC 2831
	* @param array $data Needs data from the client-server connection
	* @access public
	* @return string
	*/
	function encrypt_password($data)
	{
		// let's me think about <challenge> again...
		foreach (array('realm', 'cnonce', 'digest-uri') as $key)
		{
			if (!isset($data[$key]))
			{
				$data[$key] = '';
			}
		}

		$pack = md5($this->username . ':' . $data['realm'] . ':' . $this->password);

		if (isset($data['authzid']))
		{
			$a1 = pack('H32', $pack)  . sprintf(':%s:%s:%s', $data['nonce'], $data['cnonce'], $data['authzid']);
		}
		else
		{
			$a1 = pack('H32', $pack)  . sprintf(':%s:%s', $data['nonce'], $data['cnonce']);
		}

		// should be: qop = auth
		$a2 = 'AUTHENTICATE:'. $data['digest-uri'];

		return md5(sprintf('%s:%s:%s:%s:%s:%s', md5($a1), $data['nonce'], $data['nc'], $data['cnonce'], $data['qop'], md5($a2)));
	}

	/**
	* parse_data like a="b",c="d",... or like a="a, b", c, d="e", f=g,...
	* @param string $data
	* @access public
	* @return array a => b ...
	*/
	function parse_data($data)
	{
		$data = explode(',', $data);
		$pairs = array();
		$key = false;

		foreach ($data as $pair)
		{
			$dd = strpos($pair, '=');

			if ($dd)
			{
				$key = trim(substr($pair, 0, $dd));
				$pairs[$key] = trim(trim(substr($pair, $dd + 1)), '"');
			}
			else if (strpos(strrev(trim($pair)), '"') === 0 && $key)
			{
				// We are actually having something left from "a, b" values, add it to the last one we handled.
				$pairs[$key] .= ',' . trim(trim($pair), '"');
				continue;
			}
		}

		return $pairs;
	}

	/**
	* opposite of jabber::parse_data()
	* @param array $data
	* @access public
	* @return string
	*/
	function implode_data($data)
	{
		$return = array();
		foreach ($data as $key => $value)
		{
			$return[] = $key . '="' . $value . '"';
		}
		return implode(',', $return);
	}

	/**
	* xmlize()
	* @author Hans Anderson
	* @copyright Hans Anderson / http://www.hansanderson.com/php/xml/
	*/
	function xmlize($data, $skip_white = 1, $encoding = 'UTF-8')
	{
		$data = trim($data);

		if (substr($data, 0, 5) != '<?xml')
		{
			// mod
			$data = '<root>'. $data . '</root>';
		}

		$vals = $index = $array = array();
		$parser = xml_parser_create($encoding);
		xml_parser_set_option($parser, XML_OPTION_CASE_FOLDING, 0);
		xml_parser_set_option($parser, XML_OPTION_SKIP_WHITE, $skip_white);
		xml_parse_into_struct($parser, $data, $vals, $index);
		xml_parser_free($parser);

		$i = 0;
		$tagname = $vals[$i]['tag'];

		$array[$tagname][0]['@'] = (isset($vals[$i]['attributes'])) ? $vals[$i]['attributes'] : array();
		$array[$tagname][0]['#'] = $this->_xml_depth($vals, $i);

		if (substr($data, 0, 5) != '<?xml')
		{
			$array = $array['root'][0]['#'];
		}

		return $array;
	}

	/**
	* _xml_depth()
	* @author Hans Anderson
	* @copyright Hans Anderson / http://www.hansanderson.com/php/xml/
	*/
	function _xml_depth($vals, &$i)
	{
		$children = array();

		if (isset($vals[$i]['value']))
		{
			array_push($children, $vals[$i]['value']);
		}

		while (++$i < sizeof($vals))
		{
			switch ($vals[$i]['type'])
			{
				case 'open':

					$tagname = (isset($vals[$i]['tag'])) ? $vals[$i]['tag'] : '';
					$size = (isset($children[$tagname])) ? sizeof($children[$tagname]) : 0;

					if (isset($vals[$i]['attributes']))
					{
						$children[$tagname][$size]['@'] = $vals[$i]['attributes'];
					}

					$children[$tagname][$size]['#'] = $this->_xml_depth($vals, $i);

				break;

				case 'cdata':
					array_push($children, $vals[$i]['value']);
				break;

				case 'complete':

					$tagname = $vals[$i]['tag'];
					$size = (isset($children[$tagname])) ? sizeof($children[$tagname]) : 0;
					$children[$tagname][$size]['#'] = (isset($vals[$i]['value'])) ? $vals[$i]['value'] : array();

					if (isset($vals[$i]['attributes']))
					{
						$children[$tagname][$size]['@'] = $vals[$i]['attributes'];
					}

				break;

				case 'close':
					return $children;
				break;
			}
		}

		return $children;
	}
}

?>