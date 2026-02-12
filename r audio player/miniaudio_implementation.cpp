#define MINIAUDIO_IMPLEMENTATION

#ifdef _WIN32
#define MA_WIN32_USE_WCHAR
#endif

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#undef STB_VORBIS_HEADER_ONLY

#include "miniaudio.h"

#include "stb_vorbis.c"