#!/bin/sh

cd "$(dirname "$0")"

killall launcher &> /dev/null

node build
sudo rm -r /usr/local/bin/launcher /usr/local/bin/launcher-linux-x64
sudo mv ../bin/* /usr/local/bin

echo "To significantly reduce the time to display the launcher, the background service can be autostarted when logging in."
echo
read -p "Do you wish the background service to autostart when logging in?" -n 1 -r
echo

if [[ $REPLY =~ ^[Yy]$ ]]; then

	cat >~/.config/autostart/launcher.desktop <<EOL
[Desktop Entry]
Type=Application
Name=Proto-Launcher
Exec=launcher --service
Terminal=false
EOL
	
	nohulauncher --service & disown

fi

echo "Installation successful!"
echo
echo "Start the launcher by running \"launcher\""
