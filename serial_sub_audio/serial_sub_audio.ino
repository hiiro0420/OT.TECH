#include <SDHCI.h>
#include <Audio.h>

#define PLAYBACK_FILE_NAME "Sound.wav"
#define PLAYBACK_FILE_NAME_ "Sound2.wav"

SDClass theSD;
AudioClass *theAudio;

File myFile;

WavContainerFormatParser theParser;

const int32_t sc_buffer_size = 6144;
uint8_t s_buffer[sc_buffer_size];

uint32_t s_remain_size = 0;
bool ErrEnd = false;

char data[10];
int j = 0;

static void audio_attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");
  
  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING)
    {
      ErrEnd = true;
    }
}

static const uint32_t sc_prestore_frames = 5;

void setup() {
  // put your setup code here, to run once:
  Serial2.begin(115200);
  Serial.begin(115200);
  digitalWrite(LED0, LOW);
  digitalWrite(LED1, LOW);
    /* Initialize SD */
  while (!theSD.begin())
    {
      /* wait until SD card is mounted. */
      Serial.println("Insert SD card.");
    }

  // Get wav file info

  fmt_chunk_t fmt;

  handel_wav_parser_t *handle
    = (handel_wav_parser_t *)theParser.parseChunk("/mnt/sd0/" PLAYBACK_FILE_NAME, &fmt);
  if (handle == NULL)
    {
      printf("Wav parser error.\n");
      exit(1);
    }

  // Get data chunk info from wav format
  uint32_t data_offset = handle->data_offset;
  s_remain_size = handle->data_size;

  theParser.resetParser((handel_wav_parser *)handle);

  // start audio system
  theAudio = AudioClass::getInstance();

  theAudio->begin(audio_attention_cb);

  puts("initialization Audio Library");

  /* Set clock mode to normal */

  theAudio->setRenderingClockMode((fmt.rate <= 48000) ? AS_CLKMODE_NORMAL : AS_CLKMODE_HIRES);

  /* Set output device to speaker with first argument.
   * If you want to change the output device to I2S,
   * specify "AS_SETPLAYER_OUTPUTDEVICE_I2SOUTPUT" as an argument.
   * Set speaker driver mode to LineOut with second argument.
   * If you want to change the speaker driver mode to other,
   * specify "AS_SP_DRV_MODE_1DRIVER" or "AS_SP_DRV_MODE_2DRIVER" or "AS_SP_DRV_MODE_4DRIVER"
   * as an argument.
   */
  printf("set\n");
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_LINEOUT);

  /*
   * Set main player to decode wav. Initialize parameters are taken from wav header.
   * Search for WAV decoder in "/mnt/sd0/BIN" directory
   */

  printf("init\n");
  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_WAV, "/mnt/sd0/BIN", fmt.rate, fmt.bit, fmt.channel);
  err = theAudio->initPlayer(AudioClass::Player1, AS_CODECTYPE_WAV, "/mnt/sd0/BIN", fmt.rate, fmt.bit, fmt.channel);

  /* Verify player initialize */
  if (err != AUDIOLIB_ECODE_OK)
    {
      printf("Player0 initialize error\n");
      exit(1);
    }

  /* Open file placed on SD card */
  myFile = theSD.open(PLAYBACK_FILE_NAME);
  /* Verify file open */
  if (!myFile)
    {
      printf("File open error\n");
      exit(1);
    }
  printf("Open! %s\n", myFile.name());
  /* Set file position to beginning of data */
  myFile.seek(data_offset);
  for (uint32_t i = 0; i < sc_prestore_frames; i++)
    {
      size_t supply_size = myFile.read(s_buffer, sizeof(s_buffer));
      s_remain_size -= supply_size;
      
      err = theAudio->writeFrames(AudioClass::Player0, s_buffer, supply_size);
      if (err != AUDIOLIB_ECODE_OK)
        {
          break;
        }
        
      if (s_remain_size == 0)
        {
          break;
        }
    }

  myFile = theSD.open(PLAYBACK_FILE_NAME_);
  /* Verify file open */
  if (!myFile)
    {
      printf("File open error\n");
      exit(1);
    }
  printf("Open! %s\n", myFile.name());
  /* Set file position to beginning of data */
  myFile.seek(data_offset);
  for (uint32_t i = 0; i < sc_prestore_frames; i++)
    {
      size_t supply_size = myFile.read(s_buffer, sizeof(s_buffer));
      s_remain_size -= supply_size;
      
      err = theAudio->writeFrames(AudioClass::Player1, s_buffer, supply_size);
      if (err != AUDIOLIB_ECODE_OK)
        {
          break;
        }
        
      if (s_remain_size == 0)
        {
          break;
        }
    }



}

