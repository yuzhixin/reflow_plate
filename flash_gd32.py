"""GD32E230 serial ISP flash tool using standard bootloader protocol."""
import serial
import sys
import time
import struct

ACK = 0x79
NACK = 0x1F

def connect_and_sync(port, baud, timeout_seconds=40):
    """Attempt to connect and sync with bootloader, handling resets or USB replugs."""
    start_time = time.time()
    last_msg_time = 0
    while time.time() - start_time < timeout_seconds:
        ser = None
        try:
            ser = serial.Serial(port, baud, parity=serial.PARITY_EVEN, timeout=0.2)
            ser.reset_input_buffer()
            ser.write(bytes([0x7F]))
            ser.flush()
            time.sleep(0.05)
            resp = ser.read(1)
            if resp and resp[0] == ACK:
                ser.timeout = 2.0
                print("  Bootloader synced (ACK)")
                return ser
        except Exception:
            pass
        if ser:
            try:
                ser.close()
            except Exception:
                pass

        if time.time() - last_msg_time > 3.0:
            remaining = int(timeout_seconds - (time.time() - start_time))
            print(f"  Waiting for device: please press RESET or replug Type-C (BOOT0=1)... ({remaining}s remaining)")
            last_msg_time = time.time()

        time.sleep(0.3)

    print("  Connection timeout - please check BOOT0 jumper and USB cable")
    return None

def send_command(ser, cmd):
    """Send a command and its complement, verify ACK."""
    ser.write(bytes([cmd, cmd ^ 0xFF]))
    resp = ser.read(1)
    if resp and resp[0] == ACK:
        return True
    print(f"  Command 0x{cmd:02X} failed: 0x{resp[0]:02X}" if resp else f"  Command 0x{cmd:02X} timeout")
    return False

def cmd_get(ser):
    """Get bootloader version and supported commands."""
    if not send_command(ser, 0x00):
        return None
    count_raw = ser.read(1)
    if not count_raw:
        print("  Get: no count byte")
        return None
    count = count_raw[0]
    data = ser.read(count + 1)
    final_ack = ser.read(1)
    if not data or len(data) < count + 1:
        print(f"  Get: incomplete data, got {len(data) if data else 0} bytes")
        return None
    if not final_ack or final_ack[0] != ACK:
        print(f"  Get: missing final ACK: 0x{final_ack[0]:02X}" if final_ack else "  Get: final ACK timeout")
        return None
    version = data[0]
    commands = list(data[1:])
    print(f"  Bootloader version: {version}.{version>>4}.{version&0xF}")
    print(f"  Supported commands: {[f'0x{c:02X}' for c in commands]}")
    return commands

def cmd_get_id(ser):
    """Get chip ID."""
    if not send_command(ser, 0x02):
        return None
    time.sleep(0.05)
    count = ser.read(1)
    if not count:
        print("  Get ID: no count byte")
        return None
    count = count[0]
    if count > 16:
        print(f"  Get ID: bad count={count}, flushing and retrying...")
        ser.reset_input_buffer()
        return None
    data = ser.read(count + 1)
    final_ack = ser.read(1)
    if not data or len(data) < count + 1:
        print(f"  Get ID: incomplete data, got {len(data) if data else 0} bytes")
        return None
    if not final_ack or final_ack[0] != ACK:
        print(f"  Get ID: missing final ACK: 0x{final_ack[0]:02X}" if final_ack else "  Get ID: final ACK timeout")
        return None
    print(f"  Get ID raw: count={count}, data={data.hex()}")
    pid = struct.unpack('>H', data[:2])[0] if len(data) >= 2 else data[0]
    print(f"  PID: 0x{pid:04X}")
    return pid

def _send_payload_with_checksum(ser, payload, label="payload"):
    checksum = 0
    for b in payload:
        checksum ^= b
    return _send_raw_payload(ser, payload + bytes([checksum]), label)


def _send_raw_payload(ser, payload, label="payload"):
    ser.write(payload)
    resp = ser.read(1)
    if resp and resp[0] == ACK:
        return True
    print(f"  {label} failed: 0x{resp[0]:02X}" if resp else f"  {label} timeout")
    return False


def cmd_erase(ser, commands=None):
    """Global erase using the bootloader erase commands."""
    old_timeout = ser.timeout
    ser.timeout = 15  # Mass erase can take several seconds

    try:
        # AN3155: Standard erase (0x43) global erase payload is 0xFF followed by checksum 0x00
        if commands is None or 0x43 in commands:
            if send_command(ser, 0x43):
                if _send_raw_payload(ser, bytes([0xFF, 0x00]), "standard global erase (0x43)"):
                    print("  Global erase via standard erase (0x43): OK")
                    return True
                print("  Global erase via standard erase (0x43): failed")

        # AN3155: Extended erase (0x44) global erase payload is 0xFF 0xFF followed by checksum 0x00
        if commands is None or 0x44 in commands:
            if send_command(ser, 0x44):
                if _send_raw_payload(ser, bytes([0xFF, 0xFF, 0x00]), "extended global erase (0x44)"):
                    print("  Global erase via extended erase (0x44): OK")
                    return True
                print("  Global erase via extended erase (0x44): failed")
    finally:
        ser.timeout = old_timeout

    return False


