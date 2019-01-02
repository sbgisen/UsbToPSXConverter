#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>
#include "pgmstrings.h"

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

#include "JoystickReportParser.h"

USB Usb;
//USBHub Hub(&Usb);
HIDUniversal Hid(&Usb);
JoystickReportParser Uni;

void setup()
{
    Serial.begin(115200);
#if !defined(__MIPSEL__)
    while (!Serial)
        ; // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
    Serial.println("Start");

    if (Usb.Init() == -1)
        Serial.println("OSC did not start.");

    delay(200);

    if (!Hid.SetReportParser(0, &Uni))
        ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1);
}

void loop()
{
    Usb.Task();
}
