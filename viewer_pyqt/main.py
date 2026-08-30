#!/usr/bin/env python3
"""ZeroTrace Monitor — entry point."""
import sys
from PyQt6.QtWidgets import QApplication
from PyQt6.QtCore import Qt

from ztrace_viewer.widgets.main_window import MainWindow


def main():
    # HiDPI support
    QApplication.setHighDpiScaleFactorRoundingPolicy(
        Qt.HighDpiScaleFactorRoundingPolicy.PassThrough
    )
    app = QApplication(sys.argv)
    app.setStyle("Fusion")

    window = MainWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
