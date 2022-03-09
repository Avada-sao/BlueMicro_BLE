// SPDX-FileCopyrightText: 2018-2022 BlueMicro contributors (https://github.com/jpconstantineau/BlueMicro_BLE/graphs/contributors)
//
// SPDX-License-Identifier: BSD-3-Clause

/**************************************************************************************************************************/
#include "firmware_setup.h"

commandlist_t commandList; 
commandqueue_t setupQueue;
commandqueue_t commandQueue;
commandqueue_t loopQueue;

using namespace Adafruit_LittleFS_Namespace;
#define SETTINGS_FILE "/settings"
File file(InternalFS);

/**************************************************************************************************************************/
// Keyboard Matrix
byte rows[] MATRIX_ROW_PINS;        // Contains the GPIO Pin Numbers defined in keyboard_config.h
byte columns[] MATRIX_COL_PINS;     // Contains the GPIO Pin Numbers defined in keyboard_config.h  
 std::vector<uint16_t> stringbuffer; // buffer for macros to type into...
 std::vector<HIDKeyboard> reportbuffer; 

/**************************************************************************************************************************/
void setupConfig() {
  InternalFS.begin();
  loadConfig();

  keyboardstate.statusble=0;  //initialize to a known state.
  keyboardstate.statuskb=0;   //initialize to a known state.

  keyboardstate.user1=0;   //initialize to a known state.  
  keyboardstate.user2=0;   //initialize to a known state. 
  keyboardstate.user3=0;   //initialize to a known state.

  keyboardstate.helpmode = false;
  keyboardstate.timestamp = millis();
  keyboardstate.lastupdatetime = keyboardstate.timestamp;
  keyboardstate.lastreporttime = 0;
  keyboardstate.lastuseractiontime = 0;

  keyboardstate.connectionState = CONNECTION_NONE;
  keyboardstate.needReset = false;
  keyboardstate.needUnpair = false;
  keyboardstate.needFSReset = false;
  keyboardstate.save2flash = false;

}

/**************************************************************************************************************************/
void loadConfig()
{
  file.open(SETTINGS_FILE, FILE_O_READ);

  if(file)
  {
    file.read(&keyboardconfig, sizeof(keyboardconfig));
    file.close();
  }
  else
  {
    resetConfig();
    saveConfig();
  }

 if (keyboardconfig.version != BLUEMICRO_CONFIG_VERSION) // SETTINGS_FILE format changed. we need to reset and re-save it.
 {
    resetConfig();
    saveConfig();
 }
}

/**************************************************************************************************************************/
void resetConfig()
{
  keyboardconfig.version=BLUEMICRO_CONFIG_VERSION;
  keyboardconfig.pinPWMLED=BACKLIGHT_LED_PIN;
  keyboardconfig.pinRGBLED=WS2812B_LED_PIN;
  keyboardconfig.pinBLELED=STATUS_BLE_LED_PIN;  
  keyboardconfig.pinKBLED=STATUS_KB_LED_PIN;

  keyboardconfig.enablePWMLED=BACKLIGHT_PWM_ON;
  keyboardconfig.enableRGBLED=WS2812B_LED_ON;
  keyboardconfig.enableBLELED=BLE_LED_ACTIVE;
  keyboardconfig.enableKBLED=STATUS_KB_LED_ACTIVE;

  keyboardconfig.polarityBLELED=BLE_LED_POLARITY;
  keyboardconfig.polarityKBLED=STATUS_KB_LED_POLARITY;

  keyboardconfig.enableVCCSwitch=VCC_ENABLE_GPIO;
  keyboardconfig.polarityVCCSwitch=VCC_DEFAULT_ON;

  keyboardconfig.enableChargerControl=VCC_ENABLE_CHARGER;
  keyboardconfig.polarityChargerControl=true;

  keyboardconfig.enableSerial = SERIAL_DEBUG_CLI_DEFAULT_ON;   

  keyboardconfig.mode = 0; 
  keyboardconfig.user1 = 0;  
  keyboardconfig.user2 = 0; 

  keyboardconfig.matrixscaninterval=HIDREPORTINGINTERVAL;
  keyboardconfig.batteryinterval=BATTERYINTERVAL;
  keyboardconfig.keysendinterval=HIDREPORTINGINTERVAL;
  keyboardconfig.lowpriorityloopinterval=LOWPRIORITYLOOPINTERVAL;
  keyboardconfig.lowestpriorityloopinterval = HIDREPORTINGINTERVAL*2;
  keyboardconfig.connectionMode  = CONNECTION_MODE_AUTO;
  keyboardconfig.BLEProfile = 0;
  keyboardconfig.BLEProfileEdiv[0] = 0xFFFF;
  keyboardconfig.BLEProfileEdiv[1] = 0xFFFF;
  keyboardconfig.BLEProfileEdiv[2] = 0xFFFF;
  memset(keyboardconfig.BLEProfileAddr[0], 0, 6);
  memset(keyboardconfig.BLEProfileAddr[1], 0, 6);
  memset(keyboardconfig.BLEProfileAddr[2], 0, 6);
  strcpy(keyboardconfig.BLEProfileName[0], "unpaired");
  strcpy(keyboardconfig.BLEProfileName[1], "unpaired");
  strcpy(keyboardconfig.BLEProfileName[2], "unpaired");

}

