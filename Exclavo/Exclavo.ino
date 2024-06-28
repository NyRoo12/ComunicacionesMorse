#include <Wire.h>
#define buttonPin 6
#define ledPin 10
#define buzzerPin 8
#define slaveAddress 8

// Constantes de tiempo para puntos, rayas y espacios
const int punto = 200;
const int raya = punto * 3;
const int variablePunto = 150;
const unsigned long tiempoEspacioLetras = 3 * punto;
const unsigned long tiempoEspacioPalabra = 7 * punto; // Aumentado para mayor claridad en la separación
String currentMorse = "";
byte value_pot;
bool transmitiendoMensaje = true;

// Variables para el estado del botón y tiempos
int buttonState = 0;
int lastButtonState = 0;
unsigned long pressStartTime = 0;
String message = "";
unsigned long pressDuration = 0;
unsigned long tiempoUltimaAccion = 0;
bool detected = false;
bool recibiendoMensaje = true;


String morse[] = {".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."};
char letras[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};


//Variables del maestro
String mensajeRecieved="";
bool pressRecieved = false;
bool pressRelease = false;
bool recievedLastDetected = false;
unsigned long tiempoUltimaAccionRecieved = 0;
unsigned long pressDurationRecieved = 0;
unsigned long lastReceiveTime = 0;
byte received;

bool unaVez = false;

enum MorseState {IDLE, SENDING, PAUSE};
MorseState morseState = IDLE;
unsigned long morseTimer = 0;
int morseIndex = 0;
int morseLetterIndex = 0;
int morseCodeIndex = 0;
String currentMorseCode = "";
String textoMorse = "";

byte sensorValue2;
unsigned long startMorseTime = 0;
unsigned long morseDuration = 0;
int morseIndexText = 0;
int morseIndexChar = 0;
bool enviandoMorse = false;

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  Serial.begin(9600);
  Wire.begin(8);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(dataRqst);
}

void loop() {
  buttonState = digitalRead(buttonPin);
  value_pot = analogRead(A0);

  if (buttonState == HIGH && lastButtonState == LOW) {
    pressStartTime = millis();
    digitalWrite(ledPin, HIGH);
    tone(buzzerPin, 1500);
    detected = false;
  }

  if (buttonState == LOW && lastButtonState == HIGH) {
    pressDuration = millis() - pressStartTime;
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);

    if (pressDuration > punto - variablePunto && pressDuration < punto + variablePunto) {
      message += ".";
    } else if (pressDuration > raya - variablePunto && pressDuration < raya + variablePunto) {
      message += "-";
    }
    detected = true;
    tiempoUltimaAccion = millis();
  }

  if (detected && (millis() - tiempoUltimaAccion > tiempoEspacioLetras) && message != "") {
    if (transmitiendoMensaje == true){
      Serial.print("[Transmitiendo Mensaje]- ");
      transmitiendoMensaje = false;
    }

    for (int i = 0; i < 26; i++) {
      if (morse[i] == message) {
        Serial.print(letras[i]);
        break;
      }
    }

    message = "";
    tiempoUltimaAccion = millis();
  }

  if (detected && (millis() - tiempoUltimaAccion > tiempoEspacioPalabra)) {
    Serial.println(" - [Mensaje Transmitido]");
    detected = false;
    transmitiendoMensaje = true;
  }

  lastButtonState = buttonState;

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
    Wire.write(sensorValue2);
  }
  digitalWrite(ledPin, LOW);
  noTone(buzzerPin);
  sensorValue2 = analogRead(A1);
  Wire.write(sensorValue2);
}

void tonoRaya() {
  digitalWrite(ledPin, HIGH);
  tone(buzzerPin, 1500);
  sensorValue2 = analogRead(A1);
  unsigned long tiempo = millis(); 
  while(millis() - tiempo <= raya){
    Wire.write(sensorValue2);
  }
  digitalWrite(ledPin, LOW);
  noTone(buzzerPin);
  sensorValue2 = analogRead(A1);
  Wire.write(sensorValue2);
}

void receiveEvent(int howMany) {
  while (Wire.available()) {
    received = Wire.read();
    if (received > 30 && pressRecieved == false){
      pressStartTime = millis();
      digitalWrite(ledPin, HIGH);
      tone(buzzerPin, 500);
      pressRecieved = true;
      tiempoUltimaAccionRecieved = millis();
    }
    else if (received <= 30 && pressRecieved == true){
      pressDurationRecieved = millis() - pressStartTime;
      digitalWrite(ledPin, LOW);
      noTone(buzzerPin);
      pressRecieved = false;

      if (pressDurationRecieved > punto - variablePunto && pressDurationRecieved < punto + variablePunto) {
        mensajeRecieved += ".";
      } else if (pressDurationRecieved > raya - variablePunto && pressDurationRecieved < raya + variablePunto) {
        mensajeRecieved += "-";
      }
      recievedLastDetected = true;
      tiempoUltimaAccionRecieved = millis();
    }

    if (recievedLastDetected && (millis() - tiempoUltimaAccionRecieved > tiempoEspacioLetras) && mensajeRecieved != "" && pressRecieved == false){

      if (recibiendoMensaje == true){
        Serial.print("[Recibiendo mensaje] - ");
        recibiendoMensaje = false;
      }
      
      for (int i = 0; i < 26; i++) {
        if (morse[i] == mensajeRecieved) {
          Serial.print(letras[i]);
          break;
        }
      }
      mensajeRecieved = "";
    }

    if (recievedLastDetected && (millis() - tiempoUltimaAccionRecieved > tiempoEspacioPalabra) && pressRecieved == false) {
      Serial.println(" - [Mensaje Terminado]");
      recibiendoMensaje = true;
      recievedLastDetected = false;
    }
  }
}

void dataRqst(){
  if (buttonState == HIGH ) {
    Wire.write(value_pot); // send potentiometer position
  }
  else{
    Wire.write(sensorValue2);
  }
}