# MQTT_Siren
Source code for the $10 wireless siren project.

Refer to my YouTube video at https://www.youtube.com/watch?v=Nt3InIGqKDw which describes how to make a simple alarm.

The alarm uses an ESP8266-01 with a single channel relay module.  The N.O. contacts are used to control the relay.  The .ino file is used 
to connect to the MQTT server and responds to MQTT messages to activate the relay.

Home Assistant is used to control the relay using an input_boolean variable that is used in the automation (alarms.yaml)
to turn on / off the siren.  The definition for the relay is in switches.yaml.
