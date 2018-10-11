#!/bin/sh
killall launcher &> /dev/null
sudo rm -rf /usr/local/bin/launcher
sudo rm -rf /usr/local/bin/launcher-linux-x64
sudo rm -rf ~/.config/autostart/launcher.desktop
echo "Uninstall successful"
