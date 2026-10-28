// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
// macOS UTF-8 string bridge. Modified 2026-10-28, AGPL-3.0-or-later.
#if defined(__APPLE__)
#include <cstdlib>
#include "Utf8Paths.h"
#include "codedefs.h"
#include "slout.h"
#include "pstr.h"

int PT_DECLSPEC pstrConvertToWideCharString(char* input, wchar_t* output, int capacity)
{
    if (!output || capacity <= 0) return NOT_OKAY;
    output[0] = 0;
    std::wstring text;
    if (!FxUtf8::decode(input, text) || text.size() >= (size_t)capacity) return NOT_OKAY;
    std::wmemcpy(output, text.c_str(), text.size() + 1);
    return OKAY;
}
int PT_DECLSPEC pstrConvertToWideCharString_WithAlloc(char* input, wchar_t** output, int* capacity)
{
    if (!output || !capacity) return NOT_OKAY;
    *output = nullptr; *capacity = 0;
    std::wstring text;
    if (!FxUtf8::decode(input, text)) return NOT_OKAY;
    auto* result = (wchar_t*)std::calloc(text.size() + 1, sizeof(wchar_t));
    if (!result) return NOT_OKAY;
    std::wmemcpy(result, text.c_str(), text.size() + 1);
    *output = result; *capacity = (int)text.size() + 1;
    return OKAY;
}
int PT_DECLSPEC pstrConvertWideCharStringToAnsiCharString(wchar_t* input, char* output, int capacity)
{
    if (!output || capacity <= 0) return NOT_OKAY;
    output[0] = 0;
    std::string text;
    if (!FxUtf8::encode(input, text) || text.size() >= (size_t)capacity) return NOT_OKAY;
    std::memcpy(output, text.c_str(), text.size() + 1);
    return OKAY;
}
int PT_DECLSPEC pstrCovertWideCharStringToUTF8String_WithAlloc(wchar_t* input, char** output, int* bytes)
{
    if (!output || !bytes) return NOT_OKAY;
    *output = nullptr; *bytes = 0;
    std::string text;
    if (!FxUtf8::encode(input, text)) return NOT_OKAY;
    auto* result = (char*)std::calloc(text.size() + 1, 1);
    if (!result) return NOT_OKAY;
    std::memcpy(result, text.c_str(), text.size() + 1);
    *output = result; *bytes = (int)text.size() + 1;
    return OKAY;
}
int PT_DECLSPEC pstrCovertUTF8StringToWideCharString_WithAlloc(char* input, wchar_t** output, int* count)
{
    return pstrConvertToWideCharString_WithAlloc(input, output, count);
}
#endif
