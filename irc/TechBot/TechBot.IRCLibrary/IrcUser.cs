using System;

namespace TechBot.IRCLibrary
{
	/// <summary>
	/// IRC user.
	/// </summary>
	public class IrcUser
	{
		#region Private fields

		private string nickname;
		private string decoratedNickname;

		#endregion

		#region Public properties

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
		/// <param name="nickname">Nickname (possibly decorated) of user.</param>
		public IrcUser(string nickname)
		{
			this.decoratedNickname = nickname.Trim();
			this.nickname = StripDecoration(decoratedNickname);
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