/**************************************************************************************************************************/
void saveConfig()
{
  InternalFS.remove(SETTINGS_FILE);

  if (file.open(SETTINGS_FILE, FILE_O_WRITE))
  {
    file.write((uint8_t*)&keyboardconfig, sizeof(keyboardconfig));
    file.close();
  }
}


void setuphid()
{
  bluemicro_hid.setBLEManufacturer(MANUFACTURER_NAME);
  bluemicro_hid.setBLETxPower(DEVICE_POWER);
  bluemicro_hid.setBLEModel(DEVICE_NAME);
  bluemicro_hid.setHIDMessageDelay(HIDREPORTINGINTERVAL);
  bluemicro_hid.setUSBPollInterval(HIDREPORTINGINTERVAL/2);
  bluemicro_hid.setUSBStringDescriptor(DEVICE_NAME);
  bluemicro_hid.begin(); 
}

void setupnrf52()
{
  setupGpio();                                                                // checks that NFC functions on GPIOs are disabled.
  setupWDT();
}

void serialsplash()
{
  Serial.begin(115200);
        Serial.println(" ____  _            __  __ _                   ____  _     _____ ");
        Serial.println("| __ )| |_   _  ___|  \\/  (_) ___ _ __ ___    | __ )| |   | ____|");
        Serial.println("|  _ \\| | | | |/ _ \\ |\\/| | |/ __| '__/ _ \\   |  _ \\| |   |  _|  ");
        Serial.println("| |_) | | |_| |  __/ |  | | | (__| | | (_) |  | |_) | |___| |___ ");
        Serial.println("|____/|_|\\__,_|\\___|_|  |_|_|\\___|_|  \\___/___|____/|_____|_____|");
        Serial.println("                                         |_____|                 ");
        Serial.println("");
}

/**************************************************************************************************************************/
//
/**************************************************************************************************************************/
void setupMatrix(void) {
    //inits all the columns as INPUT
   for (const auto& column : columns) {
      LOG_LV2("BLEMIC","Setting to INPUT Column: %i" ,column);
      pinMode(column, INPUT);
    }

   //inits all the rows as INPUT_PULLUP
   for (const auto& row : rows) {
      LOG_LV2("BLEMIC","Setting to INPUT_PULLUP Row: %i" ,row);
      pinMode(row, INPUT_PULLUP);
    }
};

