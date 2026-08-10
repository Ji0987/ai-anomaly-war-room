/*
  TinyML Shadow PoC — H0 vibration capture firmware for ESP32-S3.

  Assumptions to verify against your board/module before flashing:
  - The IMU on I2C address 0x68 follows the standard MPU9250 register map.
    Some GY-91 clones use an MPU6500 instead of an MPU9250. Their accelerometer
    register map is compatible for this program, but their magnetometer differs;
    this program deliberately does not use the magnetometer.
  - PWR_MGMT_1 is 0x6B, ACCEL_CONFIG is 0x1C, and accel output starts at 0x3B.
  - ACCEL_CONFIG value 0x08 selects +/-4 g. Under the standard MPU9250 mapping,
    +/-4 g sensitivity is 8192 LSB/g, so acceleration_g = signed_raw / 8192.0.
  - I2C pins are set to GPIO8 (SDA) / GPIO9 (SCL), confirmed from the ATK-
    DNESP32S3M-MiniBoard's labeled pinout diagram (both are broken out on the
    header) and matching the arduino-esp32 core's own ESP32-S3 Dev Module I2C
    defaults. If you flash a different ESP32-S3 board, check its pinout before
    trusting these two numbers — do not assume they hold across boards.
  - Sampling is scheduled with micros() against an advancing target timestamp,
    rather than delay(), to avoid cumulative delay error. When a deadline is
    missed (for example while JSON is sent at 115200 baud), missed ticks are
    skipped instead of being captured as an artificial burst.
*/

#include <Arduino.h>
#include <Wire.h>

constexpr uint8_t MPU_ADDRESS = 0x68;
constexpr uint8_t REG_PWR_MGMT_1 = 0x6B;
constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
constexpr uint8_t REG_WHO_AM_I = 0x75;

constexpr int I2C_SDA_PIN = 8;  // ATK-DNESP32S3M-MiniBoard: confirmed on header, see file header note.
constexpr int I2C_SCL_PIN = 9;  // Same board; change both if you flash different hardware.
constexpr uint32_t I2C_CLOCK_HZ = 400000;

constexpr size_t WINDOW_SAMPLES = 1024;
constexpr uint32_t TARGET_SAMPLE_RATE_HZ = 1000;
constexpr uint32_t SAMPLE_PERIOD_US = 1000000UL / TARGET_SAMPLE_RATE_HZ;
constexpr float ACCEL_LSB_PER_G = 8192.0f;

float axG[WINDOW_SAMPLES];
float ayG[WINDOW_SAMPLES];
float azG[WINDOW_SAMPLES];

bool sensorReady = false;
bool streaming = false;
size_t sampleCount = 0;
uint32_t nextSampleUs = 0;
uint32_t firstSampleUs = 0;
uint32_t lastSampleUs = 0;
uint32_t windowsSent = 0;
uint8_t whoAmI = 0;

char commandBuffer[32];
size_t commandLength = 0;

bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission(true) == 0;
}

bool readRegister(uint8_t reg, uint8_t *value) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(MPU_ADDRESS, static_cast<uint8_t>(1), static_cast<uint8_t>(true)) != 1) {
    return false;
  }
  *value = Wire.read();
  return true;
}

bool readAccelerationG(float *ax, float *ay, float *az) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(REG_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const uint8_t bytesRequested = 6;
  if (Wire.requestFrom(MPU_ADDRESS, bytesRequested, static_cast<uint8_t>(true)) != bytesRequested) {
    return false;
  }

  const int16_t rawX = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  const int16_t rawY = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  const int16_t rawZ = static_cast<int16_t>((Wire.read() << 8) | Wire.read());

  // Standard MPU9250 +/-4 g conversion: g = signed 16-bit raw value / 8192 LSB/g.
  *ax = static_cast<float>(rawX) / ACCEL_LSB_PER_G;
  *ay = static_cast<float>(rawY) / ACCEL_LSB_PER_G;
  *az = static_cast<float>(rawZ) / ACCEL_LSB_PER_G;
  return true;
}

void sendStatus(const char *state) {
  Serial.print(F("{\"type\":\"status\",\"state\":\""));
  Serial.print(state);
  Serial.print(F("\",\"streaming\":"));
  Serial.print(streaming ? F("true") : F("false"));
  Serial.print(F(",\"windows_sent\":"));
  Serial.print(windowsSent);
  Serial.print(F(",\"sample_rate_hz\":"));
  Serial.print(TARGET_SAMPLE_RATE_HZ);
  Serial.print(F(",\"window_samples\":"));
  Serial.print(WINDOW_SAMPLES);
  Serial.print(F(",\"i2c_address\":\"0x68\",\"who_am_i\":"));
  Serial.print(whoAmI);
  Serial.println(F("}"));
}

