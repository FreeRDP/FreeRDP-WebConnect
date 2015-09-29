#include "pluginManager.h"
#include <windows.h>

PluginManager* PluginManager::instance = NULL;

PluginManager::PluginManager(){
    this->loadPlugins(false);
}

PluginManager::~PluginManager(){
    this->loadPlugins(true);
}

void PluginManager::loadPlugins(bool orUnload){
    if (!orUnload){
        std::vector<std::string> files;
        listPlugins("./plugins/*", files);
        for (int i = 0; i < files.size(); i++){
            queryPluginFUNC func = this->loadPlugin(files[i]);
            if (func != NULL){
                functionPointers.push_back(func);
            }
        }
    }
    //unload
    else{
        for (int i = 0; i < pluginHandles.size(); i++){
            unloadPlugin(pluginHandles[i]);
        }
    }
}

PluginManager* PluginManager::getInstance(){
    if (PluginManager::instance == NULL)
        PluginManager::instance = new PluginManager();
    return PluginManager::instance;
}

bool PluginManager::queryPlugins(std::string query, std::map<std::string, std::string>& output){
    bool result = false;
    for (int i = 0; i < functionPointers.size(); i++){
        result |= functionPointers[i](query, output);
    }
    return result;
}

//TODO will not work with unicode
#define string2LPCSTR(str) (LPCSTR)std::wstring( str .begin(), str .end()).c_str()

queryPluginFUNC PluginManager::loadPlugin(std::string fileName){
    LIBHANDLER handle = LoadLibrary(string2LPCSTR(fileName));
    if (handle != NULL){
        queryPluginFUNC function = (queryPluginFUNC)GetProcAddress(handle, "queryPlugin");
        if (function != NULL){
            wsgate::logger::debug << "Found plugin " << fileName << std::endl;
            pluginHandles.push_back(handle);
            functionPointers.push_back(function);
        }
        else{
            FreeLibrary(handle);
            wsgate::logger::err << "Error getting function pointer to \"queryPlugin\" in " << fileName << std::endl;
        }
    }
    else{
        wsgate::logger::err << "Error loading plugin " << fileName << std::endl;
    }
    return 0;
}

void PluginManager::unloadPlugin(LIBHANDLER handle){
    FreeLibrary(handle);
}

void PluginManager::listPlugins(std::string findPath, std::vector<std::string>& pluginFileNames){
    WIN32_FIND_DATA file;
    size_t length_of_arg;
    HANDLE findHandle = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;

    findHandle = FindFirstFile(string2LPCSTR(findPath), &file);

    if (findHandle == INVALID_HANDLE_VALUE)
    {
        wsgate::logger::err << "Bad find path." << std::endl;
    }

    do
    {
        if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            //TODO find recursively
        }
        else
        {
            pluginFileNames.push_back(std::string(file.cFileName));
        }
    } while (FindNextFile(findHandle, &file) != 0);

    FindClose(findHandle);
}
