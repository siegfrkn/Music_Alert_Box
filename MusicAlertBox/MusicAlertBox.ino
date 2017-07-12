//WiFi Music Alert Box
// Uses Feather Huzzah ESP8266 WiFi board and Music Maker FeatherWing
// along with Adafruit IO and IFTTT to play triggered sound files
// written by John Park
// based on Todd Trece's Digital Input sample code
//MIT license
/************************** Configuration ***********************************/

// edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config.h"

/************************ libraries *****************************************/
// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>

// include IR libraries for remote
#include <IRrecv.h>
#include <IRutils.h>

/************************ Music Maker definitions ***************************/
// These are the pins used

int RECV_PIN = 4; //an IR detector/demodulatord is connected to GPIO pin 2
IRrecv irrecv(RECV_PIN);
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)

// Feather ESP8266
  #define VS1053_CS      16     // VS1053 chip select pin (output)
  #define VS1053_DCS     15     // VS1053 Data/command select pin (output)
  #define CARDCS          2     // Card chip select pin
  #define VS1053_DREQ     0     // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = 
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);



/************************ Define Feeds**************************************/
// set up the 'song' feed
AdafruitIO_Feed *song = io.feed("music-player-01-song");
//set up the 'volume' feed
AdafruitIO_Feed *volume = io.feed("music-player-01-volume");
//set up the 'pause' feed
AdafruitIO_Feed *pause = io.feed("music-player-01-pause");

/************************ Define IR REMOTE **********************************/ 
// the name of what we're going to play
char foundname[20];
boolean isPaused = false;
uint8_t vol = 10;
int lastRemoteVal = 0;

/************************ setup ********************************************/
void setup() {

  if (! musicPlayer.begin()) { // initialise the music player
     while (1);
  }

  musicPlayer.sineTest(0x44, 500);// tone to indicate VS1053 is working
  
  if (!SD.begin(CARDCS)) {
    while (1);  // don't do anything more
  }
  
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(20,20);

  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int


  // set led pin as a digital output
  //pinMode(LED_BUILTIN, OUTPUT);

  // start the serial connection
  Serial.begin(115200);

  // wait for serial monitor to open
  //while(! Serial);

  // connect to io.adafruit.com
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  // set up a message handler for each of the feeds.
  // e.g., the handleSongMessage function (defined below)
  // will be called whenever a "song message" is
  // received from adafruit io
  song->onMessage(handleSongMessage);
  volume->onMessage(handleVolumeMessage);
  pause->onMessage(handlePauseMessage);

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    Serial.println(io.statusText());
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());

 //Setup IR Remote
 
 // If DREQ is on an interrupt pin we can do background
 // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
 
  irrecv.enableIRIn(); // Start the receiver
  Serial.println("Remote Receiver Ready");

}


/************************ loop *********************************************/
void loop() {
  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data
  io.run();

  // THIS IS CODE FOR IR REMOTE
  decode_results results;
  //Serial.println("Decoding");

  if(digitalRead(VS1053_DREQ) && !musicPlayer.stopped() && !isPaused) {
    musicPlayer.feedBuffer(); 
  }
  // look for a message!
  if (irrecv.decode(&results)) {
    //Serial.println(results.value, HEX);
    irrecv.resume(); // Receive the next value
 
    // handle repeat codes!
    if (results.value == 0xFFFFFFFF) {
      // only for vol+ or vol-
      if ( (lastRemoteVal == 0xFD40BF) || (lastRemoteVal == 0xFD00FF))
         results.value = lastRemoteVal;
    } 
    else {
      lastRemoteVal = results.value;
    }
    
    if (results.value == 0xFD08F7) {
        musicPlayer.stopPlaying();
        Serial.println("playing track #1");
        musicPlayer.startPlayingFile("Panda.mp3");
        yield();
    }
    if (results.value == 0xFD8877) {
       musicPlayer.stopPlaying();
       Serial.println("playing track #2");
       musicPlayer.startPlayingFile("TrashDay.mp3");
    }
    if (results.value == 0xFD48B7) {
       musicPlayer.stopPlaying();
       Serial.println("playing track #3");
       musicPlayer.startPlayingFile("BicycleRace.mp3");
    }
 
    if (results.value == 0xFD40BF) { //vol+
      Serial.println("Vol+");
      if (vol > 0) {
         vol;
         musicPlayer.setVolume(vol,vol);
      }
    }
    if (results.value == 0xFD00FF) { //vol-
      Serial.println("Vol-");
      if (vol < 100) {
         vol++;
         musicPlayer.setVolume(vol,vol);
      }
    }
 
    if (results.value == 0xFD807F) { // playpause
      Serial.println("Play/Pause");
      Serial.print(isPaused);
      if (isPaused == false) {
        musicPlayer.pausePlaying(true);
        isPaused = true; // toggle!
      } 
      if (isPaused ==true) {
        musicPlayer.pausePlaying(false);
        isPaused = true; // toggle!
      }
      isPaused = !isPaused; // toggle!
  }
  }
  yield();
  delay(1);
}

