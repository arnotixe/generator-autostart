/**
   Arno's Generator watcher and commander
   forked off the ESP32 BasicHttpClient.ino example code

   GeneratorController.ino

   FEATURES:
   * 3 inputs for generator status:
    - Mains OK
    - Generator producing power (running)
    - Generator connected (supplying instead of mains)
    
   * 3 LED outputs showing status
    - Green  = Mains OK / NOK status
    - Red    = Generator running and producing voltage
    - Yellow = Steady light: Mains NOK and yet Generator not connected. Either didn't start, isn't producing power, or something went wrong connecting.
               2 short blinks: Wifi connection problem, or server didn't respond OK to the report.
    
   * 2 relay outputs for controlling the generator
    - MANual  = In the case of an NC Mains OK detection circuit, connect in series to Manually command the generator to start
    - INHibit = In the case of an NC Mains OK detection circuit, connect in parallel to Inhibit the generator from starting

   This wifi controller requires:
   * ESP32 WROOM or similar (ESP32 arduino-like controller with Wifi)
   * ESP32 libraries installed   

   the ESP talks to a server like the one in the php/ or node/ folders.

   You will need an internet server for this module to report against, where code similar to the above is hosted.
   
   Unsure what happens when the certificate runs out - probably you need to re-flash the chip with a new certificate?
   The certificate selected by the below oneliners is the root one, so it likely is valid for years to come.
   Alternatively you can go HTTP only.

   (L) Arno Teigseth 05 Oct 2024+
*/

//=== config =================================================
const char* ssid = ""; // your wifi network name
const char* password = ""; // your wifi network password

// get HTTPS certificates with a little bash scripting
// setup YOUR hostname on the following line and run lines to hopefully get a certificate to copy+paste here in the const char* ca = value
// HOST="peppam.net"
// both=`openssl s_client  -showcerts -servername $HOST -connect $HOST:443 </dev/null  |  sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p'|tr -d "\n"`
// echo $both |tr -d "\n"|sed 's/-----BEGIN/\n-----BEGIN/g'|sed 's/\(BEGIN CERTIFICATE-----\)/\1\\n/'|sed 's/\(-----END\)/\\n\1/'|grep -v ^$|tail -n 1 > cert.txt
// cat cert.txt |sed 's/^/const char* ca="/'|sed 's/$/";/'

