# MDP-RaspberryPi
<i>Disclaimer - This repository was submitted as part of the CZ3004 - Multidisciplinary Project course at Nanyang Technological University and is no longer maintained.</i>

This repository contains the code for a raspberry pi on board computer in a two wheel differential drive robot. The program allows the raspberry pi to interact with an Arduino over USB serial, an Android app over bluetooth and a code running on another computer over tcp/ip sockets. This repository is part of a bigger project to drive a robot autonomously through a maze to achieve two tasks - Exploration and Solving the maze in the fastest possible path.

# Dependencies
1. [WiringPi](http://wiringpi.com/)
2. [BlueZ](http://www.bluez.org/)
3. [GNU Compiler Collection](https://gcc.gnu.org/)

# Running
1. First clone the repository using ```git clone https://github.com/nikv96/MDP-RaspberryPi``` or download the zip [here](https://github.com/nikv96/MDP-RaspberryPi/archive/master.zip).
2. Compile the code by running ```make``` within the repository.
3. Run the code with ```./rpi``` in the repository.

# Contributors
1. [hongsia](https://github.com/hongsia)
2. [nikv96](https://github.com/nikv96)
