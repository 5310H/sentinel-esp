#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>

// --- System Capacity ---
#define MAX_ZONES   32
#define MAX_USERS   10
#define MAX_RELAYS  8
#define MAX_RULES   20

// --- Runtime Enums ---

typedef enum {
    ARMSTATE_DISARMED,
    ARMSTATE_STAY,
    ARMSTATE_AWAY,
    ARMSTATE_NIGHT,
    ARMSTATE_VACATION
} ArmState_t;

typedef enum {
    ALARM_IDLE,         // Normal operation
    ALARM_EXITING,      // Exit delay active (Buzzer 1Hz)
    ALARM_PENDING,      // Entry delay active (Buzzer 2Hz)
    ALARM_TRIGGERED,    // Active alarm (Siren ON, Noonlight Sent)
    ALARM_CANCELLED,    // Silenced, waiting for reset
    ALARM_TROUBLE       // Hardware/Comms failure
} AlarmStatus_t;

// --- Data Structures ---

typedef struct {
    char AccountID[64];
    int PIN;
    char Name[64];
    char Address1[64];
    char Address2[64];
    char City[32];
    char State[16];
    char ZipCode[12];
    char Email[64];
    char Phone[20];
    char Instructions[128];
    double Latitude;
    double Longitude;
    double Accuracy;
    char MonitorServiceID[64];
    char MonitorServiceKey[128];
    char MonitoringURL[128];
    char Notify[16];         // none, email, telegram, service
    bool MonitorFire;
    bool MonitorPolice;
    bool MonitorMedical;
    bool MonitorOther;
    char SMTPServer[64];
    int SMTPPort;
    char SMTPUser[64];
    char SMTPPass[64];
    char MQTTServer[64];
    int MQTTPort;
    char MQTTUser[64];
    char MQTTPass[64];
    char TelegramID[32];
    char TelegramToken[128];
    bool TelegramEnabled;
    char NVRServerURL[128];
    char HAIntegrationURL[128];
    int EntryDelay;
    int ExitDelay;
    int CancelDelay;
} Owner_t;

typedef struct {
    int ID;
    char Name[32];
    char PIN[8];
    char Phone[20];
    char Email[64];
    char Notify[16];
    bool IsAdmin;
} User_t;

typedef struct {
    int ID;
    char Name[32];
    char Description[64];
    char Type[16];           // fire, police, medical, other
    char Location[32];
    char Model[32];
    char Manufacturer[32];
    bool Chime;
    bool AlarmOnArmedOnly;   // false = 24-hour (Smokes/Panic)
    int PinNumber;
    bool IsI2C;
    uint8_t I2CAddress;
    bool IsPerimeter;
    bool IsInterior;
    bool IsPanic;
} Zone_t;

typedef struct {
    int ID;
    char Name[32];
    char Description[64];
    int DurationSec;
    char Location[32];
    char Type[16];           // siren, strobe, light, lock
    bool Repeat;
    int PinNumber;
} Relay_t;

typedef struct {
    int ID;
    char Name[32];
    bool Enabled;
    int TriggerZoneID;       // Link to Zone_t.ID
    char TriggerCondition[16]; // "open", "closed"
    ArmState_t RequiredState; // Only run if in this state
    int ActionRelayID;       // Link to Relay_t.ID
    int ActionType;          // 0:Toggle, 1:Momentary
    int DurationSec;
} Rule_t;

#endif