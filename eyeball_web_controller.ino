/*
 * EyeballKit Web Controller Example
 * Description: Controll your eyeball kit through web server. No phone app required.
 * Instructions:
  - Update WiFi SSID and password as necessary.
  - Flash this sketch to the ESP8266 board
  - Install host software:
    - For Linux, install Avahi (http://avahi.org/).
    - For Windows, install Bonjour (http://www.apple.com/support/bonjour/).
    - For Mac OSX and iOS support is built in through Bonjour already.
    - For Androids, sorry you can only access the web server through IP address.
  - Point your browser to http://[host].local, you should see a response.
 * Author: naozhendang.com
 * More info: http://www.naozhendang.com/p/eyeball-kit
 * Tutorial: http://www.naozhendang.com/t/eyeball-web-controller
 */
#include <Arduino.h>
#include <Servo.h> 

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>

/**************************************
 * !!!!!Config Here!!!
 * ***********************************/
const char* host = "eyeball-kit";     // http://eyeball-kit.local
const char* ssid     = "SSID NAME";         //Your wifi ssid
const char* password = "SSID PASSWORD";  //Your wifi password

Servo myservo_v;  // create servo object to control the vertical servo 
Servo myservo_h;  // create servo object to control the horizontal servo 

#define USE_SERIAL Serial

//index html
const char* serverIndex = "<html><head><meta charset='utf-8'><meta name='viewport' content='user-scalable=no,initial-scale=1,maximum-scale=1,minimum-scale=1,width=device-width'><title>眼球盒子</title><style>body{background:#0995ff;color:#fff;position:relative;text-align:center;font-family:'HelveticaNeue-Light','HelveticaNeue',Helvetica,Arial,sans-serif;font-size:13px;}h1{margin-top:50px;word-spacing:10px;font-size:2em;}h1 em{font-size:.6em;display:block;font-weight:normal;opacity:.8;}div{position:absolute;bottom:20px;width:100%;}</style></head><body><h1>眼球盒子遥控器<em>(试一试拖动黑圈)</em></h1><svg version='1.1' id='eye' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' x='0' y='0' width='300px' height='300px' viewBox='0 0 300 300' enable-background='new 0 0 300 300'><circle fill='#F1F1F1' cx='150' cy='150' r='150'></circle><circle id='eyeball' cx='150' cy='150' r='30'></circle></svg><div>2016 &copy; 脑震荡 naozhendang.com</div><script>var conn=new WebSocket('ws://'+location.hostname+':81/',['arduino']);conn.onopen=function(){connection.send('Connect '+new Date())};conn.onerror=function(a){console.log('WebSocket Error',a)};conn.onmessage=function(a){console.log('Server: ',a.data)};var drag=null,dPoint;var initPos=[150,150];document.getElementById('eye').addEventListener('mousedown',DragStart,false);document.getElementById('eye').addEventListener('mouseup',DragEnd,false);document.getElementById('eye').addEventListener('touchstart',DragStart,false);document.getElementById('eye').addEventListener('touchend',DragEnd,false);function DragStart(b){if(b.preventDefault){b.preventDefault()}var a=b.target?b.target:b.srcElement;if(a.id=='eyeball'){drag='eyeball';document.addEventListener('touchmove',Dragging,false);document.addEventListener('mousemove',Dragging,false);b=MousePos(b);dPoint=b}}function Dragging(d){var c=d.target?d.target:d.srcElement;d=MousePos(d);if(drag){initPos[0]+=parseInt(d.x-dPoint.x);initPos[1]+=parseInt(d.y-dPoint.y);if(Math.pow((initPos[0]-150),2)+Math.pow((initPos[1]-150),2)<=14400){document.getElementById(drag).setAttribute('cx',initPos[0]);document.getElementById(drag).setAttribute('cy',initPos[1]);var b=remap(initPos[0],31,269,45,135).toString(16);var a=remap(initPos[1],31,269,45,135).toString(16);if(b.length<2){b='0'+r}if(a.length<2){a='0'+g}var f='#'+b+a+'00';conn.send(f)}else{initPos[0]-=parseInt(d.x-dPoint.x);initPos[1]-=parseInt(d.y-dPoint.y)}dPoint=d}}function DragEnd(b){var a=b.target?b.target:b.srcElement;document.removeEventListener('touchmove',Dragging,false);document.removeEventListener('mousemove',Dragging,false);drag=null}function MousePos(a){a=(a?a:window.event);if(a.type=='touchmove'||a.type=='touchstart'){return{x:a.touches[0].pageX,y:a.touches[0].pageY}}else{return{x:a.pageX,y:a.pageY}}}function remap(a,b,d,e,c){return parseInt((a-b)*(c-e)/(d-b)+e)};</script></body></html>";

ESP8266WebServer server = ESP8266WebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

            // send message to client
            webSocket.sendTXT(num, "Connected");
        }
            break;
        case WStype_TEXT:
            USE_SERIAL.printf("[%u] get Data: %s\n", num, payload);

            if(payload[0] == '#') {
                // get and decode data
                uint32_t xyz = (uint32_t) strtol((const char *) &payload[1], NULL, 16);
                myservo_h.write(((xyz >> 16) & 0xFF));
                myservo_v.write(((xyz >> 8) & 0xFF));
            }

            break;
    }

}

//index page
void handleRoot() {
  server.send(200, "text/html", serverIndex);
}

//404 page
void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
    myservo_v.attach(4);  // attach the servo on GPIO4 to the servo object
    myservo_h.attach(5);  // attach the servo on GPIO5 to the servo object
    
    USE_SERIAL.begin(115200);

    //USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

      for(uint8_t t = 4; t > 0; t--) {
          USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
          USE_SERIAL.flush();
          delay(1000);
      }

    WiFi.disconnect();
    WiFi.begin(ssid,password);
    USE_SERIAL.println();

    while(WiFi.status() != WL_CONNECTED) {
        delay(100);
        USE_SERIAL.print(".");
    }

    USE_SERIAL.print("\nIP address: ");
    USE_SERIAL.flush();
    USE_SERIAL.println(WiFi.localIP());

    if(!MDNS.begin(host)) {
        USE_SERIAL.println("Error setting up MDNS responder!");
        while(1) { 
           delay(1000);
        }
    }
    USE_SERIAL.printf("MDNS responder started: http://%s.local",host);

    // start webSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    server.on("/", handleRoot);
    server.onNotFound(handleNotFound);

    server.begin();

    // Add service to MDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
}

void loop() {
    webSocket.loop();
    server.handleClient();
}
