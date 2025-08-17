#include "file_utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/**
 * Проверяет, что сегмент пути или имени файла содержит только допустимые символы.
 *
 * @param seg Указатель на строку сегмента (часть пути или имя файла).
 * @param allow_dot Флаг, разрешающий точку '.' в сегменте.
 * @return 0 если все символы допустимы или сегмент пустой/NULL,
 *         -1 если найден недопустимый символ.
 */
int is_valid_segment(const char *seg, int allow_dot)
{
    if (seg == NULL || *seg == '\0')
        return 0;

    for (int i = 0; seg[i]; i++)
    {
        if (!(isalnum((unsigned char)seg[i]) || seg[i] == '-' || seg[i] == '_' || (allow_dot && seg[i] == '.')))
        {
            return -1;
        }
    }
    return 0;
}

/**
 * Проверяет расширение файла в строке segment.
 * 
 * @param segment — строка с именем файла (включая расширение).
 * @return 0, если расширение корректное или отсутствует,
 *         FAT_ERR_EXT_LEN, если длина расширения 0 или больше 3,
 *         FAT_ERR_INVALID_CHAR, если в расширении есть недопустимые символы (не буквы).
 */
int check_extension(const char *segment)
{
    const char *dot = strrchr(segment, '.');
    if (!dot)
        return 0;

    int ext_len = strlen(dot + 1);
    if (ext_len == 0 || ext_len > 3)
        return FAT_ERR_EXT_LEN;

    for (int i = 1; i <= ext_len; i++)
    {
        if (!isalpha((unsigned char)dot[i]))
            return FAT_ERR_INVALID_CHAR;
    }
    return 0;
}



int validate_path(const char *path)
{
    if (!path || *path != '/')
        return 0;

    const char *p = path + 1;
    char segment[256];
    int len = 0;
    int status = 0;

    while (*p)
    {
        if (*p == '/')
        {
            if (len == 0)
                return FAT_ERR_PATH_EMPTY_SEG;
            segment[len] = '\0';
            status = is_valid_segment(segment, 0);
            if (status)
                return status;
            len = 0;
            p++;
            continue;
        }

        if (len >= 255)
            return FAT_ERR_TOO_LONG;
        segment[len++] = *p++;
    }

    if (len > 0)
    {
        segment[len] = '\0';
        if (is_valid_segment(segment, 1))
            return FAT_ERR_INVALID_CHAR;
        status = check_extension(segment);
        if (status)
            return status;
    }

    return 0;
}

int validate_fat_sfn_file(const char *name)
{
    if (!name || !*name)
        return 0;

    const char *dot = strchr(name, '.');

    int name_len = (dot ? (dot - name) : strlen(name));
    int ext_len = (dot ? strlen(dot + 1) : 0);

    if (dot && strchr(dot + 1, '.'))
        return FAT_ERR_INVALID_CHAR;

    if (name_len > 8 || name_len == 0)
    {
        return FAT_ERR_TOO_LONG;
    }
    if (ext_len > 3 || ext_len == 0)
        return FAT_ERR_TOO_LONG;

    // Проверка допустимых символов в имени и расширении (A-Z, 0-9, _)
    for (int i = 0; i < name_len; i++)
    {
        char c = name[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
            return FAT_ERR_INVALID_CHAR;
    }
    for (int i = 0; i < ext_len; i++)
    {
        char c = dot[1 + i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
            return FAT_ERR_INVALID_CHAR;
    }

    return 0;
}


int validate_fat_lfn_file(const char *name)
{
    if (!name || !*name)
        return 0;

    const char *dot = strchr(name, '.');

    int name_len = (dot ? (dot - name) : strlen(name));
    int ext_len = (dot ? strlen(dot + 1) : 0);

    if (dot && strchr(dot + 1, '.'))
        return -1;

    if (ext_len > 5 || ext_len == 0)
        return -2;

    // Проверка допустимых символов в имени и расширении (A-Z, a-z, 0-9, _, -)
    for (int i = 0; i < name_len; i++)
    {
        char c = name[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return -3;
    }
    for (int i = 0; i < ext_len; i++)
    {
        char c = dot[1 + i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'))
            return -4;
    }

    return 0;
}

int validate_fat_sfn_dir(const char *name) {
    if (!name || !*name) return 0;

    if (strchr(name, '.')) return -1; // Директория SFN не должна содержать точек

    int len = strlen(name);
    if (len == 0 || len > 8) return -2;

    for (int i = 0; i < len; i++) {
        char c = name[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
            return -3;
    }

    return 0;
}

int validate_fat_lfn_dir(const char *name) {
    if (!name || !*name) return 0;

    if (strchr(name, '.')) return -1; // В имени папки LFN не должно быть точки

    int len = strlen(name);
    if (len == 0 || len > 255) return -2; // FAT ограничивает длину LFN до 255

    for (int i = 0; i < len; i++) {
        char c = name[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return -3;
    }

    return 0;
}