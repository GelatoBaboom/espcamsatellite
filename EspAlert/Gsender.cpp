#include "Gsender.h"
#include <base64.h>
//openssl s_client -connect smtp.googlemail.com:587 -starttls smtp | openssl x509 -fingerprint -noout
const char fingerprint[] PROGMEM = "40 DB 15 E9 E0 D8 DC 87 EF 1B 37 57 F9 8E 5B 4F 9E 38 4F 10";
//const char fingerprint[] PROGMEM = "BB DC E1 3E 9D 53 7A 52 29 91 5C B1 23 C7 AA B0 A8 55 E7 98";
Gsender* Gsender::_instance = 0;
Gsender::Gsender() {}
Gsender* Gsender::Instance()
{
  if (_instance == 0)
    _instance = new Gsender;
  return _instance;
}

Gsender* Gsender::Subject(const char* subject)
{
  delete [] _subject;
  _subject = new char[strlen(subject) + 1];
  strcpy(_subject, subject);
  return _instance;
}
Gsender* Gsender::Subject(const String &subject)
{
  return Subject(subject.c_str());
}

bool Gsender::AwaitSMTPResponse(WiFiClient &client, const String &resp, uint16_t timeOut)
{
  uint32_t ts = millis();
  _serverResponce = "";
  while (!client.available())
  {
    if (millis() > (ts + timeOut)) {
      _error = "SMTP Response TIMEOUT!";
      return false;
    }
  }
  while (client.available())
  {
    String sl = client.readStringUntil('\n');
    _serverResponce += sl;
#if defined(GS_SERIAL_LOG_1) || defined(GS_SERIAL_LOG_2)
    Serial.println(sl);
#endif
  }

  if (resp && _serverResponce.indexOf(resp) == -1) return false;
  return true;
}

String Gsender::getLastResponce()
{
  return _serverResponce;
}

const char* Gsender::getError()
{
  return _error;
}
String encodeStr(const char* input )
{
  return base64::encode(input);
}
bool Gsender::Send(const String &to, const String &message)
{
  WiFiClient client;
  //client.setFingerprint(fingerprint);
  //client.setInsecure();
#if defined(GS_SERIAL_LOG_2)
  Serial.print("Connecting to :");
  Serial.println(SMTP_SERVER);
#endif
  if (!client.connect(SMTP_SERVER, SMTP_PORT)) {
    _error = "Could not connect to mail server";
    return false;
  }
  if (!AwaitSMTPResponse(client, "220")) {
    _error = "Connection Error";
    return false;
  }

#if defined(GS_SERIAL_LOG_2)
  Serial.println("HELO friend:");
#endif
  client.println("EHLO ESP8266_test");
  if (!AwaitSMTPResponse(client, "250")) {
    _error = "identification error";
    return false;
  }
#if defined(GS_SERIAL_LOG_2)
  Serial.println("AUTH LOGIN:");
#endif
  client.println("AUTH LOGIN");
  AwaitSMTPResponse(client);

#if defined(GS_SERIAL_LOG_2)
  Serial.println("EMAILBASE64_LOGIN:");
#endif
  client.println(encodeStr(EMAILBASE64_LOGIN));
  AwaitSMTPResponse(client);

#if defined(GS_SERIAL_LOG_2)
  Serial.println("EMAILBASE64_PASSWORD:");
#endif
  client.println(encodeStr(EMAILBASE64_PASSWORD));
  if (!AwaitSMTPResponse(client, "235")) {
    _error = "SMTP AUTH error";
    return false;
  }

  String mailFrom = "MAIL FROM: <" + String(FROM) + '>';
#if defined(GS_SERIAL_LOG_2)
  Serial.println(mailFrom);
#endif
  client.println(mailFrom);
  AwaitSMTPResponse(client);

  String rcpt = "RCPT TO: <" + to + '>';
#if defined(GS_SERIAL_LOG_2)
  Serial.println(rcpt);
#endif
  client.println(rcpt);
  AwaitSMTPResponse(client);

#if defined(GS_SERIAL_LOG_2)
  Serial.println("DATA:");
#endif
  client.println("DATA");
  if (!AwaitSMTPResponse(client, "354")) {
    _error = "SMTP DATA error";
    return false;
  }

  client.println("From: <" + String(FROM) + '>');
  client.println("To: <" + to + '>');

  client.print("Subject: ");
  client.println(_subject);

  client.println("Mime-Version: 1.0");
  client.println("Content-Type: text/html; charset=\"UTF-8\"");
  client.println("Content-Transfer-Encoding: 7bit");
  client.println();
  String body = "<!DOCTYPE html><html lang=\"en\">" + message + "</html>";
  client.println(body);
  client.println(".");
  if (!AwaitSMTPResponse(client, "250")) {
    _error = "Sending message error";
    return false;
  }
  client.println("QUIT");
  if (!AwaitSMTPResponse(client, "221")) {
    _error = "SMTP QUIT error";
    return false;
  }
  return true;
}
