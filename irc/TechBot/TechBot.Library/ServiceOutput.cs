using System;

namespace TechBot.Library
{
	public interface IServiceOutput
	{
		void WriteLine(MessageContext context,
		               string message);
	}
}
