cmake_minimum_required(VERSION 3.24)
project(VNMTweaks VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(VNM_BUILD_GUI "Build the Qt desktop application" ON)
option(VNM_BUILD_TESTS "Build the unit test binary" ON)

function(vnm_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /Zc:__cplusplus)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

# ---------------------------------------------------------------------------
# vnm_core: pure C++20. No Qt, no Win32. Everything here is unit tested.
# ---------------------------------------------------------------------------
add_library(vnm_core STATIC
  src/core/json_lite.cpp
  src/core/journal.cpp
  src/core/catalog.cpp
  src/core/gate.cpp
  src/core/plan.cpp
  src/core/paths.cpp
  src/core/gamemode.cpp
)
target_include_directories(vnm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(vnm_core PUBLIC VNM_VERSION="${PROJECT_VERSION}")
vnm_warnings(vnm_core)
if(NOT MSVC)
  target_compile_options(vnm_core PRIVATE -Werror)
endif()

# ---------------------------------------------------------------------------
# vnm_platform: the only place that talks to the operating system.
# On non-Windows hosts the sources compile to stubs so the tree still builds.
# ---------------------------------------------------------------------------
add_library(vnm_platform STATIC
  src/platform/win/registry_win.cpp
  src/platform/win/restore_win.cpp
)
target_link_libraries(vnm_platform PUBLIC vnm_core)
vnm_warnings(vnm_platform)
if(WIN32)
  target_link_libraries(vnm_platform PUBLIC advapi32 shell32 ole32)
  target_compile_definitions(vnm_platform PUBLIC UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# ---------------------------------------------------------------------------
# VNMHelper: the only process allowed to change system state.
# ---------------------------------------------------------------------------
add_executable(VNMHelper src/helper/main.cpp)
target_link_libraries(VNMHelper PRIVATE vnm_platform)
set_target_properties(VNMHelper PROPERTIES OUTPUT_NAME "VNMHelper")
vnm_warnings(VNMHelper)
target_compile_definitions(VNMHelper PRIVATE VNM_VERSION="${PROJECT_VERSION}")

# ---------------------------------------------------------------------------
# VNMTweaks: Qt 6 Widgets front end.
# ---------------------------------------------------------------------------
if(VNM_BUILD_GUI)
  find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)
  qt_standard_project_setup()

  qt_add_executable(VNMTweaks
    src/app/main.cpp
    src/app/main_window.cpp
    src/app/pages.cpp
    src/app/helper_client.cpp
    src/app/theme.cpp
  )
  target_link_libraries(VNMTweaks PRIVATE Qt6::Widgets vnm_platform)
  target_compile_definitions(VNMTweaks PRIVATE VNM_VERSION="${PROJECT_VERSION}")
  set_target_properties(VNMTweaks PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(VNM_BUILD_TESTS)
  enable_testing()
  add_executable(vnm_tests
    tests/test_main.cpp
    tests/test_json.cpp
    tests/test_journal.cpp
    tests/test_gamemode.cpp
    tests/test_gate.cpp
    tests/test_plan.cpp
  )
  target_link_libraries(vnm_tests PRIVATE vnm_core)
  target_include_directories(vnm_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  vnm_warnings(vnm_tests)
  add_test(NAME vnm_unit COMMAND vnm_tests)
endif()
