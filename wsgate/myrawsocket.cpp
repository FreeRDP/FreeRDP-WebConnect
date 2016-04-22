#include "myrawsocket.hpp"
#include "wsgateEHS.hpp"
#include "myWsHandler.hpp"

namespace wsgate{
    MyRawSocketHandler::MyRawSocketHandler(WsGate *parent)
        : m_parent(parent)
          , m_cmap(conn_map())
    { }

    bool MyRawSocketHandler::OnData(EHSConnection *conn, std::string data)
    {
        if (m_cmap.end() != m_cmap.find(conn)) {
            m_cmap[conn].get<0>()->AddRxData(data);
            return true;
        }
        return false;
    }

    void MyRawSocketHandler::OnConnect(EHSConnection * /* conn */)
    {
        log::debug << "GOT WS CONNECT" << endl;
    }

    void MyRawSocketHandler:: OnDisconnect(EHSConnection *conn)
    {
        log::debug << "GOT WS DISCONNECT" << endl;
        m_parent->UnregisterRdpSession(m_cmap[conn].get<2>());
        m_cmap.erase(conn);
    }

    void MyRawSocketHandler::OnMessage(EHSConnection *conn, const std::string & data)
    {
        if (m_cmap.end() != m_cmap.find(conn)) {
            m_cmap[conn].get<2>()->OnWsMessage(data);
        }
    }

    bool MyRawSocketHandler::Prepare(EHSConnection *conn, const string host, const string pcb,
            const string user, const string pass, const WsRdpParams &params, EmbeddedContext embeddedContext)
    {
        try
        {
            handler_ptr h(new MyWsHandler(conn, m_parent, this));
            conn_ptr c(new wspp::wsendpoint(h.get()));
            rdp_ptr r(new RDP(h.get(), this));
            m_cmap[conn] = conn_tuple(c, h, r);

            r->setEmbeddedContext(embeddedContext);

            this->conn = conn;
            if (embeddedContext == CONTEXT_EMBEDDED){
                PrepareRDP(host, pcb, user, pass, params);
            }
        }
        catch(...)
        {
            log::info << "Attemtped double connection to the same machine" << endl;
            return false;
        }
        return true;
    }

    void MyRawSocketHandler::PrepareRDP(const std::string _host, const std::string _pcb, const std::string _user, const std::string _pass, const WsRdpParams &_params){
        std::string host = _host;
        std::string pcb = _pcb;
        std::string user = _user;
        std::string pass = _pass;
        WsRdpParams params = _params;

        string username;
        string domain;

        //do needed overrides
        WsRdpOverrideParams op = this->m_parent->getOverrideParams();
        if (op.m_bOverrideRdpFntlm) params.fntlm = op.m_RdpOverrideParams.fntlm;
        if (op.m_bOverrideRdpNomani) params.nomani = op.m_RdpOverrideParams.nomani;
        if (op.m_bOverrideRdpNonla) params.nonla = op.m_RdpOverrideParams.nonla;
        if (op.m_bOverrideRdpNotheme) params.notheme = op.m_RdpOverrideParams.notheme;
        if (op.m_bOverrideRdpNotls) params.notls = op.m_RdpOverrideParams.notls;
        if (op.m_bOverrideRdpNowallp) params.nowallp = op.m_RdpOverrideParams.nowallp;
        if (op.m_bOverrideRdpNowdrag) params.nowdrag = op.m_bOverrideRdpNowdrag;
        if (op.m_bOverrideRdpPerf) params.perf = op.m_RdpOverrideParams.perf;
        if (op.m_bOverrideRdpPort) params.port = op.m_RdpOverrideParams.port;
        if (op.m_bOverrideRdpHost) host = op.m_sRdpOverrideHost;
        if (op.m_bOverrideRdpPass) pass = op.m_sRdpOverridePass;
        if (op.m_bOverrideRdpPcb) pcb = op.m_sRdpOverridePcb;
        if (op.m_bOverrideRdpUser) user = op.m_sRdpOverrideUser;

        SplitUserDomain(user, username, domain);

        rdp_ptr r = this->m_cmap[this->conn].get<2>();
        r->Connect(host, pcb, username, domain, pass, params);
        m_parent->RegisterRdpSession(r);

        log::debug << "RDP Host:              '" << host << "'" << endl;
        log::debug << "RDP Pcb:               '" << pcb << "'" << endl;
        log::debug << "RDP User:              '" << user << "'" << endl;
        log::info << "RDP Port:               '" << params.port << "'" << endl;
        log::debug << "RDP Desktop size:       " << params.width << "x" << params.height << endl;
        log::debug << "RDP Performance:        " << params.perf << endl;
        log::debug << "RDP No wallpaper:       " << params.nowallp << endl;
        log::debug << "RDP No full windowdrag: " << params.nowdrag << endl;
        log::debug << "RDP No menu animation:  " << params.nomani << endl;
        log::debug << "RDP No theming:         " << params.notheme << endl;
        log::debug << "RDP Disable TLS:        " << params.notls << endl;
        log::debug << "RDP Disable NLA:        " << params.nonla << endl;
        log::debug << "RDP NTLM auth:          " << params.fntlm << endl;
    }
}