cmake_minimum_required(VERSION 3.15)
project(fat32_project C CXX)

option(BUILD_EXAMPLE "Build example main program" OFF)
option(BUILD_TESTS "Build tests" OFF)



# Добавляем src как библиотеку
add_subdirectory(src)

# Пример main
if(BUILD_EXAMPLE)
    add_executable(fat32_example
        examples/main_example.c
        examples/mock_sd.c
        examples/log_linux_example.c
    )

    target_include_directories(fat32_example PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        examples
    )

    target_link_libraries(fat32_example PRIVATE fat32_lib)
endif()

# Тесты
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()