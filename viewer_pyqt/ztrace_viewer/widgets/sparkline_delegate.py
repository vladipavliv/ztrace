"""Table delegate that draws a sparkline in a cell."""
from PyQt6.QtWidgets import QStyledItemDelegate
from PyQt6.QtCore import Qt, QPointF
from PyQt6.QtGui import QPainter, QPen, QColor, QBrush


class SparklineDelegate(QStyledItemDelegate):
    def paint(self, painter, option, index):
        data = index.data(Qt.ItemDataRole.UserRole)
        if not data or len(data) < 2:
            painter.save()
            painter.setPen(Qt.PenStyle.NoPen)
            painter.setBrush(QBrush(QColor("#2d3436")))
            painter.drawRect(option.rect)
            painter.restore()
            return

        painter.save()
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)

        rect = option.rect.adjusted(6, 4, -6, -4)
        w = rect.width()
        h = rect.height()

        min_val = min(data)
        max_val = max(data)
        rng = max_val - min_val if max_val != min_val else 1.0

        points = []
        for i, v in enumerate(data):
            x = rect.left() + (i / (len(data) - 1)) * w
            y = rect.bottom() - ((v - min_val) / rng) * h
            points.append((x, y))

        # Fill area under line
        fill_color = QColor("#00d4aa")
        fill_color.setAlpha(30)
        painter.setBrush(QBrush(fill_color))
        painter.setPen(Qt.PenStyle.NoPen)

        poly = [QPointF(points[0][0], points[0][1])]
        for p in points[1:]:
            poly.append(QPointF(p[0], p[1]))
        poly.append(QPointF(points[-1][0], rect.bottom()))
        poly.append(QPointF(points[0][0], rect.bottom()))
        painter.drawPolygon(poly)

        # Line
        pen = QPen(QColor("#00d4aa"))
        pen.setWidth(2)
        pen.setCapStyle(Qt.PenCapStyle.RoundCap)
        pen.setJoinStyle(Qt.PenJoinStyle.RoundJoin)
        painter.setPen(pen)
        painter.setBrush(Qt.BrushStyle.NoBrush)

        for i in range(1, len(points)):
            painter.drawLine(
                int(points[i - 1][0]), int(points[i - 1][1]),
                int(points[i][0]), int(points[i][1])
            )

        painter.restore()
