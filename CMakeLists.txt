cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


project(RedBase
        VERSION 2024.1
        DESCRIPTION "The DBMS"
        LANGUAGES C CXX
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        message(STATUS "Setting build type to `Debug` as none was specified.")
        set(CMAKE_BUILD_TYPE "Debug")
endif()


if(EMSCRIPTEN)
        add_compile_options(-fexceptions)
        add_link_options(-fexceptions)
endif()


if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if(CMAKE_CXX_COMPILER_VERSION MATCHES "^14.")
                message(STATUS "You're using ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
        else()
                message(WARNING "!! We recommend that you use clang-14 for developing Redbase. You're using ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}, a different version.")
        endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(STATUS "You're using ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
else()
        message(WARNING "!! We recommend that you use clang-14 for developing Redbase. You're using ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}, which is not clang.")
endif()


enable_testing()
# =============================================================================================
if(NOT DEFINED REDBASE_SANITIZER)
        set(REDBASE_SANITIZER address)
endif()

set(REDBASE_BUILD_SUPPORT_DIR "${CMAKE_SOURCE_DIR}/build_support")

message("Build mode: ${CMAKE_BUILD_TYPE}")
message("${REDBASE_SANITIZER} sanitizer will be enabled in debug mode.")

# Set Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unused-parameter -Wno-attributes") # TODO: remove
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O1 -g -fsanitize=${REDBASE_SANITIZER} -fno-omit-frame-pointer -fno-optimize-sibling-calls")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")


# Set Includes
set(REDBASE_SRC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/include)
set(REDBASE_TEST_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/test/include)
set(REDBASE_THIRD_PARTY_INCLUDE_DIR
        ${PROJECT_SOURCE_DIR}/third_party
        ${PROJECT_SOURCE_DIR}/third_party/fmt/include)

include_directories(${REDBASE_SRC_INCLUDE_DIR} ${REDBASE_THIRD_PARTY_INCLUDE_DIR} ${REDBASE_TEST_INCLUDE_DIR})
include_directories(BEFORE src)


# #####################################################################################################################
# Other CMake modules
# MUST BE ADDED AFTER CONFIGURING COMPILER PARAMETERS
# #####################################################################################################################
set(CMAKE_MODULE_PATH "${REDBASE_BUILD_SUPPORT_DIR}/cmake;${CMAKE_MODULE_PATH}")
find_package(LibElf)
find_package(LibDwarf)

# Add Sub Module
add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(test)



#################################################
# Make TARGETS
#################################################

