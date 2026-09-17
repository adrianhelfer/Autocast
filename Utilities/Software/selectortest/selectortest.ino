const int SENSOR_VP_PIN = 36; // SENSOR_VP
const int SENSOR_VN_PIN = 39; // SENSOR_VN

void setup() {
  Serial.begin(115200);
  delay(200);

  // No INPUT_PULLUP available on these pins in hardware.
  pinMode(SENSOR_VP_PIN, INPUT);
  pinMode(SENSOR_VN_PIN, INPUT);
}

void loop() {
  int vpState = digitalRead(SENSOR_VP_PIN);
  int vnState = digitalRead(SENSOR_VN_PIN);

  if (vpState & vnState)
    Serial.println("OFF");
  else if (!vpState & vnState)
    Serial.println("BLUETOOTH");
  else if (vpState & !vnState)
    Serial.println("USB");
  delay(300);
}