def cmd_unlock_protection(ser, commands=None):
    """Try to clear erase/write protection before retrying erase."""
    unlocked = False
    for cmd, label in ((0x73, "write unprotect"), (0x92, "readout unprotect")):
        if commands is not None and cmd not in commands:
            continue
        if send_command(ser, cmd):
            unlocked = True
            print(f"  {label} accepted; waiting for reset...")
            time.sleep(2)
            ser.reset_input_buffer()
            if not init_bootloader(ser):
                print("  Re-sync after unlock failed")
                return True
    return unlocked

def cmd_write_memory(ser, address, data):
    """Write data to memory."""
    if len(data) % 4:
        data += bytes([0xFF]) * (4 - (len(data) % 4))
    ser.reset_input_buffer()
    if not send_command(ser, 0x31):
        return False
    # Send address (4 bytes, MSB first) + checksum
    addr_bytes = struct.pack('>I', address)
    checksum = addr_bytes[0] ^ addr_bytes[1] ^ addr_bytes[2] ^ addr_bytes[3]
    if not _send_raw_payload(ser, addr_bytes + bytes([checksum]), "Write address"):
        return False

    # Send data length (N-1), data, and xor checksum
    n = len(data) - 1
    payload = bytes([n]) + data
    checksum = 0
    for b in payload:
        checksum ^= b
    if _send_raw_payload(ser, payload + bytes([checksum]), "Write data"):
        return True
    return False

def cmd_go(ser, address):
    """Jump to address."""
    if not send_command(ser, 0x21):
        return False
    addr_bytes = struct.pack('>I', address)
    checksum = addr_bytes[0] ^ addr_bytes[1] ^ addr_bytes[2] ^ addr_bytes[3]
    ser.write(addr_bytes + bytes([checksum]))
    resp = ser.read(1)
    if resp and resp[0] == ACK:
        print("  Jump to app: OK")
        return True
    return False

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "COM6"
    firmware = sys.argv[2] if len(sys.argv) > 2 else ".pio/build/gd32e230c8t6/firmware.bin"
    baud = next((int(arg) for arg in sys.argv[3:] if arg.isdigit()), 57600)

    print(f"Connecting to {port} at {baud} 8E1...")
    ser = connect_and_sync(port, baud, timeout_seconds=45)
    if not ser:
        sys.exit(1)

    print("Getting bootloader info...")
    commands = cmd_get(ser)
    if commands is None:
        print("  Warning: could not get bootloader info, continuing anyway")

    print("Getting chip ID...")
    pid = cmd_get_id(ser)
    if pid is None:
        print("  Warning: could not get PID, continuing anyway")

    with open(firmware, 'rb') as f:
        fw_data = f.read()

    print(f"Firmware size: {len(fw_data)} bytes")

    print("Checking protection state...")
    if commands is not None and (0x73 in commands or 0x92 in commands):
        print("  Protection commands are supported, but skipping unlock unless erase/write fails")

    skip_erase = "--no-erase" in sys.argv
    if skip_erase:
        print("Skipping erase (--no-erase)")
    else:
        print("Erasing...")
        # Use erase commands advertised by the bootloader
        if not cmd_erase(ser, commands):
            ser.close()
            sys.exit(1)

        time.sleep(0.5)

    # Write firmware in 256-byte chunks
    FLASH_BASE = 0x08000000
    chunk_size = 256
    total = len(fw_data)

    print(f"Writing firmware ({total} bytes)...")
    for i in range(0, total, chunk_size):
        chunk = fw_data[i:i+chunk_size]
        addr = FLASH_BASE + i
        retries = 3
        while retries > 0:
            if cmd_write_memory(ser, addr, chunk):
                break
            retries -= 1
            if retries > 0:
                print(f"  Retrying at 0x{addr:08X}...")
                time.sleep(0.1)
        if retries == 0:
            print(f"  Failed to write at 0x{addr:08X}")
            ser.close()
            sys.exit(1)
        if (i // chunk_size) % 16 == 0:
            print(f"  Progress: {i*100//total}%")

    print("  Progress: 100%")
    print("Write complete!")

    print("Jumping to application...")
    cmd_go(ser, FLASH_BASE)

    ser.close()
    print("Done!")

if __name__ == '__main__':
    main()