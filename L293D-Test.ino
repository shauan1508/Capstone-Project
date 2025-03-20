#include <Arduino.h>

// Motor 1 (Front Left)
#define M1_EN 5   
#define M1_DIR1 18  
#define M1_DIR2 19  

// Motor 2 (Front Right)
#define M2_EN 21  
#define M2_DIR1 23  
#define M2_DIR2 22  

// Motor 3 (Rear Left)
#define M3_EN 17  
#define M3_DIR1 16  
#define M3_DIR2 4  

// Motor 4 (Rear Right)
#define M4_EN 2  
#define M4_DIR1 15  
#define M4_DIR2 13  

// PWM Configuration
#define PWM_FREQ 1000  
#define PWM_RES 8      
#define PWM_CH_M1 0
#define PWM_CH_M2 1
#define PWM_CH_M3 2
#define PWM_CH_M4 3

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32 AWD Motor Control");

    // Set up motor direction pins
    pinMode(M1_DIR1, OUTPUT); pinMode(M1_DIR2, OUTPUT);
    pinMode(M2_DIR1, OUTPUT); pinMode(M2_DIR2, OUTPUT);
    pinMode(M3_DIR1, OUTPUT); pinMode(M3_DIR2, OUTPUT);
    pinMode(M4_DIR1, OUTPUT); pinMode(M4_DIR2, OUTPUT);

    // Configure PWM channels for speed control
    ledcSetup(PWM_CH_M1, PWM_FREQ, PWM_RES); ledcAttachPin(M1_EN, PWM_CH_M1);
    ledcSetup(PWM_CH_M2, PWM_FREQ, PWM_RES); ledcAttachPin(M2_EN, PWM_CH_M2);
    ledcSetup(PWM_CH_M3, PWM_FREQ, PWM_RES); ledcAttachPin(M3_EN, PWM_CH_M3);
    ledcSetup(PWM_CH_M4, PWM_FREQ, PWM_RES); ledcAttachPin(M4_EN, PWM_CH_M4);
}

void controlMotor(int motor, int speed, char direction) {
    int dir1, dir2, en, pwmCh;
    
    if (motor == 1) { dir1 = M1_DIR1; dir2 = M1_DIR2; en = M1_EN; pwmCh = PWM_CH_M1; }
    if (motor == 2) { dir1 = M2_DIR1; dir2 = M2_DIR2; en = M2_EN; pwmCh = PWM_CH_M2; }
    if (motor == 3) { dir1 = M3_DIR1; dir2 = M3_DIR2; en = M3_EN; pwmCh = PWM_CH_M3; }
    if (motor == 4) { dir1 = M4_DIR1; dir2 = M4_DIR2; en = M4_EN; pwmCh = PWM_CH_M4; }

    if (direction == 'F') {
        digitalWrite(dir1, HIGH);
        digitalWrite(dir2, LOW);
    } else if (direction == 'B') {
        digitalWrite(dir1, LOW);
        digitalWrite(dir2, HIGH);
    } else {
        digitalWrite(dir1, LOW);
        digitalWrite(dir2, LOW);
        speed = 0;
    }

    ledcWrite(pwmCh, speed);
}

void loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');  
        input.trim();

        if (input == "S") {
            Serial.println("Stopping all motors...");
            for (int i = 1; i <= 4; i++) controlMotor(i, 0, 'S');
        } else {
            int comma1 = input.indexOf(',');
            int comma2 = input.indexOf(',', comma1 + 1);
            int comma3 = input.indexOf(',', comma2 + 1);
            int comma4 = input.indexOf(',', comma3 + 1);
            int comma5 = input.indexOf(',', comma4 + 1);
            int comma6 = input.indexOf(',', comma5 + 1);
            int comma7 = input.indexOf(',', comma6 + 1);

            int m1Speed = input.substring(0, comma1).toInt();
            char m1Dir = input.substring(comma1 + 1, comma2).charAt(0);
            int m2Speed = input.substring(comma2 + 1, comma3).toInt();
            char m2Dir = input.substring(comma3 + 1, comma4).charAt(0);
            int m3Speed = input.substring(comma4 + 1, comma5).toInt();
            char m3Dir = input.substring(comma5 + 1, comma6).charAt(0);
            int m4Speed = input.substring(comma6 + 1).toInt();
            char m4Dir = input.substring(comma7 + 1).charAt(0);

            controlMotor(1, m1Speed, m1Dir);
            controlMotor(2, m2Speed, m2Dir);
            controlMotor(3, m3Speed, m3Dir);
            controlMotor(4, m4Speed, m4Dir);
        }
    }
}
