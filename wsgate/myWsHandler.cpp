#include "myWsHandler.hpp"

namespace wsgate{
    MyWsHandler::MyWsHandler(EHSConnection *econn, EHS *ehs, MyRawSocketHandler *rsh)
                    : m_econn(econn)
                      , m_ehs(ehs)
                      , m_rsh(rsh){
    }

    void MyWsHandler::on_message(std::string hdr, std::string data){
        if (1 == (hdr[0] & 0x0F)) {
            // A text message
            if (':' == data[1]) {
                switch (data[0]) {
                    case 'D':
                        log::debug << "JS: " << data.substr(2) << endl;
                        break;
                    case 'I':
                        log::info << "JS: " << data.substr(2) << endl;
                        break;
                    case 'W':
                        log::warn << "JS: " << data.substr(2) << endl;
                        break;
                    case 'E':
                        log::err << "JS: " << data.substr(2) << endl;
                        break;
                }
            }
            return;
        }
        // binary message;
        m_rsh->OnMessage(m_econn, data);
    }

    void MyWsHandler::on_close(){
        log::debug << "GOT Close" << endl;
        ehs_autoptr<GenericResponse> r(new GenericResponse(0, m_econn));
        m_ehs->AddResponse(ehs_move(r));
    }

    bool MyWsHandler::on_ping(const std::string & data){
        log::debug << "GOT Ping: '" << data << "'" << endl;
        return true;
    }

    void MyWsHandler::on_pong(const std::string & data){
        log::debug << "GOT Pong: '" << data << "'" << endl;
    }

    void MyWsHandler::do_response(const std::string & data){
        ehs_autoptr<GenericResponse> r(new GenericResponse(0, m_econn));
        r->SetBody(data.data(), data.length());
        m_ehs->AddResponse(ehs_move(r));
    }
}