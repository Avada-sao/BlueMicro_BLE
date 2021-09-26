#include <arduino.h>
#include "hid_common.h"

// BLE HID object
BLEDis bledis;
BLEHidAdafruit ble_hid;

// USB HID object
Adafruit_USBD_HID usb_hid;

 //------------- HID START -------------//
void hid_start(void)
{
  hid_USB_start();
  hid_BLE_start();
}

void hid_USB_start(void)
{
    usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  //usb_hid.setStringDescriptor("TinyUSB HID Composite");

  usb_hid.begin();
}

void hid_BLE_start(void)
{
  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values

  // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather 52");
  bledis.begin();
  /* Start BLE HID
   * Note: Apple requires BLE device must have min connection interval >= 20m
   * ( The smaller the connection interval the faster we could send data).
   * However for HID and MIDI device, Apple could accept min connection interval 
   * up to 11.25 ms. Therefore ble_hidAdafruit::begin() will try to set the min and max
   * connection interval to 11.25  ms and 15 ms respectively for best performance.
   */
  ble_hid.begin();

  // Set callback for set LED from central
  ble_hid.setKeyboardLedCallback(set_keyboard_led);

  /* Set connection interval (min, max) to your perferred value.
   * Note: It is already set by ble_hidAdafruit::begin() to 11.25ms - 15ms
   * min = 9*1.25=11.25 ms, max = 12*1.25= 15 ms 
   */
  /* Bluefruit.Periph.setConnInterval(9, 12); */
  }

 /*bool hid_sendReport(uint8_t report_id, void const *report, uint8_t len){
    return ble_hid.sendReport( report_id,  report, len);
  }*/

 //------------- BLE Start ADV -------------//
void startAdv(void)
{  
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);
  
  // Include BLE HID service
  Bluefruit.Advertising.addService(ble_hid);

  // There is enough room for the dev name in the advertising packet
  Bluefruit.Advertising.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

//------------- HID_Ready -------------//

bool hid_ready(void)
{
  return hid_USB_ready() ||  hid_BLE_ready();
}

bool hid_BLE_ready(void)
{
  return (Bluefruit.connected()>0);
}

bool hid_USB_ready(void)
{
  return usb_hid.ready();
}

//------------- USB only HID API -------------//
  bool hid_USB_sendReport(uint8_t report_id, void const *report, uint8_t len){
    
   
    return hid_USB_ready() ? usb_hid.sendReport( report_id,  report, len): false;
  }

  bool hid_USB_sendReport8(uint8_t report_id, uint8_t num){
    return hid_USB_ready() ? usb_hid.sendReport8( report_id,  num): false;
  }
  bool hid_USB_sendReport16(uint8_t report_id, uint16_t num){
    return hid_USB_ready() ? usb_hid.sendReport16( report_id, num): false;
  }
  bool hid_USB_sendReport32(uint8_t report_id, uint32_t num){
    return hid_USB_ready() ? usb_hid.sendReport32( report_id,  num): false;
  }

