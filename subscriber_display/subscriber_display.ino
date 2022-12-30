#include <LTE.h>
#include <ArduinoMqttClient.h>
#include <SPI.h>
#include <Audio.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Fonts/FreeSerifItalic9pt7b.h"
#include <SDHCI.h>

// For the Adafruit shield, these are the default.
#define TFT_CS 32
#define TFT_DC 9
#define TFT_MOSI 31
// #define TFT_CLK 29
#define TFT_CLK 7
#define TFT_RST 6
#define TFT_MISO 30

// APN name
#define APP_LTE_APN "iijmio.jp" // replace your APN

/* APN authentication settings
 * Ignore these parameters when setting LTE_NET_AUTHTYPE_NONE.
 */
#define APP_LTE_USER_NAME "mio@iij"     // replace with your username
#define APP_LTE_PASSWORD  "iij" // replace with your password

// APN IP type
#define APP_LTE_IP_TYPE (LTE_NET_IPTYPE_V4V6) // IP : IPv4v6
// #define APP_LTE_IP_TYPE (LTE_NET_IPTYPE_V4) // IP : IPv4
// #define APP_LTE_IP_TYPE (LTE_NET_IPTYPE_V6) // IP : IPv6

// APN authentication type
#define APP_LTE_AUTH_TYPE (LTE_NET_AUTHTYPE_CHAP) // Authentication : CHAP
// #define APP_LTE_AUTH_TYPE (LTE_NET_AUTHTYPE_PAP) // Authentication : PAP
// #define APP_LTE_AUTH_TYPE (LTE_NET_AUTHTYPE_NONE) // Authentication : NONE

/* RAT to use
 * Refer to the cellular carriers information
 * to find out which RAT your SIM supports.
 * The RAT set on the modem can be checked with LTEModemVerification::getRAT().
 */

#define APP_LTE_RAT (LTE_NET_RAT_CATM) // RAT : LTE-M (LTE Cat-M1)
// #define APP_LTE_RAT (LTE_NET_RAT_NBIOT) // RAT : NB-IoT

// MQTT broker
// #define BROKER_NAME "test.mosquitto.org"   // replace with your broker
#define BROKER_NAME "broker.emqx.io"
#define BROKER_PORT 1883                   // port 8883 is the default for MQTT over TLS.
                                           // for this client, if required by the server.

// MQTT topic
#define MQTT_TOPIC  "spresense/mqtt"       // replace with your topic

LTE lteAccess;
LTEClient client;
MqttClient mqttClient(client);
AudioClass *theAudio;

int numOfPubs = 0;
char broker[] = BROKER_NAME;
int port = BROKER_PORT;
char topic[]  = MQTT_TOPIC;

#define MQTT_ID "spresense_sub"
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

class Score
{
public:

  typedef struct {
    int fs;  
    int time;
  } Note;

  void init(){
    pos = 0;
  }
  
  Note get(){
    return data[pos++];
  }
  
private:

  int pos;

  Note data[17] =
  {
    {523,1000},
    {523,1000},
    {523,1000},
    {523,1000},
    {523,1000},
    {523,1000},
    {523,1000},
    {523,1000},

    {523,500},
    {494,500},
    {440,500},
    {392,500},
    {349,500},
    {330,500},
    {294,500},
    {262,1000},

    {0,0}
  };};

Score theScore;

void music_setup(){
  theAudio = AudioClass::getInstance();
  theAudio->begin();
  puts("initialization Audio Library");

  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, 0, 0);

  theScore.init();
}

void music_loop(){
  puts("loop!!");

  Score::Note theNote = theScore.get();
  if (theNote.fs == 0) {
    puts("End,");
    theAudio->setReadyMode();
    theAudio->end();
    // exit(1);
  }
  theAudio->setVolume(-160);
  theAudio->setBeep(1,0,theNote.fs);
  // theAudio->stopPlayer(AudioClass::Player0,AS_STOPPLAYER_NORMAL);

  /* The usleep() function suspends execution of the calling thread for usec
   * microseconds. But the timer resolution depends on the OS system tick time
   * which is 10 milliseconds (10,000 microseconds) by default. Therefore,
   * it will sleep for a longer time than the time requested here.
   */
 
  usleep(theNote.time * 1000);

  theAudio->setBeep(0,0,0);

  /* The usleep() function suspends execution of the calling thread for usec
   * microseconds. But the timer resolution depends on the OS system tick time
   * which is 10 milliseconds (10,000 microseconds) by default. Therefore,
   * it will sleep for a longer time than the time requested here.
   */
 
  usleep(100000);
}

// void printError(enum CamErr err)
// {
//   Serial.print("Error: ");
//   switch (err)
//     {
//       case CAM_ERR_NO_MEMORY: 
//         Serial.println("No memory");
//         break;

//       default:
//         break;
//     }
// }

