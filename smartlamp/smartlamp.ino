// Pinos usados
int ledPin = 18;     // GPIO do LED
int ldrPin = 34;     // GPIO analógico do LDR

int ldrMax = 3026;   // Valor máximo estimado do LDR (ajuste conforme seu teste)
int led_intensity = 10;  // Valor inicial do LED

void setup() {
  Serial.begin(9600);

  // Inicializa PWM no canal 0
  ledcSetup(0, 5000, 8);         // Canal 0, 5kHz, resolução 8 bits (0–255)
  ledcAttachPin(ledPin, 0);      // Liga o pino ao canal

  pinMode(ldrPin, INPUT);

  // Inicializa o LED com brilho 10%
  ledUpdate(led_intensity);

  processCommand("GET_LDR");

  Serial.println("SmartLamp Initialized.");
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }
}

// Processa os comandos recebidos via serial
void processCommand(String command) {
  if (command.startsWith("SET_LED ")) {
    int value = command.substring(8).toInt();

    if (value >= 0 && value <= 100) {
      ledUpdate(value);
      Serial.println("RES SET_LED 1"); // tratamento das resposta com base na condição
    } else {
      Serial.println("RES SET_LED -1");
    }

  } else if (command == "GET_LED") {
    Serial.print("RES GET_LED ");
    Serial.println(led_intensity);

  } else if (command == "GET_LDR") {
    int ldrValue = ldrGetValue();
    Serial.print("RES GET_LDR ");
    Serial.println(ldrValue);

  } else {
    Serial.println("ERR Unknown command.");
  }
}
// Atualiza intensidade do LED via PWM
void ledUpdate(int value) {
  led_intensity = value;
  int pwmValue = map(value, 0, 100, 0, 255); // Converte de 0–100 para 0–255
  ledcWrite(0, pwmValue);
}

int getLDRNormalized(int value) {
  return ((float)value/100)*255;
}

// Lê e normaliza o valor do LDR (0 a 100)
int ldrGetValue() {
  int ldrValueRead = analogRead(ldrPin);
  int percent = map(ldrValueRead, 0, 1023, 0, 100);
  return percent;
}
