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
#ifndef _NTSERVICE_H_
#define _NTSERVICE_H_

#if defined(_WIN32) || defined(DOXYGEN_RUN)

#include <string>

namespace wsgate {

    /**
     * A class for implementing a Windows service.
     * Internally, this class is a singleton, so you
     * can only have one instance in a program.
     * In convert a program into a service, its main() function
     * should be renamed an then being invoked from your derived
     * class Execute() method.
     */
    class NTService {

        public:

            enum {
                SERVICE_CONTROL_USER = 128
            };

            /**
             * Constructor
             * @param displayName Mandatory display name of the service
             * @param serviceName Optional internal service name
             */
            NTService(const std::string & displayName, const std::string & serviceName = "");

            // Destructor
            virtual ~NTService();

            /**
             * Retrieve installation status
             * @return true, if the service is installed.
             * @throws An instance of tracing::runtime_error, if the service
             * manager can not be contacted.
             */
            bool IsServiceInstalled() const;

            /**
             * Starts the service.
             * @throws An instance of tracing::runtime_error, if the service
             * manager can not be contacted or the service can not be started.
             */
            void Start() const;

            /**
             * Stops the service.
             * @throws An instance of tracing::runtime_error, if the service
             * manager can not be contacted or the service can not be stopped.
             */
            void Stop() const;

            /**
             * Installs the service.
             * @throws An instance of tracing::runtime_error, if the service
             * manager can not be contacted or the service can not be installed.
             */
            void InstallService() const;

            /**
             * Uninstalls the service.
             * @throws An instance of tracing::runtime_error, if the service
             * manager can not be contacted or the service can not be uninstalled.
             */
            void UninstallService() const;
           
            /**
             * Attempts to start the service control dispatcher.
             * @return true, if the dispatcher was successfully started.
             * @throws An instance of tracing::runtime_error, if the service
             * manager can not be contacted.
             */
            bool Execute() const;

            /**
             * Retrieves the exit code of the service.
             * @return The exit code of the service after stopping the service.
             */
            uint32_t ServiceExitCode() const;

        protected:
            /**
             * Perform initialization using the service arguments.
             * To be overridden by your derived class. The default
             * implementation just returns true.
             * @param argc The number of arguments.
             * @param argv A NULL terminated array of string pointers.
             * @return true on success.
             */
            virtual bool OnServiceInit(uint32_t argc, char **argv);
            /**
             * Runs the service main loop.
             * To be overridden by your derived class.
             * The default implementation just loops until
             * the service is stopped.
             */ 
            virtual void RunService();
            /**
             * Notifies the service to be stopped.
             * To be overridden by your derived class.
             * The default implementation just returns true.
             * @return true on success.
             */
            virtual bool OnServiceStop();
            /**
             * Notifies the service to be queried.
             * To be overridden by your derived class.
             * The default implementation just returns true.
             * @return true on success.
             */
            virtual bool OnServiceInterrogate();
            /**
             * Notifies the service to be paused.
             * To be overridden by your derived class.
             * The default implementation just returns true.
             * @return true on success.
             */
            virtual bool OnServicePause();
            /**
             * Notifies the service to resume.
             * To be overridden by your derived class.
             * The default implementation just returns true.
             * @return true on success.
             */
            virtual bool OnServiceContinue();
            /**
             * Notifies the service about a system shutdown.
             * To be overridden by your derived class.
             * The default implementation just returns true.
             * @return true on success.
             */
            virtual bool OnServiceShutdown();
            /**
             * Notifies the service about a parameter change.
             * To be overridden by your derived class.
             * The default implementation just returns true.
             * @return true on success.
             */
            virtual bool OnServiceParamChange();
            /**
             * Notifies the service about a device event.
             * To be overridden by your derived class.
             * The default implementation just returns true.
             * @return true on success.
             */
            virtual bool OnServiceDeviceEvent();
            /**
             * Notifies the service about a changed hardware profile.
             * To be overridden by your derived class.
             * The default implementation just returns true.
             * @return true on success.
             */
            virtual bool OnServiceHardwareProfileChange();
            /**
             * Notifies the service about a power management event.
             * To be overridden by your derived class.
             * The default implementation just returns true.
             * @return true on success.
             */
            virtual bool OnServicePowerEvent();
            /**
             * Notifies the service about a session change.
             * To be overridden by your derived class.
             * The default implementation just returns true.
             * @return true on success.
             */
            virtual bool OnServiceSessionChange();
            /**
             * Notifies the service about a user-defined control message.
             * To be overridden by your derived class.
             * The default implementation just returns true.
             * @param opcode The user defined control code.
             * @return true on success.
             */
            virtual bool OnServiceUserControl(uint32_t opcode);

            /**
             * Converts a service state to a human readable string.
             * @param state The state to convert.
             * @return The string representation of the provided state.
             */
            std::string StatusString(uint32_t state);
            /**
             * Converts a control code to a human readable string.
             * @param ctrl The control code to convert.
             * @return The string representation of the provided control code.
             */
            std::string CtrlString(uint32_t ctrl);
            /**
             * Adds a dependency to the list of prerequisites for this service.
             * @param s The name of another service on which this service depends.
             */
            void AddDependency(const std::string &s);
            /**
             * Changes and reports the service status to the service control manager.
             * @param state The new state of this service.
             */
            void SetServiceStatus(uint32_t state);
            /**
             * Reports the current service status to the service control manager.
             */
            void ReportServiceStatus();

            // data members
        protected:
            /// Flag: service is running
            bool m_bServiceRunning;
            /// The startup type of this service
            uint32_t m_dwServiceStartupType;
            /// The startupt type of this service
            SERVICE_STATUS m_ServiceStatus;
            /// The internal service name
            std::string m_sServiceName;
            /// The display name
            std::string m_sDisplayName;
            /// An optional description
            std::string m_sDescription;
            /// Internal path to our executable
            std::string m_sModulePath;

        private:
            bool InitializeService(uint32_t argc, char **argv);
            void ServiceMain(uint32_t argc, char **argv);
            uint32_t ControlHandler(uint32_t opcode, uint32_t eventType, void *eventData);

            std::string m_sDependencies;
            SERVICE_STATUS_HANDLE m_hServiceStatus;


            // nasty hack to get object ptr
            static NTService* m_pSelf;

            // static callback functions (called from Service-Manager)
            static void WINAPI _ServiceMain(unsigned long argc, char **argv);
            static unsigned long WINAPI _ControlHandler(unsigned long opcode,
                    unsigned long eventType, void *eventData, void *context);
    };

}
#endif

#endif // _NTSERVICE_H_
