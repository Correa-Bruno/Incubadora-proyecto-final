#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PID_v1.h>
#include <TimeLib.h>
#include <RTClib.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>
#include <EEPROM.h>

#define DHTTYPE DHT22  //Define el tipo de sensor (en este caso es el modelo DHT22)
#define SENSOR 7 // DHT22 conectado al pin 7 del Arduino
#define PIN_OUTPUT 3 // Pin por el cual se realiza el feedback de la temperatura (pwm)
 
int fcA = 11;    // final de carrera A pin 11
int fcB = 12;    // final de carrera B pin12
int motorvolteo = 2; // motor encargado de realizar el volteo pin 2

int ventilador = 6;  // ventilador encargado de la circulacion del aire 
int humidificador = 5; // humidificador
bool estadoHumidificador = false; // indica el estado del humidificador, empieza apagado 
static boolean humidificadorEncendido = false; // bandera para corregir encendido y apagado del humidificador 


int led13 = 13;  // LED 13 de la placa 


bool posA = false;     // posicion A de bandeja de huevos
bool posB = false;     // posicion B de bandeja de huevos
bool voltear= false;   // Variable que se pone en true cada vez que hace un volteo 
int pinpruebavolteo=A2; // pulsador de volteo 
bool varpruebavolteo;   // Anti rebote pulsador volteo



int pinEnt = 4;         //enter pin 4
int pinA = 10; 			//variable A a pin digital 9 (CLK en modulo)
int pinB = 9;  	      //variable B a pin digital 10 (DT en modulo)
int Pos = 0;            //indica la posicion del encoder 
int Posant=Pos;         //indica posicion anterior del encoder 

unsigned long time;   // registra el tiempo 
unsigned long t;      // varable utilizada en los delay cortos 
unsigned long tt;     // variable utulizada en el delay para transmitir temperatura 
unsigned long ttt;    // variable utilizada para hacer el delay en el control de humedad

bool C = true;		  //variables necesarias para el manej del menu 
bool D = true;		
bool E = false;		
bool Ent = false;			


int marcadorpuntero=0;  // puntero para scrolear 

float temp, hum; //variables para almacenar temperatura y humedad
float humAnt;    //variable necesaria para corregir el encendido y apagado del humidificador 

double spT=36.5;  // Define el set point de temperatura 
double spH=65;  // Define el set point de HUMEDAD

bool banderaH=true; // bandera para el encendido y apagado del humidificador

double tempcali=0;  // variable para la calibracion de la lectura de la temperatura 
double humecali=0;  // variable para la calibracion de la lectura de la humedad 

double Setpoint, Input, Output; // Definir variables necesarias para l control de temperatura
double Kp=0.7, Ki=0.0006, Kd=0.001;//Prametros de regulacion de pid

int volteohs; // intervalo entre volteo y volteo expresado en horas 

bool conf=false;    //Variables necesarias para el menu 
bool cali=false;
bool incub=false;
String incubacionmod;
bool enter=false;

const int botconf = A0;   // boton para ingresar a configuracion 
const int botcali = A1;   // boton para ingresar a calibracion 
bool varCONF; 
bool varCALI;


double cap; //capacidad en porsentaje del encendido calentador 
int diasTRANS=0;   // dias transcurridos de incubacion 
int horasTRANS=0;  // horas transcurridas 
int minTRANS=0;    // min transcurridos 
int dias; 
int horas;
int min;

int diasmov=18; //cuantos dias efectua al volteo 
int mov=2;    // frecuencia con la cual se mueven los huevos espresado en hs
int proxmov=mov;  // proximo movimiento de buevos 

bool alarma=false;  // alarma de temperatura alta o baja 
int difalarma = 3;  // diferencia de valor con respecto al set point para encender o apagar la alarma 

const char startlimite = '<';     // indica el comienzo de la cadena de datos para mandar a la esp 
const char endlimite   = '>';     // indica el final de la cadena de datos para mandar a la esp 


 void encoder(); 
 void menu();
 void configuracion();
 void calibracion();
 void volteo();
 void onoff();
 void fechahora();
 void muestrahorafecha();
 void alarmas(); 
 void controlhumedad();
 void onoff_Humumidificador ();
 
 
DHT dht(SENSOR, DHTTYPE);    // Crea el objeto dht
LiquidCrystal_I2C lcd(0x27,16,2);  // Crea el objeto de la LCD
RTC_DS3231 rtc;			// crea objeto del tipo RTC_DS3231
SoftwareSerial D1(5, 6); 	// pin 5 como RX, pin 6 como TX
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT); //crea el objeto del PID

 


