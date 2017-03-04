
/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /* Create a WiFi access point and provide a web server on it. */

#include <Hash.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "Debug_ClientSsid.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include "DRV8830MotorDriver.h"
#include <ArduinoJson.h>
#include <Servo.h>

/* Set these to your desired credentials. */
const char *ssid = "MKZ4";
const char *password = "";

/* set I2C library*/
#include <Wire.h>
#define ADDR1  0x64

#define command_start  0
#define command_stop   1
#define command_back  2
#define forward       0x01
#define reverse       0x02
#define servo_left    65
#define servo_right   120   // 110 -> 120

#define LED_H       (digitalWrite( 12, HIGH ))
#define LED_L       (digitalWrite( 12, LOW ))

template <class T> T map(T x, T in_min, T in_max, T out_min, T out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
};

/**
 * Servoクラスのラッパ
 * アナログスティックの値、左右に-1.0〜1.0の範囲を直接渡せるようにする。
 */
class MKZ4Servo : public Servo
{
private:
	int _maxRight = 0;
	int _maxLeft = 0;
	int _steerCenter = 90;

public:
	MKZ4Servo(int maxLeftRad, int maxRightRad)
	: Servo(), _maxLeft(maxLeftRad), _maxRight(maxRightRad)
	{
	}

	void addTrim(int trim){
		_steerCenter += trim;
	}

	void steer(float val)
	{
		int rad = _steerCenter;

		if(val > 0) {
			rad = (int)map(val, 0.0, 1.0, _steerCenter, _maxRight);
		}
		else if(val < 0) {
			rad = (int)map(val, 0.0, -1.0, _steerCenter, _maxLeft);
		}
		write(rad);
	}
};

class ControlValues {
public:
	float axel;
	float steer;

	bool merge(ControlValues &src)
	{
		bool chg = false;

		if(axel != src.axel){
			chg |= true;
			axel = src.axel;
		}

		if(steer != src.steer){
			chg != true;
			steer = src.steer;
		}

		return chg;
	}
};

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DRV8830MotorDriver drv8830(ADDR1);
MKZ4Servo mkz4Servo(servo_left, servo_right);
StaticJsonBuffer<200> jsonBuff;
ControlValues lastControl;

char state = command_stop;
int offset = 10;


/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */
void setup() {
	delay(1000);
	Serial.begin(115200);
	Serial.println();
	Serial.print(F("Configuring access point..."));

	Wire.begin(4, 14);
	delay(40);

#if 1
	WiFi.begin(DebugClientSsid, DebugClientPswd);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println(F(""));
	Serial.println(F("WiFi connected"));
	Serial.println(F("IP address: "));
	Serial.println(WiFi.localIP());
#else
	/* You can remove the password parameter if you want the AP to be open. */
	WiFi.softAP(ssid, password);

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
#endif

	//server.on("/", handleRoot);

	drv8830.setSpeed(0);
	mkz4Servo.attach(16);
	mkz4Servo.steer(0.0);

	ws.onEvent(onWSEvent);
	server.addHandler(&ws);

	server.serveStatic("/", SPIFFS, "/")
	      .setDefaultFile("index.html");
	server.begin();

	Serial.println(F("HTTP server started"));
	pinMode(16, OUTPUT);
	pinMode(12, OUTPUT);

	LED_H;
	delay(100);
}

void onWSEvent(AsyncWebSocket * server,
			AsyncWebSocketClient * client,
			AwsEventType type,
			void * arg,
			uint8_t *data,
			size_t len)
{
  if(type == WS_EVT_CONNECT){
    //client connected
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    //client disconnected
    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    //error was received from the other end
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    //pong message was received (in response to a ping request maybe)
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      if(info->opcode == WS_TEXT){
		ControlValues ctl;
        data[len] = 0;
		JsonObject& json = jsonBuff.parseObject((char*)data);
		// parseJson(obj, method, )
		parseJson(json, &ctl);
		if(lastControl.merge(ctl)){
			setControl(ctl.axel, ctl.steer);
		}
      }
    }
  }
}

void loop() {
}

void parseJson(JsonObject &json, ControlValues *control)
{
	ControlValues tmp;
	auto orZero = [=](String s) { return s != NULL && s == "" ? s : String(F("0")); };

	if(control == NULL) {
		control = &tmp;
	}

	control->axel  = orZero(json[F("axel")]).toFloat();
	control->steer = orZero(json[F("steer")]).toFloat();
}

void setControl(float axel, float steer){
	int spd = map(axel, -1.0, 1.0, -255, 255);

	drv8830.setSpeed(spd);
	mkz4Servo.steer(steer);
}

