import serial
import time

try:
    ser = serial.Serial('COM3', 115200, timeout=1)
    print("Connected to COM3 at 115200 baud")
    print("Waiting for serial data...")
    
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode().strip()
            if line:
                print(line)
        time.sleep(0.1)
        
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
except KeyboardInterrupt:
    print("\nExiting...")
    ser.close()
except Exception as e:
    print(f"Error: {e}")
