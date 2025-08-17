#include "CppUTest/TestHarness.h" // Основной заголовок

extern "C"
{
#include "file_utils.h"
}

TEST_GROUP(FileUtilsTests){
    void setup(){} void teardown(){}};

TEST(FileUtilsTests, ValidPaths)
{
    const char *tests[] = {
        "/a/b/c",
        "/home/sergeyathlete/Documents/TimeWork/",
        "/home/sergeyathlete/Documents/TimeWork122.txt",
        "/home/sergeyathlete/Documents/TimeWork122.txt2",
        "/folder/invalid#name",
        "/folder/.hidden",
        "/folder/file.",
        "/folder/file.a1",
        "/folder/file.a",

        "/folder/.a",
        NULL};
    for (int i = 0; tests[i]; i++)
    {
        status = validate_fat_sfn(test_names[i]);
        CHECK_EQUAL(status, 0);
    }
}

TEST(FileUtilsTests, ValidateSFN_ValidNames)
{
    const char *test_names[] = {
        "README.TXT",
        "HELLO123.AB",
        "LONGNAME.TXT",
        "file-name.txt",
        "bad name.txt",
        "toolongextension.jpeg",
        "A.TXT",
        NULL};

    int errors = 0;
    for (int i = 0; tests[i]; i++)
    {
        status = validate_fat_sfn(test_names[i]);
        if (status != 0)
            ++errors;
    }
    CHECK_EQUAL(status, 0);
}
TEST(FileUtilsTests, ValidateSFN_InvalidNames)
{
    const char *test_names[] = {
        "README.TXT",
        "HELLO123.AB",
        "LONGNAME.TXT",
        "file-name.txt",
        "bad name.txt",
        "toolongextension.jpeg",
        "A.TXT",
        NULL};
    int errors = 0;
    for (int i = 0; tests[i]; i++)
    {
        status = validate_fat_sfn(test_names[i]);
        if (status != 0)
            ++errors;
    }
    CHECK_EQUAL(status, 0);
}

TEST(FileUtilsTests, ValidateLFN_ValidNames)
{
    const char *valid_names[] = {
        "This is a valid long filename.txt",
        "Another_long-file_name123.doc",
        "example.LONGEXT",
        "file with spaces.txt",
        NULL
    };
    int errors = 0;
    for (int i = 0; tests[i]; i++)
    {
        status = validate_fat_sfn(test_names[i]);
        if (status != 0)
            ++errors;
    }
    CHECK_EQUAL(status, 0);
}

TEST(FileUtilsTests, ValidateLFN_InvalidNames)
{
    const char *invalid_names[] = {
        "name/with/slash.txt",       
        "name*with*star.txt",        
        "name?with?question.txt",    
        "",                          
        NULL
    };
    int errors = 0;
    for (int i = 0; tests[i]; i++)
    {
        status = validate_fat_sfn(test_names[i]);
        if (status != 0)
            ++errors;
    }
    CHECK_EQUAL(status, 0);
}
