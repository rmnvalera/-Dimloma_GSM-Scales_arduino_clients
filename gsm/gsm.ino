#include <SoftwareSerial.h>  // Библиотека програмной реализации обмена по UART-протоколу

#define SIM800_TX_PIN 3
#define SIM800_RX_PIN 2

#define DEFAULT_TIMEOUT 5000
#define _internalBufferSize 200


// Internal memory for the shared buffer
// Used for all reception of message from the module
char *internalBuffer;
uint16_t internalBufferSize = 0;


enum NetworkRegistration {NOT_REGISTERED, REGISTERED_HOME, SEARCHING, DENIED, NET_UNKNOWN, REGISTERED_ROAMING, NET_ERROR};

boolean enableDebug = true;

const char APN[]          = "www.kyivstar.net";
const char URL[]          = "https://postman-echo.com/post";
const char CONTENT_TYPE[] = "application/json";
const char PAYLOAD[]      = "{\"name\": \"morpheus\", \"job\": \"leader\"}";

String _response = ""; // Переменная для хранения ответа модуля
long   lastcmd   = millis();
long   lastCSQ   = millis();


const char AT_CMD_BASE[] PROGMEM = "AT";                                      // Basic AT command to check the link
const char AT_CMD_CSQ[] PROGMEM = "AT+CSQ";                                   // Check the signal strengh
const char AT_CMD_ATI[] PROGMEM = "ATI";                                      // Output version of the module
const char AT_CMD_GMR[] PROGMEM = "AT+GMR";                                   // Output version of the firmware

const char AT_CMD_CFUN_TEST[] PROGMEM = "AT+CFUN?";                           // Check the current power mode
const char AT_CMD_CFUN0[] PROGMEM = "AT+CFUN=0";                              // Switch minimum power mode
const char AT_CMD_CFUN1[] PROGMEM = "AT+CFUN=1";                              // Switch normal power mode
const char AT_CMD_CFUN4[] PROGMEM = "AT+CFUN=4";                              // Switch sleep power mode

const char AT_CMD_CREG_TEST[] PROGMEM = "AT+CREG?";                           // Check the network registration status
const char AT_CMD_SAPBR_GPRS[] PROGMEM = "AT+SAPBR=3,1,\"Contype\",\"GPRS\""; // Configure the GPRS bearer
const char AT_CMD_SAPBR_APN[] PROGMEM = "AT+SAPBR=3,1,\"APN\",";              // Configure the APN for the GPRS
const char AT_CMD_SAPBR1[] PROGMEM = "AT+SAPBR=1,1";                          // Connect GPRS
const char AT_CMD_SAPBR0[] PROGMEM = "AT+SAPBR=0,1";                          // Disconnect GPRS

const char AT_CMD_HTTPINIT[] PROGMEM = "AT+HTTPINIT";                         // Init HTTP connection
const char AT_CMD_HTTPPARA_CID[] PROGMEM = "AT+HTTPPARA=\"CID\",1";           // Connect HTTP through GPRS bearer
const char AT_CMD_HTTPPARA_URL[] PROGMEM = "AT+HTTPPARA=\"URL\",";            // Define the URL to connect in HTTP
const char AT_CMD_HTTPPARA_CONTENT[] PROGMEM = "AT+HTTPPARA=\"CONTENT\",";    // Define the content type for the HTTP POST
const char AT_CMD_HTTPSSL_Y[] PROGMEM = "AT+HTTPSSL=1";                       // Enable SSL for HTTP connection
const char AT_CMD_HTTPSSL_N[] PROGMEM = "AT+HTTPSSL=0";                       // Disable SSL for HTTP connection
const char AT_CMD_HTTPACTION0[] PROGMEM = "AT+HTTPACTION=0";                  // Launch HTTP GET action
const char AT_CMD_HTTPACTION1[] PROGMEM = "AT+HTTPACTION=1";                  // Launch HTTP POST action
const char AT_CMD_HTTPREAD[] PROGMEM = "AT+HTTPREAD";                         // Start reading HTTP return data
const char AT_CMD_HTTPTERM[] PROGMEM = "AT+HTTPTERM";                         // Terminate HTTP connection

const char AT_RSP_OK[] PROGMEM = "OK";                                        // Expected answer OK
const char AT_RSP_DOWNLOAD[] PROGMEM = "DOWNLOAD";                            // Expected answer DOWNLOAD
const char AT_RSP_HTTPREAD[] PROGMEM = "+HTTPREAD: ";                         // Expected answer HTTPREAD

