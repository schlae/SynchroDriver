/**
 * Drive the FDAI controller with flight data provided over the serial line.
 * Ken Shirriff, righto.com
*/

IntervalTimer myTimer;  // Interrupt to update the PWM levels

volatile unsigned long count = 0;  // Index into the sine wave

// Analog inputs from pots
#define ROLL A0
#define PITCH A1
#define YAW A2
#define ROLL_RATE A3
#define PITCH_RATE A4
#define YAW_RATE A5
#define ROLL_ERR A6
#define PITCH_ERR A7
#define YAW_ERR A8

// Outputs
#define PWM_R0 0
#define PWM_R1 1
#define PWM_R2 2
#define PWM_P0 3
#define PWM_P1 4
#define PWM_P2 5
#define PWM_Y0 6
#define PWM_Y1 7
#define PWM_Y2 8
#define PWM_RR 9
#define PWM_RE 10
#define PWM_PR 11
#define PWM_PE 12
#define PWM_YR 24
#define PWM_YE 25
#define FLAG 26

// Sync input
#define SYNC 30

// Sine wave, arbitrarily 100 samples long.
float sine[100] = {
  0.0000,
  0.0628,
  0.1253,
  0.1874,
  0.2487,
  0.3090,
  0.3681,
  0.4258,
  0.4818,
  0.5358,
  0.5878,
  0.6374,
  0.6845,
  0.7290,
  0.7705,
  0.8090,
  0.8443,
  0.8763,
  0.9048,
  0.9298,
  0.9511,
  0.9686,
  0.9823,
  0.9921,
  0.9980,
  1.0000,
  0.9980,
  0.9921,
  0.9823,
  0.9686,
  0.9511,
  0.9298,
  0.9048,
  0.8763,
  0.8443,
  0.8090,
  0.7705,
  0.7290,
  0.6845,
  0.6374,
  0.5878,
  0.5358,
  0.4818,
  0.4258,
  0.3681,
  0.3090,
  0.2487,
  0.1874,
  0.1253,
  0.0628,
  0.0000,
  -0.0628,
  -0.1253,
  -0.1874,
  -0.2487,
  -0.3090,
  -0.3681,
  -0.4258,
  -0.4818,
  -0.5358,
  -0.5878,
  -0.6374,
  -0.6845,
  -0.7290,
  -0.7705,
  -0.8090,
  -0.8443,
  -0.8763,
  -0.9048,
  -0.9298,
  -0.9511,
  -0.9686,
  -0.9823,
  -0.9921,
  -0.9980,
  -1.0000,
  -0.9980,
  -0.9921,
  -0.9823,
  -0.9686,
  -0.9511,
  -0.9298,
  -0.9048,
  -0.8763,
  -0.8443,
  -0.8090,
  -0.7705,
  -0.7290,
  -0.6845,
  -0.6374,
  -0.5878,
  -0.5358,
  -0.4818,
  -0.4258,
  -0.3681,
  -0.3090,
  -0.2487,
  -0.1874,
  -0.1253,
  -0.0628,
};

// Port tables

#define N_AIN 9
#define NEEDLE_OFFSET 3
const int ainputs[N_AIN] = { ROLL, PITCH, YAW, ROLL_RATE, PITCH_RATE, YAW_RATE, ROLL_ERR, PITCH_ERR, YAW_ERR };

#define N_SYNCHRO 9
const int synchroOutputs[N_SYNCHRO] = { PWM_R0, PWM_R1, PWM_R2, PWM_P0, PWM_P1, PWM_P2, PWM_Y0, PWM_Y1,
                                        PWM_Y2 };
#define N_OTHER 6
const int otherOutputs[N_OTHER] = { PWM_RR, PWM_RE, PWM_PR, PWM_PE, PWM_YR, PWM_YE };

// Magnitudes for the 400 Hz PWM outputs
volatile float mag[N_SYNCHRO] = { 0 };

// Interrupt handler
void update() {
  for (int i = 0; i < N_SYNCHRO; i++) {
    analogWrite(synchroOutputs[i], int(sine[count] * mag[i] * 120 + 120));
  }
  count = (count + 1) % 100;
}

// Sync interrupt
void syncInterrupt() {
  if (count < 20 || count > 80) {  // Ignore sync if it is wildly off
    count = 0;                     // Reset the counter on rising edge of sync input
  }
}

