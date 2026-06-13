#include <TFT_eSPI.h>
#include <SPI.h>

// ======================== CONFIGURATION ========================
#define HALL_SENSOR_PIN 12     //  - DIGITAL pin for A3144
#define BUTTON_PIN 0           // D3 on ESP8266 (GPIO0)
#define SPEED_BUFFER_SIZE 5
#define DEBOUNCE_TIME 50       // ms (minimum time between valid detections)
#define WHEEL_CIRCUMFERENCE 2.1 // meters

TFT_eSPI tft = TFT_eSPI();

// Display dimensions (adjust for your 2.4" display)
const uint16_t SCREEN_WIDTH = 340;
const uint16_t SCREEN_HEIGHT = 240;

enum State { STARTUP, READY, RUNNING, FINISHED };
State currentState = STARTUP;

// Metrics
float calories = 0;
float speedBuffer[SPEED_BUFFER_SIZE] = {0};
int speedBufferIndex = 0;
unsigned long sessionStartTime = 0;
unsigned long lastUpdate = 0;
volatile bool buttonPressed = false;
float maxSpeed = 0;
float avgSpeed = 0;
const float USER_WEIGHT = 55.0;  // kg

// Sensor variables
unsigned long lastDetectionTime = 0;
unsigned long lastDebounceTime = 0;
float currentSpeed = 0;
bool magnetDetected = false;

// ======================== MET CALCULATION ========================
float getDynamicMET(float speed_kmh) {
  if (speed_kmh < 5.0) return 2.5;
  else if (speed_kmh < 10.0) return 4.0;
  else if (speed_kmh < 15.0) return 6.0;
  else if (speed_kmh < 20.0) return 8.0;
  else if (speed_kmh < 25.0) return 10.0;
  else return 12.0;
}

// ======================== SPEED BUFFER ========================
void addSpeedToBuffer(float newSpeed) {
  speedBuffer[speedBufferIndex] = newSpeed;
  speedBufferIndex = (speedBufferIndex + 1) % SPEED_BUFFER_SIZE;
}

float getAverageSpeed() {
  float sum = 0;
  int count = 0;
  for (int i = 0; i < SPEED_BUFFER_SIZE; i++) {
    if (speedBuffer[i] > 0) {
      sum += speedBuffer[i];
      count++;
    }
  }
  return (count > 0) ? sum / count : 0;
}

// ======================== BUTTON INTERRUPT ========================
void IRAM_ATTR buttonISR() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 300) {
    buttonPressed = true;
  }
  last_interrupt_time = interrupt_time;
}

// ======================== UI SCREENS ========================
void showStartupScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(3);
  tft.setCursor(SCREEN_WIDTH/2 - 70, SCREEN_HEIGHT/2 - 16);
  tft.print("CYCLOMETER");
  delay(1000);
}

void drawReadyScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(3);
  tft.setCursor(SCREEN_WIDTH/2 - 35, SCREEN_HEIGHT/2 - 50);
  tft.print("READY");
  
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 10);
  tft.print("Press Button");
  tft.setCursor(SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT/2 + 40);
  tft.print("to Start");
}

void drawRunningScreen() {
  tft.fillScreen(TFT_BLACK);
  
  // Title
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(SCREEN_WIDTH/2 - 35, 10);
  tft.print("CYCLING");
  
  // Divider line
  tft.drawFastHLine(10, 40, SCREEN_WIDTH - 20, TFT_BLUE);
  
  // Speed section
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(SCREEN_WIDTH/2 - 30, 60);
  tft.print("SPEED");
  
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(4);
  tft.setCursor(SCREEN_WIDTH/2 - 50, 95);
  tft.print("0.0 km/h");
  
  // Calories section
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(SCREEN_WIDTH/2 - 45, 150);
  tft.print("CALORIES");
  
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(SCREEN_WIDTH/2 - 30, 180);
  tft.print("0.0");
  
  // Status bar
  tft.fillRect(0, 220, SCREEN_WIDTH, 30, TFT_DARKGREY);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(SCREEN_WIDTH/2 - 50, 228);
  tft.print("PRESS TO STOP");
  
  // Max speed
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(20, 260);
  tft.print("MAX SPEED:");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(110, 257);
  tft.print("0.0 km/h");
  
  // Avg speed
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(20, 290);
  tft.print("AVG SPEED:");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(110, 287);
  tft.print("0.0 km/h");
  
  maxSpeed = 0;
  avgSpeed = 0;
}

