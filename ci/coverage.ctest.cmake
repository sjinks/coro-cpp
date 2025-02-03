include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

set(CTEST_CONFIGURATION_TYPE "Coverage")

find_program(CTEST_COVERAGE_COMMAND llvm-cov)
if(CTEST_COVERAGE_COMMAND)
    set(CTEST_COVERAGE_EXTRA_FLAGS gcov)
endif()

ctest_start(Experimental)
set(options -DCMAKE_CXX_COMPILER=clang++ -DBUILD_DOCS=OFF -DBUILD_EXAMPLES=OFF)
ctest_configure(OPTIONS "${options}")
ctest_build()
ctest_test(
    OUTPUT_JUNIT ${CTEST_BINARY_DIRECTORY}/junit.xml
    RETURN_VALUE test_results
)

if(test_results)
    message(FATAL_ERROR "Tests failed")
endif()

if(CTEST_COVERAGE_COMMAND)
    ctest_coverage(CAPTURE_CMAKE_ERROR err QUIET)
else()
    message(WARNING "llvm-cov command not found, skipping coverage generation")
endif()