void setup() {
   Serial.begin(9600);  // inicia la comunicacion serie en 9600 baudios 
   wdt_enable(WDTO_2S); // Habilitar el Watchdog Timer con un tiempo de espera de 2 segundos
   Wire.begin();       // inicia al bus i2c como master
   rtc.begin();   // crea el objeto rtc
   dht.begin();   // crea el objeto dht
 
 //Inicialización LCD y definición texto de arranque
 lcd.backlight();
 lcd.init();
 lcd.print("Incubadora");
 delay(1000);
 lcd.clear();

/////////////pid/////////////////////////////////////////////////////////////////////
  Input = 0;
  Setpoint = spT/80*1023; // combierte el valor del set point a entero
  myPID.SetMode(AUTOMATIC);
////////////////////////////////////////////////////////////
   pinMode(botconf, INPUT);  //boton de configuracion como entrada
   pinMode(botcali, INPUT);  //boton de calibracion como entrada
   
   
   ////////////////////////////////////////////////////
	pinMode (pinA, INPUT);     //DERECHA
   pinMode (pinB, INPUT);     //IZQUIERDA
   pinMode (pinEnt, INPUT);     //ENTER					  
   //////////////////////////////////////////////////////////
    pinMode (ventilador, OUTPUT);
    digitalWrite(ventilador,HIGH);
    pinMode (humidificador, OUTPUT);
    
   ///////////////volteo///////////////////
    pinMode(fcA,INPUT);
    pinMode(fcB,INPUT);
    pinMode(motorvolteo,OUTPUT);
    pinMode(pinpruebavolteo,INPUT);
    
    digitalWrite(motorvolteo,HIGH);
    
    ///////////////////////////////moduo DS3231/////////////////////
     // rtc.adjust(DateTime(__DATE__, __TIME__));	// funcion que permite establecer fecha y horario
    DateTime fecha = rtc.now();
    dias = fecha.day();
    horas = fecha.hour();
    min = fecha.minute();

    pinMode (led13, OUTPUT); 
    digitalWrite(led13,LOW); 
////////////////////////lee valores guardados en la EEPROM /////////////////////////////////////////////////////////////
          EEPROM.get(0, spT);
          EEPROM.get(sizeof(spT), spH);
          EEPROM.get(sizeof(spT)+sizeof(spH),incub);
          EEPROM.get(sizeof(spT)+sizeof(spH)+sizeof(incub),diasTRANS);
          EEPROM.get(sizeof(spT)+sizeof(spH)+sizeof(incub)+sizeof(diasTRANS),mov);
          EEPROM.get(sizeof(spT)+sizeof(spH)+sizeof(incub)+sizeof(diasTRANS)+sizeof(mov),diasmov);
          EEPROM.get(sizeof(spT)+sizeof(spH)+sizeof(incub)+sizeof(diasTRANS)+sizeof(mov)+sizeof(mov),tempcali);
          EEPROM.get(sizeof(spT)+sizeof(spH)+sizeof(incub)+sizeof(diasTRANS)+sizeof(mov)+sizeof(mov)+sizeof(tempcali),humecali);
     
///////////////////////////////////////////////////
     if (incub==false){
         incubacionmod = "OF"; // si la incubacion esta apagada muestra OF
      }
      if (incub==true){
         incubacionmod ="ON"; // si la incubacion esta encendida muestra ON 
      }


    wdt_reset(); // Reiniciar el temporizador del Watchdog Timer
    
}
    


void loop() {
 
   Setpoint = spT/80*1024; 
   hum = dht.readHumidity()+humecali; // Guarda la humedad monitorizada por el sensor en la variable hum y ajusta el valor con calibracion
   temp = dht.readTemperature()+tempcali; // Guarda la temperatura monitorizada por el sensor en la variable temp y ajusta el valor con calibracion

    
  //////////////////////////pid////////////////////////////////////////////////////////
  Input = (temp/80*1023); //se combierte el valor de la temperatura a entero
  myPID.Compute();
  cap = (Output/255*100);   //se combierte el valor de la salida en un valor porsentual 
  /////////////////////////////////////////////////////////////////////////////////////////
 
 
    menu();
    encoder();
    volteo();
    onoff();
    
    
   
   ////////////////////////////////envio de datos//////////////////
  if(time-tt>1000){
      tt=time;
      Serial.print (startlimite);   //inicio   
      Serial.print (temp,1);        //temperatura
      Serial.print ('h');           // separador 
      Serial.print (hum,1);         // humedad
      Serial.print ('c');           // separardor 
      Serial.print (cap);           // capacidad 
      Serial.print (endlimite);     // fin
      Serial.println ();
   }

 time = millis(); //registra el tiempo 
 wdt_reset(); // Reiniciar el temporizador del Watchdog Timer
       
}