void doAttach()
{
  while (true) {

    /* Power on the modem and Enable the radio function. */

    if (lteAccess.begin() != LTE_SEARCHING) {
      Serial.println("Could not transition to LTE_SEARCHING.");
      Serial.println("Please check the status of the LTE board.");
      for (;;) {
        sleep(1);
      }
    }

    /* The connection process to the APN will start.
     * If the synchronous parameter is false,
     * the return value will be returned when the connection process is started.
     */
    if (lteAccess.attach(APP_LTE_RAT,
                         APP_LTE_APN,
                         APP_LTE_USER_NAME,
                         APP_LTE_PASSWORD,
                         APP_LTE_AUTH_TYPE,
                         APP_LTE_IP_TYPE,
                         false) == LTE_CONNECTING) {
      Serial.println("Attempting to connect to network.");
      break;
    }

    /* If the following logs occur frequently, one of the following might be a cause:
     * - APN settings are incorrect
     * - SIM is not inserted correctly
     * - If you have specified LTE_NET_RAT_NBIOT for APP_LTE_RAT,
     *   your LTE board may not support it.
     */
    Serial.println("An error has occurred. Shutdown and retry the network attach preparation process after 1 second.");
    lteAccess.shutdown();
    sleep(1);
  }
}

void testText1(int bun) {
  unsigned long start = micros();
  tft.setFont(&FreeSerifItalic9pt7b);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(0, 120);
  tft.setTextSize(3);
  tft.println("boar");

  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(0, 120);
  tft.setTextSize(3);
  tft.println("fox");

  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(40, 120);
  tft.setTextSize(3);
  tft.println("OT.TECH");

  if(bun==0){
  tft.setCursor(0, 60);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  // tft.setFont(&FreeSerifItalic9pt7b);
  tft.println("I found");

  tft.setTextColor(ILI9341_RED);
  tft.setCursor(0, 120);
  tft.setTextSize(3);
  tft.println("boar");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(0, 180);
  tft.setTextSize(3);
  tft.println("Be careful");
  }
  else if(bun==1){
  tft.setCursor(0, 60);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setFont(&FreeSerifItalic9pt7b);
  tft.println("I found");

  tft.setTextColor(ILI9341_RED);
  tft.setCursor(0, 120);
  tft.setTextSize(3);
  tft.println("fox");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(0, 180);
  tft.setTextSize(3);
  tft.println("Be careful");
  }
  else if(bun==2){

  tft.setCursor(0, 60);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setFont(&FreeSerifItalic9pt7b);
  tft.println("I found");

  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(0, 180);
  tft.setTextSize(3);
  tft.println("Be careful");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(40, 120);
  tft.setTextSize(3);
  tft.println("OT.TECH");

  } 

  return micros() - start;
}

void start(){
  tft.setCursor(0, 130);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setFont(&FreeSerifItalic9pt7b);
  tft.println("Starting Now!");
}

void setup()
{

  Serial.println("tft begin");
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(1);
  start();
  tft.fillScreen(ILI9341_BLACK);

  music_setup();
  // Open serial communications and wait for port to open
  Serial.begin(115200);
  while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Starting .");

  /* Connect LTE network */
  doAttach();

  int result;

  // Wait for the modem to connect to the LTE network.
  Serial.println("Waiting for successful attach.");
  LTEModemStatus modemStatus = lteAccess.getStatus();

  while(LTE_READY != modemStatus) {
    if (LTE_ERROR == modemStatus) {

      /* If the following logs occur frequently, one of the following might be a cause:
       * - Reject from LTE network
       */
      Serial.println("An error has occurred. Shutdown and retry the network attach process after 1 second.");
      lteAccess.shutdown();
      sleep(1);
      doAttach();
    }
    sleep(1);
    modemStatus = lteAccess.getStatus();
  }

  Serial.println("attach succeeded.");

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    // do nothing forevermore:
    for (;;)
      sleep(1);
  }
  mqttClient.setId(MQTT_ID);

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  Serial.print("Subscribing to topic: ");
  Serial.println(topic);
  Serial.println();
  
  mqttClient.subscribe(topic);
}

void loop()
{

  int messageSize = mqttClient.parseMessage();

  
  if (messageSize) {
    // we received a message, print out the topic and contents
    Serial.print("Received a message with topic '");
    Serial.print(mqttClient.messageTopic());
    Serial.print("', length ");
    Serial.print(messageSize);
    Serial.println(" bytes:");

    // use the Stream interface to print the contents
    while (mqttClient.available()) {
      Serial.print(mqttClient.read());
    }

 switch (messageSize) {
        case 1:
          printf("鹿\n");
          theAudio->setBeep(1,-30,262);
          usleep(100000);
          theAudio->setBeep(0,0,0);
          usleep(10000);
          tft.setRotation(1);
          testText1(0);
          break;
        case 2:
          printf("狐\n");
          theAudio->setBeep(1,-30,523);
          usleep(100000);
          theAudio->setBeep(0,0,0);
          usleep(10000);
          tft.setRotation(1);
          testText1(1);
          break;
        case 3:
          printf("他\n");
          tft.setRotation(1);
          testText1(2);
          break;
        default:
          printf("その他\n");
          break;
      }

    Serial.println();

    Serial.println();
  }
  // delay(15000);

  // usleep(10*1000);

}

