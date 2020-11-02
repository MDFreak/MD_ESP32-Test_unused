#include "main.h"

// --- declarations
  // ------ system -----------------------
    #define CHECK_ERRORS
    #ifdef CHECK_ERRORS
      uint16_t     md_error  = 0    // Error-Status bitkodiert -> 0: alles ok
                               #ifdef USE_WIFI
                                 + ERRBIT_WIFI
                                 #ifdef USE_NTP_SERVER
                                   + ERRBIT_NTPTIME
                                 #endif
                               #endif
                               #ifdef USE_WEBSERVER
                                 + ERRBIT_SERVER
                               #endif
                               #ifdef USE_TOUCHSCREEN
                                 + ERRBIT_TOUCH
                               #endif
                               ;
    #endif

    msTimer       dispT  = msTimer(DISP_CYCLE);
    uint32_t      ze     = 1;      // aktuelle Schreibzeile
    char          outBuf[STAT_LINELEN + 1] = "";
    #ifdef USE_STATUS
      msTimer     statT  = msTimer(STAT_TIMEDEF);
      char        statOut[STAT_LINELEN + 1] = "";
      bool        statOn = false;
      //char        timeOut[STAT_LINELEN + 1] = "";
      #endif
  // ------ network ----------------------
    #ifdef USE_WIFI
      msTimer wifiT = msTimer(WIFI_CONN_CYCLE);
      #endif

    #ifdef USE_WEBSERVER
      msTimer servT = msTimer(WEBSERVER_CYCLE);
      #endif // USE_WEBSERVER

    #ifdef USE_NTP_SERVER
      msTimer ntpT    = msTimer(NTPSERVER_CYCLE);
      time_t  ntpTime = 0;
      bool    ntpGet  = true;
      #endif // USE_WEBSERVER

  // ------ user interface ---------------
    #ifdef USE_TOUCHSCREEN
        md_touch touch = md_touch();
      #endif

    #ifdef USE_KEYPADSHIELD
        md_kpad kpad(KEYS_ADC);
        uint8_t key;
      #endif // USE_KEYPADSHIELD

    #ifdef USE_BUZZER
        md_buzzer     buzz       = md_buzzer();
      #endif // USE_BUZZER

    #ifdef USE_OLED
        md_oled oled = md_oled();
        //      msTimer     oledT      = msTimer(DISP_CYCLE);
        uint8_t oledIdx = 0;
      #endif //USE_OLED

    #if (defined(USE_TFT1602_GPIO_RO_3V3) || defined(USE_TFT1602_GPIO_RO_3V3))
        LiquidCrystal  lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
        void*          plcd = (void*) &lcd;
        md_lcd         mlcd(plcd);
      #endif
