/*
 * Works with Dymo M10 USB postage scale. This scale automatically powers off
 * after 3 minutes of no activity. This is based on the scale example but
 * removes the LCD code.
 */

#include <M5Stack.h>
#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>

#define DBSerial Serial

const char * const UNITS[] = {
  "units",        // unknown unit
  "mg",           // milligram
  "g",            // gram
  "kg",           // kilogram
  "cd",           // carat
  "taels",        // lian
  "gr",           // grain
  "dwt",          // pennyweight
  "tonnes",       // metric tons
  "tons",         // avoir ton
  "ozt",          // troy ounce
  "oz",           // ounce
  "lbs"           // pound
};

/* Scale status constants */
#define REPORT_FAULT    0x01
#define ZEROED          0x02
#define WEIGHING        0x03
#define WEIGHT_VALID    0x04
#define WEIGHT_NEGATIVE 0x05
#define OVERWEIGHT      0x06
#define CALIBRATE_ME    0x07
#define ZERO_ME         0x08

const char * const SCALE_STATUS[] = {
  NULL,                         // 0
  "Report fault",               // 1
  "Scale zero set",             // 2
  "Weighing...",                // 3
  "Weight: ",                   // 4
  "Negative weight",            // 5
  "Max weight reached",         // 6
  "Scale calibration required", // 7
  "Scale zeroing required"      // 8
};

/* input data report */
struct ScaleEventData
{
  uint8_t reportID; //must be 3
  uint8_t status;
  uint8_t unit;
  int8_t exp;       //scale factor for the weight
  uint16_t weight;  //
};

class ScaleEvents
{

  public:

    virtual void OnScaleChanged(const ScaleEventData *evt);
};

#define RPT_SCALE_LEN sizeof(ScaleEventData)/sizeof(uint8_t)

class ScaleReportParser : public HIDReportParser
{
  public:
    ScaleReportParser(ScaleEvents *evt);
    virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);

  private:
    uint8_t oldScale[RPT_SCALE_LEN];
    ScaleEvents *scaleEvents;
};

ScaleReportParser::ScaleReportParser(ScaleEvents *evt) :
  scaleEvents(evt)
{
  memset(oldScale, 0, RPT_SCALE_LEN);
}

void ScaleReportParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf)
{
  // If new report same as old report, ignore it.
  if (len != RPT_SCALE_LEN) return;
  if (memcmp(buf, oldScale, RPT_SCALE_LEN) == 0) return;

  // Calling scale event handler
  if (scaleEvents) {
    scaleEvents->OnScaleChanged((const ScaleEventData*)buf);
    memcpy(oldScale, buf, RPT_SCALE_LEN);
  }
}

void ScaleEvents::OnScaleChanged(const ScaleEventData *evt)
{
  if( evt->reportID != 3 ) {
    DBSerial.println("Invalid report!");
    return;
  }
  M5.Lcd.fillScreen(TFT_NAVY);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLUE);
  M5.Lcd.fillRect(0, 0, 320, 30, TFT_BLUE);
  M5.Lcd.println("Postage Scale");

  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(0, 31);
  M5.Lcd.setTextColor(TFT_YELLOW);

  switch( evt->status ) {
    case REPORT_FAULT:
    case ZEROED:
    case WEIGHING:
    case WEIGHT_NEGATIVE:
    case OVERWEIGHT:
    case CALIBRATE_ME:
    case ZERO_ME:
      DBSerial.println(SCALE_STATUS[evt->status]);
      M5.Lcd.println(SCALE_STATUS[evt->status]);
      break;

    case WEIGHT_VALID:
      DBSerial.print( SCALE_STATUS[evt->status] );
      DBSerial.print( evt->weight * pow( 10, evt->exp ) );
      DBSerial.print(' ');
      DBSerial.println( UNITS[ evt->unit ]);
      M5.Lcd.print(SCALE_STATUS[evt->status]);
      M5.Lcd.print(evt->weight * pow(10, evt->exp));
      M5.Lcd.print(' ');
      M5.Lcd.println(UNITS[ evt->unit ]);
      break;

    default:
      DBSerial.print("Undefined status code: ");
      DBSerial.println( evt->status );
      M5.Lcd.print("Undefined status code: ");
      M5.Lcd.println(evt->status);
      break;
  }
}

USB                 UsbH;
USBHub              Hub(&UsbH);
HIDUniversal        Hid(&UsbH);
ScaleEvents         Events;
ScaleReportParser   Scale(&Events);

void setup()
{
  M5.begin();
  M5.Lcd.fillScreen(TFT_NAVY);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLUE);
  M5.Lcd.fillRect(0, 0, 320, 30, TFT_BLUE);
  M5.Lcd.println("Postage Scale");

  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(0, 31);
  M5.Lcd.setTextColor(TFT_YELLOW);

  DBSerial.begin( 115200 );
  DBSerial.println("Start");

  if (UsbH.Init())
    DBSerial.println("USB host did not start.");

  delay( 200 );

  if (!Hid.SetReportParser(0, &Scale))
    DBSerial.println("SetReportParser failed");
}

void loop()
{
  UsbH.Task();
}
