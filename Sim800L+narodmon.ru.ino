#include <SoftwareSerial.h>
SoftwareSerial SIM800(7, 6); // RX, TX  // для новых плат
#include <DallasTemperature.h>          // подключаем библиотеку чтения датчиков температуры
#define ONE_WIRE_BUS 4                  // и настраиваем  пин 4 как шину подключения датчиков DS18B20
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

#define BAT_Pin A0                  // на батарею, через делитель напряжения 39кОм / 11 кОм

/*  ----------------------------------------- ИНДИВИДУАЛЬНЫЕ НАСТРОЙКИ !!!---------------------------------------------------------   */
String MAC = "80-01-AA-00-00-00";  // МАС-Адрес устройства для индентификации на сервере narodmon.ru (придумать свое 80-01-XX-XX-XX-XX-XX)
String SENS = "NameSens";          // Название устройства (придумать свое Citroen 566 или др. для отображения в программе и на карте)
String APN = "internet.mts.by";    // тчка доступа выхода в интернет вашего сотового оператора
String USER = "mts";               // имя выхода в интернет вашего сотового оператора
String PASS = "mts";               // пароль доступа выхода в интернет вашего сотового оператора
/*  ----------------------------------------- ДАЛЕЕ НЕ ТРОГАЕМ --------------------------------------------------------------------   */
String SERVER = "narodmon.ru";     // сервер народмона (на октябрь 2017) 
String PORT = "8283";              // порт народмона   (на октябрь 2017) 
float TempDS0, TempDS1, TempDS2 ;  // переменные хранения температуры
int k = 0;
int interval = 10;                  // интервал отправки данных на народмон 
String at = "";
unsigned long Time1 = 0; 
float Vbat;                        // переменная хранящая напряжение бортовой сети
float m = 66.91;                   // делитель для перевода АЦП в вольты для резистров 39/11kOm

void setup() {
  
  Serial.begin(115200);              //скорость порта
  SIM800.begin(9600);                //скорость связи с модемом
  Serial.println("Starting. | SIM800L+narodmon.ru. | MAC:"+MAC+" | NAME:"+SENS+" | APN:"+APN+" | 13/12/2018"); 
  delay (2000);
  SIM800.println("ATE1"), delay(50);        // отключаем режим ЭХА
             }


void loop() {
  if (SIM800.available()) {                                               // если что-то пришло от SIM800 в SoftwareSerial Ардуино
    while (SIM800.available()) k = SIM800.read(), at += char(k),delay(1); // набиваем в переменную at

           if (at.indexOf("AT+CGATT=1\r\r\nOK\r\n") > -1 )         {SIM800.println("AT+CSTT=\""+APN+"\""),     delay (500);   // установливаем точку доступа  
    } else if (at.indexOf("AT+CSTT=\""+APN+"\"\r\r\nOK\r\n") > -1 ){SIM800.println("AT+CIICR"),                delay (2000);  // устанавливаем соеденение   
    } else if (at.indexOf("AT+CIICR") > -1 )                       {SIM800.println("AT+CIFSR"),                delay (1000);  // возвращаем IP-адрес модуля    
    } else if (at.indexOf("AT+CIFSR") > -1 )                       {SIM800.println("AT+CIPSTART=\"TCP\",\""+SERVER+"\",\""+PORT+"\""), delay (1000);    
    } else if (at.indexOf("CONNECT OK\r\n") > -1 )                 {SIM800.println("AT+CIPSEND"),              delay (1200) ;  // меняем статус модема
    } else if (at.indexOf("AT+CIPSEND\r\r\n>") > -1 )              {  // "набиваем" пакет данными 
                         SIM800.print("#" +MAC+ "#" +SENS);           // заголовок пакета с MAC адресом и именем устройства      
                         if (TempDS0 > -40 && TempDS0 < 54)        SIM800.print("\n#Temp1#"), SIM800.print(TempDS0); // Температура с датчика №1      
                         if (TempDS1 > -40 && TempDS1 < 54)        SIM800.print("\n#Temp2#"), SIM800.print(TempDS1); // Температура с датчика №2      
                         if (TempDS2 > -40 && TempDS2 < 54)        SIM800.print("\n#Temp3#"), SIM800.print(TempDS2); // Температура с датчика №3      
                         SIM800.print("\n#Vbat#"),                 SIM800.print(Vbat);                               // Напряжение аккумулятора
                         SIM800.print("\n#Uptime#"),               SIM800.print(millis()/1000);                      // Время непрерывной работы
                         SIM800.println("\n##");                                                                     // закрываем пакет
                         SIM800.println((char)26), delay (100);                                                      // отправляем пакет
     } 
     Serial.println(at);                                                   // Возвращаем ответ можема в монитор порта
     at = "";  // очищаем переменную
                         }
if (millis()> Time1 + 10000) Time1 = millis(), detection();       // выполняем функцию detection () каждые 10 сек 
}

void detection(){                           // условия проверяемые каждые 10 сек  
    sensors.requestTemperatures();          // читаем температуру с трех датчиков
    TempDS0 = sensors.getTempCByIndex(0);   // датчик на двигатель
    Serial.println(TempDS0);
    TempDS1 = sensors.getTempCByIndex(1);   // датчик в салон
    Serial.println(TempDS1);
    TempDS2 = sensors.getTempCByIndex(2);   // датчик на улицу 
    Serial.println(TempDS2);
    Vbat = analogRead(BAT_Pin);             // замеряем напряжение на батарее
    Vbat = Vbat / m ;                       // переводим попугаи в вольты
    Serial.println(Vbat);
    interval--;
    if (interval <1) interval = 30, SIM800.println ("AT+CGATT=1"), delay (200);    // подключаемся к GPRS 
    if (interval == 28) SIM800.println ("AT+CIPSHUT");  
    Serial.println("------------------------");
                     }             
