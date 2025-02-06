get_filename_component(CORO_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

list(APPEND CMAKE_MODULE_PATH ${CORO_CMAKE_DIR})

if(NOT TARGET wwa-coro)
    include("${CORO_CMAKE_DIR}/wwa-coro-target.cmake")
    add_library(wwa::coro ALIAS wwa-coro)
endif()
