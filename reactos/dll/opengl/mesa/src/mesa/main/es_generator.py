#*************************************************************************
# Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# TUNGSTEN GRAPHICS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
# OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#*************************************************************************


import sys, os
import APIspecutil as apiutil

# These dictionary entries are used for automatic conversion.
# The string will be used as a format string with the conversion
# variable.
Converters = {
    'GLfloat': {
        'GLdouble': "(GLdouble) (%s)",
        'GLfixed' : "(GLint) (%s * 65536)",
    },
    'GLfixed': {
        'GLfloat': "(GLfloat) (%s / 65536.0f)",
        'GLdouble': "(GLdouble) (%s / 65536.0)",
    },
    'GLdouble': {
        'GLfloat': "(GLfloat) (%s)",
        'GLfixed': "(GLfixed) (%s * 65536)",
    },
    'GLclampf': {
        'GLclampd': "(GLclampd) (%s)",
        'GLclampx': "(GLclampx) (%s * 65536)",
    },
    'GLclampx': {
        'GLclampf': "(GLclampf) (%s / 65536.0f)",
        'GLclampd': "(GLclampd) (%s / 65536.0)",
    },
    'GLubyte': {
        'GLfloat': "(GLfloat) (%s / 255.0f)",
    },
}

def GetBaseType(type):
    typeTokens = type.split(' ')
    baseType = None
    typeModifiers = []
    for t in typeTokens:
        if t in ['const', '*']:
            typeModifiers.append(t)
        else:
            baseType = t
    return (baseType, typeModifiers)

def ConvertValue(value, fromType, toType):
    """Returns a string that represents the given parameter string, 
    type-converted if necessary."""

    if not Converters.has_key(fromType):
        print >> sys.stderr, "No base converter for type '%s' found.  Ignoring." % fromType
        return value

    if not Converters[fromType].has_key(toType):
        print >> sys.stderr, "No converter found for type '%s' to type '%s'.  Ignoring." % (fromType, toType)
        return value

    # This part is simple.  Return the proper conversion.
    conversionString = Converters[fromType][toType]
    return conversionString % value

FormatStrings = {
    'GLenum' : '0x%x',
    'GLfloat' : '%f',
    'GLint' : '%d',
    'GLbitfield' : '0x%x',
}
def GetFormatString(type):
    if FormatStrings.has_key(type):
        return FormatStrings[type]
    else:
        return None


######################################################################
# Version-specific values to be used in the main script
# header: which header file to include
# api: what text specifies an API-level function
VersionSpecificValues = {
    'GLES1.1' : {
        'description' : 'GLES1.1 functions',
        'header' : 'GLES/gl.h',
        'extheader' : 'GLES/glext.h',
        'shortname' : 'es1'
    },
    'GLES2.0': {
        'description' : 'GLES2.0 functions',
        'header' : 'GLES2/gl2.h',
        'extheader' : 'GLES2/gl2ext.h',
        'shortname' : 'es2'
    }
}


######################################################################
# Main code for the script begins here.

# Get the name of the program (without the directory part) for use in
# error messages.
program = os.path.basename(sys.argv[0])

# Set default values
verbose = 0
functionList = "APIspec.xml"
version = "GLES1.1"

# Allow for command-line switches
import getopt, time
options = "hvV:S:"
try:
    optlist, args = getopt.getopt(sys.argv[1:], options)
except getopt.GetoptError, message:
    sys.stderr.write("%s: %s.  Use -h for help.\n" % (program, message))
    sys.exit(1)

for option, optarg in optlist:
    if option == "-h":
        sys.stderr.write("Usage: %s [-%s]\n" % (program, options))
        sys.stderr.write("Parse an API specification file and generate wrapper functions for a given GLES version\n")
        sys.stderr.write("-h gives help\n")
        sys.stderr.write("-v is verbose\n")
        sys.stderr.write("-V specifies GLES version to generate [%s]:\n" % version)
        for key in VersionSpecificValues.keys():
            sys.stderr.write("    %s - %s\n" % (key, VersionSpecificValues[key]['description']))
        sys.stderr.write("-S specifies API specification file to use [%s]\n" % functionList)
        sys.exit(1)
    elif option == "-v":
        verbose += 1
    elif option == "-V":
        version = optarg
    elif option == "-S":
        functionList = optarg

