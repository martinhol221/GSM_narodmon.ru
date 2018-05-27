#include <SoftwareSerial.h>         // управление обогревателем на даче по GPRS каналу из приложения
SoftwareSerial m590(4, 5);          // RX, TX  для новой платы
#include <DallasTemperature.h>      // https://github.com/milesburton/Arduino-Temperature-Control-Library
#define ONE_WIRE_BUS 8              // https://github.com/PaulStoffregen/OneWire
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
/*  ----------------------------------------- НАЗНАЧАЕМ ВЫВОДЫ --------------------------------------------------------------------   */
#define WEBASTO_Pin 10              // на реле включение оборгевателя
#define BAT_Pin A0                  // на батарею, через делитель напряжения 39кОм / 11 кОм

/*  ----------------------------------------- ИНДИВИДУАЛЬНЫЕ НАСТРОЙКИ !!!---------------------------------------------------------   */
String call_phone = "3750000000"; // телефон входящего вызова
String MAC = "59-01-78-00-00-00";   // МАС-Адрес устройства для индентификации на сервере narodmon.ru (придумать свое 59-01-XX-XX-XX-XX-XX)
String SENS = "Termostat";          // Название устройства (придумать свое Dacha или др. для отображения в программе и на карте)
String APN = "internet.mts.by";     // тчка доступа выхода в интернет вашего сотового оператора
String USER = "mts";                // имя выхода в интернет вашего сотового оператора
String PASS = "mts";                // пароль доступа выхода в интернет вашего сотового оператора
String SERVER = "185.245.187.136,8283";  // сервер, порт народмона (на май 2018) 
bool  n_send = true;                // отправка данных на народмон включена (true), отключена (false)
bool SMS_report = true;             // флаг СМС отчета
float m = 80.41;                    // делитель для перевода АЦП в вольты для резистров 39/11kOm
/*  ----------------------------------------- ДАЛЕЕ НЕ ТРОГАЕМ --------------------------------------------------------------------   */
int inDS;                           // индекс датчиков температуры
String at = "";
String buf ;
float TempDS[10];                  // переменная хранения температуры с датчика двигателя
float Vbat;                        // переменная хранящая напряжение питание устройства
int k = 0;
int interval = 5;                  // интервал отправки данных на народмон 50 сек после старта
bool heating_comand, heating_status = false;       
bool SMS_send = false;             // флаг разовой отправки СМС
unsigned long Time1 = 0;


void setup() {

  pinMode(WEBASTO_Pin, OUTPUT);
  pinMode(3, INPUT_PULLUP);        // указываем пин на вход для с внутричипной подтяжкой к +3.3V
  Serial.begin(9600);              // скорость порта для отладки 9600
  m590.begin(38400);               // скорость порта модема, может быть 38400
  delay(5000);
  m590.println("ATE1;+CMGF=1;+CSCS=\"gsm\";+CLIP=1");
  Serial.println("MAC: "+MAC+" Sensor name: "+SENS+"V2.0/25.03.2018");
  attachInterrupt(1, callback, FALLING);   // 3-й пин Ардуино
  

/*-смена скорости модема с 9600 на 38400:
установить m590.begin(9600);, раскоментировать m590.println("AT+IPR=38400"), delay (1000);  и m590.begin(38400), прошить
вернуть  вернуть все обратно и прошить. снова.
*/
        }

void callback(){                                                  // обратный звонок при появлении напряжения на входе 3
    m590.println("ATD"+call_phone+";"),    delay(5000);} 
        

