cmake_minimum_required(VERSION 3.14)

option(EINU_ENGINE_BUILD_TESTS "Build all of EINU Engine's own tests." OFF)

option(EINU_ENGINE_BUILD_EXAMPLES "Build all of EINU Engine's own examples."
       OFF)

option(EINU_ENGINE_PROFILE "Profile EINU Engine." OFF)

project(einu-engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0077 NEW)

if(MSVC)
  add_compile_options(
    /wd4005 # macro-redefinition
    /wd4068 # unknown pragma
    /wd4244 # conversion from 'type1' to 'type2'
    /wd4267 # conversion from 'size_t' to 'type2'
    /wd4800 # force value to bool 'true' or 'false'
    /wd4996 # deprecated allocator
  )
  add_definitions(/DNOMINMAX /DWIN32_LEAN_AND_MEAN=1 /D_CRT_SECURE_NO_WARNINGS)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(EINU_ENGINE_BUILD_TESTS)
  enable_testing()
  set(EINU_FETCH_GOOGLETEST ON)
  set(EINU_CORE_BUILD_TESTS ON)
endif()

if(EINU_ENGINE_PROFILE)
  set(EINU_FETCH_EASY_PROFILER ON)
  set(EINU_CORE_PROFILE ON)
endif()

add_subdirectory(extern)
add_subdirectory(core)
add_subdirectory(common)
add_subdirectory(window)
add_subdirectory(graphics)
add_subdirectory(ai)

if(EINU_ENGINE_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