// --- private prototypes
  // ------ user interface -----------------
    // --- user output
      // standard outputs
        void dispStatus(const char* msg)
          {
            #ifdef USE_STATUS
              size_t statLen = strlen(msg);
              bool   doIt    = false;

              memset(statOut, ' ', STAT_LINELEN);
              if (statLen)
                {
                  if ( statLen > STAT_LINELEN)
                    statLen = STAT_LINELEN;
                  memcpy(statOut, msg, statLen);
                  statOn = true;
                  statT.startT();
                  doIt = true;    // output statOut
                  statT.startT();
                }
              else // empty input
                if (statOn && statT.TOut())
                  statOn = false;

              if (!statOn) // disp actual time
                {
                  sprintf(statOut,"%02d.%02d. %02d:%02d:%02d ", day(), month(), hour(), minute(), second());
                  doIt = true;
                }
              if (doIt)
                {
                  #if defined(USE_TOUCHXPT2046_AZ_3V3)
                    touch.wrStatus(msg);
                    #endif
                  #if defined(USE_OLED)
                    oled.wrStatus(msg);
                          #if (DEBUG_MODE >= CFG_DEBUG_DETAILS)
                            SOUT("  md_error="); SOUTLN(md_error);
                          #endif
                    #endif
                  #if defined(USE_TFT)
                    mlcd.wrStatus((char*) statOut);
                          #if (DEBUG_MODE >= CFG_DEBUG_DETAILS)
                            SOUT("  md_error="); SOUTLN(md_error);
                          #endif
                    #endif // USE_DISP
                }
              #endif // USE_STATUS
          }

        void dispText(char* msg, uint8_t row, uint8_t col)
          {
            #ifdef USE_DISP
                #if (defined(USE_TOUCHXPT2046_AZ_3V3))
                  touch.wrTouch(msg, col, row);
                  #endif
                #if defined(USE_OLED)
                  oled.wrText(msg, col, row);
                        #if (DEBUG_MODE >= CFG_DEBUG_DETAIL)
                          SOUT("  md_error="); SOUTLN(md_error);
                        #endif
                  #endif
                #if defined(USE_TFT)
                  mlcd.wrText(msg, row, col);
                  #endif
              #endif
          }

        void dispText(char* msg)
          {
            dispText(msg, 0, 0);
          }

      // --- start display
        void startDisp()
          {
            #ifdef USE_DISP
              #ifdef USE_STATUS
                statOut[STAT_LINELEN] = 0;  // limit strlen
                #endif
              #if defined(USE_TFT)
                mlcd.start(plcd);
                #endif
              #if defined(USE_TOUCHXPT2046_AZ_3V3)
                bool ret = touch.startTouch();
                      #if (DEBUG_MODE >= CFG_DEBUG_DETAIL)
                        SOUT("startTouch ret="); SOUT(ret);
                      #endif
                md_error = setBit(md_error, ERRBIT_TOUCH, ret);
                      #if (DEBUG_MODE >= CFG_DEBUG_DETAIL)
                        SOUT("  md_error="); SOUTLN(md_error);
                      #endif
                #endif
              #if defined (USE_OLED)
                  oled.begin();
                #endif
              #endif
          }
    // --- user input
      // --- keypad
        void startKeys()
          {
            #if defined(USE_KEYPADSHIELD)
                kpad.init(KEYS_ADC);
              #endif // USE_KEYPADSHIELD
          }

        uint8_t getKey()
          {
            #if defined(USE_KEYPADSHIELD)
                return kpad.getKey();
              #endif // USE_KEYPADSHIELD
          }
  // ------ NTP server -------------------
    #ifdef USE_NTP_SERVER
      void initNTPTime()
        {
          bool ret = wifiMD.md_initNTPTime();
                #if (DEBUG_MODE >= CFG_DEBUG_DETAIL)
                  Serial.print("initNTPTime ret="); Serial.print(ret);
                #endif
          md_error = setBit(md_error, ERRBIT_NTPTIME, ret);
                #if (DEBUG_MODE >= CFG_DEBUG_DETAIL)
                  Serial.print("  md_error="); Serial.println(md_error);
                #endif
          if ((md_error & ERRBIT_WIFI) == 0)
            {
              dispStatus("NTPTime ok");
            }
            else
            {
              dispStatus("NTPTime error");
            }
        }
      #endif // USE_NTP_SERVER

  // ------ WIFI -------------------------
    #ifdef USE_WIFI
      void startWIFI()
        {
          #ifdef USE_TOUCHSCREEN
            while (touch.wrStatus("  start WIFI"));
          #endif
          wifiMD.setLocIP();
          bool ret = wifiMD.md_initWIFI();
                  #if (DEBUG_MODE >= CFG_DEBUG_DETAIL)
                    SOUT(millis()); SOUT(" initWIFI ret="); SOUTLN(ret);
                  #endif
          if (ret == MDOK)
          {
            ret = wifiMD.md_scanWIFI();
                  #if (DEBUG_MODE >= CFG_DEBUG_DETAIL)
                    SOUT(millis()); SOUT(" scanWIFI ret="); SOUTLN(ret);
                  #endif
          }
          if (ret == MDOK)
          {
            ret = wifiMD.md_startWIFI();
                  #if (DEBUG_MODE >= CFG_DEBUG_DETAIL)
                    SOUT("startWIFI ret="); SOUT(ret);
                    #endif
          }
          md_error = setBit(md_error, ERRBIT_WIFI, ret);
                #if (DEBUG_MODE >= CFG_DEBUG_DETAIL)
                  SOUT("  md_error="); SOUTLN(md_error);
                  #endif

          if ((md_error & ERRBIT_WIFI) == 0)
              dispStatus("WIFI connected");
            else
              dispStatus("WIFI error");
          #ifdef USE_NTP_SERVER
            if((md_error & ERRBIT_WIFI) == 0) // WiFi ok
                if((md_error & ERRBIT_NTPTIME) != 0) // WiFi ok
                  initNTPTime();
            #endif
        }
      #endif // USE_WIFI

    #ifdef USE_WEBSERVER
      void startWebServer()
        {
          bool ret = MDERR;
          if ((md_error & ERRBIT_SERVER) != 0)
            {
              dispStatus("start webserver");
              if ((md_error & ERRBIT_WIFI) == 0)
                {
                  ret = webMD.md_startServer();
                      #if (DEBUG_MODE >= CFG_DEBUG_DETAIL)
                        // SOUT("startServer ret="); SOUT(ret);
                      #endif
                }
              md_error = setBit(md_error, ERRBIT_SERVER, ret);
                    #if (DEBUG_MODE >= CFG_DEBUG_DETAIL)
                      // SOUT("  md_error="); SOUTLN(md_error);
                    #endif

              if ((md_error & ERRBIT_SERVER) == 0)
                {
                  dispStatus("Webserver online");
                }
                else
                {
                  dispStatus("Webserver error");
                }
            }
        }
      #endif // USE_WEBSERVER


  // ------ passive buzzer --------------
    #ifdef PLAY_MUSIC
      tone_t test = {0,0,0};
      void playSong(int8_t songIdx)
        {
          if (buzz.setSong(SONG0_LEN,(void*) SONG0_NOTES) == MDOK)
            {
              #ifndef USE_SONGTASK
                buzz.playSong();
              #endif
            }
        }
      void playSong() { playSong(0); }

    #endif


