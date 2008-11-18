<?php
/**
*
* @package phpBB3
* @version $Id: functions_messenger.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
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
* Messenger
* @package phpBB3
*/
class messenger
{
	var $vars, $msg, $extra_headers, $replyto, $from, $subject;
	var $addresses = array();

	var $mail_priority = MAIL_NORMAL_PRIORITY;
	var $use_queue = true;
	var $tpl_msg = array();

	/**
	* Constructor
	*/
	function messenger($use_queue = true)
	{
		global $config;

		$this->use_queue = (!$config['email_package_size']) ? false : $use_queue;
		$this->subject = '';
	}

	/**
	* Resets all the data (address, template file, etc etc) to default
	*/
	function reset()
	{
		$this->addresses = $this->extra_headers = array();
		$this->vars = $this->msg = $this->replyto = $this->from = '';
		$this->mail_priority = MAIL_NORMAL_PRIORITY;
	}

	/**
	* Sets an email address to send to
	*/
	function to($address, $realname = '')
	{
		global $config;

		$pos = isset($this->addresses['to']) ? sizeof($this->addresses['to']) : 0;

		$this->addresses['to'][$pos]['email'] = trim($address);

		// If empty sendmail_path on windows, PHP changes the to line
		if (!$config['smtp_delivery'] && DIRECTORY_SEPARATOR == '\\')
		{
			$this->addresses['to'][$pos]['name'] = '';
		}
		else
		{
			$this->addresses['to'][$pos]['name'] = trim($realname);
		}
	}

	/**
	* Sets an cc address to send to
	*/
	function cc($address, $realname = '')
	{
		$pos = isset($this->addresses['cc']) ? sizeof($this->addresses['cc']) : 0;
		$this->addresses['cc'][$pos]['email'] = trim($address);
		$this->addresses['cc'][$pos]['name'] = trim($realname);
	}

	/**
	* Sets an bcc address to send to
	*/
	function bcc($address, $realname = '')
	{
		$pos = isset($this->addresses['bcc']) ? sizeof($this->addresses['bcc']) : 0;
		$this->addresses['bcc'][$pos]['email'] = trim($address);
		$this->addresses['bcc'][$pos]['name'] = trim($realname);
	}

	/**
	* Sets a im contact to send to
	*/
	function im($address, $realname = '')
	{
		$pos = isset($this->addresses['im']) ? sizeof($this->addresses['im']) : 0;
		$this->addresses['im'][$pos]['uid'] = trim($address);
		$this->addresses['im'][$pos]['name'] = trim($realname);
	}

	/**
	* Set the reply to address
	*/
	function replyto($address)
	{
		$this->replyto = trim($address);
	}

	/**
	* Set the from address
	*/
	function from($address)
	{
		$this->from = trim($address);
	}

	/**
	* set up subject for mail
	*/
	function subject($subject = '')
	{
		$this->subject = trim($subject);
	}

	/**
	* set up extra mail headers
	*/
	function headers($headers)
	{
		$this->extra_headers[] = trim($headers);
	}

	/**
	* Set the email priority
	*/
	function set_mail_priority($priority = MAIL_NORMAL_PRIORITY)
	{
		$this->mail_priority = $priority;
	}

	/**
	* Set email template to use
	*/
	function template($template_file, $template_lang = '')
	{
		global $config, $phpbb_root_path;

		if (!trim($template_file))
		{
			trigger_error('No template file set', E_USER_ERROR);
		}

		if (!trim($template_lang))
		{
			$template_lang = basename($config['default_lang']);
		}

		if (empty($this->tpl_msg[$template_lang . $template_file]))
		{
			$tpl_file = "{$phpbb_root_path}language/$template_lang/email/$template_file.txt";

			if (!file_exists($tpl_file))
			{
				trigger_error("Could not find email template file [ $tpl_file ]", E_USER_ERROR);
			}

			if (($data = @file_get_contents($tpl_file)) === false)
			{
				trigger_error("Failed opening template file [ $tpl_file ]", E_USER_ERROR);
			}

			$this->tpl_msg[$template_lang . $template_file] = $data;
		}

		$this->msg = $this->tpl_msg[$template_lang . $template_file];

		return true;
	}

	/**
	* assign variables to email template
	*/
	function assign_vars($vars)
	{
		$this->vars = (empty($this->vars)) ? $vars : $this->vars + $vars;
	}

