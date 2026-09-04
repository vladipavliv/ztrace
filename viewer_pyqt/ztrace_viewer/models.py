"""Data models for ztrace variables."""
from dataclasses import dataclass, field
from typing import Any, List

@dataclass
class Variable:
    name: str
    var_type: str
    value: Any
    order: str
    offset: int
    update_rate: int
    history: List[float] = field(default_factory=list)
    changed: bool = False
    min_value: Any = None
    max_value: Any = None

    def update(self, new_value: Any) -> bool:
        """Update value and statistics. Returns True if value changed."""
        if self.value != new_value:
            self.value = new_value
            self.changed = True

            try:
                numeric_value = float(new_value)

                self.history.append(numeric_value)
                if len(self.history) > 120:
                    self.history.pop(0)

                if self.min_value is None or numeric_value < self.min_value:
                    self.min_value = numeric_value

                if self.max_value is None or numeric_value > self.max_value:
                    self.max_value = numeric_value

            except (ValueError, TypeError):
                pass

            return True

        self.changed = False
        return False

    def clear_changed(self):
        self.changed = False