// --- system startup
  void setup()
    {
      // --- system
        // start system
          Serial.begin(SER_BAUDRATE);
              #if (DEBUG_MODE >= CFG_DEBUG_STARTUP)
                SOUTLN(); SOUTLN("setup start ...");
              #endif
          #ifdef SCAN_IIC
            scanIIC(1, I2C1_SDA, I2C1_SCL);
            sleep(5);
            #ifdef SDA2
              scanIIC(2, I2C2_SDA, I2C2_SCL);
              sleep(5);
              #endif
            #endif
      // --- user interface
        // start display - output to user
          startDisp();
          dispStatus("setup start ...");
        // start input device
          startKeys();
        // start buzzer (task)
          #ifdef USE_BUZZER
            pinMode(PIN_BUZZ, OUTPUT);                                                                               // Setting pin 11 as output
            #ifdef PLAY_MUSIC
              buzz.initMusic();
              #ifdef PLAY_START_MUSIC
                playSong();
              #endif
            #endif
          #endif

      // --- network
        // start WIFI
          #ifdef USE_WIFI
            startWIFI();
            if ((md_error & ERRBIT_WIFI) == 0)
              {
                dispStatus("WIFI connected");
              }
              else
              {
                dispStatus("WIFI error");
              }
            #endif // USE_WIFI
      // --- finish setup
          #if (DEBUG_MODE >= CFG_DEBUG_STARTUP)
              SOUTLN();
              SOUT("... end setup -- error="); Serial.println(md_error);
              SOUTLN();
            #endif
    }

// --- system run = endless loop
  void loop()
    {
      //uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
      #ifdef USE_WIFI  // restart WIFI if offline
          if(wifiT.TOut())
          {
            //Serial.print("WiFi md_error = "); Serial.println(md_error);
            wifiT.startT();
            if((md_error & ERRBIT_WIFI) > 0)
              {
                SOUTLN("WiFi startWIFI");
                dispStatus("WiFi startWIFI");
                startWIFI();
              }
          }
        #endif // USE_WIFI

      // ----------------------
      #ifdef USE_NTP_SERVER
        if (ntpT.TOut() == true)
        {
          setTime(++ntpTime);
          if ((md_error & ERRBIT_WIFI) == 0)
            { // WiFi online
              if (((md_error & ERRBIT_NTPTIME) > 0) || (year() < 2000))   // time not initialized
                {
                  initNTPTime();
                  ntpGet = true;
                }
              if (ntpGet == true)
                {
                  ntpGet = wifiMD.md_getTime(&ntpTime);
                  setTime(ntpTime);
                }
            }
          ntpT.startT();
                #if (DEBUG_MODE == CFG_DEBUG_DETAILS)
                  SOUT("Datum "); SOUT(day()); SOUT("."); SOUT(month()); SOUT("."); SOUT(year()); SOUT(" ");
                  SOUT("Zeit "); SOUT(hour()); SOUT("."); SOUT(minute()); SOUT(":"); SOUTLN(second());
                #endif
        }
        #endif // USE_NTP_SERVER


      // ----------------------
      #ifdef USE_WEBSERVER
        if (servT.TOut()) // run webserver - restart on error
          {
            servT.startT();
            if ((md_error & ERRBIT_SERVER) != 0)
              startWebServer();
            else
              //bool ret = webMD.md_handleClient();
              md_error = setBit(md_error, ERRBIT_SERVER, webMD.md_handleClient());
          }
        #endif

      // ----------------------
      #ifdef USE_TOUCHSCREEN
        touch.runTouch(outBuf);
        #endif // USE_TOUCHSCREEN

      // ----------------------
      #ifdef USE_KEYPADSHIELD
        key = getKey();
        if (key)
          {
            sprintf(outBuf,"key %d", key);
            dispStatus(outBuf);
          }
        #endif
      // ----------------------
      #if defined(USE_DISP)
        if (dispT.TOut())    // handle touch output
          {
            dispT.startT();
              #ifdef RUN_OLED_TEST
                oled.clearBuffer();
                switch (oledIdx)
                  {
                    case 0:
                      oled.prepare();
                      oled.box_frame();
                      break;
                    case 1:
                      oled.disc_circle();
                      oled.sendBuffer();
                      break;
                    case 2:
                      oled.r_frame_box();
                      break;
                    case 3:
                      oled.prepare();
                      oled.string_orientation();
                      oledIdx--;
                      break;
                    case 4:
                      oled.line();
                      break;
                    case 5:
                      oled.triangle();
                      break;
                    case 6:
                      oled.bitmap();
                      break;
                    default:
                      break;
                  }
                if (++oledIdx > 6) { oledIdx = 0; }
                oled.sendBuffer();
              #endif
            #ifdef USE_STATUS
              dispStatus("");
              #endif
          }

        #endif // defined(DISP)
      // ----------------------
      sleep(1);
    }