SoftwareSerial SIM800(SIM800_TX_PIN, SIM800_RX_PIN); // RX, TX

void setup()
{
  Serial.begin(115200); // Скорость обмена данными с компьютером
  while(!Serial);
  SIM800.begin(9600); // Скорость обмена данными с модемом
  delay(1000);
  
  Serial.println("Start!");

  setupModule();
}

String waitResponse(int timeout2)
{                                      // Функция ожидания ответа и возврата полученного результата
  String _resp = "";                   // Переменная для хранения результата
  long _timeout = millis() + timeout2; // Переменная для отслеживания таймаута (10 секунд)
  while (!SIM800.available() && millis() < _timeout)
  {
    // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
  };
  if (SIM800.available())
  {                              // Если есть, что считывать...
    _resp = SIM800.readString(); // ... считываем и запоминаем
  }
  else
  {                               // Если пришел таймаут, то...
    _resp = "\nTimeout...";
  }
  return _resp; // ... возвращаем результат. Пусто, если проблема
}


//bool ishttpCallready = false;

void loop()
{

  readResponseCheckAnswer_P(1000, AT_RSP_OK, 1);
  
//  if (SIM800.available())
//  {                                  // Если модем, что-то отправил...
//    _response = waitResponse(10000); // Получаем ответ от модема для анализа
//    Serial.println(_response);       // Если нужно выводим в монитор порта
//    // ... здесь можно анализировать данные полученные от GSM-модуля
//  }
  if (Serial.available())
  {                              // Ожидаем команды по Serial...
    SIM800.write(Serial.read()); // ...и отправляем полученную команду модему
  }

//  if (millis() - lastCSQ > 20000)
//  {
//    lastCSQ = millis();
//    sendATCommand("AT+CREG?", true);
//    _response = sendATCommand("AT+CSQ", true);
//    if (_response.indexOf("ERROR") > 0 || _response.indexOf("+CSQ: 0,0") > 0 || _response.indexOf("Timeout...") > 0 || _response.indexOf("?") > 0 )
//    {
//      Serial.println("--------------CSQ--EROR");
//      ishttpCallready = false;
//    }
//    else
//    {
//      Serial.println("--------------CSQ--OK");
//      ishttpCallready = true;
//    }
//
//    if (ishttpCallready)
//    {
//      Serial.println("APN init ...");
//      sendATCommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", true);
//      sendATCommand("AT+SAPBR=3,1,\"APN\",\"www.kyivstar.net\"", true);
//      sendATCommand("AT+SAPBR=3,1,\"APN\",\"www.kyivstar.net\"", true);
//      sendATCommand("AT+SAPBR=1,1", true);
//      _response = sendATCommand("AT+SAPBR=2,1", true);
//      if (_response.indexOf("ERROR") > 0 || _response.indexOf("+SAPBR: 1,3,\"0.0.0.0\"") > 0 || _response.indexOf("Timeout...") > 0 || _response.indexOf("?") > 0 )
//      {
//        Serial.println("--------------SAPBR--EROR");
//        ishttpCallready = false;
//      }
//      else
//      {
//        Serial.println("--------------SAPBR--OK");
//        ishttpCallready = true;
//      }
//      lastCSQ = millis();
//    }
//  }
//
//  if (ishttpCallready)
//  {
//    lastcmd = millis();
//    Serial.println("HTTP init ...");
//    sendATCommand("AT+HTTPINIT", true);
//    sendATCommand("AT+HTTPPARA=\"CID\",1", true);
//    sendATCommand("AT+HTTPPARA=\"URL\",\"http://35.188.154.232/v1/ping\"", true);
//    sendATCommand("AT+HTTPACTION=0", true, 20000);
//    sendATCommand("AT+HTTPREAD", true, 20000);
//    sendATCommand("AT+HTTPTERM", false);
//    Serial.println("HTTP close ...");
//    ishttpCallready = false;
//
//    lastCSQ = millis();
//  }
}




