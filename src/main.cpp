#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PID_v1.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <RTClib.h>
#include <SoftwareSerial.h>

#define DHTTYPE DHT22  //Define el tipo de sensor (en este caso es el modelo DHT22)
#define SENSOR 7 // DHT22 conectado al pin 7 del Arduino
#define PIN_OUTPUT 3 // Pin por el cual se realiza el feedback de la temperatura (pwm)
 
int fcA = 11;    // final de carrera A pin 11
int fcB = 12;    // final de carrera B pin12
int motorvolteo = 2; // motor encargado de realizar el volteo pin 2

int led13 = 13; 


bool posA = false; 
bool posB = false;
bool voltear= false; 
int pinpruebavolteo=A0;
bool varpruebavolteo;

byte monitor;

int pinEnt = 4;        //enter pin 4
int pinA = 10; 			//variable A a pin digital 9 (CLK en modulo)
int pinB = 9;  	  //variable B a pin digital 10 (DT en modulo)
int Pos = 0;       //indica la posicion del encoder 
int Posant=Pos;
int aux;
unsigned long time;
unsigned long t;      // varable utilizada en los delay cortos 
unsigned long tt;     // variable utulizada en el delay para transmitir temperatura 
unsigned long tEnt;
bool C = true;		
bool D = true;		
bool E = false;		
bool Ent = true;			
int EncMenu = Pos;	
 void encoder(); 
 void menu();
 void configuracion();
 void calibracion();
 void volteo();
 void onoff();
 void fechahora();
 void muestrahorafecha();
 void alarmas(); 
 int marcadorpuntero=0; 
 


 


DHT dht(SENSOR, DHTTYPE);    // Crea el objeto dht
LiquidCrystal_I2C lcd(0x27,16,2);  // Crea el objeto de la LCD
RTC_DS3231 rtc;			// crea objeto del tipo RTC_DS3231
SoftwareSerial D1(5, 6); 	// pin 5 como RX, pin 6 como TX




				                

float temp, hum; //Define variables para almacenar temperatura y humedad
double spT=37.5;  // Define el set point de temperatura 
double spH=50;  // Define el set point de HUMEDAD
double tempcali=0;
double humecali=0;
double Setpoint, Input, Output; // Definir variables necesarias para l control de temperatura
double Kp=35, Ki=0.2, Kd=0; //Prametros de regulacion de pid
int volteohs; // intervalo entre volteo y volteo expresado en horas 

bool conf=false;    //Variables necesarias para el menu 
bool cali=false;
bool incub=true;
String incubacionmod = "ON" ;
bool enter=false;
int contMenu=0;
const int botconf = 8;   //boton de configuracion pin8
const int botcali = A1;
bool varCONF;
bool varCALI;


double cap; //capacidad en porsentaje del calentador 
int diasTRANS=0;   // dias transcurridos de incubacion 
int horasTRANS=0;
int minTRANS=0;
int dias;
int horas;
int min;

int mov=1;    // frecuencia con la cual se mueven los huevos espresado en hs
int proxmov=mov;

bool alarma=false; 
int difalarma =15;

const char startlimite = '<';         
const char endlimite   = '>';


        
  




PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT); //crea el objeto del PID

 AlarmId id;


void setup() {
 Serial.begin(9600); 
 Wire.begin();       // unirse al bus i2c como master
 rtc.begin(); 
 dht.begin();   // Inicia la librería dht
 
 //Inicialización LCD y definición texto de arranque
 lcd.backlight();
 lcd.init();
 lcd.print("Incubadora");
 delay(1000);
 lcd.clear();

/////////////pid/////////////////////////////////////////////////////////////////////
  Input = 0;
  Setpoint = spT/80*1023; //se combierte el valor del set point a entero
  //PID on
  myPID.SetMode(AUTOMATIC);
////////////////////////////////////////////////////////////
   pinMode(botconf, INPUT);  //boton de configuracion como entrada
   pinMode(botcali, INPUT);  //boton de calibracion como entrada
   
   
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
    
    ///////////////////////////////moduo DS3231/////////////////////
     // rtc.adjust(DateTime(__DATE__, __TIME__));	// funcion que permite establecer fecha y horario
    DateTime fecha = rtc.now();
    dias = fecha.day();
    horas = fecha.hour();
    min = fecha.minute();

    pinMode (led13, OUTPUT); 
    digitalWrite(led13,LOW); 

}
    


