include(GoogleTest)

if(ENABLE_MAINTAINER_MODE)
    string(REPLACE " " ";" COMPILE_OPTIONS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_MM} -Wno-global-constructors -Wno-exit-time-destructors -Wno-self-move")
    set_directory_properties(PROPERTIES COMPILE_OPTIONS "${COMPILE_OPTIONS}")
    unset(COMPILE_OPTIONS)
endif()

set_directory_properties(PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/src")

add_executable(
    coro_test
    async_generator.cpp
    eager_task.cpp
    generator.cpp
    task.cpp
)
target_link_libraries(coro_test PRIVATE GTest::gtest_main)
target_compile_features(coro_test PRIVATE cxx_std_20)

if(NOT CMAKE_CROSSCOMPILING)
    include(GoogleTest)
    gtest_discover_tests(coro_test)
endif()

set(ENABLE_COVERAGE OFF)
if("coverage" IN_LIST CMAKE_CONFIGURATION_TYPES_LOWER OR "coverage" STREQUAL CMAKE_BUILD_TYPE_LOWER)
    if(CMAKE_COMPILER_IS_GNU OR CMAKE_COMPILER_IS_CLANG)
        find_program(GCOVR gcovr)
        if (GCOVR)
            if(CMAKE_COMPILER_IS_GNU)
                find_program(GCOV gcov)
                set(GCOV_TOOL_NAME gcov)
                set(GCOV_TOOL gcov)
            elseif(CMAKE_COMPILER_IS_CLANG)
                find_program(GCOV llvm-cov)
                set(GCOV_TOOL_NAME llvm-cov)
                set(GCOV_TOOL llvm-cov gcov)
            endif()

            if(GCOV)
                set(ENABLE_COVERAGE ON)
            else()
                message(WARNING "${GCOV_TOOL_NAME} not found, coverage report will not be generated")
            endif()
        else()
            message(WARNING "gcovr not found, coverage report will not be generated")
        endif()
    endif()
endif()

if(ENABLE_COVERAGE)
    add_custom_target(
        clean_coverage
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${PROJECT_BINARY_DIR}/coverage"
        COMMAND find "${PROJECT_BINARY_DIR}" -name *.gcda -type f -delete
        COMMENT "Cleaning coverage files"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )

    add_dependencies(coro_test clean_coverage)

    add_custom_target(
        generate_coverage
        COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> -T test --output-on-failure
        COMMENT "Running tests to generate coverage data"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )

    add_dependencies(generate_coverage coro_test)

    add_custom_command(
        OUTPUT "${PROJECT_BINARY_DIR}/coverage/index.html"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${PROJECT_BINARY_DIR}/coverage"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/coverage"
        COMMAND
            gcovr -f "${PROJECT_SOURCE_DIR}/src/" -r "${PROJECT_SOURCE_DIR}"
                --html-details -o "${PROJECT_BINARY_DIR}/coverage/index.html"
                --exclude-noncode-lines --exclude-throw-branches --exclude-unreachable-branches --decisions
                --gcov-executable="${GCOV_TOOL}"
                --gcov-exclude-directories "${PROJECT_SOURCE_DIR}/vcpkg"
                --gcov-delete
                --print-summary
                --txt "${PROJECT_BINARY_DIR}/coverage/coverage.txt"
                --lcov "${PROJECT_BINARY_DIR}/coverage/coverage.lcov"
                --sonarqube "${PROJECT_BINARY_DIR}/coverage/coverage.xml"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMENT "Generating coverage report"
    )

    add_custom_target(
        coverage_report
        DEPENDS "${PROJECT_BINARY_DIR}/coverage/index.html"
    )

    add_custom_target(coverage DEPENDS "${PROJECT_BINARY_DIR}/coverage/index.html")
    add_dependencies(coverage generate_coverage)
endif()
