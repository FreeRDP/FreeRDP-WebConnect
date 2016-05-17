#include "pluginCommon.h"

bool entryPoint(std::map<std::string, std::string> formValues, std::map<std::string, std::string> & result){
    //to test if a field exists use formValues.count("field_name")
    //to get a value for a field use formValues["field_name"]
    //
    //to change the RDP parameters fill the following fields
    //result["rdphost"] = "127.0.0.1"
    //result["rdpport"] = "12345"
    //result["rdppcb"] = "pcb id"
    //result["rdpuser"] = "username"
    //result["rdppass"] = "password"
    //
    //to pass a debug message to wsgate use
    //result["debug"] = "somedebug message"
    //
    //debug messages can be split by new line character and
    //will appear as separate debug messages
    //result["debug"] = "somedebug message" + "\n" + "otherdebug message"
    //
    //to pass an error message
    //result["err"] = "error message here" + "\n" + "error message there"
    //
    //to take the path for the config file that WebConnect takes as "-c" parameter use
    //result["configfile"]
    return true;
}
