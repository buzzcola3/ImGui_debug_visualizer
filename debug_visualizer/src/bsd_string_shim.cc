#include <cstring>

// Provide BSD strlcpy for dependencies (e.g., libx11) on glibc versions lacking it.
extern "C" size_t strlcpy(char *dst, const char *src, size_t size)
{
    if (size == 0)
    {
        return std::strlen(src);
    }

    size_t i = 0;
    for (; i + 1 < size && src[i] != '\0'; ++i)
    {
        dst[i] = src[i];
    }

    dst[i] = '\0';
    return i + std::strlen(src + i);
}