String serialRead(){
  if (Serial2.available()){
    data[j] = Serial2.read();

    if(j > 10 || data[j] == '\0'){
      // data[i] = '\0';
      j = 0;
      }
      
    else{
      j++;
    }
  }
  else{
  Serial.println("Serial is not available!!!");
  digitalWrite(LED0, LOW);
  digitalWrite(LED1, LOW);
  }
  return String(data);
}

void stop(int k){
  puts("stop");
  // theAudio->stopPlayer(AudioClass::Player0,AS_STOPPLAYER_NORMAL);
  if(k%100==0){
  theAudio->stopPlayer(AudioClass::Player0);
  }
  else{
  theAudio->stopPlayer(AudioClass::Player1);
  }

  myFile.close();
}

static const uint32_t sc_store_frames = 10;

void restart(int k){

  puts("restart");
  if(k%100==0){
    puts("0");
    PLAYBACK_FILE_NAME "Sound.wav";
  }
  else{
    puts("1");
    PLAYBACK_FILE_NAME "Sound2.wav";
  }

  fmt_chunk_t fmt;
  handel_wav_parser_t *handle
    = (handel_wav_parser_t *)theParser.parseChunk("/mnt/sd0/" PLAYBACK_FILE_NAME, &fmt);
  if (handle == NULL)
    {
      printf("Wav parser error.\n");
      exit(1);}
    
  uint32_t data_offset = handle->data_offset;
  s_remain_size = handle->data_size;
  theParser.resetParser((handel_wav_parser *)handle);
  
  if(k%100==0){
    myFile = theSD.open(PLAYBACK_FILE_NAME);
  }
  else{
    myFile = theSD.open(PLAYBACK_FILE_NAME_);
  }

  // myFile = theSD.open(PLAYBACK_FILE_NAME);
  myFile.seek(data_offset);
  err_t err;
  for (uint32_t i = 0; i < sc_prestore_frames; i++)
  {
    size_t supply_size = myFile.read(s_buffer, sizeof(s_buffer));

    s_remain_size -= supply_size;
    if(k%100==0){
    err = theAudio->writeFrames(AudioClass::Player0, s_buffer, supply_size);}
    else{
    err = theAudio->writeFrames(AudioClass::Player1, s_buffer, supply_size);}
    // err_t err = theAudio->writeFrames(AudioClass::Player0, s_buffer, supply_size);
    if (err != AUDIOLIB_ECODE_OK)
      {

        break;
      }
    if (s_remain_size == 0)
      {
        break;
      }
  }

}

void saisei(int k){

  puts("saisei");
  static bool is_carry_over = false;
  static size_t supply_size = 0;
  theAudio->setVolume(-50);
  if(k%100==0){
  theAudio->startPlayer(AudioClass::Player0);
  puts("Play0!");
  }
  else{
  theAudio->startPlayer(AudioClass::Player1);
  puts("Play1!");
  }


  /* Send new frames to decode in a loop until file ends */
  int err;
  for (uint32_t i = 0; i < sc_store_frames; i++)
    {
      if (!is_carry_over)
        {
          supply_size = myFile.read(s_buffer, (s_remain_size < sizeof(s_buffer)) ? s_remain_size : sizeof(s_buffer));
          s_remain_size -= supply_size;
        }
      is_carry_over = false;
      if(k%100==0){
        err = theAudio->writeFrames(AudioClass::Player0, s_buffer, supply_size);
      }
      else{
        err = theAudio->writeFrames(AudioClass::Player1, s_buffer, supply_size);
      }
      // int err = theAudio->writeFrames(AudioClass::Player0, s_buffer, supply_size);

      if (err == AUDIOLIB_ECODE_SIMPLEFIFO_ERROR)
        {
          is_carry_over = true;
          break;
        }

      if (s_remain_size == 0)
        {
          printf("stop\n");
          stop(k);
        }
    }

  if (ErrEnd)
    {
      printf("Error End\n");
      exit(1);
    }

  usleep(1000);
}

int k = 0;

void loop() {
  // put your main code here, to run repeatedly:
  String cmd = serialRead();
  Serial.println(cmd);
  if(cmd.indexOf("D")>=0){
    printf("in OK\n");
    k = 100;
    restart(k);
    saisei(k);
    stop(k);
  }
  else if(cmd.indexOf("F")>=0){
    k = 50;
    printf("in NO\n");
    restart(k);
    saisei(k);
    stop(k);
  }
    if(j > 2){
    j = 0;
  }
  delay(1000);

}