///////////////////////////////////////////////////////////////////////////////////
void setupModule() {
  // Prepare internal buffers
  if(enableDebug) {
    Serial.print(F("SIM800L : Prepare internal buffer of "));
    Serial.print(_internalBufferSize);
    Serial.println(F(" bytes"));
  }

  internalBufferSize = _internalBufferSize;
  internalBuffer = (char*) malloc(internalBufferSize); 
//
//  recvBufferSize = 512;
//  recvBuffer = (char *) malloc(recvBufferSize);

// Wait until the module is ready to accept AT commands
  while(!isReady()) {
    Serial.println(F("Problem to initialize AT command, retry in 1 sec"));
    delay(1000);
  }
  Serial.println(F("Setup Complete!"));


   // Wait for the GSM signal
  uint8_t signal = getSignal();
  while(signal <= 0) {
    delay(1000);
    signal = getSignal();
  }
  Serial.print(F("Signal OK (strenght: "));
  Serial.print(signal);
  Serial.println(F(")"));
  delay(1000);

  // Wait for operator network registration (national or roaming network)
  NetworkRegistration network = getRegistrationStatus();
  while(network != REGISTERED_HOME && network != REGISTERED_ROAMING) {
    delay(1000);
    network = getRegistrationStatus();
  }
  Serial.println(F("Network registration OK"));
  delay(1000);

  // Setup APN for GPRS configuration
  bool success = setupGPRS(APN);
  while(!success) {
    success = setupGPRS(APN);
    delay(5000);
  }
  Serial.println(F("GPRS config OK"));
}





//////////////////////////////////////////////////////////////////////////////////

/**
 * Setup the GPRS connectivity
 * As input, give the APN string of the operator
 */
bool setupGPRS(const char* apn) {
  // Prepare the GPRS connection as the bearer
  sendCommand_P(AT_CMD_SAPBR_GPRS);
  if(!readResponseCheckAnswer_P(20000, AT_RSP_OK)) {
    return false;
  }

  // Set the config of the bearer with the APN
  sendCommand_P(AT_CMD_SAPBR_APN, apn);
  return readResponseCheckAnswer_P(20000, AT_RSP_OK);
}


/**
 * Status function: Check if the module is registered on the network
 */
NetworkRegistration getRegistrationStatus() {
  sendCommand_P(AT_CMD_CREG_TEST);
  if(readResponse(DEFAULT_TIMEOUT)) {
    // Check if there is an error
    int16_t errIdx = strIndex(internalBuffer, "ERROR");
    if(errIdx > 0) {
      return NET_ERROR;
    }

    // Extract the value
    int16_t idx = strIndex(internalBuffer, "+CREG: ");
    char value = internalBuffer[idx + 9];
  
    // Prepare the clear output
    switch(value) {
      case '0' : return NOT_REGISTERED;
      case '1' : return REGISTERED_HOME;
      case '2' : return SEARCHING;
      case '3' : return DENIED;
      case '5' : return REGISTERED_ROAMING;
      default  : return NET_UNKNOWN;
    }
  }
  
  return NET_ERROR;
}

/**
 * Status function: Check the strengh of the signal
 */
uint8_t getSignal() {
  sendCommand_P(AT_CMD_CSQ);
  if(readResponse(DEFAULT_TIMEOUT)) {
    int16_t idxBase = strIndex(internalBuffer, "AT+CSQ");
    if(idxBase != 0) {
      return 0;
    }
    int16_t idxEnd = strIndex(internalBuffer, ",", idxBase);
    uint8_t value = internalBuffer[idxEnd - 1] - '0';
    if(internalBuffer[idxEnd - 2] != ' ') {
      value += (internalBuffer[idxEnd - 2] - '0') * 10;
    }
    if(value > 31) {
      return 0;
    }
    return value;
  }
  return 0;
}


bool isReady() {
  sendCommand_P(AT_CMD_BASE);
  return readResponseCheckAnswer_P(DEFAULT_TIMEOUT, AT_RSP_OK);
}


/**
 * Send AT command coming from the PROGMEM with a parameter
 */
void sendCommand_P(const char* command, const char* parameter) {
  char cmdBuff[32];
  strcpy_P(cmdBuff, command);
  sendCommand(cmdBuff, parameter);
}

/**
 * Send AT command coming from the PROGMEM
 */
void sendCommand_P(const char* command) {
  char cmdBuff[32];
  strcpy_P(cmdBuff, command);
  sendCommand(cmdBuff);
}


/**
 * Send AT command to the module
 */
void sendCommand(const char* command) {
  if(enableDebug) {
    Serial.println();
    Serial.print(F("SIM800L : Send \""));
    Serial.print(command);
    Serial.println(F("\""));
  }
  
  purgeSerial();
  SIM800.println(command);
  SIM800.print("\r\n");
  purgeSerial();
}

