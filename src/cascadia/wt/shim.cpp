#include <string>
#include <filesystem>
#include <vector>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <wil/stl.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>

static std::vector<std::wstring> _commandlineToArgs(const wchar_t* commandLine)
{
    int argc = 0;
    const wil::unique_hlocal_ptr<LPWSTR> argv{ CommandLineToArgvW(commandLine, &argc) };
    if (argc < 0)
    {
        argc = 0;
    }

    std::vector<std::wstring> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i)
    {
        args.emplace_back(argv.get()[i]);
    }
    return args;
}

static bool _isQueryThatNeedsStdHandles(const std::vector<std::wstring>& args) noexcept
{
    return args.size() >= 1 && _wcsicmp(args[0].c_str(), L"list-windows") == 0;
}

#pragma warning(suppress : 26461) // we can't change the signature of wWinMain
int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR pCmdLine, int)
{
    std::filesystem::path module{ wil::GetModuleFileNameW<std::wstring>(nullptr) };
    const auto args = _commandlineToArgs(pCmdLine);
    const auto forwardStdHandles = _isQueryThatNeedsStdHandles(args);

    // Cache our name (wt, wtd)
    std::wstring ourFilename{ module.filename() };

    // Swap wt[d].exe for WindowsTerminal.exe
    module.replace_filename(L"WindowsTerminal.exe");

    // Append the rest of the commandline to the saved name
    std::wstring cmdline;
    if (FAILED(wil::str_printf_nothrow(cmdline, L"%s %s", ourFilename.c_str(), pCmdLine)))
    {
        return 1;
    }

    // Get our startup info so it can be forwarded
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    GetStartupInfoW(&si);
    if (forwardStdHandles)
    {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }

    // Go!
    wil::unique_process_information pi;
    return !CreateProcessW(module.c_str(), cmdline.data(), nullptr, nullptr, forwardStdHandles, 0, nullptr, nullptr, &si, &pi);
}
