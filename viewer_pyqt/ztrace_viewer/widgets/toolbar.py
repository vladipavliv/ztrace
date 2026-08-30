"""Top toolbar with interval control and status."""
from PyQt6.QtWidgets import QWidget, QHBoxLayout, QLineEdit, QPushButton, QLabel
from PyQt6.QtCore import pyqtSignal, Qt
from PyQt6.QtGui import QIntValidator

class Toolbar(QWidget):
    interval_changed = pyqtSignal(int)
    toggle_scan = pyqtSignal(bool)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._setup_ui()

    def _setup_ui(self):
        layout = QHBoxLayout(self)
        layout.setContentsMargins(16, 10, 16, 10)
        layout.setSpacing(14)

        self.lbl_interval = QLabel("Interval (ms):")
        self.lbl_interval.setStyleSheet("color: #b2bec3; font-size: 13px;")
        layout.addWidget(self.lbl_interval)

        self.edit_interval = QLineEdit("100")
        self.edit_interval.setFixedWidth(70)

        validator = QIntValidator(1, 60000, self)
        self.edit_interval.setValidator(validator)

        self.edit_interval.setStyleSheet("""
            QLineEdit {
                background-color: #2d3436;
                color: #dfe6e9;
                border: 1px solid #353b48;
                border-radius: 5px;
                padding: 5px 10px;
                font-family: 'JetBrains Mono', 'Fira Code', monospace;
                font-size: 12px;
            }
            QLineEdit:focus {
                border: 1px solid #00d4aa;
            }
        """)
        self.edit_interval.returnPressed.connect(self._on_interval_changed)
        layout.addWidget(self.edit_interval)

        self.btn_toggle = QPushButton("▶  Start")
        self.btn_toggle.setFixedWidth(110)
        self.btn_toggle.setCursor(Qt.CursorShape.PointingHandCursor)
        self._set_start_style()
        self.btn_toggle.clicked.connect(self._on_toggle)
        layout.addWidget(self.btn_toggle)

        layout.addStretch()

        self.lbl_status = QLabel("Waiting for ztrace_shm…")
        self.lbl_status.setStyleSheet("color: #636e72; font-size: 13px;")
        layout.addWidget(self.lbl_status)

    def _set_start_style(self):
        self.btn_toggle.setStyleSheet("""
            QPushButton {
                background-color: #00d4aa;
                color: #1e272e;
                border: none;
                border-radius: 5px;
                padding: 7px 18px;
                font-weight: bold;
                font-size: 12px;
            }
            QPushButton:hover {
                background-color: #00b894;
            }
            QPushButton:pressed {
                background-color: #009e7f;
            }
        """)

    def _set_stop_style(self):
        self.btn_toggle.setStyleSheet("""
            QPushButton {
                background-color: #ff6b6b;
                color: #ffffff;
                border: none;
                border-radius: 5px;
                padding: 7px 18px;
                font-weight: bold;
                font-size: 12px;
            }
            QPushButton:hover {
                background-color: #ee5a5a;
            }
            QPushButton:pressed {
                background-color: #d63031;
            }
        """)

    def _on_interval_changed(self):
        text = self.edit_interval.text()

        if not text:
            return

        value = int(text)
        self.interval_changed.emit(value)

    def _on_toggle(self):
        running = self.btn_toggle.text().startswith("⏹")
        if running:
            self.btn_toggle.setText("▶  Start")
            self._set_start_style()
            self.toggle_scan.emit(False)
        else:
            self.btn_toggle.setText("⏹  Stop")
            self._set_stop_style()
            self.toggle_scan.emit(True)

    def set_running(self, running: bool):
        if running:
            self.btn_toggle.setText("⏹  Stop")
            self._set_stop_style()
        else:
            self.btn_toggle.setText("▶  Start")
            self._set_start_style()

    def set_status(self, text: str, color: str = "#636e72"):
        self.lbl_status.setText(text)
        self.lbl_status.setStyleSheet(f"color: {color}; font-size: 13px;")
