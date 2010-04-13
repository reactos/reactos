using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("contributor")]
    public class ContributorTask : Task
    {
        private RBuildContributor m_Contributor = new RBuildContributor();

        [TaskAttribute("firstname", Required = true)]
        public string FirstName { get { return m_Contributor.FirstName; } set { m_Contributor.FirstName = value; } }

        [TaskAttribute("city")]
        public string City { get { return m_Contributor.City; } set { m_Contributor.City = value; } }

        [TaskAttribute("country")]
        public string Country { get { return m_Contributor.Country; } set { m_Contributor.Country = value; } }

        [TaskAttribute("lastname")]
        public string LastName { get { return m_Contributor.LastName; } set { m_Contributor.LastName = value; } }

        [TaskAttribute("alias")]
        public string Alias { get { return m_Contributor.Alias; } set { m_Contributor.Alias = value; } }

        [TaskAttribute("mail")]
        public string Mail { get { return m_Contributor.Mail; } set { m_Contributor.Mail = value; } }

        [TaskAttribute("website")]
        public string Website { get { return m_Contributor.Website; } set { m_Contributor.Website = value; } }

        [TaskAttribute("active")]
        public bool Active { get { return m_Contributor.Active; } set { m_Contributor.Active = value; } }

        protected override void ExecuteTask()
        {
            Project.Contributors.Add(m_Contributor);
        }
    }
}