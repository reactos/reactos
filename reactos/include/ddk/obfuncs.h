/* OBJECT MANAGER ************************************************************/

/*
 * FUNCTION: Decrements the object's reference count and performs retention
 * checks
 * ARGUMENTS:
 *        Object = Object's body
 */
VOID ObDereferenceObject(PVOID Object);

/*
 * FUNCTION: Performs access validation on an object handle and if access
 * is granted returns a pointer to the object's body
 * ARGUMENTS:
 *         Handle = Handle to the object 
 *         DesiredAccess = Desired access to the object
 *         ObjectType (OPTIONAL) = Pointer to the object's type definition
 *         AccessMode = Access mode to use for the check
 *         Object (OUT) = Caller supplied storage for a pointer to the object's
 *                        body
 *         HandleInformation (OUT) = Points to a structure which receives
 *                                   information about the handle
 * RETURNS: Status
 */
NTSTATUS ObReferenceObjectByHandle(HANDLE Handle,
				   ACCESS_MASK DesiredAccess,
				   POBJECT_TYPE ObjectType,
				   KPROCESSOR_MODE AccessMode,
				   PVOID* Object,
				   POBJECT_HANDLE_INFORMATION HandleInfo);

/*
 * FUNCTION: Increments the reference count for a given object
 * ARGUMENTS:
 *      Object = Points to the body of the object
 *      AccessMode = Requested access to the object
 *      ObjectType = Pointer to the object's type definition
 *      AccessMode = Access mode to use for the security check
 * RETURNS: Status
 */
NTSTATUS ObReferenceObjectByPointer(PVOID Object,
				    ACCESS_MASK DesiredAccess,
				    POBJECT_TYPE ObjectType,
				    KPROCESSOR_MODE AccessMode);

NTSTATUS ObReferenceObjectByName(PUNICODE_STRING ObjectPath,
				 ULONG Attributes,
				 PACCESS_STATE PassedAccessState,
				 ACCESS_MASK DesiredAccess,
				 POBJECT_TYPE ObjectType,
				 KPROCESSOR_MODE AccessMode,
				 PVOID ParseContext,
				 PVOID* ObjectPtr);

PVOID ObCreateObject(PHANDLE Handle,
		     ACCESS_MASK DesiredAccess,
		     POBJECT_ATTRIBUTES ObjectAttributes,
		     POBJECT_TYPE Type);