/**
 * Sets the specified rotation axis to the specified angle.
 */
void setAxis(int axis, float ang) {
  float m0 = (sin(ang) + 1) / 2;
  float m1 = (sin(ang + 2 * PI / 3) + 1) / 2;
  float m2 = (sin(ang + 4 * PI / 3) + 1) / 2;
  noInterrupts();
  mag[3 * axis] = m0;  // Rotate the selected axis
  mag[3 * axis + 1] = m1;
  mag[3 * axis + 2] = m2;
  interrupts();
}

extern int data[];   // Hardcoded roll, pitch, yaw data for the flight
float scale = 10;    // Angles are scaled by a factor of 10

void setup() {
  Serial.begin(9600);
  pinMode(SYNC, INPUT);

  // Initialize analog inputs
  for (int i = 0; i < N_AIN; i++) {
    pinMode(ainputs[i], INPUT);
  }

  // Initialize PWM outputs
  for (int i = 0; i < N_SYNCHRO; i++) {
    pinMode(synchroOutputs[i], OUTPUT);
    analogWriteFrequency(synchroOutputs[i], 15000000 / 64);  // divide by 64 for 8-bit resolution
  }
  setAxis(0, 0.);
  setAxis(1, 0.);
  setAxis(2, 0.);

  // Initialize needles
  for (int i = 0; i < N_OTHER; i++) {
    pinMode(otherOutputs[i], OUTPUT);
    analogWriteFrequency(otherOutputs[i], 15000000 / 64);
    analogWrite(otherOutputs[i], 127);
  }
  pinMode(FLAG, OUTPUT);
  digitalWrite(FLAG, 1);

  myTimer.begin(update, 25);  // 25 microseconds * 100 samples give 400 Hz

  attachInterrupt(digitalPinToInterrupt(SYNC), syncInterrupt, RISING);

  // Initialize and wait to stabilize
  float rollDeg = data[0] / scale;
  float pitchDeg = data[1] / scale;
  float yawDeg = (data[2] - data[2]) / scale;
  roll(rad(rollDeg));
  pitch(rad(pitchDeg));
  yaw(rad(yawDeg));
  delay(3000);
}

extern int len;      // Length of data[]
int interval = 10;  // 10 ms between updates (100 ms between data entries)
int idx = 0;         // Index into data
int interpCount = 0;      // Interpolation count: 0-9
const int interpRange = 10; // 10 interpolated updates per data point

/*
 * Linear interpolation between v0 and v1. Value is v0 when interpCount == 0 and v1 when interpCount == interpRange
 * Handle wraparound of angle, so interpolating 0 and 360 won't yield 180, for instance. Input must be in degrees.
*/
float interp(float v0, float v1, float interpCount) {
  // Adjust v0 and v1 by multiples of 360 degrees until they are within half a circle (180 degrees) of each other.
  // Then the interpolation will take the short path.
  while (v0 - v1 > 180) {
    v1 += 360;
  }
  while (v1 - v0 > 180) {
    v1 -= 360;
  }
  // Invariants: |v0 - v1| <= 180. v0 and v1 are the same as the initial v0 and v1 module 360 degrees.
  // It doesn't matter if v0 and v1 are less than 0 or greater than 360 at this point, as long as they are close together.
  return v0 * (1 - interpCount / interpRange) + v1 * interpCount / interpRange;
}

float rad(float deg) {
  return deg / 360. * 2 * PI;
}

void roll(float r) {
  setAxis(0, r + rad(92 + 180));
}

void yaw(float r) {
  setAxis(1, -r + rad(25));
}

void pitch(float r) {
  setAxis(2, r + rad(20 + 180));
}

void loop() {
  float rollDeg = interp(data[idx], data[idx + 3], interpCount) / scale;
  float pitchDeg = interp(data[idx + 1], data[idx + 4], interpCount) / scale;
  float yawDeg = (interp(data[idx + 2], data[idx + 5], interpCount) - data[2]) / scale;
  roll(-rad(rollDeg));
  pitch(rad(pitchDeg));
  yaw(rad(yawDeg));
  interpCount += 1;
  if (interpCount == interpRange) {
    interpCount = 0;
    idx += 3;
  }
  
  delay(interval);
  
  if (idx >= len - 6) {
    // End of data
    idx = 0;
    Serial.println("Restarting");
  }
}
