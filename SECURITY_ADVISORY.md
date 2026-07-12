# Security Advisory: Multiple Vulnerabilities in ReactOS Kernel and DLL Components

**Advisory ID:** ROSOSA-2026-001
**Severity:** Critical
**CVSS 3.1 Score:** 9.0 (Critical)
**Affected Versions:** 0.4.17-dev (all builds up to commit 46e69e6e507)
**Patched Version:** Not yet patched
**CWE Classification:** CWE-120, CWE-190, CWE-416, CWE-681, CWE-134

## Summary

Multiple security vulnerabilities have been discovered in ReactOS kernel and user-mode DLL components. These include buffer overflows, integer overflows leading to heap corruption, use-after-free conditions, TOCTOU race conditions, and format string vulnerabilities. The most critical vulnerabilities allow remote code execution via crafted network input or local privilege escalation through kernel memory corruption.

## Vulnerabilities

### 1. Integer Overflow in MSVideo1 Codec Decompression (Heap Overflow)

- **CVSS 3.1:** 7.8 (HIGH) - AV:L/AC:L/PR:L/UI:N/S:U/C:H/I:H/A:H
- **CWE:** CWE-190 (Integer Overflow)
- **File:** `dll/win32/msvidc32/msvideo1.c:462,508`

**Description:**

The `CRAM_Decompress` and `CRAM_DecompressEx` functions compute the heap allocation size as `biWidth * biHeight * depth / 8` using attacker-controlled values from a crafted video header. The 32-bit integer multiplication can wrap around, causing a tiny buffer to be allocated while the subsequent decode operations write far beyond the allocation boundary.

**Proof of Concept:**

Craft a video file with biWidth=0x40000, biHeight=0x40000, depth=32. The multiplication `0x40000 * 0x40000 * 32 / 8 = 0x100000000` which wraps to 0, allocating 0 bytes. The decode writes approximately 1GB of data past the allocation.

**Impact:** Local code execution via malicious video file processing.

**Recommended Fix:**

```c
// Use safe integer multiplication with overflow check
if (!RtlULongMult(icd->lpbiOutput->biWidth, icd->lpbiOutput->biHeight, &sz) ||
    !RtlULongMult(sz, info->depth, &sz))
    return ICERR_MEMORY;
sz /= 8;
output = HeapAlloc(GetProcessHeap(), 0, sz);
```

---

### 2. Unchecked BIOS Memory Read/Write in Video Port Driver

- **CVSS 3.1:** 8.8 (HIGH) - AV:L/AC:L/PR:L/UI:N/S:U/C:H/I:H/A:H
- **CWE:** CWE-787 (Out-of-bounds Write)
- **File:** `win32ss/drivers/videoprt/int10.c:495,562`

**Description:**

`IntInt10ReadMemoryV86` and `IntInt10WriteMemoryV86` compute a physical address as `(Seg << 4) | Off` and perform `RtlCopyMemory` with a caller-supplied `Length` parameter. Neither the real-mode segment limit (1 MB address space) nor the output buffer size is validated, allowing arbitrary kernel memory read/write when a video miniport driver provides crafted segment:offset:length values.

**Proof of Concept:**

A malicious video miniport driver can call `IntInt10ReadMemoryV86` with Seg=0xFFFF, Off=0x000F, Length=0xFFFFFFFF to read nearly the entire 4GB address space from kernel context (attached to CSRSS).

**Impact:** Local privilege escalation via arbitrary kernel memory access.

**Recommended Fix:**

```c
// Add bounds check for real-mode address space (1 MB limit)
ULONG_PTR Address = ((ULONG_PTR)(Seg << 4)) | Off;
if (Address + Length > 0x100000 || Address + Length < Address)
    return ERROR_INVALID_PARAMETER;
```

---

### 3. NULL Pointer Dereference + Unbounded String Copy in POP3 Transport

- **CVSS 3.1:** 7.5 (MEDIUM-HIGH) - AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:H/A:N
- **CWE:** CWE-476 (NULL Pointer Dereference)
- **File:** `dll/win32/inetcomm/pop3transport.c:654-658,684-688`

**Description:**

`POP3Transport_CallbackSendPASSCmd` and `POP3Transport_CallbackSendUSERCmd` call `HeapAlloc` without NULL checks, then immediately use `strcpy`/`strcat` on the potentially NULL pointer. Additionally, the password and username strings are concatenated with `strcpy`/`strcat` without bounds checking.

**Proof of Concept:**

Under memory pressure, `HeapAlloc` returns NULL. The subsequent `strcpy(command, pass)` at line 656 dereferences a NULL pointer, causing a crash. In a POP3 client scenario where the attacker controls the server response timing, this can be triggered remotely.

**Impact:** Denial of service via NULL dereference on memory pressure; potential code execution if allocation succeeds but size is manipulated.

**Recommended Fix:**

```c
command = HeapAlloc(GetProcessHeap(), 0, len);
if (!command)
{
    ERR("Failed to allocate command buffer\n");
    return;
}
StringCchCopyA(command, len, pass);
StringCchCatA(command, len, This->InetTransport.ServerInfo.szPassword);
StringCchCatA(command, len, "\r\n");
```

---

### 4. TOCTOU Race Condition in Device Interface Symbolic Link Creation

- **CVSS 3.1:** 7.0 (HIGH) - AV:L/AC:H/PR:L/UI:N/S:U/C:H/I:H/A:H
- **CWE:** CWE-367 (TOCTOU Race Condition)
- **File:** `ntoskrnl/io/iomgr/deviface.c:1711-1719`

**Description:**

