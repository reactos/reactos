using System;

namespace TechBot.IRCLibrary
{
	/*
	<message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
	<prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
	<command>  ::= <letter> { <letter> } | <number> <number> <number>
	<SPACE>    ::= ' ' { ' ' }
	<params>   ::= <SPACE> [ ':' <trailing> | <middle> <params> ]

	<middle>   ::= <Any *non-empty* sequence of octets not including SPACE
				   or NUL or CR or LF, the first of which may not be ':'>
	<trailing> ::= <Any, possibly *empty*, sequence of octets not including
					 NUL or CR or LF>

	<crlf>     ::= CR LF

	NOTES:

	  1)    <SPACE> is consists only of SPACE character(s) (0x20).
			Specially notice that TABULATION, and all other control
			characters are considered NON-WHITE-SPACE.

	  2)    After extracting the parameter list, all parameters are equal,
			whether matched by <middle> or <trailing>. <Trailing> is just
			a syntactic trick to allow SPACE within parameter.

	  3)    The fact that CR and LF cannot appear in parameter strings is
			just artifact of the message framing. This might change later.

	  4)    The NUL character is not special in message framing, and
			basically could end up inside a parameter, but as it would
			cause extra complexities in normal C string handling. Therefore
			NUL is not allowed within messages.

	  5)    The last parameter may be an empty string.

	  6)    Use of the extended prefix (['!' <user> ] ['@' <host> ]) must
			not be used in server to server communications and is only
			intended for server to client messages in order to provide
			clients with more useful information about who a message is
			from without the need for additional queries.
	 */
	/*
   NOTICE AUTH :*** Looking up your hostname
   NOTICE AUTH :*** Checking Ident
   NOTICE AUTH :*** Found your hostname
   NOTICE AUTH :*** No ident response
	 */

	/// <summary>
	/// IRC message.
	/// </summary>
	public class IrcMessage
	{
		#region Private fields
		private string line;
		private string prefix;
		private string prefixServername;
		private string prefixNickname;
		private string prefixUser;
		private string prefixHost;
		private string command;
		private string parameters;
		#endregion

		/// <summary>
		/// Line of text that is to be parsed as an IRC message.
		/// </summary>
		public string Line
		{
			get
			{
				return line;
			}
		}

		/// <summary>
		/// Does the message have a prefix?
		/// </summary>
		public bool HasPrefix
		{
			get
			{
				return prefix != null;
			}
		}

		/// <summary>
		/// Prefix or null if none exists.
		/// </summary>
		public string Prefix
		{
			get
			{
				return prefix;
			}
		}

		/// <summary>
		/// Servername part of prefix or null if no prefix or servername exists.
		/// </summary>
		public string PrefixServername
		{
			get
			{
				return prefixServername;
			}
		}

		/// <summary>
		/// Nickname part of prefix or null if no prefix or nick exists.
		/// </summary>
		public string PrefixNickname
		{
			get
			{
				return prefixNickname;
			}
		}

		/// <summary>
		/// User part of (extended) prefix or null if no (extended) prefix exists.
		/// </summary>
		public string PrefixUser
		{
			get
			{
				return prefixUser;
			}
		}

		/// <summary>
		/// Host part of (extended) prefix or null if no (extended) prefix exists.
		/// </summary>
		public string PrefixHost
		{
			get
			{
				return prefixHost;
			}
		}

		/// <summary>
		/// Command part of message.
		/// </summary>
		public string Command
		{
			get
			{
				return command;
			}
		}

		/// <summary>
		/// Is command numeric?
		/// </summary>
		public bool IsCommandNumeric
		{
			get
			{
				if (command == null || command.Length != 3)
				{
					return false;
				}
				try
				{
					Int32.Parse(command);
					return true;
				}
				catch (Exception)
				{
					return false;
				}
			}
		}

		/// <summary>
		/// Command part of message as text.
		/// </summary>
		public string CommandText
		{
			get
			{
				return command;
			}
		}