// copy+paste output here: should look like the below [char* ca ="<value>";]
const char* ca="-----BEGIN CERTIFICATE-----\nMIIFBjCCAu6gAwIBAgIRAIp9PhPWLzDvI4a9KQdrNPgwDQYJKoZIhvcNAQELBQAwTzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2VhcmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAwWhcNMjcwMzEyMjM1OTU5WjAzMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3MgRW5jcnlwdDEMMAoGA1UEAxMDUjExMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuoe8XBsAOcvKCs3UZxD5ATylTqVhyybKUvsVAbe5KPUoHu0nsyQYOWcJDAjs4DqwO3cOvfPlOVRBDE6uQdaZdN5R2+97/1i9qLcT9t4x1fJyyXJqC4N0lZxGAGQUmfOx2SLZzaiSqhwmej/+71gFewiVgdtxD4774zEJuwm+UE1fj5F2PVqdnoPy6cRms+EGZkNIGIBloDcYmpuEMpexsr3E+BUAnSeI++JjF5ZsmydnS8TbKF5pwnnwSVzgJFDhxLyhBax7QG0AtMJBP6dYuC/FXJuluwme8f7rsIU5/agK70XEeOtlKsLPXzze41xNG/cLJyuqC0J3U095ah2H2QIDAQABo4H4MIH1MA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAgYIKwYBBQUHAwEwEgYDVR0TAQH/BAgwBgEB/wIBADAdBgNVHQ4EFgQUxc9GpOr0w8B6bJXELbBeki8m47kwHwYDVR0jBBgwFoAUebRZ5nu25eQBc4AIiMgaWPbpm24wMgYIKwYBBQUHAQEEJjAkMCIGCCsGAQUFBzAChhZodHRwOi8veDEuaS5sZW5jci5vcmcvMBMGA1UdIAQMMAowCAYGZ4EMAQIBMCcGA1UdHwQgMB4wHKAaoBiGFmh0dHA6Ly94MS5jLmxlbmNyLm9yZy8wDQYJKoZIhvcNAQELBQADggIBAE7iiV0KAxyQOND1H/lxXPjDj7I3iHpvsCUf7b632IYGjukJhM1yv4Hz/MrPU0jtvfZpQtSlET41yBOykh0FX+ou1Nj4ScOt9ZmWnO8m2OG0JAtIIE3801S0qcYhyOE2G/93ZCkXufBL713qzXnQv5C/viOykNpKqUgxdKlEC+Hi9i2DcaR1e9KUwQUZRhy5j/PEdEglKg3l9dtD4tuTm7kZtB8v32oOjzHTYw+7KdzdZiw/sBtnUfhBPORNuay4pJxmY/WrhSMdzFO2q3Gu3MUBcdo27goYKjL9CTF8j/Zz55yctUoVaneCWs/ajUX+HypkBTA+c8LGDLnWO2NKq0YD/pnARkAnYGPfUDoHR9gVSp/qRx+ZWghiDLZsMwhN1zjtSC0uBWiugF3vTNzYIEFfaPG7Ws3jDrAMMYebQ95JQ+HIBD/RPBuHRTBpqKlyDnkSHDHYPiNX3adPoPAcgdF3H2/W0rmoswMWgTlLn1Wu0mrks7/qpdWfS6PJ1jty80r2VKsM/Dj3YIDfbjXKdaFU5C+8bhfJGqU3taKauuz0wHVGT3eo6FlWkWYtbt4pgdamlwVeZEW+LM7qZEJEsMNPrfC03APKmZsJgpWCDWOKZvkZcvjVuYkQ4omYCTX5ohy+knMjdOmdH9c7SpqEWBDC86fiNex+O0XOMEZSa8DA\n-----END CERTIFICATE-----";

//=== config =================================================


// -- BELOW THIS LINE only physical setup and code below, no site configs
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#define USE_SERIAL Serial
WiFiMulti wifiMulti;

// I/O setup
// NOTE ESP32 pins 34, 35, 36 and 39 are input only pins and do not have internal pullup resistors https://forum.arduino.cc/t/input-pullup-not-working-properly/1090449/2
//                                // Arno's notes for his physical build
const int pin_man = 27;    // D27
const int pin_inh = 26;    // D26
const int pin_g = 12;      // D12 // Green LED works with just 5V, the high imp OC is enough to light it bright when connected between 12VDC and OC (!)
const int pin_r = 13;      // D13 // Red LED works only with 12VDC/OC, no lower voltage
const int pin_y = 14;      // D14 // Yellow LED works only with 12VDC/OC, no lower voltage
const int pin_mainsv = 32; // D32 // Cat pin 2b
const int pin_genv = 33;   // D33 // Cat pin 3a
const int pin_gencon = 23; // D23 // Cat pin 3b

// constants
const int trbl_wifi = 0;   // bitfield const wifi problem
const int trbl_gen = 1;    // bitfield const generator problem

// global vars
// BITFIELDS
int yellowbits = 0;        // bit 1 = wifi problem   bit 2 = generator problem
int greenbits = 0;         // bit 1 = got main volt
int redbits = 0;           // bit 1 = got gen volt


// for boot time testing of stuff. pass relay pin#
void testRelay(int rel, int endstate = 1) {
  int del = 500;

  for (int x = 0; x < 3; x++) {
    digitalWrite(rel, 0);
    delay(del);
    digitalWrite(rel, 1);
    delay(del);
  }
  digitalWrite(rel, endstate);
}

void manualON() {
  digitalWrite(pin_man, 0);
}
void manualOFF() {
  digitalWrite(pin_man, 1);
}
void inhibitON() {
  digitalWrite(pin_inh, 0);
}
void inhibitOFF() {
  digitalWrite(pin_inh, 1);
}

