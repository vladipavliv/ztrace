"""ZeroTrace shared-memory client."""
import multiprocessing.shared_memory as shm
import struct
from typing import List, Optional

from .models import Variable


HEADER_SIZE = 64
MAGIC = 0x5A5452414345
MAX_NAME_LEN = 32

VAR_INT32 = 0
VAR_INT64 = 1
VAR_FLOAT = 2
VAR_DOUBLE = 3
VAR_BOOL = 4


class ZeroTraceClient:
    """Reads ZeroTrace variables from POSIX shared memory 'ztrace_shm'."""

    def __init__(self):
        self._shm: Optional[shm.SharedMemory] = None
        self._buf = None

    @property
    def connected(self) -> bool:
        return self._shm is not None

    def connect(self) -> bool:
        self.disconnect()
        try:
            self._shm = shm.SharedMemory(name="ztrace_shm", create=False)
            self._buf = self._shm.buf
            return True
        except FileNotFoundError:
            return False
        except Exception as e:
            print(f"[ZeroTrace] Connection error: {e}")
            return False

    def disconnect(self):
        if self._shm:
            try:
                self._shm.close()
            except Exception:
                pass
            self._shm = None
            self._buf = None

    def _read_header(self) -> Optional[dict]:
        if not self.connected:
            return None
        try:
            magic = struct.unpack_from("<Q", self._buf, 0)[0]
            if magic != MAGIC:
                return None
            return {
                "magic": magic,
                "version": struct.unpack_from("<I", self._buf, 8)[0],
                "total_size": struct.unpack_from("<Q", self._buf, 16)[0],
                "used_size": struct.unpack_from("<Q", self._buf, 24)[0],
                "var_count": struct.unpack_from("<I", self._buf, 32)[0],
            }
        except Exception as e:
            print(f"[ZeroTrace] Header error: {e}")
            return None

    def scan(self) -> Optional[List[Variable]]:
        """Scan shared memory and return list of variables.
        Returns None if header is invalid (needs reconnect).
        """
        header = self._read_header()
        if not header:
            return None

        variables: List[Variable] = []
        offset = HEADER_SIZE
        used = header["used_size"]
        buf_len = len(self._buf)

        if used > buf_len:
            return []

        while offset + 64 <= used:
            name_bytes = bytes(self._buf[offset : offset + MAX_NAME_LEN])
            name = name_bytes.split(b"\x00", 1)[0].decode("utf-8", errors="replace")
            if not name:
                offset += 64
                continue

            var_type = self._buf[offset + 32]
            order = self._buf[offset + 33]

            if var_type == VAR_INT32:
                value = struct.unpack_from("<i", self._buf, offset + 36)[0]
                tname = "int32"
            elif var_type == VAR_INT64:
                value = struct.unpack_from("<q", self._buf, offset + 40)[0]
                tname = "int64"
            elif var_type == VAR_FLOAT:
                value = struct.unpack_from("<f", self._buf, offset + 36)[0]
                tname = "float"
            elif var_type == VAR_DOUBLE:
                value = struct.unpack_from("<d", self._buf, offset + 40)[0]
                tname = "double"
            elif var_type == VAR_BOOL:
                value = bool(self._buf[offset + 34])
                tname = "bool"
            else:
                value = None
                tname = f"unknown({var_type})"

            order_str = "acq_rel" if order == 1 else "relaxed"

            variables.append(Variable(
                name=name,
                var_type=tname,
                value=value,
                order=order_str,
                offset=offset,
            ))
            offset += 64

        return variables
