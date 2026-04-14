#include <Wire.h>
#include <VL53L0X.h>

// =============================
#define SDA_PIN 17
#define SCL_PIN 16

// =============================
class LaserSensors {
private:
    int* xshutPins;
    int count;

    VL53L0X* sensors;
    bool* sensorOK;
    uint8_t* addresses;

    // -------------------------
    void resetSensor(int i) {
        Serial.printf("[SENSOR %d] HARD RESET\n", i);

        digitalWrite(xshutPins[i], LOW);
        delay(100);
        digitalWrite(xshutPins[i], HIGH);
        delay(200);

        sensors[i].setTimeout(300);

        if (sensors[i].init()) {
            sensors[i].setAddress(addresses[i]);
            sensors[i].startContinuous();
            sensorOK[i] = true;

            Serial.printf("[SENSOR %d] RECOVERED\n", i);
        } else {
            sensorOK[i] = false;
            Serial.printf("[SENSOR %d] RESET FAILED\n", i);
        }
    }

public:

    LaserSensors(int pins[], int size) {

        count = size;

        xshutPins = new int[count];
        sensors   = new VL53L0X[count];
        sensorOK  = new bool[count];
        addresses = new uint8_t[count];

        for (int i = 0; i < count; i++) {
            xshutPins[i] = pins[i];
            sensorOK[i] = false;
            addresses[i] = 0x30 + i;
        }
    }

    // -------------------------
    void begin() {

        Serial.println("\n=== LASER SENSOR INIT ===");

        Wire.begin(SDA_PIN, SCL_PIN);
        Wire.setClock(400000);

        for (int i = 0; i < count; i++) {
            pinMode(xshutPins[i], OUTPUT);
            digitalWrite(xshutPins[i], LOW);
        }

        delay(200);

        for (int i = 0; i < count; i++) {

            Serial.printf("\n[SENSOR %d] Boot...\n", i);

            digitalWrite(xshutPins[i], HIGH);
            delay(200);

            sensors[i].setTimeout(300);

            if (!sensors[i].init()) {
                Serial.printf("[SENSOR %d] INIT FAILED\n", i);
                sensorOK[i] = false;
                continue;
            }

            sensors[i].setAddress(addresses[i]);
            sensors[i].startContinuous();

            sensorOK[i] = true;

            Serial.printf("[SENSOR %d] OK -> 0x%X\n", i, addresses[i]);
        }
    }

    // -------------------------
    int read(int i) {

        if (!sensorOK[i]) {
            resetSensor(i);
            return -1;
        }

        int d = sensors[i].readRangeContinuousMillimeters();

        if (sensors[i].timeoutOccurred()) {
            Serial.printf("[SENSOR %d] TIMEOUT\n", i);
            sensorOK[i] = false;
            return -1;
        }

        // =============================
        // NOVAS REGRAS INTELIGENTES
        // =============================

        if (d > 10000) {
            Serial.printf("[SENSOR %d] INVALID (>10000) -> RESET\n", i);
            resetSensor(i);
            return -2;
        }

        if (d > 5000) {
            Serial.printf("[SENSOR %d] UNDEFINED (>5000)\n", i);
            return -3;
        }

        return d;
    }

    // -------------------------
    void readAll() {

        for (int i = 0; i < count; i++) {

            int d = read(i);

            Serial.print("S");
            Serial.print(i);
            Serial.print(": ");

            if (d == -1) {
                Serial.print("ERR");
            }
            else if (d == -2) {
                Serial.print("RESET");
            }
            else if (d == -3) {
                Serial.print("UNDEF");
            }
            else {
                Serial.print(d);
                Serial.print(" mm");
            }

            Serial.print("\t");
        }

        Serial.println();
    }
};

// =============================
int pins[] = {5};

LaserSensors lasers(pins, sizeof(pins) / sizeof(pins[0]));

// -----------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);

    lasers.begin();
}

// -----------------------------
void loop() {
    lasers.readAll();
    delay(200);
}