#include "pluginManager.hpp"
#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

PluginManager* PluginManager::instance = NULL;

PluginManager::PluginManager(){
    this->loadPlugins(false);
}

PluginManager::~PluginManager(){
    this->loadPlugins(true);
}

void PluginManager::shutDown(){
    delete PluginManager::instance;
}

void PluginManager::loadPlugins(bool orUnload){
    if (!orUnload){
        std::vector<std::string> files;
        listPlugins("plugins/", files);
        for (int i = 0; i < files.size(); i++){
            loadPlugin(files[i]);
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
    //execute queries with respect to the config file
    for (int i = 0; i < pluginOrder.size(); i++){
        for (int j = 0; j < pluginNames.size(); j++){
            if (boost::algorithm::ends_with(pluginNames[j], pluginOrder[i])){
                result |= functionPointers[j](query, output);
            }
        }
    }
    //execute remaining queries
    for (int j = 0; j < pluginNames.size(); j++){
        bool found = false;
        for (int i = 0; i < pluginOrder.size(); i++){
            if (boost::algorithm::ends_with(pluginNames[j], pluginOrder[i])){
                found = true;
            }
        }
        if (!found)
            result |= functionPointers[j](query, output);
    }
    return result;
}
#ifdef _WIN32
#define string2LPCSTR(str) (LPCSTR)str.c_str()
#endif
std::string getexepath()
{
    char result[MAX_PATH];
#ifdef _WIN32
    return std::string(result, GetCurrentDirectory(MAX_PATH, result));
#else
    return std::string(result, getcwd(result, MAX_PATH));
#endif
}

void PluginManager::loadPlugin(std::string fileName){
#ifdef _WIN32
    LIBHANDLER handle = LoadLibrary(string2LPCSTR(fileName));
#else
    LIBHANDLER handle = dlopen(fileName.c_str(), RTLD_NOW);
#endif
    if (handle != NULL){
#ifdef _WIN32
        queryPluginFUNC function = (queryPluginFUNC)GetProcAddress(handle, "queryPlugin");
#else
        queryPluginFUNC function = (queryPluginFUNC)dlsym(handle, "queryPlugin");
#endif
        if (function != NULL){
            wsgate::logger::debug << "Found plugin " << fileName << std::endl;
            pluginHandles.push_back(handle);
            functionPointers.push_back(function);
            pluginNames.push_back(fileName);
        }
        else{
            unloadPlugin(handle);
            wsgate::logger::err << "Error getting function pointer to \"queryPlugin\" in " << fileName << std::endl;
        }
    }
    else{
#ifdef _WIN32
        DWORD lastError = GetLastError();
        wsgate::logger::err << "Error loading plugin " << fileName << " ; error code:" << lastError << std::endl;
#else
        wsgate::logger::err << "Error loading plugin " << fileName << " ; error message:\"" << dlerror() << "\"" << std::endl;
#endif
    }
}

void PluginManager::unloadPlugin(LIBHANDLER handle){
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
}

void PluginManager::listPlugins(std::string findPath, std::vector<std::string>& pluginFileNames){
    std::string path(getexepath());
    //check if path ends with '/' or '\\'
#ifdef _WIN32
    if (!boost::algorithm::ends_with(path, "\\"))
        path += "\\";
#else
    if (!boost::algorithm::ends_with(path, "/"))
        path += "/";
#endif

    path += findPath;
    findPath = path;

    namespace fs = boost::filesystem;
    fs::path directory(findPath);
    fs::directory_iterator end;

    if (fs::exists(directory) && fs::is_directory(directory))
    {
        for (fs::directory_iterator i(path); i != end; ++i)
        {
            if (fs::is_regular_file(i->status()))
            {
                pluginFileNames.push_back(i->path().string());
            }
        }
    }

/*    WIN32_FIND_DATA file;
    size_t length_of_arg;
    HANDLE findHandle = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;
    std::string path(getexepath());
    //find path delimiter
    int n = path.length();
    int i = n - 1;
    while (path[i] != '\\') i--;
    path.erase(i + 1, n);
    path += findPath;
    findPath = path;
    findHandle = FindFirstFile(string2LPCSTR((findPath + "*.dll")), &file);

    if (findHandle == INVALID_HANDLE_VALUE)
    {
        wsgate::logger::err << "Bad find path." << std::endl;
        return;
    }

    do
    {
        if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            //TODO find recursively
        }
        else
        {
            pluginFileNames.push_back(findPath + std::string(file.cFileName));
        }
    } while (FindNextFile(findHandle, &file) != 0);

    FindClose(findHandle);*/
}
