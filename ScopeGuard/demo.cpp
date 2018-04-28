#include "ScopeGuard.h"
#include <stdio.h>
#include <string>
#include <string.h>
#include <memory>
#include <iostream>
#include <Windows.h>   // IsBadReadPtr

class DataBuf
{
public:
	DataBuf() :buff_(NULL)
	{
		buff_ = new char[1000];
		strcpy_s(buff_, 1000 - 1, "A String Buffer.");
	}
	void DelData()
	{
		if (buff_)
		{
			delete[] buff_;
			buff_ = NULL;
			std::cout << __FUNCTION__ << std::endl;
		}
	}
	~DataBuf()
	{
		std::cout << __FUNCTION__ << std::endl;
	}
private:
	char *buff_;
};

void ScopeGuardTest1()
{
	char* pData = new char[1000];
	SCOPE_EXIT{
		if (pData)
		delete[] pData;
	};
	DataBuf * pNew = new DataBuf;
	SCOPE_EXIT{
		if (pNew)
		{
			pNew->DelData();
			delete pNew;
		}
	};
	// 模拟一些错误导致的返回
	std::string  str;
	if (str.empty()) {
		return;
	}
	// 正常的删除代码
	if (pData)
	{
		delete[] pData; pData = NULL;
	}

	if (pNew)
	{
		pNew->DelData();
		delete pNew;
		pNew = NULL;
	}
}

void ScopeGuardTest2()
{
	DataBuf * pNew = new DataBuf;
	auto guard = watchman::makeGuard([&] {
		if (pNew)
		{
			pNew->DelData();
			delete pNew;
			pNew = NULL;
		}
	});

	// 模拟一些错误导致的返回
	std::string  str;
	if (str.empty()) {
		return;
	}

	if (pNew)
	{
		pNew->DelData();
		delete pNew;
		pNew = NULL;
	}
}

void std_unique_Test1()
{
	char* pStr = NULL;
	{
		std::unique_ptr<char[]> pStr_unique_ptr = std::unique_ptr<char[]>(new char[1000]);
		pStr = pStr_unique_ptr.get();
		memset(pStr_unique_ptr.get(), 0, 1000);
		const char* pStr1 = "ABC123北京abc";
		memcpy(pStr_unique_ptr.get(), pStr1, strlen(pStr1));
		printf("%s\n", pStr_unique_ptr.get());
	}
	bool bMemoryOk = IsBadReadPtr(pStr, 1000);
	if (bMemoryOk)
	{
		printf("Ok,Memory No Free\n");
	}
	else
	{
		printf("BadPtr,Memory Is Free\n");
	}
}

void std_unique_Test2()
{
	DataBuf* pData = NULL;
	{
		std::unique_ptr<DataBuf[]> pData_Unique = std::unique_ptr<DataBuf[]>(new DataBuf[2]);
		pData = pData_Unique.get();
	}
	bool bMemoryOk = IsBadReadPtr(pData, sizeof(DataBuf));
	if (bMemoryOk)
	{
		printf("Ok,Class Memory No Free\n");
	}
	else
	{
		printf("BadPtr,Class Memory Is Free\n");
	}
}

int main(int agrc, char** agrv)
{
	std_unique_Test1();
	std_unique_Test2();


	ScopeGuardTest1();
	ScopeGuardTest2();
	getchar();
	return 0;
}