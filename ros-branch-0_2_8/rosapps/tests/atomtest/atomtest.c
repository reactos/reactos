#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <ddk/ntddk.h>

#define BUFFER_SIZE 256

int main(int argc, char* argv[])
{
   PRTL_ATOM_TABLE AtomTable = NULL;
   RTL_ATOM AtomA = -1, AtomB = -1, AtomC = -1;
   NTSTATUS Status;
   WCHAR Buffer[BUFFER_SIZE];
   ULONG NameLength, Data1, Data2;

   printf("Atom table test app\n\n");

   printf("RtlCreateAtomTable()\n");
   Status = RtlCreateAtomTable(37,
			       &AtomTable);
   printf("  Status 0x%08lx\n", Status);

   if (NT_SUCCESS(Status))
     {
	printf("  AtomTable %p\n", AtomTable);

	printf("RtlAddAtomToAtomTable()\n");
	Status = RtlAddAtomToAtomTable(AtomTable,
				       L"TestAtomA",
				       &AtomA);
	printf("  Status 0x%08lx\n", Status);
	if (NT_SUCCESS(Status))
	  {
	     printf("  AtomA 0x%x\n", AtomA);
	  }

	printf("RtlAddAtomToAtomTable()\n");
	Status = RtlAddAtomToAtomTable(AtomTable,
				       L"TestAtomB",
				       &AtomB);
	printf("  Status 0x%08lx\n", Status);
	if (NT_SUCCESS(Status))
	  {
	     printf("  AtomB 0x%x\n", AtomB);
	  }


	printf("RtlLookupAtomInAtomTable()\n");
	Status = RtlLookupAtomInAtomTable(AtomTable,
					  L"TestAtomA",
					  &AtomC);
	printf("  Status 0x%08lx\n", Status);
	if (NT_SUCCESS(Status))
	  {
	     printf("  AtomC 0x%x\n", AtomC);
	  }


	printf("RtlPinAtomInAtomTable()\n");
	Status = RtlPinAtomInAtomTable(AtomTable,
				       AtomC);
	printf("  Status 0x%08lx\n", Status);

	printf("RtlPinAtomInAtomTable()\n");
	Status = RtlPinAtomInAtomTable(AtomTable,
				       AtomC);
	printf("  Status 0x%08lx\n", Status);


//	printf("RtlDeleteAtomFromAtomTable()\n");
//	Status = RtlDeleteAtomFromAtomTable(AtomTable,
//					    AtomC);
//	printf("  Status 0x%08lx\n", Status);


//	printf("RtlEmptyAtomTable()\n");
//	Status = RtlEmptyAtomTable(AtomTable,
//				   TRUE);
//	printf("  Status 0x%08lx\n", Status);


//	printf("RtlLookupAtomInAtomTable()\n");
//	Status = RtlLookupAtomInAtomTable(AtomTable,
//					  L"TestAtomA",
//					  &AtomC);
//	printf("  Status 0x%08lx\n", Status);


	printf("RtlQueryAtomInAtomTable()\n");
	NameLength = sizeof(WCHAR) * BUFFER_SIZE;
	Status = RtlQueryAtomInAtomTable(AtomTable,
					 AtomC,
					 &Data1,
					 &Data2,
					 Buffer,
					 &NameLength);
	printf("  Status 0x%08lx\n", Status);
	if (NT_SUCCESS(Status))
	  {
	     printf("  RefCount %ld\n", Data1);
	     printf("  PinCount %ld\n", Data2);
	     printf("  NameLength %lu\n", NameLength);
	     printf("  AtomName: %S\n", Buffer);
	  }

	printf("RtlDestroyAtomTable()\n");
	RtlDestroyAtomTable(AtomTable);


	printf("Atom table test app finished\n");
     }

   return(0);
}
