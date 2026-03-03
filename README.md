🤖 MINI-Servo Surveillance robo
Hey folks, I’m introducing my first project, a surveillance robo for your room or your desk..
The idea of this project is to keep your desk or room secured and monitored from the possible threats and intrusion when you are not around.

🔄 Working
I’ve designed a state machine for the robo to separate the working of robo in different scenarios’

The States are:-
1️⃣ IDLE
In this state the robo will be at sleeping stage with folded leg-Servo’s and tail pointing down indicating a relaxed position

2️⃣ SCANNING
As soon as PIR detects any motion Robo’s head servo starts scanning the surroundings using HCSR04 sensor with tail pointing upwards indicating suspicious activity detected.
The total scanning duration before timeout is 15seconds. if nothing appears in the entry radius for 15seconds the robot will go to the cooldown state.

3️⃣ CONFIRMING
This stage will last for 3sec, As the object/obstacle detected in 100cm(entry-distance) radius the robo will start wobbling its tail indicating the object is in the red-zone.
if the object moves out of exit-distance the scanning will start again for the remaining time.

4️⃣ TARGET_LOCKED
After confirming the robo will change its position from sleeping to Standing, with an Alert sound coming out of passive buzzer indicating the Target is confirmed and locked inside the radius.
if the object moves out of 120cm(exit-distance) and didn't appeared again in 3seconds the state will change to the scanning again for 15seconds.

5️⃣ COOL_DOWN
In this state the robo will be at rest, pausing all the execution and scanning for 3seconds and after that the robo will move to the IDLE state and start everything all over.

🧩 COMPONENTS USED ARE

1. ESP32 MCU
2. 6sg90 sevo’s (4-leg servo’s and 1-head and 1-Tail servo)
3. PIR motion sensor:- TO detect motion
4. HCSR04 ultrasonic sensor:- TO detect the distance from the obstacle
5. Passive buzzer
6. Buck convertor:- Set the output voltage approx 5V
7. 2200uF, 25V rated capacitor
8. small size breadboard
9. Jumper-wires and 22AWG wires
10. 3S-18650 battery pack

⚙️ STRUGGLES AND LEARNING’S
As this was my project, there were a lot of software and hardware struggles I faced from designing state machines to isolating different stages…

1️⃣ Handling edge cases
i use a time-buffer concept for each stage to avoid random jitters’ and state changes. assigned separate exit and entry distance radius of 100cm and 120cm to avoid the rapid unusual state changes.

2️⃣ Replaced delay(x) with millis(x) function
Delay is a widely used concept to structure a program but it pauses the execution for the given time interval, i used millis() concept to overcome this problem… you can go through the source code to understand this

3️⃣ Peripheral resource contention
While adding a passive buzzer (PWM-based) to the miniServoRobo, the buzzer worked but the servos stopped responding. After debugging, I discovered the issue was LEDC timer contention in the ESP32.

Servos require a stable 50 Hz signal, while the buzzer was configured at 2 kHz. Both were unintentionally sharing the same hardware timer, and the buzzer reconfigured the timer frequency, breaking the servo signal.

I resolved the issue by explicitly allocating LEDC timers for the servo library and separating the PWM configuration for the buzzer. This prevented frequency overwriting and stabilized the system.

4️⃣ Eliminating electrical noises
Eliminating the electrical noises by attaching a (2200uF , 25V)rated capacitor and stable wiring connection to handle sudden spikes and stall current, this also helps my HCSR04 sensor to provide a stable reading.