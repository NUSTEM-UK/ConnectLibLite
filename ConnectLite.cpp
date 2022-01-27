/* Connect.ino - bundle of helper functions for Connected devices,
originally built on top of Kniwwelino. This version rolls its own
MQTT code, and omits Kniwwelino-specific calls.

Jonathan Sanderson, Northumbria University, July 2021
TODO: License
*/

#include "ConnectLite.h"
#include "arduino_secrets.h"

// WiFi setup
static const char* ssid = "connect";
static const char* password = "nustem123";
// Username & pass in arduino_secrets.h


// Initialise the mood array. Doesn't work correctly if I move this to
// above setup() (first string is truncated).
// To add moods, append them to this array, and adjust NUMBER_OF_MOODS
// accordingly.
Mood moods[NUMBER_OF_MOODS];

// Start happy, because we're optimistic about the world
Mood myMood = moods[0];
Mood extrinsicMood = moods[0];
Mood performedMood = moods[0];

bool isInverted = false; // Retained for compatibility, though not used
bool moodReceived = false; // Has a new mood been received?

String received_string = "";
SoftwareSerial myPort(RX_PIN, TX_PIN, false, 256);
bool isSerialZombie = false;
String received;
char incomingChar = 0;
int receivedMoodAnimationRate = 75;


static void messageReceived(String &topic, String &payload) {
    // FIXME: probably shouldn't have a Serial operation in a callback
    Serial.println(F("---> MESSAGE RECEIVED"));

    if (topic=="MESSAGE") {
        received_string = payload;
    } else if (topic=="MOOD") {
        //   Serial.println(F("Got a mood"));
        int tempIndex = getMoodIndexFromString(payload);
        if (tempIndex != -1) {
            extrinsicMood = moods[tempIndex];
        }
        // Set flag that mood has been received
        moodReceived = true;
    }
}


int getMoodIndexFromString (String moodString) {
    int foundMoodIndex = -1; // Initialise
    int i = 0;
    // Serial.print(F("Checking for index for Mood: "));
    // Serial.println(moodString);

    while (i < NUMBER_OF_MOODS) {
        // This is ugly because we always loop through the entire array,
        // but the obvious alternatives seem worse
        // TODO: find an obvious alternative that isn't worse.
        // TODO: it's to set a flag when we find the mood, and then
        //       return the index. Loop tests the flag.
        //       ...but still need to ensure we don't go beyond the
        //       end of the array.
        //       ...so maybe leave this as-is. It seeems to work fast enough.
        // Serial.print(F("Stepping through: "));
        // Serial.print(i);
        // Serial.print(moods[i].text);

        if (moodString == moods[i].text) {
            // Serial.print(F("  Found a match at "));
            // Serial.println(i);
            foundMoodIndex = i;
        } else {
            // Serial.print(F("  Did not find a match at "));
            // Serial.println(i);
        }
        i++;
    }
    // Will return match or -1 if no match
    if (foundMoodIndex != -1) {
        // Serial.print(F("Got a matching mood: "));
        // Serial.println(moods[foundMoodIndex].text);
    }

    return foundMoodIndex;
}


void change_mood() {
    // Increment mood. Note that we can't just increment
    // myMood.index, that would break the connection to the
    // other data in the struct
    int tempMoodIndex = myMood.index;
    tempMoodIndex++;
    // Loop around moods (self-adjusting for number of moods.)
    // Note the obvious off-by-one error here, which took me
    // much longer to spot than now seems reasonable.
    // 2022-01-17: edited to skip over DUCK mood. Still in there,
    //             but no longer accessible.
    if (tempMoodIndex > (NUMBER_OF_MOODS-2)) {
        tempMoodIndex = 0;
    }
    Serial.println(F(">>>Changing mood"));
    Serial.print(F(">>>New mood is: "));
    Serial.println(moods[tempMoodIndex].text);
    // Display our new internal mood
    myMood = moods[tempMoodIndex];
    Kniwwelino.MATRIXdrawIcon(myMood.icon);
    Kniwwelino.sleep((unsigned long) 1000);
    Serial.println(F(">>>Returning to extrinsicMood"));
    // Now return to the network mood
    Kniwwelino.MATRIXdrawIcon(extrinsicMood.icon);
}


