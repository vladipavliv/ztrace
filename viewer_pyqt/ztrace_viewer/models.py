"""Data models for ZeroTrace variables."""
from dataclasses import dataclass, field
from typing import Any, List


@dataclass
class Variable:
    name: str
    var_type: str
    value: Any
    order: str
    offset: int
    history: List[float] = field(default_factory=list)
    changed: bool = False

    def update(self, new_value: Any) -> bool:
        """Update value and history. Returns True if value changed."""
        if self.value != new_value:
            self.value = new_value
            self.changed = True
            try:
                self.history.append(float(new_value))
                if len(self.history) > 120:
                    self.history.pop(0)
            except (ValueError, TypeError):
                pass
            return True
        self.changed = False
        return False

    def clear_changed(self):
        self.changed = False
