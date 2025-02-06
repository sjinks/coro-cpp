if(ENABLE_MAINTAINER_MODE)
    string(REPLACE " " ";" COMPILE_OPTIONS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_MM}")
    set_directory_properties(PROPERTIES COMPILE_OPTIONS "${COMPILE_OPTIONS}")
    unset(COMPILE_OPTIONS)
endif()

set_directory_properties(PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/src")

add_executable(eager_task eager_task.cpp)
target_compile_features(eager_task PRIVATE cxx_std_20)

add_executable(run_awaitable run_awaitable.cpp)
target_compile_features(run_awaitable PRIVATE cxx_std_20)

add_executable(sync_generator_adapter sync_generator_adapter.cpp)
target_compile_features(sync_generator_adapter PRIVATE cxx_std_20)

add_executable(async_generator async_generator.cpp)
target_compile_features(async_generator PRIVATE cxx_std_20)

add_executable(generator generator.cpp)
target_compile_features(generator PRIVATE cxx_std_20)

add_executable(generator_iterator generator_iterator.cpp)
target_compile_features(generator_iterator PRIVATE cxx_std_20)

add_executable(advance_with_begin advance_with_begin.cpp)
target_compile_features(advance_with_begin PRIVATE cxx_std_20)

add_executable(task task.cpp)
target_compile_features(task PRIVATE cxx_std_20)

if(TARGET generate_coverage)
    add_custom_target(
        run_examples
        COMMAND eager_task
        COMMAND run_awaitable
        COMMAND sync_generator_adapter
        COMMAND async_generator
        COMMAND generator
        COMMAND generator_iterator
        COMMAND advance_with_begin
        COMMAND task
        DEPENDS eager_task run_awaitable sync_generator_adapter async_generator generator generator_iterator advance_with_begin task
    )

    add_dependencies(generate_coverage run_examples)
endif()