/**
 * Send AT command to the module with a parameter
 */
void sendCommand(const char* command, const char* parameter) {
  if(enableDebug) {
    Serial.println();
    Serial.print(F("SIM800L : Send \""));
    Serial.print(command);
    Serial.print(F("\""));
    Serial.print(parameter);
    Serial.print(F("\""));
    Serial.print(F("\""));
    Serial.print("\r\n");
  }
  
  purgeSerial();
  SIM800.print(command);
  SIM800.print("\"");
  SIM800.print(parameter);
  SIM800.print("\"");
  SIM800.print("\r\n");
  purgeSerial();
}


/**
 * Purge the serial data
 */
void purgeSerial() {
  SIM800.flush();
  while (SIM800.available()) {
    SIM800.read();
  }
  SIM800.flush();
}


/**
 * Find string "findStr" in another string "str"
 * Returns true if found, false elsewhere
 */
int16_t strIndex(const char* str, const char* findStr) {
  return strIndex(str, findStr, 0);
}

int16_t strIndex(const char* str, const char* findStr, uint16_t startIdx) {
  int16_t firstIndex = -1;
  int16_t sizeMatch = 0;
  for(int16_t i = startIdx; i < strlen(str); i++) {
    if(sizeMatch >= strlen(findStr)) {
      break;
    }
    if(str[i] == findStr[sizeMatch]) {
      if(firstIndex < 0) {
        firstIndex = i;
      }
      sizeMatch++;
    } else {
      firstIndex = -1;
      sizeMatch = 0;
    }
  }

  if(sizeMatch >= strlen(findStr)) {
    return firstIndex;
  } else {
    return -1;
  }
}


/**
 * Read from module and expect a specific answer (timeout in millisec)
 */
bool readResponseCheckAnswer_P(uint16_t timeout, const char* expectedAnswer) {
  return readResponseCheckAnswer_P(timeout, expectedAnswer, 2);
}

bool readResponseCheckAnswer_P(uint16_t timeout, const char* expectedAnswer, uint8_t crlfToWait) {
  if(readResponse(timeout, crlfToWait)) {
    // Prepare the local expected answer
    char rspBuff[16];
    strcpy_P(rspBuff, expectedAnswer);
    
    // Check if it's the expected answer
    int16_t idx = strIndex(internalBuffer, rspBuff);
    if(idx > 0) {
      return true;
    }
  }
  return false;
}


/**
 * Init internal buffer
 */
void initInternalBuffer() {
  for(uint16_t i = 0; i < internalBufferSize; i++) {
    internalBuffer[i] = '\0';
  }
}


/**
 * Read from the module for a specific number of CRLF
 * True if we have some data
 */
bool readResponse(uint16_t timeout) {
  return readResponse(timeout, 2);
}
 
bool readResponse(uint16_t timeout, uint8_t crlfToWait) {
  uint16_t currentSizeResponse = 0;
  bool seenCR = false;
  uint8_t countCRLF = 0;

  // First of all, cleanup the buffer
  initInternalBuffer();
  
  uint32_t timerStart = millis();

  while(1) {
    // While there is data available on the buffer, read it until the max size of the response
    if(SIM800.available()) {
      // Load the next char
      internalBuffer[currentSizeResponse] = SIM800.read();

      // Detect end of transmission (CRLF)
      if(internalBuffer[currentSizeResponse] == '\r') {
        seenCR = true;
      } else if (internalBuffer[currentSizeResponse] == '\n' && seenCR) {
        countCRLF++;
        if(countCRLF == crlfToWait) {
          if(enableDebug) Serial.println(F("SIM800L : End of transmission"));
          break;
        }
      } else {
        seenCR = false;
      }

      // Prepare for next read
      currentSizeResponse++;

      // Avoid buffer overflow
      if(currentSizeResponse == internalBufferSize) {
        if(enableDebug) Serial.println(F("SIM800L : Received maximum buffer size"));
        break;
      }
    }

    // If timeout, abord the reading
    if(millis() - timerStart > timeout) {
      if(enableDebug) Serial.println(F("SIM800L : Receive timeout"));
      // Timeout, return false to parent function
      return false;
    }
  }

  if(enableDebug) {
    Serial.print(F("SIM800L : Receive \""));
    Serial.print(internalBuffer);
    Serial.println(F("\""));
  }

  // If we are here, it's OK ;-)
  return true;
}
