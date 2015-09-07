#include "wshandler.hpp"
#include "wsendpoint.hpp"

namespace wspp{
    void wshandler::send_text(const std::string & data) {
            if (m_endpoint) {
                m_endpoint->send(data, frame::opcode::TEXT);
            }
        }
    void wshandler::send_binary(const std::string & data) {
        if (m_endpoint) {
            m_endpoint->send(data, frame::opcode::BINARY);
        }
    }
}