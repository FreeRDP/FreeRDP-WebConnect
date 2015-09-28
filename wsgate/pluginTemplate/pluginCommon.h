#ifndef _PLUGIN_COMMON_
#define _PLUGIN_COMMON_

#define EXPORT_FUNC __declspec(dllexport)

#include<string>
#include<map>
#include<sstream>
#include<vector>


bool entryPoint(std::map<std::string, std::string> formValues, std::map<std::string, std::string> & result);

void split(std::string input, std::vector<std::string>& tokens, char delim){
    std::stringstream stream(input);
    std::string token;
    while (std::getline(stream, token, delim)){
        tokens.push_back(token);
    }
}

bool EXPORT_FUNC queryPlugin(std::string queryInput, std::map<std::string, std::string> & result){
    std::map<std::string, std::string> params;
    //split the link in link body and parameter body
    std::vector<std::string> trims;
    split(queryInput, trims, '?');
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
    return entryPoint(params, result);
}

#endif //_PLUGIN_COMMON_