	/**
	* Send the mail out to the recipients set previously in var $this->addresses
	*/
	function send($method = NOTIFY_EMAIL, $break = false)
	{
		global $config, $user;

		// We add some standard variables we always use, no need to specify them always
		$this->vars['U_BOARD'] = (!isset($this->vars['U_BOARD'])) ? generate_board_url() : $this->vars['U_BOARD'];
		$this->vars['EMAIL_SIG'] = (!isset($this->vars['EMAIL_SIG'])) ? str_replace('<br />', "\n", "-- \n" . htmlspecialchars_decode($config['board_email_sig'])) : $this->vars['EMAIL_SIG'];
		$this->vars['SITENAME'] = (!isset($this->vars['SITENAME'])) ? htmlspecialchars_decode($config['sitename']) : $this->vars['SITENAME'];

		// Escape all quotes, else the eval will fail.
		$this->msg = str_replace ("'", "\'", $this->msg);
		$this->msg = preg_replace('#\{([a-z0-9\-_]*?)\}#is', "' . ((isset(\$this->vars['\\1'])) ? \$this->vars['\\1'] : '') . '", $this->msg);

		eval("\$this->msg = '$this->msg';");

		// We now try and pull a subject from the email body ... if it exists,
		// do this here because the subject may contain a variable
		$drop_header = '';
		$match = array();
		if (preg_match('#^(Subject:(.*?))$#m', $this->msg, $match))
		{
			$this->subject = (trim($match[2]) != '') ? trim($match[2]) : (($this->subject != '') ? $this->subject : $user->lang['NO_EMAIL_SUBJECT']);
			$drop_header .= '[\r\n]*?' . preg_quote($match[1], '#');
		}
		else
		{
			$this->subject = (($this->subject != '') ? $this->subject : $user->lang['NO_EMAIL_SUBJECT']);
		}

		if ($drop_header)
		{
			$this->msg = trim(preg_replace('#' . $drop_header . '#s', '', $this->msg));
		}

		if ($break)
		{
			return true;
		}

		switch ($method)
		{
			case NOTIFY_EMAIL:
				$result = $this->msg_email();
			break;

			case NOTIFY_IM:
				$result = $this->msg_jabber();
			break;

			case NOTIFY_BOTH:
				$result = $this->msg_email();
				$this->msg_jabber();
			break;
		}

		$this->reset();
		return $result;
	}

	/**
	* Add error message to log
	*/
	function error($type, $msg)
	{
		global $user, $phpEx, $phpbb_root_path, $config;

		// Session doesn't exist, create it
		if (!isset($user->session_id) || $user->session_id === '')
		{
			$user->session_begin();
		}

		$calling_page = (!empty($_SERVER['PHP_SELF'])) ? $_SERVER['PHP_SELF'] : $_ENV['PHP_SELF'];

		$message = '';
		switch ($type)
		{
			case 'EMAIL':
				$message = '<strong>EMAIL/' . (($config['smtp_delivery']) ? 'SMTP' : 'PHP/' . $config['email_function_name'] . '()') . '</strong>';
			break;

			default:
				$message = "<strong>$type</strong>";
			break;
		}

		$message .= '<br /><em>' . htmlspecialchars($calling_page) . '</em><br /><br />' . $msg . '<br />';
		add_log('critical', 'LOG_ERROR_' . $type, $message);
	}

	/**
	* Save to queue
	*/
	function save_queue()
	{
		global $config;

		if ($config['email_package_size'] && $this->use_queue && !empty($this->queue))
		{
			$this->queue->save();
			return;
		}
	}

	/**
	* Return email header
	*/
	function build_header($to, $cc, $bcc)
	{
		global $config;

		$headers = array();

		$headers[] = 'From: ' . $this->from;

		if ($cc)
		{
			$headers[] = 'Cc: ' . $cc;
		}

		if ($bcc)
		{
			$headers[] = 'Bcc: ' . $bcc;
		}

		$headers[] = 'Reply-To: ' . $this->replyto;
		$headers[] = 'Return-Path: <' . $config['board_email'] . '>';
		$headers[] = 'Sender: <' . $config['board_email'] . '>';
		$headers[] = 'MIME-Version: 1.0';
		$headers[] = 'Message-ID: <' . md5(unique_id(time())) . '@' . $config['server_name'] . '>';
		$headers[] = 'Date: ' . date('r', time());
		$headers[] = 'Content-Type: text/plain; charset=UTF-8'; // format=flowed
		$headers[] = 'Content-Transfer-Encoding: 8bit'; // 7bit

		$headers[] = 'X-Priority: ' . $this->mail_priority;
		$headers[] = 'X-MSMail-Priority: ' . (($this->mail_priority == MAIL_LOW_PRIORITY) ? 'Low' : (($this->mail_priority == MAIL_NORMAL_PRIORITY) ? 'Normal' : 'High'));
		$headers[] = 'X-Mailer: PhpBB3';
		$headers[] = 'X-MimeOLE: phpBB3';
		$headers[] = 'X-phpBB-Origin: phpbb://' . str_replace(array('http://', 'https://'), array('', ''), generate_board_url());

		// We use \n here instead of \r\n because our smtp mailer is adjusting it to \r\n automatically, whereby the php mail function only works
		// if using \n.

		if (sizeof($this->extra_headers))
		{
			$headers[] = implode("\n", $this->extra_headers);
		}

		return implode("\n", $headers);
	}

