
list(APPEND UCRT_INTERNAL_SOURCES
    internal/CreateProcessA.cpp
    internal/GetModuleFileNameA.cpp
    internal/initialization.cpp
    internal/LoadLibraryExA.cpp
    internal/locks.cpp
    internal/OutputDebugStringA.cpp
    internal/peb_access.cpp
    internal/per_thread_data.cpp
    internal/report_runtime_error.cpp
    internal/SetCurrentDirectoryA.cpp
    internal/SetEnvironmentVariableA.cpp
    internal/shared_initialization.cpp
    internal/winapi_thunks.cpp
    internal/win_policies.cpp
)
