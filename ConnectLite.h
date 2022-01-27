/**
 * ConnectLite.h - Library for Connect devices
 *                 using bare ESP boards rather than
 *                 complete Kniwwelinos
 * Created by Jonathan Sanderson, Jan 2022
 * Northumbria University
 * See https://nustem.uk/connect
 * TODO: License
 *
 */

#ifndef ConnectLite_h
#define ConnectLite_h
#include <ConnectServo.h>
#include <SoftwareSerial.h>
#include <ESP8266WiDFi.h>
#include <PubSubClient.h>

#define WIFI_ON 1
#define VERSION "0.04" // Forked from ConnectLib 2022-01-21
#define NUMBER_OF_MOODS 6

// Function prototypes
static void messageReceived(String &t, String &p);
int getMoodIndexFromString(String moodString);
void change_mood(bool);
void publish_mood();
void handleButtons();
void checkMood();
void connectSetup();
void setInverted(bool);
// Mood prototypes (all declared weak so they can be overridden)
void doHappy();
void doSad();
void doHeart();
void doSkull();
void doSilly();
void doDuck();
void receivedMoodWiggleAnimation();
// Main event loop
void connectLoop();

// void updateConnectServos();

/**
 * Mood structures and supporting scafolding
 */
// We need a function pointer type to include in our Mood struct
typedef void (*CallbackFunction)(void);
// ...and the struct itself
typedef struct {
    int index;
    String text;
    String icon;
    CallbackFunction callback;
} Mood;

// We need to keep track of the number of moods manually, because C
// ...and this has to be const for the array declaration to work
// ...and we need the array declaration here because we want moods[]
// to be global
// extern Mood moods[NUMBER_OF_MOODS];   // Initialiser has to be inside setup(), or bad things happen.

extern Mood myMood;        // Should really be intrinsicMood or internalMood
extern Mood extrinsicMood; // The current network mood. Should be externalMood?
extern Mood performedMood; // The current displayed mood

extern String received_string;
// static void messageReceived (String &t, String &p);
// void handleButtons();
// void checkMood();

extern ServoMessenger ConnectMessenger;

// Setting up for direct serial control
// extern bool isSerialZombie;
// extern String received;
// extern char incomingChar;
#define RX_PIN D2 // would use D2 on a D1 Mini
#define TX_PIN D1 // would use D1 on a D1 Mini

#endif

