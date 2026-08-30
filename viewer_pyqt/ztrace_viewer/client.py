"""ZeroTrace shared-memory client."""
from multiprocessing import shared_memory as shm
from multiprocessing import resource_tracker
import struct
from typing import List, Optional

from .models import Variable


HEADER_SIZE = 64

VARIABLE_SIZE = 64

VALUE_OFFSET = 0
NAME_OFFSET = 8
UPDATE_RATE_OFFSET = 40
TYPE_OFFSET = 44
ORDER_OFFSET = 45

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
            resource_tracker.unregister(self._shm._name, "shared_memory")

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
                "total_size": struct.unpack_from("<I", self._buf, 12)[0],
                "used_size": struct.unpack_from("<I", self._buf, 16)[0],
                "var_count": struct.unpack_from("<I", self._buf, 20)[0],
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

        while offset + VARIABLE_SIZE <= used:
            name_bytes = bytes(
                self._buf[offset + NAME_OFFSET:
                        offset + NAME_OFFSET + MAX_NAME_LEN]
            )
            name = name_bytes.split(b"\x00", 1)[0].decode(
                "utf-8", errors="replace"
            )

            if not name:
                offset += VARIABLE_SIZE
                continue

            var_type = self._buf[offset + TYPE_OFFSET]
            order = self._buf[offset + ORDER_OFFSET]

            if var_type == VAR_INT32:
                value = struct.unpack_from(
                    "<i", self._buf, offset + VALUE_OFFSET
                )[0]
                tname = "int32"

            elif var_type == VAR_INT64:
                value = struct.unpack_from(
                    "<q", self._buf, offset + VALUE_OFFSET
                )[0]
                tname = "int64"

            elif var_type == VAR_FLOAT:
                value = struct.unpack_from(
                    "<f", self._buf, offset + VALUE_OFFSET
                )[0]
                tname = "float"

            elif var_type == VAR_DOUBLE:
                value = struct.unpack_from(
                    "<d", self._buf, offset + VALUE_OFFSET
                )[0]
                tname = "double"

            elif var_type == VAR_BOOL:
                value = bool(self._buf[offset + VALUE_OFFSET])
                tname = "bool"

            else:
                value = None
                tname = f"unknown({var_type})"

            update_rate = struct.unpack_from(
                "<i", self._buf, offset + UPDATE_RATE_OFFSET
            )[0]

            order_str = "acq_rel" if order == 1 else "relaxed"

            var = Variable(
                name=name,
                var_type=tname,
                value=value,
                order=order_str,
                offset=offset,
                update_rate=update_rate,
            )

            try:
                numeric_value = float(value)
                var.history.append(numeric_value)
                var.min_value = numeric_value
                var.max_value = numeric_value
            except (ValueError, TypeError):
                pass

            variables.append(var)

            offset += VARIABLE_SIZE

        return variables
