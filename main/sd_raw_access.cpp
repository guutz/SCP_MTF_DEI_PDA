#include "sd_raw_access.h"
#include <string.h>
#include <sys/stat.h> // For stat() in sd_raw_file_exists
#include <errno.h> // Required for errno
#include "esp_log.h"
#include "sd_manager.h"

// Expose the mutex for SD operations
extern SemaphoreHandle_t s_sd_mutex;

static const char* s_raw_mount_point = MOUNT_POINT;
static const char* TAG_RAW_SD = "sd_raw_access";

#define MAX_FULL_PATH_LEN 256 // Max length for full path including mount point

static void build_full_path(const char* path_suffix, char* full_path_out, size_t max_len) {
    snprintf(full_path_out, max_len, "%s/%s", s_raw_mount_point, path_suffix);
}

FILE* sd_raw_fopen(const char* path_suffix, const char* mode) {
    if (s_sd_mutex == NULL || s_raw_mount_point == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Raw SD access not initialized (fopen)");
        return NULL;
    }
    if (path_suffix == NULL || mode == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Path suffix or mode is NULL for fopen");
        return NULL;
    }

    char full_path[MAX_FULL_PATH_LEN];
    build_full_path(path_suffix, full_path, sizeof(full_path));

    FILE* fp = NULL;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(30000)) == pdTRUE) {
        fp = fopen(full_path, mode);
        if (fp == NULL) {
            ESP_LOGE(TAG_RAW_SD, "Failed to open file: %s, mode: %s (errno %d: %s)", full_path, mode, errno, strerror(errno));
        }
        xSemaphoreGive(s_sd_mutex);
    } else {
        ESP_LOGE(TAG_RAW_SD, "fopen: Mutex timeout for %s", full_path);
    }
    return fp;
}

size_t sd_raw_fread(void* ptr, size_t size, size_t count, FILE* stream) {
    if (ptr == NULL || stream == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Invalid parameters for fread");
        return 0;
    }

    if (s_sd_mutex == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Raw SD access not initialized (fread)");
        return 0;
    }

    size_t bytes_read = 0;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        bytes_read = fread(ptr, size, count, stream);
        if (bytes_read != count) {
            if (feof(stream)) {
                ESP_LOGD(TAG_RAW_SD, "End of file reached");
            } else if (ferror(stream)) {
                ESP_LOGE(TAG_RAW_SD, "Error reading file (errno %d: %s)", errno, strerror(errno));
            }
        }
        xSemaphoreGive(s_sd_mutex);
    } else {
        ESP_LOGE(TAG_RAW_SD, "fread: Mutex timeout");
    }
    return bytes_read;
}

size_t sd_raw_fwrite(const void* ptr, size_t size, size_t count, FILE* stream) {
    if (ptr == NULL || stream == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Invalid parameters for fwrite");
        return 0;
    }

    if (s_sd_mutex == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Raw SD access not initialized (fwrite)");
        return 0;
    }

    size_t bytes_written = 0;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        bytes_written = fwrite(ptr, size, count, stream);
        if (bytes_written != count) {
            ESP_LOGE(TAG_RAW_SD, "Failed to write all data (errno %d: %s)", errno, strerror(errno));
        }
        xSemaphoreGive(s_sd_mutex);
    } else {
        ESP_LOGE(TAG_RAW_SD, "fwrite: Mutex timeout");
    }
    return bytes_written;
}

int sd_raw_fseek(FILE* stream, long offset, int whence) {
    if (stream == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Invalid parameters for fseek");
        return -1;
    }

    if (s_sd_mutex == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Raw SD access not initialized (fseek)");
        return -1;
    }

    int result = -1;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        result = fseek(stream, offset, whence);
        if (result != 0) {
            ESP_LOGE(TAG_RAW_SD, "Failed to seek (errno %d: %s)", errno, strerror(errno));
        }
        xSemaphoreGive(s_sd_mutex);
    } else {
        ESP_LOGE(TAG_RAW_SD, "fseek: Mutex timeout");
    }
    return result;
}

long sd_raw_ftell(FILE* stream) {
    if (stream == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Invalid parameters for ftell");
        return -1L;
    }

    if (s_sd_mutex == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Raw SD access not initialized (ftell)");
        return -1L;
    }

    long position = -1L;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        position = ftell(stream);
        if (position < 0) {
            ESP_LOGE(TAG_RAW_SD, "Failed to get file position (errno %d: %s)", errno, strerror(errno));
        }
        xSemaphoreGive(s_sd_mutex);
    } else {
        ESP_LOGE(TAG_RAW_SD, "ftell: Mutex timeout");
    }
    return position;
}