void publish_mood() {
    // FIXME:: None of this left!

    // Publish our mood to the network
    // Arrow direction depends on whether we're inverted or not
    // if (isInverted) {
    //     Kniwwelino.MATRIXdrawIcon(ICON_ARROW_DOWN);
    // } else {
    //     Kniwwelino.MATRIXdrawIcon(ICON_ARROW_UP);
    // }
    // Kniwwelino.sleep((unsigned long)500);
    // #if WIFI_ON
    // Kniwwelino.MQTTpublish("MOOD", myMood.text); // May need to reorder this
    // #else
    // extrinsicMood = myMood;
    // // network_mood = String(my_icon);
    // #endif
}

void handleButtons() {
    // FIXME: None of this left!
    // if (Kniwwelino.BUTTONAclicked()) {
    //     Serial.println(F(">>>BUTTON press: A"));
    //     // By default, change the mood. If we're inverted, publish instead.
    //     isInverted ? publish_mood() : change_mood();
    // }
    // if (Kniwwelino.BUTTONBclicked()) {
    //     Serial.println(F(">>>BUTTON press: B"));
    //     // By default, publish the mood. If we're inverted, change it instead.
    //     isInverted ? change_mood() : publish_mood();
    // }
}

void checkMood() {
    // Serial.println("Mood check");

    // Play the received animation
    // receivedMoodWiggleAnimation();
    // Reassign mode variables
    performedMood = extrinsicMood;
    myMood = extrinsicMood;
    // Unset the received flag (NB. doing this after playing the animation, which is blocky)
    moodReceived = false;
    // Call the associated action function
    performedMood.callback();
    // Set the icon dipslay
    // Kniwwelino.MATRIXdrawIcon(performedMood.icon);

}

void checkSerialConnection() {
    // Has anything arrived over the software serial port?
    // Serial.println(F("Checking serial connection"));
    if (myPort.available() > 0) {
        // Read incoming and append to string
        Serial.println(F("Read character"));
        incomingChar = myPort.read();
        received += incomingChar;

        // Check if we've received a newline terminator
        if (incomingChar == '\n'){
            // Trim the closing newline
            received.trim();
            // Now parse the received string, looking for ACK
            if (received == F("ACK")) {
                Serial.println(received);
                Serial.println(F(" >>> CEDING CONTROL"));

                Kniwwelino.sleep(80); // Can be to fast for the Pico to handle?
                // Respond in kind
                myPort.write("ACK ACK ACK");
                // Turn off Connect mood messaging stuff
                // TODO: Check if this can be F'd. Tripped a compiler error when I tried.
                Kniwwelino.MQTTunsubscribe("MOOD");
                // Display hint we're in zombie mode
                // 1 1 1 1 1
                // 0 0 0 1 0
                // 0 0 1 0 0
                // 0 1 0 0 0
                // 1 1 1 1 1
                Kniwwelino.MATRIXdrawIcon(F("B1111100010001000100011111"));
                Kniwwelino.MATRIXsetBlinkRate(MATRIX_BLINK_1HZ);
                // Flag that we're now a serial zombie
                isSerialZombie = true;

                // SShort pause, then clear the capture string
                Kniwwelino.sleep(500);
                received = "";
                // Stop the I2C bus
                // Kniwwelino.bgI2CStop(); // Removed because it's no better this way, and resets fail a bit
            } else {
                Serial.println(received);
                Serial.println(F("Missed ACK, reset and try again"));
                // Reset the capture string
                received = "";
            }
        }
    }
}

void parseSerialConnectionAndDriveDevices() {
    // Get commands
    if (myPort.available() > 0) {
        // read incoming and append to capture string
        incomingChar = myPort.read();
        received += incomingChar;

        // Check if we have a newline terminator
        if (incomingChar == '\n') {
            // Remove that closing newline
            received.trim();
            // Now send the received string to the ConnectMessenger object for handling
            ConnectMessenger.serialCommand(received);
            // Reset the capture string
            received = "";
        }
    }
}