void menu(){

////////////////////////////////////////////////lectura boton configuracion/////////////////////////////////////////////////////////////////////////
  if(enter==false){      //para no poder salir de configuracion cuando se esta modificando un valor 
  if(time-t>350){
          varCONF=true; /////////configuracion///////////
      }

            if (digitalRead(botconf)==HIGH){
               t=time;
               if(varCONF==true){
                  //Serial.println("Configuracion");
                  conf=!conf;
                  cali=false;      // si estamos en cali y activamos a conf me desactive cali
                  if(conf==false)alarma=false;    // resetea la alarma
                  varCONF=false;
                  lcd.clear();
                  
               }
              
      }
      if(time-t>350){ 
          varCALI=true;   /////////calibracion///////////
      }

            if (digitalRead(botcali)==HIGH){
               t=time;
               if(varCALI==true){
                  //Serial.println("Calibracion");
                  cali=!cali;
                  conf=false;      // si estamos en conf y activamos a cali me desactive conf
                  varCALI=false;
                  lcd.clear();
                  if(cali==false)alarma=false;    // resetea la alarma
                  
               }
              
            }
  }
  
///////////////////////////////////////////////////////////////////lectura boton enter (encoder)//////////////////////////////////////////////////////
 if((conf==true)||(cali==true)){               //para que el enter ande solamente en configuracion y calibracion   
 if(time-t>350){
          E=false;
      }

            if (digitalRead(pinEnt)==LOW){
               t=time;
               if(E==false){
                  //Serial.println("ENTER");
                  enter=!enter;
                  E=true;
                  Ent=true;
               }
              
            }
 }
 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////           
   if ((conf==false)&&(cali==false)&&(alarma==false)) {
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
   else if(cali==true){
      calibracion();
   }
   
}

 void encoder(){
     

      if(digitalRead(pinA)==LOW){
         t = time;
         if(C==true){
            Pos++;
             //if(Pos>100){Pos--;}
           // Serial.print("Encoder Posicion: ") ;
           // Serial.println(Pos);
            C=false;
            D=false;    //Bloquea a B porque A llego primero

         }

      }
      if(digitalRead(pinB)==LOW){
         t = time;
         if(D==true){
            Pos--;
             //if(Pos<0){Pos++;}
           // Serial.print("Encoder Posicion: ") ;
            //Serial.println(Pos);      
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
      lcd.setCursor(5,1);
      lcd.print("D:");
      lcd.print(diasTRANS);
      lcd.setCursor(10,1);
      lcd.print("M:");
      lcd.print(mov);
      lcd.setCursor(14,1);
      lcd.print(diasmov);
       
   if(enter==false){
       if (Posant<Pos) {
       lcd.clear();
        marcadorpuntero++;
       }
      else if (Posant>Pos){
        lcd.clear();
        marcadorpuntero--;
      }
      if(marcadorpuntero<=0){marcadorpuntero=0;}
      if(marcadorpuntero>=5){marcadorpuntero=5;}
   }
     
     
   switch (marcadorpuntero){
     
    case 0:  ////set point temperatura 
      lcd.setCursor(7,0);
      lcd.print("<");
          if(enter==true){      // para que no cambie el valor hasta que se precone enter
             if (Posant<Pos) {     //incrementa el valor 
             spT=spT+0.1;}
          else if (Posant>Pos){   //decrementa el valor 
             spT=spT-0.1;}
           }
           
           if((enter==false)&&(Ent==true)){            // entra al confirmar la modificacion en configuracion
            EEPROM.put(0, spT);                        // guarda en la eeporm
            Serial.print("se guardo spT eeprom");
            Ent=false;                                 // bandera para que entre una sola vez al if 
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
       if((enter==false)&&(Ent==true)){            // entra al confirmar la modificacion en configuracion
            EEPROM.put(sizeof(spT), spH);          // guarda en la eeporm
            Serial.print("se guardo spH eeprom");
            Ent=false;                              // bandera para que entre una sola vez al if 
            }
  break;

  case 2:             //encender o apagar incubacion 
      lcd.setCursor(4,1);
      lcd.print("<");
   if(enter==true){ 
      if (Posant!=Pos) {     //prende o apaga el proceso de incubacion 
      incub=!incub;
      }
      if (incub==false){
         incubacionmod = "OF";
      }
      if (incub==true){
         incubacionmod ="ON";
      }
      }
      if((enter==false)&&(Ent==true)){                  // entra al confirmar la modificacion en configuracion
            EEPROM.put(sizeof(spT)+sizeof(spH),incub);  // guarda en la eeprom
            Serial.print("se guardo on off eeprom");
            Ent=false;                                  // bandera para que entre una sola vez al if 
            }
        
  break;
  case 3:                    // dias transcurridos de incubacion 
          if (diasTRANS<10){
            lcd.setCursor(8,1);
            lcd.print("<");}
      else if (diasTRANS>=10){
      lcd.setCursor(9,1);
      lcd.print("<");}
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
       if((enter==false)&&(Ent==true)){                                   // entra al confirmar la modificacion en configuracion
            EEPROM.put(sizeof(spT)+sizeof(spH)+sizeof(incub),diasTRANS); // guarda en la eeprom
            Serial.print("se guardo dias transcurridos eeprom");
            Ent=false;                                                    // bandera para que entre una sola vez al if 
            }
  break;
  case 4:           //intervalo en hs de movimiento de huevos
      lcd.setCursor(9,1);
      lcd.print(">");
      if(enter==true){      // para que no cambie el valor hasta que se precone enter
      if (Posant<Pos) {     //incrementa el valor 
      mov=mov+1;
      if (mov>=9){
         mov=9;}
      proxmov=horasTRANS+mov;     }
    else if (Posant>Pos){   //decrementa el valor 
      mov=mov-1;
      if (mov<=0){
         mov=0;}
      proxmov=horasTRANS+mov;     }
      }

       if((enter==false)&&(Ent==true)){                                               // entra al confirmar la modificacion en configuracion
             EEPROM.put(sizeof(spT)+sizeof(spH)+sizeof(incub)+sizeof(diasTRANS),mov); // guarda en la eeprom
            Serial.print("se guardo intervalo en hs mov eeprom");
            Ent=false;                                          // bandera para que entre una sola vez al if 
            }
  break;

case 5:           //cantidad de dias que relizara el movimiento de huevos
 if (diasmov<10){
      lcd.setCursor(15,1);
      lcd.print(" ");}     // borra el 0 que deja el 10 al decrementar 
      
      lcd.setCursor(13,1);
      lcd.print(">");
      if(enter==true){      // para que no cambie el valor hasta que se precone enter
      if (Posant<Pos) {     //incrementa el valor 
      diasmov++;
      if (diasmov>=30){
         diasmov=30;}
       }
    else if (Posant>Pos){   //decrementa el valor 
      diasmov--;
      if (diasmov<=0){
         diasmov=0;} 
         }}
         if((enter==false)&&(Ent==true)){                                               // entra al confirmar la modificacion en configuracion
            EEPROM.put(sizeof(spT)+sizeof(spH)+sizeof(incub)+sizeof(diasTRANS)+sizeof(mov),diasmov); // guarda en la eeprom
            Serial.print("se guardo dias mov eeprom");
            Ent=false;                                          // bandera para que entre una sola vez al if 
            }
  break;

}
    Posant=Pos;
    
 }

void volteo(){
      

   if(time-t>350){
    varpruebavolteo=true;
   }

   if (digitalRead(pinpruebavolteo)==HIGH){
     t=time;
         if((varpruebavolteo==true)&&(voltear==false)){
          voltear=true;
         }
   }
       if((horasTRANS==proxmov)&&(diasTRANS<=diasmov)&&(voltear==false)){
         voltear=true;                  //Mueve los huevos 
         proxmov=proxmov+mov;           //PROXIMO MOVIMIENTO 
         //Serial.println("prende volteo");
         //muestrahorafecha();
        // voltear=false;    /// para probar 
       }
                   
      if (voltear==true){
         digitalWrite(motorvolteo,LOW);   //enciende el motor de volteo  
      }
      
      if((digitalRead(fcA)==HIGH)&&(posA==false)){
         digitalWrite(motorvolteo,HIGH);   //apaga el motor de volteo
         voltear=false;
         posA=true;
         posB=false;
         //Serial.println("apaga volteo");
      }
      if ((digitalRead(fcB)==HIGH)&&(posB==false)){
         digitalWrite(motorvolteo,HIGH);   //apaga el motor de volteo
         voltear=false;
         posB=true;
         posA=false;
         //Serial.println("apaga volteo");
         
      }
    }

        
    void onoff(){

      if (incub==false){                       //quiere decir que la incubadora esta apagada 
         myPID.SetMode(MANUAL);          
        digitalWrite(PIN_OUTPUT,LOW);
        Output=0;
        digitalWrite(ventilador,HIGH);   //apaga el ventilador 

        if(humidificadorEncendido==true){   // si el humidificador esta encendido, lo apaga 
         onoff_Humumidificador();
         humidificadorEncendido=false;
          }
      }

       if (incub==true){               //quiere decir que la incubadora esta prendida 
         myPID.SetMode(AUTOMATIC);    
        analogWrite(PIN_OUTPUT, Output);
        fechahora();
        digitalWrite(ventilador,LOW);   //prende el ventilador 
        controlhumedad();
        if((conf==false)&&(cali==false))alarmas(); // la alarma se activa si no esta en conf o en cali y la incubadora encendida

        


      }
    }

    void fechahora(){
       DateTime fecha = rtc.now();			// funcion que devuelve fecha y horario en formato
		 
       if (fecha.day()!=dias){
         diasTRANS++;
         EEPROM.put(sizeof(spT)+sizeof(spH)+sizeof(incub),diasTRANS); // guarda en la eeprom
         Serial.print("se guardo dias transcurridos eeprom");
         dias = fecha.day();
       }
       if (fecha.hour()!=horas){
         horasTRANS++;
         horas = fecha.hour();
       }
       if (fecha.minute()!=min){
         minTRANS++;
         min = fecha.minute();
       }
    }

     /*  ESTA FUNCION SE USO E UN PRIMER MOMENTO PARA REALIZAR LAS PRUEBAS DE INCUBACION 
        void muestrahorafecha(){
         DateTime fecha = rtc.now();	
         Serial.print(fecha.day());			// funcion que obtiene el dia de la fecha completa
         Serial.print("/");				// caracter barra como separador
         Serial.print(fecha.month());			// funcion que obtiene el mes de la fecha completa
         Serial.print("/");				// caracter barra como separador
         Serial.print(fecha.year());			// funcion que obtiene el año de la fecha completa
         Serial.print(" ");	
         if (fecha.hour()<10){Serial.print("0");	}		
         Serial.print(fecha.hour());			// funcion que obtiene la hora de la fecha completa
         Serial.print(":");	
         if (fecha.minute()<10){Serial.print("0");	}
         Serial.println(fecha.minute());			// funcion que obtiene los minutos de la fecha completa
      }
      */

      void calibracion(){

         lcd.setCursor(0,0);
         lcd.print("T:");
         lcd.print(tempcali,1);
         lcd.print("C");
         lcd.setCursor(0,1);
         lcd.print("H:");
         lcd.print(humecali,1);
         lcd.print("%");
   
         if(enter==false){
            if (Posant<Pos) {
               lcd.clear();
               marcadorpuntero++;}
               else if (Posant>Pos){
                  lcd.clear();
                  marcadorpuntero--;}
            if(marcadorpuntero<=0){marcadorpuntero=0;}
            if(marcadorpuntero>=1){marcadorpuntero=1;}
         }
     switch (marcadorpuntero){
  case 0:  ////calibracion lectura temperatura 
      lcd.setCursor(8,0);
      lcd.print("<");
      if(enter==true){      // para que no cambie el valor hasta que se precone enter
         if (Posant<Pos) {     //incrementa el valor 
            tempcali=tempcali+0.1;
            lcd.clear();}
         else if (Posant>Pos){   //decrementa el valor 
            tempcali=tempcali-0.1;
            lcd.clear();}
      }
       if((enter==false)&&(Ent==true)){                                               // entra al confirmar la modificacion en configuracion
            EEPROM.put(sizeof(spT)+sizeof(spH)+sizeof(incub)+sizeof(diasTRANS)+sizeof(mov)+sizeof(mov),tempcali); // guarda en la eeprom
            Serial.print("se guardo calibracion temperatura eeprom");
            Ent=false;                                          // bandera para que entre una sola vez al if 
         }
  break;

  case 1:  ////calibracion lectura humedad
     
       lcd.setCursor(8,1);
       lcd.print("<");
         if(enter==true){      // para que no cambie el valor hasta que se precone enter
            if (Posant<Pos) {     //incrementa el valor 
               humecali=humecali+0.1;
            }
            else if (Posant>Pos){   //decrementa el valor 
               humecali=humecali-0.1;
            }
         }
      if((enter==false)&&(Ent==true)){                                               // entra al confirmar la modificacion en configuracion
         EEPROM.put(sizeof(spT)+sizeof(spH)+sizeof(incub)+sizeof(diasTRANS)+sizeof(mov)+sizeof(mov)+sizeof(tempcali),humecali); // guarda en la eeprom
         Serial.print("se guardo calibracion humedad eeprom");
         Ent=false;                                          // bandera para que entre una sola vez al if 
      }
  break;
}
     Posant=Pos;
}       
   
 void alarmas(){
   if((temp>spT+difalarma)&&(alarma==false)){   ///ALTA TEMPERATURA ENTRA UNA VEZ 
      lcd.clear();
         // Serial.println("¡¡ALTA TEMPERATURA!!");
      alarma=true;
   }
   else if((temp<spT-difalarma)&&(alarma==false)){   /// BAJA TEMPERATURA ENTRA EUNA VEZ
       lcd.clear();
         // Serial.println("¡¡BAJA TEMPERATURA!!");
       alarma=true;
   }
   else if((temp>=spT-difalarma)&&(temp<=spT+difalarma)&&(alarma==true)){       //ESTADO NORMAL 
         lcd.clear();
         alarma=false;
      }
      
     if ((alarma==true)&&(temp>spT+difalarma)){      ///ALTA TEMPERATURA ENTRA HASTA QUE SE NORMALICE 
         lcd.setCursor(2,0);
         lcd.print("ALTA");
         lcd.setCursor(9,0);
         lcd.print("T:");
         lcd.print(temp,1);
         lcd.print("C");
         lcd.setCursor(2,1);
         lcd.print("TEMPERATURA");
     }
       
     if ((alarma==true)&&(temp<spT-difalarma)){     ///BAJA TEMPERATURA ENTRA HASTA QUE SE NORMALICE 
         lcd.setCursor(2,0);
         lcd.print("BAJA");
         lcd.setCursor(9,0);
         lcd.print("T:");
         lcd.print(temp,1);
         lcd.print("C");
         lcd.setCursor(2,1);
         lcd.print("TEMPERATURA");
      }

      
        
 }
       

 void controlhumedad(){
     

   if ((hum<spH-2.5)&&(humidificadorEncendido == false)){
      humidificadorEncendido = true;
      onoff_Humumidificador ();
      Serial.print("enciende humidificador");
      ttt=time;
      }

   if ((hum>spH-2.5)&&(humidificadorEncendido == true)){
      humidificadorEncendido = false;
      onoff_Humumidificador ();
      Serial.print("apaga  humidificador");
      ttt=time;
   }

   

   if(time-ttt>25000){
         
      if(humidificadorEncendido==true){       //si indica que el humidificador esta encendido pero la humedad sigue bajando, 
            if(humAnt>hum){                    // quiere decir que en realidad esta apagado 
               onoff_Humumidificador();
               Serial.print("corrigio humidificador prendiendolo");
            }
      }   

      if(humidificadorEncendido==false){       //si indica que el humidificador esta apagado pero la humedad sigue subiendo, 
         if(humAnt<hum){                    // quiere decir que en realidad esta prendido 
            onoff_Humumidificador();
            Serial.print("corrigio humidificador apagandolo");
         }
      }  
         ttt=time;
         humAnt=hum; 
   }

   if ((diasTRANS<diasmov)&&(banderaH==false)){
      spH=spH-15;
      banderaH=true;
   }

   if ((diasTRANS>=diasmov)&&(banderaH==true)){
     spH=spH+15; 
     banderaH=false;
     Serial.print("Incrementa Humedad");
   }

 }

   void onoff_Humumidificador () {
       
     digitalWrite(humidificador, LOW); // en alto la salida
     delay(50);
     digitalWrite(humidificador, HIGH); // en bajo la apaga
     estadoHumidificador=!estadoHumidificador;
     // genera un pulso para encender o apagar el humidificador 
  
}
 