int sd_raw_fclose(FILE* stream) {
    if (stream == NULL) {
        return 0; // Not an error to close NULL
    }

    if (s_sd_mutex == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Raw SD access not initialized (fclose)");
        return EOF;
    }

    int result = EOF;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        result = fclose(stream);
        if (result != 0) {
            ESP_LOGE(TAG_RAW_SD, "Failed to close file (errno %d: %s)", errno, strerror(errno));
        }
        xSemaphoreGive(s_sd_mutex);
    } else {
        ESP_LOGE(TAG_RAW_SD, "fclose: Mutex timeout");
    }
    return result;
}

int sd_raw_remove(const char* path_suffix) {
    if (path_suffix == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Path suffix is NULL for remove");
        return -1;
    }

    if (s_sd_mutex == NULL || s_raw_mount_point == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Raw SD access not initialized (remove)");
        return -1;
    }

    char full_path[MAX_FULL_PATH_LEN];
    build_full_path(path_suffix, full_path, sizeof(full_path));

    int result = -1;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        result = remove(full_path);
        if (result != 0) {
            ESP_LOGE(TAG_RAW_SD, "Failed to remove file: %s (errno %d: %s)", full_path, errno, strerror(errno));
        }
        xSemaphoreGive(s_sd_mutex);
    } else {
        ESP_LOGE(TAG_RAW_SD, "remove: Mutex timeout for %s", full_path);
    }
    return result;
}

int sd_raw_rename(const char* old_path_suffix, const char* new_path_suffix) {
    if (old_path_suffix == NULL || new_path_suffix == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Path suffix is NULL for rename");
        return -1;
    }

    if (s_sd_mutex == NULL || s_raw_mount_point == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Raw SD access not initialized (rename)");
        return -1;
    }

    char old_full_path[MAX_FULL_PATH_LEN];
    char new_full_path[MAX_FULL_PATH_LEN];
    build_full_path(old_path_suffix, old_full_path, sizeof(old_full_path));
    build_full_path(new_path_suffix, new_full_path, sizeof(new_full_path));

    int result = -1;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        result = rename(old_full_path, new_full_path);
        if (result != 0) {
            ESP_LOGE(TAG_RAW_SD, "Failed to rename file: %s to %s (errno %d: %s)", 
                    old_full_path, new_full_path, errno, strerror(errno));
        }
        xSemaphoreGive(s_sd_mutex);
    } else {
        ESP_LOGE(TAG_RAW_SD, "rename: Mutex timeout for %s", old_full_path);
    }
    return result;
}

bool sd_raw_file_exists(const char* path_suffix) {
    if (path_suffix == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Path suffix is NULL for file_exists");
        return false;
    }

    if (s_sd_mutex == NULL || s_raw_mount_point == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Raw SD access not initialized (file_exists)");
        return false;
    }

    char full_path[MAX_FULL_PATH_LEN];
    build_full_path(path_suffix, full_path, sizeof(full_path));

    bool exists = false;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        struct stat st;
        exists = (stat(full_path, &st) == 0);
        xSemaphoreGive(s_sd_mutex);
    } else {
        ESP_LOGE(TAG_RAW_SD, "file_exists: Mutex timeout for %s", full_path);
    }
    return exists;
}

long sd_raw_get_file_size(const char* path_suffix) {
    if (path_suffix == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Path suffix is NULL for get_file_size");
        return -1L;
    }

    if (s_sd_mutex == NULL || s_raw_mount_point == NULL) {
        ESP_LOGE(TAG_RAW_SD, "Raw SD access not initialized (get_file_size)");
        return -1L;
    }

    char full_path[MAX_FULL_PATH_LEN];
    build_full_path(path_suffix, full_path, sizeof(full_path));

    long size = -1L;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        struct stat st;
        if (stat(full_path, &st) == 0) {
            size = st.st_size;
        } else {
            ESP_LOGE(TAG_RAW_SD, "Failed to get file size: %s (errno %d: %s)", full_path, errno, strerror(errno));
        }
        xSemaphoreGive(s_sd_mutex);
    } else {
        ESP_LOGE(TAG_RAW_SD, "get_file_size: Mutex timeout for %s", full_path);
    }
    return size;
}