void receivedMoodWiggleAnimation() {
    // Display a wiggle animation to indicate a mood has been received
    // TODO: This is horrid code, rewrite it so less horrid
    for (int i = 0; i < 2; i++) {
        Kniwwelino.MATRIXdrawIcon(String("B0000000000100000000000000"));
        Kniwwelino.sleep((unsigned long) receivedMoodAnimationRate);
        Kniwwelino.MATRIXdrawIcon(String("B0000000000110000000000000"));
        Kniwwelino.sleep((unsigned long) receivedMoodAnimationRate);
        Kniwwelino.MATRIXdrawIcon(String("B0000000000011000000000000"));
        Kniwwelino.sleep((unsigned long) receivedMoodAnimationRate);
        Kniwwelino.MATRIXdrawIcon(String("B0000000000001100000000000"));
        Kniwwelino.sleep((unsigned long) receivedMoodAnimationRate);
        Kniwwelino.loop();
        Kniwwelino.MATRIXdrawIcon(String("B0000000000000110000000000"));
        Kniwwelino.sleep((unsigned long) receivedMoodAnimationRate);
        Kniwwelino.MATRIXdrawIcon(String("B0000000000000010000000000"));
        Kniwwelino.sleep((unsigned long) receivedMoodAnimationRate);
        Kniwwelino.loop();
        Kniwwelino.MATRIXdrawIcon(String("B0000000000000110000000000"));
        Kniwwelino.sleep((unsigned long) receivedMoodAnimationRate);
        Kniwwelino.MATRIXdrawIcon(String("B0000000000001100000000000"));
        Kniwwelino.sleep((unsigned long) receivedMoodAnimationRate);
        Kniwwelino.loop();
        Kniwwelino.MATRIXdrawIcon(String("B0000000000011000000000000"));
        Kniwwelino.sleep((unsigned long) receivedMoodAnimationRate);
        Kniwwelino.MATRIXdrawIcon(String("B0000000000110000000000000"));
        Kniwwelino.sleep((unsigned long) receivedMoodAnimationRate);
        Kniwwelino.loop(); // do background stuff...
    }

}

void connectSetup() {
    Kniwwelino.begin("Connected_Device", WIFI_ON, true, false); // Wifi=true, Fastboot=true, MQTT Logging=false
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("-------"));
    Serial.println(F("START " __FILE__ "\r\nVersion " VERSION " from " __DATE__));
    Serial.println(F("-------"));
    Serial.println(F("MQTT server overridden in custom KniwwelinoLib"));
    Serial.println(DEF_MQTTSERVER);
    Serial.println(DEF_MQTTPORT);
    Serial.println(DEF_MQTTUSER);
    Serial.println(F("---------"));

    #if WIFI_ON
    Kniwwelino.MQTTsetGroup(String("NUSTEM"));
    Kniwwelino.MQTTonMessage(messageReceived);
    Kniwwelino.MQTTsubscribe(F("MESSAGE"));
    Kniwwelino.MQTTsubscribe(F("MOOD"));
    #endif

    Kniwwelino.RGBsetBrightness((int)150);
    Kniwwelino.RGBclear();
    #if WIFI_ON
    Kniwwelino.MQTTpublish(F("hello_my_name_is"), String(Kniwwelino.getMAC()));
    #endif
    Kniwwelino.sleep(1000);

    // Check if we're inverted and assign mood icons accordingly.
    if (isInverted)
    {
        Serial.println(F(">>> INVERTED, setting icons accordingly"));
        moods[0] = {0, F("HAPPY"), F("B0111010001000000101000000"), &doHappy};
        moods[1] = {1, F("HEART"), F("B0010001110111111111101010"), &doHeart};
        moods[2] = {2, F("SAD"),   F("B1000101110000000101000000"), &doSad};
        moods[3] = {3, F("SKULL"), F("B0111001110111111010101110"), &doSkull};
        moods[4] = {4, F("SILLY"), F("B0001100011111110000001010"), &doSilly};
        moods[5] = {5, F("DUCK"),  F("B0000001110011111110001100"), &doDuck};
    } else {
        moods[0] = {0, F("HAPPY"), F("B0000001010000001000101110"), &doHappy};
        moods[1] = {1, F("HEART"), F("B0101011111111110111000100"), &doHeart};
        moods[2] = {2, F("SAD"), F("B0000001010000000111010001"), &doSad};
        moods[3] = {3, F("SKULL"), F("B0111010101111110111001110"), &doSkull};
        moods[4] = {4, F("SILLY"), F("B0101000000111110001100011"), &doSilly};
        moods[5] = {5, F("DUCK"), F("B0110011100011110111000000"), &doDuck};
    }


    // Cross-check that we have moods correctly.
    for (size_t i = 0; i < NUMBER_OF_MOODS; i++) {
        Serial.print(moods[i].text);
        Serial.print(F(" : "));
    }

    // Set up software serial connection for trainer bot
    myPort.begin(57600);
    if (!myPort) {
        Serial.println(F("Invalid SoftwareSerial config"));
        // Keep going, assume we never want serial. Ulp.
    }
    Serial.println();
    Serial.println(F("SoftwareSerial started"));

    // Confirm we're not currently zombied
    isSerialZombie = false;

    // Display a startup mood
    // For some reason we need the animation first to ensure the display works.
    receivedMoodWiggleAnimation();
    Kniwwelino.MATRIXdrawIcon(moods[0].icon);

    Serial.println();

}

