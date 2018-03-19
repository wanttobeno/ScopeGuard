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

void ScopeGuardTest()
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
	// 故意返回
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

int main(int agrc, char** agrv)
{
	ScopeGuardTest();
	getchar();
	return 0;
}