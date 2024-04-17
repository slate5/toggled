## Out-of-the-box desktop entries
Toggled comes with desktop entries for the following services:
```
bluetooth
minidlna
mosquitto
proftpd
ssh
```
Each service on the list has a desktop entry and three icons ([see below](#icon-convention)).

## Desktop entry example
If users wish to add a custom desktop entry, they can use this as a reference:
```bash
[Desktop Entry]
Version=1.0
Type=Application
Name=MQTT toggle
Comment=Toggle mosquitto
Exec=toggled mosquitto
Icon=toggled-mqtt
Categories=Utility
Keywords=mqtt;mosquitto;toggled;
OnlyShowIn=XFCE;
Terminal=false
StartupNotify=false
```
**NOTE—In case of using only the name of the icon (such as in the example above) and not the full path to the icon:**\
To ensure dynamic icon switching, it's important that the name of the icon in the directive *Icon* does NOT include an extension (such as *.svg*). That is when a launcher relies on finding the icon in dedicated directories, such as *~/.local/share/icons* or */usr/share/icons*. This is necessary because *XFCE* panel launchers for some reason dislike extensions (tested on Debian 12/XFCE4). For absolute icon paths, this warning can be ignored.

## Icon convention
Icons are found and stored inside of *~/.local/share/icons*.\
Each desktop entry should be accompanied by three icons. One icon is for the desktop entry and it should represent the neutral state of a service (neither on nor off), while the other two represent the on/off state of the service for the *XFCE* panel launcher. To work properly, a dynamic launcher has to have two icons with *-on* and *-off* appended to the *neutral* icon name.\
Example:
```bash
# Neutral icon name for desktop entry
icon-name.svg
# Icon name for "on" state of XFCE panel launcher
icon-name-on.svg
# Icon name for "off" state of XFCE panel launcher
icon-name-off.svg
```
