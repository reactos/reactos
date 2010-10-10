using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;

using SysGen.BuildEngine.Attributes;
using SysGen.BuildEngine.Log;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("if")]
    public class IfTask : TaskContainer
    {
        protected string _propName = null;
        protected string _propValue = null;
        protected string _propNameTrue = null;
        protected string _propNameExists = null;

        /// <summary>
        /// Used to test whether a property is true.
        /// </summary>
        [TaskAttribute("propertytrue")]
        public string PropertyNameTrue {
            set {_propNameTrue = value;}
        }

        /// <summary>
        /// Used to test whether a property exists.
        /// </summary>
        [TaskAttribute("propertyexists")]
        public string PropertyNameExists {
            set {_propNameExists = value;}
        }

        /// <summary>
        /// Used to test whether a property exists.
        /// </summary>
        [TaskAttribute("property")]
        public string PropertyName
        {
            set { _propName = value; }
        }

        /// <summary>
        /// Used to test whether a property exists.
        /// </summary>
        [TaskAttribute("value")]
        public string PropertyValue
        {
            set { _propValue = value; }
        }

        protected override void PreExecuteTask()
        {
            if (!ConditionsTrue)
            {
                m_ExecuteChilds = false;
            }
        }

        /*
        protected override void ExecuteTask() {
            if(!ConditionsTrue) {
                m_ExecuteChilds = false;
            }
        }*/

        protected virtual bool ConditionsTrue 
        {
            get 
            {
                bool ret = true;

                if (_propName != null)
                {
                    if (_propValue != null)
                    {
                        if (SysGen.Project.Properties.PropertyExists(_propName))
                        {
                            return (SysGen.Project.Properties[_propName].Value == _propValue);
                        }
                         
                        return false;
                    }
                    else
                    {
                        ret = ret && SysGen.Project.Properties.PropertyExists(_propNameExists);
                    }
                }

                ////check for target
                //if(_targetName != null) {
                //    ret = ret && (SysGen.Targets.Find(_targetName) != null);
                //    if (!ret) return false;
                //}

                //Check for the Property value of true.
                if (_propNameTrue != null)
                {
                    try
                    {
                        ret = ret && bool.Parse(SysGen.Project.Properties[_propNameTrue].Value);
                    }
                    catch (Exception e)
                    {
                        throw new BuildException("Property True test failed for '" + _propNameTrue + "'", Location, e);
                    }
                }

                //Check for Property existence
                if(_propNameExists != null) 
                {
                    ret = ret && SysGen.Project.Properties.PropertyExists(_propNameExists);
                }

                ////check for uptodate file
                //if(_uptodateFile != null) {
                //    FileInfo primaryFile = new FileInfo(_uptodateFile);
                //    if(primaryFile == null) {
                //        ret = true;
                //    }
                //    else {
                //        string newerFile = FileSet.FindMoreRecentLastWriteTime(_compareFiles.FileNames, primaryFile.LastWriteTime);
                //        bool bNeedsAnUpdate = (null == newerFile);
                //        BuildLog.WriteLineIf(SysGen.Verbose && bNeedsAnUpdate, "{0) is newer than {1}" , newerFile, primaryFile.Name);
                //        ret = !bNeedsAnUpdate;
                //    }
                //}

                return ret;
            }
        }
    }
}
