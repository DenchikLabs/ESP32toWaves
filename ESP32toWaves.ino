/*
 * ESP32toWaves - HID Keyboard with Web Config Interface
 * Version: 1.0
 * Author: Denis Sinkovskiy
 * License: CC BY-NC 4.0 (https://creativecommons.org/licenses/by-nc/4.0/)
 *
 * This software is licensed under the Creative Commons Attribution-NonCommercial 4.0 
 * International Public License (CC BY-NC 4.0)
 *
 * You are free to:
 * - Share — copy and redistribute the material in any medium or format
 * - Adapt — remix, transform, and build upon the material
 *
 * Under the following terms:
 * - Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
 * - NonCommercial — You may not use the material for commercial purposes.
 *
 * No additional restrictions — You may not apply legal terms or technological measures 
 * that legally restrict others from doing anything the license permits.
 *
 * This software is provided without warranties or guarantees of any kind. Other rights 
 * such as publicity, privacy, or moral rights may limit how you use the material.
 *
 * Full license text: https://creativecommons.org/licenses/by-nc/4.0/legalcode
 */


 #include <WiFi.h>
 #include <WebServer.h>
 #include <Preferences.h>
 #include "USB.h"
 #include "USBHIDKeyboard.h"
 
 USBHIDKeyboard Keyboard;
 WebServer server(80);
 Preferences prefs;
 
 const char* default_ssid = "ESP32toWaves";
 const char* default_pass = "12345678";
 
 const int resetPin = 0;
 bool disableHID = false;
 bool serialMode = false;
 bool useAPMode = true;
 
 String commandText = "killall AG_NSServer\nAG_NSServer -w -W 0-17 -D 18 -C 19";
 int delaySec = 40;
 int charDelay = 30;
 bool prependEnter = true;
 String customSSID, customPASS;
 String staSSID = "", staPASS = "";
 
 
 String staStatusMessage = "";  // Stores STA connection result
 String staIPAddress = "";      // Stores STA IP if connected
 
 
 bool darkMode = true;
 String commandLog = "";
 
 unsigned long commandStartMillis = 0;
 unsigned long nextCharMillis = 0;
 bool commandSendPending = false;
 int currentCharIndex = 0;
 
 bool pendingReboot = false;
 unsigned long rebootMillis = 0;
 
 void loadSettings() {
   prefs.begin("settings", true);
   serialMode = prefs.getBool("serialMode", false);
   commandText = prefs.getString("commands", commandText);
   delaySec = prefs.getInt("delay", delaySec);
   charDelay = prefs.getInt("charDelay", charDelay);
   prependEnter = prefs.getBool("prependEnter", prependEnter);
   disableHID = prefs.getBool("disableHID", false);
   customSSID = prefs.getString("ssid", default_ssid);
   customPASS = prefs.getString("pass", default_pass);
   staSSID = prefs.getString("sta_ssid", "");
   staPASS = prefs.getString("sta_pass", "");
   darkMode = prefs.getBool("themeDark", true);
   useAPMode = prefs.getBool("useAP", true);
   prefs.end();
 }
 
 void saveSettings() {
   prefs.begin("settings", false);
   prefs.putBool("serialMode", serialMode);
   prefs.putString("commands", commandText);
   prefs.putInt("delay", delaySec);
   prefs.putInt("charDelay", charDelay);
   prefs.putBool("prependEnter", prependEnter);
   prefs.putBool("disableHID", disableHID);
   prefs.putString("ssid", customSSID);
   prefs.putString("pass", customPASS);
   prefs.putString("sta_ssid", staSSID);
   prefs.putString("sta_pass", staPASS);
   prefs.putBool("themeDark", darkMode);
   prefs.putBool("useAP", useAPMode);
   prefs.end();
 }
 
 void startCommandSend(int secDelay) {
   if (disableHID || serialMode) return;
   commandStartMillis = millis() + secDelay * 1000UL;
   nextCharMillis = commandStartMillis;
   currentCharIndex = -1;
   commandSendPending = true;
   Keyboard.begin();
 }
 
 void processCommandSending() {
   if (!commandSendPending || millis() < nextCharMillis) return;
 
   if (currentCharIndex == -1) {
     Keyboard.press(KEY_RETURN);
     Keyboard.releaseAll();
     nextCharMillis = millis() + 200;
     currentCharIndex++;
     return;
   }
 
   if (currentCharIndex < commandText.length()) {
     char c = commandText[currentCharIndex];
     if (c == '\n' || c == '\r') {
       Keyboard.press(KEY_RETURN);
       Keyboard.releaseAll();
       nextCharMillis = millis() + 200;
     } else {
       Keyboard.print(c);
       nextCharMillis = millis() + charDelay;
     }
     currentCharIndex++;
   } else {
     Keyboard.press(KEY_RETURN);
     Keyboard.releaseAll();
     Keyboard.end();
     commandSendPending = false;
     commandLog = "Last sent at: " + String(millis() / 1000) + "s\n" + commandText;
   }
 }
 
 void handleSendNow() {
   startCommandSend(0);
   server.send(200, "text/html", "<html><body><h3>Commands sent.</h3><a href='/'>Back</a></body></html>");
 }
 
 void handleThemeToggle() {
   darkMode = !darkMode;
   saveSettings();
   server.sendHeader("Location", "/");
   server.send(302, "text/plain", "Toggled theme");
 }
 
 void handleReset() {
   prefs.begin("settings", false);
   prefs.clear();
   prefs.end();
   ESP.restart();
 }
 
 void handleUpdate() {
   if (server.hasArg("commands")) {
     commandText = server.arg("commands");
   }
   if (server.hasArg("delay")) {
     delaySec = server.arg("delay").toInt();
   }
   saveSettings();
   server.send(200, "text/html", "<html><body><h3>Settings saved.</h3><a href='/'>Back</a></body></html>");
 }
 
 void handleRebootCommand() {
   server.send(200, "text/html", "<html><body><h3>Rebooting with command in 5s...</h3></body></html>");
   commandText = "reboot";
   startCommandSend(5);
 }
 
 void handlePoweroffCommand() {
   server.send(200, "text/html", "<html><body><h3>Powering off in 5s...</h3></body></html>");
   commandText = "poweroff";
   startCommandSend(5);
 }
 
 void handleToggleSerialMode() {
   serialMode = !serialMode;
   saveSettings();
   ESP.restart();
 }
 
 void handleWiFiSave() {
   if (server.hasArg("ssid")) customSSID = server.arg("ssid");
   if (server.hasArg("pass")) customPASS = server.arg("pass");
   if (server.hasArg("sta_ssid")) staSSID = server.arg("sta_ssid");
   if (server.hasArg("sta_pass")) staPASS = server.arg("sta_pass");
   saveSettings();
   server.send(200, "text/html", "<html><body><h3>Wi-Fi settings saved. Rebooting...</h3></body></html>");
   pendingReboot = true;
   rebootMillis = millis() + 2000;
 }
 
 extern const char* helpText;
 
 void handleRoot() {
   String bg = darkMode ? "#1e1e1e" : "#fefefe";
   String fg = darkMode ? "#f0f0f0" : "#000000";
   String html = "<html><head><style>body{font-family:sans-serif;background:" + bg + ";color:" + fg + ";padding:20px;}input,textarea{width:100%;margin:5px 0;padding:10px;border-radius:5px;border:none;}button,input[type=submit]{cursor:pointer;background:#008cba;color:white;}fieldset{margin-top:20px;border:1px solid #ccc;padding:10px;}legend{font-weight:bold;} .commandBlock { margin-bottom: 10px; padding: 10px; border: 1px dashed #aaa; border-radius: 5px; }</style></head><body>";
 
   html += "<h2>ESP32toWaves Config</h2>";
   html += buildToggleControls();
 
   html += "<fieldset><legend>Command Control</legend>";
   html += "<form action='/sendNow' method='GET'><input type='submit' value='Send Now'></form>";
   html += "<form action='/update' method='POST'>";
   html += "<div id='commandContainer'>";
   html += "<div class='commandBlock'>";
   html += "<label>Commands:</label><textarea name='commands'>" + commandText + "</textarea>";
   html += "<label>Delay (sec):</label><input type='number' name='delay' value='" + String(delaySec) + "'>";
   html += "</div>";
   html += "</div>";
   html += "<button type='button' onclick='addCommandBlock()'>Add Command</button><br><br>";
   html += "<input type='submit' value='Save Command and Delay'>";
   html += "</form>";
   html += R"rawliteral(
 <script>
 let commandCount = 1;
 function addCommandBlock() {
   if (commandCount >= 5) return;
   const container = document.getElementById('commandContainer');
   const div = document.createElement('div');
   div.classList.add('commandBlock');
   div.innerHTML = `
     <label>Commands:</label><textarea name="commands"></textarea>
     <label>Delay (sec):</label><input type="number" name="delay" value="0">
   `;
   container.appendChild(div);
   commandCount++;
 }
 </script>
 )rawliteral";
   html += "</fieldset>";
 
   html += "<fieldset><legend>System Actions</legend>";
   html += "<form action='/rebootCmd' method='GET'><input type='submit' value='Reboot with Command'></form>";
   html += "<form action='/poweroffCmd' method='GET'><input type='submit' value='Power Off'></form>";
   html += "<form action='/reset' method='GET'><input type='submit' value='Reset All Settings'></form>";
   html += "</fieldset>";
 
   html += "<fieldset><legend>Wi-Fi Settings</legend>";
   html += "<form action='/wifiSave' method='POST'>";
   html += "<label>AP Mode SSID:</label><input name='ssid' value='" + customSSID + "'>";
   html += "<label>AP Mode Password:</label><input name='pass' value='" + customPASS + "'>";
 
   html += "<input type='submit' value='Save Wi-Fi Settings'>";
   html += "</form>";
   html += "<p><b>Wi-Fi Mode:</b> Access Point (AP Only)</p>";
 
   html += "</fieldset>";
 
   html += "<fieldset><legend>Command Log</legend><pre>" + commandLog + "</pre></fieldset>";
   html += "<fieldset><legend>AG_NSServer Help</legend><pre>";
   html += helpText;
   html += "</pre></fieldset>";
 
   html += "</body></html>";
   server.send(200, "text/html", html);
 }
 
 String buildToggleControls() {
   String html = "<div style='margin-bottom: 20px;'>";
   html += "<form action='/toggleTheme' method='GET'><input type='submit' value='Switch to " + String(darkMode ? "Light" : "Dark") + " Theme'></form>";
   html += "<form action='/toggleSerial' method='GET'><input type='submit' value='Switch to " + String(serialMode ? "HID Mode" : "Serial Mode") + "'></form>";
   html += "</div>";
   return html;
 }
 
 const char* helpText = R"rawliteral(
 Usage: AG_NSServer [options]
 -a    AudioGrid reserve CPU percentage override
 -W    Worker core set format: "1-7,10-12,15,17"
 -D    Master core
 -C    Control core
 -d    Debug
 -i    Interface
 -k    Kernel reserve CPU percentage override
 -L    One line SGS info
 -l    SGS case controller serial interface device
 -M    Node protocol XML (WFI style)
 -m    SGS mode: sgnf, user_space
 -o    Audio driver device
 -P    SGS info, pretty print
 -s    SG versions info file path
 -S    Print lib so cache folder path
 -v    External version & kernel arch
 -V    External version (ag_linux-release)
 -VV   External version and OS version
 -VVV  External version, OS version and arch
 -w    Watchdog
 -t    Max temp for fan control
 -H    Print supported hardware
 -h    Prints this help
 )rawliteral";
 
 void startWiFi() {
   WiFi.mode(WIFI_AP);  // AP mode only
 
   // Start AP Mode with default IP range (192.168.4.1)
   WiFi.softAP(customSSID.c_str(), customPASS.c_str());
 
   // Web server route handlers
   server.on("/", handleRoot);
   server.on("/sendNow", handleSendNow);
   server.on("/toggleTheme", handleThemeToggle);
   server.on("/reset", handleReset);
   server.on("/update", HTTP_POST, handleUpdate);
   server.on("/rebootCmd", handleRebootCommand);
   server.on("/poweroffCmd", handlePoweroffCommand);
   server.on("/toggleMode", []() {
     server.send(200, "text/html", "<html><body><h3>STA Disabled. AP-Only Mode.</h3><a href='/'>Back</a></body></html>");
   });
   server.on("/toggleSerial", handleToggleSerialMode);
   server.on("/wifiSave", HTTP_POST, handleWiFiSave);
 
   server.begin();
 }
 
 
 
 void setup() {
   Serial.begin(115200);
   USB.begin();
   loadSettings();
   startWiFi();
 
   if (!serialMode && !disableHID) {
     startCommandSend(delaySec);
   }
 }
 
 void loop() {
   server.handleClient();
   processCommandSending();
 
   if (pendingReboot && millis() >= rebootMillis) {
     ESP.restart();
   }
 }
 