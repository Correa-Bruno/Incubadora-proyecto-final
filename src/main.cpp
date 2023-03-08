#include <Arduino.h>
#include<DHT.h>
#include<DHT_U.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PID_v1.h>
#include <TimeLib.h>
#include <TimeAlarms.h>

#define DHTTYPE DHT22  //Define el tipo de sensor (en este caso es el modelo DHT22)
#define SENSOR 7 // DHT22 conectado al pin 7 del Arduino
#define PIN_OUTPUT 3 // Pin por el cual se realiza el feedback de la temperatura (pwm)
 
int fcA = 11;    // final de carrera A pin 11
int fcB = 12;    // final de carrera B pin12
int motorvolteo = 2; // motor encargado de realizar el volteo pin 2



bool posA = false; 
bool posB = false;
bool voltear= false; 
int pinpruebavolteo=A0;
bool varpruebavolteo;

int pinEnt = 4;        //enter pin 4
int pinA = 10; 			//variable A a pin digital 9 (CLK en modulo)
int pinB = 9;  	  //variable B a pin digital 10 (DT en modulo)
int Pos = 0;       //indica la posicion del encoder 
int Posant=Pos;
int aux;
unsigned long time;
unsigned long t;
unsigned long tEnt;
bool C = true;		
bool D = true;		
bool E = false;		
bool Ent = true;			
int EncMenu = Pos;	
 void encoder(); 
 void menu();
 void configuracion();
 void volteo();
 void onoff();
 int marcadorpuntero=0; 
 


 


DHT dht(SENSOR, DHTTYPE);    // Crea el objeto dht
LiquidCrystal_I2C lcd(0x27,16,2);  // Crea el objeto de la LCD



				                

float temp, hum; //Define variables para almacenar temperatura y humedad
double spT=37.5;  // Define el set point de temperatura 
double spH=50;  // Define el set point de HUMEDAD
double Setpoint, Input, Output; // Definir variables necesarias para l control de temperatura
double Kp=35, Ki=0.2, Kd=0; //Prametros de regulacion de pid
int volteohs; // intervalo entre volteo y volteo expresado en horas 

bool conf=false;    //Variables necesarias para el menu 
bool incub=false;
String incubacionmod = "OFF" ;
bool enter=false;
int contMenu=0;
const int botconf = 8;   //boton de configuracion pin8
bool varCONF;


double cap; //capacidad en porsentaje del calentador 
int diasTRANS=0;   // dias transcurridos de incubacion 
int mov=3;    // frecuencia con la cual se mueven los huevos espresado en hs




PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT); //crea el objeto del PID

 AlarmId id;


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
  Setpoint = spT/80*1023; //se combierte el valor del set point a entero
  //PID on
  myPID.SetMode(AUTOMATIC);
////////////////////////////////////////////////////////////
   pinMode(botconf, INPUT);  //boton de configuracion como entrada
   
   ///*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*
	pinMode (pinA, INPUT);     //DERECHA
   pinMode (pinB, INPUT);     //IZQUIERDA
   pinMode (pinEnt, INPUT);     //ENTER
							  
///*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*/*
   ///////////////volteo///////////////////
    pinMode(fcA,INPUT);
    pinMode(fcB,INPUT);
    pinMode(motorvolteo,OUTPUT);
    pinMode(pinpruebavolteo,INPUT);
    
    digitalWrite(motorvolteo,HIGH);
    
    
}

void loop() {
 
   Setpoint = spT/80*1024; 
   hum = dht.readHumidity(); // Guarda la humedad monitorizada por el sensor en la variable hum
   temp = dht.readTemperature(); // Guarda la temperatura monitorizada por el sensor en la variable temp
   
   //Serial.print("Temperatura: ");	// escritura en monitor serial de los valores
   //Serial.print(temp);
   //Serial.print(" Humedad: ");
   //Serial.println(hum);
   //delay(500);

    
  //////////////////////////pid////////////////////////////////////////////////////////
  Input = (temp/80*1023); //se combierte el valor de la temperatura a entero
  myPID.Compute();
  cap = (Output/255*100);   //se combierte el valor de la salida en un valor porsentual 
  
 
   //////////////////////////////////////////volteo///////////////////////////////////
    menu();
    encoder();
    volteo();
    onoff();

   

   
    
    
}

