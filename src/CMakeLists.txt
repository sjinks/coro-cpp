add_library("${PROJECT_NAME}" INTERFACE)
add_library(wwa::coro ALIAS "${PROJECT_NAME}")
set_target_properties(
    "${PROJECT_NAME}"
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        INTERFACE_COMPILE_FEATURES cxx_std_20
)

target_include_directories(
    ${PROJECT_NAME}
    INTERFACE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_sources(
    "${PROJECT_NAME}"
    PUBLIC
        FILE_SET HEADERS
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            async_generator.h
            detail.h
            eager_task.h
            exceptions.h
            generator.h
            task.h
)

include(GNUInstallDirs)
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-target
    FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/wwa/coro"
)

install(
    EXPORT ${PROJECT_NAME}-target
    FILE ${PROJECT_NAME}-target.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}-config-version.cmake.in
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    @ONLY
)

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    COPYONLY
)

install(
    FILES
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config.cmake
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

configure_file("${CMAKE_SOURCE_DIR}/cmake/pkg-config.pc.in" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc" @ONLY)
install(
    FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
