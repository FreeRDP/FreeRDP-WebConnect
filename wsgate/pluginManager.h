#ifndef _PLUGIN_MANAGER_
#define _PLUGIN_MANAGER_

#include "logging.hpp"

#include<Windows.h>
#define LIBHANDLER HMODULE

#include<string>
#include<map>
#include<vector>

typedef bool(*queryPluginFUNC)(std::string, std::map<std::string, std::string>&);

class PluginManager {
public:
    static PluginManager* getInstance();
    static void shutDown();
    bool queryPlugins(std::string query, std::map<std::string, std::string>& output);
private:
    static PluginManager* instance;
    void listPlugins(std::string findPath, std::vector<std::string>& pluginFileNames);
    void loadPlugins(bool orUnload);
    void loadPlugin(std::string fileName);
    void unloadPlugin(LIBHANDLER handle);
    std::vector<queryPluginFUNC> functionPointers;
    PluginManager();
    ~PluginManager();

    std::vector<LIBHANDLER> pluginHandles;
};
#endif //_PLUGIN_MANAGER_