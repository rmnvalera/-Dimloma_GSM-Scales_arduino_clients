#include <SoftwareSerial.h>                 // Библиотека програмной реализации обмена по UART-протоколу
SoftwareSerial SIM800(3, 2);                // RX, TX

String _response = "";                      // Переменная для хранения ответа модуля
void setup() {
  Serial.begin(9600);                       // Скорость обмена данными с компьютером
  SIM800.begin(9600);                       // Скорость обмена данными с модемом
  Serial.println("Start!");

  sendATCommand("AT", true);
  sendATCommand("AT+CREG?", true);
//  sendATCommand("AT+CFUN=0", true);
//  sendATCommand("AT+CFUN=1", true);
  
//  sendATCommand("AT", true);
//  sendATCommand("AT+CMEE=2", true);
//  sendATCommand("AT+CPIN?", true);
//  sendATCommand("AT+CREG?", true);
//  
//  sendATCommand("AT+CIPSHUT", true);
//  sendATCommand("AT+CGATT=1", true);
//  sendATCommand("AT+CGATT?", true);


  sendATCommand("AT+CSQ", true);


//  AT+CSQ
//
//+CSQ: 6,0

//sendATCommand("AT+SAPBR=0,1", true);

  sendATCommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", true);
  sendATCommand("AT+SAPBR=3,1,\"APN\",\"www.kyivstar.net\"", true);
  sendATCommand("AT+SAPBR=3,1,\"APN\",\"www.kyivstar.net\"", true);
//sendATCommand("AT+HTTPSSL=1", true);

  
  sendATCommand("AT+SAPBR=1,1", true, 30000);
  sendATCommand("AT+SAPBR=2,1", true);

}

String sendATCommand(String cmd, bool waiting) {
  return sendATCommand(cmd, waiting, 10000);
 }


String sendATCommand(String cmd, bool waiting, int timeout) {
  String _resp = "";                            // Переменная для хранения результата
  Serial.println(cmd);                          // Дублируем команду в монитор порта
  SIM800.println(cmd);                          // Отправляем команду модулю
  if (waiting) {                                // Если необходимо дождаться ответа...
    _resp = waitResponse(timeout);                     // ... ждем, когда будет передан ответ
    // Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
    if (_resp.startsWith(cmd)) {  // Убираем из ответа дублирующуюся команду
      _resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
    }
    Serial.println(_resp);                      // Дублируем ответ в монитор порта
  }
  return _resp;                                 // Возвращаем результат. Пусто, если проблема
}

String waitResponse(int timeout2) {                         // Функция ожидания ответа и возврата полученного результата
  String _resp = "";                            // Переменная для хранения результата
  long _timeout = millis() + timeout2;             // Переменная для отслеживания таймаута (10 секунд)
  while (!SIM800.available() && millis() < _timeout)  {}; // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
  if (SIM800.available()) {                     // Если есть, что считывать...
    _resp = SIM800.readString();                // ... считываем и запоминаем
  }
  else {                                        // Если пришел таймаут, то...
    Serial.println("Timeout...");               // ... оповещаем об этом и...
  }
  return _resp;                                 // ... возвращаем результат. Пусто, если проблема
}


long lastcmd = millis();
void loop() {
  if (SIM800.available())   {                   // Если модем, что-то отправил...
    _response = waitResponse(10000);                 // Получаем ответ от модема для анализа
    Serial.println(_response);                  // Если нужно выводим в монитор порта
    // ... здесь можно анализировать данные полученные от GSM-модуля
  }
  if (Serial.available())  {                    // Ожидаем команды по Serial...
    SIM800.write(Serial.read());                // ...и отправляем полученную команду модему
  };


    if (millis() - lastcmd > 10000) {
    lastcmd = millis(); 
//    sendATCommand("AT", true);
    sendATCommand("AT+HTTPINIT", true);
    
//    sendATCommand("AT+HTTPSSL=1", true);
    sendATCommand("AT+HTTPPARA=\"CID\",1", true);
    sendATCommand("AT+HTTPPARA=\"URL\",\"http://codius.ru\"", true);
    sendATCommand("AT+HTTPACTION=0", true);
    sendATCommand("AT+HTTPREAD", true);
    sendATCommand("AT+HTTPTERM", true);
    }
 
  
}