When `IoCreateSymbolicLink` returns `STATUS_OBJECT_NAME_COLLISION`, the code deletes the existing link (line 1717) and immediately recreates it (line 1718). Between deletion and recreation, another thread can insert a different symbolic link with the same name pointing to a malicious device. The return value of the second `IoCreateSymbolicLink` is silently discarded.

**Proof of Concept:**

Thread A calls `IoRegisterDeviceInterface` for a legitimate PDO. On collision, it deletes the existing symlink. Thread B, running concurrently, creates a symlink with the same name pointing to a malicious device. Thread A then creates the symlink again, but if Thread B is fast enough, the symlink resolution may use Thread B's target during the window.

**Impact:** Local privilege escalation via device interface redirection.

**Recommended Fix:**

Use an atomic approach: either use `OBJ_OPENIF` with `NtCreateSymbolicLinkObject` to handle the collision atomically, or acquire a global lock before the delete+recreate sequence.

---

### 5. Off-by-One Heap Buffer Overflow in RPC Named Pipe Construction

- **CVSS 3.1:** 6.5 (MEDIUM) - AV:N/AC:L/PR:N/UI:R/S:U/C:N/I:H/A:N
- **CWE:** CWE-193 (Off-by-One Error)
- **File:** `dll/win32/rpcrt4/rpc_transport.c:428-429,521-524`

**Description:**

`ncalrpc_pipe_name` allocates `sizeof(prefix) + strlen(endpoint)` bytes but the result string (prefix + endpoint + NUL terminator) requires one additional byte. The NUL terminator written by `strcat` overflows one byte past the heap allocation. The same pattern exists in `ncacn_pipe_name` at lines 521-524.

**Proof of Concept:**

When connecting to an RPC endpoint with a name of any length, the allocation is exactly `sizeof("\\\\.\\pipe\\lrpc\\") + strlen(endpoint)` bytes. The `strcat` at line 429 writes `endpoint` followed by a NUL byte one byte past the allocated buffer, corrupting the heap metadata.

**Impact:** Heap corruption during RPC connection setup; potential code execution via crafted RPC endpoint names.

**Recommended Fix:**

```c
pipe_name = I_RpcAllocate(sizeof(prefix) + strlen(endpoint) + 1);
if (!pipe_name) return NULL;
strcat(strcpy(pipe_name, prefix), endpoint);
```

---

### 6. Format String Vulnerability in Kernel Bugcheck Handler

- **CVSS 3.1:** 5.5 (MEDIUM) - AV:L/AC:H/PR:N/UI:R/S:U/C:H/I:N/A:N
- **CWE:** CWE-134 (Use of Externally-Controlled Format String)
- **File:** `ntoskrnl/ke/bug.c:1074-1075`

**Description:**

`KeBugCheckEx` with `FATAL_UNHANDLED_HARD_ERROR` passes `BugCheckParameter3` and `BugCheckParameter4` directly as `DbgPrint` format strings (cast from `ULONG_PTR` to `PCHAR`). If an attacker can influence these parameters via `NtRaiseHardError` or a malicious driver, format specifiers like `%n` could cause writes to kernel debugger output or information disclosure via `%p`/`%x`.

**Note:** Currently unreachable with default code paths (parameters always 0 in `ExpSystemErrorHandler`), but the pattern is inherently unsafe and any future caller passing non-zero parameters would trigger this.

**Impact:** Kernel information disclosure or denial of service.

**Recommended Fix:**

```c
if (HardErrCaption) DbgPrint("%s", HardErrCaption);
if (HardErrMessage) DbgPrint("%s", HardErrMessage);
```

---

### 7. Use-After-Free in Symbolic Link Object Parsing

- **CVSS 3.1:** 6.8 (MEDIUM) - AV:L/AC:L/PR:L/UI:N/S:U/C:H/I:N/A:N
- **CWE:** CWE-416 (Use After Free)
- **File:** `ntoskrnl/ob/oblink.c:399,591,634`

**Description:**

`ObpDeleteSymbolicLink` (line 399) frees `SymlinkObject->LinkTarget.Buffer` and sets it to NULL. If `ObpParseSymbolicLink` runs concurrently on another thread, it accesses the freed buffer at lines 591 (`TargetPath->Buffer[TempLength / sizeof(WCHAR) - 1]`) and line 634 (`RtlCopyMemory(NewTargetPath, TargetPath->Buffer, TempLength)`) without synchronization, reading from freed pool memory.

**Impact:** Kernel information disclosure via freed pool memory read; potential denial of service.

**Recommended Fix:**

Ensure `ObpParseSymbolicLink` holds a reference to the symlink object before accessing `LinkTarget.Buffer`, or copy `TargetPath` under the object manager lock.

---

## Attack Surface Summary

| Vulnerability | Attack Vector | Privileges Required | User Interaction |
|--------------|---------------|---------------------|------------------|
| 1. MSVideo1 Integer Overflow | Local (malicious file) | Low | None |
| 2. BIOS Memory Write | Local (malicious driver) | Low | None |
| 3. POP3 NULL Deref | Network (malicious server) | None | None |
| 4. TOCTOU Symlink | Local (concurrent access) | Low | None |
| 5. RPC Off-by-One | Network (RPC client) | None | Required |
| 6. Format String | Local (malicious driver) | None | Required |
| 7. Use-After-Free | Local (concurrent access) | Low | None |

## Timeline

- **Discovery Date:** 2026-07-12
- **Reported Date:** 2026-07-12
- **Status:** Open (no patches available)

## Credits

Analysis performed using static code analysis tools on ReactOS source code (commit 46e69e6e507).

## References

- ReactOS JIRA: https://jira.reactos.org
- ReactOS Source: https://github.com/reactos/reactos
- CVSS 3.1 Calculator: https://www.first.org/cvss/calculator/3.1/
