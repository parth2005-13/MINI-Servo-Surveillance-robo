#include <Arduino.h>
#include <ESP32Servo.h>

// defining all the parts of the servo's and sensors
Servo head;
Servo tail;
Servo LF;
Servo LB;
Servo RF;
Servo RB;

// defining the for servo's and
#define head_pin 13
#define LF_pin 12
#define LB_pin 14
#define RF_pin 27
#define RB_pin 26
#define tail_pin 25
#define pir_pin 15
#define echo_pin 23
#define trig_pin 21
#define buzzer_pin 18

// defined timeout's to avoid random state changing behaviours and jitter's
const unsigned long pir_stable = 500;
const unsigned long scan_duration = 20000;
const unsigned long confirmation_time = 3000;
const unsigned long output_interval = 500;
const unsigned long cool_down = 3000;
const int entry_distance = 100;
const int exit_distance = 120;

// intially starting all the timer's from 0second
unsigned long tail_timer = 0;
unsigned long pir_start = 0;
unsigned long scanStartTime = 0;
unsigned long confirmation_start = 0;
unsigned long sweap_time = 0;
unsigned long start_coolDown = 0;
unsigned long exit_start = 0;
unsigned long alertTimer = 0;

bool alertState = false;

// initial angle of tail, will be used to wobble the tail
int tail_angle = 40;
int tail_direction = 1;

bool isStanding = false;

// defined state-machine:- all the states robo will go through
enum robo_state
{
    IDLE,
    SCANNING,
    CONFIRMING,
    TARGET_LOCKED,
    COOLDOWN
};
robo_state State = IDLE;

// THESE parameter's will help to operate head servo, from 0 to 180 and VICE-VERSA
int angle = 0;
int direction = 1;
int locked_angle = 90;

// SLA means sleeping_angle
// SA means standing_angle
const int LF_SLA = 0;
const int LF_SA = 110;
const int LB_SLA = 110;
const int LB_SA = 0;
const int RF_SLA = 110;
const int RF_SA = 0;
const int RB_SLA = 0;
const int RB_SA = 110;

const int buzzerChannel = 7; // highest channel
const int buzzerTimer = 3;   // use separate timer

// defined standing robo function with a generalised formula to operate each leg as-
//-all of them different standing and sleeping angle
void standingRobo()
{
    for (int step = 0; step <= 100; step++)
    {
        LF.write(LF_SLA + (LF_SA - LF_SLA) * step / 100);
        LB.write(LB_SLA + (LB_SA - LB_SLA) * step / 100);
        RF.write(RF_SLA + (RF_SA - RF_SLA) * step / 100);
        RB.write(RB_SLA + (RB_SA - RB_SLA) * step / 100);
        delay(10);
    }
    isStanding = true;
}

// defined sleeping_State of robo
void sleepyRobo()
{
    for (int step = 0; step <= 100; step++)
    {
        LF.write(LF_SA + (LF_SLA - LF_SA) * step / 100);
        LB.write(LB_SA + (LB_SLA - LB_SA) * step / 100);
        RF.write(RF_SA + (RF_SLA - RF_SA) * step / 100);
        RB.write(RB_SA + (RB_SLA - RB_SA) * step / 100);
        delay(10);
    }
    isStanding = false;
}

void setup()
{
    Serial.begin(115200);
    // allocating timer to the servo, for proper resource contention
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    pinMode(trig_pin, OUTPUT);
    pinMode(echo_pin, INPUT);
    pinMode(pir_pin, INPUT);

    head.attach(head_pin);
    tail.attach(tail_pin);
    LF.attach(LF_pin);
    LB.attach(LB_pin);
    RF.attach(RF_pin);
    RB.attach(RB_pin);

    // initial state of head and tail
    head.write(90);
    tail.write(120);

    ledcSetup(buzzerChannel, 2000, 8);
    ledcAttachPin(buzzer_pin, buzzerChannel);
    ledcWrite(buzzerChannel, 0);

    // initial condition of robo
    sleepyRobo();
}

