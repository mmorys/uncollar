// FTM Initiator for the Adafruit QT Py ESP32-S3 (mobile node).
//
// Discovers every anchor whose SSID starts with "FTM_Anchor_", ranges each one
// with an unassociated WiFi FTM burst, and shows the distances on a 16x2 I2C
// LCD — one anchor per line, in decimal metres. Serial output mirrors the same
// data for debugging and can be compiled out entirely via DEBUG_SERIAL.

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include "I2C_LCD.h"

// ---------------------------------------------------------------------------
// Debug serial. Comment out the next line to build a headless, LCD-only
// initiator: with DEBUG_SERIAL undefined the board never opens, waits on, or
// writes to the USB serial port — only the LCD is driven.
// ---------------------------------------------------------------------------
// #define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
  #define DBG(...)  Serial.printf(__VA_ARGS__)
  #define DBGLN(s)  Serial.println(s)
#else
  #define DBG(...)  ((void)0)
  #define DBGLN(s)  ((void)0)
#endif

// --- LCD: SunFounder LCD1602 @ I2C 0x27 on the STEMMA QT bus (Wire1). Pins
// match the QT Py ESP32-S3 wiring in collar/include/pins.h. ---
static const uint8_t LCD_ADDR = 0x27;
static const int      LCD_SDA  = 41; // QT Py ESP32-S3 STEMMA QT SDA
static const int      LCD_SCL  = 40; // QT Py ESP32-S3 STEMMA QT SCL
I2C_LCD lcd(LCD_ADDR, &Wire1);

// Anchors advertise SSIDs beginning with this prefix. The initiator discovers
// every matching AP, records its BSSID + channel, then loops over that list
// measuring distance to each one in turn. Matching is by BSSID, so multiple
// anchors are handled uniformly — add more boards and they appear automatically.
static const char ANCHOR_SSID_PREFIX[] = "FTM_Anchor_";

// FTM frames per burst: higher = more averages = better accuracy, but slower.
static const uint8_t FTM_FRAMES_PER_BURST = 16;
// Inter-burst gap in units of 100 ms (0 = no preference, driver picks).
static const uint8_t FTM_BURST_PERIOD = 0;
// Per-anchor wait for the driver's FTM report.
static const uint32_t FTM_TIMEOUT_MS = 6000;

struct Anchor {
    uint8_t bssid[6];
    int     channel;
    String  ssid;
    bool    have_dist; // true once a measurement has succeeded
    float   dist_m;    // last good distance estimate, metres
};

static const int MAX_ANCHORS = 8; // only the first 2 fit on a 16x2 LCD
static Anchor    anchors[MAX_ANCHORS];
static int       anchorCount = 0;

static volatile bool     ftm_report_ready = false;
static volatile bool     ftm_success      = false;
static volatile uint32_t ftm_rtt_raw_ps   = 0; // raw round-trip time, picoseconds
static volatile uint32_t ftm_rtt_est_ps   = 0; // picoseconds (IDF 5.x)
static volatile uint32_t ftm_dist_est_cm  = 0; // centimetres
static volatile uint8_t  ftm_num_frames   = 0; // FTM frames actually exchanged

static void onFtmReport(arduino_event_t *event) {
    const wifi_event_ftm_report_t &r = event->event_info.wifi_ftm_report;
    ftm_success     = (r.status == FTM_STATUS_SUCCESS);
    ftm_rtt_raw_ps  = r.rtt_raw;
    ftm_rtt_est_ps  = r.rtt_est;
    ftm_dist_est_cm = r.dist_est;
    ftm_num_frames  = r.ftm_report_num_entries;
    ftm_report_ready = true;
}

// The short anchor label shown on the LCD: the SSID with the prefix stripped,
// e.g. "FTM_Anchor_A" -> "A".
static String anchorLabel(const Anchor &a) {
    return a.ssid.substring(strlen(ANCHOR_SSID_PREFIX));
}

// Scans once and fills the global anchors[] list with every AP whose SSID
// starts with ANCHOR_SSID_PREFIX. Sorted by SSID so a given anchor always lands
// on the same LCD line. Returns the number found.
static int discoverAnchors() {
    int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);
    anchorCount = 0;
    for (int i = 0; i < n && anchorCount < MAX_ANCHORS; i++) {
        if (WiFi.SSID(i).startsWith(ANCHOR_SSID_PREFIX)) {
            memcpy(anchors[anchorCount].bssid, WiFi.BSSID(i), 6);
            anchors[anchorCount].channel   = WiFi.channel(i);
            anchors[anchorCount].ssid      = WiFi.SSID(i);
            anchors[anchorCount].have_dist = false;
            anchors[anchorCount].dist_m    = 0.0f;
            anchorCount++;
        }
    }
    WiFi.scanDelete();

    for (int i = 1; i < anchorCount; i++) { // insertion sort by SSID
        Anchor tmp = anchors[i];
        int j = i - 1;
        while (j >= 0 && anchors[j].ssid > tmp.ssid) {
            anchors[j + 1] = anchors[j];
            j--;
        }
        anchors[j + 1] = tmp;
    }
    return anchorCount;
}

