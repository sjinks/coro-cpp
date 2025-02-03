cmake_minimum_required(VERSION 3.23)

include(CMakeDependentOption)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_DOCS "Build documentation" ON)
cmake_dependent_option(BUILD_INTERNAL_DOCS "Build internal documentation" OFF "BUILD_DOCS" OFF)
option(ENABLE_MAINTAINER_MODE "Enable maintainer mode" OFF)
option(USE_CLANG_TIDY "Use clang-tidy" OFF)

include(FetchContent)

project(
    wwa-coro
    VERSION 0.0.1
    DESCRIPTION "Yet Another C++20 Coroutine Library"
    LANGUAGES CXX
)

if(CMAKE_CONFIGURATION_TYPES)
    list(APPEND CMAKE_CONFIGURATION_TYPES "Coverage" "ASAN" "LSAN" "TSAN" "UBSAN")
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
string(TOLOWER "${CMAKE_CONFIGURATION_TYPES}" CMAKE_CONFIGURATION_TYPES_LOWER)

string(REGEX MATCH "Clang" CMAKE_COMPILER_IS_CLANG "${CMAKE_CXX_COMPILER_ID}")
string(REGEX MATCH "GNU" CMAKE_COMPILER_IS_GNU "${CMAKE_CXX_COMPILER_ID}")

if(BUILD_TESTS)
    include(FindGTest)
    find_package(GTest CONFIG)
    if(NOT TARGET GTest::gtest)
        message(STATUS "Google Test not found, fetching it from GitHub")
        # renovate: datasource=github-tags depName=google/googletest
        set(GTEST_VERSION "v1.15.2")
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest
            GIT_TAG "${GTEST_VERSION}"
            GIT_SHALLOW ON
        )

        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
endif()

if(BUILD_DOCS)
    include(FindDoxygen)
    find_package(Doxygen)
    if(NOT DOXYGEN_FOUND)
        message(WARNING "Doxygen not found, documentation will not be built")
        set(BUILD_DOCS OFF)
    else()
        if(TARGET Doxygen::dot)
            set(HAVE_DOT "YES")
        else()
            set(HAVE_DOT "NO")
        endif()

        if(BUILD_INTERNAL_DOCS)
            set(ENABLED_SECTIONS "INTERNAL")
            set(EXTRACT_PRIVATE "YES")
        else()
            set(ENABLED_SECTIONS "")
            set(EXTRACT_PRIVATE "NO")
        endif()

        configure_file("${CMAKE_SOURCE_DIR}/cmake/Doxyfile.in" "${CMAKE_BINARY_DIR}/Doxyfile" @ONLY)
        add_custom_target(
            docs
            ALL
            COMMAND Doxygen::doxygen "${CMAKE_BINARY_DIR}/Doxyfile"
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_BINARY_DIR}/docs/html/" "${CMAKE_SOURCE_DIR}/apidocs"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

if(CMAKE_COMPILER_IS_GNU OR CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_CXX_FLAGS_ASAN "-O1 -g -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
    set(CMAKE_CXX_FLAGS_TSAN "-O1 -g -fsanitize=thread -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_LSAN "-O1 -g -fsanitize=leak -fno-omit-frame-pointer -fno-optimize-sibling-calls")

    if(CMAKE_COMPILER_IS_GNU)
        set(CMAKE_CXX_FLAGS_COVERAGE "-Og -g --coverage -fprofile-abs-path")
        set(CMAKE_CXX_FLAGS_UBSAN "-O1 -g -fsanitize=undefined -fsanitize=float-divide-by-zero -fno-omit-frame-pointer")
    elseif(CMAKE_COMPILER_IS_CLANG)
        set(CMAKE_CXX_FLAGS_COVERAGE "-Og -g --coverage")
        set(CMAKE_CXX_FLAGS_UBSAN "-O1 -g -fsanitize=undefined -fsanitize=float-divide-by-zero -fsanitize=integer -fsanitize=implicit-conversion -fsanitize=local-bounds -fsanitize=nullability -fno-omit-frame-pointer")
    endif()
endif()

if(ENABLE_MAINTAINER_MODE)
    if(CMAKE_COMPILER_IS_CLANG)
        set(CMAKE_CXX_FLAGS_MM -Weverything -Werror -pedantic -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-c++20-compat -Wno-c++20-extensions -Wno-unknown-pragmas -Wno-padded -Wno-weak-vtables)
    elseif(CMAKE_COMPILER_IS_GNU)
        set(CMAKE_CXX_FLAGS_MM -Wall -Wextra -Werror -pedantic -Wno-unknown-pragmas)
    endif()
endif()

if(USE_CLANG_TIDY)
    find_program(CMAKE_CXX_CLANG_TIDY NAMES clang-tidy)
    if(CMAKE_CXX_CLANG_TIDY)
        list(APPEND CMAKE_CXX_CLANG_TIDY -p;${CMAKE_BINARY_DIR})
    endif()
endif()

add_subdirectory(src)

if(BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

find_program(CLANG_FORMAT NAMES clang-format)
find_program(CLANG_TIDY NAMES clang-tidy)
if(CLANG_FORMAT OR CLANG_TIDY)
    file(GLOB_RECURSE ALL_SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} LIST_DIRECTORIES OFF examples/*.cpp src/*.cpp test/*.cpp)
    file(GLOB_RECURSE ALL_HEADER_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} LIST_DIRECTORIES OFF examples/*.h src/*.h test/*.h)

    if(CLANG_FORMAT)
        add_custom_target(
            format
            COMMAND ${CLANG_FORMAT} --Wno-error=unknown -i -style=file ${ALL_SOURCE_FILES} ${ALL_HEADER_FILES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()

    if(CLANG_TIDY)
        add_custom_target(
            tidy
            COMMAND ${CLANG_TIDY} -p ${CMAKE_BINARY_DIR} ${ALL_SOURCE_FILES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endif()
