// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#pragma once
// macOS paths and preset labels are UTF-8, independent of the process C locale.
#include <codecvt>
#include <locale>
#include <string>
#include <cstring>
#include <cwchar>
namespace FxUtf8 {
inline bool encode(const wchar_t* input, std::string& output) noexcept {
    if (!input) return false;
    try { output = std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(input); return true; }
    catch (...) { return false; }
}
inline bool decode(const char* input, std::wstring& output) noexcept {
    if (!input) return false;
    try { output = std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.from_bytes(input); return true; }
    catch (...) { return false; }
}
}
