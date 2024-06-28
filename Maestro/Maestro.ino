#include <Wire.h>
#define buttonPin 6
#define ledPin 10
#define buzzerPin 8
#define slaveAddress 0x08

// Constantes de tiempo para puntos, rayas y espacios
const int punto = 200;
const int raya = punto * 3;
const int variablePunto = 150;
const unsigned long tiempoEspacioLetras = 3 * punto;
const unsigned long tiempoEspacioPalabra = 7 * punto;

// Funcionamiento principal Maestro
int buttonState = 0;
bool pressLocalState = false;
bool messageTraduction = false;
bool transmittingMessage = true;
unsigned long pressLocalTime = 0;
unsigned long pressLocalDuration = 0;
unsigned long lastTimeActionLocal; 
String messageLocal; 

String morse[] = {".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."};
char letras[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

// Funcionamiento Respuesta Exclavo
unsigned long pressResponseTime;
unsigned long pressDurationResponse;
String messageRecieved;
bool recievedLastDetected = false;
unsigned long lastPressTimeRecieved;
bool recievedMessage = true;
byte slaveResponse;
byte sensorValue;
bool unaVez = false;
bool pressReceived = false;

//Enviar mensaje a exclavo por consola
byte sensorValue2;
unsigned long startMorseTime = 0;
unsigned long morseDuration = 0;
int morseIndexText = 0;
int morseIndexChar = 0;
bool enviandoMorse = false;

enum MorseState {IDLE, SENDING, PAUSE};
MorseState morseState = IDLE;
unsigned long morseTimer = 0;
int morseIndex = 0;
int morseLetterIndex = 0;
int morseCodeIndex = 0;
String currentMorseCode = "";
String textoMorse = "";


void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  Serial.begin(9600);
  Wire.begin();
}


void loop() {

  // Funcionamiento Local Para enviar 
  buttonState = digitalRead(buttonPin);
  sensorValue = analogRead(A0);

  Wire.beginTransmission(0x08);
  Wire.write(sensorValue);
  Wire.endTransmission();

  if (buttonState == HIGH && pressLocalState == false) {
    digitalWrite(ledPin, HIGH);
    tone(buzzerPin, 1500);
    pressLocalState = true;
    pressLocalTime = millis();
  }
  else if (buttonState == LOW && pressLocalState == true){
    pressLocalDuration = millis() - pressLocalTime;
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
    pressLocalState = false;


    if (pressLocalDuration > punto - variablePunto && pressLocalDuration < punto + variablePunto) {
      messageLocal += ".";
    } else if (pressLocalDuration > raya - variablePunto && pressLocalDuration < raya + variablePunto) {
      messageLocal += "-";
    }

    messageTraduction = true;
    lastTimeActionLocal = millis();
  }
  if (messageLocal != "" && messageTraduction == true && (millis () - lastTimeActionLocal > tiempoEspacioLetras) && pressLocalState == false){
    if (transmittingMessage == true){
      Serial.print("[Transmitiendo Mensaje]- ");
      transmittingMessage = false;
    }

    for (int i = 0; i < 26; i++) {
      if (morse[i] == messageLocal) {
        Serial.print(letras[i]);
        break;
      }
    }
    messageLocal = "";
  }

  if (messageTraduction == true && (millis () - lastTimeActionLocal > tiempoEspacioPalabra) ){
    Serial.println(" - [Mensaje Transmitido]");
    messageTraduction = false;
    transmittingMessage = true;
  }

  // Respuesta del esclavo
  Wire.requestFrom(8, 1);

  if(Wire.available()){
    slaveResponse = Wire.read();
    if(slaveResponse > 168 && pressReceived == false){
      pressResponseTime = millis();
      digitalWrite(ledPin, HIGH);
      tone(buzzerPin, 1000);
      pressReceived = true;

    } else if(slaveResponse <= 168 && pressReceived == true){
      pressDurationResponse = millis() - pressResponseTime;
      digitalWrite(ledPin, LOW);
      noTone(buzzerPin);

      if (pressDurationResponse > punto - variablePunto && pressDurationResponse < punto + variablePunto) {
        messageRecieved += ".";
      } else if (pressDurationResponse > raya - variablePunto && pressDurationResponse < raya + variablePunto) {
        messageRecieved += "-";
      }

      lastPressTimeRecieved = millis();
      pressReceived = false;
      recievedLastDetected = true;
    }

    if (messageRecieved != "" && pressReceived == false && recievedLastDetected == true && (millis() - lastPressTimeRecieved > tiempoEspacioLetras)){
      if (recievedMessage == true){
        Serial.print("[Recibiendo mensaje] - ");
        recievedMessage = false;
      }
      for (int i = 0; i < 26; i++) {
        if (morse[i] == messageRecieved) {
          Serial.print(letras[i]);
          break;
        }
      }
      messageRecieved = "";
    }

    if (recievedLastDetected && (millis() - lastPressTimeRecieved > tiempoEspacioPalabra) && pressReceived == false) {
      Serial.println(" - [Mensaje Terminado]");
      recievedMessage = true;
      recievedLastDetected = false;
    }
  }

  if (Serial.available() > 0) {
    textoMorse = Serial.readStringUntil('\n');
    textoMorse.toUpperCase(); // Convertir el texto a mayúsculas
    Serial.print("[Enviando mensaje] - " + textoMorse);
    morseIndex = 0;
    morseLetterIndex = 0;
    morseCodeIndex = 0;
    morseState = SENDING;
    morseTimer = millis();
    enviandoMorse = true;
  }

  if (enviandoMorse) {
    reproducirMorse();
  }
  if (unaVez == true){
    Serial.println(" - [Mensaje enviado]");
    unaVez = false;
  }
}

