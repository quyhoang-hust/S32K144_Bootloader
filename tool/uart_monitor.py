import argparse
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is missing. Install it with:  pip install pyserial")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Passive UART monitor for the S32K144 bootloader.")
    parser.add_argument("port",
                        help="Serial port, e.g. COM13 or /dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200,
                        help="Baud rate (default: 115200)")
    parser.add_argument("--raw", action="store_true",
                        help="Show a hex dump of every byte as well")
    return parser.parse_args()


def describe_byte(value):
    """Return a readable form of one byte."""
    if value == 0x0A:
        return "\\n"
    if value == 0x0D:
        return "\\r"
    if 0x20 <= value <= 0x7E:
        return chr(value)
    return f"<{value:02X}>"


def main():
    args = parse_args()

    try:
        port = serial.Serial(args.port, args.baud, timeout=0.1)
    except serial.SerialException as exc:
        sys.exit(f"Cannot open {args.port}: {exc}")

    print(f"Listening on {args.port} at {args.baud} baud.")
    print("Press the board RESET button now. Ctrl+C to stop.\n")
    print("-" * 60)

    line = bytearray()
    byte_count = 0
    start = time.time()

    try:
        while True:
            chunk = port.read(256)

            if not chunk:
                continue

            byte_count += len(chunk)

            if args.raw:
                stamp = time.time() - start
                hexed = " ".join(f"{b:02X}" for b in chunk)
                print(f"[{stamp:7.3f}] RAW {hexed}")

            for value in chunk:
                if value == 0x0A:          
                    stamp = time.time() - start
                    text = line.decode("ascii", errors="replace").rstrip("\r")
                    print(f"[{stamp:7.3f}] {text}")
                    line.clear()
                elif value == 0x0D:
                    pass                   
                else:
                    line.append(value)

    except KeyboardInterrupt:
        if line:
            text = line.decode("ascii", errors="replace")
            print(f"[partial] {text}")

        print("\n" + "-" * 60)
        print(f"Received {byte_count} bytes total.")

        if byte_count == 0:
            print("\nNothing was received. Check, in this order:")
            print("  1. Is this the right COM port?")
            print("     Unplug the board and see which port disappears.")
            print("  2. Is the CPU actually running?")
            print("     A halted debugger keeps the core stopped.")
            print("  3. Are TX/RX swapped, and is GND connected?")
            print("  4. Did ClockDriver_Init() succeed?")
            print("     If it returns false the UART is never initialized.")
        else:
            print("\nThe board is transmitting, so clock and UART TX work.")

    finally:
        port.close()


if __name__ == "__main__":
    main()
