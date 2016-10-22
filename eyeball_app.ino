/*
 * EyeballKit App Example
 * Description: Communicate with EyeballKit App(Android Platform only) through the websocket protocol
 * Instructions:
  - Update WiFi SSID and password as necessary.
  - Flash this sketch to the ESP8266 board
  - Install EyeballKit App on your Android phone, download it from: http://d.naozhendang.com/eyeballkit.apk
  - Controll the kit with the installed EyabllKit App
 * Author: naozhendang.com
 * More info: http://www.naozhendang.com/p/eyeball-kit
 * Tutorial: http://www.naozhendang.com/t/eyeball-app
 */
 
#include <Arduino.h>
#include <Servo.h> 

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WebSocketsClient.h>
#include <Hash.h>

/**************************************
 * !!!!!Config Here!!!
 * ***********************************/
const char* ssid     = "SSID NAME";         //Your wifi ssid
const char* password = "SSID PASSWORD";  //Your wifi password

char hostString[16] = {0};

Servo myservo_v;  // create servo object to control the vertical servo 
Servo myservo_h;  // create servo object to control the horizontal servo 

#define USE_SERIAL Serial1

WebSocketsClient webSocket;

char* x = NULL;
char* y = NULL;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t lenght) {


    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[WSc] Disconnected!\n");
            break;
        case WStype_CONNECTED:
            {
                USE_SERIAL.printf("[WSc] Connected to url: %s\n",  payload);

                // send message to server when Connected
                webSocket.sendTXT("{\"WSc\":\"Connected\"}");
            }
            break;
        case WStype_TEXT:
            USE_SERIAL.printf("[WSc] get text: %s\n", payload);

            x = value_pointer("x",(char*) &payload[0]);
            y = value_pointer("y",(char*) &payload[0]);
            myservo_h.write(map(atoi(x),31,269,45,135));
            myservo_v.write(map(atoi(y),31,269,45,135));
            break;
        case WStype_BIN:
            USE_SERIAL.printf("[WSc] get binary lenght: %u\n", lenght);
            hexdump(payload, lenght);
            break;
    }

}

void setup() {
    myservo_h.attach(5);  // attaches the servo on GPIO5/D1
    myservo_v.attach(4);  // attaches the servo on GPIO4/D2
    
    USE_SERIAL.begin(115200);

    USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

      for(uint8_t t = 4; t > 0; t--) {
          USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
          USE_SERIAL.flush();
          delay(1000);
      }

    sprintf(hostString, "ESP_%06X", ESP.getChipId());
    WiFi.begin(ssid,password);

    //WiFi.disconnect();
    while(WiFi.status() != WL_CONNECTED) {
        delay(100);
    }

    //webSocket.begin("eyeball.local", 8888);
    char* serverIP = resolveMDNS("eyeball");
    if(serverIP != NULL){
      webSocket.begin(serverIP, 8888);
      webSocket.onEvent(webSocketEvent);
    }else{
      //No server found
      USE_SERIAL.printf("[mNDS] NO server found\n");
    }

}

void loop() {
    webSocket.loop();
}

/*
 * Helper Functions
 */

// Give this a pointer to some JSON data and it will
// return the length of that JSON.  
int json_length(char* json) {
  if (json == 0) {
    return 0;    // Null pointer
  }
  
  if (json[0] != '{') {
    return 0;    // Not JSON  
  }
  // Now that we know we have a JSON object, we defer
  // the actual calculation to value_length
  return value_length(json);
}


// This is given a fragment of JSON and returns how
// many characters it contains. This fragment might
// be an object, a number, a string , etc.
int value_length(char* json) {
  if (json == 0) {
    return 0;    // Null pointer
  }
  
  // Switch over each possibility
  int index = 0;
  switch (json[index]) {
    case '{':
        // This is a JSON object. Find the matching '}'
        do {
          index++;    // Read ahead
          if (json[index] == '"') {
            // Skip strings, as they may contain unwanted '}'
            index = index + value_length(json+index);
          }
          if (json[index] == '{') {
            // Recurse past nested objects
            index = index + value_length(json+index);
          }
        } while (json[index] != '}');
        return index + 1;    // Include the '{' and '}' in the length
    case '"':
      // This is a string. Scan ahead to the first unescaped '"'
      do {
        if (json[index] == '\\') {
          index++; // Skip escaped quotes
        }
        index++;    // Read ahead
      } while (json[index] != '"');
      return index+1;    // Include the quotes in the string's length
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
      // We're a number. Loop forever until we find a non-number character.
      // Note, this is a simplistic parser that is equivalent to the regex  
      // [0123456789-][0123456789.eE]* This allows malformed numbers like
      // 0.0.0.0e0.0e.0
      do {
        index++;
        switch (json[index]) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
          case '.':
          case 'e':
          case 'E':
            break;    // Numeric
          default:
            return index;    // Non-numeric. Stop counting.
        }
      } while (1);
    default:
      // Unknown. Ignore it.
      return 0;
  }
}

