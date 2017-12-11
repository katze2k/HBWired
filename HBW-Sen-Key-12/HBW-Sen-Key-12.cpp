//*******************************************************************
//
// HBW-Sen-Key-12
//
// Homematic Wired Hombrew Hardware
// Arduino Uno als Homematic-Device
// HBW-Sen-Key-12 zum Einlesen von 12 Tastern
// - Active HIGH oder LOW kann konfiguriert werden
// - Pullup kann aktiviert werden
// - Erkennung von Doppelklicks
// - Zusaetzliches Event beim Loslassen einer lang gedrueckten Taste
// TODO (list)
// - konfig von doppelklick zeit
// - konfig von long klick zeit
// - konfig von long klick action
// - konfig von doppelklick action
// - I/O anpassen
// - debug entfernen
// - neue device ID
//*******************************************************************

#include <Arduino.h>
#include "HBW-Sen-Key-12.h"		//Device config, Pin Defs
#include "EEPROM/EEPROM.h"			// EEProm
#include "FreeRam/FreeRam.h"	// Free RAM Memory
#include "ClickButton/ClickButton.h" 	// lib for read in buttons
#include "HBWired/HBWired.h"         // HB Wired protocol and module
#include "HBWSoftwareSerial/HBWSoftwareSerial.h" //soft serial for rs485 - hardware serial for debug
#include "HBWLinkKey/HBWLinkKey.h"   // Links Key=button (? 2 Actuator ?)
//#include "HBWKey/HBWKey.h" //  ?not linked key=button?
//#include "HBWLinkSwitchSimple/HBWLinkSwitchSimple.h" // ? linked Switch=actuator
//#include "HBWSwitch/HBWSwitch.h" // ? not linked switch=actuator

// HBWSoftwareSerial can only do 19200 baud
HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX

// Class HBSenKey
class HBSenKey : public HBWChannel {
  public:
    HBSenKey(uint8_t _pin, hbw_config_key* _config);
    virtual void loop(HBWDevice*, uint8_t channel);
    void afterReadConfig();
  private:
    uint8_t pin;   // Pin
    uint32_t lastSentLong;      // Zeit, zu der das letzte Mal longPress gesendet wurde
    uint8_t keyPressNum;
    int8_t keyState;   
    hbw_config_key* config;    
    ClickButton button;
};


HBSenKey* keys[NUM_CHANNELS];

class HBSenDevice : public HBWDevice {
    public: 
    HBSenDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
            Stream* _rs485, uint8_t _txen, 
            uint8_t _configSize, void* _config, 
        uint8_t _numChannels, HBWChannel** _channels,
        Stream* _debugstream, HBWLinkSender* linksender = NULL, HBWLinkReceiver* linkreceiver = NULL) :
          HBWDevice(_devicetype, _hardware_version, _firmware_version,
            _rs485, _txen, _configSize, _config, _numChannels, ((HBWChannel**)(_channels)),
            _debugstream, linksender, linkreceiver) {
      // looks like virtual methods are not properly called here
      afterReadConfig();        
    };

    virtual ~HBSenDevice(){}; //Destructor to avoid Warning

    void afterReadConfig() {
        // defaults setzen
        if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 20;
        // if(config.central_address == 0xFFFFFFFF) config.central_address = 0x00000001;
        for(uint8_t channel = 0; channel < NUM_CHANNELS; channel++){
            if(hbwconfig.keys[channel].long_press_time == 0xFF) 
                hbwconfig.keys[channel].long_press_time = 10;
            keys[channel]->afterReadConfig();    
        };
    };
};


HBSenDevice* device = NULL;



HBSenKey::HBSenKey(uint8_t _pin, hbw_config_key* _config) 
              : config(_config), pin(_pin), 
                button(_pin,LOW,HIGH) { 
};

void HBSenKey::afterReadConfig(){
    button = ClickButton(pin, config->inverted ? LOW : HIGH, config->pullup ? HIGH : LOW);
    button.debounceTime   = 20;   // Debounce timer in ms
    button.multiclickTime = 250;  // Time limit for multi clicks
    button.longClickTime  = config->long_press_time;
    button.longClickTime *= 100; // Time until long clicks register 
}


void HBSenKey::loop(HBWDevice* device, uint8_t channel) {

  long now = millis();
  uint8_t data; 

  button.Update();
  if (button.clicks) {
    keyState = button.clicks;
    keyPressNum++;
    if (button.clicks == 1) { // Einfachklick
        // TODO: Peering. Only for normal short and long click?
        // TODO: doesn't waiting for multi-clicks make everything slow?  
        device->sendKeyEvent(channel,keyPressNum, false);  // short press
    }
    // Multi-Click
    else if (button.clicks >= 2) {  // Mehrfachklick
        data = (keyPressNum << 2) + 1;
        device->sendKeyEvent(channel, 1, &data);

    } else if (button.clicks < 0) {  // erstes LONG
        lastSentLong = now;
        device->sendKeyEvent(channel,keyPressNum, true);  // long press
    }
  } else if (keyState < 0) {   // Taste ist oder war lang gedr�ckt
        if (button.depressed) {  // ist noch immer gedrueckt --> alle 300ms senden
          if(now - lastSentLong >= 300){ // alle 300ms erneut senden
            lastSentLong = lastSentLong + 300;
            device->sendKeyEvent(channel,keyPressNum, true);  // long press
          }
        } else {    // "Losgelassen" senden
            data = keyPressNum << 2; // + 0
            device->sendKeyEvent(channel, 1, &data);
            keyState = 0;
        }

  }

}


void setup()
{
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  Serial.begin(19200);
  rs485.begin();    // RS485 via SoftwareSerial

   // create channels
   PIN_ARRAY
  // Keys
   for(uint8_t i = 0; i < NUM_CHANNELS; i++){
      keys[i] = new HBSenKey(pins[i], &(hbwconfig.keys[i]));
   };

  device = new HBSenDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &rs485,RS485_TXEN,sizeof(hbwconfig),&hbwconfig,NUM_CHANNELS,(HBWChannel**)keys,&Serial,
                         new HBWLinkKey(NUM_LINKS,LINKADDRESSSTART), NULL);

  device->setConfigPins();  // 8 and 13 is the default
 
  hbwdebug(F("B: 2A "));
  hbwdebug(freeRam());
  hbwdebug(F("\n")); 
}


void loop()
{
  device->loop();
};
