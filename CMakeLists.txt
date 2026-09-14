# --- CORRECTED WARNING SUPPRESSION STEP ---
# Inject flags using the mandatory -clang: prefix required by clang-cl.exe on Windows
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (MSVC OR CMAKE_CXX_SIMULATE_ID MATCHES "MSVC")
        # Handles clang-cl on Windows CI runner
        target_compile_options(SharedCode INTERFACE -clang:-Wno-error=nan-infinity-disabled -clang:-Wno-nan-infinity-disabled)
        target_compile_options("${PROJECT_NAME}" PRIVATE -clang:-Wno-error=nan-infinity-disabled -clang:-Wno-nan-infinity-disabled)
    else()
        # Fallback for standard macOS/Linux Clang runners
        target_compile_options(SharedCode INTERFACE -Wno-error=nan-infinity-disabled -Wno-nan-infinity-disabled)
        target_compile_options("${PROJECT_NAME}" PRIVATE -Wno-error=nan-infinity-disabled -Wno-nan-infinity-disabled)
    endif()
endif()
