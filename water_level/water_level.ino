void setup() {
  // put your setup code here, to run once:
  pinMode(7, OUTPUT);
  pinMode(8, INPUT_PULLUP);
  
  digitalWrite(7, LOW);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  bool low = digitalRead(8);

  Serial.println(low ? "empty" : "full");
  delay(300);
}
