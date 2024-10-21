#include <AccelStepper.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>

// Definición de pines para relés y LED
const int relayPin1 = 34;  // Pin para el relé 1
const int relayPin2 = 35;  // Pin para el relé 2
const int ledPin = 2;       // Pin para el LED

// Número de pasos por revolución para el motor (28BYJ-48)
const int stepsPerRevolution = 2048; 

// Configuración del LCD (dirección I2C 0x27, 16 columnas y 2 filas)
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Pines conectados al driver ULN2003
const int motorPin1 = 4; 
const int motorPin2 = 5; 
const int motorPin3 = 15; 
const int motorPin4 = 16; 

// Creación del objeto AccelStepper (modo FULL4WIRE)
AccelStepper stepper(AccelStepper::FULL4WIRE, motorPin1, motorPin3, motorPin2, motorPin4);

// Definición de pines para los pulsadores (pull-down)
const int pinHorario = 13;      // Pulsador para el giro horario
const int pinAntihorario = 12;  // Pulsador para el giro antihorario
const int pinBoton3 = 14;        // Pulsador adicional para relés

// Variables para almacenar el estado de los botones
bool estadoHorario = false;
bool estadoAntihorario = false;
bool estadoBoton3 = false;

// Variable para rastrear el estado de los relés
bool relayState = false;

// Variables para control de tiempo
unsigned long tiempoInicio = 0;      // Tiempo en que comenzó el giro
const unsigned long duracionGiro = 4000;   // Duración del giro en milisegundos (4 segundos)
bool girando = false;                // Indicador de si el motor está girando

// Contador de fallas
int contadorFallas = 0;

void setup() {
  Serial.begin(115200);
  
  stepper.setMaxSpeed(500);      
  stepper.setSpeed(500);         
  
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(ledPin, OUTPUT);
  
  digitalWrite(relayPin1, LOW);
  digitalWrite(relayPin2, LOW);
  digitalWrite(ledPin, LOW);
  
  pinMode(pinHorario, INPUT_PULLDOWN);      
  pinMode(pinAntihorario, INPUT_PULLDOWN);  
  pinMode(pinBoton3, INPUT_PULLDOWN);       

  Wire.begin();
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0); 
  lcd.print(" Bina's Control");
  lcd.setCursor(0, 1); 
  lcd.print("------ARGQ------");
}

void loop() {
  estadoHorario = digitalRead(pinHorario);
  estadoAntihorario = digitalRead(pinAntihorario);
  estadoBoton3 = digitalRead(pinBoton3);

  if (estadoHorario == HIGH && !girando) {  
    Serial.println("Giro horario iniciado.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Subiendo plataforma");
    stepper.setSpeed(500);         
    girando = true;                
    tiempoInicio = millis();       
  }

  if (estadoAntihorario == HIGH && !girando) {  
    Serial.println("Giro antihorario iniciado.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Bajando plataforma.");
    stepper.setSpeed(-500);        
    girando = true;                
    tiempoInicio = millis();       
  }

  if (estadoBoton3 == HIGH) {  
    relayState = !relayState; 

    digitalWrite(relayPin1, relayState ? HIGH : LOW);
    digitalWrite(relayPin2, relayState ? HIGH : LOW);
    digitalWrite(ledPin, relayState ? HIGH : LOW);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(relayState ? "Relays On" : "Relays Off");

    Serial.println(relayState ? "Relés encendidos." : "Relés apagados.");
    
    lcd.print("Estado activo");
    delay(200); 
  }

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();  

    Serial.println("Comando recibido: " + command);  

    if (command == "RELAY_ON") {
      digitalWrite(relayPin1, HIGH);
      digitalWrite(relayPin2, HIGH);
      digitalWrite(ledPin, HIGH);  
      Serial.println("Relés encendidos.");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Relays On");
    }
    else if (command == "RELAY_OFF") {
      digitalWrite(relayPin1, LOW);
      digitalWrite(relayPin2, LOW);
      digitalWrite(ledPin, LOW);  
      Serial.println("Relés apagados.");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Relays Off");
    }
    else {
      moverMotor(command);
    }
  }

  if (girando) {
    stepper.runSpeed(); 

    if (millis() - tiempoInicio >= duracionGiro) {
      stepper.setSpeed(0); 
      girando = false;      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Giro completado");
      mostrarFallas();  // Mostrar el contador de fallas después del giro
      Serial.println("Giro completado.");
    }
  }
}

void moverMotor(const String& resultado) {
  if (resultado == "APROBADO") {
    Serial.println("Giro horario solicitado vía comando serial.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Subiendo");
    stepper.setSpeed(500);         
    girando = true;                
    tiempoInicio = millis();       
  } 
  else if (resultado == "FALLA") {
    Serial.println("Giro antihorario solicitado vía comando serial.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Bajando");
    
    contadorFallas++; // Incrementa el contador de fallas
    Serial.print("Contador de fallas: ");
    Serial.println(contadorFallas);
    
    stepper.setSpeed(500); 
    girando = true; 
    unsigned long tiempoBajar = millis();
    
    while (millis() - tiempoBajar < 5000) {
      stepper.runSpeed(); 
    }
    
    stepper.setSpeed(0);
    delay(500); 
    
    Serial.println("Regresando a posición inicial.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Subiendo");
    stepper.setSpeed(-500); 
    
    tiempoBajar = millis();
    while (millis() - tiempoBajar < 5000) {
      stepper.runSpeed(); 
    }
    
    stepper.setSpeed(0);
    girando = false; 
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Giro completado");
    mostrarFallas();  // Mostrar el contador de fallas después del giro
    Serial.println("Proceso completado.");
  }
}

void mostrarFallas() {
  lcd.setCursor(0, 1);
  lcd.print("Fallas: ");
  lcd.print(contadorFallas);
}


