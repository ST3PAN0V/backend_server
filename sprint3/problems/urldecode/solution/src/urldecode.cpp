#include "urldecode.h"

#include <charconv>
#include <stdexcept>

std::string UrlDecode(std::string_view str) {
    std::string decode_str;
    decode_str.reserve(str.size());
    for (int i = 0; i < str.size(); i++) {
        if (str[i] == '%') {
            
            if (i + 2 >= str.size()) {
                throw std::invalid_argument("Incomplete percent-encoding");
            }
            char hex1 = str[i + 1];
            char hex2 = str[i + 2];

            if (!std::isxdigit(hex1) || !std::isxdigit(hex2)) {
                throw std::invalid_argument("Invalid percent-encoding");
            }

            int value = (std::isdigit(hex1) ? hex1 - '0' : std::toupper(hex1) - 'A' + 10) * 16 +
                         (std::isdigit(hex2) ? hex2 - '0' : std::toupper(hex2) - 'A' + 10);
            decode_str.push_back(static_cast<char>(value));
            i += 2; 

        } else if (str[i] == '+') {
            decode_str.push_back(' ');
        } else {
            decode_str.push_back(str[i]);
        }
    }
    return decode_str;
}
