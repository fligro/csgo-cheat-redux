#pragma once
#include <fstream>


/*
Usage: Logger("This is log message"); Writes a file (or appends existing file)

/somedir/log_2017-10-20.txt

with content:

2017-10-20 09:50:59 This is log message
*/

extern inline std::string getCurrentDateTime(std::string s);
extern inline void Logger(std::string logMsg);