	/**
	* Send out emails
	*/
	function msg_email()
	{
		global $config, $user;

		if (empty($config['email_enable']))
		{
			return false;
		}

		$use_queue = false;
		if ($config['email_package_size'] && $this->use_queue)
		{
			if (empty($this->queue))
			{
				$this->queue = new queue();
				$this->queue->init('email', $config['email_package_size']);
			}
			$use_queue = true;
		}

		if (empty($this->replyto))
		{
			$this->replyto = '<' . $config['board_contact'] . '>';
		}

		if (empty($this->from))
		{
			$this->from = '<' . $config['board_contact'] . '>';
		}

		// Build to, cc and bcc strings
		$to = $cc = $bcc = '';
		foreach ($this->addresses as $type => $address_ary)
		{
			if ($type == 'im')
			{
				continue;
			}

			foreach ($address_ary as $which_ary)
			{
				$$type .= (($$type != '') ? ', ' : '') . (($which_ary['name'] != '') ? '"' . mail_encode($which_ary['name']) . '" <' . $which_ary['email'] . '>' : $which_ary['email']);
			}
		}

		// Build header
		$headers = $this->build_header($to, $cc, $bcc);

		// Send message ...
		if (!$use_queue)
		{
			$mail_to = ($to == '') ? 'undisclosed-recipients:;' : $to;
			$err_msg = '';

			if ($config['smtp_delivery'])
			{
				$result = smtpmail($this->addresses, mail_encode($this->subject), wordwrap(utf8_wordwrap($this->msg), 997, "\n", true), $err_msg, $headers);
			}
			else
			{
				ob_start();
				$result = $config['email_function_name']($mail_to, mail_encode($this->subject), wordwrap(utf8_wordwrap($this->msg), 997, "\n", true), $headers);
				$err_msg = ob_get_clean();
			}

			if (!$result)
			{
				$this->error('EMAIL', $err_msg);
				return false;
			}
		}
		else
		{
			$this->queue->put('email', array(
				'to'			=> $to,
				'addresses'		=> $this->addresses,
				'subject'		=> $this->subject,
				'msg'			=> $this->msg,
				'headers'		=> $headers)
			);
		}

		return true;
	}

	/**
	* Send jabber message out
	*/
	function msg_jabber()
	{
		global $config, $db, $user, $phpbb_root_path, $phpEx;

		if (empty($config['jab_enable']) || empty($config['jab_host']) || empty($config['jab_username']) || empty($config['jab_password']))
		{
			return false;
		}

		$use_queue = false;
		if ($config['jab_package_size'] && $this->use_queue)
		{
			if (empty($this->queue))
			{
				$this->queue = new queue();
				$this->queue->init('jabber', $config['jab_package_size']);
			}
			$use_queue = true;
		}

		$addresses = array();
		foreach ($this->addresses['im'] as $type => $uid_ary)
		{
			$addresses[] = $uid_ary['uid'];
		}
		$addresses = array_unique($addresses);

		if (!$use_queue)
		{
			include_once($phpbb_root_path . 'includes/functions_jabber.' . $phpEx);
			$this->jabber = new jabber($config['jab_host'], $config['jab_port'], $config['jab_username'], $config['jab_password'], $config['jab_use_ssl']);

			if (!$this->jabber->connect())
			{
				$this->error('JABBER', $user->lang['ERR_JAB_CONNECT'] . '<br />' . $this->jabber->get_log());
				return false;
			}

			if (!$this->jabber->login())
			{
				$this->error('JABBER', $user->lang['ERR_JAB_AUTH'] . '<br />' . $this->jabber->get_log());
				return false;
			}

			foreach ($addresses as $address)
			{
				$this->jabber->send_message($address, $this->msg, $this->subject);
			}

			$this->jabber->disconnect();
		}
		else
		{
			$this->queue->put('jabber', array(
				'addresses'		=> $addresses,
				'subject'		=> $this->subject,
				'msg'			=> $this->msg)
			);
		}
		unset($addresses);
		return true;
	}
}

/**
* handling email and jabber queue
* @package phpBB3
*/
class queue
{
	var $data = array();
	var $queue_data = array();
	var $package_size = 0;
	var $cache_file = '';

	/**
	* constructor
	*/
	function queue()
	{
		global $phpEx, $phpbb_root_path;

		$this->data = array();
		$this->cache_file = "{$phpbb_root_path}cache/queue.$phpEx";
	}

	/**
	* Init a queue object
	*/
	function init($object, $package_size)
	{
		$this->data[$object] = array();
		$this->data[$object]['package_size'] = $package_size;
		$this->data[$object]['data'] = array();
	}

	/**
	* Put object in queue
	*/
	function put($object, $scope)
	{
		$this->data[$object]['data'][] = $scope;
	}