void updateRunningScreen(float currentSpeed, float currentCalories) {
  static float lastDisplaySpeed = -1;
  static float lastDisplayCalories = -1;
  
  // Update speed
  if (abs(currentSpeed - lastDisplaySpeed) > 0.1) {
    tft.fillRect(SCREEN_WIDTH/2 - 70, 95, 140, 40, TFT_BLACK);
    tft.setTextColor(currentSpeed > 0 ? TFT_GREEN : TFT_WHITE);
    tft.setTextSize(4);
    tft.setCursor(SCREEN_WIDTH/2 - 50, 95);
    tft.printf("%.1f km/h", currentSpeed);
    lastDisplaySpeed = currentSpeed;
    
    // Update max speed
    if (currentSpeed > maxSpeed) {
      maxSpeed = currentSpeed;
      tft.fillRect(110, 257, 100, 20, TFT_BLACK);
      tft.setTextColor(TFT_GREEN);
      tft.setTextSize(2);
      tft.setCursor(110, 257);
      tft.printf("%.1f", maxSpeed);
    }
  }
  
  // Update calories
  if (abs(currentCalories - lastDisplayCalories) > 0.1) {
    tft.fillRect(SCREEN_WIDTH/2 - 40, 180, 80, 30, TFT_BLACK);
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(3);
    tft.setCursor(SCREEN_WIDTH/2 - 30, 180);
    tft.printf("%.0f", currentCalories);
    lastDisplayCalories = currentCalories;
  }
  
  // Update average speed every 5 seconds
  static unsigned long lastAvgUpdate = 0;
  if (millis() - lastAvgUpdate >= 5000) {
    avgSpeed = getAverageSpeed();
    tft.fillRect(110, 287, 100, 20, TFT_BLACK);
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(2);
    tft.setCursor(110, 287);
    tft.printf("%.1f", avgSpeed);
    lastAvgUpdate = millis();
  }
}

void drawResultScreen() {
  tft.fillScreen(TFT_BLACK);
  
  // Title
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(SCREEN_WIDTH/2 - 45, 20);
  tft.print("SESSION");
  tft.setCursor(SCREEN_WIDTH/2 - 50, 45);
  tft.print("SUMMARY");
  
  // Stats boxes
  tft.drawRect(20, 80, 90, 50, TFT_BLUE);
  tft.drawRect(130, 80, 90, 50, TFT_BLUE);
  tft.drawRect(20, 150, 90, 50, TFT_BLUE);
  
  // Labels
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(40, 85);
  tft.print("Avg Speed");
  tft.setCursor(150, 85);
  tft.print("Calories");
  tft.setCursor(45, 155);
  tft.print("Max Speed");
  
  // Values
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setCursor(45, 105);
  tft.printf("%.1f", avgSpeed);
  tft.setCursor(155, 105);
  tft.printf("%.0f", calories);
  tft.setCursor(45, 175);
  tft.printf("%.1f", maxSpeed);
  
  // Units
  tft.setTextSize(1);
  tft.setCursor(65, 105);
  tft.print("km/h");
  tft.setCursor(175, 105);
  tft.print("kcal");
  tft.setCursor(65, 175);
  tft.print("km/h");
  
  // Prompt
  tft.setTextColor(TFT_ORANGE);
  tft.setTextSize(1);
  tft.setCursor(SCREEN_WIDTH/2 - 70, 270);
  tft.print("Press to Restart");
}

// ======================== SETUP ========================
void setup() {
  Serial.begin(115200);
  
  // Configure pins
  pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);  // Digital input with pull-up
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Attach button interrupt
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  // Initialize display
  tft.init();
  tft.setRotation(0); // Portrait mode (240x320)
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(1);
  
  showStartupScreen();
  currentState = READY;
  drawReadyScreen();
}

// ======================== MAIN LOOP ========================
void loop() {
  unsigned long currentMillis = millis();

  // Handle button press
  if (buttonPressed) {
    buttonPressed = false;
    
    switch (currentState) {
      case READY:
        currentState = RUNNING;
        sessionStartTime = currentMillis;
        calories = 0;
        currentSpeed = 0;
        lastDetectionTime = 0;
        for (int i = 0; i < SPEED_BUFFER_SIZE; i++) speedBuffer[i] = 0;
        drawRunningScreen();
        break;
        
      case RUNNING:
        currentState = FINISHED;
        avgSpeed = getAverageSpeed();
        drawResultScreen();
        break;
        
      case FINISHED:
        currentState = READY;
        drawReadyScreen();
        break;
        
      default:
        break;
    }
    delay(300); // Debounce
  }

  // Hall sensor processing (RUNNING state only)
  if (currentState == RUNNING) {
    bool currentState = digitalRead(HALL_SENSOR_PIN);
    
    // Detect falling edge (magnet near sensor)
    if (currentState == LOW && !magnetDetected) {
      unsigned long now = millis();
      
      // Debounce check
      if (now - lastDebounceTime > DEBOUNCE_TIME) {
        if (lastDetectionTime > 0) {
          unsigned long deltaTime = now - lastDetectionTime;
          
          // Calculate speed in km/h
          float speedMs = WHEEL_CIRCUMFERENCE / (deltaTime / 1000.0);
          currentSpeed = speedMs * 3.6;
          
          // Add to buffer for averaging
          addSpeedToBuffer(currentSpeed);
        }
        lastDetectionTime = now;
        lastDebounceTime = now;
      }
      magnetDetected = true;
    }
    else if (currentState == HIGH) {
      magnetDetected = false;
    }
  }

  // Running state updates (every second)
  if (currentState == RUNNING && currentMillis - lastUpdate >= 1000) {
    // Timeout for zero speed
    if (millis() - lastDetectionTime > 2000) {
      currentSpeed = 0;
    }
    
    // Calculate calories using averaged speed
    float smoothedSpeed = getAverageSpeed();
    float metValue = getDynamicMET(smoothedSpeed);
    calories += metValue * USER_WEIGHT / 3600.0; // Calories per second
    
    // Update display
    updateRunningScreen(smoothedSpeed, calories);
    
    lastUpdate = currentMillis;
  }
}