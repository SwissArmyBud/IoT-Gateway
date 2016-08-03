//***** INCLUDES AND PROGRAM/COMPILER SETUP *****

#include <U8glib.h>
#include <Wire.h>
#include <SoftwareSerial.h>

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);  // Display which does not send AC

#define softSerialRX 2                    // assign the compiler some named values for our hardware ports
#define softSerialTX 3
#define linkPortUSR 4
#define readyPortUSR 5
#define resetLineUSR 6
#define outputPort1 8
#define outputPort2 7
#define outputPort3 9


#define serialBaud 9600


String tmpString = "";

//***** MAIN PROGRAM RUNNIG SPACE *****

void setup() {

  pinMode(linkPortUSR, INPUT);
  pinMode(readyPortUSR, INPUT);
  pinMode(resetLineUSR, OUTPUT);
  pinMode(outputPort1, OUTPUT);
  pinMode(outputPort2, OUTPUT);

  u8g.setColorIndex(1);                   // sets the LCD to be "ON" when we write to it

  loadingScreen();

}


void loop() {                              // ***** THIS AREA SETS UP VARIABLES IN THE SAME SCOPE AS THE LOGIC LOOP *****

  Serial.begin(28800);                                   // while allowing debug and update through the h/w port

  int upTime = 0;                           // initialize a variable to hold the uptime value
  int sentCount = 0;
  int lineCount = 0;
  char tmpChar = ' ';                      // setup a char to keep track of a single serial line input character
  char testChar = ' ';
  bool sendFlag = 0;
  bool functionFlag = 0;
  bool delayCounter = 0;


  do {                                     // ***** THIS IS THE INTERIOR/PROGRAM/LOGIC LOOP, RUNS FOREVER *****

    SoftwareSerial mySerial(softSerialRX, softSerialTX);
    mySerial.begin(serialBaud);

    upTime = micros();

    if (upTime - delayCounter > 5000) {    // if more than 1/20 of a second has passed since the last update

      delayCounter = micros();                // bring the delayCounter up to the current
      upTime = micros() / 1000000;

      u8g.firstPage();                      // refresh the page, passing in upTime converted to seconds
      do {
        updateScreen(upTime, lineCount, sentCount);
      } while ( u8g.nextPage() );
    }

    if (Serial.available()) {              // if the hardware/debug serial line sends information, do something with the stream
      while (Serial.available()) {
        tmpChar = Serial.read();
        mySerial.print(tmpChar);             // send the h/w information to the WIFI chip

      }
    }


    if (mySerial.available()) {       // if the WIFI chip sends information, do something with the stream
      while (mySerial.available()) {

        tmpChar = mySerial.read();    // assign the latest character from the s/w serial buffer to tmpChar

        if (tmpString.length() < 50) {      // if the tempString is less than 50 char in length, then keep adding to it
          tmpString += tmpChar;             // NOTE - this value will limit the HTTP response parsing ability of the Arduino
        }                                   //        as each variable requires ~ 6 char to parse, not including the 24 char header

        if (tmpChar == '\n') {              // if the last character received was a LF (line-feed) then start parsing the response

          Serial.print(F("Receved line: "));             // let the debug monitor know that parsing has begun
          Serial.println(tmpString);                     // print the line being parsed to the debugger, as well
          lineCount++;

          if (tmpString.indexOf(F("GET /")) != -1) {     // if the response contained an HTTP GET request, then we flag a HTML page send
            sendFlag = 1;
          }

          // TODO: build a line parser that understands function calls, use it to process the specific method call
          // TODO: then use string.replace(FROM, TO) to reformat the line without question marks

          if (sendFlag == 1) {                    // if parsing sets the page send flag, we go through the following setps

            sendPage();                           // call the sendPage function, which controls the entire HTML response from the Arduino
            sentCount++;                          // keeps track of the number of times the sendPage function is called - "attempted replies"
            sendFlag = 0;                         // reset the page send flag, clear the parsed response, flush all buffers, and confirm to debugger

            tmpString = "";
            Serial.flush();
            mySerial.flush();
            Serial.println(F("PAGE SENT"));
          }

          else {                                   // if parsing doesn't set the page send flag, we just clean up and start over

            functionFlag = false;
            tmpString = "";
            Serial.flush();
            mySerial.flush();
          }
        }
      }
    }
  } while (true);                          // ***** THIS ENDS THE INTERIOR LOGIC LOOP *****
}                                          // ***** THIS ENDS THE GLOBAL LOOP *****

