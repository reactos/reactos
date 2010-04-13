using System;
using System.Xml;

using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks 
{
    /// <summary>Sets a property in the current project.</summary>
    /// <remarks>
    ///   <note>NAnt uses a number of predefined properties.</note>
    /// </remarks>
    /// <example>
    ///   <para>Define a <c>debug</c> property with the value <c>true</c>.</para>
    ///   <code><![CDATA[<property name="debug" value="true"/>]]></code>
    ///   <para>Use the user-defined <c>debug</c> property.</para>
    ///   <code><![CDATA[<property name="trace" value="${debug}"/>]]></code>
    ///   <para>Define a Read-Only property.</para><para>This is just like passing in the param on the command line.</para>
    ///   <code><![CDATA[<property name="do_not_touch_ME" value="hammer" readonly="true"/>]]></code>
    /// </example>
    [TaskName("property")]
    public class PropertyTask : PropertyBaseTask 
    {
    }
}