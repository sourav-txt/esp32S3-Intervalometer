/**
   A BLE client example that is rich in capabilities.
   There is a lot new capabilities implemented.
   author unknown
   updated by chegewara
*/

#include <Arduino.h>
#include "BLEDevice.h"
//#include "BLEScan.h"
#include "HIDKeys.h"
//wifi headers
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
//////////////LED ///////////////////
#include <FastLED.h>
#define NUM_LEDS 1         //Number of RGB LED beads
#define LED_TYPE WS2812B  //RGB LED strip type
CRGB leds[NUM_LEDS];       //Instantiate RGB LED
////////////////////////////////////
#define RGB_PIN 21        //The pin for controlling RGB LED //D8 for DFROBOT
int focusPin = 17;
int shutterPin = 18;
/////////////////////////////////////

bool intrvalmtrMode = false;
int Delay = 50;
int Shutter= 100;
int Interval= 100;
int Shots= 1;


// The remote service we wish to connect to.
static BLEUUID serviceUUID((uint16_t)0x1812);
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID((uint16_t)0x2a4d);

//int ledPin = D9; //Define LED pin
static bool doConnect = false;
static bool connected = false;
static bool doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;


AsyncWebServer server(80);

const char* ssid = "ddwrt";
const char* password = "Ngjm99erTL5ikD";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
void shutter(int delayTime = 50) {
  digitalWrite(focusPin, HIGH);
  //Serial.print("Focus ");
  delay(100);
  digitalWrite(shutterPin, HIGH);
  //Serial.print("Shutter \n");
  //Serial.println("");
  delay(delayTime);
  digitalWrite(shutterPin, LOW);
  digitalWrite(focusPin, LOW);
}

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (pData[0] == 0x1) {
    CRGB lastcolor = leds[0];
    leds[0] = CRGB::Red;  //LED shows red light
    FastLED.show();
    shutter();
    Serial.println("Delay :");
    Serial.println(Delay);
    Serial.println("Shutter: ");
    Serial.println(Shutter);
    Serial.println("Interval: ");
    Serial.println(Interval);
    Serial.println("Shots: ");
    Serial.println(Shots);
    #ifdef DEBUG
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    for (size_t i = 0; i < length; i++) {
      Serial.printf("%2x", pData[i]);
    }
    Serial.println("");

    if (pData[0] == 0x0 && pData[2] != 0x0) {
      Serial.printf("string_0: %c", keys[pData[2]]);
    } else if (pData[0] == 0x02 && pData[2] != 0x0) {
      Serial.printf("string_2: %c", shift_keys[pData[2]]);
    }
    #endif

    leds[0] = lastcolor;  // set our current dot to black before we continue
    FastLED.show();
  }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    connected = true;
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
    leds[0] = CRGB::Black;
    FastLED.show();
  }
};

bool connectToServer() {

  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();

  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  if (!pClient->connect(myDevice)) {  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    return false;
  }

  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {

    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());

    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  do {
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {

      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());

      pClient->disconnect();
      return false;
    }

    if (pRemoteCharacteristic->canNotify())
      break;
  } while (1);

  Serial.println(" - Found our characteristic");

  pRemoteCharacteristic->registerForNotify(notifyCallback);

  return true;
}
/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
        Called for each advertising BLE server.
    */
  void onResult(BLEAdvertisedDevice advertisedDevice) {

    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    }  // Found our server
  }    // onResult
};     // MyAdvertisedDeviceCallbacks


const char* PARAM_INPUT_1 = "Delay";
const char* PARAM_INPUT_2 = "Shutter";
const char* PARAM_INPUT_3 = "Interval";
const char* PARAM_INPUT_4 = "Shots";
const char* PARAM_INPUT_5 = "state";


// HTML web page to handle 3 input fields (Delay, Shutter, Interval)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    Delay: <input type="text" name="Delay">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    Shutter: <input type="text" name="Shutter">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    Interval: <input type="text" name="Interval">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    Shots: <input type="text" name="Shots">
    <input type="submit" value="Submit">
  </form>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      if( this.responseText == 1){ 
        inputChecked = true;
        outputStateM = "On";
      }
      else { 
        inputChecked = false;
        outputStateM = "Off";
      }
      document.getElementById("output").checked = inputChecked;
      document.getElementById("outputState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000 ) ;
</script>
</body></html>)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String outputStateValue = outputState();
    buttons+= "<h4>Intervalometer Mode <span id=\"outputState\"></span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

String outputState(){
  if(intrvalmtrMode){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}

void wifisetup(){////////////////////////////////wifi setup//////////////////////
    WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?Delay=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET Delay value on <ESP_IP>/get?Delay=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      Delay = inputMessage.toInt();
    }
    // GET Shutter value on <ESP_IP>/get?Shutter=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
      Shutter= inputMessage.toInt();
    }
    // GET Interval value on <ESP_IP>/get?Interval=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
      Interval=inputMessage.toInt();
      
    }
     else if (request->hasParam(PARAM_INPUT_4)) {
      inputMessage = request->getParam(PARAM_INPUT_4)->value();
      inputParam = PARAM_INPUT_4;
      Shots=inputMessage.toInt();
    }
    else {
      inputMessage = "Form submit error";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send_P(200, "text/html", index_html, processor);
    
  });
  ////////////////////////////////////
  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_5)) {
      inputMessage = request->getParam(PARAM_INPUT_5)->value();
      inputParam = PARAM_INPUT_5;
      intrvalmtrMode = !intrvalmtrMode;
    }
    else {
      inputMessage = "Checkbox error";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/state
  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain",((intrvalmtrMode)?"1":"") );
  });
  //////////////////////////////////////////////////////////////////////////////////////
  server.onNotFound(notFound);
  server.begin();
  }

void setup() {
  Serial.begin(115200);
  wifisetup();
  Serial.println("Starting Arduino BLE Client application...");

  /////////////////////////////////////////////////////////////////////////////
  FastLED.addLeds<LED_TYPE, RGB_PIN>(leds, NUM_LEDS);  //Initialize RGB LED
                                                        /////////////////////////////////////////////////////////////////////////////
  pinMode(focusPin, OUTPUT);
  pinMode(shutterPin, OUTPUT);
  BLEDevice::init("");
  
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

  //wifisetup();
  
}  // End of setup.


// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
      leds[0] = CRGB::Blue;  //LED shows BLUE light
      FastLED.show();
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    // do nothing, all data is handled in notifications callback
  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  delay(100);  // Delay a second between loops.
}  // End of loop
