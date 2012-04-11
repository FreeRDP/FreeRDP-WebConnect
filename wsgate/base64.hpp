/* vim: set et ts=4 sw=4 cindent: */
#ifndef _BASE64_H_
#define _BASE64_H_

#include <string>

std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);

#endif
