cmake_minimum_required(VERSION 3.25)

# This tells cmake we have goodies in the /cmake folder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include (PamplejuceVersion)

# Modern concise way to add dependencies to your project
include (CPM)

# Configures universal binaries and decides which version of macOS to support
include(PamplejuceMacOS)

# Couple tweaks that IMO should be JUCE defaults
include(JUCEDefaults)

# Change me!
# This is the internal name of the project and the name of JUCE's shared code target
set(PROJECT_NAME "Pyscleeffect")

# Worry not, JUCE's PRODUCT_NAME can have spaces (and is what DAWs will display)
set(PRODUCT_NAME "Pyscle adapter")

# Change me! Used for the MacOS bundle name and Installers
set(COMPANY_NAME "Dexterus")

# Change me! Used for the MacOS bundle identifier (and signing)
set(BUNDLE_ID "com.pamplejuce.pamplejuce")

# Change me! Set the plugin formats you want built
set(FORMATS Standalone Unity VST3)

# Force JUCE to work with sccache
set(CMAKE_POLICY_DEFAULT_CMP0141        NEW      CACHE STRING "" FORCE)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded CACHE STRING "" FORCE)

# For simplicity, the name of the CMake project is also the name of the target
project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

# ==============================================================================
# --- FORCE REPLACEMENT OF FAST MATH OPTIMIZATION FLOATS ---
# Strips the /fp:fast and fast-math optimization flags that clash with JUCE frameworks
# ==============================================================================
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Tell CMake globally to drop fast-math warning halts across all targets
    add_compile_options(-Wno-error=nan-infinity-disabled -Wno-nan-infinity-disabled)
    
    # Overwrite environment strings to force strict floating precision
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=nan-infinity-disabled -Wno-nan-infinity-disabled")
    
    # Force override of the MSVC wrapper floating point type if present
    if (MSVC OR CMAKE_CXX_SIMULATE_ID MATCHES "MSVC")
        string(REPLACE "/fp:fast" "/fp:precise" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        add_compile_options(-clang:-Wno-error=nan-infinity-disabled -clang:-Wno-nan-infinity-disabled)
    endif()
endif()
# ==============================================================================

# Log build environment info (after project() so compiler info is available)
include(PamplejuceLog)

# Defines the pamplejuce_* functions we call below (also fetches Catch2 at include time)
include(PamplejuceFunctions)

# JUCE is setup as a submodule in the /JUCE folder
add_subdirectory(JUCE)

# Add CLAP format
add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)

# See `docs/CMake API.md` in the JUCE repo for all config options
juce_add_plugin("${PROJECT_NAME}"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon.png"
    COMPANY_NAME "Dexter_US"
    BUNDLE_ID "${BUNDLE_ID}"
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Pamp
    PLUGIN_CODE P001
    FORMATS "${FORMATS}"
    PRODUCT_NAME "Dexter U.S. Psycle adapter"
)

# This lets us use our code in both the JUCE targets and our Test target
add_library(SharedCode INTERFACE)

clap_juce_extensions_plugin(TARGET "${PROJECT_NAME}"
    CLAP_ID "${BUNDLE_ID}"
    CLAP_FEATURES audio-effect)

# Enable fast math, C++23 and a few other target defaults
pamplejuce_shared_code_defaults()

# Override fast math specifically for our targets right after Pamplejuce appends them
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (MSVC OR CMAKE_CXX_SIMULATE_ID MATCHES "MSVC")
        target_compile_options(SharedCode INTERFACE "-clang:-Wno-error=nan-infinity-disabled" "-clang:-Wno-nan-infinity-disabled")
        target_compile_options("${PROJECT_NAME}" PRIVATE "-clang:-Wno-error=nan-infinity-disabled" "-clang:-Wno-nan-infinity-disabled")
    else()
        target_compile_options(SharedCode INTERFACE "-Wno-error=nan-infinity-disabled" "-Wno-nan-infinity-disabled")
        target_compile_options("${PROJECT_NAME}" PRIVATE "-Wno-error=nan-infinity-disabled" "-Wno-nan-infinity-disabled")
    endif()
endif()

# Manually list all .h and .cpp files for the plugin
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})

# Adds a BinaryData target for embedding assets into the binary
pamplejuce_add_assets()

# MacOS only: Cleans up folder and target organization on Xcode.
pamplejuce_xcode_prettify()

# This is where you can set preprocessor definitions for JUCE and your plugin
target_compile_definitions(SharedCode
    INTERFACE
    JUCE_WEB_BROWSER=0  
    JUCE_USE_CURL=0     
    JUCE_VST3_CAN_REPLACE_VST2=0
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${CURRENT_VERSION}"
    PRODUCT_NAME_WITHOUT_VERSION="Pamplejuce"
)

target_link_libraries(SharedCode
    INTERFACE
    Assets
    juce_audio_utils
    juce_audio_processors
    juce_dsp
    juce_gui_basics
    juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Link the JUCE plugin targets our SharedCode target
target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)

# IPP support, comment out to disable
include(PamplejuceIPP)

# Everything related to the tests target
pamplejuce_add_tests()

# A separate target for Benchmarks (keeps the Tests target fast)
pamplejuce_add_benchmarks()

# Output some config for CI (like our PRODUCT_NAME)
include(GitHubENV)