void connectLoop() {
    // Update Kniwwelino stuff
    Kniwwelino.loop();

    // Everything else depends on serial zombie state
    if (isSerialZombie) {
        // We *are* a serial zombie, so do as bidden
        parseSerialConnectionAndDriveDevices();
    } else {
        // Do normal things
        handleButtons();
        ConnectMessenger.updateServos();
        // ...and check the SoftwareSerial port in case we're being zombied
        checkSerialConnection();
        // debugSerialConnection();
        // Check if the mood has changed
        if (moodReceived) {
            // It has, so update our performed mood accordingly
            checkMood();
        }

    }
}

/**
 * ACTION STUBS
 * These are all defined weak so they can be overriden
 * in the user sketch. Ah, C - there's always a way.
 * (virtual methods in a class might be preferable,
 * but this works for now.)
*/

__attribute__ ((weak))
void doHappy() {
    Serial.println(F("New mood received: HAPPY"));
    // TODO: Default waving behaviour here?
    // servos_engage();
    // servo1Speed = 100;)
    // for (int i = 0; i < 3; i++) {
    //     Servo1.startEaseTo(180, servo1Speed, true);
    //     Kniwwelino.RGBsetColorEffect(String("00FF00"), RGB_BLINK, -1);
    //     while (Servo1.isMovingAndCallYield()) {
    //         // Nothing here, intentionally
    //     }
    //     Servo1.startEaseTo(0, servo1Speed, true);
    //     Kniwwelino.RGBsetColorEffect(String("FF0000"), RGB_GLOW, -1);
    //     while (Servo1.isMovingAndCallYield()) {
    //         // Nothing here, intentionally
    //     }
    // }
    // Kniwwelino.RGBclear(); // Turn the LED off.
    // servos_disengage();
}

__attribute__ ((weak))
void doSad() {
    Serial.println(F("New mood received: SAD"));
}

__attribute__ ((weak))
void doHeart() {
    Serial.println(F("New mood received: HEART"));
}

__attribute__ ((weak))
void doSkull() {
    Serial.println(F("New mood received: SKULL"));
}

__attribute__ ((weak))
void doSilly() {
    Serial.println(F("New mood received: SILLY"));
}

__attribute__ ((weak))
void doDuck() {
    Serial.println(F("New mood received: DUCK"));
}

void setInverted(bool inverted) {
    isInverted = inverted;
    if (isInverted)
    {
        Serial.println(F(">>> INVERTED, setting icons accordingly"));
        moods[0] = {0, F("HAPPY"), F("B0111010001000000101000000"), &doHappy};
        moods[1] = {1, F("HEART"), F("B0010001110111111111101010"), &doHeart};
        moods[2] = {2, F("SAD"),   F("B1000101110000000101000000"), &doSad};
        moods[3] = {3, F("SKULL"), F("B0111001110111111010101110"), &doSkull};
        moods[4] = {4, F("SILLY"), F("B0001100011111110000001010"), &doSilly};
        moods[5] = {5, F("DUCK"),  F("B0000001110011111110001100"), &doDuck};
    } else {
        Serial.println(F(">>> NOT INVERTED, setting icons accordingly"));
        moods[0] = {0, F("HAPPY"), F("B0000001010000001000101110"), &doHappy};
        moods[1] = {1, F("HEART"), F("B0101011111111110111000100"), &doHeart};
        moods[2] = {2, F("SAD"), F("B0000001010000000111010001"), &doSad};
        moods[3] = {3, F("SKULL"), F("B0111010101111110111001110"), &doSkull};
        moods[4] = {4, F("SILLY"), F("B0101000000111110001100011"), &doSilly};
        moods[5] = {5, F("DUCK"), F("B0110011100011110111000000"), &doDuck};
    }
    // redisplay current mood
    // Kniwwelino isn't necessarily alive yet.
    // Kniwwelino.MATRIXdrawIcon(performedMood.icon);
}