void loop() {
 
   Setpoint = spT/80*1024; 
   hum = dht.readHumidity()+humecali; // Guarda la humedad monitorizada por el sensor en la variable hum y ajusta el valor con calibracion
   temp = dht.readTemperature()+tempcali; // Guarda la temperatura monitorizada por el sensor en la variable temp y ajusta el valor con calibracion
 
   //Serial.print("Temperatura: ");	// escritura en monitor serial de los valores
   //Serial.print(temp);
   //Serial.print(" Humedad: ");
   //Serial.println(hum);
   //delay(500);

    
  //////////////////////////pid////////////////////////////////////////////////////////
  Input = (temp/80*1023); //se combierte el valor de la temperatura a entero
  myPID.Compute();
  cap = (Output/255*100);   //se combierte el valor de la salida en un valor porsentual 
  /////////////////////////////////////////////////////////////////////////////////////////
 
 /////////////////////////////////////D1/////////////////////////////////////////////
  /*if (D1.available()){     	// consulta si hay informacion disponible desde el modulo
   Serial.write(D1.read()); 	// lee desde el modulo y envia a monitor serie
  }
   if (Serial.available()){   	// consulta si hay informacion disponible desde el monitor serie
   D1.write(Serial.read()); 	// lee monitor serie y envia al modulo
   }*/
    menu();
    encoder();
    volteo();
    onoff();
    
   

  if(time-tt>1000){
  
   tt=time;
      Serial.print (startlimite);    
      Serial.print (temp,1);    //temperatura
      Serial.print ('h');
      Serial.print (hum,1); 
      Serial.print ('c');
      Serial.print (cap); 
      Serial.print (endlimite);  
      Serial.println ();
     
  }
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
 if((conf==true)||(cali==true)){               //para que el enter ande solamente en configuracion  
 if(time-t>350){
          E=false;
      }

            if (digitalRead(pinEnt)==LOW){
               t=time;
               if(E==false){
                  //Serial.println("ENTER");
                  enter=!enter;
                  E=true;
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
      time = millis(); //registra el tiempo 

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
      mov=mov+1;
      proxmov=horasTRANS+mov;     }
    else if (Posant>Pos){   //decrementa el valor 
      mov=mov-1;
      proxmov=horasTRANS+mov;     }
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
           if((varpruebavolteo==true)&&(voltear==false)){
            voltear=true;
             //Serial.println("prueba de volteo");
             //muestrahorafecha();
             //Serial.print("horas trancurridos:");
             //Serial.println(horasTRANS);
             //Serial.print("proximo volteo:");
             //Serial.println(proxmov);
            }
       }
       if((horasTRANS==proxmov)&&(diasTRANS<=18)&&(voltear==false)){
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

      if (incub==false){               //quiere decir que la incubadora esta apagada 
        digitalWrite(PIN_OUTPUT,LOW);
      }
       if (incub==true){               //quiere decir que la incubadora esta prendida 
        analogWrite(PIN_OUTPUT, Output);
        fechahora();
        if((conf==false)&&(cali==false))alarmas(); // la alarma se activa si esta el menu 
      }
    }

    void fechahora(){
       DateTime fecha = rtc.now();			// funcion que devuelve fecha y horario en formato
		 
       if (fecha.day()!=dias){
         diasTRANS++;
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
     switch (marcadorpuntero)
{
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
  break;

  case 1:  ////calibracion lectura humedad
     
      lcd.setCursor(8,1);
      lcd.print("<");
      if(enter==true){      // para que no cambie el valor hasta que se precone enter
      if (Posant<Pos) {     //incrementa el valor 
      humecali=humecali+0.1;}
    else if (Posant>Pos){   //decrementa el valor 
      humecali=humecali-0.1;}
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
   else if((temp>=spT-difalarma)&&(temp<=spT+difalarma)){       //ESTADO NORMAL 
      alarma=false;}
      
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
 