void sendError(const char *message) {
  Serial.print(F("{\"type\":\"error\",\"message\":\""));
  Serial.print(message);
  Serial.println(F("\"}"));
}

void sendWindow(float actualSampleRateHz) {
  Serial.print(F("{\"type\":\"window\",\"window_index\":"));
  Serial.print(windowsSent + 1);
  Serial.print(F(",\"sample_rate_hz\":"));
  Serial.print(actualSampleRateHz, 4);

  const float *axes[] = {axG, ayG, azG};
  const char *axisNames[] = {"ax_g", "ay_g", "az_g"};
  for (uint8_t axis = 0; axis < 3; ++axis) {
    Serial.print(F(",\""));
    Serial.print(axisNames[axis]);
    Serial.print(F("\":["));
    for (size_t i = 0; i < WINDOW_SAMPLES; ++i) {
      if (i != 0) {
        Serial.print(',');
      }
      // Arduino's precision argument produces four decimal places, limiting JSON size.
      Serial.print(axes[axis][i], 4);
    }
    Serial.print(']');
  }
  Serial.println(F("}"));
  ++windowsSent;
}

void startStreaming() {
  if (!sensorReady) {
    sendError("sensor_not_ready");
    return;
  }
  sampleCount = 0;
  streaming = true;
  nextSampleUs = micros();
  sendStatus("streaming");
}

void stopStreaming() {
  streaming = false;
  sampleCount = 0;  // A partial window is intentionally not emitted.
  sendStatus("stopped");
}

void handleCommand(const char *command) {
  if (strcmp(command, "start") == 0) {
    startStreaming();
  } else if (strcmp(command, "stop") == 0) {
    stopStreaming();
  } else if (strcmp(command, "status") == 0) {
    sendStatus(sensorReady ? "ready" : "sensor_init_failed");
  } else if (command[0] != '\0') {
    sendError("unknown_command");
  }
}

void pollSerialCommands() {
  while (Serial.available() > 0) {
    const char incoming = static_cast<char>(Serial.read());
    if (incoming == '\r') {
      continue;
    }
    if (incoming == '\n') {
      commandBuffer[commandLength] = '\0';
      handleCommand(commandBuffer);
      commandLength = 0;
    } else if (commandLength < sizeof(commandBuffer) - 1) {
      commandBuffer[commandLength++] = incoming;
    } else {
      commandLength = 0;
      sendError("command_too_long");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);  // GPIO8/9 for ATK-DNESP32S3M-MiniBoard; edit above for other boards.
  Wire.setClock(I2C_CLOCK_HZ);

  const bool wakeOk = writeRegister(REG_PWR_MGMT_1, 0x00);
  // ACCEL_CONFIG FS_SEL = 1 (bits 4:3 = 01) selects +/-4 g in the MPU9250 map.
  const bool rangeOk = writeRegister(REG_ACCEL_CONFIG, 0x08);
  const bool idOk = readRegister(REG_WHO_AM_I, &whoAmI);
  sensorReady = wakeOk && rangeOk && idOk;

  sendStatus(sensorReady ? "ready" : "sensor_init_failed");
}

void loop() {
  pollSerialCommands();
  if (!streaming) {
    return;
  }

  const uint32_t captureStartUs = micros();
  if (static_cast<int32_t>(captureStartUs - nextSampleUs) < 0) {
    return;
  }

  float ax;
  float ay;
  float az;
  if (!readAccelerationG(&ax, &ay, &az)) {
    streaming = false;
    sampleCount = 0;
    sendError("accelerometer_read_failed");
    sendStatus("stopped_after_i2c_error");
    return;
  }

  if (sampleCount == 0) {
    firstSampleUs = captureStartUs;
  }
  lastSampleUs = captureStartUs;
  axG[sampleCount] = ax;
  ayG[sampleCount] = ay;
  azG[sampleCount] = az;
  ++sampleCount;

  if (sampleCount == WINDOW_SAMPLES) {
    const uint32_t elapsedUs = lastSampleUs - firstSampleUs;
    const float actualSampleRateHz = elapsedUs > 0
        ? (1000000.0f * static_cast<float>(WINDOW_SAMPLES - 1) / static_cast<float>(elapsedUs))
        : 0.0f;
    sendWindow(actualSampleRateHz);
    sampleCount = 0;
    // Serializing a full JSON window at 115200 baud can take noticeable time.
    // Restart the schedule after it, rather than immediately trying to catch up.
    nextSampleUs = micros() + SAMPLE_PERIOD_US;
  } else {
    // Advance from the previous target, then skip missed deadlines without burst sampling.
    const uint32_t afterReadUs = micros();
    do {
      nextSampleUs += SAMPLE_PERIOD_US;
    } while (static_cast<int32_t>(afterReadUs - nextSampleUs) >= 0);
  }
}