void green() {
  if (bitRead(greenbits, 0))  {
    
    // USE_SERIAL.println("Green ON");
    digitalWrite(pin_g, 0);
  } else {
    // USE_SERIAL.println("Green OFF");
    digitalWrite(pin_g, 1);
  }
}

void red() {
  if (bitRead(redbits, 0))  {
    digitalWrite(pin_r, 0);
  } else {
    digitalWrite(pin_r, 1);
  }
}

void yellow() {
  // BITFIELD

  // bit1
  if (bitRead(yellowbits, trbl_wifi)) {
    USE_SERIAL.println("Yellow blink - bad wifi");
    // bad wifi
    digitalWrite(pin_y, 0);
    delay(100);
    digitalWrite(pin_y, 1); // off
    delay(100);
    digitalWrite(pin_y, 0);
    delay(100);
    digitalWrite(pin_y, 1); // off
  } else {
    delay(300); // always spend 300ms
  }

  // and THEN set trouble on

  // bit2
  if (bitRead(yellowbits, trbl_gen)) {
    // USE_SERIAL.println("Yellow on - gen starting or in trouble");
    // generator trouble
    digitalWrite(pin_y, 0);
  }

  if (!bitRead(yellowbits, trbl_gen) && !bitRead(yellowbits, trbl_wifi)) {
    // USE_SERIAL.println("Yellow off - no problems");
    // no err
    digitalWrite(pin_y, 1); // off
  }
}


void setLights() {
  yellow();
  green();
  red();
}


String readInputs() {
  int genID = 1;
  int genvoltsOK = digitalRead(pin_genv) == 0 ? 1 : 0;
  int mainsOK = digitalRead(pin_mainsv) == 0 ? 1 : 0;
  int genConnected  = digitalRead(pin_gencon) == 0 ? 1 : 0;

  // gen error or startup (and mains down)
  if (!mainsOK && !genConnected) {
    bitSet(yellowbits, trbl_gen);
  } else {
    bitClear(yellowbits, trbl_gen);
  }

  // No mains + generating power + not connected(supplying): COULD report with blinks as a separate issue.
  // Ignoring for now, just using the combined "steady yellow=generator issue" above.
  // if (!mainsOK && genvoltsOK && !genConnected) {
  //    bitSet(yellowbits, 1);
  //  } else {
  //    bitClear(yellowbits, 1);
  //  }

  // mains ok
  if (mainsOK) {
    bitSet(greenbits, 0);
  } else {
    bitClear(greenbits, 0);
  }

  // genvolts ok
  if (genvoltsOK) {
    bitSet(redbits, 0);
  } else {
    bitClear(redbits, 0);
  }

  USE_SERIAL.println("MAINS ok: " + String(mainsOK ? "Y" : "N"));
  USE_SERIAL.println("GEN volt: " + String(genvoltsOK ? "Y" : "N"));
  USE_SERIAL.println("GEN connected: " + String(genConnected ? "Y" : "N"));

  String url = String("https://my.host.name/and/path/to/report.php") + String("?g=") + genID + String("&r=") + genConnected + String("&m=") + mainsOK + String("&v=") + genvoltsOK;
  return url;
}

void testLights() {
  // yes, this could be written more elegantly.
  const int wait = 500;
  USE_SERIAL.println("Test Green LED");
  digitalWrite(pin_g, 0);
  delay(wait);
  digitalWrite(pin_g, 1);
  delay(wait);
  digitalWrite(pin_g, 0);
  delay(wait);
  digitalWrite(pin_g, 1);
  
  USE_SERIAL.println("Test Red LED");
  digitalWrite(pin_r, 0);
  delay(wait);
  digitalWrite(pin_r, 1);
  delay(wait);
  digitalWrite(pin_r, 0);
  delay(wait);
  digitalWrite(pin_r, 1);

  USE_SERIAL.println("Test Yellow LED");
  digitalWrite(pin_y, 0);
  delay(wait);
  digitalWrite(pin_y, 1);
  delay(wait);
  digitalWrite(pin_y, 0);
  delay(wait);
  digitalWrite(pin_y, 1);
}