	/**
	* Process queue
	* Using lock file
	*/
	function process()
	{
		global $db, $config, $phpEx, $phpbb_root_path, $user;

		set_config('last_queue_run', time(), true);

		// Delete stale lock file
		if (file_exists($this->cache_file . '.lock') && !file_exists($this->cache_file))
		{
			@unlink($this->cache_file . '.lock');
			return;
		}

		if (!file_exists($this->cache_file) || (file_exists($this->cache_file . '.lock') && filemtime($this->cache_file) > time() - $config['queue_interval']))
		{
			return;
		}

		$fp = @fopen($this->cache_file . '.lock', 'wb');
		fclose($fp);
		@chmod($this->cache_file . '.lock', 0666);

		include($this->cache_file);

		foreach ($this->queue_data as $object => $data_ary)
		{
			@set_time_limit(0);

			if (!isset($data_ary['package_size']))
			{
				$data_ary['package_size'] = 0;
			}

			$package_size = $data_ary['package_size'];
			$num_items = (!$package_size || sizeof($data_ary['data']) < $package_size) ? sizeof($data_ary['data']) : $package_size;

			// If the amount of emails to be sent is way more than package_size than we need to increase it to prevent backlogs...
			if (sizeof($data_ary['data']) > $package_size * 2.5)
			{
				$num_items = sizeof($data_ary['data']);
			}

			switch ($object)
			{
				case 'email':
					// Delete the email queued objects if mailing is disabled
					if (!$config['email_enable'])
					{
						unset($this->queue_data['email']);
						continue 2;
					}
				break;

				case 'jabber':
					if (!$config['jab_enable'])
					{
						unset($this->queue_data['jabber']);
						continue 2;
					}

					include_once($phpbb_root_path . 'includes/functions_jabber.' . $phpEx);
					$this->jabber = new jabber($config['jab_host'], $config['jab_port'], $config['jab_username'], $config['jab_password'], $config['jab_use_ssl']);

					if (!$this->jabber->connect())
					{
						messenger::error('JABBER', $user->lang['ERR_JAB_CONNECT']);
						continue 2;
					}

					if (!$this->jabber->login())
					{
						messenger::error('JABBER', $user->lang['ERR_JAB_AUTH']);
						continue 2;
					}

				break;

				default:
					return;
			}

			for ($i = 0; $i < $num_items; $i++)
			{
				// Make variables available...
				extract(array_shift($this->queue_data[$object]['data']));

				switch ($object)
				{
					case 'email':
						$err_msg = '';
						$to = (!$to) ? 'undisclosed-recipients:;' : $to;

						if ($config['smtp_delivery'])
						{
							$result = smtpmail($addresses, mail_encode($subject), wordwrap(utf8_wordwrap($msg), 997, "\n", true), $err_msg, $headers);
						}
						else
						{
							ob_start();
							$result = $config['email_function_name']($to, mail_encode($subject), wordwrap(utf8_wordwrap($msg), 997, "\n", true), $headers);
							$err_msg = ob_get_clean();
						}

						if (!$result)
						{
							@unlink($this->cache_file . '.lock');

							messenger::error('EMAIL', $err_msg);
							continue 2;
						}
					break;

					case 'jabber':
						foreach ($addresses as $address)
						{
							if ($this->jabber->send_message($address, $msg, $subject) === false)
							{
								messenger::error('JABBER', $this->jabber->get_log());
								continue 3;
							}
						}
					break;
				}
			}

			// No more data for this object? Unset it
			if (!sizeof($this->queue_data[$object]['data']))
			{
				unset($this->queue_data[$object]);
			}

			// Post-object processing
			switch ($object)
			{
				case 'jabber':
					// Hang about a couple of secs to ensure the messages are
					// handled, then disconnect
					$this->jabber->disconnect();
				break;
			}
		}
	
		if (!sizeof($this->queue_data))
		{
			@unlink($this->cache_file);
		}
		else
		{
			if ($fp = @fopen($this->cache_file, 'w'))
			{
				@flock($fp, LOCK_EX);
				fwrite($fp, "<?php\n\$this->queue_data = " . var_export($this->queue_data, true) . ";\n?>");
				@flock($fp, LOCK_UN);
				fclose($fp);

				@chmod($this->cache_file, 0666);
			}
		}

		@unlink($this->cache_file . '.lock');
	}

	/**
	* Save queue
	*/
	function save()
	{
		if (!sizeof($this->data))
		{
			return;
		}
		
		if (file_exists($this->cache_file))
		{
			include($this->cache_file);
			
			foreach ($this->queue_data as $object => $data_ary)
			{
				if (isset($this->data[$object]) && sizeof($this->data[$object]))
				{
					$this->data[$object]['data'] = array_merge($data_ary['data'], $this->data[$object]['data']);
				}
				else
				{
					$this->data[$object]['data'] = $data_ary['data'];
				}
			}
		}

		if ($fp = @fopen($this->cache_file, 'w'))
		{
			@flock($fp, LOCK_EX);
			fwrite($fp, "<?php\n\$this->queue_data = " . var_export($this->data, true) . ";\n?>");
			@flock($fp, LOCK_UN);
			fclose($fp);

			@chmod($this->cache_file, 0666);
		}
	}
}

