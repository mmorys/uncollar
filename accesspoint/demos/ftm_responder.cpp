#include <Arduino.h>
#include <WiFi.h>

static const char AP_SSID[]  = "FTM_Responder";
static const int  AP_CHANNEL = 1;

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("FTM Responder — starting");

    WiFi.mode(WIFI_AP);

    // Last parameter enables the 802.11mc FTM responder on this AP.
    // softAP(ssid, passphrase, channel, hidden, max_connections, ftm_responder)
    if (!WiFi.softAP(AP_SSID, nullptr, AP_CHANNEL, 0, 4, true)) {
        Serial.println("ERROR: softAP() failed — halting");
        while (true) delay(1000);
    }

    Serial.printf("AP up  SSID=%s  channel=%d  IP=%s\n",
                  AP_SSID, AP_CHANNEL,
                  WiFi.softAPIP().toString().c_str());
    Serial.println("FTM responder ready. Waiting for initiator...");
}

void loop() {
    // FTM exchanges are handled entirely by the Wi-Fi driver; nothing to do here.
    delay(5000);
    Serial.printf("[heartbeat] uptime=%lus\n", millis() / 1000);
}
