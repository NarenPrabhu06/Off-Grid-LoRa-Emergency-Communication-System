#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>

#define SS 5
#define RST 14
#define DIO0 2

#define GPS_RX 16
#define GPS_TX 17

TinyGPSPlus gps;
HardwareSerial GPS(2);

WebServer server(80);

float latitude = 0.0;
float longitude = 0.0;

String chatLog = "";

const char* ssid = "RescueNet_D1";
const char* password = "12345678";

IPAddress local_IP(192,168,10,1);
IPAddress gateway(192,168,10,1);
IPAddress subnet(255,255,255,0);

String webpage(){

String page = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta name="viewport" content="width=device-width, initial-scale=1">

<style>

body{
background:#0d0d0d;
font-family:Arial;
color:#bb86fc;
padding:15px;
}

h2{
text-align:center;
color:#d0a9ff;
}

#chat{
height:320px;
overflow-y:auto;
background:#1a1a1a;
padding:10px;
border-radius:10px;
border:1px solid #bb86fc;
}

.msg{
background:#262626;
padding:8px;
margin:5px;
border-radius:8px;
color:#ffffff;
}

input{
width:70%;
padding:12px;
background:#222;
color:white;
border:none;
border-radius:8px;
}

button{
padding:12px;
background:#bb86fc;
color:black;
border:none;
border-radius:8px;
font-weight:bold;
}

.card{
margin-top:15px;
background:#1a1a1a;
padding:10px;
border-radius:10px;
border:1px solid #bb86fc;
color:white;
}

</style>

<script>

function sendMessage(){

let msg = document.getElementById("msg").value;

if(msg=="") return;

fetch("/send?msg=" + encodeURIComponent(msg));

document.getElementById("msg").value="";
}

function refreshData(){

fetch("/chat")
.then(response => response.text())
.then(data => {
document.getElementById("chat").innerHTML=data;
});

fetch("/gps")
.then(response => response.text())
.then(data => {
document.getElementById("gps").innerHTML=data;
});
}

setInterval(refreshData,2000);

</script>

</head>

<body>

<h2>RescueNet D1</h2>

<div id="chat"></div>

<br>

<input type="text" id="msg" placeholder="Type message">
<button onclick="sendMessage()">Send</button>

<div class="card">

<h3>Live GPS</h3>

<div id="gps"></div>

</div>

</body>

</html>

)rawliteral";

return page;
}

void handleRoot(){

server.send(200,"text/html",webpage());
}

void handleChat(){

server.send(200,"text/html",chatLog);
}

void handleGPS(){

String gpsData;

if(latitude != 0.0){

gpsData =
"Latitude: " + String(latitude,6) +
"<br>Longitude: " + String(longitude,6);

}else{

gpsData = "Waiting for GPS signal...";
}

server.send(200,"text/html",gpsData);
}

void handleSend(){

String msg = server.arg("msg");

String fullMsg = "D1: " + msg;

LoRa.beginPacket();
LoRa.print(fullMsg);
LoRa.endPacket();

chatLog += "<div class='msg'><b>ME:</b> " + msg + "</div>";

server.send(200,"text/plain","OK");
}

void setup(){

Serial.begin(115200);

GPS.begin(9600,SERIAL_8N1,GPS_RX,GPS_TX);

WiFi.mode(WIFI_AP);

WiFi.softAPConfig(local_IP,gateway,subnet);

WiFi.softAP(ssid,password);

server.on("/",handleRoot);
server.on("/chat",handleChat);
server.on("/gps",handleGPS);
server.on("/send",handleSend);

server.begin();

LoRa.setPins(SS,RST,DIO0);

if(!LoRa.begin(433E6)){

while(true);
}

LoRa.setSpreadingFactor(12);
LoRa.setSignalBandwidth(62.5E3);
LoRa.setCodingRate4(8);
LoRa.setTxPower(20);

LoRa.enableCrc();

Serial.println("D1 READY");
}

void loop(){

server.handleClient();

while(GPS.available()){

gps.encode(GPS.read());

if(gps.location.isUpdated()){

latitude = gps.location.lat();
longitude = gps.location.lng();
}
}

int packetSize = LoRa.parsePacket();

if(packetSize){

String incoming="";

while(LoRa.available()){

incoming += (char)LoRa.read();
}

chatLog += "<div class='msg'>" + incoming + "</div>";
}
}
