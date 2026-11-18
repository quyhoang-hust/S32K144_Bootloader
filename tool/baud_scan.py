import argparse
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is missing. Install it with:  pip install pyserial")

DEFAULT_RATES = [
    9600, 14400, 19200, 28800, 38400, 57600,
    76800, 115200, 153600, 230400, 460800,
]

REFERENCE_BAUD = 115200
ASSUMED_CLOCK_HZ = 48_000_000


def score_bytes(data):
    """Return (good, bad, ratio) for a chunk of received bytes."""
    if not data:
        return 0, 0, 0.0

    good = 0

    for value in data:
        if 0x20 <= value <= 0x7E:      # printable ASCII
            good += 1
        elif value in (0x0A, 0x0D, 0x09):   # LF, CR, TAB
            good += 1

    bad = len(data) - good
    return good, bad, good / len(data)


def preview(data, limit=70):
    """Readable one-line preview of the received bytes."""
    out = []

    for value in data[:limit]:
        if value in (0x0A, 0x0D):
            out.append(" ")
        elif 0x20 <= value <= 0x7E:
            out.append(chr(value))
        else:
            out.append(".")

    return "".join(out)


def scan_one(port_name, baud, dwell):
    """Open the port at one baud rate and collect for `dwell` seconds."""
    try:
        port = serial.Serial(port_name, baud, timeout=0.1)
    except serial.SerialException as exc:
        print(f"  cannot open at {baud}: {exc}")
        return None

    try:
        port.reset_input_buffer()
        collected = bytearray()
        deadline = time.time() + dwell

        while time.time() < deadline:
            chunk = port.read(512)
            if chunk:
                collected.extend(chunk)

        return bytes(collected)

    finally:
        port.close()


def parse_args():
    parser = argparse.ArgumentParser(
        description="Detect the real transmit baud rate of the board.")
    parser.add_argument("port", help="Serial port, e.g. COM13")
    parser.add_argument("--dwell", type=float, default=3.0,
                        help="Seconds to listen per baud rate (default: 3)")
    parser.add_argument("--list", type=int, nargs="+", default=None,
                        help="Explicit list of baud rates to try")
    return parser.parse_args()


def main():
    args = parse_args()
    rates = args.list if args.list else DEFAULT_RATES

    print(f"Scanning {args.port}, {args.dwell:.1f}s per rate.")
    print("Keep pressing RESET so the board keeps printing.\n")
    print(f"{'baud':>8}  {'bytes':>6}  {'ascii%':>7}  preview")
    print("-" * 78)

    results = []

    for baud in rates:
        data = scan_one(args.port, baud, args.dwell)

        if data is None:
            continue

        good, bad, ratio = score_bytes(data)
        results.append((ratio, len(data), baud, data))

        marker = "  <== looks like text" if (ratio > 0.90 and
                                             len(data) > 20) else ""

        print(f"{baud:>8}  {len(data):>6}  {ratio * 100:>6.1f}%  "
              f"{preview(data)}{marker}")

    print("-" * 78)

    # Pick the best candidate: high ASCII ratio and a meaningful sample.
    usable = [r for r in results if r[1] > 20]

    if not usable:
        print("\nNo data was received at any rate.")
        print("The board was probably not reset while the scan ran.")
        print("Re-run and press RESET every couple of seconds.")
        return

    usable.sort(reverse=True)
    best_ratio, best_len, best_baud, best_data = usable[0]

    if best_ratio < 0.90:
        print("\nNo rate produced clean text.")
        print("The clock is likely unstable rather than simply mis-scaled,")
        print("which matches an out-of-spec core frequency.")
        print("Apply the corrected clock_driver.c and retest.")
        return

    print(f"\nBest match: {best_baud} baud "
          f"({best_ratio * 100:.1f}% printable)\n")
    print("Sample:")
    print("  " + preview(best_data, 200))

    ratio = best_baud / REFERENCE_BAUD
    real_clock = ASSUMED_CLOCK_HZ * ratio

    print(f"\nThe firmware assumes {ASSUMED_CLOCK_HZ / 1e6:.0f} MHz for "
          f"LPUART and targets {REFERENCE_BAUD} baud.")
    print(f"Detected {best_baud} baud implies a real LPUART clock of about "
          f"{real_clock / 1e6:.1f} MHz (ratio {ratio:.3f}).")

    if abs(ratio - 1.0) < 0.02:
        print("\nThat matches the configuration, so the clock is fine.")
    else:
        print("\nThe clock tree is not delivering what the firmware expects.")
        print("Update SCG_SPLLCFG / SCG_RCCR in clock_driver.c, or set")
        print("BOOTLOADER_LPUART_CLOCK_HZ to the real value.")


if __name__ == "__main__":
    main()
