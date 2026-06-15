// protocol.cpp — implementation of the wire-protocol codec and control law.
#include "protocol.h"

namespace protocol {

namespace {

// Trim leading/trailing ASCII whitespace.
std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    const size_t begin = s.find_first_not_of(ws);
    if (begin == std::string::npos) return "";
    const size_t end = s.find_last_not_of(ws);
    return s.substr(begin, end - begin + 1);
}

bool is_start_marker(const std::string& f) { return f == "0x55" || f == "55"; }
bool is_end_marker(const std::string& f)   { return f == "0xED" || f == "ED"; }

}  // namespace

std::vector<std::string> split_fields(const std::string& msg) {
    std::vector<std::string> fields;
    size_t start = 0;
    while (true) {
        const size_t comma = msg.find(',', start);
        if (comma == std::string::npos) {
            fields.push_back(trim(msg.substr(start)));
            break;
        }
        fields.push_back(trim(msg.substr(start, comma - start)));
        start = comma + 1;
    }
    return fields;
}

bool parse_frame(const std::string& msg, Frame& out) {
    const std::vector<std::string> f = split_fields(msg);
    if (f.size() != 5) return false;
    if (!is_start_marker(f[0]) || !is_end_marker(f[4])) return false;
    out.id = f[1];
    out.type = f[2];
    out.value = f[3];
    return true;
}

std::string encode_sensor_frame(const std::string& id,
                                const std::string& type,
                                const std::string& value) {
    return "0x55, " + id + ", " + type + ", " + value + ", 0xED";
}

std::string encode_fan_frame(char side, int speed) {
    return "55,Bike01,M_" + std::string(1, side) + "," + std::to_string(speed) + ",ED";
}

int co2_to_fan_speed(int ppm) {
    if (ppm >= 2000) return 0;
    if (ppm >= 1700) return 2;
    if (ppm >= 1300) return 3;
    if (ppm >= 1000) return 4;
    if (ppm >= 600)  return 5;
    return 25;
}

int fan_speed_to_display(int speed) {
    switch (speed) {
        case 0:  return 5;
        case 2:  return 4;
        case 3:  return 3;
        case 4:  return 2;
        case 5:  return 1;
        case 25: return 0;
        default: return speed;
    }
}

uint16_t modbus_crc(const uint8_t* buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < len; ++pos) {
        crc ^= static_cast<uint16_t>(buf[pos]);
        for (int i = 8; i != 0; --i) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

}  // namespace protocol
