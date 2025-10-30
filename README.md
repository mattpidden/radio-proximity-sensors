# Rally Proximity Alert System

## 📖 Overview
A long-range proximity detection system designed for rally teams. It uses LoRa communication between two devices — one mounted in the rally car and one with the service crew — to alert the team as the car approaches.


## 📄 Project Breif
Overview
- Allow rally service crew some early warning of car arrival, in management situations where phone signal is low or nill.
- Give some indication to rally service crew of the condition of car and the requirements from management services

Spesific functions
A) Audible or visual warning to the crew of an impending car arrival (priority)
B) Warning of arrival to indicate the rate of closing distance or eta with a basic change in either frequency or amplitude of sound/light (second priority)
C) An alternative to B, because of time constraints of the project, would be a single button that indicates major damage/major service required.

Potential ways to achieve spec with design
A) Transmitter in the car
- 12v to 5v usb plug
- Magnetic aerial on roof of car
- 1 on/off switch
- 3 buttons - No service required - Normal service required/fuel and tyres - Major service required/emergency repairs. Press to activate, press and hold to deactivate,
- Buttons lights up or have led next to them when actively transmitting

B) Receiver in management vehicle
- 12v to 5v usb plug or 5v USB Powerbank
- 1 on/off switch
- buzzer and or light bar
- 3 indicator lights (green, yellow, red)
- Noise-cancelling button that resets after the car leaves service or after a set time


## 🧩 Hardware Used
- 2 × Waveshare RP2040-LoRa-HF Development Kit (SX1262)
- 2 × Buzzers or small speakers
- 2 × USB power sources (e.g. power banks)
