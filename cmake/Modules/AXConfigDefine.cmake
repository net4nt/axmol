include(${_AX_ROOT}/1k/platforms.cmake)

# custom target property for lua/js link
define_property(TARGET
  PROPERTY AX_LUA_DEPEND
  BRIEF_DOCS "axmol lua depend libs"
  FULL_DOCS "use to save depend libs of axmol lua project"
)

# UWP min deploy target support, VS property: targetPlatformMinVersion
if(WINRT)
  # The minmal deploy target version: Windows 10, version 1809 (Build 10.0.17763) for building msix package
  # refer to: https://learn.microsoft.com/en-us/windows/msix/supported-platforms?source=recommendations
  set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION "10.0.17763" CACHE STRING "")
  set(AX_CPPWINRT_VERSION "2.0.250303.1" CACHE STRING "")

  # For axmol deprecated policy, we need disable /sdl checks explicitly to avoid compiler traits invoking deprecated functions as error
  set(CMAKE_C_FLAGS "/sdl- ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "/sdl- ${CMAKE_CXX_FLAGS}")
endif()

if(ANDROID OR LINUX)
  set(CMAKE_C_FLAGS "-fPIC ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "-fPIC ${CMAKE_CXX_FLAGS}")
endif()

# config c standard
if(WINDOWS)
  message(STATUS "CMAKE_HOST_SYSTEM_VERSION: ${CMAKE_HOST_SYSTEM_VERSION}")
  message(STATUS "CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")
  message(STATUS "CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION: ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")

  if(DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION)
    message(STATUS "CMAKE_VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION: ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION}")
  endif()

  if(NOT CMAKE_SYSTEM_VERSION)
    set(CMAKE_SYSTEM_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
  endif()

  # Fix win32 llvm-clang
  if(NOT CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION ${CMAKE_SYSTEM_VERSION})
  endif()

  # CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION aka selected windows sdk version
  if(${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} VERSION_GREATER_EQUAL "10.0.22000.0")
    set(CMAKE_C_STANDARD 11)
  else()
    # windows sdk < 10.0.22000.0, The c11 header stdalign.h was missing, so workaroud fallback C standard to 99
    # refer to:
    # - https://github.com/axmolengine/axmol/issues/991
    # - https://github.com/axmolengine/axmol/issues/1246
    message(AUTHOR_WARNING "Forcing set CMAKE_C_STANDARD to 99 when winsdk < 10.0.22000.0")
    set(CMAKE_C_STANDARD 99)
  endif()
else()
  if(NOT DEFINED CMAKE_C_STANDARD)
    set(CMAKE_C_STANDARD 11)
  endif()
endif()

message(STATUS "CMAKE_C_STANDARD=${CMAKE_C_STANDARD}")

if(NOT DEFINED CMAKE_C_STANDARD_REQUIRED)
  set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

# config c++ standard, minimal require c++23
set(_AX_MIN_CXX_STD 23)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD ${_AX_MIN_CXX_STD})
endif()

if(CMAKE_CXX_STANDARD GREATER_EQUAL ${_AX_MIN_CXX_STD})
  message(STATUS "Building axmol with c++${CMAKE_CXX_STANDARD}")
else()
  message(STATUS "Building axmol require c++ std >= ${_AX_MIN_CXX_STD}")
endif()

# used to set 3rdparty c++ standard same with axmol
set(_AX_CXX_STD ${CMAKE_CXX_STANDARD} CACHE STRING "" FORCE)

if(NOT DEFINED CMAKE_CXX_STANDARD_REQUIRED)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(NOT DEFINED CMAKE_CXX_EXTENSIONS)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# Axmol not use c++20 modules, so disable cmake scan for modules
# benefits:
# 1. speed up build time
# 2. fix wasm build fail on windows host due to emsdk has bugs not trim some
# emsdk spec flags, i.e. -sUSE_LIBJPEG -msse2, -mss4.1 ...
# remark: The feature scan for modules was added in cmake 3.28
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# check compiler on windows
if(WINDOWS)
  # not support other compile tools except MSVC for now
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Using Windows clang generate axmol project, CLANG_VERSION: ${CLANG_VERSION_STRING}")
    set(FUZZ_CLANG TRUE) # clang-cl or clang++

    if(NOT MSVC)
      set(FULL_CLANG TRUE) # clang++
    else()
      set(FUZZ_MSVC TRUE) # clang-cl
    endif()
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # Visual Studio 2015, MSVC_VERSION 1900      (v140 toolset)
    # Visual Studio 2017, MSVC_VERSION 1910-1919 (v141 toolset)
    set(FUZZ_MSVC TRUE)
    set(FULL_MSVC TRUE)

    if(${MSVC_VERSION} EQUAL 1900 OR ${MSVC_VERSION} GREATER 1900)
      message(STATUS "Using Windows MSVC generate axmol project, MSVC_VERSION:${MSVC_VERSION}")
    else()
      message(FATAL_ERROR "Using Windows MSVC generate axmol project, MSVC_VERSION:${MSVC_VERSION} lower than needed")
    endif()
  else()
    message(FATAL_ERROR "Please using Windows MSVC/LLVM-Clang compile axmol project")
  endif()
endif()

if(EMSCRIPTEN_VERSION)
  message(STATUS "Using emsdk generate axmol project, EMSCRIPTEN_VERSION: ${EMSCRIPTEN_VERSION}")
endif()

set(_ax_compile_options)

if(FUZZ_MSVC)
  list(APPEND _ax_compile_options /GF)
  set(CMAKE_CXX_FLAGS "/Zc:char8_t- ${CMAKE_CXX_FLAGS}")
else() # others
  set(CMAKE_CXX_FLAGS "-fno-char8_t ${CMAKE_CXX_FLAGS}")
endif()

if(APPLE)
  add_compile_options("$<$<COMPILE_LANGUAGE:OBJC>:-Werror=objc-method-access>")
  add_compile_options("$<$<COMPILE_LANGUAGE:OBJCXX>:-Werror=objc-method-access>")
endif()

set(CMAKE_DEBUG_POSTFIX "" CACHE STRING "Library postfix for debug builds. Normally left blank." FORCE)
set(CMAKE_PLATFORM_NO_VERSIONED_SONAME TRUE CACHE BOOL "Disable dynamic libraries symblink." FORCE)

if(ANDROID)
  # Ensure fseeko available on ndk > 23
  math(EXPR _ARCH_BITS "${CMAKE_SIZEOF_VOID_P} * 8")
  add_definitions(-D_FILE_OFFSET_BITS=${_ARCH_BITS})

  # set hash style to both for android old device compatible
  # see also: https://github.com/axmolengine/axmol/discussions/614
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--hash-style=both")
endif()

# Set macro definitions for special platforms
function(use_ax_compile_define target)
  target_compile_definitions(${target} PUBLIC $<$<CONFIG:Debug>:_AX_DEBUG=1>)

  # !important axmol not use double precision
  # target_compile_definitions(${target} PUBLIC CP_USE_CGTYPES=0)
  # target_compile_definitions(${target} PUBLIC CP_USE_DOUBLES=0)
  if(LINUX)
    ax_config_pred(${target} AX_ENABLE_VLC_MEDIA)
    target_compile_definitions(${target} PUBLIC _GNU_SOURCE)
  elseif(WINDOWS)
    ax_config_pred(${target} AX_ENABLE_VLC_MEDIA)
    target_compile_definitions(${target}
      PUBLIC WIN32
      PUBLIC _WIN32
      PUBLIC _WINDOWS
      PUBLIC UNICODE
      PUBLIC _UNICODE
      PUBLIC _CRT_SECURE_NO_WARNINGS
      PUBLIC _SCL_SECURE_NO_WARNINGS
      # PUBLIC GLAD_GLAPI_EXPORT
    )

    if(BUILD_SHARED_LIBS)
      target_compile_definitions(${target} PRIVATE AX_DLLEXPORT INTERFACE AX_DLLIMPORT)
    endif()
  endif()

  # render api
  if(AX_RENDER_API STREQUAL "gl")
    target_compile_definitions(${target} PUBLIC AX_RENDER_API=1)
    if(APPLE)
      target_compile_definitions(${target} PUBLIC GL_SILENCE_DEPRECATION=1)
    endif()
    target_compile_definitions(${target} PUBLIC AX_GLES_PROFILE=${AX_GLES_PROFILE})
  elseif(AX_RENDER_API STREQUAL "mtl")
    target_compile_definitions(${target} PUBLIC AX_RENDER_API=2)
  elseif(AX_RENDER_API STREQUAL "d3d")
    target_compile_definitions(${target} PUBLIC AX_RENDER_API=3)
  endif()
endfunction()

# Set compiler options for engine lib: axmol
function(use_ax_compile_options target)
  if(FULL_MSVC)
    # Enable msvc multi-process building
    target_compile_options(${target} PUBLIC /MP)
  endif()

  if(WASM)
    # refer to: https://github.com/emscripten-core/emscripten/blob/main/src/settings.js
    target_link_options(${target} PUBLIC -sFORCE_FILESYSTEM=1 -sFETCH=1 -sUSE_GLFW=3)
  elseif(LINUX)
    target_link_options(${target} PUBLIC "-lpthread")
  endif()
endfunction()

if(EMSCRIPTEN)
  # Tell emcc build port libjpeg in cache and add link flag manually to
  # fix build fail on windows host when cmake invoking emscan-deps (raise unknown options)
  set(_AX_EM_C_FLAGS "-sUSE_LIBJPEG=1")
  set(_AX_EM_LD_FLAGS -ljpeg)

  set(AX_WASM_THREADS "4" CACHE STRING "Wasm threads count")
  set(_threads_hint "")

  if(AX_WASM_THREADS STREQUAL "auto") # not empty string or not 0
    # Enable pthread support globally
    set(_threads_hint "(auto)")
    include(ProcessorCount)
    set(_AX_WASM_THREADS_INT 0)
    ProcessorCount(_AX_WASM_THREADS_INT)
    set(AX_WASM_THREADS "${_AX_WASM_THREADS_INT}" CACHE STRING "Wasm threads count" FORCE)
  endif()

  message(STATUS "AX_WASM_THREADS=${AX_WASM_THREADS}${_threads_hint}")

  if(AX_WASM_THREADS MATCHES "^([0-9]+)$" OR AX_WASM_THREADS STREQUAL "navigator.hardwareConcurrency")
    list(APPEND _ax_compile_options -pthread)
    list(APPEND _AX_EM_LD_FLAGS -pthread -sPTHREAD_POOL_SIZE=${AX_WASM_THREADS})
  endif()

  set(AX_WASM_INITIAL_MEMORY "128MB" CACHE STRING "")
  list(APPEND _AX_EM_LD_FLAGS -sINITIAL_MEMORY=${AX_WASM_INITIAL_MEMORY} -sALLOW_MEMORY_GROWTH=1)

  # list(APPEND _AX_EM_LD_FLAGS -sALLOW_MEMORY_GROWTH=1)

  # apply emcc c flags & link flags
  string(APPEND CMAKE_C_FLAGS " ${_AX_EM_C_FLAGS}")
  add_link_options(${_AX_EM_LD_FLAGS})
endif()

# apply axmol spec compile options
add_compile_options(${_ax_compile_options})

if(APPLE)
  enable_language(C CXX OBJC OBJCXX)
else()
  enable_language(C CXX)
endif()

# Try enable asm & nasm compiler support
set(can_use_assembler TRUE)
enable_language(ASM)
enable_language(ASM_NASM OPTIONAL)

if(NOT EXISTS "${CMAKE_ASM_NASM_COMPILER}")
  set(CMAKE_ASM_NASM_COMPILER_LOADED FALSE CACHE BOOL "Does cmake asm nasm compiler loaded" FORCE)
  message(AUTHOR_WARNING "The nasm compiler doesn't present on your system PATH, please download from: https://www.nasm.us/pub/nasm/releasebuilds/2.16.01/")
endif()

# we don't need cmake BUILD_TESTING feature
set(BUILD_TESTING FALSE CACHE BOOL "" FORCE)
