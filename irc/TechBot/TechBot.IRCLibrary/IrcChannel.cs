/*
   Channels names are strings (beginning with a '&' or '#' character) of
   length up to 200 characters.  Apart from the the requirement that the
   first character being either '&' or '#'; the only restriction on a
   channel name is that it may not contain any spaces (' '), a control G
   (^G or ASCII 7), or a comma (',' which is used as a list item
   separator by the protocol).
 */
using System;
using System.Collections;

namespace TechBot.IRCLibrary
{
	/// <summary>
	/// IRC channel type.
	/// </summary>
	public enum IrcChannelType
	{
		Public,
		Private,
		Secret
	}



	/// <summary>
	/// IRC channel.
	/// </summary>
	public class IrcChannel
	{
		#region Private fields

		private IrcClient owner;
		private string name;
		private IrcChannelType type = IrcChannelType.Public;
		private ArrayList users = new ArrayList();

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
		/// Name of channel (no leading #).
		/// </summary>
		public string Name
		{
			get
			{
				return name;
			}
		}

		/// <summary>
		/// Type of channel.
		/// </summary>
		public IrcChannelType Type
		{
			get
			{
				return type;
			}
		}

		/// <summary>
		/// Users in this channel.
		/// </summary>
		public ArrayList Users
		{
			get
			{
				return users;
			}
		}

		#endregion

		/// <summary>
		/// Constructor.
		/// </summary>
		/// <param name="owner">Owner of this channel.</param>
		/// <param name="name">Name of channel.</param>
		public IrcChannel(IrcClient owner, string name)
		{
			if (owner == null)
			{
				throw new ArgumentNullException("owner", "Owner cannot be null.");
			}
			if (name == null)
			{
				throw new ArgumentNullException("name", "Name cannot be null.");
			}
			this.owner = owner;
			this.name = name;
		}

		/// <summary>
		/// Locate a user.
		/// </summary>
		/// <param name="nickname">Nickname of user (no decorations).</param>
		/// <returns>User or null if not found.</returns>
		public IrcUser LocateUser(string nickname)
		{
			foreach (IrcUser user in Users)
			{
				/* FIXME: There are special cases for nickname comparison */
				if (nickname.ToLower().Equals(user.Nickname.ToLower()))
				{
					return user;
				}
			}
			return null;
		}

		/// <summary>
		/// Talk to the channel.
		/// </summary>
		/// <param name="text">Text to send to the channel.</param>
		public void Talk(string text)
		{
			owner.SendMessage(new IrcMessage(IRC.PRIVMSG,
			                                 String.Format("#{0} :{1}",
			                                               name,
			                                               text)));
		}
	}
}
