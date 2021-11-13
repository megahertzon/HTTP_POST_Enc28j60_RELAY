#include <EtherCard.h>
#include <avr/wdt.h> 

/*
  PIN MAP:
   SO  - 12
   SCK - 13
   SI  - 11
   CS  - 10 (AS PER #DEFINE)
*/

#define ethCSpin 10 //put the digital pin that the CS is connected to (library default is 8)
#define relay1 A1
#define relay2 A0
#define buzzer A3
#define led 8

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 }; //this is an example but any other uniqe number can be used

// giving static ip address (must be unique on the LAN)
static byte myip[] = { 192, 168, 1, 203 };

byte Ethernet::buffer[500]; //This ethernet buffer contains all the information necessary for data transfer like ip addresses and tcp incoming data
//this buffer is what takes up most of the global variable space so adjust it as necessary
BufferFiller bfill; //to access buffer fill functions

Stash stash; //This is a datatype in the EtherCard library which is sent in HTTP requests. I believe it operates as a linked list

int res = 100; //timing variable

boolean isPulse1 = false;
boolean isPulse2 = false;
boolean isPulseOk=false;

//initializing the ethernet module to preset static ip address
void staticInit() {
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  ether.staticSetup(myip);
  ether.printIp("IP: ", ether.myip);
  res = 180; //reset init value
}

/*
   ether server is used to receive incoming tcp packets
   here we just keep listening(on loop) and if we get a message
   we print the message to the serial monitor and reply back to the client
   i've set it up as int so we can monitor whether a request was received or not
*/

int etherServer() {
  word len = ether.packetReceive(); //checks for packets
  word pos = ether.packetLoop(len); //gets the pointer to the data in the buffer

  if (pos) {  //accesses the incoming message and prints it to the serial monitor
    bfill = ether.tcpOffset();
    char* request = (char *) Ethernet::buffer + pos;
    Serial.println(request);
    String req(request);
    if (req.indexOf("Rel123$$") >= 0) {
      Serial.print("Relay1");
      isPulse1 = true;
      isPulseOk=true;
    }
    if (req.indexOf("Rel223$$") >= 0) {
      Serial.print("Relay2");
      isPulse2 = true;
      isPulseOk=true;
    }
    return 1; //true (request received)
  }
  else {
    return 0; //false (no meaningful packets received)
  }
}

static word serverReply() {
  bfill = ether.tcpOffset(); //putting the response in the buffer
  if (isPulseOk){
    bfill.emit_p(PSTR(
                 "HTTP/1.1 200 OK" "\r\n"    //http headers and status
                 "Content-Type: text/plain" "\r\n"
                 "\r\n"
                 "Pulse Ok")); //reply message (payload). adjust the reply as required  
  }else {
    bfill.emit_p(PSTR(
                 "HTTP/1.1 200 OK" "\r\n"    //http headers and status
                 "Content-Type: text/plain" "\r\n"
                 "\r\n"
                 "No Req")); //reply message (payload). adjust the reply as required
  }
  isPulseOk=false;
  
  return bfill.position(); //returns the position of the reply which then gets sent out when the function is called using httpServerReply function in ethercard library.
}


// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind) {
  char found = 0;
  char index = 0;
  char len;

  len = strlen(str);

  if (strlen(sfind) > len) {
    return 0;
  }
  while (index < len) {
    if (str[index] == sfind[found]) {
      found++;
      if (strlen(sfind) == found) {
        return 1;
      }
    }
    else {
      found = 0;
    }
    index++;
  }

  return 0;
}


void setup() {
  wdt_disable();
  wdt_enable(WDTO_4S);
  Serial.begin(9600);//Setting baud rate for serial communication
  Serial.println("\n[Ethernet HTTP Requests]");
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(led, OUTPUT);
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  staticInit();

  digitalWrite(buzzer, HIGH);
  delay(500);
  digitalWrite(buzzer, LOW);

}



void loop() {
  wdt_reset();
  
  if (etherServer()) {
    ether.httpServerReply(serverReply());
  }
  digitalWrite(led,digitalRead(led));
  if (isPulse1) {
    isPulse1 = false;
    digitalWrite(relay1, HIGH);
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(relay1, LOW);
    digitalWrite(buzzer, LOW);
  }
  if (isPulse2) {
    isPulse2 = false;
    digitalWrite(relay1, HIGH);
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(relay1, LOW);
    digitalWrite(buzzer, LOW);
  }

}
