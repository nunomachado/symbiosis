#include "klee/ClapUtil.h"

std::string tname;

std::string getThreadName()
{
	return tname;
}
void setThreadName(std::string name)
{
	tname = name;
}
