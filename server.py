import sys
import socket
import json
import threading
from PyQt6.QtWidgets import (QApplication, QMainWindow, QTableView, QVBoxLayout, 
                             QWidget, QMenu, QInputDialog, QHBoxLayout, QPushButton)
from PyQt6.QtCore import Qt, QAbstractTableModel, QSortFilterProxyModel, pyqtSignal, QObject
from PyQt6.QtGui import QAction

# --- 通信シグナル ---
class CommunicationSignals(QObject):
    data_received = pyqtSignal(dict)

# --- TCP サーバー (Archicadからの受信) ---
class TcpServerThread(threading.Thread):
    def __init__(self, signals, host='127.0.0.1', port=5000):
        super().__init__(daemon=True)
        self.signals = signals
        self.host = host
        self.port = port

    def run(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.bind((self.host, self.port))
            s.listen()
            while True:
                conn, addr = s.accept()
                with conn:
                    data = conn.recv(1024 * 1024)
                    if data:
                        try:
                            payload = json.loads(data.decode('utf-8'))
                            self.signals.data_received.emit(payload)
                        except Exception as e:
                            print(f"Server Error: {e}")

# --- テーブルモデル (編集可能) ---
class ElementModel(QAbstractTableModel):
    def __init__(self, data=None):
        super().__init__()
        self._data = data or [] # 辞書のリスト [{'guid':..., 'type':..., 'floor':...}]
        self._headers = ["GUID", "Type", "Floor (Editable)"]

    def rowCount(self, parent=None):
        return len(self._data)

    def columnCount(self, parent=None):
        return len(self._headers)

    def data(self, index, role=Qt.ItemDataRole.DisplayRole):
        if role in (Qt.ItemDataRole.DisplayRole, Qt.ItemDataRole.EditRole):
            row = self._data[index.row()]
            col = index.column()
            if col == 0: return row.get('guid', "")
            if col == 1: return row.get('type', "")
            if col == 2: return str(row.get('floor', 0))
        return None

    def setData(self, index, value, role=Qt.ItemDataRole.EditRole):
        if role == Qt.ItemDataRole.EditRole and index.column() == 2:
            try:
                self._data[index.row()]['floor'] = int(value)
                self.dataChanged.emit(index, index)
                return True
            except ValueError:
                return False
        return False

    def flags(self, index):
        base_flags = super().flags(index)
        if index.column() == 2:
            return base_flags | Qt.ItemFlag.ItemIsEditable
        return base_flags

    def headerData(self, section, orientation, role=Qt.ItemDataRole.DisplayRole):
        if role == Qt.ItemDataRole.DisplayRole and orientation == Qt.Orientation.Horizontal:
            return self._headers[section]
        return None

    def update_data(self, new_elements):
        self.beginResetModel()
        self._data = new_elements
        self.endResetModel()

# --- フィルタプロキシ ---
class FilterProxyModel(QSortFilterProxyModel):
    def __init__(self):
        super().__init__()
        self.numeric_filters = {}

    def set_numeric_filter(self, col, filter_tuple):
        self.numeric_filters[col] = filter_tuple
        self.invalidateFilter()

    def filterAcceptsRow(self, source_row, source_parent):
        for col, (ftype, *fvals) in self.numeric_filters.items():
            idx = self.sourceModel().index(source_row, col, source_parent)
            try:
                val = float(idx.data())
                if ftype == "gte" and not (val >= fvals[0]): return False
                if ftype == "lte" and not (val <= fvals[0]): return False
                if ftype == "eq" and not (val == fvals[0]): return False
                if ftype == "range" and not (fvals[0] <= val <= fvals[1]): return False
            except: pass
        return super().filterAcceptsRow(source_row, source_parent)

# --- メインウィンドウ ---
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Archicad - PyQt 双方向連携 (詳細版)")
        self.resize(800, 500)

        self.table = QTableView()
        self.model = ElementModel()
        self.proxy = FilterProxyModel()
        self.proxy.setSourceModel(self.model)
        self.table.setModel(self.proxy)
        self.table.setSortingEnabled(True)

        # コンテキストメニュー
        self.table.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.table.customContextMenuRequested.connect(self.show_filter_menu)

        # 反映ボタン
        self.btn_update = QPushButton("変更をArchicadへ反映 (未実装)")
        self.btn_update.clicked.connect(self.send_update_to_archicad)

        layout = QVBoxLayout()
        layout.addWidget(self.table)
        layout.addWidget(self.btn_update)
        
        container = QWidget()
        container.setLayout(layout)
        self.setCentralWidget(container)

        self.signals = CommunicationSignals()
        self.signals.data_received.connect(self.on_data_received)
        self.server = TcpServerThread(self.signals)
        self.server.start()

    def on_data_received(self, data):
        print("Data received from Archicad!")
        elements = data.get("elements", [])
        self.model.update_data(elements)
        
        # 確実に表示させるための処理
        self.setWindowState(self.windowState() & ~Qt.WindowState.WindowMinimized | Qt.WindowState.WindowActive)
        self.show()
        self.raise_()
        self.activateWindow()
        print(f"Showing window with {len(elements)} elements.")

    def send_update_to_archicad(self):
        # 編集されたデータをJSON化して送信
        data_to_send = {"command": "update_elements", "elements": self.model._data}
        json_payload = json.dumps(data_to_send)
        
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect(('127.0.0.1', 5001))
                s.sendall(json_payload.encode('utf-8'))
            print("Successfully sent update to Archicad.")
        except Exception as e:
            print(f"Failed to send update to Archicad: {e}")

    def show_filter_menu(self, pos):
        index = self.table.indexAt(pos)
        if not index.isValid(): return
        col = index.column()
        global_pos = self.table.viewport().mapToGlobal(pos)
        menu = QMenu(self)
        # (以前と同様のフィルタメニュー項目)
        menu.addAction("数値フィルタ (略)").setEnabled(False)
        menu.exec(global_pos)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setQuitOnLastWindowClosed(False) # ウィンドウを閉じてもサーバーは維持
    window = MainWindow()
    # 起動直後は window.show() を呼ばない
    sys.exit(app.exec())
