#include <WiFi.h>
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

// ============ БУФЕР ВВОДА ============
String inputBuffer = "";
bool commandReady = false;

// ============ СОСТОЯНИЯ WIFI ============
enum WiFiState {
  WIFI_IDLE,
  WIFI_SCANNING,
  WIFI_SELECTING,
  WIFI_ENTER_PASSWORD
};
WiFiState wifiState = WIFI_IDLE;

String wifiSSID = "";
int networkCount = 0;
String networks[20];

// ============ ВЫВОД В ОБА КАНАЛА ============
void termPrint(String text) {
  Serial.print(text);
  SerialBT.print(text);
}

void termPrintln(String text) {
  Serial.println(text);
  SerialBT.println(text);
}

void termPrintln() {
  Serial.println();
  SerialBT.println();
}

void termWrite(char c) {
  Serial.write(c);
  SerialBT.write(c);
}

// ============ ИНИЦИАЛИЗАЦИЯ ============
void setup() {
  Serial.begin(115200);
  SerialBT.begin("FatonOS");
  
  delay(2000);
  
  termPrintln();
  termPrintln("================================");
  termPrintln("   Foton OS v0.3");
  termPrintln("   Dual Output: USB + BT");
  termPrintln("================================");
  termPrintln("Type 'help' for commands");
  termPrintln();
  termPrint("faton> ");
}

// ============ ГЛАВНЫЙ ЦИКЛ ============
void loop() {
  // Ввод из USB
  while (Serial.available()) {
    char c = Serial.read();
    processInput(c);
  }
  
  // Ввод из Bluetooth
  while (SerialBT.available()) {
    char c = SerialBT.read();
    processInput(c);
  }
  
  // Обработка команды
  if (commandReady) {
    processCommand(inputBuffer);
    inputBuffer = "";
    commandReady = false;
    showPrompt();
  }
  
  systemMonitor();
}

// ============ ОБРАБОТКА ВВОДА ============
void processInput(char c) {
  if (c == '\n' || c == '\r') {
    if (inputBuffer.length() > 0) {
      termPrintln();
      commandReady = true;
    }
  } else if (c == '\b' || c == 127) {
    if (inputBuffer.length() > 0) {
      inputBuffer.remove(inputBuffer.length() - 1);
      termWrite(8);
      termWrite(32);
      termWrite(8);
    }
  } else {
    inputBuffer += c;
    termWrite(c);
  }
}

// ============ ПРИГЛАШЕНИЕ ============
void showPrompt() {
  switch (wifiState) {
    case WIFI_SELECTING:
      termPrint("wifi-select> ");
      break;
    case WIFI_ENTER_PASSWORD:
      termPrint("password> ");
      break;
    default:
      termPrint("faton> ");
      break;
  }
}

// ============ ОБРАБОТЧИК КОМАНД ============
void processCommand(String cmd) {
  cmd.trim();
  
  if (wifiState == WIFI_SELECTING) {
    selectNetwork(cmd);
    return;
  }
  if (wifiState == WIFI_ENTER_PASSWORD) {
    connectToNetwork(cmd);
    return;
  }
  
  if (cmd == "help") {
    showHelp();
  }
  else if (cmd == "sys-i") {
    showSysInfo();
  }
  else if (cmd == "conn") {
    startWiFiConnect();
  }
  else if (cmd == "clear") {
    for (int i = 0; i < 20; i++) termPrintln();
  }
  else if (cmd == "reboot") {
    termPrintln("Rebooting...");
    delay(500);
    ESP.restart();
  }
  else if (cmd == "ping") {
    termPrintln("pong");
  }
  else {
    termPrint("Unknown: ");
    termPrintln(cmd);
  }
}

// ============ HELP ============
void showHelp() {
  termPrintln();
  termPrintln("=== Foton OS Commands ===");
  termPrintln(" sys-i    - System info");
  termPrintln(" conn     - Connect WiFi");
  termPrintln(" ping     - Test link");
  termPrintln(" clear    - Clear screen");
  termPrintln(" reboot   - Reboot");
  termPrintln(" help     - This help");
  termPrintln("========================");
  termPrintln();
}