/************************ handleSongMessage ********************************/
// this function is called whenever a 'song' feed message
// is received from Adafruit IO. it was attached to
// the 'song' feed in the setup() function above.
void handleSongMessage(AdafruitIO_Data *data) {

  Serial.print("song received <- ");

  //change the songs named here for the one's you've uploaded to your SD card
  if(data->toInt()==1){
    musicPlayer.startPlayingFile("Panda.mp3"); 
  }
  else if(data->toInt()==2){
    musicPlayer.startPlayingFile("TrashDay.mp3"); 
  }
  else if(data->toInt()==3){
    musicPlayer.startPlayingFile("BicycleRace.mp3"); 
  }
  else if(data->toInt()==4){
    musicPlayer.startPlayingFile("BackHome.mp3"); 
  }
  else if(data->toInt()==5){
    musicPlayer.startPlayingFile("happy.mp3"); 
  }
  else if(data->toInt()==6){
    musicPlayer.startPlayingFile("kittin.mp3"); 
  }
  else if(data->toInt()==7){
    musicPlayer.startPlayingFile("new1.mp3"); 
  }
  else if(data->toInt()==8){
    musicPlayer.startPlayingFile("new2.mp3"); 
  }
  else if(data->toInt()==9){
    musicPlayer.startPlayingFile("pop_1.mp3"); 
  }
  else if(data->toInt()==10){
    musicPlayer.startPlayingFile("wawawa.mp3"); 
  }
  else if(data->toInt()==11){
    musicPlayer.startPlayingFile("zoom1.mp3"); 
  }
  else if(data->toInt()==12){
    musicPlayer.startPlayingFile("metroply.mp3"); 
  }
  else if(data->toInt()==13){
    musicPlayer.startPlayingFile("pascal.mp3"); 
  }
  else if(data->toInt()==14){
    musicPlayer.startPlayingFile("solder.mp3"); 
  }
 
  Serial.print(data->feedName());
  Serial.print(" ");
  Serial.println(data->value());
}

/************************ handleVolume ************************************/
// this function is called whenever a 'volume' feed message
// is received from Adafruit IO. it was attached to
// the 'volume' feed in the setup() function above.
void handleVolumeMessage(AdafruitIO_Data *data) {

  Serial.print("volume received <- ");
    // since we are using the same function to handle
  // messages for two feeds, we can use feedName() in
  // order to find out which feed the message came from.
  Serial.print(data->feedName());
  Serial.print(" ");
  Serial.println(data->value());

  musicPlayer.setVolume(data->toInt(), data->toInt());
  
}

/************************ handlePauseMessage *******************************/
// this function is called whenever a 'pause' feed message
// is received from Adafruit IO. it was attached to
// the 'pause' feed in the setup() function above.
void handlePauseMessage(AdafruitIO_Data *data) {

  Serial.print("pause received <- ");

  // write the current state to the led
  if(data->toInt()==1){
    musicPlayer.pausePlaying(false);
 
  }
  else if(data->toInt()==0){
   musicPlayer.pausePlaying(true);
 
  }
  Serial.print(data->feedName());
  Serial.print(" ");
  Serial.println(data->value());

}

/************************ IR REMOTE *******************************/
 
 
//boolean findFileStartingWith(char *start) {
//  File root;
//  root = SD.open("/");
//  root.rewindDirectory();
//  while (true) {
//    File entry =  root.openNextFile();
//    if (! entry) {
//      return false;
//    }
//    String filename = entry.name();
//    Serial.print(filename);
//    if (entry.isDirectory()) {
//      Serial.println("/");
//    } else {
//      Serial.println();
//      if (filename.startsWith(start)) {
//        filename.toCharArray(foundname, 20); 
//        entry.close();
//        root.close();
//        return true;
//      }
//    }
//    entry.close();
//  }
//}
// 
 
/// File listing helper
void printDirectory(File dir, int numTabs) {
   while(true) {
     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}
