#include "wifi/wifi_ui.h"
#include "wifi/my_wifi.h"

// Color definitions
#define COLOR_BG        0x0000  // Black background
#define COLOR_TEXT      0xFFFF  // White text
#define COLOR_TITLE     0x07FF  // Cyan title
#define COLOR_SUCCESS   0x07E0  // Green success
#define COLOR_WARNING   0xFD20  // Orange warning
#define COLOR_ERROR     0xF800  // Red error
#define COLOR_ACCENT    0x001F  // Blue accent
#define COLOR_LIGHT_BG  0x4208  // Light gray background

// Static variables to track state between function calls
static WiFiStatus lastDisplayedStatus = (WiFiStatus)-1; // Invalid initial value to force first draw
static unsigned long lastUpdateTime = 0;

// 全局判断变量，用于控制清屏和重绘
bool wifiUIRedrawNeeded = true;

// Display WiFi connection UI with built-in anti-flicker logic
void displayWiFiUI(WiFiStatus status) {
    unsigned long currentTime = millis();
    
    // 检测状态变化，需要完全重绘
    if (status != lastDisplayedStatus) {
        wifiUIRedrawNeeded = true;
    }
    
    // For dynamic elements, update at most every 250ms
    bool updateDynamicElements = (currentTime - lastUpdateTime >= 250);
    
    // If no update needed, return immediately
    if (!wifiUIRedrawNeeded && !updateDynamicElements) {
        return;
    }
    
    // FULL REDRAW
    if (wifiUIRedrawNeeded) {
        // Clear screen
        M5.Display.fillScreen(COLOR_BG);
        
        // Draw header - smaller to save space
        M5.Display.fillRect(0, 0, 128, 16, COLOR_LIGHT_BG);
        M5.Display.setTextColor(COLOR_TITLE);
        M5.Display.setTextSize(1);
        M5.Display.setCursor(5, 4);
        M5.Display.print("WiFi Setup");
        
        // Draw static content based on status
        if (status == WIFI_CONNECTED) {
            // ===== CONNECTED STATE =====
            
            // Draw WiFi icon - corrected orientation (arcs facing upward)
            int centerX = 16;
            int centerY = 30;
            int radius = 8;
            
            // Draw signal arcs - changed angle range from (45,135) to (225,315)
            // This makes the arcs face upward instead of downward
            for (int i = 0; i < 3; i++) {
                int arcSize = (i + 1) * radius / 3;
                M5.Display.drawArc(centerX, centerY, arcSize, arcSize, 225, 315, COLOR_SUCCESS);
            }
            
            // Draw dot in center
            M5.Display.fillCircle(centerX, centerY, 3, COLOR_SUCCESS);
            
            // Connection info - more compact
            M5.Display.setTextSize(1);
            M5.Display.setTextColor(COLOR_SUCCESS);
            M5.Display.setCursor(40, 25);
            M5.Display.print("CONNECTED");
            
            M5.Display.setTextColor(COLOR_TEXT);
            
            // SSID - truncate if too long
            String ssid = WiFi.SSID();
            if (ssid.length() > 14) {
                ssid = ssid.substring(0, 12) + "..";
            }
            M5.Display.setCursor(5, 45);
            M5.Display.printf("SSID:%s", ssid.c_str());
            
            // IP - split into two lines if needed
            String ip = getLocalIP();
            M5.Display.setCursor(5, 60);
            M5.Display.print("IP:");
            M5.Display.setCursor(5, 75);
            M5.Display.print(ip);
            
            // Signal strength as simple text
            M5.Display.setCursor(5, 90);
            M5.Display.print("Signal:");
            
            int rssi = WiFi.RSSI();
            String signal;
            if (rssi > -60) signal = "Good";
            else if (rssi > -75) signal = "OK";
            else signal = "Poor";
            
            M5.Display.setCursor(50, 90);
            M5.Display.print(signal);
            
            // Draw bottom status bar
            M5.Display.fillRect(0, 112, 128, 16, COLOR_SUCCESS);
            M5.Display.setTextColor(COLOR_BG);
            M5.Display.setCursor(5, 116);
            M5.Display.print("Hold A: Reset");
        } 
        else if (status == WIFI_AP_MODE) {
            // AP Mode info - simplified
            M5.Display.setTextSize(1);
            M5.Display.setTextColor(COLOR_WARNING);
            M5.Display.setCursor(5, 25);
            M5.Display.print("AP MODE");
            
            M5.Display.setTextColor(COLOR_TEXT);
            M5.Display.setCursor(5, 40);
            M5.Display.print("Connect to:");
            
            M5.Display.setCursor(5, 55);
            M5.Display.print(AP_NAME);
            
            M5.Display.setCursor(5, 70);
            M5.Display.print("No password");
            
            M5.Display.setCursor(5, 85);
            M5.Display.print("Open browser:");
            
            M5.Display.setCursor(5, 100);
            M5.Display.print("192.168.4.1");
            
            // Draw WiFi AP icon in corner (simplified)
            int apX = 100;
            int apY = 30;
            int radius = 8;
            
            // Draw signal arcs facing upward
            for (int i = 0; i < 3; i++) {
                int arcSize = (i + 1) * radius / 3;
                M5.Display.drawArc(apX, apY, arcSize, arcSize, 225, 315, COLOR_WARNING);
            }
            
            // Draw dot in center
            M5.Display.fillCircle(apX, apY, 3, COLOR_WARNING);
            
            // Draw bottom status bar
            M5.Display.fillRect(0, 112, 128, 16, COLOR_WARNING);
            M5.Display.setTextColor(COLOR_BG);
            M5.Display.setCursor(5, 116);
            M5.Display.print("Setup WiFi");
        }
        else if (status == WIFI_CONNECTING) {
            M5.Display.setTextSize(1);
            M5.Display.setTextColor(COLOR_TEXT);
            M5.Display.setCursor(5, 30);
            M5.Display.print("CONNECTING...");
            
            // Simplified loading indicator
            M5.Display.setCursor(5, 50);
            M5.Display.print("Please wait");
            
            // Progress bar outline
            int barWidth = 100;
            int barHeight = 8;
            int barX = 14;
            int barY = 70;
            
            M5.Display.drawRect(barX, barY, barWidth, barHeight, COLOR_TEXT);
            
            // Draw bottom status bar
            M5.Display.fillRect(0, 112, 128, 16, COLOR_ACCENT);
            M5.Display.setTextColor(COLOR_TEXT);
            M5.Display.setCursor(5, 116);
            M5.Display.print("Connecting...");
        }
        else if (status == WIFI_FAILED) {
            M5.Display.setTextSize(1);
            M5.Display.setTextColor(COLOR_ERROR);
            M5.Display.setCursor(5, 25);
            M5.Display.print("CONNECT FAILED");
            
            M5.Display.setTextColor(COLOR_TEXT);
            M5.Display.setCursor(5, 40);
            M5.Display.print("Try:");
            
            M5.Display.setCursor(5, 55);
            M5.Display.print("- Check WiFi");
            
            M5.Display.setCursor(5, 70);
            M5.Display.print("- Hold A: Reset");
            
            M5.Display.setCursor(5, 85);
            M5.Display.print("- Restart device");
            
            // Draw error icon
            int iconX = 100;
            int iconY = 30;
            int iconSize = 8;
            
            // Draw X mark
            M5.Display.drawLine(iconX-iconSize, iconY-iconSize, iconX+iconSize, iconY+iconSize, COLOR_ERROR);
            M5.Display.drawLine(iconX+iconSize, iconY-iconSize, iconX-iconSize, iconY+iconSize, COLOR_ERROR);
            
            // Draw bottom status bar
            M5.Display.fillRect(0, 112, 128, 16, COLOR_ERROR);
            M5.Display.setTextColor(COLOR_TEXT);
            M5.Display.setCursor(5, 116);
            M5.Display.print("Error");
        }
        else { // WIFI_INIT or default
            M5.Display.setTextSize(1);
            M5.Display.setTextColor(COLOR_TEXT);
            M5.Display.setCursor(5, 30);
            M5.Display.print("INITIALIZING");
            
            M5.Display.setCursor(5, 50);
            M5.Display.print("Starting WiFi...");
            
            // Progress bar outline
            int barWidth = 100;
            int barHeight = 8;
            int barX = 14;
            int barY = 70;
            
            M5.Display.drawRect(barX, barY, barWidth, barHeight, COLOR_TEXT);
            
            // Draw bottom status bar
            M5.Display.fillRect(0, 112, 128, 16, COLOR_LIGHT_BG);
            M5.Display.setTextColor(COLOR_TEXT);
            M5.Display.setCursor(5, 116);
            M5.Display.print("Please wait");
        }
        
        // Mark full redraw as complete
        wifiUIRedrawNeeded = false;
        lastDisplayedStatus = status;
    }
    
    // UPDATE DYNAMIC ELEMENTS
    if (updateDynamicElements) {
        // Only update dynamic elements for states that need animation
        if (status == WIFI_CONNECTING || status == WIFI_INIT) {
            // Progress bar parameters
            int barWidth = 100;
            int barHeight = 8;
            int barX = 14;
            int barY = 70;
            
            // Calculate progress based on time
            int maxProgress = barWidth - 2;
            int progress;
            
            // Different animation styles for different states
            if (status == WIFI_CONNECTING) {
                // Bouncing animation for connecting
                int cycleLength = maxProgress * 2;
                int position = (currentTime / 100) % cycleLength;
                progress = (position < maxProgress) ? position : cycleLength - position;
            } else {
                // Continuous fill for initializing
                progress = (currentTime / 100) % maxProgress;
            }
            
            // Clear old progress
            M5.Display.fillRect(barX + 1, barY + 1, barWidth - 2, barHeight - 2, COLOR_BG);
            
            // Draw new progress
            uint16_t progressColor = (status == WIFI_CONNECTING) ? COLOR_ACCENT : COLOR_TITLE;
            M5.Display.fillRect(barX + 1, barY + 1, progress, barHeight - 2, progressColor);
        }
        
        // Update timestamp for dynamic elements
        lastUpdateTime = currentTime;
    }
}