//------------- Keyboard API -------------//
  bool hid_keyboardReport(uint8_t modifier, uint8_t keycode[6]){
    return hid_BLE_keyboardReport(modifier, keycode) || hid_USB_keyboardReport(RID_KEYBOARD, modifier, keycode);
  }
  bool hid_keyboardPress(char ch)
  {
    return hid_BLE_keyboardPress(ch) || hid_USB_keyboardPress(RID_KEYBOARD,ch);
  }
  bool hid_keyboardRelease(){
    return hid_BLE_keyboardRelease() || hid_USB_keyboardRelease(RID_KEYBOARD);
  }


  bool hid_BLE_keyboardReport(uint8_t modifier, uint8_t keycode[6]){
    Serial.println("hid_BLE_keyboardReport");
    return hid_BLE_ready() ?  ble_hid.keyboardReport( modifier,  keycode): false;
  }
  bool hid_BLE_keyboardPress(char ch){
    return hid_BLE_ready() ?   ble_hid.keyPress(ch): false;
  }
  bool hid_BLE_keyboardRelease(){
    return hid_BLE_ready() ?   ble_hid.keyRelease(): false;
  }

  bool hid_USB_keyboardReport(uint8_t report_id, uint8_t modifier, uint8_t keycode[6]){
    return hid_USB_ready() ? usb_hid.keyboardReport( report_id,  modifier,  keycode): false;
  }
  bool hid_USB_keyboardPress(uint8_t report_id, char ch){
    return hid_USB_ready() ? usb_hid.keyboardPress( report_id, ch): false;
  }
  bool hid_USB_keyboardRelease(uint8_t report_id){
    return hid_USB_ready() ? usb_hid.keyboardRelease( report_id): false;
  }
  //------------- Mouse API -------------//

  bool hid_mouseReport( uint8_t buttons, int8_t x, int8_t y,
                   int8_t vertical, int8_t horizontal){
    return hid_USB_mouseReport(RID_MOUSE, buttons,  x,  y, vertical, horizontal) || hid_BLE_mouseReport(buttons,  x,  y, vertical, horizontal);
  }
  bool hid_mouseMove( int8_t x, int8_t y){
    return hid_USB_mouseMove(RID_MOUSE, x,  y) || hid_BLE_mouseMove(x,  y);
  }
  bool hid_mouseScroll( int8_t scroll, int8_t pan){
    return hid_USB_mouseScroll( RID_MOUSE, scroll,  pan) || hid_BLE_mouseScroll( scroll,  pan);
  }
  bool hid_mouseButtonPress( uint8_t buttons){
    return hid_USB_mouseButtonPress(RID_MOUSE,buttons) || hid_BLE_mouseButtonPress(buttons);
  }
  bool hid_mouseButtonRelease(){
    return hid_USB_mouseButtonRelease(RID_MOUSE) || hid_BLE_mouseButtonRelease();
  }

  
  bool hid_BLE_mouseReport( uint8_t buttons, int8_t x, int8_t y,
                   int8_t vertical, int8_t horizontal){
                    Serial.println("hid_BLE_mouseReport");
    return hid_BLE_ready() ?    ble_hid.mouseReport(buttons,  x,  y, vertical, horizontal): false;
  }
  bool hid_BLE_mouseMove( int8_t x, int8_t y){
    return hid_BLE_ready() ?    ble_hid.mouseMove(  x,  y): false;
  }
  bool hid_BLE_mouseScroll( int8_t scroll, int8_t pan){
    return hid_BLE_ready() ?    ble_hid.mouseScroll(  scroll,  pan): false;
  }
  bool hid_BLE_mouseButtonPress( uint8_t buttons){
    return hid_BLE_ready() ?    ble_hid.mouseButtonPress(buttons): false;
  }
  bool hid_BLE_mouseButtonRelease(){
    return hid_BLE_ready() ?    ble_hid.mouseButtonRelease(): false;
  }


  bool hid_USB_mouseReport(uint8_t report_id, uint8_t buttons, int8_t x, int8_t y,
                   int8_t vertical, int8_t horizontal){
    return hid_USB_ready() ? usb_hid.mouseReport( report_id,  buttons,  x,  y, vertical, horizontal): false;
  }
  bool hid_USB_mouseMove(uint8_t report_id, int8_t x, int8_t y){
    return hid_USB_ready() ? usb_hid.mouseMove( report_id,  x,  y): false;
  }
  bool hid_USB_mouseScroll(uint8_t report_id, int8_t scroll, int8_t pan){
    return hid_USB_ready() ? usb_hid.mouseScroll( report_id,  scroll,  pan): false;
  }
  bool hid_USB_mouseButtonPress(uint8_t report_id, uint8_t buttons){
    return hid_USB_ready() ? usb_hid.mouseButtonPress( report_id,  buttons): false;
  }
  bool hid_USB_mouseButtonRelease(uint8_t report_id){
    return hid_USB_ready() ? usb_hid.mouseButtonRelease( report_id): false;
  }

  //------------- Consumer API -------------//

  bool hid_consumerKeyPress(uint16_t usage_code)
  {
    return hid_USB_consumerKeyPress(RID_CONSUMER_CONTROL,usage_code) || hid_BLE_consumerKeyPress(usage_code);
  }
  
  bool hid_consumerKeyRelease(void)
  {
    return hid_USB_consumerKeyRelease(RID_CONSUMER_CONTROL) || hid_BLE_consumerKeyRelease();
  }
  
  bool hid_BLE_consumerKeyPress(uint16_t usage_code)
  {
    Serial.println("hid_BLE_consumerKeyPress");
    return hid_BLE_ready() ?   ble_hid.consumerKeyPress(usage_code): false; 
  }
  
  bool hid_BLE_consumerKeyRelease(void)
  {
    return hid_BLE_ready() ?   ble_hid.consumerKeyRelease(): false; 
  }

  bool hid_USB_consumerKeyPress(uint8_t report_id,uint16_t usage_code)
  {
    return hid_USB_ready() ?  hid_USB_sendReport16(report_id, usage_code): false;  
  }
  
  bool hid_USB_consumerKeyRelease(uint8_t report_id)
  {
    return hid_USB_ready() ?  hid_USB_sendReport16(report_id, 0): false;  
  }

