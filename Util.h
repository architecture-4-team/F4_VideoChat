#pragma once
#include <tchar.h>
#include <string>
#include <stdio.h>
#include <Windows.h>

class Util
{
public:
	Util();
	~Util();
	std::string WideStringToString(const std::wstring& wideStr);
	std::string LoadFile(TCHAR path);
	void SetStringToTCharBuffer(const std::string& str, TCHAR* buffer, int bufferSize);
private:

};

