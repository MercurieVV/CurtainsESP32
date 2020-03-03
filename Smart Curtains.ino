//#include <ESP8266WiFi.h>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <AccelStepper.h>
#include <WiFi.h>

#define MQTT_VERSION MQTT_VERSION_3_1_1

String clientId = "ESP32-Shabarova"; //Шабарова, Зоя Алексеевна
#define MQTT_ID "/ESP32-Shabarova/"
#define MQTT_STEP2 "/ESP32-Shabarova/Borisov/" //Борисов Юрий Алексеевич
#define MQTT_STEP1 "/ESP32-Shabarova/Obuhov/"  //Обухов Дмитрий Константинович
#define PUB_STEPS2 "/ESP32-Shabarova/Borisov_step/"
#define PUB_STEPS1 "/ESP32-Shabarova/Obuhov_step/"
#define CURTMAXIMUM 550
#define STOPHYSTERESIS 5
#define MSG_BUFFER_SIZE 20

const char *Topic1 = MQTT_STEP1;
//to do add array of pins and steppers
int switch_1_pin = 17;
int switch_2_pin = 16;

int switch_3_pin = 14;
int switch_4_pin = 12;

int32_t got_int1;
int32_t got_int2;
bool CurtHyster1 = false;
int32_t steps_from_zero1 = 0;
bool CurtHyster2 = false;
int32_t steps_from_zero2 = 0;
char m_msg_buffer[MSG_BUFFER_SIZE];

//int LED_BUILTIN = 2;

// Define a stepper and the pins it will use
AccelStepper stepper1(AccelStepper::HALF4WIRE, 32, 25, 33, 26); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5
AccelStepper stepper2(AccelStepper::HALF4WIRE, 23, 21, 22, 19);

// Update these with values suitable for your network.
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

const char *p_payload;
float got_float;
int i;

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(MQTT_STEP1);
  client.subscribe(MQTT_STEP2);

  Serial.println("AWS IoT Connected!");
}

void messageHandler(String &topic, String &payload)
{
  Serial.println(topic);
  const char *cstr = payload.c_str();
  got_float = atof(cstr);
  if (strcmp(topic.c_str(), MQTT_STEP1) == 0)
  {
    got_int1 = (int)got_float * 100;
    stepper1.moveTo(got_int1);
    stepper1.run();
  }
  else
  {
    got_int2 = (int)got_float * 100;
    stepper2.moveTo(got_int2);
    stepper2.run();
  }
}

//end callback

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    // Create a random client ID

    //clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {

      //once connected to MQTT broker, subscribe command if any
      client.subscribe(MQTT_STEP1);
      client.subscribe(MQTT_STEP2);
    }
    else
    {
      // Wait 6 seconds before retrying
      delay(6000);
    }
  }
} //end reconnect()

void setup()
{
  Serial.begin(115200);
  gpio_pad_select_gpio(LED_BUILTIN);
    /* Set the GPIO as a push/pull output */
  pinMode(LED_BUILTIN, GPIO_MODE_OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  pinMode(switch_1_pin, INPUT_PULLUP);
  pinMode(switch_2_pin, INPUT_PULLUP);
  pinMode(switch_3_pin, INPUT_PULLUP);
  pinMode(switch_4_pin, INPUT_PULLUP);
  connectAWS();
  // set the speed at 600 rpm:
  stepper1.setMaxSpeed(600);
  stepper1.setAcceleration(300);
  stepper2.setMaxSpeed(600);
  stepper2.setAcceleration(300);
}

//to do rebase for 1 function
void checkStep1 (void)
{
  if (CurtHyster1 == true)
  {
    if (((steps_from_zero2 > STOPHYSTERESIS) && (steps_from_zero2 < CURTMAXIMUM - STOPHYSTERESIS)) || (steps_from_zero2 < -STOPHYSTERESIS) || (steps_from_zero2 > CURTMAXIMUM + STOPHYSTERESIS))
    {
      CurtHyster1 = false;
    }
  }
  else
  {
    if (digitalRead(switch_1_pin) == LOW)
    {
      stepper1.stop();
      stepper1.disableOutputs();
      stepper1.setCurrentPosition(CURTMAXIMUM * 100);
      CurtHyster1 = true;
    }

    //myStepper.step(-stepsPerRevolution);
    if (digitalRead(switch_2_pin) == LOW)
    {
      stepper1.stop();
      stepper1.disableOutputs();
      stepper1.setCurrentPosition(0);
      CurtHyster1 = true;
    }
  }
  if (steps_from_zero1 != stepper1.currentPosition() / 100)
  {
    steps_from_zero1 = stepper1.currentPosition() / 100;
    //String thisString = String(steps_from_zero1);
    //thisString.toCharArray(warn, 8);
    snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", steps_from_zero1);
    client.publish(PUB_STEPS1, m_msg_buffer, true);
  }
  //delay(1000);

  if (got_int1 != stepper1.currentPosition())
  {
    stepper1.run();
  }
}

void checkStep2 (void)
{
  if (CurtHyster2 == true)
  {
    if (((steps_from_zero2 > STOPHYSTERESIS) && (steps_from_zero2 < CURTMAXIMUM - STOPHYSTERESIS)) || (steps_from_zero2 < -STOPHYSTERESIS) || (steps_from_zero2 > CURTMAXIMUM + STOPHYSTERESIS))
    {
      CurtHyster2 = false;
    }
  }
  else
  {
    if (digitalRead(switch_3_pin) == LOW)
    {
      stepper2.stop();
      stepper2.disableOutputs();
      stepper2.setCurrentPosition(CURTMAXIMUM * 100);
      CurtHyster2 = true;
    }

    //myStepper.step(-stepsPerRevolution);
    if (digitalRead(switch_4_pin) == LOW)
    {
      stepper2.stop();
      stepper2.disableOutputs();
      stepper2.setCurrentPosition(0);
      CurtHyster2 = true;
    }
  }
  if (steps_from_zero2 != stepper2.currentPosition() / 100)
  {
    steps_from_zero2 = stepper2.currentPosition() / 100;
    //String thisString = String(steps_from_zero2);
    //thisString.toCharArray(warn, 8);
    snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", steps_from_zero2);
    client.publish(PUB_STEPS2, m_msg_buffer, true);
  }
  //delay(1000);

  if (got_int2 != stepper2.currentPosition())
  {
    stepper2.run();
  }
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
    delay(1000);
  }
  checkStep1();
  checkStep2();
  client.loop();
}