/**
* Replacement or substitute for PHP's mail command
*/
function smtpmail($addresses, $subject, $message, &$err_msg, $headers = '')
{
	global $config, $user;

	// Fix any bare linefeeds in the message to make it RFC821 Compliant.
	$message = preg_replace("#(?<!\r)\n#si", "\r\n", $message);

	if ($headers != '')
	{
		if (is_array($headers))
		{
			$headers = (sizeof($headers) > 1) ? join("\n", $headers) : $headers[0];
		}
		$headers = chop($headers);

		// Make sure there are no bare linefeeds in the headers
		$headers = preg_replace('#(?<!\r)\n#si', "\r\n", $headers);

		// Ok this is rather confusing all things considered,
		// but we have to grab bcc and cc headers and treat them differently
		// Something we really didn't take into consideration originally
		$header_array = explode("\r\n", $headers);
		$headers = '';

		foreach ($header_array as $header)
		{
			if (strpos(strtolower($header), 'cc:') === 0 || strpos(strtolower($header), 'bcc:') === 0)
			{
				$header = '';
			}
			$headers .= ($header != '') ? $header . "\r\n" : '';
		}

		$headers = chop($headers);
	}

	if (trim($subject) == '')
	{
		$err_msg = (isset($user->lang['NO_EMAIL_SUBJECT'])) ? $user->lang['NO_EMAIL_SUBJECT'] : 'No email subject specified';
		return false;
	}

	if (trim($message) == '')
	{
		$err_msg = (isset($user->lang['NO_EMAIL_MESSAGE'])) ? $user->lang['NO_EMAIL_MESSAGE'] : 'Email message was blank';
		return false;
	}

	$mail_rcpt = $mail_to = $mail_cc = array();

	// Build correct addresses for RCPT TO command and the client side display (TO, CC)
	if (isset($addresses['to']) && sizeof($addresses['to']))
	{
		foreach ($addresses['to'] as $which_ary)
		{
			$mail_to[] = ($which_ary['name'] != '') ? mail_encode(trim($which_ary['name'])) . ' <' . trim($which_ary['email']) . '>' : '<' . trim($which_ary['email']) . '>';
			$mail_rcpt['to'][] = '<' . trim($which_ary['email']) . '>';
		}
	}

	if (isset($addresses['bcc']) && sizeof($addresses['bcc']))
	{
		foreach ($addresses['bcc'] as $which_ary)
		{
			$mail_rcpt['bcc'][] = '<' . trim($which_ary['email']) . '>';
		}
	}

	if (isset($addresses['cc']) && sizeof($addresses['cc']))
	{
		foreach ($addresses['cc'] as $which_ary)
		{
			$mail_cc[] = ($which_ary['name'] != '') ? mail_encode(trim($which_ary['name'])) . ' <' . trim($which_ary['email']) . '>' : '<' . trim($which_ary['email']) . '>';
			$mail_rcpt['cc'][] = '<' . trim($which_ary['email']) . '>';
		}
	}

	$smtp = new smtp_class();

	$errno = 0;
	$errstr = '';

	$smtp->add_backtrace('Connecting to ' . $config['smtp_host'] . ':' . $config['smtp_port']);

	// Ok we have error checked as much as we can to this point let's get on it already.
	ob_start();
	$smtp->socket = fsockopen($config['smtp_host'], $config['smtp_port'], $errno, $errstr, 20);
	$error_contents = ob_get_clean();

	if (!$smtp->socket)
	{
		if ($errstr)
		{
			$errstr = utf8_convert_message($errstr);
		}

		$err_msg = (isset($user->lang['NO_CONNECT_TO_SMTP_HOST'])) ? sprintf($user->lang['NO_CONNECT_TO_SMTP_HOST'], $errno, $errstr) : "Could not connect to smtp host : $errno : $errstr";
		$err_msg .= ($error_contents) ? '<br /><br />' . htmlspecialchars($error_contents) : '';
		return false;
	}

	// Wait for reply
	if ($err_msg = $smtp->server_parse('220', __LINE__))
	{
		$smtp->close_session($err_msg);
		return false;
	}

	// Let me in. This function handles the complete authentication process
	if ($err_msg = $smtp->log_into_server($config['smtp_host'], $config['smtp_username'], $config['smtp_password'], $config['smtp_auth_method']))
	{
		$smtp->close_session($err_msg);
		return false;
	}

	// From this point onward most server response codes should be 250
	// Specify who the mail is from....
	$smtp->server_send('MAIL FROM:<' . $config['board_email'] . '>');
	if ($err_msg = $smtp->server_parse('250', __LINE__))
	{
		$smtp->close_session($err_msg);
		return false;
	}

	// Specify each user to send to and build to header.
	$to_header = implode(', ', $mail_to);
	$cc_header = implode(', ', $mail_cc);

	// Now tell the MTA to send the Message to the following people... [TO, BCC, CC]
	$rcpt = false;
	foreach ($mail_rcpt as $type => $mail_to_addresses)
	{
		foreach ($mail_to_addresses as $mail_to_address)
		{
			// Add an additional bit of error checking to the To field.
			if (preg_match('#[^ ]+\@[^ ]+#', $mail_to_address))
			{
				$smtp->server_send("RCPT TO:$mail_to_address");
				if ($err_msg = $smtp->server_parse('250', __LINE__))
				{
					// We continue... if users are not resolved we do not care
					if ($smtp->numeric_response_code != 550)
					{
						$smtp->close_session($err_msg);
						return false;
					}
				}
				else
				{
					$rcpt = true;
				}
			}
		}
	}

	// We try to send messages even if a few people do not seem to have valid email addresses, but if no one has, we have to exit here.
	if (!$rcpt)
	{
		$user->session_begin();
		$err_msg .= '<br /><br />';
		$err_msg .= (isset($user->lang['INVALID_EMAIL_LOG'])) ? sprintf($user->lang['INVALID_EMAIL_LOG'], htmlspecialchars($mail_to_address)) : '<strong>' . htmlspecialchars($mail_to_address) . '</strong> possibly an invalid email address?';
		$smtp->close_session($err_msg);
		return false;
	}

	// Ok now we tell the server we are ready to start sending data
	$smtp->server_send('DATA');

	// This is the last response code we look for until the end of the message.
	if ($err_msg = $smtp->server_parse('354', __LINE__))
	{
		$smtp->close_session($err_msg);
		return false;
	}

	// Send the Subject Line...
	$smtp->server_send("Subject: $subject");

	// Now the To Header.
	$to_header = ($to_header == '') ? 'undisclosed-recipients:;' : $to_header;
	$smtp->server_send("To: $to_header");

	// Now the CC Header.
	if ($cc_header != '')
	{
		$smtp->server_send("CC: $cc_header");
	}

	// Now any custom headers....
	$smtp->server_send("$headers\r\n");

	// Ok now we are ready for the message...
	$smtp->server_send($message);

	// Ok the all the ingredients are mixed in let's cook this puppy...
	$smtp->server_send('.');
	if ($err_msg = $smtp->server_parse('250', __LINE__))
	{
		$smtp->close_session($err_msg);
		return false;
	}

	// Now tell the server we are done and close the socket...
	$smtp->server_send('QUIT');
	$smtp->close_session($err_msg);

	return true;
}