void addsetupcommands()
{
  SETUPCOMMAND(commandList, COMMANDID(0) , setuphid());
  ADDCOMMAND(setupQueue, COMMANDID(0));
  SETUPCOMMAND(commandList, COMMANDID(1) , setupnrf52());
  ADDCOMMAND(setupQueue, COMMANDID(1));
  SETUPCOMMAND(commandList, COMMANDID(2) , serialsplash());
  if (keyboardconfig.enableSerial) 
  {
    ADDCOMMAND(setupQueue, COMMANDID(2));
  }
  SETUPCOMMAND(commandList, COMMANDID(3) , switchVCC(keyboardconfig.polarityVCCSwitch)); // turn on VCC when starting up if needed.
  if(keyboardconfig.enableVCCSwitch)
  {
    ADDCOMMAND(setupQueue, COMMANDID(3));
  }
  SETUPCOMMAND(commandList, COMMANDID(4) , switchCharger(keyboardconfig.polarityChargerControl)); // turn on Charger when starting up if needed.
  if(keyboardconfig.enableChargerControl)
  {
    ADDCOMMAND(setupQueue, COMMANDID(4));
  }

  SETUPCOMMAND(commandList, COMMANDID(5) , setupKeymap());
  ADDCOMMAND(setupQueue, COMMANDID(5));
  SETUPCOMMAND(commandList, COMMANDID(6) , setupMatrix());
  ADDCOMMAND(setupQueue, COMMANDID(6));
  
  SETUPCOMMAND(commandList, COMMANDID(7) , stringbuffer.clear());
  ADDCOMMAND(setupQueue, COMMANDID(7));
  SETUPCOMMAND(commandList, COMMANDID(8) , reportbuffer.clear());
  ADDCOMMAND(setupQueue, COMMANDID(8));

  SETUPCOMMAND(commandList, COMMANDID(9) , statusLEDs.enable());
  ADDCOMMAND(setupQueue, COMMANDID(9));
  SETUPCOMMAND(commandList, COMMANDID(10) , statusLEDs.hello()); // blinks Status LEDs a couple as last step of setup.
  ADDCOMMAND(setupQueue, COMMANDID(10));
}

void toggleserial()
{
  keyboardconfig.enableSerial = !keyboardconfig.enableSerial; 
  keyboardstate.save2flash = true; 
  keyboardstate.needReset = true;
}

void togglehelpmode()
{
    keyboardstate.helpmode = !keyboardstate.helpmode;
}

void clearbonds()
{
        // Bluefruit.clearBonds(); //removed in next BSP?
    if (keyboardstate.connectionState == CONNECTION_BLE)
      keyboardstate.needUnpair = true;
    // Bluefruit.Central.clearBonds();
}

void automode()
{
    keyboardconfig.connectionMode = CONNECTION_MODE_AUTO;
    if (keyboardstate.helpmode) {
      addStringToQueue("Automatic USB/BLE - Active");
      addKeycodeToQueue(KC_ENTER);
      addStringToQueue("USB Only");
      addKeycodeToQueue(KC_ENTER);
      addStringToQueue("BLE Only");
      addKeycodeToQueue(KC_ENTER);
  }
}

void usbmode()
{
#ifdef NRF52840_XXAA // only the 840 has USB available.
    keyboardconfig.connectionMode = CONNECTION_MODE_USB_ONLY;
    if (keyboardstate.helpmode) {
      addStringToQueue("Automatic USB/BLE");
      addKeycodeToQueue(KC_ENTER);
      addStringToQueue("USB Only - Active");
      addKeycodeToQueue(KC_ENTER);
      addStringToQueue("BLE Only");
      addKeycodeToQueue(KC_ENTER);
    }
#else
    if (keyboardstate.helpmode) [&](){
      addStringToQueue("USB not available on NRF52832");
      addKeycodeToQueue(KC_ENTER);
    }
#endif
}

void blemode()
{
    keyboardconfig.connectionMode = CONNECTION_MODE_BLE_ONLY;
    if (keyboardstate.helpmode) {
      addStringToQueue("Automatic USB/BLE");
      addKeycodeToQueue(KC_ENTER);
      addStringToQueue("USB Only");
      addKeycodeToQueue(KC_ENTER);
      addStringToQueue("BLE Only - Active");
      addKeycodeToQueue(KC_ENTER);
    }  
}


