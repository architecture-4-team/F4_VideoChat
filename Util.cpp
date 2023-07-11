#include "Util.h"

Util& Util::GetInstance() {
    static Util utilInstance;
    return utilInstance;
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

std::string Util::LoadFile(TCHAR* path)
{
    std::string content;

    // 파일 열기
    HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        // 파일 열기 실패
        return "";
    }

    // 파일 크기 가져오기
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE)
    {
        // 파일 크기 가져오기 실패
        CloseHandle(hFile);
        return "";
    }

    // 파일 내용 읽기
    DWORD bytesRead;
    std::string buffer(fileSize, '\0');
    if (!ReadFile(hFile, &buffer[0], fileSize, &bytesRead, NULL) || bytesRead != fileSize)
    {
        // 파일 읽기 실패
        CloseHandle(hFile);
        return "";
    }

    CloseHandle(hFile);

    // null 문자 제거
    buffer.pop_back();

    return buffer;
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