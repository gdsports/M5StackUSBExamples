/* Simplified Thrustmaster T.16000M FCS Joystick Report Parser */

#include <M5Stack.h>
#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

// Thrustmaster T.16000M HID report
struct GamePadEventData
{
  uint16_t	buttons;
  uint8_t		hat;
  uint16_t	x;
  uint16_t	y;
  uint8_t		twist;
  uint8_t		slider;
}__attribute__((packed));

class JoystickEvents
{
  public:
    virtual void OnGamePadChanged(const GamePadEventData *evt);
};

#define RPT_GAMEPAD_LEN	sizeof(GamePadEventData)

class JoystickReportParser : public HIDReportParser
{
  JoystickEvents		*joyEvents;

  uint8_t oldPad[RPT_GAMEPAD_LEN];

  public:
  JoystickReportParser(JoystickEvents *evt);

  virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};


JoystickReportParser::JoystickReportParser(JoystickEvents *evt) :
  joyEvents(evt)
{}

void JoystickReportParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf)
{
  // Checking if there are changes in report since the method was last called
  bool match = (sizeof(oldPad) == len) && (memcmp(oldPad, buf, len) == 0);

  // Calling Game Pad event handler
  if (!match && joyEvents) {
    joyEvents->OnGamePadChanged((const GamePadEventData*)buf);
    memcpy(oldPad, buf, len);
  }
}

void JoystickEvents::OnGamePadChanged(const GamePadEventData *evt)
{
  M5.Lcd.fillScreen(TFT_NAVY);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLUE);
  M5.Lcd.fillRect(0, 0, 320, 30, TFT_BLUE);
  M5.Lcd.println("T16K Flight Stick");

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 31);
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.print("X      : "); M5.Lcd.println(evt->x, HEX);
  M5.Lcd.print("Y      : "); M5.Lcd.println(evt->y, HEX);
  M5.Lcd.print("Hat    : "); M5.Lcd.println(evt->hat, HEX);
  M5.Lcd.print("Twist  : "); M5.Lcd.println(evt->twist, HEX);
  M5.Lcd.print("Slider : "); M5.Lcd.println(evt->slider, HEX);
  M5.Lcd.print("Buttons: "); M5.Lcd.println(evt->buttons, HEX);
}

USB                                             Usb;
USBHub                                          Hub(&Usb);
HIDUniversal                                    Hid(&Usb);
JoystickEvents                                  JoyEvents;
JoystickReportParser                            Joy(&JoyEvents);

void setup()
{
  M5.begin();
  M5.Lcd.fillScreen(TFT_NAVY);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLUE);
  M5.Lcd.fillRect(0, 0, 320, 30, TFT_BLUE);
  M5.Lcd.println("T16K Flight Stick");

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 31);
  M5.Lcd.setTextColor(TFT_YELLOW);

  Serial.begin( 115200 );
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Start");

  if (Usb.Init() == -1)
    Serial.println("OSC did not start.");

  delay( 200 );

  if (!Hid.SetReportParser(0, &Joy))
    ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1  );
}

void loop()
{
  Usb.Task();

  M5.update();
}
