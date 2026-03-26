import serial
import csv
import time
import argparse
import sys

# CONSTANTS
DEFAULT_BAUD_RATE = 115200
OUTPUT_FILE = "sensor_data.csv"

def collect_data(port, baud_rate, label):
    """
    Reads temperature and humidity from the serial port and saves to a CSV file.
    :param port: COM port ('COM3' or '/dev/ttyUSB0')
    :param baud_rate: Serial baud rate
    :param label: The class label to assign to the collected data (0, 1, or 2)
    """
    print(f"[INFO] [COLLECTOR] Starting data collection on {port} at {baud_rate} baud.")
    print(f"[INFO] [COLLECTOR] Target Label: {label}")
    print(f"[INFO] [COLLECTOR] Press Ctrl+C to stop and save.")

    try:
        ser = serial.Serial(port, baud_rate, timeout=1)
        time.sleep(2) # Wait for MCU to reset upon serial connection
    except Exception as e:
        print(f"[ERR]  [COLLECTOR] Failed to open serial port {port}: {e}")
        sys.exit(1)

    # Open CSV file in append mode
    try:
        with open(OUTPUT_FILE, mode='a', newline='') as file:
            writer = csv.writer(file)
            
            # Write header if file is empty
            if file.tell() == 0:
                writer.writerow(["temp", "humidity", "label"])
                print(f"[INFO] [COLLECTOR] Created new dataset file: {OUTPUT_FILE}")

            samples_collected = 0

            while True:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8').strip()
                    
                    if not line:
                        continue

                    # Expected format from ESP32: "temp,hum" (e.g., "5.5,65.2")
                    try:
                        parts = line.split(',')
                        if len(parts) == 2:
                            temp = float(parts[0])
                            hum = float(parts[1])
                            
                            # Save to CSV with the assigned label
                            writer.writerow([temp, hum, label])
                            samples_collected += 1
                            
                            print(f"[INFO] [COLLECTOR] Sample {samples_collected} | Temp: {temp:.1f}C, Hum: {hum:.1f}%, Label: {label}")
                        else:
                            print(f"[WARN] [COLLECTOR] Invalid data format received: {line}")
                    except ValueError:
                        print(f"[WARN] [COLLECTOR] Could not parse line: {line}")
                        
    except KeyboardInterrupt:
        print(f"\n[INFO] [COLLECTOR] Data collection stopped by user.")
        print(f"[INFO] [COLLECTOR] Total samples collected this session: {samples_collected}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print(f"[INFO] [COLLECTOR] Serial port closed.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Collect sensor data from ESP32 via Serial")
    parser.add_argument("--port", type=str, required=True, help="Serial port (COM3, /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD_RATE, help="Baud rate (default: 115200)")
    parser.add_argument("--label", type=int, required=True, choices=[0, 1, 2], 
                        help="Data label (0: Normal, 1: Door Open, 2: System Fault)")
    
    args = parser.parse_args()
    collect_data(args.port, args.baud, args.label)