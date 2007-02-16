/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/env.c
 * PURPOSE:         Environment functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

PPEB NTAPI RtlpCurrentPeb(VOID);
/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS NTAPI
RtlCreateEnvironment(BOOLEAN Inherit,
                     PWSTR *Environment)
{
   MEMORY_BASIC_INFORMATION MemInfo;
   PVOID EnvPtr = NULL;
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG RegionSize = PAGE_SIZE;

   if (Inherit == TRUE)
   {
      RtlAcquirePebLock();

      if (NtCurrentPeb()->ProcessParameters->Environment != NULL)
      {
         Status = NtQueryVirtualMemory(NtCurrentProcess(),
                                       NtCurrentPeb()->ProcessParameters->Environment,
                                       MemoryBasicInformation,
                                       &MemInfo,
                                       sizeof(MEMORY_BASIC_INFORMATION),
                                       NULL);
         if (!NT_SUCCESS(Status))
         {
            RtlReleasePebLock();
            *Environment = NULL;
            return(Status);
         }

         RegionSize = MemInfo.RegionSize;
         Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                          &EnvPtr,
                                          0,
                                          &RegionSize,
                                          MEM_RESERVE | MEM_COMMIT,
                                          PAGE_READWRITE);
         if (!NT_SUCCESS(Status))
         {
            RtlReleasePebLock();
            *Environment = NULL;
            return(Status);
         }

         memmove(EnvPtr,
                 NtCurrentPeb ()->ProcessParameters->Environment,
                 MemInfo.RegionSize);

         *Environment = EnvPtr;
      }

      RtlReleasePebLock ();
   }
   else
   {
      Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                       &EnvPtr,
                                       0,
                                       &RegionSize,
                                       MEM_RESERVE | MEM_COMMIT,
                                       PAGE_READWRITE);
      if (NT_SUCCESS(Status))
      {
         memset(EnvPtr,
                0,
                RegionSize);
         *Environment = EnvPtr;
      }
   }

   return(Status);
}


/*
 * @implemented
 */
VOID NTAPI
RtlDestroyEnvironment(PWSTR Environment)
{
   ULONG Size = 0;

   NtFreeVirtualMemory(NtCurrentProcess(),
                       (PVOID)&Environment,
                       &Size,
                       MEM_RELEASE);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlExpandEnvironmentStrings_U(PWSTR Environment,
                              PUNICODE_STRING Source,
                              PUNICODE_STRING Destination,
                              PULONG Length)
{
   UNICODE_STRING Variable;
   UNICODE_STRING Value;
   NTSTATUS ReturnStatus = STATUS_SUCCESS;
   NTSTATUS Status;
   PWSTR SourceBuffer;
   PWSTR DestBuffer;
   PWSTR CopyBuffer;
   PWSTR VariableEnd;
   ULONG SourceLength;
   ULONG DestMax;
   ULONG CopyLength;
   ULONG Tail;
   ULONG TotalLength = 1; /* for terminating NULL */

   DPRINT("RtlExpandEnvironmentStrings_U %p %wZ %p %p\n",
          Environment, Source, Destination, Length);

   SourceLength = Source->Length / sizeof(WCHAR);
   SourceBuffer = Source->Buffer;
   DestMax = Destination->MaximumLength / sizeof(WCHAR);
   DestBuffer = Destination->Buffer;

   while (SourceLength)
   {
      if (*SourceBuffer != L'%')
      {
         CopyBuffer = SourceBuffer;
         CopyLength = 0;
         while (SourceLength != 0 && *SourceBuffer != L'%')
         {
            SourceBuffer++;
            CopyLength++;
            SourceLength--;
         }
      }
      else
      {
         /* Process environment variable. */

         VariableEnd = SourceBuffer + 1;
         Tail = SourceLength - 1;
         while (*VariableEnd != L'%' && Tail != 0)
         {
            VariableEnd++;
            Tail--;
         }

         if (Tail != 0)
         {
            Variable.MaximumLength =
            Variable.Length = (VariableEnd - (SourceBuffer + 1)) * sizeof(WCHAR);
            Variable.Buffer = SourceBuffer + 1;

            Value.Length = 0;
            Value.MaximumLength = DestMax * sizeof(WCHAR);
            Value.Buffer = DestBuffer;

            Status = RtlQueryEnvironmentVariable_U(Environment, &Variable,
                                                   &Value);
            if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_TOO_SMALL)
            {
                SourceBuffer = VariableEnd + 1;
                SourceLength = Tail - 1;
                TotalLength += Value.Length / sizeof(WCHAR);
                if (Status != STATUS_BUFFER_TOO_SMALL)
                {
                   DestBuffer += Value.Length / sizeof(WCHAR);
                   DestMax -= Value.Length / sizeof(WCHAR);
                }
                else
                {
                   DestMax = 0;
                   ReturnStatus = STATUS_BUFFER_TOO_SMALL;
                }
                continue;
            }
            else
            {
               /* Variable not found. */
               CopyBuffer = SourceBuffer;
               CopyLength = SourceLength - Tail + 1;
               SourceLength -= CopyLength;
               SourceBuffer += CopyLength;
            }
         }
         else
         {
            /* Unfinished variable name. */
            CopyBuffer = SourceBuffer;
            CopyLength = SourceLength;
            SourceLength = 0;
         }
      }

      TotalLength += CopyLength;
      if (DestMax)
      {
         if (DestMax < CopyLength)
         {
            CopyLength = DestMax;
            ReturnStatus = STATUS_BUFFER_TOO_SMALL;
         }
         RtlCopyMemory(DestBuffer, CopyBuffer, CopyLength * sizeof(WCHAR));
         DestMax -= CopyLength;
         DestBuffer += CopyLength;
      }
   }

   /* NULL-terminate the buffer. */
   if (DestMax)
      *DestBuffer = 0;
   else
      ReturnStatus = STATUS_BUFFER_TOO_SMALL;

   Destination->Length = (DestBuffer - Destination->Buffer) * sizeof(WCHAR);
   if (Length != NULL)
      *Length = TotalLength * sizeof(WCHAR);

   DPRINT("Destination %wZ\n", Destination);

   return ReturnStatus;
}


