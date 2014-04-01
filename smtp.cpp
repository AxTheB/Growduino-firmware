#include "GrowduinoFirmware.h"
#include <Arduino.h>

#include "smtp.h"

/* * *
  sends email mesasge
  copied from http://playground.arduino.cc/Code/Email#.Uzc30HWx0S4
* * */

extern EthernetClient eth_client;
extern Config config;

int send_mail(char * dest, char * subject, char * body){
    byte thisByte = 0;
    byte respCode;
    if(eth_client.connect(config.smtp,25)) {
        Serial.println(F("SMTP connect"));
    } else {
        Serial.println(F("connection failed"));
        return 0;
    }

    if(!eRcv()) return 0;
    Serial.println(F("Sending helo"));

    // change to your public ip
    char send_mail_buf[] = "255.255.255.255";
    config.inet_ntoa(Ethernet.localIP(), send_mail_buf);
    eth_client.print(F("helo "));
    eth_client.println(send_mail_buf);

    if(!eRcv()) return 0;
    Serial.println(F("Sending From"));

    // change to your email address (sender)
    eth_client.print(F("MAIL From: <"));
    eth_client.print(config.mail_from);
    eth_client.println(F(">"));

    if(!eRcv()) return 0;

    // change to recipient address
    Serial.println(F("Sending To"));
    eth_client.print(F("RCPT To: <"));
    eth_client.print(dest);
    eth_client.print(F(">"));

    if(!eRcv()) return 0;

    Serial.println(F("Sending DATA"));
    eth_client.println(F("DATA"));

    if(!eRcv()) return 0;

    Serial.println(F("Sending email"));

    // change to recipient address
    eth_client.print(F("To: <"));
    eth_client.print(dest);
    eth_client.println(F(">"));

    // change to your address
    eth_client.print(F("From: Me <"));
    eth_client.print(config.mail_from);
    eth_client.print(F(">"));

    eth_client.print(F("Subject: "));
    eth_client.print(subject);
    eth_client.println(F("\r\n"));

    eth_client.println(body);

    eth_client.println(F("."));

    if(!eRcv()) return 0;

    Serial.println(F("Sending QUIT"));
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