/**
* SMTP Class
* Auth Mechanisms originally taken from the AUTH Modules found within the PHP Extension and Application Repository (PEAR)
* See docs/AUTHORS for more details
* @package phpBB3
*/
class smtp_class
{
	var $server_response = '';
	var $socket = 0;
	var $responses = array();
	var $commands = array();
	var $numeric_response_code = 0;

	var $backtrace = false;
	var $backtrace_log = array();

	function smtp_class()
	{
		// Always create a backtrace for admins to identify SMTP problems
		$this->backtrace = true;
		$this->backtrace_log = array();
	}

	/**
	* Add backtrace message for debugging
	*/
	function add_backtrace($message)
	{
		if ($this->backtrace)
		{
			$this->backtrace_log[] = utf8_htmlspecialchars($message);
		}
	}

	/**
	* Send command to smtp server
	*/
	function server_send($command, $private_info = false)
	{
		fputs($this->socket, $command . "\r\n");

		(!$private_info) ? $this->add_backtrace("# $command") : $this->add_backtrace('# Omitting sensitive information');

		// We could put additional code here
	}

	/**
	* We use the line to give the support people an indication at which command the error occurred
	*/
	function server_parse($response, $line)
	{
		global $user;

		$this->server_response = '';
		$this->responses = array();
		$this->numeric_response_code = 0;

		while (substr($this->server_response, 3, 1) != ' ')
		{
			if (!($this->server_response = fgets($this->socket, 256)))
			{
				return (isset($user->lang['NO_EMAIL_RESPONSE_CODE'])) ? $user->lang['NO_EMAIL_RESPONSE_CODE'] : 'Could not get mail server response codes';
			}
			$this->responses[] = substr(rtrim($this->server_response), 4);
			$this->numeric_response_code = (int) substr($this->server_response, 0, 3);

			$this->add_backtrace("LINE: $line <- {$this->server_response}");
		}

		if (!(substr($this->server_response, 0, 3) == $response))
		{
			$this->numeric_response_code = (int) substr($this->server_response, 0, 3);
			return (isset($user->lang['EMAIL_SMTP_ERROR_RESPONSE'])) ? sprintf($user->lang['EMAIL_SMTP_ERROR_RESPONSE'], $line, $this->server_response) : "Ran into problems sending Mail at <strong>Line $line</strong>. Response: $this->server_response";
		}

		return 0;
	}

	/**
	* Close session
	*/
	function close_session(&$err_msg)
	{
		fclose($this->socket);

		if ($this->backtrace)
		{
			$message = '<h1>Backtrace</h1><p>' . implode('<br />', $this->backtrace_log) . '</p>';
			$err_msg .= $message;
		}
	}
	