/*
 * @implemented
 */
VOID NTAPI
RtlSetCurrentEnvironment(PWSTR NewEnvironment,
                         PWSTR *OldEnvironment)
{
   PVOID EnvPtr;

   DPRINT("NewEnvironment 0x%p OldEnvironment 0x%p\n",
          NewEnvironment, OldEnvironment);

   RtlAcquirePebLock();

   EnvPtr = NtCurrentPeb()->ProcessParameters->Environment;
   NtCurrentPeb()->ProcessParameters->Environment = NewEnvironment;

   if (OldEnvironment != NULL)
      *OldEnvironment = EnvPtr;

   RtlReleasePebLock();
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlSetEnvironmentVariable(PWSTR *Environment,
                          PUNICODE_STRING Name,
                          PUNICODE_STRING Value)
{
   MEMORY_BASIC_INFORMATION mbi;
   UNICODE_STRING var;
   int hole_len, new_len, env_len = 0;
   WCHAR *new_env = 0, *env_end = 0, *wcs, *env, *val = 0, *tail = 0, *hole = 0;
   PWSTR head = NULL;
   ULONG size = 0, new_size;
   LONG f = 1;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("RtlSetEnvironmentVariable(Environment %p Name %wZ Value %wZ)\n",
          Environment, Name, Value);

   /* Variable names can't contain a '=' except as a first character. */
   for (wcs = Name->Buffer + 1;
        wcs < Name->Buffer + (Name->Length / sizeof(WCHAR));
        wcs++)
   {
      if (*wcs == L'=')
          return STATUS_INVALID_PARAMETER;
   }

   if (Environment)
   {
      env = *Environment;
   }
   else
   {
      RtlAcquirePebLock();
      env = NtCurrentPeb()->ProcessParameters->Environment;
   }

   if (env)
   {
      /* get environment length */
      wcs = env_end = env;
      do
      {
         env_end += wcslen(env_end) + 1;
      }
      while (*env_end);
      env_end++;
      env_len = env_end - env;
      DPRINT("environment length %ld characters\n", env_len);

      /* find where to insert */
      while (*wcs)
      {
         var.Buffer = wcs++;
         wcs = wcschr(wcs, L'=');
         if (wcs == NULL)
         {
            wcs = var.Buffer + wcslen(var.Buffer);
         }
         if (*wcs)
         {
            var.Length = (wcs - var.Buffer) * sizeof(WCHAR);
            var.MaximumLength = var.Length;
            val = ++wcs;
            wcs += wcslen(wcs);
            f = RtlCompareUnicodeString(&var, Name, TRUE);
            if (f >= 0)
            {
               if (f) /* Insert before found */
               {
                  hole = tail = var.Buffer;
               }
               else /* Exact match */
               {
                  head = var.Buffer;
                  tail = ++wcs;
                  hole = val;
               }
               goto found;
            }
         }
         wcs++;
      }
      hole = tail = wcs; /* Append to environment */
   }

found:
   if (Value != NULL && Value->Length > 0)
   {
      hole_len = tail - hole;
      /* calculate new environment size */
      new_size = Value->Length + sizeof(WCHAR);
      /* adding new variable */
      if (f)
         new_size += Name->Length + sizeof(WCHAR);
      new_len = new_size / sizeof(WCHAR);
      if (hole_len < new_len)
      {
         /* enlarge environment size */
         /* check the size of available memory */
         new_size += (env_len - hole_len) * sizeof(WCHAR);
         new_size = ROUNDUP(new_size, PAGE_SIZE);
         mbi.RegionSize = 0;
         DPRINT("new_size %lu\n", new_size);

         if (env)
         {
            Status = NtQueryVirtualMemory(NtCurrentProcess(),
                                          env,
                                          MemoryBasicInformation,
                                          &mbi,
                                          sizeof(MEMORY_BASIC_INFORMATION),
                                          NULL);
            if (!NT_SUCCESS(Status))
            {
               if (Environment == NULL)
               {
                  RtlReleasePebLock();
               }
               return(Status);
            }
         }

         if (new_size > mbi.RegionSize)
         {
            /* reallocate memory area */
            Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                             (PVOID)&new_env,
                                             0,
                                             &new_size,
                                             MEM_RESERVE | MEM_COMMIT,
                                             PAGE_READWRITE);
            if (!NT_SUCCESS(Status))
            {
               if (Environment == NULL)
               {
                  RtlReleasePebLock();
               }
               return(Status);
            }

            if (env)
            {
               memmove(new_env,
                       env,
                       (hole - env) * sizeof(WCHAR));
               hole = new_env + (hole - env);
            }
            else
            {
               /* absolutely new environment */
               tail = hole = new_env;
               *hole = 0;
               env_end = hole + 1;
            }
         }
      }

      /* move tail */
      memmove (hole + new_len, tail, (env_end - tail) * sizeof(WCHAR));

      if (new_env)
      {
         /* we reallocated environment, let's free the old one */
         if (Environment)
            *Environment = new_env;
         else
            NtCurrentPeb()->ProcessParameters->Environment = new_env;

         if (env)
         {
            size = 0;
            NtFreeVirtualMemory(NtCurrentProcess(),
                                (PVOID)&env,
                                &size,
                                MEM_RELEASE);
         }
      }

      /* and now copy given stuff */
      if (f)
      {
         /* copy variable name and '=' character */
         memmove(hole,
                 Name->Buffer,
                 Name->Length);
         hole += Name->Length / sizeof(WCHAR);
         *hole++ = L'=';
      }

      /* copy value */
      memmove(hole,
              Value->Buffer,
              Value->Length);
      hole += Value->Length / sizeof(WCHAR);
      *hole = 0;
   }
   else
   {
      /* remove the environment variable */
      if (f == 0)
      {
         memmove(head,
                 tail,
                 (env_end - tail) * sizeof(WCHAR));
      }
      else
      {
         Status = STATUS_VARIABLE_NOT_FOUND;
      }
   }

   if (Environment == NULL)
   {
      RtlReleasePebLock();
   }

   return(Status);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlQueryEnvironmentVariable_U(PWSTR Environment,
                              PUNICODE_STRING Name,
                              PUNICODE_STRING Value)
{
   NTSTATUS Status;
   PWSTR wcs;
   UNICODE_STRING var;
   PWSTR val;
   BOOLEAN SysEnvUsed = FALSE;

   DPRINT("RtlQueryEnvironmentVariable_U Environment %p Variable %wZ Value %p\n",
          Environment, Name, Value);

   if (Environment == NULL)
   {
      PPEB Peb = RtlpCurrentPeb();
      if (Peb) {
          Environment = Peb->ProcessParameters->Environment;
          SysEnvUsed = TRUE;
      }
   }

   if (Environment == NULL)
   {
      return(STATUS_VARIABLE_NOT_FOUND);
   }

   Value->Length = 0;
   if (SysEnvUsed == TRUE)
      RtlAcquirePebLock();

   wcs = Environment;
   DPRINT("Starting search at :%p\n", wcs);
   while (*wcs)
   {
      var.Buffer = wcs++;
      wcs = wcschr(wcs, L'=');
      if (wcs == NULL)
      {
         wcs = var.Buffer + wcslen(var.Buffer);
         DPRINT("Search at :%S\n", wcs);
      }
      if (*wcs)
      {
         var.Length = var.MaximumLength = (wcs - var.Buffer) * sizeof(WCHAR);
         val = ++wcs;
         wcs += wcslen(wcs);
         DPRINT("Search at :%S\n", wcs);

         if (RtlEqualUnicodeString(&var, Name, TRUE))
         {
            Value->Length = (wcs - val) * sizeof(WCHAR);
            if (Value->Length <= Value->MaximumLength)
            {
               memcpy(Value->Buffer, val,
                      min(Value->Length + sizeof(WCHAR), Value->MaximumLength));
               DPRINT("Value %S\n", val);
               DPRINT("Return STATUS_SUCCESS\n");
               Status = STATUS_SUCCESS;
            }
            else
            {
               DPRINT("Return STATUS_BUFFER_TOO_SMALL\n");
               Status = STATUS_BUFFER_TOO_SMALL;
            }

            if (SysEnvUsed == TRUE)
               RtlReleasePebLock();

            return(Status);
         }
      }
      wcs++;
   }

   if (SysEnvUsed == TRUE)
      RtlReleasePebLock();

   DPRINT("Return STATUS_VARIABLE_NOT_FOUND: %wZ\n", Name);
   return(STATUS_VARIABLE_NOT_FOUND);
}

/* EOF */
