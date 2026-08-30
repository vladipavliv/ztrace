"""Variable table with QAbstractTableModel and sparkline delegate."""
from PyQt6.QtWidgets import QTableView, QAbstractItemView
from PyQt6.QtCore import Qt, QAbstractTableModel, QModelIndex
from PyQt6.QtGui import QColor, QBrush, QFont

from ..models import Variable
from .sparkline_delegate import SparklineDelegate


class VariableTableModel(QAbstractTableModel):
    COLUMNS = ["Name", "Type", "Value", "Order", "Offset", "History"]

    def __init__(self, parent=None):
        super().__init__(parent)
        self._vars: list[Variable] = []
        self._highlight = QColor("#ff9f43")
        self._highlight.setAlpha(90)
        self._bg1 = QColor("#2d3436")
        self._bg2 = QColor("#353b48")

    def set_variables(self, variables: list[Variable]):
        self.beginResetModel()
        self._vars = variables
        self.endResetModel()

    def sync_variables(self, new_vars: list[Variable]):
        """Smart merge: keep history, add/remove rows, highlight changes."""
        new_map = {v.name: v for v in new_vars}
        existing_names = {v.name for v in self._vars}

        # 1. Clear old highlights
        for row, v in enumerate(self._vars):
            if v.changed:
                v.clear_changed()
                self.dataChanged.emit(
                    self.index(row, 0),
                    self.index(row, len(self.COLUMNS) - 1),
                    [Qt.ItemDataRole.BackgroundRole]
                )

        # 2. Remove gone variables
        for i in range(len(self._vars) - 1, -1, -1):
            if self._vars[i].name not in new_map:
                self.beginRemoveRows(QModelIndex(), i, i)
                self._vars.pop(i)
                self.endRemoveRows()

        # 3. Update existing
        for row, v in enumerate(self._vars):
            if v.name in new_map:
                changed = v.update(new_map[v.name].value)
                if changed:
                    self.dataChanged.emit(
                        self.index(row, 0),
                        self.index(row, len(self.COLUMNS) - 1),
                        [Qt.ItemDataRole.DisplayRole,
                         Qt.ItemDataRole.BackgroundRole,
                         Qt.ItemDataRole.UserRole]
                    )

        # 4. Add new variables
        for nv in new_vars:
            if nv.name not in existing_names:
                self.beginInsertRows(QModelIndex(), len(self._vars), len(self._vars))
                self._vars.append(nv)
                self.endInsertRows()

    def rowCount(self, parent=QModelIndex()):
        return len(self._vars)

    def columnCount(self, parent=QModelIndex()):
        return len(self.COLUMNS)

    def headerData(self, section, orientation, role):
        if orientation == Qt.Orientation.Horizontal and role == Qt.ItemDataRole.DisplayRole:
            return self.COLUMNS[section]
        return None

    def data(self, index, role=Qt.ItemDataRole.DisplayRole):
        if not index.isValid() or index.row() >= len(self._vars):
            return None

        var = self._vars[index.row()]
        col = index.column()

        if role == Qt.ItemDataRole.DisplayRole:
            return [
                var.name,
                var.var_type,
                str(var.value),
                var.order,
                f"0x{var.offset:X}",
                f"{len(var.history)} pts"
            ][col]

        if role == Qt.ItemDataRole.TextAlignmentRole:
            if col == 0:
                return Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter
            if col == 2:
                return Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter
            return Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter

        if role == Qt.ItemDataRole.BackgroundRole:
            if var.changed:
                return QBrush(self._highlight)
            return QBrush(self._bg2 if index.row() % 2 else self._bg1)

        if role == Qt.ItemDataRole.ForegroundRole:
            return QBrush(QColor("#dfe6e9"))

        if role == Qt.ItemDataRole.FontRole:
            font = QFont("JetBrains Mono", 10)
            if col == 2:
                font.setBold(True)
            return font

        if role == Qt.ItemDataRole.UserRole and col == 5:
            return var.history

        return None


class VariableTableView(QTableView):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self.setSelectionMode(QAbstractItemView.SelectionMode.SingleSelection)
        self.verticalHeader().setVisible(False)
        self.horizontalHeader().setStretchLastSection(True)
        self.horizontalHeader().setDefaultAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setShowGrid(False)
        self.setStyleSheet("""
            QTableView {
                background-color: #2d3436;
                border: none;
                outline: none;
            }
            QHeaderView::section {
                background-color: #1e272e;
                color: #00d4aa;
                padding: 10px 8px;
                border: none;
                border-bottom: 2px solid #00d4aa;
                font-weight: bold;
                font-size: 12px;
            }
            QTableView::item {
                padding: 8px;
                border-bottom: 1px solid #404a52;
            }
            QTableView::item:selected {
                background-color: #0984e3;
                color: white;
            }
        """)
        self.setColumnWidth(0, 180)
        self.setColumnWidth(1, 80)
        self.setColumnWidth(2, 140)
        self.setColumnWidth(3, 80)
        self.setColumnWidth(4, 90)
        self.setColumnWidth(5, 160)

        self.sparkline_delegate = SparklineDelegate(self)
        self.setItemDelegateForColumn(5, self.sparkline_delegate)
