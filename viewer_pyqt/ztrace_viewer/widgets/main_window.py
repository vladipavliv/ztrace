"""Main application window."""
from PyQt6.QtWidgets import QMainWindow, QWidget, QVBoxLayout
from PyQt6.QtCore import QTimer

from ..client import ZeroTraceClient
from .toolbar import Toolbar
from .variable_table import VariableTableView, VariableTableModel


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("ZeroTrace Monitor")
        self.setMinimumSize(1000, 650)
        self.resize(1200, 800)

        self.client = ZeroTraceClient()
        self.model = VariableTableModel()
        self.table = VariableTableView()
        self.table.setModel(self.model)

        header = self.table.horizontalHeader()

        self.table.setColumnWidth(0, 100)  # Name
        self.table.setColumnWidth(1, 100)  # Value
        self.table.setColumnWidth(2, 70)   # Type
        self.table.setColumnWidth(3, 70)   # Min
        self.table.setColumnWidth(4, 70)   # Max
        self.table.setColumnWidth(5, 70)   # Rate
        self.table.setColumnWidth(6, 70)   # Order
        self.table.setColumnWidth(7, 70)   # Offset

        self.toolbar = Toolbar()
        self.toolbar.interval_changed.connect(self._set_interval)
        self.toolbar.toggle_scan.connect(self._set_scanning)

        central = QWidget()
        layout = QVBoxLayout(central)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(self.toolbar)

        table_container = QWidget()
        table_layout = QVBoxLayout(table_container)
        table_layout.setContentsMargins(16, 16, 16, 16)
        table_layout.setSpacing(0)
        table_layout.addWidget(self.table)

        layout.addWidget(table_container)

        self.setCentralWidget(central)

        self._scanning = False
        self._interval_ms = 100

        # Timer that tries to connect every 500 ms
        self._connect_timer = QTimer(self)
        self._connect_timer.timeout.connect(self._try_connect)
        self._connect_timer.start(500)

        # Timer that updates the table
        self._update_timer = QTimer(self)
        self._update_timer.timeout.connect(self._update)

        self._apply_styles()

    def _apply_styles(self):
        self.setStyleSheet("""
            QMainWindow {
                background-color: #1e272e;
            }
            QWidget {
                background-color: #1e272e;
            }
        """)

    def _try_connect(self):
        if self.client.connected:
            return
        if self.client.connect():
            self.toolbar.set_status("Connected", "#00d4aa")
            if not self._scanning:
                self._set_scanning(True)
        else:
            self.toolbar.set_status("Waiting for ztrace_shm…", "#e17055")

    def _set_interval(self, ms: int):
        self._interval_ms = max(10, ms)
        if self._update_timer.isActive():
            self._update_timer.start(self._interval_ms)

    def _set_scanning(self, active: bool):
        self._scanning = active
        self.toolbar.set_running(active)
        if active:
            self._update_timer.start(self._interval_ms)
        else:
            self._update_timer.stop()

    def _update(self):
        if not self.client.connected:
            self.toolbar.set_status("Disconnected", "#ff6b6b")
            self._set_scanning(False)
            return

        try:
            variables = self.client.scan()
        except Exception as e:
            print(f"[Update] {e}")
            variables = None

        if variables is None:
            self.toolbar.set_status("Lost connection — reconnecting…", "#ff6b6b")
            self.client.disconnect()
            self.model.set_variables([])
            self._set_scanning(False)
            return

        self.model.sync_variables(variables)
        self.toolbar.set_status(
            f"Connected  •  {len(variables)} variables  •  {self._interval_ms} ms",
            "#00d4aa"
        )

    def closeEvent(self, event):
        self._update_timer.stop()
        self._connect_timer.stop()
        self.client.disconnect()
        event.accept()
