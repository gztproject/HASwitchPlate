#include <telnet.h>

WiFiServer telnet::server(23);
WiFiClient telnet::client;

void telnet::handleClient()
{ // Basic telnet client handling code from: https://gist.github.com/tablatronix/4793677ca748f5f584c95ec4a2b10303
  static unsigned long telnetInputIndex = 0;
  if (server.hasClient())
  { // client is connected
    if (!client || !client.connected())
    {
      if (client)
      {
        client.stop(); // client disconnected
      }
      client = server.available(); // ready for new client
      telnetInputIndex = 0;                    // reset input buffer index
    }
    else
    {
      server.available().stop(); // have client, block new connections
    }
  }
  // Handle client input from telnet connection.
  if (client && client.connected() && client.available())
  { // client input processing
    static char telnetInputBuffer[TELNET_INPUT_MAX];

    if (client.available())
    {
      char telnetInputByte = client.read(); // Read client byte
      // debugPrintln(String("telnet in: 0x") + String(telnetInputByte, HEX));
      if (telnetInputByte == 5)
      { // If the telnet client sent a bunch of control commands on connection (which end in ENQUIRY/0x05), ignore them and restart the buffer
        telnetInputIndex = 0;
      }
      else if (telnetInputByte == 13)
      { // telnet line endings should be CRLF: https://tools.ietf.org/html/rfc5198#appendix-C
        // If we get a CR just ignore it
      }
      else if (telnetInputByte == 10)
      {                                          // We've caught a LF (DEC 10), send buffer contents to the Nextion
        telnetInputBuffer[telnetInputIndex] = 0; // null terminate our char array
        nextion::sendCmd(String(telnetInputBuffer));
        telnetInputIndex = 0;
      }
      else if (telnetInputIndex < TELNET_INPUT_MAX)
      { // If we have room left in our buffer add the current byte
        telnetInputBuffer[telnetInputIndex] = telnetInputByte;
        telnetInputIndex++;
      }
    }
  }
}

WiFiServer telnet::getServer()
{
  return server;
}