// Takes a JSON string and looks for any commands it
// contains. These are "key":value pairs, which are
// sent as arguments to the "command" function as
// they are encountered.
char* value_pointer(char* key, char* json) {
  int length = json_length(json);
  int index = 0;    // Used to loop through the contents
  int temp;    // Our parsing uses lookahead, this stores how far we've gone
  char* value = 0;
  
  // Only bother doing something if json has some contents.
  // When this condition is false, it's essentially the
  // escape clause of our recursion.
  if (length > 2) {    // 2 == empty, since we have '{' and '}'
    index++;    // Skip past the '{' to get at the contents
    while (index < length) {
      switch (json[index]) {
        case ' ':
          // Whitespace is insignificant
          index++;
          break;
        case '{':
          // We have an object in an object, let's recurse
          value_pointer(key, json+index);
          index = index + json_length(json+index);
          break;
        case '"':
          // A string. This should be part of a key:value pair
          if (index + 2 >= length) {
            // JSON can't end with an opening quote. Bail out.
            break;
          }

          // Look one character ahead, then keep going until
          // we find our matching close quote
          temp = index+1;
          while ((json[temp] != '"') && (temp < length)) {
            // We've not found our close quote, so look ahead
            if (json[temp] == '\\') {
              // Increment twice to skip over escaped characters
              temp++;
            }
            temp++;
          }
          if (temp >= length-2) {
            // We've reached the end of the JSON without finding
            // a close quote. Bail out.
            break;
          }   
          // Now we've read our name, find our associated value
          temp++;    // It must start after the close quote
          while ((json[temp] == ' ') && (temp < length)) {
            temp++;    // Skip whitespace
          }
          if (json[temp] != ':') {
            // We must have a colon between the name and the value
            // Bail out if not
            break;
          }
          temp++;    // We don't need the colon, skip it
          while ((json[temp] == ' ') && (temp < length)) {
            temp++;    // Skip whitespace
          }
          
          // Wherever we are, we must have found our value
          // Tell run_command what we've found
          //run_command(json+index, json+temp);
      if (compare_strings(json+index,key)) {
        value = json+temp;    // Read pin values
      }
      
          
          // Now let's get our parser ready for the next value
          index = temp + value_length(json+temp);    // Skip the value
          while ((json[index] == ' ') && (index < length)) {
            index++;    // Skip whitespace
          }
          if (json[index] == ',') {
            // Skip commas between name:value pairs
            index++;
          }
          break;    // Done
        default:
          // Unknown input. Oops.
          index++;
      }
    }
  }
  return value;
}

// Compare the first character array, which is not
// null-terminated and is in quotes, to the second
// which can be null-terminated and without quotes
short compare_strings(char* string1, char* string2) {
  int first_size = value_length(string1);
  int second_size;
  for (second_size = 0; string2[second_size] != '\0'; second_size++) {
    // Do nothing. The loop parameters count the string for us.
  }
  
    // if first_size includes quotes, we don't include them
    // in our check
    if (first_size - 2 != second_size) {
    // The size is different, so the strings are different
    return 0;
    }
    
    // Now do a lexicographical comparison
    int index;
    for (index = 0; index < first_size - 2; index++) {
    if (string1[index+1] != string2[index]) {
      return 0;    // Mismatch
    }
    }

    
  // If we're here then our tests couldn't find any different
  return 1; 
}

//Locate substring
//Returns a pointer to the first occurrence of str2 in str1, 
//or a null pointer if str2 is not part of str1.
//The matching process does not include the terminating null-characters.
char* find_substring(char* string1, char* string2){
  int second_size;
  for (second_size = 0; string2[second_size] != '\0'; second_size++) {
    // Do nothing. The loop parameters count the string for us.
  }
  
  if (!second_size){
  return (char *)string1;
  }

  int first_size = value_length(string1);
  while (first_size >= second_size){
  first_size--;
  if (!memcmp(string1, string2, second_size))
            return (char *)string1;
        string1++;
  }
  return NULL;
}

char* resolveMDNS(String hostname){
  if (!MDNS.begin(hostString)) {
    return NULL;
  }

  int n = MDNS.queryService("http","tcp"); // Send out query for esp tcp services
  if (n == 0) {
    return NULL;
  }else{
    for (int i = 0; i < n; ++i) {
      // compare .local hostnames
      if(strcmp(MDNS.hostname(i).c_str(),hostname.c_str())==0){
        char* c= new char[strlen(MDNS.IP(i).toString().c_str())+1];
        strcpy(c,MDNS.IP(i).toString().c_str());
        return c;
      }
    }
    return NULL;
  }
}
