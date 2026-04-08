#include "crash_handler.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <new>
#include <string>

#include <Windows.h>
#include <DbgHelp.h>
#include <crtdbg.h>

#include "path/app_paths.hpp"

#pragma comment(lib, "Dbghelp.lib")

namespace CrashHandler {
namespace {

constexpr UINT kCrashExitCode = 0xC0000409u;

bool g_installed = false;
bool g_dumpInProgress = false;
std::filesystem::path g_crashDirectory;

std::string formatTimestampForFile() {
  SYSTEMTIME localTime{};
  GetLocalTime(&localTime);

  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "%04u%02u%02u_%02u%02u%02u_%03u",
                static_cast<unsigned>(localTime.wYear),
                static_cast<unsigned>(localTime.wMonth),
                static_cast<unsigned>(localTime.wDay),
                static_cast<unsigned>(localTime.wHour),
                static_cast<unsigned>(localTime.wMinute),
                static_cast<unsigned>(localTime.wSecond),
                static_cast<unsigned>(localTime.wMilliseconds));
  return buffer;
}

std::string formatTimestampForLog() {
  std::time_t now = std::time(nullptr);
  std::tm localTime{};
  localtime_s(&localTime, &now);

  char buffer[64];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
  return buffer;
}

std::string narrowPath(const std::filesystem::path& path) {
  return path.u8string();
}

void printLine(const std::string& message) {
  std::printf("%s\n", message.c_str());
  std::fflush(stdout);
  OutputDebugStringA((message + "\n").c_str());
}

void terminateImmediately(UINT exitCode) {
  TerminateProcess(GetCurrentProcess(), exitCode);
}

void ensureCrashDirectory() {
  if (!g_crashDirectory.empty()) {
    return;
  }

  AppPaths::initialize();
  g_crashDirectory = AppPaths::savesRoot().parent_path() / "Crashes";

  std::error_code ec;
  std::filesystem::create_directories(g_crashDirectory, ec);
}

void writeMetadataFile(const std::filesystem::path& logPath,
                       const std::filesystem::path& dumpPath,
                       const char* reason,
                       DWORD threadId,
                       DWORD exceptionCode,
                       bool dumpWritten,
                       DWORD dumpErrorCode) {
  FILE* file = nullptr;
  if (_wfopen_s(&file, logPath.wstring().c_str(), L"wb") != 0 || file == nullptr) {
    return;
  }

  std::fprintf(file, "Timestamp: %s\n", formatTimestampForLog().c_str());
  std::fprintf(file, "Reason: %s\n", reason != nullptr ? reason : "Unknown");
  std::fprintf(file, "ProcessId: %lu\n", static_cast<unsigned long>(GetCurrentProcessId()));
  std::fprintf(file, "ThreadId: %lu\n", static_cast<unsigned long>(threadId));
  std::fprintf(file, "ExceptionCode: 0x%08lX\n",
               static_cast<unsigned long>(exceptionCode));
  std::fprintf(file, "Executable: %s\n",
               narrowPath(AppPaths::executablePath()).c_str());
  std::fprintf(file, "WorkingDirectory: %s\n",
               narrowPath(std::filesystem::current_path()).c_str());
  std::fprintf(file, "DumpFile: %s\n", narrowPath(dumpPath).c_str());
  std::fprintf(file, "DumpWritten: %s\n", dumpWritten ? "true" : "false");
  std::fprintf(file, "DumpWriteLastError: %lu\n",
               static_cast<unsigned long>(dumpErrorCode));

  std::fclose(file);
}

void writeDump(const char* reason,
               EXCEPTION_POINTERS* exceptionPointers,
               DWORD exceptionCode) {
  if (g_dumpInProgress) {
    terminateImmediately(kCrashExitCode);
  }

  g_dumpInProgress = true;
  ensureCrashDirectory();

  const std::string stamp = formatTimestampForFile();
  const std::string baseName =
      "VoxelParadox_" + stamp + "_" + std::to_string(GetCurrentProcessId());
  const std::filesystem::path dumpPath = g_crashDirectory / (baseName + ".dmp");
  const std::filesystem::path logPath = g_crashDirectory / (baseName + ".log");

  HANDLE dumpFile =
      CreateFileW(dumpPath.wstring().c_str(), GENERIC_WRITE, 0, nullptr,
                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  bool dumpWritten = false;
  DWORD dumpErrorCode = ERROR_SUCCESS;
  if (dumpFile != INVALID_HANDLE_VALUE) {
    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo{};
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = exceptionPointers;
    exceptionInfo.ClientPointers = FALSE;

    const MINIDUMP_TYPE dumpTypes[] = {
        static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithThreadInfo),
        MiniDumpNormal,
    };

    for (const MINIDUMP_TYPE dumpType : dumpTypes) {
      dumpWritten = MiniDumpWriteDump(
          GetCurrentProcess(), GetCurrentProcessId(), dumpFile, dumpType,
          exceptionPointers != nullptr ? &exceptionInfo : nullptr, nullptr,
          nullptr) == TRUE;
      if (dumpWritten) {
        dumpErrorCode = ERROR_SUCCESS;
        break;
      }

      dumpErrorCode = GetLastError();
      SetFilePointer(dumpFile, 0, nullptr, FILE_BEGIN);
      SetEndOfFile(dumpFile);
    }

    CloseHandle(dumpFile);
  } else {
    dumpErrorCode = GetLastError();
  }

  writeMetadataFile(logPath, dumpPath, reason, GetCurrentThreadId(),
                    exceptionCode, dumpWritten, dumpErrorCode);

  if (dumpWritten) {
    printLine("[CrashHandler] Dump written to: " + narrowPath(dumpPath));
  } else {
    printLine("[CrashHandler] Failed to write dump file. GetLastError=" +
              std::to_string(dumpErrorCode));
  }

  printLine("[CrashHandler] Crash log written to: " + narrowPath(logPath));
}

LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers) {
  DWORD exceptionCode = 0;
  if (exceptionPointers != nullptr && exceptionPointers->ExceptionRecord != nullptr) {
    exceptionCode = exceptionPointers->ExceptionRecord->ExceptionCode;
  }

  writeDump("Unhandled exception", exceptionPointers, exceptionCode);
  return EXCEPTION_CONTINUE_SEARCH;
}