//------------- Common HID API -------------//
  bool hid_send_reports(uint8_t modifier, uint8_t keycode[6], uint8_t buttons, int8_t x, int8_t y,
                   int8_t vertical, int8_t horizontal , uint16_t consumer_code )                   
{
  Serial.println("hid_send_reports");
  hid_USB_send_reports(modifier, keycode, buttons, x, y, vertical, horizontal, consumer_code);
  hid_BLE_send_reports(modifier, keycode, buttons, x, y, vertical, horizontal, consumer_code);
}

bool hid_BLE_send_reports(uint8_t modifier, uint8_t keycode[6], uint8_t buttons, int8_t x, int8_t y,
                   int8_t vertical, int8_t horizontal , uint16_t consumer_code ){
        Serial.println("hid_BLE_send_reports");
        if (hid_BLE_ready())
        {
        hid_BLE_mouseReport(buttons,  x,  y, vertical,  horizontal); 
        delay(10);
        hid_BLE_keyboardReport(modifier, keycode);
        delay(10);
        hid_BLE_consumerKeyPress(consumer_code);  
        return true;   
        }
        else
        {
          return false;             
        }
  }


bool hid_USB_send_reports(uint8_t modifier, uint8_t keycode[6], uint8_t buttons, int8_t x, int8_t y,
                   int8_t vertical, int8_t horizontal , uint16_t consumer_code ){
        Serial.println("hid_USB_send_reports");
        if (hid_USB_ready()) 
        {  
        hid_USB_mouseReport(RID_MOUSE, buttons,  x,  y, vertical,  horizontal); 
        delay(10);
        hid_USB_keyboardReport(RID_KEYBOARD, modifier, keycode);
        delay(10);
        hid_USB_sendReport16(RID_CONSUMER_CONTROL, consumer_code);  
        return true;
        }
        else
        {
          return false;
        }          
        
  }


//------------- LED Callback -------------//
/**
 * Callback invoked when received Set LED from central.
 * Must be set previously with setKeyboardLedCallback()
 *
 * The LED bit map is as follows: (also defined by KEYBOARD_LED_* )
 *    Kana (4) | Compose (3) | ScrollLock (2) | CapsLock (1) | Numlock (0)
 */
void set_keyboard_led(uint16_t conn_handle, uint8_t led_bitmap)
{
  (void) conn_handle;
  
  // light up Red Led if any bits is set
  if ( led_bitmap )
  {
    ledOn( LED_RED );
  }
  else
  {
    ledOff( LED_RED );
  }
}
