using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;

using SysGen.BuildEngine.Attributes;
using SysGen.BuildEngine.Log;

namespace SysGen.BuildEngine.Tasks 
{
    /// <summary>
    /// The opposite of the <c>if</c> task.
    /// </summary>
    /// <example>
    ///   <para>Check existence of a property</para>
    ///   <code>
    ///   <![CDATA[
    ///   <ifnot propertyexists="myProp">
    ///     <echo message="myProp does not exist."/>
    ///   </if>
    ///   ]]></code>
    ///   
    ///   <para>Check that a property value is not true</para>
    ///   <code>
    ///   <![CDATA[
    ///   <ifnot propertytrue="myProp">
    ///     <echo message="myProp is not true."/>
    ///   </if>
    ///   ]]></code>
    /// </example>
    ///
    /// <example>
    ///   <para>Check that a target does not exist</para>
    ///   <code>
    ///   <![CDATA[
    ///   <ifnot targetexists="myTarget">
    ///     <echo message="myTarget does not exist."/>
    ///   </if>
    ///   ]]></code>
    /// </example>
    [TaskName("ifnot")]
    public class IfNotTask : IfTask
    {
        protected override bool ConditionsTrue
        {
            get { return !base.ConditionsTrue; }
        }
    }
}
