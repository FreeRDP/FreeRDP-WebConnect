#ifndef _PLUGIN_MANAGER_
#define _PLUGIN_MANAGER_

#include<Windows.h>
#define LIBHANDLER HMODULE

#include<string>
#include<map>
#include<vector>

typedef bool(*queryPluginFUNC)(std::string, std::map<std::string, std::string>&);

class PluginManager {
public:
    PluginManager* getInstance();
    void shutDown();
    bool queryPlugins(std::string query, std::map<std::string, std::string>& output);
private:
    std::vector<std::string> listPlugins();
    void loadPlugins();
    std::vector<queryPluginFUNC> functionPointers;
    PluginManager();
    ~PluginManager();

    std::vector<LIBHANDLER> pluginHandles;
};
#endif //_PLUGIN_MANAGER_