SoftwareSerial sendPage () {

  SoftwareSerial mySerial (softSerialRX, softSerialTX);                // reinitialize the serial connection the the WIFI chip
  mySerial.begin(serialBaud);

  mySerial.println(F("HTTP/1.1 200 OK"));                              // send a basic header response, confirming the HTTP request
  mySerial.println(F("Content-Type: text/html"));
  mySerial.println(F("Connection: close"));
  mySerial.println();
  mySerial.println(F("<!DOCTYPE html>"));                              // let the browser know we are sending basic html
  mySerial.println(F("<html>"));                                       // HTML begins, followed by a **HEADER**
  mySerial.println(F("<head>"));
  mySerial.println(F("<title>Arduino WIFI HTTP Server</title>"));      // this area is the page title, displayed on the browser tab
  mySerial.println(F("</head>"));
  mySerial.println(F("<body>"));                                       // this area begins the HTML **BODY** area
  mySerial.println(F("<h1>Output Control Area</h1>"));                 // basic header with text
  mySerial.println(F("<form method=\"get\">"));                        // begin a **FORM** group, backbone of communication from browser
  processPortCheckbox(outputPort1, F("OUT1"), F("LED One"));                      // a mapped output port, HTML name, and common name
  processPortCheckbox(outputPort2, F("OUT2"), F("LED Two"));                      // a mapped output port, HTML name, and common name
  processPortCheckbox(outputPort3, F("OUT3"), F("Relay Trigger"));
  mySerial.println(F("</form>"));
  mySerial.println(F("<h1>System Status</h1>"));                       // basic header with text
  processPortStatus(outputPort1, F("LED 1"), F("ON"), F("OFF"));                                  // a mapped input port, common name, on text, off text
  processPortStatus(outputPort2, F("LED 2"), F("ON"), F("OFF"));                                 // a mapped input port, common name, on text, off text
  processPortStatus(outputPort3, F("RELAY"), F("ACTIVE"), F("NOT ACTIVE"));
  //mySerial.println(F("<h1>Social Media Zone</h1>"));                 // basic header with text
  //mySerial.print(F("<a href=\"https:/"));
  //mySerial.println(F("/twitter.com/intent/tweet\?button_hashtag=InternetOfThings\" class=\"twitter-hashtag-button\" data-size=\"large\">Tweet #InternetOfThings</a>"));
  //mySerial.println(F("<script>!function(d,s,id){var js,fjs=d.getElementsByTagName(s)[0],p=/^http:/.test(d.location)?'http':'https';if(!d.getElementById(id)){js=d.createElement(s);js.id=id;js.src=p+'://platform.twitter.com/widgets.js';fjs.parentNode.insertBefore(js,fjs);}}(document, 'script', 'twitter-wjs');</script>"));
  mySerial.println(F("</body>"));                                      // this ends the HTML **BODY** area
  mySerial.println(F("</html>"));                                      // this ends the HTML document
  mySerial.println();                                                  // print an extra line to the browser !!!!! HOW DO WE CLOSE THE CONNECTION??? !!!!!

}

void processPortCheckbox(int pin, String portTag, String portText) {            // this function compares the port status and request type to feed HTML checkboxes into the body

  bool portStatus;                                                  // this is used as a "desired port status" flag, and is logically set

  SoftwareSerial mySerial (softSerialRX, softSerialTX);             // reinitialize the serial connection the the WIFI chip
  mySerial.begin(serialBaud);

  if (tmpString.indexOf(F("/ ")) != -1) {                           // if the parsed request is blank, let the debugger know its a new client
    Serial.println(F("Hardware Based Reply"));
    if (digitalRead(pin) == HIGH) {                                 // it is a new client, so set the checkbox status according to the hardware
      portStatus = 1;
    }
    else {
      portStatus = 0;
    }
  }

  if (tmpString.indexOf(F("/?")) != -1) {                           // if the parsed response is not blank, let the debugger know its an old client
    Serial.println(F("Software Based Reply"));
    if (tmpString.indexOf(portTag) != -1) {                         // it is an old client, so set the checkbox status according to the parsed request
      portStatus = 1;
    }
    else {
      portStatus = 0;
    }
  }

  Serial.print(portTag);                                            // print the HTML resource tag to the debugger, along with its parsed location
  Serial.print(F(" Index Pos: "));
  Serial.println(tmpString.indexOf(portTag));

  Serial.print(portTag);                                             // sends the HTML tag and the portStatus flag to the debugger
  Serial.print(F(" status "));
  Serial.println(portStatus);

  mySerial.print(F("<p>Click to switch "));                          // prints an HTML line containing the description of the port
  mySerial.print(portText);
  mySerial.println(F(" on and off.</p>"));

  if (portStatus == 1) {                                              // set the pin status high if the port status flag is set, then print
    digitalWrite(pin, HIGH);                                          // a checked checkbox with HTML for the webpage response
    mySerial.print(F("<input type=\"checkbox\" name=\""));
    mySerial.print(portTag);
    mySerial.print(F("\" onclick=\"submit();\" checked/>"));
    mySerial.println(portText);
  }
  else {                                                              // otherwise (fail-safe) set the pin status low, then print
    digitalWrite(pin, LOW);                                           // an unchecked checkbox with HTML for the webpage response
    mySerial.print(F("<input type=\"checkbox\" name=\""));
    mySerial.print(portTag);
    mySerial.print(F("\" onclick=\"submit();\">"));
    mySerial.println(portText);
  }
}