void reproducirMorse() {
  switch (morseState) {
    case IDLE:
      break;

    case SENDING:
      if (morseIndex < textoMorse.length()) {
        char letra = textoMorse.charAt(morseIndex);
        if (letra == ' ') {
          if (millis() - morseTimer >= tiempoEspacioPalabra) {
            morseIndex++;
            morseTimer = millis();
          }
        } else {
          if (morseLetterIndex == 0) {
            for (int j = 0; j < 26; j++) {
              if (letras[j] == letra) {
                currentMorseCode = morse[j];
                break;
              }
            }
          }
          
          if (morseCodeIndex < currentMorseCode.length()) {
            char morseChar = currentMorseCode.charAt(morseCodeIndex);
            if (morseChar == '.' && millis() - morseTimer >= punto) {
              tonoPunto();
              morseCodeIndex++;
              morseTimer = millis();
            } else if (morseChar == '-' && millis() - morseTimer >= raya) {
              tonoRaya();
              morseCodeIndex++;
              morseTimer = millis();
            }
          } else if (millis() - morseTimer >= tiempoEspacioLetras) {
            Serial.print("");
            morseIndex++;
            morseLetterIndex = 0;
            morseCodeIndex = 0;
            morseTimer = millis();
            
          }
        }
      } else {
        unaVez = true;
        morseState = IDLE;
      }
      break;
      
    case PAUSE:
      if (millis() - morseTimer >= punto) {
        morseState = SENDING;
        morseTimer = millis();
      }
      break;
  }
}

void tonoPunto() {
  digitalWrite(ledPin, HIGH);
  tone(buzzerPin, 1000);
  sensorValue2 = analogRead(A1);
  unsigned long tiempo = millis(); 
  while(millis() - tiempo <= punto){
    Wire.beginTransmission(slaveAddress);
    Wire.write(sensorValue2);
    Wire.endTransmission();
  }
  digitalWrite(ledPin, LOW);
  noTone(buzzerPin);
  sensorValue2 = analogRead(A1);
  Wire.beginTransmission(slaveAddress);
  Wire.write(sensorValue2);
  Wire.endTransmission();
}

void tonoRaya() {
  digitalWrite(ledPin, HIGH);
  tone(buzzerPin, 1500);
  sensorValue2 = analogRead(A1);
  unsigned long tiempo = millis(); 
  while(millis() - tiempo <= raya){
    Wire.beginTransmission(slaveAddress);
    Wire.write(sensorValue2);
    Wire.endTransmission();
  }
  digitalWrite(ledPin, LOW);
  noTone(buzzerPin);
  sensorValue2 = analogRead(A1);
  Wire.beginTransmission(slaveAddress);
  Wire.write(sensorValue2);
  Wire.endTransmission();
}