// --- end of implementation
//
// --- templates
        // template websever
          #ifdef USE_WEBSERVER
            #ifdef DUMMY
            void drawGraph()
            {
              String out = "";
              char temp[100];
              out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
              out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
              out += "<g stroke=\"black\">\n";
              int y = rand() % 130;
              for (int x = 10; x < 390; x += 10)
              {
                int y2 = rand() % 130;
                sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
                out += temp;
                y = y2;
              }
              out += "</g>\n</svg>\n";

              server.send(200, "image/svg+xml", out);
            }

            void handleRoot()
            {
              digitalWrite(led, 1);
              char temp[400];
              int sec = millis() / 1000;
              int min = sec / 60;
              int hr = min / 60;

              snprintf(temp, 400,

                "<html>\
                  <head>\
                    <meta http-equiv='refresh' content='5'/>\
                    <title>ESP32 Demo</title>\
                    <style>\
                      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
                    </style>\
                  </head>\
                  <body>\
                    <h1>Hello from ESP32!</h1>\
                    <p>Uptime: %02d:%02d:%02d</p>\
                    <img src=\"/test.svg\" />\
                  </body>\
                </html>",

                         hr, min % 60, sec % 60
                        );
              server.send(200, "text/html", temp);
              digitalWrite(led, 0);
            }

            void handleNotFound()
            {
              digitalWrite(led, 1);
              String message = "File Not Found\n\n";
              message += "URI: ";
              message += server.uri();
              message += "\nMethod: ";
              message += (server.method() == HTTP_GET) ? "GET" : "POST";
              message += "\nArguments: ";
              message += server.args();
              message += "\n";

              for (uint8_t i = 0; i < server.args(); i++)
              {
                message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
              }

              server.send(404, "text/plain", message);
              digitalWrite(led, 0);
            }
            #endif
          #endif // USE_WEBSERVER

        //
        // template touchscreen
