#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <AftString/aft_string.h>

typedef enum AftOpenFileMode
{
    AFT_OPEN_FILE_MODE_READ,
    AFT_OPEN_FILE_MODE_WRITE,
} AftOpenFileMode;


typedef struct AftFile AftFile;

typedef struct AftPath
{
    AftString string;
} AftPath;

typedef struct AftMaybePath
{
    AftPath value;
    bool valid;
} AftMaybePath;

typedef struct AftReadFileResult
{
    uint64_t bytes;
    bool success;
} AftReadFileResult;


void aft_close_file(AftFile* file);
AftMaybeUint64 aft_get_file_size(AftFile* file);
AftMaybeString aft_load_text_file(const AftPath* path, void* allocator);
AftFile* aft_open_file(const AftPath* path, AftOpenFileMode mode, void* allocator);
AftReadFileResult aft_read_file_sync(AftFile* file, void* data, uint64_t bytes);
bool aft_write_file_sync(AftFile* file, const void* data, uint64_t bytes);
void aft_write_to_error_output(AftStringSlice slice);
void aft_write_to_standard_output(AftStringSlice slice);

AftMaybePath aft_get_executable_directory(void* allocator);

bool aft_path_append(AftPath* path, const AftPath* after);
void aft_path_destroy(AftPath* path);
AftMaybePath aft_path_from_c_string_with_allocator(const char* path, void* allocator);
AftMaybePath aft_path_from_slice_with_allocator(AftStringSlice path, void* allocator);
const char* aft_path_get_contents_const(const AftPath* path);
const AftString* aft_path_get_string_const(const AftPath* path);
void aft_path_initialise_with_allocator(AftPath* path, void* allocator);
void aft_path_remove_basename(AftPath* path);

#endif // FILESYSTEM_H_
