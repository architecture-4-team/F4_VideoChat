#include "Util.h"

Util::Util() {
}

std::string Util::WideStringToString(const std::wstring & wideStr)
{
	int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
	if (bufferSize == 0)
	{
		return "";
	}

	std::string str(bufferSize, 0);
	WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &str[0], bufferSize, NULL, NULL);

	str.pop_back();

	return str;
}

std::string Util::LoadFile(TCHAR path)
{

	return std::string ("");
}

void Util::SetStringToTCharBuffer(const std::string& str, TCHAR* buffer, int bufferSize)
{
#ifdef UNICODE
	// Convert std::string to std::wstring
	std::wstring wideStr(str.begin(), str.end());

	// Copy std::wstring to TCHAR buffer
	wcsncpy_s(buffer, bufferSize, wideStr.c_str(), _TRUNCATE);
#else
	// Copy std::string to TCHAR buffer
	strncpy_s(buffer, bufferSize, str.c_str(), _TRUNCATE);
#endif
}