include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

set(CTEST_CONFIGURATION_TYPE "Debug")

find_program(CTEST_MEMORYCHECK_COMMAND valgrind)
set(CTEST_MEMORYCHECK_COMMAND_OPTIONS "--error-exitcode=1 -q --track-origins=yes --leak-check=yes --show-reachable=yes --num-callers=50")
set(CTEST_MEMORYCHECK_TYPE "Valgrind")

ctest_start(Experimental)
if(CTEST_MEMORYCHECK_COMMAND)
    # Need -O1 becuase valgrind is too slow
    set(options -DCMAKE_CXX_COMPILER=clang++;-DCMAKE_CXX_FLAGS=-O1)
    ctest_configure(OPTIONS "${options}")
    ctest_build()
    ctest_memcheck(
        OUTPUT_JUNIT ${CTEST_BINARY_DIRECTORY}/junit.xml
        RETURN_VALUE test_results
    )

    if(test_results)
        message(FATAL_ERROR "Tests failed")
    endif()
else()
    message(WARNING "valgrind command not found, skipping check")
endif()
