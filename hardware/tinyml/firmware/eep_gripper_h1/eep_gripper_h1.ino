/*
  EEP gripper H1 -- ESP32-S3 hardware-integration firmware for the MeArm.

  Hardware assumptions to verify before connecting power:
  - PCA9685 is at I2C address 0x40 and shares the GY-91 bus on GPIO8 (SDA)
    and GPIO9 (SCL).  The PCA9685 logic supply and the ESP32-S3 I2C pull-ups
    must be 3.3 V compatible; servo power (V+) is a separate 5--6 V supply.
  - The four MG90S signal leads use PCA9685 channels 0..3 in this order:
    swap, stretch, lift, paw.  Their pulse limits/home values are deliberately
    conservative placeholders only.  H2 must calibrate them mechanically
    before using motion commands on connected hardware.
  - GPIO4 (ADC1_CH3) is reserved for the single ACS712 channel.  ACS712 needs
    a 5 V supply, so its OUT pin MUST pass through a divider before GPIO4.  A
    direct 5 V ACS712 output can damage the ESP32-S3 and is not made safe by
    software.  The current telemetry is therefore raw ADC counts in H1.
  - The GY-91 is treated as an MPU9250-compatible 6-axis IMU at 0x68, as in
    tinyml_shadow_h0. Some boards use an MPU6500; the accel/gyro registers used
    here are compatible, but the magnetometer is intentionally unused. The IMU
    trajectory estimate is a PoC only: it needs real hardware calibration and
    can drift by centimetres or more during a single movement. FK remains the
    primary position source.
  - GPIO5 is the 1-Wire data line for an optional single DS18B20 (WARROOM-2.0-
    PLAN.md: at most one probe, on the paw/gripper servo). Needs a 4.7k pull-up
    to 3.3V on the data line per the standard DS18B20 wiring. If none is wired,
    `sensors.getDeviceCount()` is 0 and ds18b20Ready simply stays false -- this
    is optional hardware, not a required subsystem. Requires the OneWire and
    DallasTemperature Arduino libraries (Library Manager: "OneWire" by Paul
    Stoffregen, "DallasTemperature" by Miles Burton). Conversion takes ~750ms
    at 12-bit resolution, so it runs on its own non-blocking ~1s cadence in
    loop(), independent of the 1kHz accel/current sampling -- do not expect
    per-window temperature samples, only a slowly-updating scalar in `status`.

  Design notes:
  - Serial commands are newline-delimited: start, stop, status, show, jog,
    center / home, release, hold, and guard.  jog also accepts the short
    s/t/l/p +/-N form from Servo_Calibration_Tool.ino (e.g. "l-25" jogs lift
    by -25 ticks).  start/stop/status/error output stays line-delimited JSON
    per the H0 convention; show/jog/center/release/hold/guard print short
    human-readable lines instead, since those are interactive calibration
    commands, not data a capture script needs to parse.
  - Sampling uses micros() and an advancing deadline.  No application-level
    delay() is used; missed deadlines are skipped instead of burst-captured.
  - PCA9685 outputs are explicitly forced fully off at boot, on release, and
    after a local command timeout -- UNLESS `hold` has disabled that timeout
    for the current session (send `guard` to re-arm it).  `hold` exists so a
    supervised H2 calibration session is not fighting a 3 s auto-release while
    fine-tuning a joint; it is not appropriate for unattended operation, and
    every fresh boot starts with the timeout armed regardless of the previous
    session's state.  This keeps uncalibrated hardware safe by default while
    still proving the PCA9685 API, I2C bus, ADC, GY-91, and telemetry path.
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <OneWire.h>
#include <DallasTemperature.h>

constexpr uint8_t MPU_ADDRESS = 0x68;
constexpr uint8_t PCA9685_ADDRESS = 0x40;
constexpr uint8_t REG_PWR_MGMT_1 = 0x6B;
constexpr uint8_t REG_GYRO_CONFIG = 0x1B;
constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
constexpr uint8_t REG_GYRO_XOUT_H = 0x43;
constexpr uint8_t REG_WHO_AM_I = 0x75;

// MPU-9250/MPU-6500 GYRO_CONFIG FS_SEL = 1 (bits 4:3 = 01) selects +/-500
// deg/s. TDK InvenSense MPU-9250 and MPU-6500 datasheets specify 65.5
// LSB/(deg/s) at this range, so raw / 65.5f is deg/s. This is not an IMU
// calibration; the zero-rate bias below still needs a stationary ZUPT sample.
constexpr uint8_t GYRO_CONFIG_500_DPS = 0x08;
constexpr float GYRO_LSB_PER_DPS = 65.5f;
// ACCEL_CONFIG below uses FS_SEL = 1 (+/-4 g), whose scale is 8192 LSB/g.
constexpr float ACCEL_LSB_PER_G = 8192.0f;
constexpr float GRAVITY_MPS2 = 9.80665f;
constexpr float DEG_TO_RAD = 0.01745329251994329577f;

constexpr int I2C_SDA_PIN = 8;
constexpr int I2C_SCL_PIN = 9;
constexpr uint32_t I2C_CLOCK_HZ = 400000;
constexpr int ACS712_ADC_PIN = 4;  // ESP32-S3 ADC1_CH3; not a strapping, PSRAM, or USB-JTAG pin.
constexpr uint8_t ADC_RESOLUTION_BITS = 12;
constexpr uint16_t ADC_MAX_READING = (1U << ADC_RESOLUTION_BITS) - 1;
constexpr int DS18B20_ONEWIRE_PIN = 5;  // Optional; see file header note. Free GPIO, not shared with I2C/ADC.
constexpr uint32_t DS18B20_CONVERSION_MS = 800;  // >=750ms datasheet spec at 12-bit, with margin.

constexpr uint32_t SERVO_PWM_FREQUENCY_HZ = 50;
constexpr uint8_t SERVO_CHANNEL_SWAP = 0;
constexpr uint8_t SERVO_CHANNEL_STRETCH = 1;
constexpr uint8_t SERVO_CHANNEL_LIFT = 2;
constexpr uint8_t SERVO_CHANNEL_PAW = 3;
constexpr uint8_t SERVO_CHANNELS[] = {
    SERVO_CHANNEL_SWAP, SERVO_CHANNEL_STRETCH, SERVO_CHANNEL_LIFT, SERVO_CHANNEL_PAW};
// Long name (used in JSON) and single-letter shorthand (used by the s/t/l/p
// jog syntax below, matching the reference Servo_Calibration_Tool.ino).
static const char *const SERVO_NAMES[] = {"swap", "stretch", "lift", "paw"};
constexpr char SERVO_SHORT_CODES[] = {'s', 't', 'l', 'p'};

// Per-axis clamp window, ordered swap/stretch/lift/paw to match
// SERVO_CHANNELS. MIN/MAX converged through the user's own current/audio
// testing (PSU current normal + no stall buzz/whine at either end): 500-2500,
// then 400-2900, then min pulled back up to 635 while max pushed to 3400
// (a standard analog servo's decoder normally just saturates past its own
// real travel rather than doing anything unsafe, so a wide firmware ceiling
// mainly costs unused headroom -- current/audio monitoring is still the
// real safety signal, this range is not), now settled at 635-3003. Applied
// to all four axes since no single axis was ever named as different; say so
// if one joint's real range should diverge from the others.
//
// HOME_US is real per-axis calibration data (2026-08-22), read back from the
// firmware's own `show`/jog confirmation output (ticks converted to us with
// this same file's pulseUsToPca9685Ticks() math, so it is self-consistent by
// construction): swap 1758us, stretch/lift 1221us, paw 2539us. This is the
// origin pose the user set the reassembled MeArm to, not a placeholder.
constexpr uint16_t SERVO_MIN_US[] = {635, 635, 635, 635};
constexpr uint16_t SERVO_MAX_US[] = {3003, 3003, 3003, 3003};
constexpr uint16_t SERVO_HOME_US[] = {1758, 1221, 1221, 2539};
constexpr uint32_t COMMAND_TIMEOUT_MS = 3000;
constexpr uint16_t PCA9685_TICKS_PER_PERIOD = 4096;
constexpr uint32_t SERVO_PWM_PERIOD_US = 1000000UL / SERVO_PWM_FREQUENCY_HZ;

// Documentation-only defaults for the intended ACS712-5A + 10k upper / 20k
// lower divider (ratio = 2/3) design.  H1 emits raw counts; do not use these for
// protection thresholds until the divider and zero-current offset are measured.
constexpr float ADC_REFERENCE_VOLTS = 3.3f;
constexpr float ACS712_DIVIDER_RATIO = 2.0f / 3.0f;
constexpr float ACS712_ZERO_SENSOR_VOLTS = 2.5f;
constexpr float ACS712_SENSITIVITY_VOLTS_PER_AMP = 0.185f;

constexpr size_t WINDOW_SAMPLES = 1024;
constexpr uint32_t TARGET_SAMPLE_RATE_HZ = 1000;
constexpr uint32_t SAMPLE_PERIOD_US = 1000000UL / TARGET_SAMPLE_RATE_HZ;

// Complementary-filter and ZUPT defaults are deliberately conservative PoC
// starting points. They need adjustment after measuring this GY-91's mounting,
// servo vibration, and zero-rate bias on the real arm.
constexpr float COMPLEMENTARY_FILTER_TIME_CONSTANT_S = 0.10f;
constexpr float ZUPT_GYRO_STILL_DPS = 10.0f;
constexpr float ZUPT_ACCEL_TOLERANCE_G = 0.08f;
constexpr uint32_t ZUPT_SERVO_SETTLE_US = 300000UL;
constexpr uint32_t ZUPT_STILL_DURATION_US = 250000UL;
constexpr uint32_t ZUPT_REQUEST_TIMEOUT_US = 3000000UL;
// This matches the dashboard's current default pulse-to-angle estimate and its
// default swap inversion. It is unverified mechanical calibration data, not an
// encoder reading; change these two constants after a physical yaw calibration.
constexpr float SWAP_US_PER_DEG = 11.11f;
constexpr float SWAP_YAW_SIGN = -1.0f;

Adafruit_PWMServoDriver pwm(PCA9685_ADDRESS, Wire);
OneWire oneWire(DS18B20_ONEWIRE_PIN);
DallasTemperature ds18b20(&oneWire);

int16_t axRaw[WINDOW_SAMPLES];
int16_t ayRaw[WINDOW_SAMPLES];
int16_t azRaw[WINDOW_SAMPLES];
int16_t gxRaw[WINDOW_SAMPLES];
int16_t gyRaw[WINDOW_SAMPLES];
int16_t gzRaw[WINDOW_SAMPLES];
uint16_t acs712Raw[WINDOW_SAMPLES];

bool imuReady = false;
bool pca9685Ready = false;
bool ds18b20Ready = false;
bool servoOutputsEnabled = false;
// Defaults on (safe): a fresh boot always starts with the auto-release timeout
// armed. `hold` turns it off for an active, supervised calibration session;
// `guard` turns it back on. This state is never persisted across a reset, on
// purpose -- every new session starts safe and must opt out explicitly.
bool commandTimeoutEnabled = true;
bool streaming = false;
bool imuTrajectoryReady = false;
bool zuptActive = false;
size_t sampleCount = 0;
uint32_t nextSampleUs = 0;
uint32_t firstSampleUs = 0;
uint32_t lastSampleUs = 0;
uint32_t windowsSent = 0;
uint8_t whoAmI = 0;
uint32_t lastServoCommandMs = 0;

bool attitudeInitialized = false;
bool zuptResetPending = false;
bool zuptStationary = false;
uint32_t lastTrajectorySampleUs = 0;
uint32_t zuptRequestUs = 0;
uint32_t zuptStationaryStartUs = 0;
uint32_t zuptBiasSamples = 0;
float gyroBiasXDps = 0.0f;
float gyroBiasYDps = 0.0f;
float gyroBiasZDps = 0.0f;
float zuptGyroSumXDps = 0.0f;
float zuptGyroSumYDps = 0.0f;
float zuptGyroSumZDps = 0.0f;
float rollRad = 0.0f;
float pitchRad = 0.0f;
float estXMeters = 0.0f;
float estYMeters = 0.0f;
float estZMeters = 0.0f;
float velocityXMps = 0.0f;
float velocityYMps = 0.0f;
float velocityZMps = 0.0f;

// Non-blocking DS18B20 state machine: idle -> requestTemperatures() -> wait
// DS18B20_CONVERSION_MS -> read result into lastTempC -> immediately request
// the next conversion. Runs continuously off the main loop's spare cycles;
// never blocks the 1kHz accel/current sampling path.
bool dsConversionPending = false;
uint32_t dsConversionStartMs = 0;
float lastTempC = NAN;

uint16_t servoOutputTicks[sizeof(SERVO_CHANNELS) / sizeof(SERVO_CHANNELS[0])] = {};
// Per-channel mirror of servoOutputsEnabled, added so status reporting can show
// which axis is actually live during H2 calibration instead of one shared flag.
bool servoChannelEnabled[sizeof(SERVO_CHANNELS) / sizeof(SERVO_CHANNELS[0])] = {};

char commandBuffer[32];
size_t commandLength = 0;

uint16_t pca9685TicksToPulseUs(uint16_t ticks);

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

bool readAccelerationRaw(int16_t *ax, int16_t *ay, int16_t *az) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(REG_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  constexpr uint8_t bytesRequested = 6;
  if (Wire.requestFrom(MPU_ADDRESS, bytesRequested, static_cast<uint8_t>(true)) != bytesRequested) {
    return false;
  }

  *ax = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  *ay = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  *az = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  return true;
}

// MPU-9250/MPU-6500 gyro outputs are the six contiguous bytes beginning at
// GYRO_XOUT_H (0x43), X/Y/Z high byte then low byte for each 16-bit value.
bool readGyroscopeRaw(int16_t *gx, int16_t *gy, int16_t *gz) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(REG_GYRO_XOUT_H);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  constexpr uint8_t bytesRequested = 6;
  if (Wire.requestFrom(MPU_ADDRESS, bytesRequested, static_cast<uint8_t>(true)) != bytesRequested) {
    return false;
  }

  *gx = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  *gy = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  *gz = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  return true;
}

void armZuptHomeReset() {
  zuptResetPending = true;
  zuptStationary = false;
  zuptRequestUs = micros();
  zuptBiasSamples = 0;
  zuptGyroSumXDps = 0.0f;
  zuptGyroSumYDps = 0.0f;
  zuptGyroSumZDps = 0.0f;
}

float swapYawRadians() {
  const float swapPulseUs = static_cast<float>(pca9685TicksToPulseUs(servoOutputTicks[0]));
  const float yawDeg = SWAP_YAW_SIGN * (swapPulseUs - SERVO_HOME_US[0]) / SWAP_US_PER_DEG;
  return yawDeg * DEG_TO_RAD;
}

void resetZuptStationaryWindow() {
  zuptStationary = false;
  zuptBiasSamples = 0;
  zuptGyroSumXDps = 0.0f;
  zuptGyroSumYDps = 0.0f;
  zuptGyroSumZDps = 0.0f;
}

void updateZupt(uint32_t captureUs, float accelMagnitudeG, float gyroXDps, float gyroYDps,
                float gyroZDps, float accelRollRad, float accelPitchRad) {
  if (!zuptResetPending) {
    return;
  }
  if (!servoOutputsEnabled ||
      static_cast<uint32_t>(captureUs - zuptRequestUs) > ZUPT_REQUEST_TIMEOUT_US) {
    zuptResetPending = false;
    resetZuptStationaryWindow();
    return;
  }
  if (static_cast<uint32_t>(captureUs - zuptRequestUs) < ZUPT_SERVO_SETTLE_US) {
    return;
  }

  const float gyroMagnitudeDps = sqrtf(gyroXDps * gyroXDps + gyroYDps * gyroYDps + gyroZDps * gyroZDps);
  const bool stationary = gyroMagnitudeDps <= ZUPT_GYRO_STILL_DPS &&
                          fabsf(accelMagnitudeG - 1.0f) <= ZUPT_ACCEL_TOLERANCE_G;
  if (!stationary) {
    resetZuptStationaryWindow();
    return;
  }

  if (!zuptStationary) {
    zuptStationary = true;
    zuptStationaryStartUs = captureUs;
  }
  zuptGyroSumXDps += gyroXDps;
  zuptGyroSumYDps += gyroYDps;
  zuptGyroSumZDps += gyroZDps;
  ++zuptBiasSamples;

  if (static_cast<uint32_t>(captureUs - zuptStationaryStartUs) < ZUPT_STILL_DURATION_US ||
      zuptBiasSamples == 0) {
    return;
  }

  gyroBiasXDps = zuptGyroSumXDps / static_cast<float>(zuptBiasSamples);
  gyroBiasYDps = zuptGyroSumYDps / static_cast<float>(zuptBiasSamples);
  gyroBiasZDps = zuptGyroSumZDps / static_cast<float>(zuptBiasSamples);
  rollRad = accelRollRad;
  pitchRad = accelPitchRad;
  velocityXMps = 0.0f;
  velocityYMps = 0.0f;
  velocityZMps = 0.0f;
  estXMeters = 0.0f;
  estYMeters = 0.0f;
  estZMeters = 0.0f;
  zuptActive = true;
  zuptResetPending = false;
  resetZuptStationaryWindow();
}

// Updates a roll/pitch complementary filter, removes gravity in the yaw-aware
// world frame, and double-integrates linear acceleration. The yaw comes from
// the commanded swap servo pulse (not the IMU); without a physical servo-angle
// calibration, X/Y are only a home-relative PoC estimate.
void updateImuTrajectory(uint32_t captureUs, int16_t ax, int16_t ay, int16_t az, int16_t gx,
                         int16_t gy, int16_t gz) {
  const float axG = static_cast<float>(ax) / ACCEL_LSB_PER_G;
  const float ayG = static_cast<float>(ay) / ACCEL_LSB_PER_G;
  const float azG = static_cast<float>(az) / ACCEL_LSB_PER_G;
  const float accelMagnitudeG = sqrtf(axG * axG + ayG * ayG + azG * azG);
  if (accelMagnitudeG < 0.10f) {
    return;
  }

  const float accelRollRad = atan2f(ayG, azG);
  const float accelPitchRad = atan2f(-axG, sqrtf(ayG * ayG + azG * azG));
  const float gyroXDps = static_cast<float>(gx) / GYRO_LSB_PER_DPS;
  const float gyroYDps = static_cast<float>(gy) / GYRO_LSB_PER_DPS;
  const float gyroZDps = static_cast<float>(gz) / GYRO_LSB_PER_DPS;

  if (!attitudeInitialized) {
    rollRad = accelRollRad;
    pitchRad = accelPitchRad;
    attitudeInitialized = true;
  }

  float dtSeconds = 0.0f;
  if (lastTrajectorySampleUs != 0) {
    dtSeconds = static_cast<float>(captureUs - lastTrajectorySampleUs) / 1000000.0f;
  }
  lastTrajectorySampleUs = captureUs;

  if (dtSeconds > 0.0f && dtSeconds <= 0.020f) {
    const float gxRadPerSec = (gyroXDps - gyroBiasXDps) * DEG_TO_RAD;
    const float gyRadPerSec = (gyroYDps - gyroBiasYDps) * DEG_TO_RAD;
    const float gzRadPerSec = (gyroZDps - gyroBiasZDps) * DEG_TO_RAD;
    const float sinRoll = sinf(rollRad);
    const float cosRoll = cosf(rollRad);
    const float rollFromGyro =
        rollRad + (gxRadPerSec + (gyRadPerSec * sinRoll + gzRadPerSec * cosRoll) * tanf(pitchRad)) * dtSeconds;
    const float pitchFromGyro =
        pitchRad + (gyRadPerSec * cosRoll - gzRadPerSec * sinRoll) * dtSeconds;
    const float alpha = COMPLEMENTARY_FILTER_TIME_CONSTANT_S /
                        (COMPLEMENTARY_FILTER_TIME_CONSTANT_S + dtSeconds);
    // Complementary filter: angle = alpha * gyro-integrated angle +
    // (1-alpha) * gravity-derived accelerometer angle.
    rollRad = alpha * rollFromGyro + (1.0f - alpha) * accelRollRad;
    pitchRad = alpha * pitchFromGyro + (1.0f - alpha) * accelPitchRad;

    const float sinPitch = sinf(pitchRad);
    const float cosPitch = cosf(pitchRad);
    const float bodyWorldX = cosPitch * axG + sinPitch * sinRoll * ayG + sinPitch * cosRoll * azG;
    const float bodyWorldY = cosRoll * ayG - sinRoll * azG;
    const float bodyWorldZ = -sinPitch * axG + cosPitch * sinRoll * ayG + cosPitch * cosRoll * azG;
    const float yawRad = swapYawRadians();
    const float cosYaw = cosf(yawRad);
    const float sinYaw = sinf(yawRad);
    const float linearXMps2 = (cosYaw * bodyWorldX - sinYaw * bodyWorldY) * GRAVITY_MPS2;
    const float linearYMps2 = (sinYaw * bodyWorldX + cosYaw * bodyWorldY) * GRAVITY_MPS2;
    const float linearZMps2 = (bodyWorldZ - 1.0f) * GRAVITY_MPS2;

    velocityXMps += linearXMps2 * dtSeconds;
    velocityYMps += linearYMps2 * dtSeconds;
    velocityZMps += linearZMps2 * dtSeconds;
    estXMeters += velocityXMps * dtSeconds;
    estYMeters += velocityYMps * dtSeconds;
    estZMeters += velocityZMps * dtSeconds;
  }

  updateZupt(captureUs, accelMagnitudeG, gyroXDps, gyroYDps, gyroZDps, accelRollRad, accelPitchRad);
}

// Advances the DS18B20 non-blocking state machine by at most one step. Safe
// to call every loop() iteration; does nothing between conversions. If no
// probe is wired, ds18b20Ready is false and this stays a harmless no-op --
// DS18B20 is optional hardware, not a required subsystem.
void pollDs18b20() {
  if (!ds18b20Ready) return;
  if (!dsConversionPending) {
    ds18b20.requestTemperatures();
    dsConversionPending = true;
    dsConversionStartMs = millis();
    return;
  }
  if (static_cast<uint32_t>(millis() - dsConversionStartMs) < DS18B20_CONVERSION_MS) {
    return;
  }
  const float reading = ds18b20.getTempCByIndex(0);
  if (reading != DEVICE_DISCONNECTED_C) {
    lastTempC = reading;
  }
  dsConversionPending = false;  // immediately eligible to start the next conversion
}

void sendStatus(const char *state);

bool disableServoOutputs() {
  if (!pca9685Ready) {
    return false;
  }

  bool allDisabled = true;
  for (uint8_t channel : SERVO_CHANNELS) {
    // In the Adafruit PCA9685 API, off = 4096 sets the channel's FULL_OFF bit.
    allDisabled = pwm.setPWM(channel, 0, 4096) == 0 && allDisabled;
  }
  return allDisabled;
}

bool releaseServoOutputs() {
  if (!disableServoOutputs()) {
    return false;
  }
  servoOutputsEnabled = false;
  for (bool &enabled : servoChannelEnabled) {
    enabled = false;
  }
  return true;
}

uint16_t pulseUsToPca9685Ticks(uint16_t pulseUs) {
  const uint32_t scaledTicks =
      static_cast<uint32_t>(pulseUs) * PCA9685_TICKS_PER_PERIOD + SERVO_PWM_PERIOD_US / 2;
  return static_cast<uint16_t>(scaledTicks / SERVO_PWM_PERIOD_US);
}

// Approximate inverse of pulseUsToPca9685Ticks(), for status reporting only.
uint16_t pca9685TicksToPulseUs(uint16_t ticks) {
  const uint32_t scaledUs =
      static_cast<uint32_t>(ticks) * SERVO_PWM_PERIOD_US + PCA9685_TICKS_PER_PERIOD / 2;
  return static_cast<uint16_t>(scaledUs / PCA9685_TICKS_PER_PERIOD);
}

uint16_t servoMinTicks(size_t servoIndex) {
  return pulseUsToPca9685Ticks(SERVO_MIN_US[servoIndex]);
}

uint16_t servoMaxTicks(size_t servoIndex) {
  return pulseUsToPca9685Ticks(SERVO_MAX_US[servoIndex]);
}

uint16_t servoHomeTicks(size_t servoIndex) {
  return pulseUsToPca9685Ticks(SERVO_HOME_US[servoIndex]);
}

bool findServoChannel(const char *name, size_t nameLength, size_t *servoIndex) {
  for (size_t i = 0; i < sizeof(SERVO_NAMES) / sizeof(SERVO_NAMES[0]); ++i) {
    if (strlen(SERVO_NAMES[i]) == nameLength && strncmp(name, SERVO_NAMES[i], nameLength) == 0) {
      *servoIndex = i;
      return true;
    }
  }
  return false;
}

bool findServoChannelByShortCode(char code, size_t *servoIndex) {
  for (size_t i = 0; i < sizeof(SERVO_SHORT_CODES) / sizeof(SERVO_SHORT_CODES[0]); ++i) {
    if (SERVO_SHORT_CODES[i] == code) {
      *servoIndex = i;
      return true;
    }
  }
  return false;
}

bool jogServo(size_t servoIndex, int32_t tickDelta) {
  if (!pca9685Ready) {
    return false;
  }

  const int32_t requestedTicks = static_cast<int32_t>(servoOutputTicks[servoIndex]) + tickDelta;
  const int32_t clampedTicks =
      constrain(requestedTicks, servoMinTicks(servoIndex), servoMaxTicks(servoIndex));
  if (pwm.setPWM(SERVO_CHANNELS[servoIndex], 0, static_cast<uint16_t>(clampedTicks)) != 0) {
    return false;
  }

  servoOutputTicks[servoIndex] = static_cast<uint16_t>(clampedTicks);
  servoOutputsEnabled = true;
  servoChannelEnabled[servoIndex] = true;
  return true;
}

bool centerServoOutputs() {
  if (!pca9685Ready) {
    return false;
  }

  bool allCentered = true;
  for (size_t i = 0; i < sizeof(SERVO_CHANNELS) / sizeof(SERVO_CHANNELS[0]); ++i) {
    allCentered = pwm.setPWM(SERVO_CHANNELS[i], 0, servoHomeTicks(i)) == 0 && allCentered;
  }
  if (!allCentered) {
    releaseServoOutputs();
    return false;
  }

  for (size_t i = 0; i < sizeof(servoOutputTicks) / sizeof(servoOutputTicks[0]); ++i) {
    servoOutputTicks[i] = servoHomeTicks(i);
    servoChannelEnabled[i] = true;
  }
  servoOutputsEnabled = true;
  return true;
}

void recordServoCommand() {
  lastServoCommandMs = millis();
}

void enforceServoCommandTimeout() {
  if (!commandTimeoutEnabled || !servoOutputsEnabled ||
      static_cast<uint32_t>(millis() - lastServoCommandMs) < COMMAND_TIMEOUT_MS) {
    return;
  }

  if (releaseServoOutputs()) {
    sendStatus("servo_timeout_released");
  }
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
  Serial.print(F(",\"gy91_ready\":"));
  Serial.print(imuReady ? F("true") : F("false"));
  Serial.print(F(",\"imu_trajectory_ready\":"));
  Serial.print(imuTrajectoryReady ? F("true") : F("false"));
  Serial.print(F(",\"zupt_active\":"));
  Serial.print(zuptActive ? F("true") : F("false"));
  Serial.print(F(",\"gy91_address\":\"0x68\",\"who_am_i\":"));
  Serial.print(whoAmI);
  Serial.print(F(",\"pca9685_ready\":"));
  Serial.print(pca9685Ready ? F("true") : F("false"));
  Serial.print(F(",\"pca9685_address\":\"0x40\",\"servo_pwm_hz\":"));
  Serial.print(SERVO_PWM_FREQUENCY_HZ);
  Serial.print(F(",\"servo_outputs_enabled\":"));
  Serial.print(servoOutputsEnabled ? F("true") : F("false"));
  Serial.print(F(",\"command_timeout_enabled\":"));
  Serial.print(commandTimeoutEnabled ? F("true") : F("false"));
  Serial.print(F(",\"servo_channels\":{\"swap\":"));
  Serial.print(SERVO_CHANNEL_SWAP);
  Serial.print(F(",\"stretch\":"));
  Serial.print(SERVO_CHANNEL_STRETCH);
  Serial.print(F(",\"lift\":"));
  Serial.print(SERVO_CHANNEL_LIFT);
  Serial.print(F(",\"paw\":"));
  Serial.print(SERVO_CHANNEL_PAW);
  Serial.print(F("}"));
  Serial.print(F(",\"servo_positions\":{"));
  for (size_t i = 0; i < sizeof(SERVO_NAMES) / sizeof(SERVO_NAMES[0]); ++i) {
    if (i != 0) {
      Serial.print(',');
    }
    Serial.print('"');
    Serial.print(SERVO_NAMES[i]);
    Serial.print(F("\":{\"ticks\":"));
    Serial.print(servoOutputTicks[i]);
    Serial.print(F(",\"pulse_us\":"));
    Serial.print(pca9685TicksToPulseUs(servoOutputTicks[i]));
    Serial.print(F(",\"enabled\":"));
    Serial.print(servoChannelEnabled[i] ? F("true") : F("false"));
    Serial.print(F(",\"min_us\":"));
    Serial.print(SERVO_MIN_US[i]);
    Serial.print(F(",\"max_us\":"));
    Serial.print(SERVO_MAX_US[i]);
    Serial.print(F(",\"home_us\":"));
    Serial.print(SERVO_HOME_US[i]);
    Serial.print('}');
  }
  Serial.print(F("}"));
  Serial.print(F(",\"acs712_adc_pin\":"));
  Serial.print(ACS712_ADC_PIN);
  Serial.print(F(",\"acs712_adc_max\":"));
  Serial.print(ADC_MAX_READING);
  Serial.print(F(",\"acs712_adc_reference_v\":"));
  Serial.print(ADC_REFERENCE_VOLTS, 3);
  Serial.print(F(",\"acs712_divider_ratio\":"));
  Serial.print(ACS712_DIVIDER_RATIO, 6);
  Serial.print(F(",\"acs712_zero_sensor_v\":"));
  Serial.print(ACS712_ZERO_SENSOR_VOLTS, 3);
  Serial.print(F(",\"acs712_sensitivity_v_per_a\":"));
  Serial.print(ACS712_SENSITIVITY_VOLTS_PER_AMP, 3);
  Serial.print(F(",\"ds18b20_ready\":"));
  Serial.print(ds18b20Ready ? F("true") : F("false"));
  Serial.print(F(",\"temp_c\":"));
  if (isnan(lastTempC)) {
    Serial.print(F("null"));
  } else {
    Serial.print(lastTempC, 2);
  }
  Serial.println(F("}"));
}

void sendError(const char *message) {
  Serial.print(F("{\"type\":\"error\",\"message\":\""));
  Serial.print(message);
  Serial.println(F("\"}"));
}

// Human-readable (non-JSON) confirmation for one axis, printed after a
// successful jog/center/release so live calibration in Serial Monitor does
// not require reading a full JSON status line to see what just moved.
// `status`/`show` still print the full JSON/table for tooling and review.
void printServoConfirmation(size_t servoIndex) {
  Serial.print(F("OK "));
  Serial.print(SERVO_NAMES[servoIndex]);
  Serial.print(F(": "));
  Serial.print(servoOutputTicks[servoIndex]);
  Serial.print(F(" ticks ("));
  Serial.print(pca9685TicksToPulseUs(servoOutputTicks[servoIndex]));
  Serial.print(F("us) ["));
  Serial.print(servoChannelEnabled[servoIndex] ? F("on") : F("off"));
  Serial.println(F("]"));
}

// Compact human-readable table of all four axes, for the `show` command.
void printAllServoPositions() {
  Serial.print(F("---- servo positions (timeout "));
  Serial.print(commandTimeoutEnabled ? F("armed") : F("HELD -- no auto-release"));
  Serial.println(F(") ----"));
  for (size_t i = 0; i < sizeof(SERVO_NAMES) / sizeof(SERVO_NAMES[0]); ++i) {
    Serial.print(SERVO_NAMES[i]);
    Serial.print(F(": "));
    Serial.print(servoOutputTicks[i]);
    Serial.print(F(" ticks ("));
    Serial.print(pca9685TicksToPulseUs(servoOutputTicks[i]));
    Serial.print(F("us) ["));
    Serial.print(servoChannelEnabled[i] ? F("on") : F("off"));
    Serial.print(F("] range "));
    Serial.print(SERVO_MIN_US[i]);
    Serial.print('-');
    Serial.print(SERVO_MAX_US[i]);
    Serial.println(F("us"));
  }
  Serial.println(F("--------------------------"));
}

void sendInt16Array(const int16_t *samples) {
  Serial.print('[');
  for (size_t i = 0; i < WINDOW_SAMPLES; ++i) {
    if (i != 0) {
      Serial.print(',');
    }
    Serial.print(samples[i]);
  }
  Serial.print(']');
}

void sendUint16Array(const uint16_t *samples) {
  Serial.print('[');
  for (size_t i = 0; i < WINDOW_SAMPLES; ++i) {
    if (i != 0) {
      Serial.print(',');
    }
    Serial.print(samples[i]);
  }
  Serial.print(']');
}

void sendWindow(float actualSampleRateHz) {
  Serial.print(F("{\"type\":\"window\",\"window_index\":"));
  Serial.print(windowsSent + 1);
  Serial.print(F(",\"sample_rate_hz\":"));
  Serial.print(actualSampleRateHz, 4);
  Serial.print(F(",\"ax_raw\":"));
  sendInt16Array(axRaw);
  Serial.print(F(",\"ay_raw\":"));
  sendInt16Array(ayRaw);
  Serial.print(F(",\"az_raw\":"));
  sendInt16Array(azRaw);
  Serial.print(F(",\"gx_raw\":"));
  sendInt16Array(gxRaw);
  Serial.print(F(",\"gy_raw\":"));
  sendInt16Array(gyRaw);
  Serial.print(F(",\"gz_raw\":"));
  sendInt16Array(gzRaw);
  Serial.print(F(",\"acs712_adc_raw\":"));
  sendUint16Array(acs712Raw);
  Serial.print(F(",\"est_x_mm\":"));
  Serial.print(estXMeters * 1000.0f, 3);
  Serial.print(F(",\"est_y_mm\":"));
  Serial.print(estYMeters * 1000.0f, 3);
  Serial.print(F(",\"est_z_mm\":"));
  Serial.print(estZMeters * 1000.0f, 3);
  Serial.println(F("}"));
  ++windowsSent;
}

void startStreaming() {
  if (!imuReady) {
    sendError("gy91_not_ready");
    return;
  }
  sampleCount = 0;
  lastTrajectorySampleUs = 0;
  streaming = true;
  nextSampleUs = micros();
  sendStatus("streaming");
}

void stopStreaming() {
  streaming = false;
  sampleCount = 0;  // A partial window is intentionally not emitted.
  sendStatus("stopped");
}

// Parses the short jog syntax shared with Servo_Calibration_Tool.ino, e.g.
// "s+10", "l-25", "p+5": one SERVO_SHORT_CODES letter, then '+' or '-', then
// a decimal tick count. Returns false (without side effects) if `command`
// does not match this shape, so the caller can fall through to other checks.
bool tryHandleShortJog(const char *command) {
  size_t servoIndex = 0;
  if (!findServoChannelByShortCode(command[0], &servoIndex)) {
    return false;
  }
  if (command[1] != '+' && command[1] != '-') {
    return false;
  }

  char *end = nullptr;
  const long parsedDelta = strtol(command + 1, &end, 10);
  if (end == command + 1 || *end != '\0' || parsedDelta == 0 || parsedDelta < -4095 ||
      parsedDelta > 4095) {
    sendError("invalid_jog_delta");
    return true;
  }

  if (!jogServo(servoIndex, static_cast<int32_t>(parsedDelta))) {
    sendError("servo_jog_failed");
    return true;
  }
  recordServoCommand();
  printServoConfirmation(servoIndex);
  return true;
}

void handleCommand(const char *command) {
  if (strcmp(command, "start") == 0) {
    startStreaming();
  } else if (strcmp(command, "stop") == 0) {
    stopStreaming();
  } else if (strcmp(command, "status") == 0) {
    sendStatus(imuReady && pca9685Ready ? "ready" : "hardware_init_degraded");
  } else if (strcmp(command, "show") == 0) {
    printAllServoPositions();
  } else if (strcmp(command, "hold") == 0) {
    commandTimeoutEnabled = false;
    Serial.println(F("OK auto-release timeout DISABLED -- servos hold position until you send"));
    Serial.println(F("   'guard', 'release', or power is cut. Do not walk away like this."));
  } else if (strcmp(command, "guard") == 0) {
    commandTimeoutEnabled = true;
    recordServoCommand();
    Serial.println(F("OK auto-release timeout ARMED"));
  } else if (strcmp(command, "center") == 0 || strcmp(command, "home") == 0) {
    if (!centerServoOutputs()) {
      sendError("servo_center_failed");
      return;
    }
    recordServoCommand();
    armZuptHomeReset();
    printAllServoPositions();
  } else if (strcmp(command, "release") == 0) {
    if (!releaseServoOutputs()) {
      sendError("servo_release_failed");
      return;
    }
    recordServoCommand();
    printAllServoPositions();
  } else if (strncmp(command, "jog:", 4) == 0) {
    const char *channelName = command + 4;
    const char *separator = strchr(channelName, ':');
    if (separator == nullptr) {
      sendError("invalid_jog_command");
      return;
    }

    size_t servoIndex = 0;
    if (!findServoChannel(channelName, static_cast<size_t>(separator - channelName), &servoIndex)) {
      sendError("invalid_servo_channel");
      return;
    }

    const char *deltaText = separator + 1;
    char *end = nullptr;
    const long parsedDelta = strtol(deltaText, &end, 10);
    if ((deltaText[0] != '+' && deltaText[0] != '-') || end == deltaText || *end != '\0' ||
        parsedDelta == 0 || parsedDelta < -4095 || parsedDelta > 4095) {
      sendError("invalid_jog_delta");
      return;
    }

    if (!jogServo(servoIndex, static_cast<int32_t>(parsedDelta))) {
      sendError("servo_jog_failed");
      return;
    }
    recordServoCommand();
    printServoConfirmation(servoIndex);
  } else if (strlen(command) >= 2 && tryHandleShortJog(command)) {
    // Handled inside tryHandleShortJog(); nothing further to do.
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
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(I2C_CLOCK_HZ);

  analogReadResolution(ADC_RESOLUTION_BITS);
  analogSetPinAttenuation(ACS712_ADC_PIN, ADC_11db);

  for (size_t i = 0; i < sizeof(servoOutputTicks) / sizeof(servoOutputTicks[0]); ++i) {
    servoOutputTicks[i] = servoHomeTicks(i);
  }

  // The verified Adafruit API initializes the selected I2C bus in begin(), then
  // setPWMFreq() applies the 50 Hz analog-servo period used by this MeArm.
  pca9685Ready = pwm.begin();
  if (pca9685Ready) {
    pwm.setPWMFreq(SERVO_PWM_FREQUENCY_HZ);
    if (!disableServoOutputs()) {
      pca9685Ready = false;
      sendError("pca9685_disable_outputs_failed");
    }
  }

  const bool wakeOk = writeRegister(REG_PWR_MGMT_1, 0x00);
  // ACCEL_CONFIG FS_SEL = 1 (bits 4:3 = 01) selects +/-4 g in the MPU9250 map.
  const bool accelRangeOk = writeRegister(REG_ACCEL_CONFIG, 0x08);
  // GYRO_CONFIG FS_SEL = 1 (bits 4:3 = 01) selects +/-500 deg/s; see the
  // GYRO_LSB_PER_DPS datasheet note beside the constant above.
  const bool gyroRangeOk = writeRegister(REG_GYRO_CONFIG, GYRO_CONFIG_500_DPS);
  const bool idOk = readRegister(REG_WHO_AM_I, &whoAmI);
  // imuReady intentionally stays accel-only (matches every H0/H1 behavior
  // before this PoC): it gates startStreaming(), which is the safety-critical
  // accel/current capture. The gyro/trajectory PoC must never be able to make
  // that path harder to start than it already was -- if GYRO_CONFIG happens
  // to fail while the accelerometer is fine, streaming should still work,
  // just without a trajectory estimate. gyroRangeOk therefore only feeds
  // imuTrajectoryReady, not imuReady.
  imuReady = wakeOk && accelRangeOk && idOk;
  imuTrajectoryReady = imuReady && gyroRangeOk;

  ds18b20.begin();
  ds18b20Ready = ds18b20.getDeviceCount() > 0;  // optional hardware; false if none wired.
  if (ds18b20Ready) ds18b20.setWaitForConversion(false);  // this file drives the timing itself.

  sendStatus(imuReady && pca9685Ready ? "ready" : "hardware_init_degraded");
}

void loop() {
  pollSerialCommands();
  enforceServoCommandTimeout();
  pollDs18b20();
  if (!streaming) {
    return;
  }

  const uint32_t captureStartUs = micros();
  if (static_cast<int32_t>(captureStartUs - nextSampleUs) < 0) {
    return;
  }

  int16_t ax;
  int16_t ay;
  int16_t az;
  if (!readAccelerationRaw(&ax, &ay, &az)) {
    streaming = false;
    sampleCount = 0;
    releaseServoOutputs();
    sendError("accelerometer_read_failed");
    sendStatus("stopped_after_i2c_error");
    return;
  }

  int16_t gx = 0;
  int16_t gy = 0;
  int16_t gz = 0;
  const bool gyroReadOk = readGyroscopeRaw(&gx, &gy, &gz);
  // Gyro data is telemetry only. A transient gyro read fault must not change
  // the existing accel-fault safety path or command any servo output; retain
  // the 1 kHz accel/current capture and mark trajectory output not-ready.
  imuTrajectoryReady = imuReady && gyroReadOk;

  if (sampleCount == 0) {
    firstSampleUs = captureStartUs;
  }
  lastSampleUs = captureStartUs;
  axRaw[sampleCount] = ax;
  ayRaw[sampleCount] = ay;
  azRaw[sampleCount] = az;
  gxRaw[sampleCount] = gx;
  gyRaw[sampleCount] = gy;
  gzRaw[sampleCount] = gz;
  acs712Raw[sampleCount] = static_cast<uint16_t>(analogRead(ACS712_ADC_PIN));
  if (gyroReadOk) {
    updateImuTrajectory(captureStartUs, ax, ay, az, gx, gy, gz);
  }
  ++sampleCount;

  if (sampleCount == WINDOW_SAMPLES) {
    const uint32_t elapsedUs = lastSampleUs - firstSampleUs;
    const float actualSampleRateHz = elapsedUs > 0
        ? (1000000.0f * static_cast<float>(WINDOW_SAMPLES - 1) / static_cast<float>(elapsedUs))
        : 0.0f;
    sendWindow(actualSampleRateHz);
    sampleCount = 0;
    // JSON serialization at 115200 baud is much slower than a 1 kHz capture.
    // Restart after a window so the next capture is not an artificial burst.
    nextSampleUs = micros() + SAMPLE_PERIOD_US;
  } else {
    const uint32_t afterReadUs = micros();
    do {
      nextSampleUs += SAMPLE_PERIOD_US;
    } while (static_cast<int32_t>(afterReadUs - nextSampleUs) >= 0);
  }
}
