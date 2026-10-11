#define STB_IMAGE_IMPLEMENTATION
#ifdef _WIN32
#define STBI_WINDOWS_UTF8
#endif
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#ifdef _WIN32
#define STBIW_WINDOWS_UTF8
#endif
#include "stb_image_write.h"
