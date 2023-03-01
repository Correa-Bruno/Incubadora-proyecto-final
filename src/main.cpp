#include <Arduino.h>
#include<DHT.h>
#include<DHT_U.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PID_v1.h>





#define DHTTYPE DHT22  //Define el tipo de sensor (en este caso es el modelo DHT22)
#define SENSOR 7 // DHT22 conectado al pin 7 del Arduino
#define PIN_OUTPUT 3 // Pin por el cual se realiza el feedback de la temperatura (pwm) 


DHT dht(SENSOR, DHTTYPE);    // Crea el objeto dht
LiquidCrystal_I2C lcd(0x27,16,2);  // Crea el objeto de la LCD




float temp, hum; //Define variables para almacenar temperatura y humedad
double sp=28.9; 
 
// Definir variables a las que nos conectaremos
double Setpoint, Input, Output;

//Prametros de regulacion de pid
double Kp=35, Ki=0.2, Kd=0;

PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT); //crea el objeto del PID

 


void setup() {
 Serial.begin(9600);  
 dht.begin();   // Inicia la librería dht
 
 //Inicialización LCD y definición texto de arranque
 lcd.backlight();
 lcd.init();
 lcd.print("Incubadora");
 delay(1000);

/////////////pid/////////////////////////////////////////////////////////////////////
  Input = 0;
  Setpoint = sp/80*1023; //se combierte el valor del set point a entero
  //PID on
  myPID.SetMode(AUTOMATIC);

}




void loop() {

   hum = dht.readHumidity(); // Guarda la humedad monitorizada por el sensor en la variable hum
   temp = dht.readTemperature(); // Guarda la temperatura monitorizada por el sensor en la variable temp
   
   Serial.print("Temperatura: ");	// escritura en monitor serial de los valores
   Serial.print(temp);
   Serial.print(" Humedad: ");
   Serial.println(hum);
   delay(500);

    lcd.setCursor(0,0);
    lcd.print("T:");
    lcd.print(temp,1);
    lcd.print("C ");
    lcd.setCursor(8,0);
    lcd.print("H:");
    lcd.print(hum,1);//1 decimal
    lcd.print("%");
  //////////////////////////pid////////////////////////////////////////////////////////
  Input = (temp/80*1023); //se combierte el valor de la temperatura a entero
  myPID.Compute();
  analogWrite(PIN_OUTPUT, Output);


}

