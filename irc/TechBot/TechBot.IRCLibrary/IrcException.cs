using System;

namespace TechBot.IRCLibrary
{
	/// <summary>
	/// Base class for all IRC exceptions.
	/// </summary>
	public class IrcException : Exception
	{
		public IrcException() : base()
		{
		}

		public IrcException(string message) : base(message)
		{
		}

		public IrcException(string message, Exception innerException) : base(message, innerException)
		{
		}
	}

	/// <summary>
	/// Thrown when there is no connection to an IRC server.
	/// </summary>
	public class NotConnectedException : IrcException
	{
	}

	/// <summary>
	/// Thrown when there is an attempt to connect to an IRC server and there is already a connection.
	/// </summary>
	public class AlreadyConnectedException : IrcException
	{
	}

	/// <summary>
	/// Thrown when there is attempted to parse a malformed or invalid IRC message.
	/// </summary>
	public class MalformedMessageException : IrcException
	{
		public MalformedMessageException(string message) : base(message)
		{
		}

		public MalformedMessageException(string message, Exception innerException) : base(message, innerException)
		{
		}
	}
}
