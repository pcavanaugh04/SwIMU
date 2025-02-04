//========================================================//
/* Header File for LEDManager for SwIMU project
Managers functions of the onboard LED

Contains:
  Classes:
    LEDManager
  
  Functions
    blink - Blink an LED in a specfic sequence
    off - Turn onboard LEDs off

*/
//==========================================================//

#ifndef LEDMANAGER_H
#define LEDMANAGER_H

#include <Arduino.h>

class LED {
  // Class for abstracting functions of an LED
  private: 
    pin_size_t pin;


  public:
    LED(pin_size_t ledPin);
    bool isOn = false;
    void turnOn();
    void turnOff();
};

// Class definition
class LEDManager {
  // Private Variables
  private:
    unsigned long LEDTimer = 0;
    int sequenceIndex = 0;   
    LED redLED;
    LED blueLED;
    LED greenLED;  

  //Public attributes
  public:
    LEDManager(pin_size_t RED_LED, pin_size_t GREEN_LED, pin_size_t BLUE_LED);
    // void init
    void blink(int sequenceArray[], int sequenceSize, char LEDColors[]);
    void turnOff();

};

#endif