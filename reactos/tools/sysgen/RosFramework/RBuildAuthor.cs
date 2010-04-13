using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public enum AuthorRole
    {
        Developer,
        Mantainer,
        Translator
    }

    public class RBuildAuthor
    {
        private AuthorRole m_AuthorRole = AuthorRole.Developer;
        private RBuildContributor m_Contributor = null;

        /// <summary>
        /// The underlying contributor.
        /// </summary>
        public RBuildContributor Contributor
        {
            get { return m_Contributor; }
            set { m_Contributor = value; }
        }

        /// <summary>
        /// The author role.
        /// </summary>
        public AuthorRole Role
        {
            get { return m_AuthorRole; }
            set { m_AuthorRole = value; }
        }
    }
}