void printbattery()
{
    char buffer [50];
  uint8_t intval;
      intval = batterymonitor.vbat_per;
      switch (batterymonitor.batt_type)
      {
        case BATT_UNKNOWN:
            snprintf (buffer, sizeof(buffer), "VDD = %.0f mV, VBatt = %.0f mV", batterymonitor.vbat_vdd*1.0, batterymonitor.vbat_mv*1.0);
        break;
        case BATT_CR2032:
            if (intval>99)
            {
              snprintf (buffer, sizeof(buffer), "VDD = %.0f mV (%4d %%)", batterymonitor.vbat_mv*1.0, intval);
            }
            else
            {
              snprintf (buffer, sizeof(buffer), "VDD = %.0f mV (%3d %%)", batterymonitor.vbat_mv*1.0, intval);
            }    
        break;
        case BATT_LIPO:
            if (intval>99)
            {
              sprintf (buffer, "LIPO = %.0f mV (%4d %%)", batterymonitor.vbat_mv*1.0, intval);
            }
            else
            {
              sprintf (buffer, "LIPO = %.0f mV (%3d %%)", batterymonitor.vbat_mv*1.0, intval);
            }   
        break;
        case BATT_VDDH:
            if (intval>99)
            {
              sprintf (buffer, "LIPO = %.0f mV (%4d %%)", batterymonitor.vbat_mv*1.0, intval);
            }
            else
            {
              sprintf (buffer, "LIPO = %.0f mV (%3d %%)", batterymonitor.vbat_mv*1.0, intval);
            }   
        break;
      } 
    addStringToQueue(buffer);
    addKeycodeToQueue(KC_ENTER);
}

