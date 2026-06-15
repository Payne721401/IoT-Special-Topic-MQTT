// protocol.h — hardware-independent wire-protocol codec and control law.
//
// Pure C++ (no Arduino / WiFi dependencies) so it can be unit-tested on a host
// machine. The firmware includes this library and delegates all framing,
// parsing, CRC and the CO2->fan decision to it. See ../../docs/protocol.md.
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace protocol {

// A decoded message: "<START>, <id>, <type>, <value>, <END>".
struct Frame {
    std::string id;     // e.g. "Temperature", "0xD11704d", "Bike01"
    std::string type;   // e.g. "SHT35", "S8", "M_R"
    std::string value;  // measurement or command, as text
};

// Split a raw message on commas and trim surrounding whitespace from each field.
std::vector<std::string> split_fields(const std::string& msg);

// Parse "START, id, type, value, END" into `out`.
// START accepts "0x55" or "55"; END accepts "0xED" or "ED".
// Returns false (and leaves `out` unchanged) if the frame is malformed.
bool parse_frame(const std::string& msg, Frame& out);

// Encoders that reproduce the exact strings the firmware puts on the wire.
std::string encode_sensor_frame(const std::string& id,
                                const std::string& type,
                                const std::string& value);  // "0x55, id, type, value, 0xED"
std::string encode_fan_frame(char side, int speed);         // "55,Bike01,M_<side>,<speed>,ED"

// Control law: map CO2 concentration (ppm) to a fan speed.
// Scale is inverted: 0 = full power ... 5 = lowest, 25 = off.
int co2_to_fan_speed(int ppm);

// Map a firmware fan speed to the intuitive 0..5 display scale used by the
// dashboard (0 = off ... 5 = full power). Unknown values are returned unchanged.
int fan_speed_to_display(int speed);

// CRC-16/MODBUS (poly 0xA001, init 0xFFFF) over `len` bytes of `buf`.
// Low byte is transmitted first on the wire.
uint16_t modbus_crc(const uint8_t* buf, size_t len);

}  // namespace protocol

#endif  // PROTOCOL_H