void menu(){

////////////////////////////////////////////////lectura boton configuracion/////////////////////////////////////////////////////////////////////////
  if(enter==false){      //para no poder salir de configuracion cuando se esta modificando un valor 
  if(time-t>350){
          varCONF=true;
      }

            if (digitalRead(botconf)==HIGH){
               t=time;
               if(varCONF==true){
                  Serial.println("Configuracion");
                  conf=!conf;
                  varCONF=false;
                  lcd.clear();
                  
               }
              
            }
  }

///////////////////////////////////////////////////////////////////lectura boton enter (encoder)//////////////////////////////////////////////////////
 if(conf==true){               //para que el enter ande solamente en configuracion  
 if(time-t>350){
          E=false;
      }

            if (digitalRead(pinEnt)==LOW){
               t=time;
               if(E==false){
                  Serial.println("ENTER");
                  enter=!enter;
                  E=true;
               }
              
            }
 }
 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////           
   if (conf==false) {
      lcd.setCursor(0,0);
      lcd.print("T:");
      lcd.print(temp,1);
      lcd.print("C ");
      lcd.setCursor(8,0);
      lcd.print("H:");
      lcd.print(hum,1);//1 decimal
      lcd.print("%");
      lcd.setCursor(0,1);
      lcd.print("C:");
      lcd.print(cap,1);
      lcd.print("%");
      if (cap<100){
      lcd.setCursor(7,1);
      lcd.print(" "); }
      if (cap<10){
      lcd.setCursor(6,1);
      lcd.print("  ");}
      lcd.setCursor(9,1);
      lcd.print("D:");
      lcd.print(diasTRANS);
      if (diasTRANS<10){
      lcd.setCursor(12,1);
      lcd.print(" ");}
      lcd.setCursor(13,1);
      lcd.print("M:");
      lcd.print(mov);
   }
   else if(conf==true){
      
      configuracion();
   }
   
}

 void encoder(){
      time = millis(); //registra el tiempo 

      if(digitalRead(pinA)==LOW){
         t = time;
         if(C==true){
            Pos++;
             //if(Pos>100){Pos--;}
            Serial.print("Encoder Posicion: ") ;
            Serial.println(Pos);
            C=false;
            D=false;    //Bloquea a B porque A llego primero

         }

      }
      if(digitalRead(pinB)==LOW){
         t = time;
         if(D==true){
            Pos--;
             //if(Pos<0){Pos++;}
            Serial.print("Encoder Posicion: ") ;
            Serial.println(Pos);      
            D=false;
            C=false;    // Bloquea a A porque B llego primero 

         }

      }
      if(time-t>8){         //Tiempo necesario para eliminar rebotes 
         C=true;           
         D=true; 
        
       } 
 }
    
   void configuracion(){
      lcd.setCursor(0,0);
      lcd.print("T:");
      lcd.print(spT,1);
      lcd.print("C");
      lcd.setCursor(8,0);
      lcd.print("H:");
      lcd.print(spH,1);
      lcd.print("%");
      lcd.setCursor(0,1);
      lcd.print("I:");
      lcd.print(incubacionmod);        
      lcd.setCursor(6,1);
      lcd.print("D:");
      lcd.print(diasTRANS);
      lcd.setCursor(11,1);
      lcd.print("M:");
      lcd.print(mov);
      lcd.setCursor(15,1);
      lcd.print("*");
       
  if(enter==false){
    if (Posant<Pos) {
      lcd.clear();
      marcadorpuntero++;}
    else if (Posant>Pos){
       lcd.clear();
      marcadorpuntero--;}
    if(marcadorpuntero<=0){marcadorpuntero=0;}
    if(marcadorpuntero>=4){marcadorpuntero=4;}
  }
     switch (marcadorpuntero)
{
  case 0:  ////set point temperatura 
      lcd.setCursor(7,0);
      lcd.print("<");
      if(enter==true){      // para que no cambie el valor hasta que se precone enter
      if (Posant<Pos) {     //incrementa el valor 
      spT=spT+0.1;}
    else if (Posant>Pos){   //decrementa el valor 
      spT=spT-0.1;}
      }
  break;

  case 1:  ////set point humedad
     
      lcd.setCursor(15,0);
      lcd.print("<");
      if(enter==true){      // para que no cambie el valor hasta que se precone enter
      if (Posant<Pos) {     //incrementa el valor 
      spH=spH+0.5;}
    else if (Posant>Pos){   //decrementa el valor 
      spH=spH-0.5;}
      }
  break;

  case 2:             //encender o apagar incubacion 
      lcd.setCursor(5,1);
      lcd.print("<");
   if(enter==true){ 
      if (Posant!=Pos) {     //prende o apaga el proceso de incubacion 
      incub=!incub;
      }
      if (incub==false){
         incubacionmod = "OFF";
      }
      if (incub==true){
         incubacionmod ="ON ";
      }
      }
        
  break;
  case 3:                    // dias transcurridos de incubacion 
      lcd.setCursor(10,1);
      lcd.print("<");
       if(enter==true){      // para que no cambie el valor hasta que se precione enter
      if (Posant<Pos) {     //incrementa el valor 
      diasTRANS=diasTRANS+1;
      if (diasTRANS>=40){diasTRANS=40;}  //limite max
      }
    else if (Posant>Pos){   //decrementa el valor 
      diasTRANS=diasTRANS-1;
      if (diasTRANS<=0){diasTRANS=0;} //limite min
      }
      if (diasTRANS<=9){
         lcd.setCursor(9,1);
         lcd.print(" ");     //Borra el 0 que queda en esa pocicion si pasamos de 10 a 9, quedaba 90
      }
      }
  break;
  case 4:           //intervalo en hs de movimiento de huevos
      lcd.setCursor(14,1);
      lcd.print("<");
      if(enter==true){      // para que no cambie el valor hasta que se precone enter
      if (Posant<Pos) {     //incrementa el valor 
      mov=mov+1;}
    else if (Posant>Pos){   //decrementa el valor 
      mov=mov-1;}
  break;
}
}
    Posant=Pos;
    
 }

