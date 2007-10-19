#ifndef COMPONENT_FACTORY_TEMPLATE_H_ //comp_factory.h
#define COMPONENT_FACTORY_TEMPLATE_H_

/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/comp_factory.h
 * PURPOSE:     component management
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */


#include "user_types.h"
#include <map>
#include <vector>

namespace System_
{
  using std::map;
  using std::vector;

//----------------------------------------------------------
///
/// ComponentFactoryTemplate
///
/// This class servess as a factory class for components. In general
/// can be used for any possible class which has base class
/// and a derrived class
///

//--------------------------------------------------------------------
///
/// create
///
/// Description: this template function creates a function which returns
/// objects of type ElementType but in real are objects of DerrivedType
///

  template<class ElementType, class DerrivedType>
	static ElementType * create ()
  {
    return new DerrivedType();
  }

//--------------------------------------------------------------------

  template<typename ElementType, typename ElementId>
    class ComponentFactoryTemplate
    {
      typedef ElementType * (*creation_function)();
      typedef map<ElementId, creation_function> ElementMap;
      typedef vector<ElementId> ElementIdVector;
    public:

//--------------------------------------------------------------------
///
/// ComponentFactoryTemplate
///
/// Description: default destructor of class ComponentFactoryTemplate
///

      ComponentFactoryTemplate()
      {

      }

//--------------------------------------------------------------------
///
/// ~ComponentFactoryTemplate
///
/// Description: default destructor of class ComponentFactoryTemplate
///

      virtual ~ComponentFactoryTemplate()
      {

      }
//--------------------------------------------------------------------
///
/// listComponent
///
/// Description: lists all registererd components ids

	  void listComponentIds()
	  {
         for(unsigned i = 0; i < ids_.size (); i++)
  		   cout<<ids_[i]<<endl;
	  }
//--------------------------------------------------------------------
///
/// isComponentRegistered
///
/// Description: this function just checks if a component with a given
/// id has been registered or not. If it is it returns true else false
///
/// @return bool
/// @param element id of the component

  bool isComponentRegistered(ElementId const & id)
  {
     typename ElementMap::const_iterator iter = elements_.find(id);
     if(iter != elements_.end())
	   return true;
	 else
	   return false;
  }

//--------------------------------------------------------------------
///
/// getNumOfComponents
///
/// Description: returns how many components have been registered

  const unsigned getNumOfComponents()
  {
    return ids_.size ();
  }

//--------------------------------------------------------------------
///
/// registerComponent
///
/// Description: this function is template function in a class template.
/// The purpose is that at compiletime for each of the derriving classes of
/// ElementType a registerComponent function is generated and then all classes
/// of that type are added at runtime to factory.
/// returns zero on success, nonzero on failure
///
/// @return bool
/// @param id the element id of the content type
        template<typename ContentType>
	bool registerComponent(ElementId const & id)
	{
	  typename ElementMap::const_iterator iter = elements_.find(id);
	  if(iter != elements_.end())
	    return false;
	  ids_.insert(ids_.begin(),id);
	  elements_.insert(std::make_pair<ElementId, creation_function >(id, &create<ElementType, ContentType>));
	  return true;

	}

   //--------------------------------------------------------------------
///
/// deregisterComponent
///
/// Description: unregisters a given component indentified by param id
/// After unregistration, the specified component can no longer be
/// created
/// @return bool
/// @param id the element id of the ContentType

      template<typename ContentType>
	const bool deregisterComponent(ElementId const & id)
	{
	  typename ElementMap::const_iterator iterator = elements_.find(id);
	  if(iterator != elements_.end())
	  {
	    elements_.erase(iterator);
	    return true;
	  }
	  else
	    return false;
	}


//--------------------------------------------------------------------
///
/// createComponent
///
/// Description: creates a component according to the specified information
/// returns the component encapsulated in an std::auto_ptr
/// @param id specifies the key id, to look for
/// @return std::auto_ptr<ElementType>
      ElementType * createComponent(ElementId const & id) const
      {
      	typename ElementMap::const_iterator first_it = elements_.find(id);
 	    if(first_it != elements_.end())
	      return (first_it->second)();
		else
		  return 0;
     }

//--------------------------------------------------------------------
///
/// deleteComponent
///
/// Description: deletes a component
/// @param element the element to be deleted

    void deleteComponent(ElementType * element)
	{
	   delete element;
	}
	protected:
     ElementMap elements_;
	 ElementIdVector ids_;
  };


} //end of namespace Sysreg_

#endif

