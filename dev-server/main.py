import threading
import uvicorn
from PySide6.QtWidgets import QApplication
import sys
from ui import ControlPanel
from server import app

def run_server():
    uvicorn.run(app, host="127.0.0.1", port=8000, log_level="info")

if __name__ == "__main__":
    threading.Thread(target=run_server, daemon=True).start()
    app_qt = QApplication(sys.argv)
    window = ControlPanel()
    window.show()
    sys.exit(app_qt.exec())
