#ifndef _PLUGIN_COMMON_
#define _PLUGIN_COMMON_

#ifdef _WIN32
#define EXPORT_FUNC __declspec(dllexport)
#endif

#include<string>
#include<map>
#include<sstream>
#include<vector>


bool entryPoint(std::map<std::string, std::string> formValues, std::map<std::string, std::string> & result);

extern "C"
{
#ifdef _WIN32
    bool EXPORT_FUNC queryPlugin(const char* queryInput, const char* configFile, char* resultBuffer);
#else
    bool queryPlugin(const char* queryInput, const char* configFile, char* resultBuffer);
#endif
}

#endif //_PLUGIN_COMMON_