// function to create sound's with stages and update the alert's
void updateAlert()
{
    if (State == TARGET_LOCKED)
    {
        if (millis() - alertTimer >= 300)
        {
            alertTimer = millis();
            alertState = !alertState;

            if (alertState)
                ledcWrite(buzzerChannel, 128);
            else
                ledcWrite(buzzerChannel, 0);
        }
    }
    else
    {
        ledcWrite(buzzerChannel, 0);
        alertState = false;
    }
}

// function calculate the distance using ultrasonic sensor
long readDistance()
{
    static long lastValidDistance = 400;

    digitalWrite(trig_pin, LOW);
    delayMicroseconds(2);

    digitalWrite(trig_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_pin, LOW);

    long duration = pulseIn(echo_pin, HIGH, 30000);

    if (duration > 0)
    {
        long distance = duration * 0.0343 / 2.0;
        lastValidDistance = distance;
    }

    return lastValidDistance;
}

void loop()
{
    float distance = readDistance();
    bool pir = digitalRead(pir_pin);

    switch (State)
    {
    // IDLE stage:- object will first take input from pir to confirm if any motion detected
    case IDLE:
        tail.write(40);

        if (pir)
        {
            if (pir_start == 0)
                pir_start = millis();

            if (millis() - pir_start >= pir_stable)
            {
                State = SCANNING;
                scanStartTime = millis();
                pir_start = 0;
            }
        }
        else
            pir_start = 0;
        break;

    // As soona as motion detected, scanning stage start's, THE SCANNING TIMEOUT PERIOD IS 15seconds
    case SCANNING:
        tail.write(130); // suspecious activity detected

        if (millis() - sweap_time > 20)
        {
            sweap_time = millis();
            angle += direction;

            if (angle >= 180 || angle <= 0)
                direction *= -1;

            head.write(angle);
        }

        if (distance < entry_distance)
        {
            locked_angle = angle;
            confirmation_start = millis();
            State = CONFIRMING;
            tail_angle = 40;
            tail_direction = 1;
        }

        if (millis() - scanStartTime >= scan_duration)
        {
            State = COOLDOWN;
            start_coolDown = millis();
        }
        break;

    // Confirming stage:- as soon as it scans a object in radius of 100cm
    case CONFIRMING:
        if (millis() - tail_timer >= 30) // alerting mode, tail wobble
        {
            tail_timer = millis();
            tail_angle += tail_direction * 5;

            if (tail_angle > 110 || tail_angle < 40)
                tail_direction *= -1;

            tail.write(tail_angle);
        }

        head.write(locked_angle);

        if (distance > entry_distance)
        {
            State = SCANNING;
        }
        else if (millis() - confirmation_start >= confirmation_time)
        {
            State = TARGET_LOCKED;
        }
        break;

    // target_locking stage:- after confirming the target for 3seconds
    case TARGET_LOCKED:
        if (!isStanding)
            standingRobo();

        if (millis() - tail_timer >= 30)
        {
            tail_timer = millis();
            tail_angle += tail_direction * 5;

            if (tail_angle > 110 || tail_angle < 40)
                tail_direction *= -1;

            tail.write(tail_angle);
        }

        head.write(locked_angle);

        if (distance > exit_distance)
        {
            if (exit_start == 0)
                exit_start = millis();

            if (millis() - exit_start >= 2000)
            {
                State = SCANNING;
                scanStartTime = millis();
                exit_start = 0;
            }
        }
        else
            exit_start = 0;
        break;

    // Cooldown stage:- gives buffer-time(3second's) to stable the robo before rentring to the idle stage
    case COOLDOWN:
        tail.write(120);

        if (isStanding)
            sleepyRobo();

        if (millis() - start_coolDown >= cool_down)
            State = IDLE;
        break;
    }

    updateAlert();

    // printing outputs
    static unsigned long output_start = 0;

    if (millis() - output_start >= output_interval)
    {
        output_start = millis();
        Serial.print("State: ");
        Serial.print(State);
        Serial.print(" | Distance: ");
        Serial.print(distance);
        Serial.print(" | angle: ");
        Serial.print(locked_angle);
        Serial.print(" | Standing: ");
        Serial.println(isStanding);
    }
}