		/// <summary>
		/// Command part of message as a number.
		/// </summary>
		/// <exception cref="InvalidOperationException">Thrown if IsCommandNumeric returns false.</exception>
		public int CommandNumber
		{
			get
			{
				if (IsCommandNumeric)
				{
					return Int32.Parse(command);
				}
				else
				{
					throw new InvalidOperationException();
				}
			}
		}

		/// <summary>
		/// Parameters part of message.
		/// </summary>
		public string Parameters
		{
			get
			{
				return parameters;
			}
		}

		/// <summary>
		/// Constructor.
		/// </summary>
		/// <param name="line">Line of text that is to be parsed as an IRC message.</param>
		public IrcMessage(string line)
		{
			/*
			 * <message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
			 * <prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
			 * :Oslo1.NO.EU.undernet.org 461 MYNICK USER :Not enough parameters
			 */
			try
			{
				this.line = line;
				int i = 0;

				#region Prefix
				if (line[i].Equals(':'))
				{
					i++;
					prefix = "";
					/* This message has a prefix */
					string s = "";
					while (i < line.Length && line[i] != ' ' && line[i] != '!' && line[i] != '@')
					{
						s += line[i++];
					}
					if (IsValidIrcNickname(s))
					{
						prefixNickname = s;
						prefix += prefixNickname;
						if (line[i] == '!')
						{
							/* This message has an extended prefix */
							i++;
							s = "";
							while (i < line.Length && line[i] != ' ' && line[i] != '@')
							{
								s += line[i];
								i++;
							}
							prefixUser = s;
							prefix += "!" + prefixUser;
						}
						if (line[i] == '@')
						{
							/* This message has a host prefix */
							s = "";
							do
							{
								s += line[++i];
							}
							while (i < line.Length && line[i] != ' ');
							prefixHost = s;
							prefix += "@" + prefixHost;
						}
					}
					else /* Assume it is a servername */
					{
						prefixServername = s;
						prefix += prefixServername;
					}

					/* Skip spaces */
					while (i < line.Length && line[i] == ' ')
					{
						i++;
					}
				}
				else
				{
					prefix = null;
				}
				#endregion

				#region Command
				if (Char.IsDigit(line[i]))
				{
					if (!Char.IsDigit(line, i + 1) || !Char.IsDigit(line, i + 2))
					{
						throw new Exception();
					}
					command = String.Format("{0}{1}{2}", line[i++], line[i++], line[i++]);
				}
				else
				{
					command = "";
					while (i < line.Length && Char.IsLetter(line[i]))
					{
						command += line[i];
						i++;
					}
				}
				#endregion

				#region Parameters
				while (true)
				{
					/* Skip spaces */
					while (i < line.Length && line[i] == ' ')
					{
						i++;
					}
					if (i < line.Length && line[i].Equals(':'))
					{
						i++;

						/* Trailing */
						while (i < line.Length && line[i] != ' ' && line[i] != '\r' && line[i] != '\n' && line[i] != 0)
						{
							if (parameters == null)
							{
								parameters = "";
							}
							parameters += line[i];
							i++;
						}
					}
					else
					{
						/* Middle */
						while (i < line.Length && line[i] != '\r' && line[i] != '\n' && line[i] != 0)
						{
							if (parameters == null)
							{
								parameters = "";
							}
							parameters += line[i];
							i++;
						}
					}
					if (i >= line.Length)
					{
						break;
					}
				}
				#endregion
			}
			catch (Exception ex)
			{
				throw new MalformedMessageException("The message is malformed.", ex);
			}
		}

		/// <summary>
		/// Constructor.
		/// </summary>
		/// <param name="prefixServername"></param>
		/// <param name="prefixNickname"></param>
		/// <param name="prefixUser"></param>
		/// <param name="prefixHost"></param>
		/// <param name="command"></param>
		/// <param name="parameters"></param>
		public IrcMessage(string prefixServername,
			string prefixNickname,
			string prefixUser,
			string prefixHost,
			string command,
			string parameters)
		{
			throw new NotImplementedException();
		}

