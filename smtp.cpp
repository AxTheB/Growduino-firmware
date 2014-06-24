#include "GrowduinoFirmware.h"
#include <Arduino.h>

#include "smtp.h"

/* * *
  sends email mesasge
  copied from http://playground.arduino.cc/Code/Email#.Uzc30HWx0S4
* * */

EthernetClient eth_client;
extern Config config;

int send_mail(char * dest, char * subject, char * body){
    byte smtp[4] = { config.smtp[0], config.smtp[1], config.smtp[2], config.smtp[3] };
    if(eth_client.connect(smtp, 25)) {
#ifdef DEBUG_SMTP
        Serial.println(F("SMTP connect"));
        Serial.println(config.smtp);
#endif
        lcd_publish(F("SMTP connect"));
    } else {
#ifdef DEBUG_SMTP
        Serial.println(F("connection failed"));
#endif
        lcd_publish(F("SMTP fail"));
        return 0;
    }

    if(!eRcv()) return 0;
#ifdef DEBUG_SMTP
    Serial.println(F("Sending helo"));
#endif

    eth_client.print(F("helo "));
    eth_client.println(config.sys_name);

    if(!eRcv()) return 0;
#ifdef DEBUG_SMTP
    Serial.println(F("Sending From"));
#endif

    // change to your email address (sender)
    eth_client.print(F("MAIL From: <"));
    eth_client.print(config.mail_from);
    eth_client.println(F(">"));

    if(!eRcv()) return 0;

    // change to recipient address
#ifdef DEBUG_SMTP
    Serial.println(F("Sending To"));
#endif
    eth_client.print(F("RCPT To: <"));
    eth_client.print(dest);
    eth_client.println(F(">"));

    if(!eRcv()) return 0;

#ifdef DEBUG_SMTP
    Serial.println(F("Sending DATA"));
#endif
    eth_client.println(F("DATA"));

    if(!eRcv()) return 0;

#ifdef DEBUG_SMTP
    Serial.println(F("Sending email"));
#endif

    // change to recipient address
    eth_client.print(F("To: <"));
    eth_client.print(dest);
    eth_client.println(F(">"));

    // change to your address
    eth_client.print(F("From: "));
    eth_client.print(config.sys_name);
    eth_client.print(F(" <"));
    eth_client.print(config.mail_from);
    eth_client.println(F(">"));

    eth_client.print(F("Subject: "));
    eth_client.print(config.sys_name);
    eth_client.print(F(" alert"));
    eth_client.println(F("\r\n"));

    eth_client.println(body);

#ifdef DEBUG_SMTP
    Serial.println(body);
#endif

    eth_client.println(F("."));

    if(!eRcv()) return 0;

#ifdef DEBUG_SMTP
    Serial.println(F("Sending QUIT"));
#endif
    eth_client.println(F("QUIT"));

    if(!eRcv()) return 0;

    eth_client.stop();

    Serial.println(F("disconnected"));

    return 1;
}

byte eRcv(){
  byte respCode;
  byte thisByte;
  int loopCount = 0;

  while(!eth_client.available()) {
    #ifdef WATCHDOG
    wdt_reset();
    #endif
    delay(1);
    loopCount++;

    // if nothing received for 10 seconds, timeout
    if(loopCount > 10000) {
      eth_client.stop();
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }

  respCode = eth_client.peek();

  while(eth_client.available())
  {
    thisByte = eth_client.read();
    Serial.write(thisByte);
  }

  if(respCode >= '4')
  {
    efail();
    return 0;
  }

  return 1;
}

void efail()
{
  byte thisByte = 0;
  int loopCount = 0;

  eth_client.println(F("QUIT"));

  while(!eth_client.available()) {
    #ifdef WATCHDOG
    wdt_reset();
    #endif
    delay(1);
    loopCount++;

    // if nothing received for 10 seconds, timeout
    if(loopCount > 10000) {
      eth_client.stop();
      Serial.println(F("\r\nTimeout"));
      return;
    }
  }

  while(eth_client.available())
  {
    thisByte = eth_client.read();
    Serial.write(thisByte);
  }

  eth_client.stop();

  Serial.println(F("disconnected"));
}

