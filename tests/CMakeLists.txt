include(FetchContent)


find_package(CppUTest QUIET)


if(NOT CppUTest_FOUND)
    message(STATUS "CppUTest not found. Fetching via FetchContent...")
    FetchContent_Declare(
        CppUTest
        GIT_REPOSITORY https://github.com/cpputest/cpputest.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(CppUTest)
endif()


file(GLOB_RECURSE UNIT_TESTS "unit/*.cpp")
add_executable(unit_tests ${UNIT_TESTS})
target_link_libraries(unit_tests fat32_lib CppUTest CppUTestExt)


file(GLOB_RECURSE MOCK_TESTS "mocks/*.cpp")
add_executable(mock_tests ${MOCK_TESTS})
target_link_libraries(mock_tests fat32_lib CppUTest CppUTestExt)


file(GLOB_RECURSE INTEGRATION_TESTS "integration/*.cpp")
add_executable(integration_tests ${INTEGRATION_TESTS})
target_link_libraries(integration_tests fat32_lib CppUTest CppUTestExt)


enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)
add_test(NAME mock_tests COMMAND mock_tests)
add_test(NAME integration_tests COMMAND integration_tests)
