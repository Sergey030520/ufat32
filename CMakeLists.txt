cmake_minimum_required(VERSION 3.15)
project(fat32_project C CXX)

option(BUILD_EXAMPLE "Build example main program" ON)
option(BUILD_TESTS "Build tests" OFF)

# Опции логирования
option(USE_STM_LOG "Use STM logging" OFF)
option(USE_LINUX_LOG "Use Linux logging" ON)  # по умолчанию для Linux

# Добавляем src как библиотеку
add_subdirectory(src)

# Пример main
if(BUILD_EXAMPLE)
    add_executable(main_exec examples/main_example.c)

    # Выбираем адаптер логирования
    if(USE_STM_LOG)
        add_library(fat32_log_adapter src/log_stm.c)
    elseif(USE_LINUX_LOG)
        add_library(fat32_log_adapter src/log_linux.c)
    else()
        add_library(fat32_log_adapter src/log_stub.c)
    endif()

    target_link_libraries(main_exec fat32_lib fat32_log_adapter)
endif()

# Тесты
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()