// ============ SYS-I ============
void showSysInfo() {
  termPrintln();
  termPrintln("=== Foton OS v0.4 ===");
  termPrint("Chip:     ");
  termPrintln(ESP.getChipModel());
  termPrint("Cores:    ");
  termPrintln(String(ESP.getChipCores()));
  termPrint("CPU:      ");
  termPrint(String(ESP.getCpuFreqMHz()));
  termPrintln(" MHz");
  termPrint("Flash:    ");
  termPrint(String(ESP.getFlashChipSize() / 1024 / 1024));
  termPrintln(" MB");
  termPrint("Free RAM: ");
  termPrint(String(ESP.getFreeHeap() / 1024));
  termPrintln(" KB");
  termPrint("Uptime:   ");
  termPrint(String(millis() / 1000));
  termPrintln(" sec");
  
  if (WiFi.status() == WL_CONNECTED) {
    termPrint("WiFi:     ");
    termPrint(WiFi.SSID());
    termPrint(" (");
    termPrint(WiFi.localIP().toString());
    termPrintln(")");
  } else {
    termPrintln("WiFi:     Off");
  }
  
  termPrintln("======================");
  termPrintln();
}

// ============ CONN ============
void startWiFiConnect() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  termPrintln();
  termPrintln("Scanning...");
  
  networkCount = WiFi.scanNetworks();
  
  if (networkCount == 0) {
    termPrintln("No networks found");
    wifiState = WIFI_IDLE;
    return;
  }
  
  termPrintln();
  termPrintln("=== Networks ===");
  
  for (int i = 0; i < networkCount; i++) {
    networks[i] = WiFi.SSID(i);
    termPrint(" [");
    termPrint(String(i + 1));
    termPrint("] ");
    termPrint(networks[i]);
    termPrint(" (");
    termPrint(String(WiFi.RSSI(i)));
    termPrintln(" dBm)");
  }
  
  termPrintln();
  termPrintln("Enter number or SSID:");
  wifiState = WIFI_SELECTING;
}

// ============ ВЫБОР СЕТИ ============
void selectNetwork(String input) {
  int index = input.toInt();
  
  if (index > 0 && index <= networkCount) {
    wifiSSID = networks[index - 1];
  } else {
    for (int i = 0; i < networkCount; i++) {
      if (networks[i] == input) {
        wifiSSID = input;
        break;
      }
    }
  }
  
  if (wifiSSID.length() == 0) {
    termPrintln("Not found. Try again:");
    return;
  }
  
  termPrint("Selected: ");
  termPrintln(wifiSSID);
  termPrintln("Enter password (Enter = none):");
  wifiState = WIFI_ENTER_PASSWORD;
}

// ============ ПОДКЛЮЧЕНИЕ ============
void connectToNetwork(String password) {
  termPrintln();
  termPrint("Connecting to ");
  termPrint(wifiSSID);
  
  if (password.length() == 0) {
    termPrintln(" (open)");
    WiFi.begin(wifiSSID.c_str());
  } else {
    termPrintln("...");
    WiFi.begin(wifiSSID.c_str(), password.c_str());
  }
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    termPrint(".");
    attempts++;
  }
  
  termPrintln();
  
  if (WiFi.status() == WL_CONNECTED) {
    termPrintln("Connected!");
    termPrint("IP: ");
    termPrintln(WiFi.localIP().toString());
  } else {
    termPrintln("Failed");
  }
  
  wifiState = WIFI_IDLE;
  wifiSSID = "";
}

// ============ МОНИТОР ============
void systemMonitor() {
  static unsigned long lastCheck = 0;
  
  if (millis() - lastCheck > 30000) {
    if (ESP.getFreeHeap() < 50000) {
      termPrintln("\n[!] Low memory!");
    }
    lastCheck = millis();
  }
}