void printinfo()
{
    char buffer [50];
  uint8_t intval;
      addStringToQueue("Keyboard Name  : " DEVICE_NAME " "); addKeycodeToQueue(KC_ENTER);
      addStringToQueue("Keyboard Model : " DEVICE_MODEL " "); addKeycodeToQueue(KC_ENTER);
      addStringToQueue("Keyboard Mfg   : " MANUFACTURER_NAME " "); addKeycodeToQueue(KC_ENTER);
      addStringToQueue("BSP Library    : " ARDUINO_BSP_VERSION " "); addKeycodeToQueue(KC_ENTER);
      sprintf(buffer,"Bootloader     : %s", getBootloaderVersion());
      addStringToQueue(buffer); 
      addKeycodeToQueue(KC_ENTER);
      sprintf(buffer,"Serial No      : %s", getMcuUniqueID());
      addStringToQueue(buffer);
      addKeycodeToQueue(KC_ENTER);
      sprintf(buffer,"Device Power   : %f", DEVICE_POWER*1.0);
      addStringToQueue(buffer); addKeycodeToQueue(KC_ENTER);
        switch (keyboardconfig.connectionMode)
        {
          case CONNECTION_MODE_AUTO:
          addStringToQueue("CONNECTION_MODE_AUTO"); addKeycodeToQueue(KC_ENTER);
          break;
          case CONNECTION_MODE_USB_ONLY:
          addStringToQueue("CONNECTION_MODE_USB_ONLY"); addKeycodeToQueue(KC_ENTER);
          break;
          case CONNECTION_MODE_BLE_ONLY:
          addStringToQueue("CONNECTION_MODE_BLE_ONLY"); addKeycodeToQueue(KC_ENTER);
          break;
        }
            switch (keyboardstate.connectionState)
            {
              case CONNECTION_USB:
                addStringToQueue("CONNECTION_USB"); addKeycodeToQueue(KC_ENTER);
              break;

              case CONNECTION_BLE:
                addStringToQueue("CONNECTION_BLE"); addKeycodeToQueue(KC_ENTER);
              break;

              case CONNECTION_NONE:
                addStringToQueue("CONNECTION_NONE"); addKeycodeToQueue(KC_ENTER);
              break;
            }
}

  void printble()
  {
  char buffer [50];
  uint8_t intval;
      addStringToQueue("Keyboard Name: " DEVICE_NAME " "); addKeycodeToQueue(KC_ENTER);
      sprintf(buffer,"Device Power : %i", DEVICE_POWER); addStringToQueue(buffer);  addKeycodeToQueue(KC_ENTER);
      sprintf(buffer,"Filter RSSI  : %i", FILTER_RSSI_BELOW_STRENGTH); addStringToQueue(buffer);  addKeycodeToQueue(KC_ENTER); 
      addStringToQueue("Type\t RSSI\t name"); addKeycodeToQueue(KC_ENTER); 
      sprintf(buffer,"cent\t %i\t %s",keyboardstate.rssi_cent, keyboardstate.peer_name_cent);addStringToQueue(buffer); addKeycodeToQueue(KC_ENTER); 
      sprintf(buffer,"prph\t %i\t %s",keyboardstate.rssi_prph, keyboardstate.peer_name_prph);addStringToQueue(buffer); addKeycodeToQueue(KC_ENTER);
      sprintf(buffer,"cccd\t %i\t %s",keyboardstate.rssi_cccd, keyboardstate.peer_name_cccd);addStringToQueue(buffer); addKeycodeToQueue(KC_ENTER);
       sprintf(buffer,  "Profile 1:   %s", keyboardconfig.BLEProfileName[0]);
      addStringToQueue(buffer);
      sprintf(buffer,  "Profile 1: %02X:%02X:%02X:%02X:%02X:%02X - %s", keyboardconfig.BLEProfileAddr[0][5],
                                                                        keyboardconfig.BLEProfileAddr[0][4],
                                                                        keyboardconfig.BLEProfileAddr[0][3],
                                                                        keyboardconfig.BLEProfileAddr[0][2],
                                                                        keyboardconfig.BLEProfileAddr[0][1],
                                                                        keyboardconfig.BLEProfileAddr[0][0],
                                                                        keyboardconfig.BLEProfileName[0]);
      addStringToQueue(buffer);
      if (keyboardconfig.BLEProfile == 0) addStringToQueue(" (active)");
      addKeycodeToQueue(KC_ENTER);
      sprintf(buffer,  "Profile 2:   %s", keyboardconfig.BLEProfileName[1]);
      addStringToQueue(buffer);
      sprintf(buffer,  "Profile 2: %02X:%02X:%02X:%02X:%02X:%02X - %s", keyboardconfig.BLEProfileAddr[1][5],
                                                                        keyboardconfig.BLEProfileAddr[1][4],
                                                                        keyboardconfig.BLEProfileAddr[1][3],
                                                                        keyboardconfig.BLEProfileAddr[1][2],
                                                                        keyboardconfig.BLEProfileAddr[1][1],
                                                                        keyboardconfig.BLEProfileAddr[1][0],
                                                                        keyboardconfig.BLEProfileName[1]);
      addStringToQueue(buffer);
      if (keyboardconfig.BLEProfile == 1) addStringToQueue(" (active)");
      addKeycodeToQueue(KC_ENTER);
      sprintf(buffer,  "Profile 3:   %s", keyboardconfig.BLEProfileName[2]);
      addStringToQueue(buffer);
      sprintf(buffer,  "Profile 3: %02X:%02X:%02X:%02X:%02X:%02X - %s", keyboardconfig.BLEProfileAddr[2][5],
                                                                        keyboardconfig.BLEProfileAddr[2][4],
                                                                        keyboardconfig.BLEProfileAddr[2][3],
                                                                        keyboardconfig.BLEProfileAddr[2][2],
                                                                        keyboardconfig.BLEProfileAddr[2][1],
                                                                        keyboardconfig.BLEProfileAddr[2][0],
                                                                        keyboardconfig.BLEProfileName[2]);
      addStringToQueue(buffer);
      if (keyboardconfig.BLEProfile == 2) addStringToQueue(" (active)");
      addKeycodeToQueue(KC_ENTER);
      addKeycodeToQueue(KC_ENTER);
      ble_gap_addr_t gap_addr;
     // gap_addr = bt_getMACAddr(); // TODO MOVE THIS FUNCTION TO BE ACCESSIBLE IN LIBRARY
      sprintf(buffer,  "BT MAC Addr: %02X:%02X:%02X:%02X:%02X:%02X", gap_addr.addr[5], gap_addr.addr[4], gap_addr.addr[3], gap_addr.addr[2], gap_addr.addr[1], gap_addr.addr[0]);
      addStringToQueue(buffer);
      addKeycodeToQueue(KC_ENTER);
      addKeycodeToQueue(KC_ENTER);
  }
  void printhelp()
  {
    ;
  }

  void sleepnow()
  {
      if (keyboardstate.connectionState != CONNECTION_USB) sleepNow();
  }