# Beyond switches, we support no further command-line arguments
if len(args) >  0:
    sys.stderr.write("%s: only switch arguments are supported - use -h for help\n" % program)
    sys.exit(1)

# If we don't have a valid version, abort.
if not VersionSpecificValues.has_key(version):
    sys.stderr.write("%s: version '%s' is not valid - use -h for help\n" % (program, version))
    sys.exit(1)

# Grab the version-specific items we need to use
versionHeader = VersionSpecificValues[version]['header']
versionExtHeader = VersionSpecificValues[version]['extheader']
shortname = VersionSpecificValues[version]['shortname']

# If we get to here, we're good to go.  The "version" parameter
# directs GetDispatchedFunctions to only allow functions from
# that "category" (version in our parlance).  This allows 
# functions with different declarations in different categories
# to exist (glTexImage2D, for example, is different between
# GLES1 and GLES2).
keys = apiutil.GetAllFunctions(functionList, version)

allSpecials = apiutil.AllSpecials()

print """/* DO NOT EDIT *************************************************
 * THIS FILE AUTOMATICALLY GENERATED BY THE %s SCRIPT
 * API specification file:   %s
 * GLES version:             %s
 * date:                     %s
 */
""" % (program, functionList, version, time.strftime("%Y-%m-%d %H:%M:%S"))

# The headers we choose are version-specific.
print """
#include "%s"
#include "%s"
#include "main/mfeatures.h"
#include "main/compiler.h"
#include "main/api_exec.h"

#if FEATURE_%s

#ifndef GLAPIENTRYP
#define GLAPIENTRYP GL_APIENTRYP
#endif
""" % (versionHeader, versionExtHeader, shortname.upper())

# Everyone needs these types.
print """
/* These types are needed for the Mesa veneer, but are not defined in
 * the standard GLES headers.
 */
typedef double GLdouble;
typedef double GLclampd;

/* Mesa error handling requires these */
extern void *_mesa_get_current_context(void);
extern void _mesa_error(void *ctx, GLenum error, const char *fmtString, ... );
"""

