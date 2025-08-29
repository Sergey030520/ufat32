#pragma once


#include <stdint.h>
#include "fat32_types.h"



typedef enum {
     FAT_ERR_NULL = -1,          // NULL или пустой указатель
    FAT_ERR_EMPTY = -2,         // Пустое имя или сегмент
    FAT_ERR_TOO_LONG = -3,      // Сегмент или имя слишком длинное
    FAT_ERR_INVALID_CHAR = -4,  // Недопустимый символ в имени или расширении
    FAT_ERR_DOT_MULTIPLE = -5,  // Несколько точек в имени
    FAT_ERR_EXT_LEN = -6,       // Недопустимая длина расширения
    FAT_ERR_PATH_INVALID = -60, // Ошибка формата пути
    FAT_ERR_PATH_EMPTY_SEG = -11 // Пустой сегмент пути
}fat_validate_status_t;

/**
 * Проверяет корректность пути файловой системы.
 * Путь должен начинаться с '/' и состоять из сегментов,
 * разделённых '/', каждый сегмент содержит допустимые символы.
 * В конце путь может содержать сегмент с расширением.
 *
 * @param path — строка с проверяемым путём.
 * @return 0 — если путь корректен,
 *         отрицательное значение — при ошибке.
 */
int validate_path(const char *path);

/**
 * Проверяет корректность имени файла в формате FAT LFN.
 * Имя может содержать буквы, цифры, символы '_' и '-'.
 * Допускается расширение длиной до 5 символов, содержащих только буквы, цифры и '_'.
 *
 * @param name — имя файла для проверки.
 * @return 0 — если имя корректно,
 *         отрицательное значение — при ошибке.
 */
int validate_fat_lfn_file(const char *name);

/**
 * Проверяет корректность имени файла в формате FAT SFN.
 * Имя должно содержать только заглавные буквы (A-Z), цифры (0-9) и символ '_'.
 * Имя — до 8 символов, расширение — до 3 символов.
 * Допускается только одно расширение, разделённое точкой.
 *
 * @param name — имя файла для проверки.
 * @return 0 — если имя корректно,
 *         отрицательное значение — при ошибке.
 */
int validate_fat_sfn_file(const char *name);


/**
 * Проверяет корректность имени файла/директории в формате FAT LFN (Long File Name).
 * Имя может содержать буквы, цифры, символы '_' и '-'.
 * Допускается расширение длиной до 5 символов, содержащее только буквы, цифры и '_'.
 *
 * @param name — строка с именем для проверки.
 * @return 0 — если имя корректно,
 *         отрицательное значение — при ошибке.
 */
int validate_fat_lfn_dir(const char *name);

/**
 * Проверяет корректность имени файла/директории в формате FAT SFN (Short File Name, 8.3).
 * Допустимы только заглавные буквы (A-Z), цифры (0-9) и символ '_'.
 * Имя — до 8 символов, расширение — до 3 символов.
 * Допускается только одно расширение, разделённое точкой.
 *
 * @param name — строка с именем для проверки.
 * @return 0 — если имя корректно,
 *         отрицательное значение — при ошибке.
 */
int validate_fat_sfn_dir(const char *name);



/**
 * @brief Вычисляет глубину пути (количество компонентов)
 */
int fat32_path_depth(const char *path);

/**
 * @brief Разбирает путь на отдельные компоненты (имена файлов/директорий)
 * @param path - строка пути
 * @param name_files - массив для хранения компонентов
 * @return количество компонентов
 */
int fat32_parse_path(const char *path, char name_files[][MAX_NAME_SIZE]);

/**
 * @brief Форматирует короткое имя FAT (SFN) из ASCII строки
 */
void fat32_format_sfn(const char *input, uint16_t length, char output[11]);

/**
 * @brief Проверяет, является ли имя специальной директорией "." или ".."
 */
int fat32_is_special_dir(const uint8_t dir_name[11]);

/**
 * @brief Сравнивает LFN (UTF-16) с ASCII строкой
 * @return 0 если совпадают, иначе != 0
 */
int fat32_compare_lfn(const char *name_ascii, const uint16_t *name_unicode);

/**
 * @brief Генерирует короткое имя (SFN) из длинного имени (LFN)
 */
void fat32_generate_sfn_from_lfn(const char *name, uint32_t length, uint8_t buffer[11]);

/**
 * @brief Вычисляет контрольную сумму SFN (для LFN)
 */
uint8_t fat32_sfn_checksum(const uint8_t *pFcbName);

/**
 * @brief Конвертирует ASCII строку в UTF-16LE
 */
void fat32_ascii_to_utf16le(const char *ascii_str, uint8_t *utf16le_buf);

/**
 * @brief Конвертирует UTF-16LE буфер в ASCII строку
 */
void fat32_utf16le_to_ascii(const uint16_t *utf16le_buf, char *ascii_str);


/**
 * @brief Находит последнее вхождение символа в строке
 * @param text - строка для поиска
 * @param symbol - символ для поиска
 * @return указатель на последнее вхождение символа или NULL, если не найдено
 */
char* fat32_find_last_char(char *text, char symbol);