# Taara-V2

This is a documentation of my replication of Project Taara, now an independent company that was initially spearheaded by X, The Moonshot Factory. Taara is a Free Space Optical Communications System. This is a system of lasers and receivers used to transmit information at really fast speeds over moderate distances, designed to revolutionize last mile connectivity. 

In this replication, I'm using XIAO ESP32S3 as the microcontroller for both the transmitter and reciever. Currently, the receiving sensor is a photoresistor, but it will soon be upgraded to a photodiode. 

The transmitter is a laser module controlled using PWM (Pulse-Width Modulation).

Data is encoded using Manchester encoding to ensure efficient packetization.

The current transmission protocol has an automatic threshhold calibration for use in various environments. 

Start frames and end frames are also implemented to create an auto-sync method. 

This is an ongoing project and I'm moving at breakneck speed to improve the system. I post about updates every week.

###Current Goals:

Implement Reed-Solomon error correction
3D-Print housing
Build an ML anomaly detection system (for added layer of security).
Build a 3-point servo system for auto-alignment (for long distances).


Updates are on [my twitter.](https://x.com/harishvin_s)
