using System;

namespace TechBot.IRCLibrary
{
	/// <summary>
	/// IRC user.
	/// </summary>
	public class IrcUser
	{
		#region Private fields

		private IrcClient owner;
		private string nickname;
		private string decoratedNickname;

		#endregion

		#region Public properties

		/// <summary>
		/// Owner of this channel.
		/// </summary>
		public IrcClient Owner
		{
			get
			{
				return owner;
			}
		}

		/// <summary>
		/// Nickname of user.
		/// </summary>
		public string Nickname
		{
			get
			{
				return nickname;
			}
		}

		/// <summary>
		/// Decorated nickname of user.
		/// </summary>
		public string DecoratedNickname
		{
			get
			{
				return decoratedNickname;
			}
		}

		/// <summary>
		/// Wether user is channel operator.
		/// </summary>
		public bool Operator
		{
			get
			{
				return decoratedNickname.StartsWith("@");
			}
		}

		/// <summary>
		/// Wether user has voice.
		/// </summary>
		public bool Voice
		{
			get
			{
				return decoratedNickname.StartsWith("+");
			}
		}

		#endregion

		/// <summary>
		/// Constructor.
		/// </summary>
		/// <param name="owner">Owner of this channel.</param>
		/// <param name="nickname">Nickname (possibly decorated) of user.</param>
		public IrcUser(IrcClient owner,
		               string nickname)
		{
			if (owner == null)
			{
				throw new ArgumentNullException("owner", "Owner cannot be null.");
			}
			this.owner = owner;
			this.decoratedNickname = nickname.Trim();
			this.nickname = StripDecoration(decoratedNickname);
		}

		/// <summary>
		/// Talk to the user.
		/// </summary>
		/// <param name="text">Text to send to the user.</param>
		public void Talk(string text)
		{
			if (text == null)
			{
				throw new ArgumentNullException("text", "Text cannot be null.");
			}

			owner.SendMessage(new IrcMessage(IRC.PRIVMSG,
			                                 String.Format("{0} :{1}",
			                                               nickname,
			                                               text)));
		}

		/// <summary>
		/// Strip docoration of nickname.
		/// </summary>
		/// <param name="nickname">Possible decorated nickname.</param>
		/// <returns>Undecorated nickname.</returns>
		public static string StripDecoration(string decoratedNickname)
		{
			if (decoratedNickname.StartsWith("@"))
			{
				return decoratedNickname.Substring(1);
			}
			else if (decoratedNickname.StartsWith("+"))
			{
				return decoratedNickname.Substring(1);
			}
			else
			{
				return decoratedNickname;
			}
		}
	}
}