# Finally we get to the all-important functions
print """/*************************************************************
 * Generated functions begin here
 */
"""
for funcName in keys:
    if verbose > 0: sys.stderr.write("%s: processing function %s\n" % (program, funcName))

    # start figuring out what this function will look like.
    returnType = apiutil.ReturnType(funcName)
    props = apiutil.Properties(funcName)
    params = apiutil.Parameters(funcName)
    declarationString = apiutil.MakeDeclarationString(params)

    # In case of error, a function may have to return.  Make
    # sure we have valid return values in this case.
    if returnType == "void":
        errorReturn = "return"
    elif returnType == "GLboolean":
        errorReturn = "return GL_FALSE"
    else:
        errorReturn = "return (%s) 0" % returnType

    # These are the output of this large calculation block.
    # passthroughDeclarationString: a typed set of parameters that
    # will be used to create the "extern" reference for the
    # underlying Mesa or support function.  Note that as generated
    # these have an extra ", " at the beginning, which will be
    # removed before use.
    # 
    # passthroughDeclarationString: an untyped list of parameters
    # that will be used to call the underlying Mesa or support
    # function (including references to converted parameters).
    # This will also be generated with an extra ", " at the
    # beginning, which will be removed before use.
    #
    # variables: C code to create any local variables determined to
    # be necessary.
    # conversionCodeOutgoing: C code to convert application parameters
    # to a necessary type before calling the underlying support code.
    # May be empty if no conversion is required.  
    # conversionCodeIncoming: C code to do the converse: convert 
    # values returned by underlying Mesa code to the types needed
    # by the application.
    # Note that *either* the conversionCodeIncoming will be used (for
    # generated query functions), *or* the conversionCodeOutgoing will
    # be used (for generated non-query functions), never both.
    passthroughFuncName = ""
    passthroughDeclarationString = ""
    passthroughCallString = ""
    prefixOverride = None
    variables = []
    conversionCodeOutgoing = []
    conversionCodeIncoming = []
    switchCode = []

    # Calculate the name of the underlying support function to call.
    # By default, the passthrough function is named _mesa_<funcName>.
    # We're allowed to override the prefix and/or the function name
    # for each function record, though.  The "ConversionFunction"
    # utility is poorly named, BTW...
    if funcName in allSpecials:
        # perform checks and pass through
        funcPrefix = "_check_"
        aliasprefix = "_es_"
    else:
        funcPrefix = "_es_"
        aliasprefix = apiutil.AliasPrefix(funcName)
    alias = apiutil.ConversionFunction(funcName)
    prefixOverride = apiutil.FunctionPrefix(funcName)
    if prefixOverride != "_mesa_":
        aliasprefix = apiutil.FunctionPrefix(funcName)
    if not alias:
        # There may still be a Mesa alias for the function
        if apiutil.Alias(funcName):
            passthroughFuncName = "%s%s" % (aliasprefix, apiutil.Alias(funcName))
        else:
            passthroughFuncName = "%s%s" % (aliasprefix, funcName)
    else: # a specific alias is provided
        passthroughFuncName = "%s%s" % (aliasprefix, alias)

    # Look at every parameter: each one may have only specific
    # allowed values, or dependent parameters to check, or 
    # variant-sized vector arrays to calculate
    for (paramName, paramType, paramMaxVecSize, paramConvertToType, paramValidValues, paramValueConversion) in params:
        # We'll need this below if we're doing conversions
        (paramBaseType, paramTypeModifiers) = GetBaseType(paramType)

        # Conversion management.
        # We'll handle three cases, easiest to hardest: a parameter
        # that doesn't require conversion, a scalar parameter that
        # requires conversion, and a vector parameter that requires
        # conversion.
        if paramConvertToType == None:
            # Unconverted parameters are easy, whether they're vector
            # or scalar - just add them to the call list.  No conversions
            # or anything to worry about.
            passthroughDeclarationString += ", %s %s" % (paramType, paramName)
            passthroughCallString += ", %s" % paramName

        elif paramMaxVecSize == 0: # a scalar parameter that needs conversion
            # A scalar to hold a converted parameter
            variables.append("    %s converted_%s;" % (paramConvertToType, paramName))

            # Outgoing conversion depends on whether we have to conditionally
            # perform value conversion.
            if paramValueConversion == "none":
                conversionCodeOutgoing.append("    converted_%s = (%s) %s;" % (paramName, paramConvertToType, paramName))
            elif paramValueConversion == "some":
                # We'll need a conditional variable to keep track of
                # whether we're converting values or not.
                if ("    int convert_%s_value = 1;" % paramName) not in variables:
                    variables.append("    int convert_%s_value = 1;" % paramName)

                # Write code based on that conditional.
                conversionCodeOutgoing.append("    if (convert_%s_value) {" % paramName)
                conversionCodeOutgoing.append("        converted_%s = %s;" % (paramName, ConvertValue(paramName, paramBaseType, paramConvertToType))) 
                conversionCodeOutgoing.append("    } else {")
                conversionCodeOutgoing.append("        converted_%s = (%s) %s;" % (paramName, paramConvertToType, paramName))
                conversionCodeOutgoing.append("    }")
            else: # paramValueConversion == "all"
                conversionCodeOutgoing.append("    converted_%s = %s;" % (paramName, ConvertValue(paramName, paramBaseType, paramConvertToType)))

            # Note that there can be no incoming conversion for a
            # scalar parameter; changing the scalar will only change
            # the local value, and won't ultimately change anything
            # that passes back to the application.

            # Call strings.  The unusual " ".join() call will join the
            # array of parameter modifiers with spaces as separators.
            passthroughDeclarationString += ", %s %s %s" % (paramConvertToType, " ".join(paramTypeModifiers), paramName)
            passthroughCallString += ", converted_%s" % paramName

        else: # a vector parameter that needs conversion
            # We'll need an index variable for conversions
            if "    register unsigned int i;" not in variables:
                variables.append("    register unsigned int i;")

            # This variable will hold the (possibly variant) size of
            # this array needing conversion.  By default, we'll set
            # it to the maximal size (which is correct for functions
            # with a constant-sized vector parameter); for true
            # variant arrays, we'll modify it with other code.
            variables.append("    unsigned int n_%s = %d;" % (paramName, paramMaxVecSize))

            # This array will hold the actual converted values.
            variables.append("    %s converted_%s[%d];" % (paramConvertToType, paramName, paramMaxVecSize))

            # Again, we choose the conversion code based on whether we
            # have to always convert values, never convert values, or 
            # conditionally convert values.
            if paramValueConversion == "none":
                conversionCodeOutgoing.append("    for (i = 0; i < n_%s; i++) {" % paramName)
                conversionCodeOutgoing.append("        converted_%s[i] = (%s) %s[i];" % (paramName, paramConvertToType, paramName))
                conversionCodeOutgoing.append("    }")
            elif paramValueConversion == "some":
                # We'll need a conditional variable to keep track of
                # whether we're converting values or not.
                if ("    int convert_%s_value = 1;" % paramName) not in variables:
                    variables.append("    int convert_%s_value = 1;" % paramName)
                # Write code based on that conditional.
                conversionCodeOutgoing.append("    if (convert_%s_value) {" % paramName)
                conversionCodeOutgoing.append("        for (i = 0; i < n_%s; i++) {" % paramName)
                conversionCodeOutgoing.append("            converted_%s[i] = %s;" % (paramName, ConvertValue("%s[i]" % paramName, paramBaseType, paramConvertToType))) 
                conversionCodeOutgoing.append("        }")
                conversionCodeOutgoing.append("    } else {")
                conversionCodeOutgoing.append("        for (i = 0; i < n_%s; i++) {" % paramName)
                conversionCodeOutgoing.append("            converted_%s[i] = (%s) %s[i];" % (paramName, paramConvertToType, paramName))
                conversionCodeOutgoing.append("        }")
                conversionCodeOutgoing.append("    }")
            else: # paramValueConversion == "all"
                conversionCodeOutgoing.append("    for (i = 0; i < n_%s; i++) {" % paramName)
                conversionCodeOutgoing.append("        converted_%s[i] = %s;" % (paramName, ConvertValue("%s[i]" % paramName, paramBaseType, paramConvertToType)))

                conversionCodeOutgoing.append("    }")

            # If instead we need an incoming conversion (i.e. results
            # from Mesa have to be converted before handing back
            # to the application), this is it.  Fortunately, we don't
            # have to worry about conditional value conversion - the
            # functions that do (e.g. glGetFixedv()) are handled
            # specially, outside this code generation.
            #
            # Whether we use incoming conversion or outgoing conversion
            # is determined later - we only ever use one or the other.

            if paramValueConversion == "none":
                conversionCodeIncoming.append("    for (i = 0; i < n_%s; i++) {" % paramName)
                conversionCodeIncoming.append("        %s[i] = (%s) converted_%s[i];" % (paramName, paramConvertToType, paramName))
                conversionCodeIncoming.append("    }")
            elif paramValueConversion == "some":
                # We'll need a conditional variable to keep track of
                # whether we're converting values or not.
                if ("    int convert_%s_value = 1;" % paramName) not in variables:
                    variables.append("    int convert_%s_value = 1;" % paramName)

                # Write code based on that conditional.
                conversionCodeIncoming.append("    if (convert_%s_value) {" % paramName)
                conversionCodeIncoming.append("        for (i = 0; i < n_%s; i++) {" % paramName)
                conversionCodeIncoming.append("            %s[i] = %s;" % (paramName, ConvertValue("converted_%s[i]" % paramName, paramConvertToType, paramBaseType))) 
                conversionCodeIncoming.append("        }")
                conversionCodeIncoming.append("    } else {")
                conversionCodeIncoming.append("        for (i = 0; i < n_%s; i++) {" % paramName)
                conversionCodeIncoming.append("            %s[i] = (%s) converted_%s[i];" % (paramName, paramBaseType, paramName))
                conversionCodeIncoming.append("        }")
                conversionCodeIncoming.append("    }")
            else: # paramValueConversion == "all"
                conversionCodeIncoming.append("    for (i = 0; i < n_%s; i++) {" % paramName)
                conversionCodeIncoming.append("        %s[i] = %s;" % (paramName, ConvertValue("converted_%s[i]" % paramName, paramConvertToType, paramBaseType)))
                conversionCodeIncoming.append("    }")

            # Call strings.  The unusual " ".join() call will join the
            # array of parameter modifiers with spaces as separators.
            passthroughDeclarationString += ", %s %s %s" % (paramConvertToType, " ".join(paramTypeModifiers), paramName)
            passthroughCallString += ", converted_%s" % paramName

        # endif conversion management

        # Parameter checking.  If the parameter has a specific list of
        # valid values, we have to make sure that the passed-in values
        # match these, or we make an error.
        if len(paramValidValues) > 0:
            # We're about to make a big switch statement with an
            # error at the end.  By default, the error is GL_INVALID_ENUM,
            # unless we find a "case" statement in the middle with a
            # non-GLenum value.
            errorDefaultCase = "GL_INVALID_ENUM"

            # This parameter has specific valid values.  Make a big
            # switch statement to handle it.  Note that the original
            # parameters are always what is checked, not the
            # converted parameters.
            switchCode.append("    switch(%s) {" % paramName)

            for valueIndex in range(len(paramValidValues)):
                (paramValue, dependentVecSize, dependentParamName, dependentValidValues, errorCode, valueConvert) = paramValidValues[valueIndex]

                # We're going to need information on the dependent param
                # as well.
                if dependentParamName:
                    depParamIndex = apiutil.FindParamIndex(params, dependentParamName)
                    if depParamIndex == None:
                        sys.stderr.write("%s: can't find dependent param '%s' for function '%s'\n" % (program, dependentParamName, funcName))

                    (depParamName, depParamType, depParamMaxVecSize, depParamConvertToType, depParamValidValues, depParamValueConversion) = params[depParamIndex]
                else:
                    (depParamName, depParamType, depParamMaxVecSize, depParamConvertToType, depParamValidValues, depParamValueConversion) = (None, None, None, None, [], None)

                # This is a sneaky trick.  It's valid syntax for a parameter
                # that is *not* going to be converted to be declared
                # with a dependent vector size; but in this case, the
                # dependent vector size is unused and unnecessary.
                # So check for this and ignore the dependent vector size
                # if the parameter is not going to be converted.
                if depParamConvertToType:
                    usedDependentVecSize = dependentVecSize
                else:
                    usedDependentVecSize = None

                # We'll peek ahead at the next parameter, to see whether
                # we can combine cases
                if valueIndex + 1 < len(paramValidValues) :
                    (nextParamValue, nextDependentVecSize, nextDependentParamName, nextDependentValidValues, nextErrorCode, nextValueConvert) = paramValidValues[valueIndex + 1]
                    if depParamConvertToType:
                        usedNextDependentVecSize = nextDependentVecSize
                    else:
                        usedNextDependentVecSize = None

                # Create a case for this value.  As a mnemonic,
                # if we have a dependent vector size that we're ignoring,
                # add it as a comment.
                if usedDependentVecSize == None and dependentVecSize != None:
                    switchCode.append("        case %s: /* size %s */" % (paramValue, dependentVecSize))
                else:
                    switchCode.append("        case %s:" % paramValue)

                # If this is not a GLenum case, then switch our error
                # if no value is matched to be GL_INVALID_VALUE instead
                # of GL_INVALID_ENUM.  (Yes, this does get confused
                # if there are both values and GLenums in the same
                # switch statement, which shouldn't happen.)
                if paramValue[0:3] != "GL_":
                    errorDefaultCase = "GL_INVALID_VALUE"

                # If all the remaining parameters are identical to the
                # next set, then we're done - we'll just create the
                # official code on the next pass through, and the two
                # cases will share the code.
                if valueIndex + 1 < len(paramValidValues) and usedDependentVecSize == usedNextDependentVecSize and dependentParamName == nextDependentParamName and dependentValidValues == nextDependentValidValues and errorCode == nextErrorCode and valueConvert == nextValueConvert:
                    continue

                # Otherwise, we'll have to generate code for this case.
                # Start off with a check: if there is a dependent parameter,
                # and a list of valid values for that parameter, we need
                # to generate an error if something other than one
                # of those values is passed.
                if len(dependentValidValues) > 0:
                    conditional=""

                    # If the parameter being checked is actually an array,
                    # check only its first element.
                    if depParamMaxVecSize == 0:
                        valueToCheck = dependentParamName
                    else:
                        valueToCheck = "%s[0]" % dependentParamName

                    for v in dependentValidValues:
                        conditional += " && %s != %s" % (valueToCheck, v)
                    switchCode.append("            if (%s) {" % conditional[4:])
                    if errorCode == None:
                        errorCode = "GL_INVALID_ENUM"
                    switchCode.append('                _mesa_error(_mesa_get_current_context(), %s, "gl%s(%s=0x%s)", %s);' % (errorCode, funcName, paramName, "%x", paramName))
                    switchCode.append("                %s;" % errorReturn)
                    switchCode.append("            }")
                # endif there are dependent valid values

                # The dependent parameter may require conditional
                # value conversion.  If it does, and we don't want
                # to convert values, we'll have to generate code for that
                if depParamValueConversion == "some" and valueConvert == "noconvert":
                    switchCode.append("            convert_%s_value = 0;" % dependentParamName)

                # If there's a dependent vector size for this parameter
                # that we're actually going to use (i.e. we need conversion),
                # mark it.
                if usedDependentVecSize:
                    switchCode.append("            n_%s = %s;" % (dependentParamName, dependentVecSize))

                # In all cases, break out of the switch if any valid
                # value is found.
                switchCode.append("            break;")


            # Need a default case to catch all the other, invalid
            # parameter values.  These will all generate errors.
            switchCode.append("        default:")
            if errorCode == None:
                errorCode = "GL_INVALID_ENUM"
            formatString = GetFormatString(paramType)
            if formatString == None:
                switchCode.append('            _mesa_error(_mesa_get_current_context(), %s, "gl%s(%s)");' % (errorCode, funcName, paramName))
            else:
                switchCode.append('            _mesa_error(_mesa_get_current_context(), %s, "gl%s(%s=%s)", %s);' % (errorCode, funcName, paramName, formatString, paramName))
            switchCode.append("            %s;" % errorReturn)

            # End of our switch code.
            switchCode.append("    }")

        # endfor every recognized parameter value

    # endfor every param

    # Here, the passthroughDeclarationString and passthroughCallString
    # are complete; remove the extra ", " at the front of each.
    passthroughDeclarationString = passthroughDeclarationString[2:]
    passthroughCallString = passthroughCallString[2:]
    if not passthroughDeclarationString:
        passthroughDeclarationString = "void"

    # The Mesa functions are scattered across all the Mesa
    # header files.  The easiest way to manage declarations
    # is to create them ourselves.
    if funcName in allSpecials:
        print "/* this function is special and is defined elsewhere */"
    print "extern %s GL_APIENTRY %s(%s);" % (returnType, passthroughFuncName, passthroughDeclarationString)

    # A function may be a core function (i.e. it exists in
    # the core specification), a core addition (extension
    # functions added officially to the core), a required
    # extension (usually an extension for an earlier version
    # that has been officially adopted), or an optional extension.
    #
    # Core functions have a simple category (e.g. "GLES1.1");
    # we generate only a simple callback for them.
    #
    # Core additions have two category listings, one simple
    # and one compound (e.g.  ["GLES1.1", "GLES1.1:OES_fixed_point"]).  
    # We generate the core function, and also an extension function.
    #
    # Required extensions and implemented optional extensions
    # have a single compound category "GLES1.1:OES_point_size_array".
    # For these we generate just the extension function.
    for categorySpec in apiutil.Categories(funcName):
        compoundCategory = categorySpec.split(":")

        # This category isn't for us, if the base category doesn't match
        # our version
        if compoundCategory[0] != version:
            continue

        # Otherwise, determine if we're writing code for a core
        # function (no suffix) or an extension function.
        if len(compoundCategory) == 1:
            # This is a core function
            extensionName = None
            extensionSuffix = ""
        else:
            # This is an extension function.  We'll need to append
            # the extension suffix.
            extensionName = compoundCategory[1]
            extensionSuffix = extensionName.split("_")[0]
        fullFuncName = funcPrefix + funcName + extensionSuffix

        # Now the generated function.  The text used to mark an API-level
        # function, oddly, is version-specific.
        if extensionName:
            print "/* Extension %s */" % extensionName

        if (not variables and
            not switchCode and
            not conversionCodeOutgoing and
            not conversionCodeIncoming):
            # pass through directly
            print "#define %s %s" % (fullFuncName, passthroughFuncName)
            print
            continue

        print "static %s GL_APIENTRY %s(%s)" % (returnType, fullFuncName, declarationString)
        print "{"

        # Start printing our code pieces.  Start with any local
        # variables we need.  This unusual syntax joins the 
        # lines in the variables[] array with the "\n" separator.
        if len(variables) > 0:
            print "\n".join(variables) + "\n"

        # If there's any sort of parameter checking or variable
        # array sizing, the switch code will contain it.
        if len(switchCode) > 0:
            print "\n".join(switchCode) + "\n"

        # In the case of an outgoing conversion (i.e. parameters must
        # be converted before calling the underlying Mesa function),
        # use the appropriate code.
        if "get" not in props and len(conversionCodeOutgoing) > 0:
            print "\n".join(conversionCodeOutgoing) + "\n"

        # Call the Mesa function.  Note that there are very few functions
        # that return a value (i.e. returnType is not "void"), and that
        # none of them require incoming translation; so we're safe
        # to generate code that directly returns in those cases,
        # even though it's not completely independent.

        if returnType == "void":
            print "    %s(%s);" % (passthroughFuncName, passthroughCallString)
        else:
            print "    return %s(%s);" % (passthroughFuncName, passthroughCallString)

        # If the function is one that returns values (i.e. "get" in props),
        # it might return values of a different type than we need, that
        # require conversion before passing back to the application.
        if "get" in props and len(conversionCodeIncoming) > 0:
            print "\n".join(conversionCodeIncoming)

        # All done.
        print "}"
        print
    # end for each category provided for a function

