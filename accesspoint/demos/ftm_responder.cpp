#include <Arduino.h>
#include <WiFi.h>

// This single source is flashed to every anchor board. Each board's identity
// (SSID) and channel are injected at build time so the same firmware can run on
// multiple responders — see the ftm_responder_* environments in platformio.ini.
#ifndef ANCHOR_NAME
#define ANCHOR_NAME "FTM_Anchor_X"
#endif
#ifndef ANCHOR_CHANNEL
#define ANCHOR_CHANNEL 1
#endif

static const char AP_SSID[]  = ANCHOR_NAME;
static const int  AP_CHANNEL = ANCHOR_CHANNEL;

void setup() {
    Serial.begin(115200);
    // Don't block forever if no monitor is attached — anchors run headless.
    uint32_t t0 = millis();
    while (!Serial && millis() - t0 < 2000) delay(10);

    Serial.println("FTM Responder — starting");

    WiFi.mode(WIFI_AP);

    // Last parameter enables the 802.11mc FTM responder on this AP.
    // softAP(ssid, passphrase, channel, hidden, max_connections, ftm_responder)
    if (!WiFi.softAP(AP_SSID, nullptr, AP_CHANNEL, 0, 4, true)) {
        Serial.println("ERROR: softAP() failed — halting");
        while (true) delay(1000);
    }

    // The initiator identifies anchors by BSSID, so print ours for reference.
    Serial.printf("AP up  SSID=%s  channel=%d  BSSID=%s  IP=%s\n",
                  AP_SSID, AP_CHANNEL,
                  WiFi.softAPmacAddress().c_str(),
                  WiFi.softAPIP().toString().c_str());
    Serial.println("FTM responder ready. Waiting for initiator...");
}

void loop() {
    // FTM exchanges are handled entirely by the Wi-Fi driver; nothing to do here.
    delay(5000);
    Serial.printf("[heartbeat] uptime=%lus\n", millis() / 1000);
}
