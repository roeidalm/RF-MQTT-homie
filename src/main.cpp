#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <RCSwitch.h>
#include <Homie.h>

int pin; // int for Receive pin.

RCSwitch mySwitch = RCSwitch();

HomieNode irRecvNode("irRecv", "IrReceiver", "reader");                // TODO: this will be the resiveing codes and sending TO the SERVER
HomieNode tvStandNode("tvStandModeNode", "TVStandModeNode", "switch"); // TODO: this will be the FROM the SERVER
bool cc_RX_Mode = true;
void setup()
{
  Serial.begin(115200);

#ifdef ESP32
  pin = 4; // for esp32! Receiver on GPIO pin 4.
#elif ESP8266
  pin = 4; // for esp8266! Receiver on pin 4 = D2.
#else
  pin = 0; // for Arduino! Receiver on interrupt 0 => that is pin #2
#endif

  if (ELECHOUSE_cc1101.getCC1101())
  { // Check the CC1101 Spi connection.
    Serial.println("Connection OK");
  }
  else
  {
    Serial.println("Connection Error");
  }

  // CC1101 Settings:                (Settings with "//" are optional!)
  ELECHOUSE_cc1101.Init(); // must be set to initialize the cc1101!
                           // ELECHOUSE_cc1101.setRxBW(812.50);  // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
  // ELECHOUSE_cc1101.setPA(10);       // set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12)   Default is max!
  ELECHOUSE_cc1101.setMHZ(433.92); // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.

  mySwitch.enableReceive(pin); // Receiver on

  ELECHOUSE_cc1101.SetRx(); // set Receive on

  Homie.disableResetTrigger();
  Homie_setFirmware("Rf-Bridge-MQTT", "1.0.0");
  Homie.setLoopFunction(loopHandler);
  Homie.onEvent(onHomieEvent);
  advertiseSetup();
  Homie.setup();
}
void loop()
{
  Homie.loop();
}

void onHomieEvent(const HomieEvent &event)
{
  switch (event.type)
  {
  case HomieEventType::ABOUT_TO_RESET:
    reset();
    break;
  default:
    break;
  }
}

void loopHandler()
{
  if (cc_RX_Mode)
  {
    if (mySwitch.available())
    {

      unsigned long rfButton = mySwitch.getReceivedValue();
      Homie.getLogger() << "rfReceiver_cmd: " << rfButton << endl;

      String proto = mySwitch.getReceivedProtocol();
      Homie.getLogger() << "rfReceiver_proto: " << rfButton << endl;


      mySwitch.resetAvailable();

      irRecvNode.setProperty("rfReceiver_cmd").send(rfButton); // String(rfButton) // will be the long data
      irRecvNode.setProperty("rfReceiver_proto").send(proto);  // will be the row data
    }
  }
}

void advertiseSetup()
{
  tvStandNode.advertise("rfSender_cmd").setName("Sending RF Command from the cc1101 TX").setDatatype("long").settable(tvStandOpenSetStateOnHandler);
  irRecvNode.advertise("rfReceiver_cmd").setName("Receiver RF Command from the cc1101 RX").setDatatype("long");
  irRecvNode.advertise("rfReceiver_proto").setName("Receiver RF Protocol from the cc1101 RX").setDatatype("string");
}

bool tvStandOpenSetStateOnHandler(const HomieRange &range, const String &value)
{
  unsigned long longValue = atol(value.c_str());
  cc_RX_Mode = false;
  ELECHOUSE_cc1101.SetTx();
  myRfSwitch.send(longValue, 24);
  ELECHOUSE_cc1101.SetRx(); // set Recive on
  cc_RX_Mode = true;
  Homie.getLogger() << "Sending RF Command from the cc1101 TX: " << longValue << endl;
  tvStandNode.setProperty("rfSender_cmd").send(longValue);

  return true;
}