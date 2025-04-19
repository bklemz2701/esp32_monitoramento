#define RELE_PIN 5

void setup() {
  pinMode(RELE_PIN, OUTPUT);
}

void loop() {
  digitalWrite(RELE_PIN, LOW); // Liga o relé
  delay(2000);
  digitalWrite(RELE_PIN, HIGH); // Desliga o relé
  delay(2000);
}
