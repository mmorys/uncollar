#include <Arduino.h>
#include <WiFi.h>

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
};

static const int MAX_ANCHORS = 8;
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

// Scans once and fills the global anchors[] list with every AP whose SSID
// starts with ANCHOR_SSID_PREFIX. Returns the number found.
static int discoverAnchors() {
    int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);
    anchorCount = 0;
    for (int i = 0; i < n && anchorCount < MAX_ANCHORS; i++) {
        if (WiFi.SSID(i).startsWith(ANCHOR_SSID_PREFIX)) {
            memcpy(anchors[anchorCount].bssid, WiFi.BSSID(i), 6);
            anchors[anchorCount].channel = WiFi.channel(i);
            anchors[anchorCount].ssid    = WiFi.SSID(i);
            anchorCount++;
        }
    }
    WiFi.scanDelete();
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

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("FTM Initiator — starting");

    WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    Serial.printf("Looking for anchors with SSID prefix '%s'...\n",
                  ANCHOR_SSID_PREFIX);
}

void loop() {
    // Discover anchors once, then reuse the list for every measurement cycle.
    if (anchorCount == 0) {
        if (discoverAnchors() == 0) {
            Serial.println("No anchors found — retrying in 3 s");
            delay(3000);
            return;
        }
        Serial.printf("Discovered %d anchor(s):\n", anchorCount);
        for (int i = 0; i < anchorCount; i++) {
            const uint8_t *b = anchors[i].bssid;
            Serial.printf("  [%d] %-14s  ch=%d  BSSID=%02X:%02X:%02X:%02X:%02X:%02X\n",
                          i, anchors[i].ssid.c_str(), anchors[i].channel,
                          b[0], b[1], b[2], b[3], b[4], b[5]);
        }
    }

    // Measure distance to each anchor in turn.
    for (int i = 0; i < anchorCount; i++) {
        float dist_m;
        if (measureAnchor(anchors[i], dist_m)) {
            Serial.printf("%-14s : %.2f m  (rtt_raw=%u ps  rtt_est=%u ps  dist_est=%u cm  frames=%u)\n",
                          anchors[i].ssid.c_str(), dist_m,
                          ftm_rtt_raw_ps, ftm_rtt_est_ps, ftm_dist_est_cm,
                          ftm_num_frames);
        } else {
            Serial.printf("%-14s : measurement failed\n",
                          anchors[i].ssid.c_str());
        }
    }
    Serial.println("----------------------------------------");

    delay(2000);
}
