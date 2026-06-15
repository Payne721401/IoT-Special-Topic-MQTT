// Host unit tests for lib/protocol (run with: pio test -e native).
#include <unity.h>

#include "protocol.h"

using namespace protocol;

void setUp(void) {}
void tearDown(void) {}

// ---- co2_to_fan_speed: every threshold boundary ----
void test_co2_thresholds(void) {
    TEST_ASSERT_EQUAL_INT(25, co2_to_fan_speed(0));
    TEST_ASSERT_EQUAL_INT(25, co2_to_fan_speed(599));
    TEST_ASSERT_EQUAL_INT(5,  co2_to_fan_speed(600));
    TEST_ASSERT_EQUAL_INT(5,  co2_to_fan_speed(999));
    TEST_ASSERT_EQUAL_INT(4,  co2_to_fan_speed(1000));
    TEST_ASSERT_EQUAL_INT(4,  co2_to_fan_speed(1299));
    TEST_ASSERT_EQUAL_INT(3,  co2_to_fan_speed(1300));
    TEST_ASSERT_EQUAL_INT(3,  co2_to_fan_speed(1699));
    TEST_ASSERT_EQUAL_INT(2,  co2_to_fan_speed(1700));
    TEST_ASSERT_EQUAL_INT(2,  co2_to_fan_speed(1999));
    TEST_ASSERT_EQUAL_INT(0,  co2_to_fan_speed(2000));
    TEST_ASSERT_EQUAL_INT(0,  co2_to_fan_speed(5000));
}

// ---- fan_speed_to_display: dashboard remap ----
void test_fan_speed_to_display(void) {
    TEST_ASSERT_EQUAL_INT(5, fan_speed_to_display(0));
    TEST_ASSERT_EQUAL_INT(4, fan_speed_to_display(2));
    TEST_ASSERT_EQUAL_INT(3, fan_speed_to_display(3));
    TEST_ASSERT_EQUAL_INT(2, fan_speed_to_display(4));
    TEST_ASSERT_EQUAL_INT(1, fan_speed_to_display(5));
    TEST_ASSERT_EQUAL_INT(0, fan_speed_to_display(25));
}

// ---- modbus_crc: known-good S8 request frames carry their own CRC ----
void test_modbus_crc_known_vectors(void) {
    // CO2req = {0xFE,0x04,0x00,0x03,0x00,0x01, CRC_lo=0xD5, CRC_hi=0xC5}
    const uint8_t co2req[] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01};
    TEST_ASSERT_EQUAL_HEX16(0xC5D5, modbus_crc(co2req, sizeof(co2req)));

    // ABCreq = {0xFE,0x03,0x00,0x1F,0x00,0x01, CRC_lo=0xA1, CRC_hi=0xC3}
    const uint8_t abcreq[] = {0xFE, 0x03, 0x00, 0x1F, 0x00, 0x01};
    TEST_ASSERT_EQUAL_HEX16(0xC3A1, modbus_crc(abcreq, sizeof(abcreq)));
}

// ---- parse_frame: valid sensor and fan frames ----
void test_parse_valid_sensor_frame(void) {
    Frame f;
    TEST_ASSERT_TRUE(parse_frame("0x55, Temperature, SHT35, 24.5, 0xED", f));
    TEST_ASSERT_EQUAL_STRING("Temperature", f.id.c_str());
    TEST_ASSERT_EQUAL_STRING("SHT35", f.type.c_str());
    TEST_ASSERT_EQUAL_STRING("24.5", f.value.c_str());
}

void test_parse_valid_fan_frame(void) {
    Frame f;
    TEST_ASSERT_TRUE(parse_frame("55,Bike01,M_R,3,ED", f));
    TEST_ASSERT_EQUAL_STRING("Bike01", f.id.c_str());
    TEST_ASSERT_EQUAL_STRING("M_R", f.type.c_str());
    TEST_ASSERT_EQUAL_STRING("3", f.value.c_str());
}

// ---- parse_frame: malformed input is rejected ----
void test_parse_rejects_malformed(void) {
    Frame f;
    TEST_ASSERT_FALSE(parse_frame("garbage", f));
    TEST_ASSERT_FALSE(parse_frame("0x55, only, four, 0xED", f));   // 4 fields
    TEST_ASSERT_FALSE(parse_frame("99, a, b, c, 0xED", f));        // bad start marker
    TEST_ASSERT_FALSE(parse_frame("0x55, a, b, c, 0xFF", f));      // bad end marker
}

// ---- encode + round-trip ----
void test_encode_sensor_frame(void) {
    TEST_ASSERT_EQUAL_STRING(
        "0x55, Temperature, SHT35, 24.5, 0xED",
        encode_sensor_frame("Temperature", "SHT35", "24.5").c_str());
}

void test_encode_fan_frame(void) {
    TEST_ASSERT_EQUAL_STRING("55,Bike01,M_R,0,ED", encode_fan_frame('R', 0).c_str());
    TEST_ASSERT_EQUAL_STRING("55,Bike01,M_L,25,ED", encode_fan_frame('L', 25).c_str());
}

void test_round_trip(void) {
    const std::string wire = encode_sensor_frame("0xD11704d", "S8", "823");
    Frame f;
    TEST_ASSERT_TRUE(parse_frame(wire, f));
    TEST_ASSERT_EQUAL_STRING("0xD11704d", f.id.c_str());
    TEST_ASSERT_EQUAL_STRING("S8", f.type.c_str());
    TEST_ASSERT_EQUAL_STRING("823", f.value.c_str());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_co2_thresholds);
    RUN_TEST(test_fan_speed_to_display);
    RUN_TEST(test_modbus_crc_known_vectors);
    RUN_TEST(test_parse_valid_sensor_frame);
    RUN_TEST(test_parse_valid_fan_frame);
    RUN_TEST(test_parse_rejects_malformed);
    RUN_TEST(test_encode_sensor_frame);
    RUN_TEST(test_encode_fan_frame);
    RUN_TEST(test_round_trip);
    return UNITY_END();
}
