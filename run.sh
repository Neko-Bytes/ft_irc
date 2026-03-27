#!/bin/bash

# ==========================================
# ft_irc & LCD Monitor Startup Script
# ==========================================

# 1. Define Server Variables (Change these if needed)
IRC_PORT=6667
IRC_PASS="pass"
ARDUINO_SKETCH="$HOME/nubt/irc/lcd"

# 2. Auto-Detect the Arduino USB Port
echo "Searching for Arduino LCD..."
ARDUINO_PORT=$(ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | head -n 1)

if [ -z "$ARDUINO_PORT" ]; then
    echo "❌ Error: Arduino not found! Please check the USB connection."
    exit 1
fi

echo "✅ Arduino found on: $ARDUINO_PORT"

# # 3. Update the Arduino Code
# echo "📱 Compiling and uploading Arduino sketch..."
# arduino-cli compile --fqbn arduino:avr:uno "$ARDUINO_SKETCH"
#
# # Upload to the detected port
# arduino-cli upload -p "$ARDUINO_PORT" --fqbn arduino:avr:uno "$ARDUINO_SKETCH"
#
# # Wait 2 seconds for the Arduino to reboot after receiving the new code
# sleep 2

# 4. Configure the Serial Port Speed (9600 baud)
echo "⚙️  Configuring serial port..."
stty -F "$ARDUINO_PORT" 9600 cs8 -cstopb -parenb

# 5. Compile the server if the executable doesn't exist
if [ ! -f "./ircserv" ]; then
    echo "🔨 Executable not found. Running make..."
    make -j4
fi

# 6. Launch the Server and Pipe to Arduino
echo "🚀 Starting IRC Server on port $IRC_PORT..."
echo "=========================================="
# ./ircserv "$IRC_PORT" "$IRC_PASS" | tee "$ARDUINO_PORT"
./ircserv "$IRC_PORT" "$IRC_PASS" | tee "$ARDUINO_PORT" | grep --line-buffered -v "^STATS:"
