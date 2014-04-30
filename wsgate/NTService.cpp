/* vim: set et ts=4 sw=4 cindent:
 *
 * FreeRDP-WebConnect,
 * A gateway for seamless access to your RDP-Sessions in any HTML5-compliant browser.
 *
 * Copyright 2012 Fritz Elfert <wsgate@fritz-elfert.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sstream>
#include <cstring>
#include <cstdlib>

#ifdef HAVE_WINDOWS_H
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "NTService.hpp"
#include "btexception.hpp"


using namespace std;

namespace wsgate {

    static string sysErrorMsg() {
        char *buf;
        uint32_t ecode = ::GetLastError();
        if (0 == ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL, ecode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    reinterpret_cast<char *>(&buf), 0, NULL)) {
            ostringstream oss;
            oss << "Unknown error 0x" << hex << ecode;
            return oss.str();
        }
        string ret(buf);
        LocalFree(buf);
        return ret;
    }

    // static
    NTService* NTService::m_pSelf = NULL;

    NTService::NTService(const string &serviceName, const string &displayName)
          : m_sServiceName(serviceName)
          , m_sDisplayName(displayName)
    {
        // copy the address of the current object so we can access it from
        // the static member callback functions.
        // WARNING: This limits the application to only one NTService object.

        if (NULL != m_pSelf) {
            throw tracing::runtime_error("duplicate NTService");
        }

        // Set the default service name and version
        m_dwServiceStartupType = SERVICE_DEMAND_START;

        // set up the initial service status
        m_hServiceStatus = 0L;
        m_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        m_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        m_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        m_ServiceStatus.dwWin32ExitCode = 0;
        m_ServiceStatus.dwServiceSpecificExitCode = 0;
        m_ServiceStatus.dwCheckPoint = 0;
        m_ServiceStatus.dwWaitHint = 0;

        // Fetch our module name
        char buf[1024];
        uint32_t len = GetModuleFileNameA(NULL, buf, sizeof(buf));
        m_sModulePath.assign(buf, len);
        m_pSelf = this;
    }

    NTService::~NTService() {
        m_pSelf = NULL;
    }

    ////////////////////////////////////////////////////////////////////////////////////////
    // Install/uninstall routines

    bool NTService::IsServiceInstalled() const {
        bool ret = false;
        SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, GENERIC_READ);
        if (hSCM) {
            SC_HANDLE hService = ::OpenService(hSCM, m_sServiceName.c_str(), SERVICE_QUERY_CONFIG);
            if (hService) {
                ret = true;
                ::CloseServiceHandle(hService);
            }
            ::CloseServiceHandle(hSCM);
        } else {
            throw tracing::runtime_error(sysErrorMsg());
        }
        return ret;
    }

    void NTService::InstallService() const {
        SC_HANDLE hSCM = ::OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (hSCM) {
            string tmp(m_sDependencies);
            tmp.append("\0\0", 2);
            SC_HANDLE hService = ::CreateServiceA(hSCM, m_sServiceName.c_str(),
                    m_sDisplayName.c_str(), SERVICE_ALL_ACCESS, m_ServiceStatus.dwServiceType,
                    m_dwServiceStartupType, SERVICE_ERROR_NORMAL, m_sModulePath.c_str(),
                    NULL, NULL, tmp.data(), NULL, NULL);
            if (hService) {
                if (!m_sDescription.empty()) {
                    SERVICE_DESCRIPTION sd;
                    sd.lpDescription = strdup(m_sDescription.c_str());
                    if (!ChangeServiceConfig2A(hService, SERVICE_CONFIG_DESCRIPTION, &sd)) {
                        free(sd.lpDescription);
                        ::CloseServiceHandle(hService);
                        ::CloseServiceHandle(hSCM);
                        throw tracing::runtime_error(sysErrorMsg());
                    }
                    free(sd.lpDescription);
                }
                ::CloseServiceHandle(hService);
            } else {
                ::CloseServiceHandle(hSCM);
                throw tracing::runtime_error(sysErrorMsg());
            }
            ::CloseServiceHandle(hSCM);
        } else {
            throw tracing::runtime_error(sysErrorMsg());
        }
    }

    void NTService::UninstallService() const {
        SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (hSCM) {
            SC_HANDLE hService = ::OpenServiceA(hSCM, m_sServiceName.c_str(), DELETE);
            if (hService) {
                if (!::DeleteService(hService)) {
                    ::CloseServiceHandle(hService);
                    ::CloseServiceHandle(hSCM);
                    throw tracing::runtime_error(sysErrorMsg());
                }
                ::CloseServiceHandle(hService);
            } else {
                ::CloseServiceHandle(hSCM);
                throw tracing::runtime_error(sysErrorMsg());
            }
            ::CloseServiceHandle(hSCM);
        } else {
            throw tracing::runtime_error(sysErrorMsg());
        }
    }

    void NTService::Start() const {
        SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (hSCM) {
            SC_HANDLE hService = ::OpenServiceA(hSCM, m_sServiceName.c_str(), SERVICE_START);
            if (hService) {
                if (!::StartServiceA(hService, 0, NULL)) {
                    ::CloseServiceHandle(hService);
                    ::CloseServiceHandle(hSCM);
                    throw tracing::runtime_error(sysErrorMsg());
                }
                ::CloseServiceHandle(hService);
            } else {
                ::CloseServiceHandle(hSCM);
                throw tracing::runtime_error(sysErrorMsg());
            }
            ::CloseServiceHandle(hSCM);
        } else {
            throw tracing::runtime_error(sysErrorMsg());
        }
    }

    void NTService::Stop() const {
        SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (hSCM) {
            SC_HANDLE hService = ::OpenServiceA(hSCM, m_sServiceName.c_str(), SERVICE_STOP);
            if (hService) {
                SERVICE_STATUS ss;
                if (!::ControlService(hService, SERVICE_CONTROL_STOP, &ss)) {
                    ::CloseServiceHandle(hService);
                    ::CloseServiceHandle(hSCM);
                    throw tracing::runtime_error(sysErrorMsg());
                }
                ::CloseServiceHandle(hService);
            } else {
                ::CloseServiceHandle(hSCM);
                throw tracing::runtime_error(sysErrorMsg());
            }
            ::CloseServiceHandle(hSCM);
        } else {
            throw tracing::runtime_error(sysErrorMsg());
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Service startup and registration

    bool NTService::Execute() const {
        SERVICE_TABLE_ENTRY st[] = {
            {const_cast<char *>(m_sServiceName.c_str()), _ServiceMain},
            {NULL, NULL}
        };
        if (0 == ::StartServiceCtrlDispatcherA(st)) {
            if (ERROR_FAILED_SERVICE_CONTROLLER_CONNECT != ::GetLastError()) { 
                throw tracing::runtime_error(sysErrorMsg());
            }
            return false;
        }
        return true;
    }

    void NTService::ServiceMain(uint32_t argc, char** argv) {
        // Register the control request handler
        m_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
        m_hServiceStatus =
            ::RegisterServiceCtrlHandlerExA(m_sServiceName.c_str(), _ControlHandler, this);
        if (0L == m_hServiceStatus) {
            return;
        }

        // Start the initialisation
        if (InitializeService(argc, argv)) {
            // Do the real work.
            // When the Run function returns, the service has stopped.
            m_ServiceStatus.dwWin32ExitCode = 0;
            m_ServiceStatus.dwCheckPoint = 0;
            m_ServiceStatus.dwWaitHint = 0;
            RunService();
        }
        // Tell the service manager we are stopped
        SetServiceStatus(SERVICE_STOPPED);
    }

    // Helper functions
    string NTService::StatusString(uint32_t state) {
        ostringstream oss;
        switch (state) {
            case SERVICE_CONTINUE_PENDING:
                oss << "SERVICE_CONTINUE_PENDING";
                break;
            case SERVICE_PAUSE_PENDING:
                oss << "SERVICE_PAUSE_PENDING";
                break;
            case SERVICE_PAUSED:
                oss << "SERVICE_PAUSED";
                break;
            case SERVICE_RUNNING:
                oss << "SERVICE_RUNNING";
                break;
            case SERVICE_START_PENDING:
                oss << "SERVICE_START_PENDING";
                break;
            case SERVICE_STOP_PENDING:
                oss << "SERVICE_STOP_PENDING";
                break;
            case SERVICE_STOPPED:
                oss << "SERVICE_STOPPED";
                break;
            default:
                oss << "Unknown value 0x" << hex << state;
                break;
        }
        return oss.str();
    }

    string NTService::CtrlString(uint32_t ctrl) {
        ostringstream oss;
        switch (ctrl) {
            case SERVICE_CONTROL_STOP:
                oss << "SERVICE_CONTROL_STOP";
            case SERVICE_CONTROL_PAUSE:
                oss << "SERVICE_CONTROL_PAUSE";
            case SERVICE_CONTROL_CONTINUE:
                oss << "SERVICE_CONTROL_CONTINUE";
            case SERVICE_CONTROL_INTERROGATE:
                oss << "SERVICE_CONTROL_INTERROGATE";
            case SERVICE_CONTROL_SHUTDOWN:
                oss << "SERVICE_CONTROL_SHUTDOWN";
            case SERVICE_CONTROL_PARAMCHANGE:
                oss << "SERVICE_CONTROL_PARAMCHANGE";
            case SERVICE_CONTROL_DEVICEEVENT:
                oss << "SERVICE_CONTROL_DEVICEEVENT";
            case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
                oss << "SERVICE_CONTROL_HARDWAREPROFILECHANGE";
            case SERVICE_CONTROL_POWEREVENT:
                oss << "SERVICE_CONTROL_POWEREVENT";
            case SERVICE_CONTROL_SESSIONCHANGE:
                oss << "SERVICE_CONTROL_SESSIONCHANGE";
            default:
                oss << ((ctrl >= SERVICE_CONTROL_USER) ? "User" : "Invalid")
                    << " value 0x" << hex << ctrl;
        }
        return oss.str();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    // status functions

    void NTService::SetServiceStatus(uint32_t state) {
        m_ServiceStatus.dwCurrentState = state;
        ReportServiceStatus();
    }

    uint32_t NTService::ServiceExitCode() const {
        return m_ServiceStatus.dwWin32ExitCode;
    }

    void NTService::ReportServiceStatus() {
        ::SetServiceStatus(m_hServiceStatus, &m_ServiceStatus);
    }

    void NTService::AddDependency(const string &s) {
        m_sDependencies.append(s.c_str(), s.length() + 1);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Service initialization

    bool NTService::InitializeService(uint32_t argc, char** argv) {
        // Start the initialization
        SetServiceStatus(SERVICE_START_PENDING);

        // Perform the actual initialization
        bool bResult = OnServiceInit(argc, argv);

        // Set final state
        m_ServiceStatus.dwWin32ExitCode = GetLastError();
        m_ServiceStatus.dwCheckPoint = 0;
        m_ServiceStatus.dwWaitHint = 0;
        if (!bResult) {
            SetServiceStatus(SERVICE_STOPPED);
            return false;
        }
        m_bServiceRunning = true;
        SetServiceStatus(SERVICE_RUNNING);
        return true;
    }

    void NTService::RunService() {
        while (m_bServiceRunning) {
            usleep(5000);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // Control request handler
    uint32_t NTService::ControlHandler(uint32_t opcode, uint32_t eventType, void * eventData) {
        uint32_t ret = ERROR_CALL_NOT_IMPLEMENTED;

        switch (opcode) {
            case SERVICE_CONTROL_STOP:
                SetServiceStatus(SERVICE_STOP_PENDING);
                OnServiceStop();
                m_bServiceRunning = false;
                ret = NO_ERROR;
                break;
            case SERVICE_CONTROL_PAUSE:
                if (OnServicePause())
                    ret = NO_ERROR;
                break;
            case SERVICE_CONTROL_CONTINUE:
                if (OnServiceContinue())
                    ret = NO_ERROR;
                break;
            case SERVICE_CONTROL_INTERROGATE:
                if (OnServiceInterrogate())
                    ret = NO_ERROR;
                break;
            case SERVICE_CONTROL_SHUTDOWN:
                if (OnServiceShutdown())
                    ret = NO_ERROR;
                break;
            case SERVICE_CONTROL_PARAMCHANGE:
                if (OnServiceParamChange())
                    ret = NO_ERROR;
                break;
            case SERVICE_CONTROL_DEVICEEVENT:
                if (OnServiceDeviceEvent())
                    ret = NO_ERROR;
                break;
            case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
                if (OnServiceHardwareProfileChange())
                    ret = NO_ERROR;
                break;
            case SERVICE_CONTROL_POWEREVENT:
                if (OnServicePowerEvent())
                    ret = NO_ERROR;
                break;
            case SERVICE_CONTROL_SESSIONCHANGE:
                if (OnServiceSessionChange())
                    ret = NO_ERROR;
                break;
            default:
                if (opcode >= SERVICE_CONTROL_USER) {
                    if (OnServiceUserControl(opcode - SERVICE_CONTROL_USER))
                        ret = NO_ERROR;
                }
                break;
        }
        return ret;
    }

    // static
    void NTService::_ServiceMain(unsigned long argc, char **argv) {
        if (m_pSelf) {
            m_pSelf->ServiceMain(argc, argv);
        }
    }

    // static
    unsigned long NTService::_ControlHandler(unsigned long opcode, unsigned long eventType,
            void *eventData, void *context) {
        NTService *self = reinterpret_cast<NTService *>(context);
        if (NULL != self) {
            return self->ControlHandler(opcode, eventType, eventData);
        }
        return ERROR_CALL_NOT_IMPLEMENTED;
    }


    //
    // Default Prototypes for vitual handlers to be overloaded;
    //

    // Called when the service is first initialized
    bool NTService::OnServiceInit(uint32_t, char **) {
        return true;
    }

    // Called when the service control manager wants to stop the service
    bool NTService::OnServiceStop() {
        return true;
    }

    // called when the service is interrogated
    bool NTService::OnServiceInterrogate() {
        ReportServiceStatus();
        return true;
    }

    // called when the service is paused
    bool NTService::OnServicePause() {
        return false;
    }

    // called when the service is continued
    bool NTService::OnServiceContinue() {
        return false;
    }

    // called when the system is shut down
    bool NTService::OnServiceShutdown() {
        return false;
    }

    // called when service parameters have changed
    bool NTService::OnServiceParamChange() {
        return false;
    }

    // called when devices have been added or removed
    bool NTService::OnServiceDeviceEvent() {
        return false;
    }

    // called when hardware profile has changed
    bool NTService::OnServiceHardwareProfileChange() {
        return false;
    }

    // called when power status has changed
    bool NTService::OnServicePowerEvent() {
        return false;
    }

    // called when session has changed
    bool NTService::OnServiceSessionChange() {
        return false;
    }

    // called when the service gets a user control message
    bool NTService::OnServiceUserControl(uint32_t opcode) {
        return false; // say not handled
    }

}