#if 0
void handleRoot() {
	server.send(200, "text/html", form);
}

void handle_stop() {
	Serial.print("stop\r\n");
	LED_L;
	stop_motor();
	state = command_stop;
	LED_H;
	server_8080.send(200, "text/html", "");
}

void handle_forward() {
	Serial.print("forward\r\n");
	drive();
	servo_control(90);
	server_8080.send(200, "text/html", "");
}

void handle_back() {
	Serial.print("back\r\n");
	back();
	servo_control(90);
	server_8080.send(200, "text/html", "");
}

void handle_left() {
	Serial.print("left\r\n");
	servo_control(servo_left);
	server_8080.send(200, "text/html", "");
}

void handle_right() {
	Serial.print("right\r\n");
	servo_control(servo_right);
	server_8080.send(200, "text/html", "");
}

void handle_f_left() {
	Serial.print("f_left\r\n");
	drive();
	servo_control(servo_left);
	server_8080.send(200, "text/html", "");
}

void handle_f_right() {
	Serial.print("f_right\r\n");
	drive();
	servo_control(servo_right);
	server_8080.send(200, "text/html", "");
}

void handle_b_left() {
	Serial.print("b_left\r\n");
	back();
	servo_control(servo_left);
	server_8080.send(200, "text/html", "");
}


void handle_b_right() {
	Serial.print("b_right\r\n");
	back();
	servo_control(servo_right);
	server_8080.send(200, "text/html", "");
}
#endif

#if 0
void drive() {
	if (state == command_back) {
		stop_motor();

		delay(10);

		start_motor();

	}
	else if (state == command_stop) {
		start_motor();
	}
	state = command_start;
}

void back() {
	if (state == command_start) {
		stop_motor();
		delay(10);
		reverse_motor();
	}
	else if (state == command_stop) {
		reverse_motor();
	}
	state = command_back;
}

void start_motor() {
	char i, volt;
	volt = 0x20;
	for (i = 0; i < 5; i++) {
		volt = volt + ((0x40)*i);
		volt = volt | forward;
		motor_func(ADDR1, volt);
		delay(10);
	}
}

void reverse_motor() {
	char i, volt;
	volt = 0x20;
	for (i = 0; i < 5; i++) {
		volt = volt + ((0x40)*i);
		volt = volt | reverse;
		motor_func(ADDR1, volt);
		delay(10);
	}
}

void stop_motor() {
	motor_func(ADDR1, 0x18);
	delay(10);
	motor_func(ADDR1, 0x1B);
	delay(10);
}

void motor_func(char add, char duty) {
	Wire.beginTransmission(add);
	Wire.write(0x00);
	Wire.write(duty);
	Wire.endTransmission();
}

void servo_control(int angle) {
	int microsec, i;
	LED_L;
	microsec = (5 * (angle + offset)) + 1000;

	for (i = 0; i < 20; i++) {
		digitalWrite(16, HIGH);
		delayMicroseconds(microsec);
		digitalWrite(16, LOW);
		delayMicroseconds(10000 - microsec);
	}
	LED_H;
}
#endif

#if 0
#define MARGIN_STEER 0.001
#define MARGIN_AXEL  0.001

float servoCenter = 90.0;
float axelLast = 0.0;
float steerLast = 0.0;
float axelCenter = 0.0;
float steerCenter = 0.0;

// value < 0 : left
// value > 0 : right
void steering(float value)
{

	// 前回値と異なるか？
	if (steerLast != value)
	{
		// right
		if ((steerCenter + MARGIN_STEER) <= value)
		{
			float start = steerCenter + MARGIN_STEER; // 有効な開始点
			float delta = value - start;			  // 開始点からの差分
			float frange = 1.0f - start;		      // 有効範囲
			float radmax = (float)servo_right - servoCenter; // 右ハンドルの角度
			float rad = 1.0f / frange * delta * radmax;  // センサ値→角度
			servo_control(servoCenter + (int)rad);
		}
		// left
		if ((steerCenter - MARGIN_STEER) >= value)
		{
			float start = steerCenter - MARGIN_STEER; // 有効な開始点
			float delta = value - start;			  // 開始点からの差分
			float frange = -1.0f - start;		      // 有効範囲
			float radmax = (float)servo_left - servoCenter; // 右ハンドルの角度
			float rad = 1.0f / frange * delta * radmax;  // TODO 要再考 センサ値→角度
			servo_control(servoCenter + (int)rad);
		}
		steerLast = value;
	}
}
#endif
