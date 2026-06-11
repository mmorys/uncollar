#include <Arduino.h>
#include <WiFi.h>

static const char TARGET_SSID[] = "FTM_Responder";

// FTM frames per burst: higher = more averages = better accuracy, but slower.
static const uint8_t FTM_FRAMES_PER_BURST = 16;
// Inter-burst gap in units of 100 ms (0 = no preference, driver picks).
static const uint8_t FTM_BURST_PERIOD = 0;

static volatile bool     ftm_report_ready = false;
static volatile bool     ftm_success      = false;
static volatile uint32_t ftm_rtt_est_ps   = 0; // picoseconds (IDF 5.x)
static volatile uint32_t ftm_dist_est_cm  = 0; // centimetres

static void onFtmReport(arduino_event_t *event) {
    const wifi_event_ftm_report_t &r = event->event_info.wifi_ftm_report;
    ftm_success     = (r.status == FTM_STATUS_SUCCESS);
    ftm_rtt_est_ps  = r.rtt_est;
    ftm_dist_est_cm = r.dist_est;
    ftm_report_ready = true;
}

// Returns true and fills bssid/channel if the target AP is found in a scan.
static bool findResponder(uint8_t *bssid_out, int *channel_out) {
    int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);
    bool found = false;
    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == TARGET_SSID) {
            memcpy(bssid_out, WiFi.BSSID(i), 6);
            *channel_out = WiFi.channel(i);
            found = true;
            break;
        }
    }
    WiFi.scanDelete();
    return found;
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("FTM Initiator — starting");

    WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    Serial.printf("Looking for AP '%s'...\n", TARGET_SSID);
}

void loop() {
    uint8_t bssid[6];
    int     channel;

    if (!findResponder(bssid, &channel)) {
        Serial.printf("'%s' not found — retrying in 3 s\n", TARGET_SSID);
        delay(3000);
        return;
    }

    Serial.printf("Found AP  ch=%d  BSSID=%02X:%02X:%02X:%02X:%02X:%02X  — initiating FTM\n",
                  channel,
                  bssid[0], bssid[1], bssid[2],
                  bssid[3], bssid[4], bssid[5]);

    ftm_report_ready = false;

    // The initiator does NOT need to be associated with the AP (unassociated FTM).
    // initiateFTM(framesPerBurst, burstPeriod, channel, bssid)
    if (!WiFi.initiateFTM(FTM_FRAMES_PER_BURST, FTM_BURST_PERIOD, channel, bssid)) {
        Serial.println("initiateFTM() rejected — retrying in 3 s");
        delay(3000);
        return;
    }

    // Driver delivers the report via the registered event callback.
    const uint32_t TIMEOUT_MS = 6000;
    uint32_t deadline = millis() + TIMEOUT_MS;
    while (!ftm_report_ready && millis() < deadline) delay(10);

    if (!ftm_report_ready) {
        Serial.println("FTM timeout — no report received");
    } else if (!ftm_success) {
        Serial.println("FTM failed (peer rejected or no frames exchanged)");
    } else {
        float dist_m = ftm_dist_est_cm / 100.0f;
        Serial.printf("Distance: %.2f m  (rtt_est=%u ps  dist_est=%u cm)\n",
                      dist_m, ftm_rtt_est_ps, ftm_dist_est_cm);
    }

    delay(2000);
}
