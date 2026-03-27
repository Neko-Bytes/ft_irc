#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;

// Colors
#define BLACK   0x0000
#define GREEN   0x07E0
#define WHITE   0xFFFF
#define CYAN    0x07FF

// Timeout tracking variables
unsigned long lastUpdate = 0;
bool isConnected = false;

// Helper function so we don't write this code twice
void drawAwaitingScreen() {
  tft.fillScreen(BLACK);
  tft.setTextSize(3); 
  tft.setTextColor(WHITE, BLACK); 
  tft.setCursor(20, 50);
  tft.print("Awaiting Server");
  tft.setCursor(20, 100);
  tft.print("Connection...");
}

void setup() {
  Serial.begin(9600);
  
  uint16_t ID = tft.readID();
  if (ID == 0xD3D3) ID = 0x9486; 
  tft.begin(ID);
  
  tft.setRotation(1); 
  
  // Draw the initial screen
  drawAwaitingScreen();
}

void loop() {
  // 1. CHECK FOR NEW DATA
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    data.trim(); 

    if (data.startsWith("STATS:")) {
      
      // We got a message! Reset the timeout clock
      lastUpdate = millis();

      // If this is the first message after being disconnected, clear the screen
      if (!isConnected) {
        tft.fillScreen(BLACK);
        isConnected = true;
      }

      // Extract the values
      int firstPipe = data.indexOf('|');
      int secondPipe = data.indexOf('|', firstPipe + 1);
      int thirdPipe = data.indexOf('|', secondPipe + 1);

      String serverName = data.substring(6, firstPipe);
      String clients = data.substring(data.indexOf(':', firstPipe) + 1, secondPipe);
      String chans = data.substring(data.indexOf(':', secondPipe) + 1, thirdPipe);
      String uptime = data.substring(data.indexOf(':', thirdPipe) + 1);

      // --- DRAW THE UI ---
      tft.setTextSize(4); 

      tft.setCursor(20, 30);
      tft.setTextColor(CYAN, BLACK);
      tft.print(serverName); tft.print("       "); 

      tft.setCursor(20, 100);
      tft.setTextColor(WHITE, BLACK);
      tft.print("Up: "); tft.print(uptime);

      tft.setCursor(20, 170);
      tft.setTextColor(GREEN, BLACK);
      tft.print("Clients:  "); tft.print(clients); tft.print("   "); 

      tft.setCursor(20, 240);
      tft.setTextColor(GREEN, BLACK);
      tft.print("Channels: "); tft.print(chans); tft.print("   "); 
    }
  }

  // 2. TIMEOUT CHECK
  // If we think we are connected, but it has been more than 3000ms (3 seconds) 
  // since the last message arrived, trigger the disconnect!
  if (isConnected && (millis() - lastUpdate > 3000)) {
    isConnected = false;
    drawAwaitingScreen();
  }
}
