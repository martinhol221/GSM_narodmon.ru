/* скетч для ардуино с отправкой данных на сервер narodmon.ru  кажные 5 минут с 3 датчиков и одного АЦП.
Наастройка ; измениете индивидуальный номер датчика для народмона на свой SI-M8-00-XX-XX-XX
проверьте правильность подключения ардуино и СИМ 800 SoftwareSerial SIM800(8, 7) (TX>RX, RX>TX)
сконфигурируйте APN AT+CSTT=\"internet.mts.by\" для своего сотового оператора
*/
#include <SoftwareSerial.h>
SoftwareSerial SIM800(8, 7); // RX, TX
#include <DallasTemperature.h> // подключаем библиотеку чтения датчиков температуры
#define ONE_WIRE_BUS 9 // и настраиваем  пин 9 как шину подключения датчиков DS18B20
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

#define BAT_Pin A3      // на батарею, через делитель напряжения к примеру 39кОм / 11 кОм
float tempds0 = 52;     // переменная хранения температуры с датчика двигателя
float tempds1 = 54;     // переменная хранения температуры с датчика на улице
float tempds2 = 53;

int k = 0;
int interval = 3;     
String at = "";
unsigned long Time1 = 0;
int modem =0;            // переменная состояние модема
float Vbat;              // переменная хранящая напряжение бортовой сети
float m = 66.91;         // делитель для перевода АЦП в вольты для резистров 39/11kOm

void setup() {
  Serial.begin(9600);  //скорость порта
  SIM800.begin(9600);  //скорость связи с модемом
  Serial.println("Starting SIM800+ narodmon 1.0 23/05/2017"), delay(2000); 
  SIM800.println("ATE0"), delay(100); // отключаем режим ЭХА 
             }

void loop() {
  if (SIM800.available()) { // если что-то пришло от SIM800 в направлении Ардуино
    while (SIM800.available()) k = SIM800.read(), at += char(k),delay(1); //пришедшую команду набиваем в переменную at
    
      if (at.indexOf("SHUT OK") > -1 && modem == 2) {Serial.println("S H U T OK"), SIM800.println("AT+CMEE=1"), modem = 3;      

    } else if (at.indexOf("OK") > -1 && modem == 3) {Serial.println("AT+CMEE=1 OK"), SIM800.println("AT+CGATT=1"), modem = 4;   

    } else if (at.indexOf("OK") > -1 && modem == 4) {Serial.println("AT+CGATT=1 OK"), SIM800.println("AT+CSTT=\"internet.mts.by\""), modem = 5;  

    } else if (at.indexOf("OK") > -1 && modem == 5) {Serial.println("internet.mts.by OK"), SIM800.println("AT+CIICR"), modem = 6; 
           
    } else if (at.indexOf("OK") > -1 && modem == 6) {Serial.println("AT+CIICR OK"), SIM800.println("AT+CIFSR"), modem = 7;  
             
    } else if (at.indexOf(".") > -1 && modem == 7) {Serial.println(at), SIM800.println("AT+CIPSTART=\"TCP\",\"narodmon.ru\",\"8283\""), modem = 8;                                
          
    } else if (at.indexOf("CONNECT OK") > -1 && modem == 8 ) {SIM800.println("AT+CIPSEND"), Serial.println("C O N N E C T OK"), modem = 9;
    
    } else if (at.indexOf(">") > -1 && modem == 9 ) {  // "набиваем" пакет данными и шлем на сервер 
         Serial.print("#SI-M8-00-XX-XX-XX#SIM800+Sensor"); // индивидуальный номер и имя уствойства для народмона, у кждого свой !
         Serial.print("\n#Temp1#"), Serial.print(tempds0);       
         Serial.print("\n#Temp2#"), Serial.print(tempds1);       
         Serial.print("\n#Temp3#"), Serial.print(tempds2);       
         Serial.print("\n#Vbat#"),  Serial.print(Vbat);         
         Serial.println("\n##");      // обязательный параметр окончания пакета данных
         delay(500);  // копию шлем в модем
         SIM800.print("#SI-M8-00-XX-XX-XX#SIM800+Sensor"); // индивидуальный номер для народмона, это правило
         if (tempds0 > -40 && tempds0 < 54) SIM800.print("\n#Temp1#"), SIM800.print(tempds0);       
         if (tempds1 > -40 && tempds1 < 54) SIM800.print("\n#Temp2#"), SIM800.print(tempds1);       
         if (tempds2 > -40 && tempds2 < 54) SIM800.print("\n#Temp3#"), SIM800.print(tempds2);       
         SIM800.print("\n#Vbat#"),  SIM800.print(Vbat);         
         SIM800.println("\n##");      // обязательный параметр окончания пакета данных
         SIM800.println((char)26), delay (100);
        // SIM800.println ("");
         modem = 0, delay (100), SIM800.println("AT+CIPSHUT"); // закрываем пакет

     } else Serial.println(at);    // если пришло что-то другое выводим в серийный порт
     at = "";  // очищаем переменную
}
/* //  раскоментировать для отладки
 if (Serial.available()) {             //если в мониторе порта ввели что-то
    while (Serial.available()) k = Serial.read(), at += char(k), delay(1);
    SIM800.println(at),at = "";  //очищаем
                         }
*/
if (millis()> Time1 + 10000) detection(), Time1 = millis(); // выполняем функцию detection () каждые 10 сек 
  
}

void detection(){ // условия проверяемые каждые 10 сек  
  sensors.requestTemperatures();   // читаем температуру с трех датчиков
  tempds0 = sensors.getTempCByIndex(0);
  tempds1 = sensors.getTempCByIndex(1);
  tempds2 = sensors.getTempCByIndex(2);   
  Vbat = analogRead(BAT_Pin);  // замеряем АЦП (напряжение на батарее)
  Vbat = Vbat / m ; // переводим попугаи в вольты
  Serial.print("Vbat= "),Serial.print(Vbat), Serial.print (" V.");  
  Serial.print(" || Temp1 : "), Serial.print(tempds0);  
  Serial.print(" || Temp2 : "), Serial.print(tempds1);  
  Serial.print(" || Temp3 : "), Serial.print(tempds2);  
  Serial.print(" || Interval : "), Serial.println(interval);  
  interval--;
  if (interval <1 ) interval = 30, modem = 1; // 30 * 10 = 300 сек = 5 минут
  // if (interval == 20) SIM800.println("AT+CIPSHUT");
  if (modem == 1) SIM800.println("AT+CIPSHUT"), modem = 2;
}             
