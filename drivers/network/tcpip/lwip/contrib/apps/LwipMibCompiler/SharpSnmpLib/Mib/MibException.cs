/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/17
 * Time: 16:33
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

using System;
using System.Globalization;
#if (!SILVERLIGHT)
using System.Runtime.Serialization;
using System.Security.Permissions; 
#endif

namespace Lextm.SharpSnmpLib.Mib
{
    /// <summary>
    /// Description of MibException.
    /// </summary>
    [Serializable]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Mib")]
    public sealed class MibException : Exception
    {
        /// <summary>
        /// Symbol.
        /// </summary>
        public Symbol Symbol { get; private set; }

        /// <summary>
        /// Creates a <see cref="MibException"/>.
        /// </summary>
        public MibException()
        {
        }
        
        /// <summary>
        /// Creates a <see cref="SnmpException"/> instance with a specific <see cref="string"/>.
        /// </summary>
        /// <param name="message">Message</param>
        public MibException(string message) : base(message)
        {
        }

        /// <summary>
        /// Creates a <see cref="MibException"/> instance with a specific <see cref="string"/> and an <see cref="Exception"/>.
        /// </summary>
        /// <param name="message">Message</param>
        /// <param name="inner">Inner exception</param>
        public MibException(string message, Exception inner)
            : base(message, inner)
        {
        }
#if (!SILVERLIGHT)        
        /// <summary>
        /// Creates a <see cref="MibException"/> instance.
        /// </summary>
        /// <param name="info">Info</param>
        /// <param name="context">Context</param>
        private MibException(SerializationInfo info, StreamingContext context) : base(info, context)
        {
            if (info == null)
            {
                throw new ArgumentNullException("info");
            }
            
            Symbol = (Symbol)info.GetValue("Symbol", typeof(Symbol));
        }
        
        /// <summary>
        /// Gets object data.
        /// </summary>
        /// <param name="info">Info</param>
        /// <param name="context">Context</param>
        [SecurityPermission(SecurityAction.Demand, SerializationFormatter = true)]
        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            base.GetObjectData(info, context);
            info.AddValue("Symbol", Symbol);
        }
#endif

        /// <summary>
        /// Creates a <see cref="MibException"/> with a specific <see cref="Symbol"/>.
        /// </summary>
        /// <param name="message">Message</param>
        /// <param name="symbol">Symbol</param>
        /// <returns></returns>
        public static MibException Create(string message, Symbol symbol)
        {
            if (symbol == null)
            {
                throw new ArgumentNullException("symbol");
            }

            if (String.IsNullOrEmpty(message))
            {
                message = "Unknown MIB Exception";
            }

            message = String.Format(
                "{0} (file: \"{1}\"; row: {2}; column: {3})",
                message,
                symbol.File,
                symbol.Row + 1,
                symbol.Column + 1);

            MibException ex = new MibException(message) { Symbol = symbol };
            return ex;
        }
    }
}