#ifdef USE_TOUCHSCREEN
  #ifdef DUMMY

  #include "FS.h"

  #include <SPI.h>
  #include <TFT_eSPI.h>      // Hardware-specific library

  TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

  // This is the file name used to store the calibration data
  // You can change this to create new calibration files.
  // The SPIFFS file name must start with "/".
  #define CALIBRATION_FILE "/TouchCalData1"

  // Set REPEAT_CAL to true instead of false to run calibration
  // again, otherwise it will only be done once.
  // Repeat calibration if you change the screen rotation.
  #define REPEAT_CAL false

  // Keypad start position, key sizes and spacing
  #define KEY_X 40 // Centre of key
  #define KEY_Y 96
  #define KEY_W 62 // Width and height
  #define KEY_H 30
  #define KEY_SPACING_X 18 // X and Y gap
  #define KEY_SPACING_Y 20
  #define KEY_TEXTSIZE 1   // Font size multiplier

  // Using two fonts since numbers are nice when bold
  #define LABEL1_FONT &FreeSansOblique12pt7b // Key label font 1
  #define LABEL2_FONT &FreeSansBold12pt7b    // Key label font 2

  // Numeric display box size and location
  #define DISP_X 1
  #define DISP_Y 10
  #define DISP_W 238
  #define DISP_H 50
  #define DISP_TSIZE 3
  #define DISP_TCOLOR TFT_CYAN

  // Number length, buffer for storing it and character index
  #define NUM_LEN 12
  char numberBuffer[NUM_LEN + 1] = "";
  uint8_t numberIndex = 0;

  // We have a status line for messages
  #define STATUS_X 120 // Centred on this
  #define STATUS_Y 65

  // Create 15 keys for the keypad
  char keyLabel[15][5] = {"New", "Del", "Send", "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "#" };
  uint16_t keyColor[15] = {TFT_RED, TFT_DARKGREY, TFT_DARKGREEN,
                           TFT_BLUE, TFT_BLUE, TFT_BLUE,
                           TFT_BLUE, TFT_BLUE, TFT_BLUE,
                           TFT_BLUE, TFT_BLUE, TFT_BLUE,
                           TFT_BLUE, TFT_BLUE, TFT_BLUE
                          };

  // Invoke the TFT_eSPI button class and create all the button objects
  TFT_eSPI_Button key[15];

  //------------------------------------------------------------------------------------------

  void setup() {
    // Use serial port
    Serial.begin(9600);

    // Initialise the TFT screen
    tft.init();

    // Set the rotation before we calibrate
    tft.setRotation(0);

    // Calibrate the touch screen and retrieve the scaling factors
    touch_calibrate();

    // Clear the screen
    tft.fillScreen(TFT_BLACK);

    // Draw keypad background
    tft.fillRect(0, 0, 240, 320, TFT_DARKGREY);

    // Draw number display area and frame
    tft.fillRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_BLACK);
    tft.drawRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_WHITE);

    // Draw keypad
    drawKeypad();
  }

  //------------------------------------------------------------------------------------------

  void loop(void) {
    uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

    // Pressed will be set true is there is a valid touch on the screen
    boolean pressed = tft.getTouch(&t_x, &t_y);

    // / Check if any key coordinate boxes contain the touch coordinates
    for (uint8_t b = 0; b < 15; b++) {
      if (pressed && key[b].contains(t_x, t_y)) {
        key[b].press(true);  // tell the button it is pressed
      } else {
        key[b].press(false);  // tell the button it is NOT pressed
      }
    }

    // Check if any key has changed state
    for (uint8_t b = 0; b < 15; b++) {

      if (b < 3) tft.setFreeFont(LABEL1_FONT);
      else tft.setFreeFont(LABEL2_FONT);

      if (key[b].justReleased()) key[b].drawButton();     // draw normal

      if (key[b].justPressed()) {
        key[b].drawButton(true);  // draw invert

        // if a numberpad button, append the relevant # to the numberBuffer
        if (b >= 3) {
          if (numberIndex < NUM_LEN) {
            numberBuffer[numberIndex] = keyLabel[b][0];
            numberIndex++;
            numberBuffer[numberIndex] = 0; // zero terminate
          }
          status(""); // Clear the old status
        }

        // Del button, so delete last char
        if (b == 1) {
          numberBuffer[numberIndex] = 0;
          if (numberIndex > 0) {
            numberIndex--;
            numberBuffer[numberIndex] = 0;//' ';
          }
          status(""); // Clear the old status
        }

        if (b == 2) {
          status("Sent value to serial port");
          Serial.println(numberBuffer);
        }
        // we dont really check that the text field makes sense
        // just try to call
        if (b == 0) {
          status("Value cleared");
          numberIndex = 0; // Reset index to 0
          numberBuffer[numberIndex] = 0; // Place null in buffer
        }

        // Update the number display field
        tft.setTextDatum(TL_DATUM);        // Use top left corner as text coord datum
        tft.setFreeFont(&FreeSans18pt7b);  // Choose a nicefont that fits box
        tft.setTextColor(DISP_TCOLOR);     // Set the font colour

        // Draw the string, the value returned is the width in pixels
        int xwidth = tft.drawString(numberBuffer, DISP_X + 4, DISP_Y + 12);

        // Now cover up the rest of the line up by drawing a black rectangle.  No flicker this way
        // but it will not work with italic or oblique fonts due to character overlap.
        tft.fillRect(DISP_X + 4 + xwidth, DISP_Y + 1, DISP_W - xwidth - 5, DISP_H - 2, TFT_BLACK);

        delay(10); // UI debouncing
      }
    }
  }


  #endif
  //------------------------------------------------------------------------------------------



#endif // USE_TOUCHSCREEN

