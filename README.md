## Launcher

Basic launcher build on Electron.js.

To run:

```
./bin/launcher
```

To run as a background service to ensure launcher opens faster:

```
./bin/launcher --service
```

Then to open the launcher:
```
./bin/launcher
```

### Systemd

To configure with systemd, first create a new unit:
```
nano /etc/systemd/system/launcher.service
```

(Note: this isn't right, it needs to wait for x to load)
```
[Unit]
Description=Launcher

[Service]
Type=simple
User=<user>
ExecStart=<path>/launcher --service
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

Then:
```
systemctl enable launcher
```

To debug:

```
journalctl -u launcher
```

### Development

```
npm install
```

To run

```
npm start
```

To build:

```
npm run build
```