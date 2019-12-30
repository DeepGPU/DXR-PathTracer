#pragma once
#include <stdio.h>

inline void printError(const char* errorMessage)
{
	if(errorMessage)
		puts(errorMessage);
}

class Error
{
public:
	Error() {}
	Error(const char* errorMessage)
	{
		printError(errorMessage);
	}
};

#define DECLARE_ERROR(error_class) \
class error_class : public Error \
{\
public:\
error_class() {} \
error_class(const char* message) : Error(message) {}\
};

