#include <LTE.h>
#include <ArduinoMqttClient.h>
#include <Camera.h>
#include <DNNRT.h>
#include <SDHCI.h>
#include <stdio.h>
#include <SPI.h>
// camera setting
#define BAUDRATE  (115200)
#define TOTAL_PICTURE_COUNT     (50)
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
#define BROKER_NAME "broker.emqx.io"   // replace with your broker
#define BROKER_PORT 1883                   // port 8883 is the default for MQTT over TLS.
                                           // for this client, if required by the server.
// MQTT topic
#define MQTT_TOPIC "spresense/mqtt"        // replace with your topic
// MQTT publish interval settings
#define PUBLISH_INTERVAL_SEC   1           // MQTT publish interval in sec
#define MAX_NUMBER_OF_PUBLISH  30          // Maximum number of publish
LTE lteAccess;
LTEClient client;
MqttClient mqttClient(client);
// set ID
#define MQTT_ID "spresense_pub"
// LTE
int numOfPubs = 0;
unsigned long lastPubSec = 0;
char broker[] = BROKER_NAME;
int port = BROKER_PORT;
char topic[]  = MQTT_TOPIC;
// recognize
SDClass  SD;
DNNRT dnnrt;
const int DNN_width = 80;
const int DNN_height = 60;
boolean save_flag = false;
int take_picture_count = 0;
DNNVariable input(DNN_width * DNN_height);
void printError(enum CamErr err)
{
  Serial.print("Error: ");
  switch (err)
    {
      case CAM_ERR_NO_MEMORY:
        Serial.println("No memory");
        break;
      default:
        break;
    }
}
void CamCB(CamImage img)
{
  if (!img.isAvailable()) return;
   // カメラ画像の切り抜きと縮小
    CamImage small_image;
    CamErr err = img.clipAndResizeImageByHW(small_image, 0, 0, 319,239 , DNN_width, DNN_height);
  // 縮小に失敗したらリターン
  if (!small_image.isAvailable())  return;
   // 認識用モノクロ画像を設定
    uint16_t* Imgbuf = (uint16_t*)small_image.getImgBuff();
    float *dnnbuf = input.data();
    for (int n = 0; n < DNN_height*DNN_width; ++n)
    {
      dnnbuf[n] =  (float)(((Imgbuf[n] & 0xf000) >> 8)
                        |  ((Imgbuf[n] & 0x00f0) >> 4))/255.0;
    }
  // 推論結果
    dnnrt.inputVariable(input, 0);
    dnnrt.forward();
    DNNVariable output = dnnrt.outputVariable(0);
    int index = output.maxIndex();
    String animal_number;
    Serial.print("Image is ");
    Serial.print(index);
    Serial.println();
    Serial.print("value ");
    Serial.print(output[index]);
    Serial.println();
    if (index==0){
      Serial.println("鹿");
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      animal_number = "0";

    }else{
      Serial.println("狐");
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, HIGH);
      digitalWrite(LED3, LOW);
      animal_number = "00";
    }
    // Publish to broker
    Serial.print("Sending message to topic: ");
    Serial.println(topic);
    Serial.print("Publish: ");
    Serial.println(animal_number);
    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(topic);
    mqttClient.print(animal_number);
    mqttClient.endMessage();
    delay(8000);
}
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
void setup()
{
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
  // set ID
  mqttClient.setId(MQTT_ID);
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    // do nothing forevermore:
    for (;;)
      sleep(1);
  }
  Serial.println("You're connected to the MQTT broker!");
  CamErr err;
  Serial.begin(BAUDRATE);
  while (!Serial)
  {
    ;
  }
  // SDカードにある学習済モデルの読み込み
  File nnbfile = SD.open("model_80_60.nnb");
  if (!nnbfile)
  {
    Serial.print("nnb not found");
    return;
  }
  // 学習済モデルでDNNRTを開始
  int ret = dnnrt.begin(nnbfile);
  if (ret < 0)
  {
    Serial.print("Runtime initialization failure. ");
    Serial.print(ret);
    Serial.println();
    return;
  }
  Serial.println("Prepare camera");
  err = theCamera.begin();
  if (err != CAM_ERR_SUCCESS)
    {
      printError(err);
    }
  Serial.println("Start streaming");
  theCamera.startStreaming(true, CamCB);
  Serial.println("Set Auto white balance parameter");
  theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_DAYLIGHT);
  Serial.println("Set still picture format");
  err =  theCamera.setStillPictureImageFormat(
     CAM_VIDEO_FPS_30,
     CAM_IMGSIZE_QVGA_H,
     CAM_IMGSIZE_QVGA_V,
     CAM_IMAGE_PIX_FMT_JPG);
  if (err != CAM_ERR_SUCCESS)
    {
      printError(err);
    }
       pinMode(LED0, OUTPUT); // LED準備
}
void loop()
{
  sleep(10);
  if (take_picture_count < TOTAL_PICTURE_COUNT)
  {
    Serial.println("call takePicture()");
    digitalWrite(LED3, HIGH);
    CamImage img = theCamera.takePicture();
    if(true)
    {
      take_picture_count++;
    }
    save_flag = false;
  }
  else
  {
    Serial.println("END.");
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    dnnrt.end();
    theCamera.end();
  }
  // String testString = "test" + String(numOfPubs) + "!";
  // unsigned long currentTime = lteAccess.getTime();
  // if (currentTime >= lastPubSec + PUBLISH_INTERVAL_SEC) {
  //   // Publish to broker
  //   Serial.print("Sending message to topic: ");
  //   Serial.println(topic);
  //   Serial.print("Publish: ");
  //   Serial.println(testString);
  //   // send message, the Print interface can be used to set the message contents
  //   mqttClient.beginMessage(topic);
  //   mqttClient.print(testString);
  //   mqttClient.endMessage();
  //   lastPubSec = currentTime;
  //   numOfPubs++;
  // }
  // if (numOfPubs >= MAX_NUMBER_OF_PUBLISH) {
  //   Serial.println("Publish end");
  //   // do nothing forevermore:
  //   for (;;)
  //     sleep(1);
  // }
}
