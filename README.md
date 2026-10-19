# robot-hand-of-hope

A robotic hand that performs sign-language letters, numbers and short words.
Control it from any phone: the ESP32 creates its own WiFi network and serves a control page.
<img width="1304" height="800" alt="rhoh" src="https://github.com/user-attachments/assets/378fea81-2bfd-4604-8ab2-ffb063e21a04" />


<img width="1298" height="814" alt="rhoh2" src="https://github.com/user-attachments/assets/7227c343-cf64-488a-9b81-742b8a3d2231" />


## How it works
- ESP32 runs a WiFi access point and a small web server.
- Open the page, tap a letter (A-Z), number (0-10) or word (Hello, Yes, No).
- 5 servos move the fingers; a stepper motor rotates the wrist.
- Each gesture is a row of angles in the code, mapped to the tested range of each servo.

## Hardware
- ESP32, 5 servos (fingers), stepper motor with ULN2003 driver (wrist), (power supply, hand body)

## Files
- `robot-hand-sign.ino` - firmware

## My role
(ghi rõ bạn làm phần nào nếu là project nhóm)