	/**
	* Log into server and get possible auth codes if neccessary
	*/
	function log_into_server($hostname, $username, $password, $default_auth_method)
	{
		global $user;

		$err_msg = '';
		$local_host = (function_exists('php_uname')) ? php_uname('n') : $user->host;

		// If we are authenticating through pop-before-smtp, we
		// have to login ones before we get authenticated
		// NOTE: on some configurations the time between an update of the auth database takes so
		// long that the first email send does not work. This is not a biggie on a live board (only
		// the install mail will most likely fail) - but on a dynamic ip connection this might produce
		// severe problems and is not fixable!
		if ($default_auth_method == 'POP-BEFORE-SMTP' && $username && $password)
		{
			global $config;

			$errno = 0;
			$errstr = '';

			$this->server_send("QUIT");
			fclose($this->socket);

			$result = $this->pop_before_smtp($hostname, $username, $password);
			$username = $password = $default_auth_method = '';

			// We need to close the previous session, else the server is not
			// able to get our ip for matching...
			if (!$this->socket = @fsockopen($config['smtp_host'], $config['smtp_port'], $errno, $errstr, 10))
			{
				if ($errstr)
				{
					$errstr = utf8_convert_message($errstr);
				}

				$err_msg = (isset($user->lang['NO_CONNECT_TO_SMTP_HOST'])) ? sprintf($user->lang['NO_CONNECT_TO_SMTP_HOST'], $errno, $errstr) : "Could not connect to smtp host : $errno : $errstr";
				return $err_msg;
			}

			// Wait for reply
			if ($err_msg = $this->server_parse('220', __LINE__))
			{
				$this->close_session($err_msg);
				return $err_msg;
			}
		}

		// Try EHLO first
		$this->server_send("EHLO {$local_host}");
		if ($err_msg = $this->server_parse('250', __LINE__))
		{
			// a 503 response code means that we're already authenticated
			if ($this->numeric_response_code == 503)
			{
				return false;
			}

			// If EHLO fails, we try HELO			
			$this->server_send("HELO {$local_host}");
			if ($err_msg = $this->server_parse('250', __LINE__))
			{
				return ($this->numeric_response_code == 503) ? false : $err_msg;
			}
		}

		foreach ($this->responses as $response)
		{
			$response = explode(' ', $response);
			$response_code = $response[0];
			unset($response[0]);
			$this->commands[$response_code] = implode(' ', $response);
		}

		// If we are not authenticated yet, something might be wrong if no username and passwd passed
		if (!$username || !$password)
		{
			return false;
		}
		
		if (!isset($this->commands['AUTH']))
		{
			return (isset($user->lang['SMTP_NO_AUTH_SUPPORT'])) ? $user->lang['SMTP_NO_AUTH_SUPPORT'] : 'SMTP server does not support authentication';
		}

		// Get best authentication method
		$available_methods = explode(' ', $this->commands['AUTH']);

		// Define the auth ordering if the default auth method was not found
		$auth_methods = array('PLAIN', 'LOGIN', 'CRAM-MD5', 'DIGEST-MD5');
		$method = '';

		if (in_array($default_auth_method, $available_methods))
		{
			$method = $default_auth_method;
		}
		else
		{
			foreach ($auth_methods as $_method)
			{
				if (in_array($_method, $available_methods))
				{
					$method = $_method;
					break;
				}
			}
		}

		if (!$method)
		{
			return (isset($user->lang['NO_SUPPORTED_AUTH_METHODS'])) ? $user->lang['NO_SUPPORTED_AUTH_METHODS'] : 'No supported authentication methods';
		}

		$method = strtolower(str_replace('-', '_', $method));
		return $this->$method($username, $password);
	}

	/**
	* Pop before smtp authentication
	*/
	function pop_before_smtp($hostname, $username, $password)
	{
		global $user;

		if (!$this->socket = @fsockopen($hostname, 110, $errno, $errstr, 10))
		{
			if ($errstr)
			{
				$errstr = utf8_convert_message($errstr);
			}

			return (isset($user->lang['NO_CONNECT_TO_SMTP_HOST'])) ? sprintf($user->lang['NO_CONNECT_TO_SMTP_HOST'], $errno, $errstr) : "Could not connect to smtp host : $errno : $errstr";
		}

		$this->server_send("USER $username", true);
		if ($err_msg = $this->server_parse('+OK', __LINE__))
		{
			return $err_msg;
		}

		$this->server_send("PASS $password", true);
		if ($err_msg = $this->server_parse('+OK', __LINE__))
		{
			return $err_msg;
		}

		$this->server_send('QUIT');
		fclose($this->socket);

		return false;
	}

	/**
	* Plain authentication method
	*/
	function plain($username, $password)
	{
		$this->server_send('AUTH PLAIN');
		if ($err_msg = $this->server_parse('334', __LINE__))
		{
			return ($this->numeric_response_code == 503) ? false : $err_msg;
		}

		$base64_method_plain = base64_encode("\0" . $username . "\0" . $password);
		$this->server_send($base64_method_plain, true);
		if ($err_msg = $this->server_parse('235', __LINE__))
		{
			return $err_msg;
		}

		return false;
	}

	/**
	* Login authentication method
	*/
	function login($username, $password)
	{
		$this->server_send('AUTH LOGIN');
		if ($err_msg = $this->server_parse('334', __LINE__))
		{
			return ($this->numeric_response_code == 503) ? false : $err_msg;
		}

		$this->server_send(base64_encode($username), true);
		if ($err_msg = $this->server_parse('334', __LINE__))
		{
			return $err_msg;
		}

		$this->server_send(base64_encode($password), true);
		if ($err_msg = $this->server_parse('235', __LINE__))
		{
			return $err_msg;
		}

		return false;
	}

	/**
	* cram_md5 authentication method
	*/
	function cram_md5($username, $password)
	{
		$this->server_send('AUTH CRAM-MD5');
		if ($err_msg = $this->server_parse('334', __LINE__))
		{
			return ($this->numeric_response_code == 503) ? false : $err_msg;
		}

		$md5_challenge = base64_decode($this->responses[0]);
		$password = (strlen($password) > 64) ? pack('H32', md5($password)) : ((strlen($password) < 64) ? str_pad($password, 64, chr(0)) : $password);
		$md5_digest = md5((substr($password, 0, 64) ^ str_repeat(chr(0x5C), 64)) . (pack('H32', md5((substr($password, 0, 64) ^ str_repeat(chr(0x36), 64)) . $md5_challenge))));

		$base64_method_cram_md5 = base64_encode($username . ' ' . $md5_digest);

		$this->server_send($base64_method_cram_md5, true);
		if ($err_msg = $this->server_parse('235', __LINE__))
		{
			return $err_msg;
		}

		return false;
	}

