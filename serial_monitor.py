#!/usr/bin/env python3
"""
Simple Serial Monitor for nRF54L15DK
"""
import sys
import time
import serial
import serial.tools.list_ports
from datetime import datetime

def list_com_ports():
    """List all available COM ports"""
    ports = serial.tools.list_ports.comports()
    available_ports = []
    
    print("\n=== Available COM Ports ===")
    for port in ports:
        print(f"  {port.device}: {port.description}")
        available_ports.append(port.device)
    
    if not available_ports:
        print("  No COM ports found!")
    
    return available_ports

def monitor_serial(port_name, baud_rate=115200):
    """Monitor serial port and display output"""
    try:
        # Try to open the serial port
        print(f"\nConnecting to {port_name} at {baud_rate} baud...")
        ser = serial.Serial(port_name, baud_rate, timeout=0.1)
        
        print(f"Connected successfully!")
        print("Press Ctrl+C to stop\n")
        print("-" * 40)
        
        while True:
            try:
                # Read available data
                if ser.in_waiting > 0:
                    # Read line
                    line = ser.readline()
                    try:
                        # Try to decode as UTF-8
                        text = line.decode('utf-8').rstrip()
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                        
                        # Color coding based on content (basic terminal output)
                        if "ERR" in text or "ERROR" in text:
                            print(f"[{timestamp}] ERROR: {text}")
                        elif "WRN" in text or "WARNING" in text:
                            print(f"[{timestamp}] WARN:  {text}")
                        elif "INF" in text or "INFO" in text:
                            print(f"[{timestamp}] INFO:  {text}")
                        else:
                            print(f"[{timestamp}] {text}")
                    except UnicodeDecodeError:
                        # If can't decode, show as hex
                        print(f"[RAW] {line.hex()}")
                
                time.sleep(0.01)  # Small delay to prevent CPU spinning
                
            except KeyboardInterrupt:
                print("\n\nStopping monitor...")
                break
            except Exception as e:
                print(f"Read error: {e}")
                
    except serial.SerialException as e:
        print(f"\nERROR: Cannot open {port_name}")
        print(f"Details: {e}")
        print("\nPossible causes:")
        print("  1. Port is in use by another program")
        print("  2. Port doesn't exist")
        print("  3. No permission to access port")
        return False
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Port closed.")
    
    return True

def main():
    """Main function"""
    print("=" * 40)
    print("  nRF54L15DK Serial Monitor")
    print("  Board: PCA10156")
    print("=" * 40)
    
    # List available ports
    ports = list_com_ports()
    
    # Default to COM3
    port = "COM3"
    baud = 115200
    
    # Check command line arguments
    if len(sys.argv) > 1:
        port = sys.argv[1]
    if len(sys.argv) > 2:
        baud = int(sys.argv[2])
    
    # If COM3 not available, ask user
    if port not in ports and ports:
        print(f"\n{port} not found in available ports.")
        port = input(f"Enter COM port to use (e.g., {ports[0]}): ").strip()
        if not port:
            port = ports[0]
    
    # Start monitoring
    monitor_serial(port, baud)
    
    print("\nPress any key to exit...")
    input()

if __name__ == "__main__":
    main()
