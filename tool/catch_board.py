
import argparse
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is missing. Install it with:  pip install pyserial")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Hold the S32K144 bootloader in its menu.")
    parser.add_argument("port", help="Serial port, e.g. COM13")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--key", default="\r",
                        help="Byte to stream (default: carriage return, "
                             "which the menu treats as no-op)")
    return parser.parse_args()


def main():
    args = parse_args()

    try:
        port = serial.Serial(args.port, args.baud, timeout=0.02)
    except serial.SerialException as exc:
        sys.exit(f"Cannot open {args.port}: {exc}")

    payload = args.key.encode("ascii")

    print(f"Streaming on {args.port} at {args.baud} baud.")
    print("Press RESET now. Ctrl+C once the board reports it stayed in the "
          "bootloader.\n")
    print("-" * 60)

    caught = False

    try:
        while True:
            port.write(payload)
            port.flush()

            chunk = port.read(512)

            if chunk:
                text = chunk.decode("ascii", errors="replace")
                sys.stdout.write(text)
                sys.stdout.flush()

                if ("entry requested" in text) and not caught:
                    caught = True
                    print("\n" + "-" * 60)
                    print(">>> Board is parked in the bootloader menu.")
                    print(">>> Press Ctrl+C, then run srec_loader.py.")
                    print("-" * 60)

            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\nStopped.")

        if not caught:
            print("\nThe entry message never appeared. Check that:")
            print("  - the bootloader was rebuilt with the UART entry window")
            print("  - RESET was actually pressed while this was running")

    finally:
        port.close()


if __name__ == "__main__":
    main()
