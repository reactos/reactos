using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using Lextm.SharpSnmpLib.Mib.Elements;
using Lextm.SharpSnmpLib.Mib.Elements.Entities;
using Lextm.SharpSnmpLib.Mib.Elements.Types;

namespace Lextm.SharpSnmpLib.Mib
{
	 public static class MibTypesResolver
	 {
		  private static readonly Regex              _namedOidPathRegex = new Regex(@"^(?<Name>[^\(]+)\((?<Value>\d+)\)$");
		  private static readonly List<IMibResolver> _resolver      = new List<IMibResolver>();
		  private static readonly List<IModule>      _cachedModules = new List<IModule>();

		  public static void RegisterResolver(IMibResolver resolver)
		  {
				if (resolver != null)
				{
					 _resolver.Add(resolver);
				}
		  }

		  public static IModule ResolveModule(string moduleName)
		  {
				// check if module is already cached
				foreach (MibModule cachedModule in _cachedModules)
				{
					 if (cachedModule.Name == moduleName)
					 {
						  return cachedModule;
					 }
				}

				foreach (IMibResolver resolver in _resolver)
				{
					 IModule resolvedModule = resolver.Resolve(moduleName);
					 if (resolvedModule != null)
					 {
						  ResolveTypes(resolvedModule);
						  _cachedModules.Add(resolvedModule);
						  return resolvedModule;
					 }
				}

				return null;
		  }

		  public static void ResolveTypes(IModule module)
		  {
				foreach (IEntity entity in module.Entities)
				{
					 ITypeReferrer typeReferringEntity = entity as ITypeReferrer;

					 if (typeReferringEntity != null)
					 {
						  CheckTypeReferrer(module, typeReferringEntity);
					 }
				}

				if (!_cachedModules.Contains(module))
				{
					 _cachedModules.Add(module);
				}
		  }

		  private static void CheckTypeReferrer(IModule module, ITypeReferrer typeReferringEntity)
		  {
				TypeAssignment unknownType = typeReferringEntity.ReferredType as TypeAssignment;
				if (unknownType != null)
				{
					 typeReferringEntity.ReferredType = ResolveType(module, unknownType);

					 if (typeReferringEntity.ReferredType is TypeAssignment)
					 {
						  Console.WriteLine(String.Format("Could not resolve type '{0}' declared in module '{1}'", (typeReferringEntity.ReferredType as TypeAssignment).Type, typeReferringEntity.ReferredType.Module.Name));
					 }
				}

				ITypeReferrer nextTypeReferringEntity = typeReferringEntity.ReferredType as ITypeReferrer;
				if (nextTypeReferringEntity != null)
				{
					 CheckTypeReferrer(module, nextTypeReferringEntity);
				}
		  }

		  public static ITypeAssignment ResolveType(IModule module, TypeAssignment type)
		  {
				ITypeAssignment result = ResolveDeclaration(module, type.Type) as ITypeAssignment;

				return (result != null) ? result : type;
		  }

		  public static IDeclaration ResolveDeclaration(IModule module, string name)
		  {
				if ((module == null) || String.IsNullOrEmpty(name))
				{
					 return null;
				}

				// check module internal types
				foreach (IDeclaration decl in module.Declarations)
				{
				if (decl.Name == name)
					 {
					return decl;
					 }
				}

				// check if type is imported
				if (module.Imports != null)
				{
					 ImportsFrom imports = module.Imports.GetImportFromType(name);
					 if (imports != null)
					 {
						  IModule importedModule = ResolveModule(imports.Module);
						  if (importedModule != null)
						  {
								return ResolveDeclaration(importedModule, name);
						  }
					 }
				}

				return null;
		  }

		  public static ObjectIdentifier ResolveOid(IEntity entity)
		  {
				ObjectIdentifier result = new ObjectIdentifier();

				if (entity != null)
				{
					 ResolveOid(entity, result);
				}

				return result;
		  }

		  private static void ResolveOid(IEntity entity, ObjectIdentifier result)
		  {
				result.Prepend(entity.Name, entity.Value);
				
				// check parent
				if (!String.IsNullOrEmpty(entity.Parent))
				{
					 string[] pathParts = entity.Parent.Split('.');
					 uint value;

					 // all parts except the first should have their value directly or indirectly with them
					 if (pathParts.Length > 1)
					 {
						  for (int i=pathParts.Length-1; i>=1; i--)
						  {
								if (uint.TryParse(pathParts[i], out value))
								{
									 result.Prepend("", value);
								}
								else
								{
									 Match m = _namedOidPathRegex.Match(pathParts[i]);
									 if (m.Success)
									 {
										  result.Prepend(m.Groups["Name"].Value, uint.Parse(m.Groups["Value"].Value));
									 }
									 else
									 {
										  throw new MibException("Invalid OID path detected for entity '" + entity.Name + "' in module '" + entity.Module + "'!");
									 }
								}
						  }
					 }

					 // parse root part: either another entity or a standard root object
					 if (IsOidRoot(pathParts[0], out value))
					 {
						  result.Prepend(pathParts[0], value);
					 }
					 else
					 {
						  // try to find entity inside this module
						  if (entity.Module != null)
						  {
								entity = ResolveDeclaration(entity.Module, pathParts[0]) as IEntity;

								if (entity != null)
								{
									 ResolveOid(entity, result);
								}
								else
								{
									 result.Prepend("", uint.MaxValue);
								}
						  }
						  else
						  {
								result.Prepend("", uint.MaxValue);
						  }
					 }
				}
		  }

		  public static bool IsOidRoot(string name, out uint value)
		  {
				value = uint.MaxValue;

				switch (name)
				{
					 case "ccitt": value = 0; return true;
					 case "iso": value = 1; return true;
					 case "joint-iso-ccitt": value = 2; return true;
				}

				return false;
		  }
	 }
}
