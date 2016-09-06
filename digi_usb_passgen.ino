#include "DigiKeyboard.h"
#include "EEPROM.h"
#include "config.h"

#define P0 0
#define P1 1

// command
enum {
    GENERATE = 0,
    DUMP_ALL = 1,
    RESET = 2,
};
const char *CMD_STR[] = { "new %u", "dmp", "rst" };

// mode selection
enum {
    MODE_CHANGE_FOR_SLOT_3 = 0,
    ACTION_CONFIRM = 3,
};

#define MAX_LENGTH 16
#define PWD_LENGTH 12
#define SLOT_COUNT 4

char password[MAX_LENGTH * SLOT_COUNT];

inline uchar randChar() {
    do {
        uchar counter = random(62);

        if (counter < 26) {
            return 'a' + counter;
        } else if(counter < 52) {
            return 'A' + counter - 26;
        } else if(counter < 62) {
            return '0' + counter - 52;
        }
    } while(1);
}

char buf[64];

inline void generate_password(uchar slot) {
    char *p = password + (MAX_LENGTH * slot);
    memset(p, 0, sizeof(char) * MAX_LENGTH);

    for (uchar i = 0; i < PWD_LENGTH; i += 1) {
        *(p + i) = randChar();
    }

    DigiKeyboard.println(p);

    for (uchar i = 0; i < MAX_LENGTH; i += 1) {
        EEPROM.write((MAX_LENGTH * slot) + i, *(password + (MAX_LENGTH * slot) + i));
    }
}

inline void dump_all() {
    for (uchar slot = 0; slot < SLOT_COUNT; slot += 1) {
        snprintf(buf, sizeof(buf), "%d: %s", slot, password + (MAX_LENGTH * slot));
        DigiKeyboard.println(buf);
    }
}

inline void reset_eeprom() {
    for (uchar slot = 0; slot < SLOT_COUNT; slot += 1) {
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf), "%s", initial[slot]);

        for (uchar i = 0; i < MAX_LENGTH; i += 1) {
            EEPROM.write(MAX_LENGTH * slot + i, *(buf + i));
        }
    }
}

inline uchar readInput() {
    uchar x = 0;

    x |= (digitalRead(P0) & 1);
    x |= ((digitalRead(P1) & 1) << 1);
    x &= 0xff;

    return x;
}

uchar select_slot = 0;
bool need_mode_change = false;

void setup() {
    pinMode(P0, INPUT);
    pinMode(P1, INPUT);

    for (uchar i = 0; i < MAX_LENGTH * SLOT_COUNT; i += 1) {
        *(password + i) = EEPROM.read(i);
    }

    DigiKeyboard.sendKeyStroke(0);
    DigiKeyboard.delay(2000);

    select_slot = readInput();
    if (select_slot == 3) {
        need_mode_change = true;
    }
    DigiKeyboard.println(password + (MAX_LENGTH * select_slot));
    randomSeed(millis());
}


void loop() {
    if (need_mode_change) {
        // until all switch off
        do {
            DigiKeyboard.delay(100);
        } while (readInput() != MODE_CHANGE_FOR_SLOT_3);
        need_mode_change = false;
    }

    // until all switch on
    do {
        DigiKeyboard.delay(100);
    } while (readInput() != ACTION_CONFIRM);

    DigiKeyboard.println("CMD Mode 0:gen 1:dmp 2:rst");

    uchar mode;
    do {
        mode = readInput();
        DigiKeyboard.delay(3000);
    } while (mode == ACTION_CONFIRM);


    snprintf(buf, sizeof(buf), CMD_STR[mode], select_slot);
    DigiKeyboard.println(buf);

    DigiKeyboard.println("plz confirm");
    DigiKeyboard.delay(3000);

    if (readInput() != ACTION_CONFIRM) {
        goto cancel;
    }

    need_mode_change = true;
    switch(mode) {
        case GENERATE:
            generate_password(select_slot);
            break;
        case RESET:
            reset_eeprom();
            break;
        case DUMP_ALL:
            dump_all();
            break;
    }
    DigiKeyboard.println("done");
    goto nextloop;

cancel:
    DigiKeyboard.println("canceled");
nextloop:
    DigiKeyboard.delay(100);
}
