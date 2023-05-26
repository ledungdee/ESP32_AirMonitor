#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareSerial.h>
#include <WebServer.h>


boolean readPMSdata(Stream *s);
void lcd_Init();
void wifi_Init();
void GSM_Init();
void send_SMS(String sms);
void server_Init();
void handle_OnConnect();
void handle_NotFound();
void check_MQ135();
void check_DHT11();
void check_PMS();
String SendHTML(int val1, int val2, int val3);

#define BUZZER 23
#define MQ135_D 18
#define DHT_SENSOR_PIN  19 
#define DHT_SENSOR_TYPE DHT22
#define RL1 32
#define RL2 33


//bool ledState = 0;
//bool buzzerState = 0;
bool gasDetect = 0;
bool alert = 0;
bool RL1State = 0;
bool RL2State = 0;
const char* ssid = "A3a.10";
const char* password = "888888Aa";
char default_num[13] = {'+', '8', '4', '9', '6', '7', '9', '9', '2', '2', '9', '8' };
float humi = 0, tempC = 0, tempF = 0;

struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};
struct pms5003data data;

DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
LiquidCrystal_I2C lcd(0x27,20,4);
WebServer server(80);


void setup() 
{
  pinMode(MQ135_D, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RL1, OUTPUT);
  pinMode(RL2,OUTPUT);
  digitalWrite(BUZZER, LOW);
  digitalWrite(RL1, LOW);
  digitalWrite(RL2, LOW);

  Serial.begin(9600);
  dht_sensor.begin();
  Serial2.begin(9600);

  lcd_Init();
  wifi_Init();
  GSM_Init();
  server_Init();
}

void loop() {
  check_MQ135();
  check_DHT11();
  check_PMS();
  server.handleClient();
}
void lcd_Init(){
  lcd.init();         
  lcd.backlight();
  lcd.cursor_off();
  lcd.setCursor(1,0);
  lcd.print("LE DUNG - 1912950");
  lcd.setCursor(6,2);
  lcd.print("Project 2");
  delay(1500);
}

void wifi_Init(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Initializing....");
  delay(1000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED);
  lcd.setCursor(0,1);
  lcd.print("WiFi connected..!");
  lcd.setCursor(0,2);
  lcd.print("IP:");
  lcd.print(WiFi.localIP());
  delay(3000);
  lcd.clear();
}

void GSM_Init(){
  Serial.println("AT+CMGF=1"); // Configuring TEXT mode
  delay(100);
  Serial.println("AT&W");
  delay(100);
}

void send_SMS(String sms)
{
  Serial.print("AT+CMGS=\"");
  Serial.print(default_num);
  Serial.println("\"");
  delay(50);
  Serial.print(sms); //text content
  Serial.write(26);
}

void server_Init(){
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.begin();
  delay(1000);

}

