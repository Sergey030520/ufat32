# Hi 👋, I'm Sergey Makarov

![Profile views](https://komarev.com/ghpvc/?username=sergey030520&label=Profile%20views&color=0e75b6&style=flat)

- 🔭 I’m currently working on [ufat32](https://github.com/Sergey030520/ufat32.git)  
- 👨‍💻 All of my projects are available at [https://github.com/Sergey030520](https://github.com/Sergey030520)

## Languages and Tools

![C](https://github.com/tandpfun/skill-icons/blob/main/icons/C.svg) 
![CMake](https://github.com/tandpfun/skill-icons/blob/main/icons/CMake-Dark.svg) 
![VSCode](https://github.com/tandpfun/skill-icons/blob/main/icons/VSCode-Dark.svg) 
![Git](https://github.com/tandpfun/skill-icons/blob/main/icons/Git.svg) 

#  ufat32 — FAT32 для STM32 и Linux

## Оглавление

1. [Описание](#description_project)  
2. [Основные возможности](#features_project)  
3. [Аллокатор памяти](#allocator_project)  
4. [Логирование](#logging_project)  
5. [Структура проекта и подключение библиотеки FAT32](#structure_project)  
6. [Сборка проекта](#build_project)  
7. [Тестирование](#testing)  
8. [Поддерживаемые платформы](#platforms)  
9. [Работа с FAT32 и абстракцией накопителя](#interface_project)  
    - [Основные функции](#functions_project)  
    - [Интерфейс блочного устройства](#block_device_project)  
10. [Примеры работы](#example_work_project)  
    - [Инициализация и открытие файла](#example_init_file)  
    - [Запись и чтение](#example_read_write)  
    - [Создание директории](#example_mkdir)  
    - [Проверка существования файла или папки](#example_path_exists)  
    - [Удаление файла или директории](#example_delete)  
    - [Перемещение указателя и получение позиции](#example_seek_tell)  
11. [Ссылка на проект](#project_link)
---

## Описание <a name="description_project"></a>

**ufat32** реализует файловую систему FAT32 для **STM32** и **Linux**, с возможностью использования **любого блочного устройства**, которое предоставляет базовые функции чтения, записи и очистки секторов.  

Это делает библиотеку универсальной и независимой от конкретного типа накопителя — будь то SD-карта, eMMC, флеш-память или виртуальный накопитель для тестирования.


## Основные возможности  <a name="features_project"></a>

- Монтирование файловой системы на любом блочном устройстве (`mount_fat32`)  
- Форматирование накопителя в FAT32 (`formatted_fat32`)  
- Работа с файлами:
  - Открытие (`open_file_fat32`) и закрытие (`close_file_fat32`)  
  - Чтение (`read_file_fat32`) и запись (`write_file_fat32`)  
  - Сброс буфера (`flush_fat32`)  
  - Перемещение указателя (`seek_file_fat32`) и получение текущей позиции (`tell_fat32`)  
- Работа с директориями:
  - Создание (`mkdir_fat32`)  
  - Поиск и получение кластера (`find_directory_fat32`)  
  - Удаление (`delete_dir_fat32`)  
- Управление файлами и путями:
  - Удаление файлов (`delete_file_fat32`)  
  - Проверка существования пути (`path_exists_fat32`)  
- Абстракция любого блочного устройства (работа с любыми накопителями через `BlockDevice`)  
- Поддержка кастомного аллокатора памяти и логирования  
- Совместимость с Linux и STM32  

## Аллокатор памяти <a name="allocator_project"></a>

По умолчанию **ufat32** использует стандартные `malloc` и `free`.  

Для использования кастомного аллокатора можно определить структуру `Fat32Allocator` и передать её в библиотеку:

```
Fat32Allocator allocator = {0};

// Пример кастомного аллокатора
allocator.alloc = pool_alloc;           // функция выделения памяти
allocator.free = pool_free_region;      // функция освобождения памяти
allocator.allocator_init = pool_init;   // функция инициализации аллокатора (не обязательна)

fat32_allocator_init(&allocator);
```
> ⚠️ **Обратите внимание:** функция `allocator_init` не является обязательной.  
> Наличие или отсутствие её вызова зависит от конкретной реализации аллокатора.

## Логирование <a name="logging_project"></a>

Логгер задается через Fat32LogCallback:
```
typedef void (*Fat32LogCallback)(Fat32LogLevel level, const char *file, int line, const char *format, va_list args);
fat32_set_logger(linux_log);
```
## Структура проекта и подключение библиотеки FAT32 <a name="structure_project"></a>
```
ufat32/
├─ include/fat32/ # Заголовочные файлы библиотеки FAT32
├─ src/ # Исходники библиотеки FAT32
├─ examples/ # Пример проекта с эмуляцией блочного устройства
├─ tests/mocs/ # Моки для эмуляции блочного устройства
├─ tests/unit/ # Unit-тесты, используют CppUTest
│ ├─ tests_fat32.cpp
│ └─ tests_file_utils.cpp
```

## Сборка проекта <a name="build_project"></a>

Через CMake:
```
mkdir build
cd build
cmake .. -DBUILD_EXAMPLE=ON   # для сборки примера
cmake .. -DBUILD_TESTS=ON     # для сборки тестов
make
```
## Тестирование <a name="testing"></a>

Unit-тесты находятся в `tests/unit/` и используют **CppUTest**:

- `tests_fat32.cpp` — тестирование основных функций FAT32  
- `tests_file_utils.cpp` — проверка вспомогательных функций (путь, имя файла)  

Для работы с блочным устройством используются моки из `tests/mocs/`, что позволяет тестировать функционал без физического накопителя.  

> ℹ️ **CppUTest** обеспечивает возможность запуска unit-тестов как на Linux, так и на STM32.


## Поддерживаемые платформы <a name="platforms"></a>

- Linux  
- STM32F407VET6  

*(на STM32 также можно интегрировать с любым совместимым блочным устройством)*

## Работа с FAT32 и абстракцией накопителя <a name="interface_project"></a>
### Основные функции <a name="functions_project"></a>

| Функция | Описание |
|---------|----------|
| `int mount_fat32(BlockDevice *device)` | Монтирование файловой системы FAT32 на заданном блочном устройстве |
| `int formatted_fat32(BlockDevice *device, uint64_t capacity)` | Форматирование блочного устройства в FAT32 с указанной ёмкостью |
| `int flush_fat32(FAT32_File *file)` | Сброс буфера файла на накопитель |
| `int mkdir_fat32(char *path)` | Создание новой директории по указанному пути |
| `int seek_file_fat32(FAT32_File *file, int32_t offset, SEEK_Mode mode)` | Установка позиции указателя файла |
| `int open_file_fat32(char *path, FAT32_File **file, uint8_t mode)` | Открытие файла с указанным режимом (чтение, запись, дозапись) |
| `int close_file_fat32(FAT32_File **file)` | Закрытие файла и освобождение ресурсов |
| `uint32_t tell_fat32(FAT32_File *file)` | Получение текущей позиции указателя в файле |
| `int find_directory_fat32(char *path, uint32_t *out_cluster)` | Поиск директории по пути и получение её кластера |
| `int delete_file_fat32(char *path)` | Удаление файла по указанному пути |
| `int delete_dir_fat32(char *path, DeleteDirMode mode)` | Удаление директории (обычное или рекурсивное) |
| `int path_exists_fat32(char *path)` | Проверка существования файла или директории |
| `int read_file_fat32(FAT32_File *file, uint8_t *buffer, const uint32_t size)` | Чтение данных из файла в буфер |
| `int write_file_fat32(FAT32_File *file, uint8_t *buffer, uint32_t length)` | Запись данных из буфера в файл |


### Интерфейс блочного устройства <a name="block_device_project"></a>

Для подключения любого накопителя необходимо реализовать структуру BlockDevice:
```
typedef int (*fs_read_t)(uint8_t *buffer, uint32_t size, uint32_t start_sector, uint32_t sector_size);
typedef int (*fs_write_t)(const uint8_t *buffer, uint32_t size, uint32_t start_sector, uint32_t sector_size);
typedef int (*fs_clear_t)(uint32_t sector_num, uint32_t count_sector, uint32_t sector_size);

typedef struct {
    fs_read_t read;
    fs_write_t write;
    fs_clear_t clear;
    uint32_t block_size;
} BlockDevice;
```

## Примеры работы <a name="example_work_project"></a>

### Инициализация и открытие файла <a name="example_init_file"></a>
```
FAT32_File *file = NULL;
int status = fat32_open("/MYDIR/TEST/test.txt", &file, F_WRITE);
if (status != 0) {
    LOG_INFO("Failed to open file (err=%d)", status);
}
```
### Запись и чтение <a name="example_read_write"></a>
```c
FAT32_File *file = NULL;
char *file_path = "/MYDIR/TEST/test.txt";
char *data = "Hello FAT32! This is test data.\n";

// Открытие файла для записи
int status = open_file_fat32(file_path, &file, F_WRITE);
if (status != 0) {
    LOG_INFO("Failed to open file %s (err=%d)", file_path, status);
}

// Запись данных
status = write_file_fat32(file, (uint8_t*)data, strlen(data));
if (status >= 0) {
    LOG_INFO("Written to %s: '%s'", file_path, data);
}

// Сброс буфера и закрытие файла
flush_fat32(file);
close_file_fat32(&file);

// Открытие файла для чтения
status = open_file_fat32(file_path, &file, F_READ);
if (status != 0) {
    LOG_INFO("Failed to open file %s (err=%d)", file_path, status);
}

// Чтение данных
char buffer[128];
status = read_file_fat32(file, (uint8_t*)buffer, sizeof(buffer) - 1);
if (status >= 0) {
    buffer[status] = '\0';
    LOG_INFO("Read from %s: '%s'", file_path, buffer);
}

close_file_fat32(&file);
```
### Создание директории <a name="example_mkdir"></a>
```
int status = mkdir_fat32("/MYDIR/NEW_FOLDER");
if (status == 0) {
    LOG_INFO("Directory created successfully!");
} else {
    LOG_INFO("Failed to create directory (err=%d)", status);
}
```

### Проверка существования файла или папки <a name="example_path_exists"></a>
```
if (path_exists_fat32("/MYDIR/TEST/test.txt")) {
    LOG_INFO("File exists!");
} else {
    LOG_INFO("File does not exist.");
}

```

### Удаление файла или директории <a name="example_delete"></a>
```
// Удаление файла
int status_file = delete_file_fat32("/MYDIR/TEST/test.txt");

// Удаление директории рекурсивно
int status_dir = delete_dir_fat32("/MYDIR/NEW_FOLDER", DELETE_DIR_RECURSIVE);

```

### Перемещение указателя и получение позиции <a name="example_seek_tell"></a>
```
FAT32_File *file = NULL;
open_file_fat32("/MYDIR/TEST/test.txt", &file, F_READ);

// Перемещение указателя
seek_file_fat32(file, 10, SEEK_SET);

// Получение текущей позиции
uint32_t pos = tell_fat32(file);

close_file_fat32(&file);
```

## Ссылка на проект <a name="project_link"></a>

[STM32 SDIO + FAT32 на GitHub](https://github.com/Sergey030520/stm32-sdio-fat32.git)
