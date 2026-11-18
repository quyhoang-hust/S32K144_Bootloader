
import argparse
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is missing. Install it with:  pip install pyserial")


APP_START = 0x00020000
APP_END = 0x00080000



ADDRESS_BYTES = {"0": 2, "1": 2, "2": 3, "3": 4, "5": 2, "6": 3,
                 "7": 4, "8": 3, "9": 2}


def validate_srec(path):
    """Check the file the way the bootloader will, and report problems."""
    data_records = 0
    lowest = None
    highest = None
    problems = []
    previous_end = None

    with open(path, "r") as handle:
        for number, raw in enumerate(handle, start=1):
            line = raw.strip()

            if not line:
                continue

            if line[0] != "S":
                problems.append(f"line {number}: does not start with 'S'")
                continue

            kind = line[1]

            if kind not in ADDRESS_BYTES:
                problems.append(f"line {number}: unknown record type S{kind}")
                continue

            count = int(line[2:4], 16)
            alen = ADDRESS_BYTES[kind]
            address = int(line[4:4 + alen * 2], 16)
            payload = count - alen - 1

            # Checksum: every byte from the count field onwards must sum
            # to 0xFF in the low byte.
            total = sum(int(line[2 + i * 2:4 + i * 2], 16)
                        for i in range(count + 1))

            if (total & 0xFF) != 0xFF:
                problems.append(f"line {number}: bad checksum")

            if kind in ("1", "2", "3"):
                data_records += 1
                end = address + payload

                if address < APP_START or end > APP_END:
                    problems.append(
                        f"line {number}: 0x{address:06X}..0x{end:06X} "
                        f"is outside the application region")

                if previous_end is not None and address < previous_end:
                    problems.append(
                        f"line {number}: address 0x{address:06X} goes "
                        f"backwards (overlap)")

                previous_end = end
                lowest = address if lowest is None else min(lowest, address)
                highest = end if highest is None else max(highest, end)

    return {
        "data_records": data_records,
        "lowest": lowest,
        "highest": highest,
        "problems": problems,
    }

class Board:
    def __init__(self, port, baud, quiet=False):
        self.port = serial.Serial(port, baud, timeout=0.05)
        self.quiet = quiet
        self._buffer = ""

    def close(self):
        self.port.close()

    def clear_buffer(self):
        """Forget the transcript so far, before starting a new step."""
        self._buffer = ""

    def drain(self):
        """Read whatever is pending and echo it."""
        chunk = self.port.read(4096)

        if not chunk:
            return ""

        text = chunk.decode("ascii", errors="replace")
        self._buffer += text

        if not self.quiet:
            sys.stdout.write(text)
            sys.stdout.flush()

        return text

    def _check_fatal(self):
        if "[FATAL]" in self._buffer:
            for line in self._buffer.splitlines():
                if "[FATAL]" in line:
                    raise RuntimeError(f"Board reported {line.strip()}")

    def wait_for(self, needle, timeout, fatal_aborts=True, poke=None):
        deadline = time.time() + timeout

        # Already there from an earlier read.
        if needle in self._buffer:
            return True

        while time.time() < deadline:
            if poke is not None:
                self.port.write(poke)
                self.port.flush()

            self.drain()

            if fatal_aborts:
                self._check_fatal()

            if needle in self._buffer:
                return True

        raise TimeoutError(f"Timed out waiting for {needle!r}")

    def send_key(self, key):
        self.port.write(key.encode("ascii"))
        self.port.flush()

    def send_line(self, text):
        self.port.write((text + "\r\n").encode("ascii"))
        self.port.flush()


def parse_args():
    parser = argparse.ArgumentParser(
        description="Drive the S32K144 mini bootloader download session.")
    parser.add_argument("port", help="Serial port, e.g. COM13")
    parser.add_argument("srec", help="SREC file to download")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--delay", type=float, default=0.005,
                        help="Delay between SREC lines in seconds "
                             "(default: 0.005). Raise it if the board "
                             "reports RX_OVERFLOW.")
    parser.add_argument("--skip-erase", action="store_true",
                        help="Skip menu option 1 (region already erased)")
    parser.add_argument("--catch", action="store_true",
                        help="Stream CR while waiting for the menu, to hold "
                             "the board in its UART entry window")
    parser.add_argument("--erase-timeout", type=float, default=120.0)
    return parser.parse_args()


def main():
    args = parse_args()

    print(f"Validating {args.srec} ...")
    report = validate_srec(args.srec)

    if report["problems"]:
        print("\nThe SREC file has problems:")
        for item in report["problems"][:20]:
            print(f"  - {item}")
        sys.exit("\nFix the file (or the application linker script) first.")

    print(f"  data records : {report['data_records']}")
    print(f"  address range: 0x{report['lowest']:06X} .. "
          f"0x{report['highest']:06X}")
    print("  looks good\n")

    board = Board(args.port, args.baud)

    try:
        # --- Step 1: menu -------------------------------------------------
        print("Press the board RESET button now (waiting for the menu) ...")

        poke = b"\r" if args.catch else None
        board.wait_for("Select option:", timeout=30.0, poke=poke)

        print("\n>>> menu detected\n")

        # --- Step 2: erase ------------------------------------------------
        if not args.skip_erase:
            print(">>> sending '1' (erase application region)")

            board.clear_buffer()
            board.send_key("1")

            board.wait_for("Erase application done",
                           timeout=args.erase_timeout)

            print("\n>>> erase complete\n")

            time.sleep(0.3)
            board.drain()

        # --- Step 3: enter receive mode -----------------------------------
        print(">>> sending '2' (start download)")

        board.clear_buffer()
        board.send_key("2")

        board.wait_for("Ready to receive SREC", timeout=10.0)

        print("\n>>> board is ready, streaming file\n")

        # --- Step 4: stream the file --------------------------------------
        with open(args.srec, "r") as handle:
            lines = [ln.strip() for ln in handle if ln.strip()]

        board.clear_buffer()

        for index, line in enumerate(lines, start=1):
            board.send_line(line)
            time.sleep(args.delay)

            text = board.drain()

            if "[FATAL]" in text:
                for entry in text.splitlines():
                    if "[FATAL]" in entry:
                        raise RuntimeError(
                            f"Board reported {entry.strip()} "
                            f"at SREC line {index}")

        print("\n>>> file sent, waiting for FINISHED\n")
        board.wait_for("state=FINISHED", timeout=30.0)

        time.sleep(0.5)
        board.drain()

        print("\n" + "=" * 60)
        print("Download complete.")
        print("Reset the board; the application should start.")

    except (TimeoutError, RuntimeError) as exc:
        print("\n" + "=" * 60)
        print(f"FAILED: {exc}")
        sys.exit(1)

    except KeyboardInterrupt:
        print("\nInterrupted.")

    finally:
        board.close()


if __name__ == "__main__":
    main()