void loop() {
  if (m590.available()) {                                  // если что-то пришло от модема 
    while (m590.available()) k = m590.read(), at += char(k),delay(1);
    
    if (at.indexOf("CLIP: \""+call_phone+"\",") > -1 ) {   // ищем номер телефона
        delay(50), m590.println("ATH0"),delay(1000), SMS_send = true;  // если нашли, сбрасываем вызов и отправляем СМС
    /*  --------------------------------------------------- ПРЕДНАСТРОЙКА МОДЕМА M590 ----------------------------------------------------------------------   */
    } else if (at.indexOf("+PBREADY\r\n") > -1)                                 {m590.println("ATE1;+CMGF=1;+CSCS=\"gsm\";+CLIP=1"); // дважды  ATE1 для модемов версии ниже 1.3
    /*  ----------------------------------------------- ЕСЛИ НЕТ СОЕДИНЕНИЯ с ИНТЕРНЕТ ТО УСТАНОАВЛИВАЕМ ЕГО -----------------------------------------------   */
    } else if (at.indexOf("+XIIC:    0,") > -1 || at.indexOf("+TCPSETUP:Error") > -1) { m590.println("AT+TCPCLOSE=0"),delay(200),m590.println("AT+XISP=0"),delay(200),interval=5;
    } else if (at.indexOf("AT+XISP=0\r\r\nOK\r\n") > -1 )                       {delay(30), m590.println ("AT+CGDCONT=1,\"IP\",\""+APN+"\""),        delay(50); 
    } else if (at.indexOf("AT+CGDCONT=1,\"IP\",\""+APN+"\"\r\r\nOK\r\n") > -1 ) {delay(30), m590.println ("AT+XGAUTH=1,1,\""+USER+"\",\""+PASS+"\""),delay(100); 
    } else if (at.indexOf("AT+XGAUTH=1,1,\""+USER+"\",\""+PASS+"\"") > -1 )     {delay(30), m590.println ("AT+XIIC=1"),                              delay(100);
   /*  ------------------------------ ЕСЛИ ПОДКЛЮЧЕНИЕ ЕСТЬ, ТО КОННЕКТИКСЯ К СЕРВЕРУ И ШВЫРЯЕМ ПАКЕТ ДАНЫХ НА СЕРВЕР---------------------------------------   */    
    } else if (at.indexOf("+XIIC:    1,") > -1 )                                { delay(50), m590.println ("AT+TCPSETUP=0," +SERVER+ "");
    } else if (at.indexOf("+TCPSETUP:0,OK") > -1 )                              { buf = ""; // в переменную и набиваем пакет данных:
              buf ="#" +MAC+ "#" +SENS+ "\n";                                               // MAC адресс устройства
         for (int i=0; i < inDS; i++) buf = buf+ "#Temp" +i+ "#" +TempDS[i]+ "\n";          // выводим массив температур
              buf=buf+ "#Vbat#" +Vbat+ "\n";                                                // напряжение питания
              buf=buf+ "#Termostat#" +heating_comand+ "\n";                                 // команда управления
              buf=buf+ "#Relay#" +heating_status+ "\n";                                     // статус реле тена           
              buf=buf+ "#Uptime#" +millis()/1000+ "\n";                                     // время работы ардуино в секундах
              buf=buf+ "##";                                                                // закрываем пакет ##
               m590.print("AT+TCPSEND=0,"),    m590.print(buf.length()),  m590.println(""), delay (200);               
    } else if (at.indexOf("AT+TCPSEND=0,") > -1 && at.indexOf("\r\r\n>") > -1) {m590.print(buf), Serial.println(buf), delay (500), m590.println("AT+TCPCLOSE=0");
    } else if (at.indexOf("+TCPRECV:0,7,#estop") > -1 )                        {heating_comand = false;
    } else if (at.indexOf("+TCPRECV:0,8,#estart") > -1 )                       {heating_comand = true ;
   }
     Serial.println(at), at = ""; }          // печатаем ответ и очищаем переменную

if (millis()> Time1 + 10000) detection(), Time1 = millis();           // выполняем функцию detection () каждые 10 сек 

  
   /*  для отладки
     if (Serial.available()) {             //если в мониторе порта ввели что-то
    while (Serial.available()) k = Serial.read(), at += char(k), delay(1);
    m590.println(at), at = ""; }  //очищаем
  */
}   
void detection(){                          // услови проверяемые каждые 10 сек  

    Vbat = analogRead(BAT_Pin);
    Vbat = Vbat / m ;
    Serial.print("Напряжение: "),        Serial.print(Vbat), Serial.println(" Вольт"); 
    Serial.print("Счетчик: "),           Serial.println(interval);
    Serial.print("Термостат: "),         Serial.println(heating_comand ? "ВКЛ.":"ОТКЛ.");
    Serial.print("Реле нагревателя: "),  Serial.println(heating_status ? "ВКЛ.":"ОТКЛ.");
    inDS = 0;                              // индекс датчиков
    sensors.requestTemperatures();         // читаем температуру с трех датчиков
    while (inDS < 10){
          TempDS[inDS] = sensors.getTempCByIndex(inDS);            // читаем температуру
      if (TempDS[inDS] == -127.00){ 
                                  TempDS[inDS]= 80;  // подменяем -127 на 80 в случае не подключенной 1-Wire шины
                                  break; }                               // пока не доберемся до неподключенного датчика
              inDS++;} 
    for (int i=0; i < inDS; i++) Serial.print("Temp"), Serial.print(i), Serial.print("= "), Serial.println(TempDS[i]); 
    Serial.println ("");
        
                         if (SMS_send == true && SMS_report == true) { SMS_send = false;  // если фаг SMS_send равен 1 высылаем отчет по СМС
                         //   m590.println("AT+CMGF=1;+CSCS=\"gsm\""), delay(100);                                         
                            m590.println("AT+CMGS=\"+"+call_phone+"\""), delay(100);
                            m590.print("Privet "+SENS+"!");
                            for (int i=0; i < inDS; i++)  m590.print("\n Temp"), m590.print(i), m590.print(": "), m590.print(TempDS[i]);
                            m590.print("\n Termostat: "), m590.print(heating_comand ? "ON.":"OFF.");  
                            m590.print("\n Reley: "),     m590.print(heating_comand ? "ON.":"OFF.");  
                            m590.print("\n Voltage: "),   m590.print(Vbat);      
                            m590.print("\n Uptime: "),    m590.print(millis()/3600000),         m590.print("H.");
                            m590.print((char)26);  }
                                 
    if (heating_comand == true  && TempDS[0] < 17) heating_status = true;  // если активирован термостат и температура ниже 17 градусов включаем реле
    if (heating_comand == false || TempDS[0] > 24) heating_status = false; // если  термостат не активирован или температура выше 24 градусов то выключаем реле
    digitalWrite(WEBASTO_Pin, heating_status);
    
    if (n_send == true)                           interval--;
    if (interval <1 ) interval = 30, m590.println ("AT+XIIC?");       // запрашиваем статус соединения           
                            
}             
 
