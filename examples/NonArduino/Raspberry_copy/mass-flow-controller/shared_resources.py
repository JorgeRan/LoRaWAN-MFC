import serial
import propar

# Shared configuration
PORT = '/dev/ttyUSB0'
BAUD = 38400
TIMEOUT = 1

# Internal singletons
_serial = None
_bus = None
_instruments = {}


def get_serial():
    """Return a single shared `serial.Serial` instance (opens once)."""
    global _serial
    if _serial is None:
        _serial = serial.Serial(port=PORT, baudrate=BAUD, timeout=TIMEOUT)
    return _serial


def get_bus():
    """Return a single shared propar bus (calls propar.instrument once)."""
    global _bus
    if _bus is None:
        _bus = propar.instrument(PORT)
    return _bus


def get_instrument(address):
    """Return a cached per-address instrument created via propar.instrument.

    Instruments are cached so repeated calls don't re-open the port
    or re-create the same object multiple times.
    """
    global _instruments
    if address in _instruments:
        return _instruments[address]
    inst = propar.instrument(PORT, address=address)
    _instruments[address] = inst
    return inst


def close_all():
    """Close any opened resources. Safe to call on shutdown."""
    global _serial, _bus, _instruments
    try:
        if _serial is not None:
            try:
                _serial.close()
            except Exception:
                pass
    finally:
        _serial = None

    _instruments = {}
    _bus = None