	/**
	* digest_md5 authentication method
	* A real pain in the ***
	*/
	function digest_md5($username, $password)
	{
		global $config, $user;

		$this->server_send('AUTH DIGEST-MD5');
		if ($err_msg = $this->server_parse('334', __LINE__))
		{
			return ($this->numeric_response_code == 503) ? false : $err_msg;
		}

		$md5_challenge = base64_decode($this->responses[0]);
		
		// Parse the md5 challenge - from AUTH_SASL (PEAR)
		$tokens = array();
		while (preg_match('/^([a-z-]+)=("[^"]+(?<!\\\)"|[^,]+)/i', $md5_challenge, $matches))
		{
			// Ignore these as per rfc2831
			if ($matches[1] == 'opaque' || $matches[1] == 'domain')
			{
				$md5_challenge = substr($md5_challenge, strlen($matches[0]) + 1);
				continue;
			}

			// Allowed multiple "realm" and "auth-param"
			if (!empty($tokens[$matches[1]]) && ($matches[1] == 'realm' || $matches[1] == 'auth-param'))
			{
				if (is_array($tokens[$matches[1]]))
				{
					$tokens[$matches[1]][] = preg_replace('/^"(.*)"$/', '\\1', $matches[2]);
				}
				else
				{
					$tokens[$matches[1]] = array($tokens[$matches[1]], preg_replace('/^"(.*)"$/', '\\1', $matches[2]));
				}
			}
			else if (!empty($tokens[$matches[1]])) // Any other multiple instance = failure
			{
				$tokens = array();
				break;
			}
			else
			{
				$tokens[$matches[1]] = preg_replace('/^"(.*)"$/', '\\1', $matches[2]);
			}

			// Remove the just parsed directive from the challenge
			$md5_challenge = substr($md5_challenge, strlen($matches[0]) + 1);
		}

		// Realm
		if (empty($tokens['realm']))
		{
			$tokens['realm'] = (function_exists('php_uname')) ? php_uname('n') : $user->host;
		}

		// Maxbuf
		if (empty($tokens['maxbuf']))
		{
			$tokens['maxbuf'] = 65536;
		}

		// Required: nonce, algorithm
		if (empty($tokens['nonce']) || empty($tokens['algorithm']))
		{
			$tokens = array();
		}
		$md5_challenge = $tokens;

		if (!empty($md5_challenge))
		{
			$str = '';
			for ($i = 0; $i < 32; $i++)
			{
				$str .= chr(mt_rand(0, 255));
			}
			$cnonce = base64_encode($str);

			$digest_uri = 'smtp/' . $config['smtp_host'];

			$auth_1 = sprintf('%s:%s:%s', pack('H32', md5(sprintf('%s:%s:%s', $username, $md5_challenge['realm'], $password))), $md5_challenge['nonce'], $cnonce);
			$auth_2 = 'AUTHENTICATE:' . $digest_uri;
			$response_value = md5(sprintf('%s:%s:00000001:%s:auth:%s', md5($auth_1), $md5_challenge['nonce'], $cnonce, md5($auth_2)));

			$input_string = sprintf('username="%s",realm="%s",nonce="%s",cnonce="%s",nc="00000001",qop=auth,digest-uri="%s",response=%s,%d', $username, $md5_challenge['realm'], $md5_challenge['nonce'], $cnonce, $digest_uri, $response_value, $md5_challenge['maxbuf']);
		}
		else
		{
			return (isset($user->lang['INVALID_DIGEST_CHALLENGE'])) ? $user->lang['INVALID_DIGEST_CHALLENGE'] : 'Invalid digest challenge';
		}

		$base64_method_digest_md5 = base64_encode($input_string);
		$this->server_send($base64_method_digest_md5, true);
		if ($err_msg = $this->server_parse('334', __LINE__))
		{
			return $err_msg;
		}

		$this->server_send(' ');
		if ($err_msg = $this->server_parse('235', __LINE__))
		{
			return $err_msg;
		}

		return false;
	}
}

/**
* Encodes the given string for proper display in UTF-8.
*
* This version is using base64 encoded data. The downside of this
* is if the mail client does not understand this encoding the user
* is basically doomed with an unreadable subject.
*
* Please note that this version fully supports RFC 2045 section 6.8.
*/
function mail_encode($str)
{
	// define start delimimter, end delimiter and spacer
	$start = "=?UTF-8?B?";
	$end = "?=";
	$spacer = $end . ' ' . $start;
	$split_length = 64;

	$encoded_str = base64_encode($str);

	// If encoded string meets the limits, we just return with the correct data.
	if (strlen($encoded_str) <= $split_length)
	{
		return $start . $encoded_str . $end;
	}

	// If there is only ASCII data, we just return what we want, correctly splitting the lines.
	if (strlen($str) === utf8_strlen($str))
	{
		return $start . implode($spacer, str_split($encoded_str, $split_length)) . $end;
	}

	// UTF-8 data, compose encoded lines
	$array = utf8_str_split($str);
	$str = '';

	while (sizeof($array))
	{
		$text = '';

		while (sizeof($array) && intval((strlen($text . $array[0]) + 2) / 3) << 2 <= $split_length)
		{
			$text .= array_shift($array);
		}

		$str .= $start . base64_encode($text) . $end . ' ';
	}

	return substr($str, 0, -1);
}

?>