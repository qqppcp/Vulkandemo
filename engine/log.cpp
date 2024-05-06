#include "log.h"
#include "windows.h"
void log(LogLevel level, std::string message)
{
	const char* level_strings[6] = { "[FATAL]:\t", "[ERROR]:\t", "[WARN]:\t", "[DEBUG]:\t", "[INFO]:\t" };
	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	// FATAL,ERROR,WARN,DEBUG,INFO
	static uint8_t colors[5] = { 64, 4, 6, 1, 2 };
	SetConsoleTextAttribute(console_handle, colors[(int)level]);
	auto outstr = level_strings[(int)level] + message + "\n";
	OutputDebugStringA(outstr.c_str());
	uint64_t length = outstr.size();
	LPDWORD number_written = 0;
	WriteConsoleA(console_handle, outstr.c_str(), (DWORD)length, number_written, 0);
}
