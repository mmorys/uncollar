// Reads SX1276 registers over raw SPI to verify physical wiring.
// Uses Unity so `pio test` can parse the result.

#include <Arduino.h>
#include <SPI.h>
#include <unity.h>

static constexpr int kSck  = 36;
static constexpr int kMiso = 37;
static constexpr int kMosi = 35;
static constexpr int kCs   = 17;
static constexpr int kRst  = 18;

static constexpr uint8_t REG_VERSION = 0x42;  // expected 0x12 for SX1276
static constexpr uint8_t REG_OPMODE  = 0x01;  // 0x00/0xFF → no bus response

static SPIClass fspi(FSPI);

static uint8_t readReg(uint8_t addr) {
    digitalWrite(kCs, LOW);
    fspi.transfer(addr & 0x7F);  // MSB=0 → read
    uint8_t val = fspi.transfer(0x00);
    digitalWrite(kCs, HIGH);
    return val;
}

void test_version_register() {
    uint8_t ver = readReg(REG_VERSION);
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x12, ver, "RegVersion != 0x12: check MOSI/MISO/CS/power");
}

void test_opmode_not_floating() {
    uint8_t opmode = readReg(REG_OPMODE);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0x00, opmode, "RegOpMode=0x00: bus not responding");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0xFF, opmode, "RegOpMode=0xFF: bus floating (check wiring)");
}

void setup() {
    // Give USB-CDC time to enumerate before Unity starts printing.
    delay(2000);

    // Hardware reset
    pinMode(kRst, OUTPUT);
    digitalWrite(kRst, LOW);
    delay(10);
    digitalWrite(kRst, HIGH);
    delay(10);

    // Init SPI
    pinMode(kCs, OUTPUT);
    digitalWrite(kCs, HIGH);
    fspi.begin(kSck, kMiso, kMosi, kCs);
    fspi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

    UNITY_BEGIN();
    RUN_TEST(test_version_register);
    RUN_TEST(test_opmode_not_floating);
    UNITY_END();

    fspi.endTransaction();
}

void loop() {}
