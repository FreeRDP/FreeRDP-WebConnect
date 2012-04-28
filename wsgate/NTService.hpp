#ifndef _NTSERVICE_H_
#define _NTSERVICE_H_

#if definded(_WIN32) || defined(DOXYGEN_RUN)

#include <string>

namespace wsgate {


    class NTService {

        public:
            enum {
                SERVICE_CONTROL_USER = 128
            };

            NTService(const std::string & displayName, const std::string & serviceName = "");
            virtual ~NTService();

            bool IsServiceInstalled() const;
            void Start() const;
            void Stop() const;
            void InstallService() const;
            void UninstallService() const;
            bool Execute() const;
            uint32_t ServiceExitCode() const;

        protected:
            virtual bool OnServiceInit(uint32_t argc, char **argv);
            virtual void RunService();
            virtual bool OnServiceStop();
            virtual bool OnServiceInterrogate();
            virtual bool OnServicePause();
            virtual bool OnServiceContinue();
            virtual bool OnServiceShutdown();
            virtual bool OnServiceParamChange();
            virtual bool OnServiceDeviceEvent();
            virtual bool OnServiceHardwareProfileChange();
            virtual bool OnServicePowerEvent();
            virtual bool OnServiceSessionChange();
            virtual bool OnServiceUserControl(uint32_t opcode);

            std::string StatusString(uint32_t state);
            std::string CtrlString(uint32_t ctrl);
            void AddDependency(const std::string &);
            void SetServiceStatus(uint32_t state);
            void ReportServiceStatus();

            // data members
        protected:
            bool m_bServiceRunning;
            uint32_t m_dwServiceStartupType;
            SERVICE_STATUS m_ServiceStatus;
            std::string m_sServiceName;
            std::string m_sDisplayName;
            std::string m_sDescription;
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
