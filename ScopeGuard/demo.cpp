#include "ScopeGuard.h"
#include <stdio.h>
#include <string>
#include <iostream>

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

int main(int agrc, char** agrv)
{
	ScopeGuardTest1();
	ScopeGuardTest2();
	getchar();
	return 0;
}