void setdefaultbatterycalc()
{
      batterymonitor.setmvToPercentCallback(mvToPercent_default);
      batterymonitor.updateBattery(); // force an update
}
void settestbatterycalc()
{
      batterymonitor.setmvToPercentCallback(mvToPercent_test);
      batterymonitor.updateBattery(); // force an update
} 

void setbleprofile1()
{
     // if (keyboardstate.connectionState != CONNECTION_USB) // reseting/rebooting KB when BLE Profile switching on USB would be ennoying...
        {
          keyboardconfig.BLEProfile = 0;
          keyboardstate.save2flash = true;
          keyboardstate.needReset = true;
      }
}

void setbleprofile2()
{
     // if (keyboardstate.connectionState != CONNECTION_USB) // reseting/rebooting KB when BLE Profile switching on USB would be ennoying...
      {
          keyboardconfig.BLEProfile = 1;
          keyboardstate.save2flash = true;
          keyboardstate.needReset = true;
      }
}

void setbleprofile3()
{
    //  if (keyboardstate.connectionState != CONNECTION_USB) // reseting/rebooting KB when BLE Profile switching on USB would be ennoying...
      {
          keyboardconfig.BLEProfile = 2;
          keyboardstate.save2flash = true;
          keyboardstate.needReset = true;
      }
}

void addkeyboardcommands()
{
  SETUPCOMMAND(commandList,  RESET, NVIC_SystemReset() );
  SETUPCOMMAND(commandList,  DEBUG, toggleserial() );
  SETUPCOMMAND(commandList,  EEPROM_RESET, {keyboardstate.needFSReset = true;} );
  SETUPCOMMAND(commandList,  CLEAR_BONDS,clearbonds());
  SETUPCOMMAND(commandList,  DFU, enterOTADfu());
  SETUPCOMMAND(commandList,  SERIAL_DFU, enterSerialDfu());
  SETUPCOMMAND(commandList,  UF2_DFU, enterUf2Dfu() );
  SETUPCOMMAND(commandList,  HELP_MODE, togglehelpmode());
  SETUPCOMMAND(commandList,  OUT_AUTO,automode());
  SETUPCOMMAND(commandList,  OUT_USB, usbmode());
  SETUPCOMMAND(commandList,  OUT_BT,blemode());
  SETUPCOMMAND(commandList,  PRINT_BATTERY, printbattery());
  SETUPCOMMAND(commandList,  PRINT_INFO, printinfo());
  SETUPCOMMAND(commandList,  PRINT_BLE, printble());
  SETUPCOMMAND(commandList,  PRINT_HELP, printhelp());
  SETUPCOMMAND(commandList,  SLEEP_NOW, sleepnow());
  SETUPCOMMAND(commandList,  BATTERY_CALC_DEFAULT, setdefaultbatterycalc());
  SETUPCOMMAND(commandList,  BATTERY_CALC_TEST, settestbatterycalc());
  SETUPCOMMAND(commandList,  BLEPROFILE_1, setbleprofile1());
  SETUPCOMMAND(commandList,  BLEPROFILE_2, setbleprofile2());
  SETUPCOMMAND(commandList,  BLEPROFILE_3, setbleprofile3());
}
