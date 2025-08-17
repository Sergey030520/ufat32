#include "CppUTest/TestHarness.h"           // Основной заголовок
#include "CppUTest/CommandLineTestRunner.h" // Для запуска
#include "mock_device.hpp"


extern "C"
{
#include "FAT.h"
#include "SD.h"
}

TEST_GROUP(FAT32Tests){
    void setup(){
        mock_device_init();
}
void teardown()
{
    mock_device_deinit();
}
}
;

TEST(FAT32_TESTS, CreateFile_Success)
{
    // Инициализация мока устройства
    mock_device_init();

    BlockDevice device = {
        .write = write_device,
        .read =  read_device,
        .clear = clear_device,
    };
    read_device

    // Монтируем тестовый том (загружаем в память заранее подготовленный образ)
    int status = mount_fat32();
    CHECK_EQUAL(status, 0);

    // Пытаемся создать файл
    FAT32_File *file = NULL;
    status = open_file_fat32("/folder/newfile.txt", &file, FILE_Mode::F_WRITE);
    CHECK_EQUAL(status, 0);

    // Проверяем, что запись файла появилась в каталоге
    // (например, читаем директорию и ищем имя)
    status = path_exists_fat32("/folder/newfile.txt");
    CHECK_EQUAL(status, 0);

    CHECK_EQUAL(0, flush_fat32(file));
    CHECK_EQUAL(0, close_file_fat32(file));

    // Очистка мока
    mock_device_deinit();
}

TEST(FAT32Tests, FormatThenMountSuccess)
{
    int status = fat32_format();
    CHECK_EQUAL(0, status);

    status = fat32_mount();
    CHECK_EQUAL(0, status);
}

TEST(FAT32Tests, CreateFileSuccessfully)
{
    int status = fat32_format();
    CHECK_EQUAL(0, status);

    status = fat32_mount();
    CHECK_EQUAL(0, status);

    FAT32_File *file = NULL;
    status = open_file_fat32("/", file, FILE_Mode::F_WRITE);
    CHECK_EQUAL(status, 0);

    CHECK_EQUAL(0, flush_fat32(file));
    CHECK_EQUAL(0, close_file_fat32(file));

    // можно дополнительно проверить, что файл присутствует:
    // например, fat32_find_entry("/HELLO.TXT") == true
}

TEST(FAT32Tests, CreateDirectory)
{
    CHECK_EQUAL(0, mkdir_fat32("/mydir"));
    CHECK(path_exists_fat32("/mydir"));
}

TEST(FAT32Tests, CreateAndDeleteFile)
{
    FAT32_File *file = NULL;
    CHECK_EQUAL(0, open_file_fat32("/file.txt", file, FILE_Mode::F_WRITE));
    CHECK(path_exists_fat32("/file.txt"));
    CHECK_EQUAL(0, flush_fat32(file));
    CHECK_EQUAL(0, close_file_fat32(file));

    CHECK_EQUAL(0, delete_file_fat32("/file.txt"));
    CHECK(!path_exists_fat32("/file.txt"));
}

TEST(FAT32Tests, CreateAndDeleteEmptyDirectory)
{
    CHECK_EQUAL(0, mkdir_fat32("/emptydir"));
    CHECK(path_exists_fat32("/emptydir"));

    CHECK_EQUAL(0, delete_dir_fat32("/emptydir", DELETE_DIR_SAFE));
    CHECK(!path_exists_fat32("/emptydir"));
}

TEST(FAT32Tests, RecursiveDeleteNonEmptyDirectory)
{
    CHECK_EQUAL(0, mkdir_fat32("/parent"));
    FAT32_File *file = NULL;
    CHECK_EQUAL(0, open_file_fat32("/parent/child.txt", file, FILE_Mode::F_WRITE));
    CHECK(path_exists_fat32("/parent/child.txt"));
    CHECK_EQUAL(0, flush_fat32(file));
    CHECK_EQUAL(0, close_file_fat32(file));

    CHECK_EQUAL(0, delete_dir_fat32("/parent", DELETE_DIR_RECURSIVE)); // должно удалить рекурсивно
    CHECK(!path_exists_fat32("/parent"));
}

TEST(FAT32Tests, WriteToFile)
{
    const char *data = "Hello FAT32";
    FAT32_File *file = NULL;
    CHECK_EQUAL(0, open_file_fat32("/write.txt"), file, FILE_Mode::F_WRITE);
    CHECK_EQUAL(0, write_file_fat32(file, data, strlen(data)));

    CHECK_EQUAL(0, flush_fat32(file));
    CHECK_EQUAL(0, close_file_fat32(file));

    char buffer[64] = {0};
    CHECK_EQUAL(0, open_file_fat32("/write.txt"), file, FILE_Mode::F_READ);
    CHECK_EQUAL(0, read_file_fat32(, buffer, strlen(data)));
    STRCMP_EQUAL(data, buffer);
    CHECK_EQUAL(0, flush_fat32(file));
    CHECK_EQUAL(0, close_file_fat32(file));
}

TEST(FAT32Tests, AppendToFile)
{
    const char *part1 = "Hello ";
    const char *part2 = "World!";
    CHECK_EQUAL(0, open_file_fat32("/append.txt"), file, FILE_Mode::F_WRITE);
    CHECK_EQUAL(0, fat32_write_file("/append.txt", part1, strlen(part1)));
    CHECK_EQUAL(0, flush_fat32(file));
    CHECK_EQUAL(0, close_file_fat32(file));

    CHECK_EQUAL(0, open_file_fat32("/append.txt"), file, FILE_Mode::F_APPEND);
    CHECK_EQUAL(0, fat32_write_file("/append.txt", part2, strlen(part2)));
    CHECK_EQUAL(0, flush_fat32(file));
    CHECK_EQUAL(0, close_file_fat32(file));

    char buffer[64] = {0};
    CHECK_EQUAL(0, open_file_fat32("/append.txt"), file, FILE_Mode::F_READ);
    CHECK_EQUAL(0, read_file_fat32("/append.txt", buffer, sizeof(part1) + srtlen(part2)));
    CHECK_EQUAL(0, flush_fat32(file));
    CHECK_EQUAL(0, close_file_fat32(file));
    STRCMP_EQUAL("Hello World!", buffer);
}

TEST(FAT32Tests, ReadNonExistentFileFails)
{
    char buffer[32];
    CHECK_EQUAL(0, open_file_fat32("/no_such_file.txt"), file, FILE_Mode::F_READ);
    int status = read_file_fat32("/no_such_file.txt", buffer, sizeof(buffer));
    CHECK(status >= 0);
}