from PySide6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, QLabel, QLineEdit,
    QPushButton, QTextEdit, QTabWidget, QListWidget, QInputDialog, QMessageBox
)
import requests

class ControlPanel(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Control Panel")
        self.setGeometry(100, 100, 1000, 600)

        layout = QVBoxLayout()
        tabs = QTabWidget()

        tabs.addTab(self.signup_tab(), "Signup")
        tabs.addTab(self.admin_tab(), "Admin")
        layout.addWidget(tabs)
        self.setLayout(layout)

    def signup_tab(self):
        tab = QWidget()
        layout = QVBoxLayout()

        self.username_input = QLineEdit()
        self.username_input.setPlaceholderText("Username")
        self.email_input = QLineEdit()
        self.email_input.setPlaceholderText("Email")
        self.password_input = QLineEdit()
        self.password_input.setPlaceholderText("Password")
        self.password_input.setEchoMode(QLineEdit.Password)
        self.access_key_input = QLineEdit()
        self.access_key_input.setPlaceholderText("Access Key")

        generate_btn = QPushButton("Generate Access Key")
        generate_btn.clicked.connect(self.get_key)
        submit_btn = QPushButton("Sign Up")
        submit_btn.clicked.connect(self.signup)

        self.signup_log = QTextEdit()
        self.signup_log.setReadOnly(True)

        for widget in [self.username_input, self.email_input, self.password_input,
                       self.access_key_input, generate_btn, submit_btn, self.signup_log]:
            layout.addWidget(widget)

        tab.setLayout(layout)
        return tab

    def admin_tab(self):
        tab = QWidget()
        layout = QHBoxLayout()

        left_panel = QVBoxLayout()
        right_panel = QVBoxLayout()

        self.user_list = QListWidget()
        self.key_list = QListWidget()
        self._key_map = {}
        self._user_map = {}

        left_panel.addWidget(QLabel("Users:"))
        left_panel.addWidget(self.user_list)
        left_panel.addWidget(QLabel("Access Keys:"))
        left_panel.addWidget(self.key_list)

        # Buttons
        refresh_btn = QPushButton("Refresh")
        refresh_btn.clicked.connect(self.load_admin_data)
        self.delete_user_btn = QPushButton("Delete User")
        self.delete_user_btn.clicked.connect(self.delete_selected_user)
        self.delete_key_btn = QPushButton("Delete Key")
        self.delete_key_btn.clicked.connect(self.delete_selected_key)
        self.reset_hwid_btn = QPushButton("Reset HWID")
        self.reset_hwid_btn.clicked.connect(self.reset_selected_hwid)
        self.export_btn = QPushButton("Export Users")
        self.export_btn.clicked.connect(self.export_users)
        self.toggle_blacklist_btn = QPushButton("Toggle Blacklist")
        self.toggle_blacklist_btn.clicked.connect(self.toggle_blacklist)
        self.assign_key_btn = QPushButton("Assign Key to User")
        self.assign_key_btn.clicked.connect(self.assign_key_to_user)
        self.remove_key_btn = QPushButton("Remove Key from User")
        self.remove_key_btn.clicked.connect(self.remove_key_from_user)

        for btn in [refresh_btn, self.delete_user_btn, self.delete_key_btn,
                    self.reset_hwid_btn, self.export_btn, self.toggle_blacklist_btn,
                    self.assign_key_btn, self.remove_key_btn]:
            right_panel.addWidget(btn)

        layout.addLayout(left_panel, 2)
        layout.addLayout(right_panel, 1)
        tab.setLayout(layout)
        return tab

    def get_key(self):
        try:
            r = requests.post("http://127.0.0.1:8000/generate-key")
            self.access_key_input.setText(r.json()["access_key"])
            self.signup_log.append(f"✅ New key: {r.json()['access_key']}")
        except Exception as e:
            self.signup_log.append(f"⚠️ Key error: {e}")

    def signup(self):
        data = {
            "username": self.username_input.text(),
            "email": self.email_input.text(),
            "password": self.password_input.text(),
            "access_key": self.access_key_input.text()
        }
        try:
            r = requests.post("http://127.0.0.1:8000/signup", json=data)
            if r.ok:
                self.signup_log.append("✅ Signup successful")
            else:
                self.signup_log.append(f"❌ {r.json()['detail']}")
        except Exception as e:
            self.signup_log.append(f"⚠️ Signup failed: {e}")

    def load_admin_data(self):
        try:
            users = requests.get("http://127.0.0.1:8000/admin/users").json()["users"]
            keys = requests.get("http://127.0.0.1:8000/admin/keys").json()["keys"]
            self.user_list.clear()
            self.key_list.clear()
            self._user_map.clear()
            self._key_map.clear()

            for u in users:
                uid, uname, email, pw, key, hwid, blacklisted = u
                user_display = f"ID:{uid} | {uname} | {email} | KEY: {key or 'None'} | HWID: {hwid or 'None'} | BL: {'Yes' if blacklisted else 'No'}"
                self.user_list.addItem(user_display)
                self._user_map[user_display] = uid

            for k in keys:
                self.key_list.addItem(k[0])
                self._key_map[k[0]] = k[0]
        except Exception as e:
            self.user_list.addItem(f"Error loading users: {e}")

    def get_selected_user_id(self):
        selected = self.user_list.currentItem()
        return self._user_map.get(selected.text()) if selected else None

    def delete_selected_user(self):
        uid = self.get_selected_user_id()
        if not uid: return
        try:
            r = requests.delete(f"http://127.0.0.1:8000/admin/delete-user/{uid}")
            self.load_admin_data()
        except Exception as e:
            self.user_list.addItem(f"⚠️ Delete error: {e}")

    def delete_selected_key(self):
        selected = self.key_list.currentItem()
        key = self._key_map.get(selected.text()) if selected else None
        if not key: return
        try:
            requests.delete(f"http://127.0.0.1:8000/admin/delete-key/{key}")
            self.load_admin_data()
        except Exception as e:
            self.key_list.addItem(f"⚠️ Key delete error: {e}")

    def reset_selected_hwid(self):
        uid = self.get_selected_user_id()
        if not uid: return
        try:
            requests.put(f"http://127.0.0.1:8000/admin/reset-hwid/{uid}")
            self.load_admin_data()
        except Exception as e:
            self.user_list.addItem(f"⚠️ HWID reset error: {e}")

    def export_users(self):
        try:
            r = requests.get("http://127.0.0.1:8000/admin/export-users")
            with open("exported_users.json", "w", encoding="utf-8") as f:
                import json
                json.dump(r.json(), f, indent=2)
        except Exception as e:
            self.user_list.addItem(f"⚠️ Export error: {e}")

    def toggle_blacklist(self):
        uid = self.get_selected_user_id()
        if not uid: return
        try:
            requests.put(f"http://127.0.0.1:8000/admin/toggle-blacklist/{uid}")
            self.load_admin_data()
        except Exception as e:
            self.user_list.addItem(f"⚠️ Blacklist error: {e}")

    def assign_key_to_user(self):
        uid = self.get_selected_user_id()
        if not uid: return
        key, ok = QInputDialog.getText(self, "Assign Key", "Enter key to assign:")
        if not ok or not key: return
        try:
            r = requests.put("http://127.0.0.1:8000/admin/assign-key", json={"user_id": uid, "key": key})
            if r.ok:
                self.load_admin_data()
            else:
                QMessageBox.warning(self, "Failed", r.json()["detail"])
        except Exception as e:
            QMessageBox.critical(self, "Error", str(e))

    def remove_key_from_user(self):
        uid = self.get_selected_user_id()
        if not uid: return
        try:
            requests.put(f"http://127.0.0.1:8000/admin/remove-key/{uid}")
            self.load_admin_data()
        except Exception as e:
            QMessageBox.critical(self, "Error", str(e))
