#ifndef _PLUGIN_MANAGER_
#define _PLUGIN_MANAGER_

#include "logging.hpp"

#ifdef _WIN32
#include<Windows.h>
#define LIBHANDLER HMODULE
#else
#include<dlfcn.h>
#include<limits.h>
#define LIBHANDLER void*
#define MAX_PATH PATH_MAX
#endif

#include<string>
#include<map>
#include<vector>

typedef bool(*queryPluginFUNC)(std::string, std::map<std::string, std::string>&);

class PluginManager {
public:
    static PluginManager* getInstance();
    static void shutDown();
    bool queryPlugins(std::string query, std::map<std::string, std::string>& output);
    inline void setOrder(std::vector<std::string> order){ pluginOrder = order; }
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
    std::vector<std::string> pluginOrder;
    std::vector<std::string> pluginNames;
};
#endif //_PLUGIN_MANAGER_