void handle_OnConnect(){
  server.send(200, "text/html", SendHTML(data.pm10_env, data.pm25_env, data.pm100_env));
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(int val1, int val2, int val3){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>Wireless Weather Station</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr += "p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "<script>\n";
  ptr += "setInterval(loadDoc,1000);\n";
  ptr += "function loadDoc() {\n";
  ptr += "var xhttp = new XMLHttpRequest();\n";
  ptr += "xhttp.onreadystatechange = function() {\n";
  ptr += "if (this.readyState == 4 && this.status == 200) {\n";
  ptr += "document.body.innerHTML =this.responseText}\n";
  ptr += "};\n";
  ptr += "xhttp.open(\"GET\", \"/\", true);\n";
  ptr += "xhttp.send();\n";
  ptr += "}\n";
  ptr += "</script>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>Particulate Matter Monitoring</h1>\n";
 
  ptr += "<p>PM1.0: ";
  ptr += val1;
  ptr += " ug/m3</p>";
 
  ptr += "<p>PM2.5: ";
  ptr += val2;
  ptr += " ug/m3</p>";
 
  ptr += "<p>PM10: ";
  ptr += val3;
  ptr += " ug/m3</p>";
 
  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}


void check_MQ135(){
  if (!digitalRead(MQ135_D) && gasDetect == 0 && alert == 0)
  {
    delay(20);
    if (!digitalRead(MQ135_D))
    {
      gasDetect = 1;
      digitalWrite(LED_BUILTIN, HIGH);
      //ledState = 1;
      digitalWrite(BUZZER, HIGH);
      //buzzerState = 1;
      send_SMS("Gas detected!");
      alert = 1;
    }
  }

  if (digitalRead(MQ135_D) && gasDetect == 1)
  {
    delay(20);
  if (digitalRead(MQ135_D))
    {
      gasDetect = 0;
      digitalWrite(LED_BUILTIN, LOW);
      //ledState = 0;
      digitalWrite(BUZZER, LOW);
      //buzzerState = 0;
      alert = 0;
    }
  }
}

void check_DHT11(){
  // read humidity
  humi  = dht_sensor.readHumidity();
  // read temperature in Celsius
  tempC = dht_sensor.readTemperature();
  // read temperature in Fahrenheit
  tempF = dht_sensor.readTemperature(true);

  // // check whether the reading is successful or not
  // if ( isnan(tempC) || isnan(tempF) || isnan(humi)) {
  //   Serial.println("Failed to read from DHT sensor!");
  // } 
  // else {
    lcd.setCursor(0,0);
    lcd.print("T:");
    lcd.print(tempC);
    lcd.print((char)223);
    lcd.print("C");

    lcd.setCursor(12,0);
    lcd.print("H:");
    lcd.print(humi);
    lcd.print("%");

    if (tempC >= 35 || humi > 75 && RL1State == 0){
      digitalWrite(RL1, HIGH);
      RL1State = 1;
    }
    if (tempC < 35 && humi < 75 && RL1State == 1){
      digitalWrite(RL1, LOW);
      RL1State = 0;
    }
  // }
}

void check_PMS(){
  if(readPMSdata(&Serial2)) {
    lcd.setCursor(0,1);
    lcd.print("PM1.0:");
    lcd.setCursor(6,1); 
    lcd.print("   ");
    lcd.setCursor(6,1); 
    lcd.print(data.pm10_env);
    lcd.setCursor(9,1);
    lcd.print("ug/m3");
    lcd.setCursor(0,2);   
    lcd.print("PM2.5:");
    lcd.setCursor(6,2); 
    lcd.print("   ");
    lcd.setCursor(6,2); 
    lcd.print(data.pm25_env);
    lcd.setCursor(9,2);
    lcd.print("ug/m3");
    lcd.setCursor(0,3);   
    lcd.print("PM10 :"); 
    lcd.setCursor(6,3); 
    lcd.print("   ");
    lcd.setCursor(6,3);
    lcd.print(data.pm100_env);
    lcd.setCursor(9,3);
    lcd.print("ug/m3");
  }
}

boolean readPMSdata(Stream *s) {
  if (! s->available()) {
    return false;
  }

  // Read a byte at a time until we get to the special '0x42' start-byte
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }

  // Now read all 32 bytes
  if (s->available() < 32) {
    return false;
  }

  uint8_t buffer[32];
  uint16_t sum = 0;
  s->readBytes(buffer, 32);

  // get checksum ready
  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }

  /* debugging
    for (uint8_t i=2; i<32; i++) {
    Serial.print("0x"); Serial.print(buffer[i], HEX); Serial.print(", ");
    }
    Serial.println();
  */

  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i = 0; i < 15; i++) {
    buffer_u16[i] = buffer[2 + i * 2 + 1];
    buffer_u16[i] += (buffer[2 + i * 2] << 8);
  }

  // put it into a nice struct :)
  memcpy((void *)&data, (void *)buffer_u16, 30);

  if (sum != data.checksum) {
    //Serial.println("Checksum failure");
    return false;
  }
  // success!
  return true;
}

