#include "pluginCommon.hpp"

void split(std::string input, std::vector<std::string>& tokens, char delim){
    std::stringstream stream(input);
    std::string token;
    while (std::getline(stream, token, delim)){
        tokens.push_back(token);
    }
}
extern "C" {
#ifdef _WIN32
bool EXPORT_FUNC queryPlugin(const char* queryInput, const char* configFile, char* resultBuffer, int max_resultBuffer)
#else
bool queryPlugin(char* queryInput, char* configFile, char* resultBuffer, int max_resultBuffer)
#endif
{
    std::map<std::string, std::string> params;
    //split the link in link body and parameter body
    std::vector<std::string> trims;
    split(std::string(queryInput), trims, '?');
    //split parameters
    std::vector<std::string> tokens;
    split(trims[1], tokens, '&');
    for (int i = 0; i < tokens.size(); i++){
        //split by name and value
        std::vector<std::string> splits;
        split(tokens[i], splits, '=');
        //test is value exists
        if (splits.size() == 1){
            params[splits[0]] = std::string();
        }
        else{
            params[splits[0]] = splits[1];
        }
        splits.resize(0);
    }

    std::map<std::string, std::string> result;
    result["configfile"] = std::string(configFile);

    bool success = entryPoint(params, result);

    //serialize the results
    std::string serialized;
    for (std::map<std::string, std::string>::iterator i = result.begin(); i != result.end(); i++){
        //each key-value pair is stored as
        //key\1value\2
        serialized += i->first;
        serialized += "\1";
        serialized += i->second;
        serialized += "\2";
    }

    memcpy_s(resultBuffer, max_resultBuffer, serialized.c_str(), serialized.size() + 1);

    return success;
}
}