void setup() {
  // 2 outputs: Manual run relay and Inhibit run relay
  pinMode(pin_man, OUTPUT);
  digitalWrite(pin_man, 1);
  pinMode(pin_inh, OUTPUT);
  digitalWrite(pin_inh, 1);

  pinMode(pin_r, OUTPUT);
  digitalWrite(pin_r, 1);
  pinMode(pin_g, OUTPUT);
  digitalWrite(pin_g, 1);
  pinMode(pin_y, OUTPUT);
  digitalWrite(pin_y, 1);

  pinMode(pin_mainsv, INPUT_PULLUP);
  pinMode(pin_genv, INPUT_PULLUP);
  pinMode(pin_gencon, INPUT_PULLUP);

  USE_SERIAL.begin(115200);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  testLights(); // 2 blink each on boot
  readInputs();
  setLights();

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("Waiting for wifi %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  wifiMulti.addAP(ssid, password);
}

void setRelays(String manual, String inhibit) {
  if (manual == "MAN1") {
    digitalWrite(pin_man, 0); // ON
  } else {
    digitalWrite(pin_man, 1);
  }
  if (inhibit == "INH1") {
    digitalWrite(pin_inh, 0); // ON
    digitalWrite(pin_man, 1); // INH overrides man - turn OFF man if inh
  } else {
    digitalWrite(pin_inh, 1);
  }
}

// REPORTs the read generator status and GETs config / commands from the output

// REPORT
// g=1   // generator number
// r=0|1 // running gen
// m=0|1 // mains 0/1
// v=0|1 // gen volts 0/1

// FETCH (body) is 1 text line in the format
//
// MANxAUTx
//    -or-
// ERRx
//
// MANx : MAN1 = Manual start now  (for example to force test runs while mains power is OK)
//        MAN0 = Manual stop now
// 
// AUTx : AUT1 = Allow autostart   (Normal mode)
//        AUT0 = Inhibit autostart (Manually, or for example by CRON schedule to prevent start/run at night)
//
// ERRx : Any kind of error message. Not handled in this code.
// 
// NOTE server should make sure to set MAN0 in case INH1, just in case.

void loop() {
  // wait for WiFi connection
  if ((wifiMulti.run() == WL_CONNECTED)) {
    bitClear(yellowbits, trbl_wifi); // 0 wifi problem  1 gen problem

    HTTPClient http;

    USE_SERIAL.print("[HTTP] begin...\n");

    // status
    String url = readInputs();
    USE_SERIAL.println("Reporting: " + url);
    http.begin(url, ca); //HTTPS - NOTE that we must have the peppam cert installed, see above

    USE_SERIAL.print("[HTTP] GET...\n");

    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

      // server responds OK
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString(); // FETCH
        USE_SERIAL.println("Raw: " + payload); // raw

        // split into settings
        String manual = payload.substring(0, 4);
        USE_SERIAL.println("Manual: " + manual); // MAN0 MAN1 NC relay

        String inhibit = payload.substring(4, 8);
        USE_SERIAL.println("Inhibit: " + inhibit); // INH0 INH1 NO relay
        setRelays(manual, inhibit);

      }
    } else {
      USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    bitSet(yellowbits, trbl_wifi);// 0 wifi problem  1 gen problem
    // TODO Would have been cool to spool up report problems in some kind of memory with their timestamps, so that statistics could be reported later, when wifi returns.    
  }

  // wait loop:  
  // for (int i = 0; i <= 60; i++) {  // setup takes a while and is on average 8 seconds late in my real time experience.
  // Let's report just a couple seconds too short of 1 minute, and handle dupes on the server.
  for (int i = 0; i <= 51; i++) {
    readInputs();
    setLights(); // takes at least 300ms for the yellow blink
    delay(699); // 300+699 total approx 999+ ms, to let this loop count roughly seconds.
  }
}
