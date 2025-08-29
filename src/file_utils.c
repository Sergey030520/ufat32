#include "fat32/file_utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "fat32/fat32_alloc.h"
#include "fat32/log_fat32.h"


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

int validate_fat_sfn_dir(const char *name)
{
    if (!name || !*name)
        return 0;

    if (strchr(name, '.'))
        return -1; // Директория SFN не должна содержать точек

    int len = strlen(name);
    if (len == 0 || len > 8)
        return -2;

    for (int i = 0; i < len; i++)
    {
        char c = name[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
            return -3;
    }

    return 0;
}

int validate_fat_lfn_dir(const char *name)
{
    if (!name || !*name)
        return 0;

    if (strchr(name, '.'))
        return -1; // В имени папки LFN не должно быть точки

    int len = strlen(name);
    if (len == 0 || len > 255)
        return -2; // FAT ограничивает длину LFN до 255

    for (int i = 0; i < len; i++)
    {
        char c = name[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return -3;
    }

    return 0;
}

int fat32_path_depth(const char *path)
{
    if (path == NULL)
        return FAT32_ERR_INVALID_ARGUMENT;
    uint32_t len = strlen(path);
    char *buff = fat32_alloc(len + 1);
    if (buff == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }
    memcpy(buff, path, len + 1);
    char *token = strtok(buff, "/");
    int depth = 0;
    while (token != NULL)
    {
        ++depth;
        token = strtok(NULL, "/");
    }
    if (fat32_free(buff, len + 1) != 0)
    {
        FAT32_LOG_ERROR("Failed to free memory region at %p, size %d", (void *)buff, len + 1);
    }
    return depth;
}


int fat32_parse_path(const char *path, char (*name_files)[MAX_NAME_SIZE])
{
    if (path == NULL || name_files == NULL)
        return FAT32_ERR_INVALID_ARGUMENT;

    int status = 0;
    const size_t path_len = strlen(path);
    char *buff = fat32_alloc(path_len + 1);
    if (buff == NULL)
    {
        status = FAT32_ERR_ALLOC_FAILED;
        goto cleanup;
    }

    memcpy(buff, path, path_len);
    buff[path_len] = '\0';

    char *token = strtok(buff, "/");
    int idxWord = 0;
    size_t token_len = 0;

    while (token != NULL)
    {
        token_len = strlen(token);
        if (token_len >= MAX_NAME_SIZE)
        {
            status = FAT32_ERR_NAME_TOO_LONG;
            goto cleanup;
        }
        memcpy(name_files[idxWord++], token, token_len + 1);
        token = strtok(NULL, "/");
    }

cleanup:
    if (buff != NULL)
    {
        if (fat32_free(buff, path_len + 1) != 0)
        {
            FAT32_LOG_ERROR("Failed to free path buffer at %p", (void *)buff);
        }
    }

    return (status == 0) ? idxWord : status;
}

char *fat32_find_last_char(char *text, char symbol)
{
    if (text == NULL)
    {
        return NULL;
    }

    char *last = NULL;
    for (char *p = text; *p != '\0'; ++p)
    {
        if (*p == symbol)
        {
            last = p;
        }
    }

    return last;
}

void fat32_format_sfn(const char *input, uint16_t length, char output[11])
{
    if (input == NULL || output == NULL)
    {
        return;
    }
    int idx = 0;
    for (idx = 0; idx < 11; ++idx)
        output[idx] = ' ';

    for (idx = 0; idx < 8 && input[idx] != '.' && idx < length; ++idx)
    {
        output[idx] = toupper(input[idx]);
    }
    if (input[idx] == '.' && idx < length)
    {
        for (int j = 8; j < 11; ++j)
        {
            output[j] = toupper(input[++idx]);
        }
    }
}

int fat32_is_special_dir(const uint8_t dir_name[11])
{
    if (dir_name[0] == '.' && dir_name[1] == ' ')
        return 0;

    if (dir_name[0] == '.' && dir_name[1] == '.')
        return 0;
    return -1;
}

int fat32_compare_lfn(const char *name_ascii, const uint16_t *name_unicode)
{
    uint8_t buffer[MAX_NAME_SIZE];
    memset(buffer, 0, MAX_NAME_SIZE);
    fat32_utf16le_to_ascii(name_unicode, (char *)buffer);
    uint32_t length = strlen(name_ascii);
    if (length != strlen((const char *)buffer))
    {
        return -1;
    }

    for (int idx = 0; idx < length; ++idx)
    {
        if (name_ascii[idx] != buffer[idx])
        {
            return -1;
        }
    }
    return 0;
}

void fat32_generate_sfn_from_lfn(const char *name, uint32_t length, uint8_t buffer[11])
{
    if (name == NULL || buffer == NULL)
    {
        return;
    }

    memset(buffer, ' ', SHORT_NAME_SIZE);
    char *dot = fat32_find_last_char(name, '.');

    uint16_t ext_len = (dot != NULL ? (name + length) - dot : 0);
    uint16_t base_len = (dot != NULL ? (length - (dot - name + 1)) : 0);
    uint16_t idx = 0;

    for (idx = 0; idx < LFN_SHORT_NAME && idx < base_len; ++idx)
    {
        if (!(isalpha(buffer[idx]) || isdigit(buffer[idx])))
            buffer[idx] = toupper(name[idx]);
    }
    buffer[6] = '~';
    buffer[7] = '1';
    if (dot != NULL && ext_len > 0)
    {
        for (idx = 0; idx < 3 && idx < ext_len; ++idx)
        {
            buffer[8 + idx] = toupper(dot[1 + idx]);
        }
    }
}

uint8_t fat32_sfn_checksum(const uint8_t *pFcbName)
{
    short FcbNameLen;
    uint8_t Sum;
    Sum = 0;
    for (FcbNameLen = 11; FcbNameLen != 0; FcbNameLen--)
    {
        // NOTE: The operation is an unsigned char rotate right
        Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *pFcbName++;
    }
    return (Sum);
}


void fat32_ascii_to_utf16le(const char *ascii_str, uint8_t *utf16le_buf)
{
    while (*ascii_str)
    {
        *utf16le_buf++ = (uint8_t)(*ascii_str++);
        *utf16le_buf++ = 0x00;
    };

    *utf16le_buf++ = 0x00;
    *utf16le_buf++ = 0x00;
}

void fat32_utf16le_to_ascii(const uint16_t *utf16le_buf, char *ascii_str)
{
    while (*utf16le_buf != 0x0000)
    {
        *ascii_str++ = (char)(*utf16le_buf & 0xFF);
        utf16le_buf++;
    }
    *ascii_str = '\0';
}