		/// <summary>
		/// Constructor.
		/// </summary>
		/// <param name="command">IRC command.</param>
		/// <param name="parameters">IRC command parameters. May be null if there are no parameters.</param>
		public IrcMessage(string command,
			string parameters)
		{
			if (command == null || !IsValidIrcCommand(command))
			{
				throw new ArgumentException("Command is not a valid IRC command.", "command");
			}
			/* An IRC message must not be longer than 512 characters (including terminating CRLF) */
			int parametersLength = (parameters != null) ? 1 + parameters.Length : 0;
			if (command.Length + parametersLength > 510)
			{
				throw new MalformedMessageException("IRC message cannot be longer than 512 characters.");
			}
			this.command = command;
			this.parameters = parameters;
			if (parameters != null)
			{
				this.line = String.Format("{0} {1}\r\n", command, parameters);
			}
			else
			{
				this.line = String.Format("{0}\r\n", command);
			}
		}

		/// <summary>
		/// Returns wether a string of text is a valid IRC command.
		/// </summary>
		/// <param name="command">The IRC command.</param>
		/// <returns>True, if <c ref="command">command</c> is a valid IRC command, false if not.</returns>
		private static bool IsValidIrcCommand(string command)
		{
			foreach (char c in command)
			{
				if (!Char.IsLetter(c))
				{
					return false;
				}
			}
			return true;
		}

		private const string IrcSpecial = @"-[]\`^{}";
		private const string IrcSpecialNonSpecs = @"_|";

		/// <summary>
		/// Returns wether a character is an IRC special character.
		/// </summary>
		/// <param name="c">Character to test.</param>
		/// <returns>True if the character is an IRC special character, false if not.</returns>
		private static bool IsSpecial(char c)
		{
			foreach (char validCharacter in IrcSpecial)
			{
				if (c.Equals(validCharacter))
				{
					return true;
				}
			}
			foreach (char validCharacter in IrcSpecialNonSpecs)
			{
				if (c.Equals(validCharacter))
				{
					return true;
				}
			}
			return false;
		}

		/// <summary>
		/// Returns wether a string of text is a valid IRC nickname.
		/// </summary>
		/// <param name="nickname">The IRC nickname.</param>
		/// <returns>True, if <c ref="nickname">nickname</c> is a valid IRC nickname, false if not.</returns>
		private static bool IsValidIrcNickname(string nickname)
		{
			/*
			 * <nick>       ::= <letter> { <letter> | <number> | <special> }
			 * <letter>     ::= 'a' ... 'z' | 'A' ... 'Z'
			 * <number>     ::= '0' ... '9'
   			 * <special>    ::= '-' | '[' | ']' | '\' | '`' | '^' | '{' | '}'
			 */
			/* An IRC nicknmame must be 1 - 9 characters in length. We don't care so much if it is larger */
			if ((nickname.Length < 1) || (nickname.Length > 30))
			{
				return false;
			}
			/* First character must be a letter. */
			if (!Char.IsLetter(nickname[0]))
			{
				return false;
			}
			/* Check the other valid characters for validity. */
			foreach (char c in nickname)
			{
				if (!Char.IsLetter(c) && !Char.IsDigit(c) && !IsSpecial(c))
				{
					return false;
				}
			}
			return true;
		}

		/// <summary>
		/// Write contents to a string.
		/// </summary>
		/// <returns>Contents as a string.</returns>
		public override string ToString()
		{
			return String.Format("Line({0})Prefix({1})Command({2})Parameters({3})",
				line, prefix != null ? prefix : "(null)",
				command != null ? command : "(null)",
				parameters != null ? parameters : "(null)");
		}
	}
}