void processPortStatus(int pin, String resource, String highString, String lowString) {

  SoftwareSerial mySerial (softSerialRX, softSerialTX);              // reinitialize the serial connection the the WIFI chip
  mySerial.begin(serialBaud);

  mySerial.print(resource);                                          // print the resource name in HTML and craft the intermediate text
  mySerial.print(F(" is: "));

  if (digitalRead(pin) == HIGH) {                                    // if the pin reads HIGH, then print the highString text as status in HTML
    mySerial.print(highString);
    mySerial.println(F("<br>"));
  }
  else {                                                             // if the pin does not read HIGH, then print the lowString text as status in HTML
    mySerial.print(lowString);
    mySerial.println(F("<br>"));
  }
}

void updateScreen(int upTime, int lineCounter, int sentCounter) {
  // graphic commands to redraw the complete screen should be placed here
  u8g.setFont(u8g_font_10x20);
  u8g.drawStr( 0, 13, F("**SYSTEM ON**"));
  u8g.setPrintPos(0, 29);
  u8g.print(F("Time: "));
  u8g.println(upTime);
  u8g.setPrintPos(0, 43);
  u8g.print(F("Ln/Pg:"));
  u8g.print(lineCounter);
  u8g.print(F("/"));
  u8g.println(sentCounter);
  u8g.setPrintPos(0, 60);
  u8g.print(F("R:"));
  if (digitalRead(linkPortUSR) == HIGH) {                                    // if the pin reads HIGH, then print the highString text as status in HTML
    u8g.print(F("NO "));
  }
  else {                                                             // if the pin does not read HIGH, then print the lowString text as status in HTML
    u8g.print(F("OK "));
  }
  u8g.print("L:");
  if (digitalRead(readyPortUSR) == HIGH) {                                    // if the pin reads HIGH, then print the highString text as status in HTML
    u8g.print(F("NO"));
  }
  else {                                                             // if the pin does not read HIGH, then print the lowString text as status in HTML
    u8g.print(F("OK"));
  }
}

void loadingScreen() {

  u8g.setFont(u8g_font_10x20);

  digitalWrite(resetLineUSR, HIGH);

  u8g.firstPage();
  do {
    u8g.drawStr( 0, 13, F("*PLEASE WAIT*"));
    u8g.setPrintPos(0, 29);
    u8g.print("SYS INIT...");
  } while ( u8g.nextPage() );

  delay(5000);
  digitalWrite(resetLineUSR, LOW);

  u8g.firstPage();
  do {
    u8g.drawStr( 0, 13, F("***LOADING***"));
    u8g.setPrintPos(0, 29);
    u8g.print(F("SYS INIT OK"));
    u8g.setPrintPos(0, 43);
    u8g.print(F("USR RESET..."));
  } while ( u8g.nextPage() );

  delay(1750);
  digitalWrite(resetLineUSR, HIGH);

  u8g.firstPage();
  do {
    u8g.drawStr( 0, 13, F("***LOADING***"));
    u8g.setPrintPos(0, 29);
    u8g.print(F("SYS INIT OK"));
    u8g.setPrintPos(0, 43);
    u8g.print(F("USR RESET OK"));
    u8g.setPrintPos(0, 60);
    u8g.print(F("USR STARTING"));
  } while ( u8g.nextPage() );

  delay(1000);

  u8g.firstPage();
  do {
    u8g.drawStr( 0, 13, F("SYS INIT DONE"));
    u8g.setPrintPos(0, 29);
    u8g.print(F("SYS INIT OK"));
    u8g.setPrintPos(0, 43);
    u8g.print(F("USR RESET OK"));
    u8g.setPrintPos(0, 60);
    u8g.print(F("USR STARTED"));
  } while ( u8g.nextPage() );

  delay(1000);

}