void __cdecl terminateHandler() {
  writeDump("std::terminate", nullptr, kCrashExitCode);
  terminateImmediately(kCrashExitCode);
}

void __cdecl pureCallHandler() {
  writeDump("Pure virtual function call", nullptr, kCrashExitCode);
  terminateImmediately(kCrashExitCode);
}

void __cdecl invalidParameterHandler(const wchar_t*,
                                     const wchar_t*,
                                     const wchar_t*,
                                     unsigned int,
                                     uintptr_t) {
  writeDump("CRT invalid parameter", nullptr, kCrashExitCode);
  terminateImmediately(kCrashExitCode);
}

void __cdecl signalHandler(int signalCode) {
  const char* reason = "CRT signal";
  switch (signalCode) {
  case SIGABRT:
    reason = "SIGABRT";
    break;
  case SIGFPE:
    reason = "SIGFPE";
    break;
  case SIGILL:
    reason = "SIGILL";
    break;
  case SIGSEGV:
    reason = "SIGSEGV";
    break;
  default:
    break;
  }

  writeDump(reason, nullptr, kCrashExitCode);
  terminateImmediately(kCrashExitCode);
}

} // namespace

void install() {
  if (g_installed) {
    return;
  }

  ensureCrashDirectory();

  SetUnhandledExceptionFilter(unhandledExceptionFilter);
  std::set_terminate(terminateHandler);
  _set_purecall_handler(pureCallHandler);
  _set_invalid_parameter_handler(invalidParameterHandler);

  std::signal(SIGABRT, signalHandler);
  std::signal(SIGFPE, signalHandler);
  std::signal(SIGILL, signalHandler);
  std::signal(SIGSEGV, signalHandler);

  g_installed = true;
  printLine("[CrashHandler] Installed. Dump directory: " +
            narrowPath(g_crashDirectory));
}

} // namespace CrashHandler