void volteo(){
      

      if(time-t>350){
          varpruebavolteo=true;
      }

            if (digitalRead(pinpruebavolteo)==HIGH){
               t=time;
               if(varpruebavolteo==true){
                  voltear=true;
                   Serial.println("prueba de volteo");
               }
               }
                   
      if (voltear==true){
         digitalWrite(motorvolteo,LOW);   //enciende el motor de volteo
         Serial.println("prende volteo");
         
      }
      
      if((digitalRead(fcA)==HIGH)&&(posA==false)){
         digitalWrite(motorvolteo,HIGH);   //apaga el motor de volteo
         voltear=false;
         posA=true;
         posB=false;
         Serial.println("apaga volteo");
      }
      if ((digitalRead(fcB)==HIGH)&&(posB==false)){
         digitalWrite(motorvolteo,HIGH);   //apaga el motor de volteo
         voltear=false;
         posB=true;
         posA=false;
         Serial.println("apaga volteo");
         
      }
    }

        
    void onoff(){

      if (incub==false){               //quiere decir que la incubadora esta apagada 
        digitalWrite(PIN_OUTPUT,LOW);
      }
       if (incub==true){               //quiere decir que la incubadora esta prendida 
        analogWrite(PIN_OUTPUT, Output);
        
      }
    }

         
         
   
    
 