#ifndef _PLUGIN_COMMON_
#define _PLUGIN_COMMON_

#define EXPORT_FUNC __declspec(dllexport)

#include<string>
#include<map>

bool entryPoint(std::map<std::string, std::string> formValues, std::map<std::string, std::string> & result);

bool EXPORT_FUNC queryPlugin(std::string queryInput, std::map<std::string, std::string> & result){
    std::map<std::string, std::string> result;
    return entryPoint(std::map<std::string, std::string>(), result);
}

#endif //_PLUGIN_COMMON_