// Runs one unassociated FTM burst against the given anchor (keyed by BSSID).
// Returns true on success and writes the distance estimate in metres.
static bool measureAnchor(const Anchor &a, float &dist_m_out) {
    ftm_report_ready = false;

    // initiateFTM(framesPerBurst, burstPeriod, channel, bssid). The initiator
    // does NOT associate with the AP — FTM runs unassociated.
    if (!WiFi.initiateFTM(FTM_FRAMES_PER_BURST, FTM_BURST_PERIOD,
                          a.channel, (uint8_t *)a.bssid)) {
        return false;
    }

    uint32_t deadline = millis() + FTM_TIMEOUT_MS;
    while (!ftm_report_ready && millis() < deadline) delay(10);

    if (!ftm_report_ready || !ftm_success) return false;
    dist_m_out = ftm_dist_est_cm / 100.0f;
    return true;
}

// Render one LCD line, space-padded to the full 16 chars so stale text clears.
static void renderLine(uint8_t row, const Anchor *a) {
    char line[17];
    if (a == nullptr) {
        snprintf(line, sizeof(line), "%-16s", "");
    } else {
        char body[17];
        if (a->have_dist) {
            snprintf(body, sizeof(body), "%s: %.2f m", anchorLabel(*a).c_str(), a->dist_m);
        } else {
            snprintf(body, sizeof(body), "%s: --- m", anchorLabel(*a).c_str());
        }
        snprintf(line, sizeof(line), "%-16s", body);
    }
    lcd.setCursor(0, row);
    lcd.print(line);
}

// The 16x2 LCD shows the first two anchors, one per line.
static void updateLcd() {
    renderLine(0, anchorCount > 0 ? &anchors[0] : nullptr);
    renderLine(1, anchorCount > 1 ? &anchors[1] : nullptr);
}

void setup() {
#ifdef DEBUG_SERIAL
    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && millis() - t0 < 2000) delay(10); // don't block if headless
#endif
    DBGLN("FTM Initiator (QT Py ESP32-S3 + LCD) — starting");

    Wire1.begin(LCD_SDA, LCD_SCL);
    lcd.begin(16, 2);
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("FTM scanning...");

    WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    DBG("Looking for anchors with SSID prefix '%s'...\n", ANCHOR_SSID_PREFIX);
}

void loop() {
    // Discover anchors once, then reuse the list for every measurement cycle.
    if (anchorCount == 0) {
        if (discoverAnchors() == 0) {
            DBGLN("No anchors found — retrying in 3 s");
            lcd.setCursor(0, 1);
            lcd.print("no anchors      ");
            delay(3000);
            return;
        }
        DBG("Discovered %d anchor(s):\n", anchorCount);
        for (int i = 0; i < anchorCount; i++) {
            const uint8_t *b = anchors[i].bssid;
            DBG("  [%d] %-14s  ch=%d  BSSID=%02X:%02X:%02X:%02X:%02X:%02X\n",
                i, anchors[i].ssid.c_str(), anchors[i].channel,
                b[0], b[1], b[2], b[3], b[4], b[5]);
        }
    }

    // Measure distance to each anchor in turn.
    int ok = 0;
    for (int i = 0; i < anchorCount; i++) {
        float dist_m;
        if (measureAnchor(anchors[i], dist_m)) {
            ok++;
            anchors[i].have_dist = true;
            anchors[i].dist_m    = dist_m;
            DBG("%-14s : %.2f m  (rtt_raw=%u ps  rtt_est=%u ps  dist_est=%u cm  frames=%u)\n",
                anchors[i].ssid.c_str(), dist_m,
                ftm_rtt_raw_ps, ftm_rtt_est_ps, ftm_dist_est_cm, ftm_num_frames);
        } else {
            anchors[i].have_dist = false;
            DBG("%-14s : measurement failed\n", anchors[i].ssid.c_str());
        }
    }
    updateLcd();
    DBGLN("----------------------------------------");

    // A fully failed cycle means the anchors may have rebooted or moved channel.
    if (ok == 0) {
        DBGLN("All measurements failed — re-scanning");
        anchorCount = 0;
    }

    delay(2000);
}