# end for each function

print """
#include "glapi/glapi.h"

#if FEATURE_remap_table

/* define esLocalRemapTable */
#include "main/api_exec_%s_dispatch.h"

#define need_MESA_remap_table
#include "main/api_exec_%s_remap_helper.h"

static void
init_remap_table(void)
{
   _glthread_DECLARE_STATIC_MUTEX(mutex);
   static GLboolean initialized = GL_FALSE;
   const struct gl_function_pool_remap *remap = MESA_remap_table_functions;
   int i;

   _glthread_LOCK_MUTEX(mutex);
   if (initialized) {
      _glthread_UNLOCK_MUTEX(mutex);
      return;
   }

   for (i = 0; i < esLocalRemapTable_size; i++) {
      GLint offset;
      const char *spec;

      /* sanity check */
      ASSERT(i == remap[i].remap_index);
      spec = _mesa_function_pool + remap[i].pool_index;

      offset = _mesa_map_function_spec(spec);
      esLocalRemapTable[i] = offset;
   }
   initialized = GL_TRUE;
   _glthread_UNLOCK_MUTEX(mutex);
}

#else /* FEATURE_remap_table */

#include "%sapi/main/dispatch.h"

static INLINE void
init_remap_table(void)
{
}

#endif /* FEATURE_remap_table */

struct _glapi_table *
_mesa_create_exec_table_%s(void)
{
   struct _glapi_table *exec;

   exec = _mesa_alloc_dispatch_table(_gloffset_COUNT);
   if (exec == NULL)
      return NULL;

   init_remap_table();
""" % (shortname, shortname, shortname, shortname)

for func in keys:
    prefix = "_es_" if func not in allSpecials else "_check_"
    for spec in apiutil.Categories(func):
        ext = spec.split(":")
        # version does not match
        if ext.pop(0) != version:
            continue
        entry = func
        if ext:
            suffix = ext[0].split("_")[0]
            entry += suffix
        print "    SET_%s(exec, %s%s);" % (entry, prefix, entry)
print ""
print "   return exec;"
print "}"

print """
#endif /* FEATURE_%s */""" % (shortname.upper())
