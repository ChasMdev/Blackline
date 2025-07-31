import secrets
import time
from fastapi import FastAPI, HTTPException, Request
from pydantic import BaseModel, EmailStr
from fastapi.middleware.cors import CORSMiddleware
from contextlib import asynccontextmanager
import sqlite3
from pathlib import Path
from utils import generate_access_key, hash_password, verify_password
import logging

logging.basicConfig(level=logging.INFO)
DB_PATH = Path("users.db")

def init_db():
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute("""
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE,
                email TEXT UNIQUE,
                password TEXT,
                access_key TEXT UNIQUE,
                hwid TEXT,
                blacklisted INTEGER DEFAULT 0
            )
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS access_keys (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                key TEXT UNIQUE
            )
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS sessions (
                token TEXT PRIMARY KEY,
                username TEXT,
                expires_at INTEGER  -- Unix timestamp
            )
        """)

def user_exists(username, email):
    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.execute("SELECT 1 FROM users WHERE username = ? OR email = ?", (username, email))
        return cursor.fetchone() is not None

def access_key_exists(key):
    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.execute("SELECT 1 FROM access_keys WHERE key = ?", (key,))
        return cursor.fetchone() is not None

def access_key_in_use(key):
    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.execute("SELECT 1 FROM users WHERE access_key = ?", (key,))
        return cursor.fetchone() is not None

def store_access_key(key):
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute("INSERT INTO access_keys (key) VALUES (?)", (key,))

def create_user(username, email, password, access_key):
    hashed = hash_password(password)
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute("""
            INSERT INTO users (username, email, password, access_key)
            VALUES (?, ?, ?, ?)
        """, (username, email, hashed, access_key))

def get_all_users():
    with sqlite3.connect(DB_PATH) as conn:
        return conn.execute("SELECT id, username, email, password, access_key, hwid, blacklisted FROM users").fetchall()

def get_all_keys():
    with sqlite3.connect(DB_PATH) as conn:
        return conn.execute("SELECT key FROM access_keys").fetchall()

# FastAPI App
@asynccontextmanager
async def lifespan(app: FastAPI):
    init_db()
    logging.info("Server started and DB initialized.")
    yield

app = FastAPI(lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

class SignupData(BaseModel):
    username: str
    email: EmailStr
    password: str
    access_key: str

class LoginData(BaseModel):
    username: str
    password: str

@app.post("/signup")
def signup(data: SignupData):
    if user_exists(data.username, data.email):
        raise HTTPException(status_code=400, detail="User already exists.")
    if not access_key_exists(data.access_key):
        raise HTTPException(status_code=403, detail="Invalid access key.")
    if access_key_in_use(data.access_key):
        raise HTTPException(status_code=403, detail="Access key already used.")
    create_user(data.username, data.email, data.password, data.access_key)
    logging.info(f"New user signed up: {data.username} ({data.email})")
    return {"message": "Signup successful"}

@app.post("/login")
def login(data: LoginData, request: Request):
    hwid = request.headers.get("X-HWID")
    if not hwid:
        raise HTTPException(status_code=400, detail="HWID missing")

    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.execute("SELECT password, access_key, hwid, blacklisted FROM users WHERE username = ?", (data.username,))
        row = cursor.fetchone()

    if not row:
        raise HTTPException(status_code=404, detail="User not found")

    stored_password, access_key, stored_hwid, blacklisted = row

    if not verify_password(data.password, stored_password):
        raise HTTPException(status_code=403, detail="Incorrect password")

    if stored_hwid is None:
        with sqlite3.connect(DB_PATH) as conn:
            conn.execute("UPDATE users SET hwid = ? WHERE username = ?", (hwid, data.username))
    elif stored_hwid != hwid:
        raise HTTPException(status_code=403, detail="Incorrect HWID")

    return {
        "message": "Login successful",
        "token": create_token(data.username),
        "blacklisted": bool(blacklisted),
        "hasKey": bool(access_key)
    }

def create_token(username: str, duration=3600) -> str:
    token = secrets.token_hex(32)
    expires = int(time.time()) + duration
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute("INSERT INTO sessions (token, username, expires_at) VALUES (?, ?, ?)", (token, username, expires))
    return token

@app.post("/token-login")
def token_login(request: Request):
    token = request.headers.get("Authorization")
    hwid = request.headers.get("X-HWID")

    if not token or not hwid:
        raise HTTPException(status_code=400, detail="Missing token or HWID")

    with sqlite3.connect(DB_PATH) as conn:
        result = conn.execute("SELECT username, expires_at FROM sessions WHERE token = ?", (token,)).fetchone()

    if not result:
        raise HTTPException(status_code=401, detail="Invalid or expired token")

    username, expires_at = result
    if time.time() > expires_at:
        raise HTTPException(status_code=401, detail="Token expired")

    with sqlite3.connect(DB_PATH) as conn:
        row = conn.execute("SELECT hwid, blacklisted, access_key FROM users WHERE username = ?", (username,)).fetchone()

    if row[0] is None:
        with sqlite3.connect(DB_PATH) as conn:
            conn.execute("UPDATE users SET hwid = ? WHERE username = ?", (hwid, username))

    if not row or (row[0] and row[0] != hwid):
        raise HTTPException(status_code=403, detail="Incorrect HWID")

    three_days = 3 * 24 * 60 * 60
    if expires_at - time.time() < three_days:
        new_expiry = int(time.time()) + 7 * 24 * 60 * 60
        with sqlite3.connect(DB_PATH) as conn:
            conn.execute("UPDATE sessions SET expires_at = ? WHERE token = ?", (new_expiry, token))

    return {
        "message": "Token login successful",
        "username": username,
        "blacklisted": bool(row[1]),
        "hasKey": bool(row[2])
    }

@app.post("/generate-key")
def generate_key():
    key = generate_access_key()
    store_access_key(key)
    logging.info(f"New access key generated: {key}")
    return {"access_key": key}

@app.delete("/admin/delete-key/{key}")
def delete_key(key: str):
    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.execute("DELETE FROM access_keys WHERE key = ?", (key,))
        if cursor.rowcount == 0:
            raise HTTPException(status_code=404, detail="Key not found.")
    logging.info(f"Deleted access key: {key}")
    return {"message": "Key deleted"}

@app.put("/admin/reset-hwid/{user_id}")
def reset_hwid(user_id: int):
    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.execute("UPDATE users SET hwid = NULL WHERE id = ?", (user_id,))
        if cursor.rowcount == 0:
            raise HTTPException(status_code=404, detail="User not found.")
    return {"message": "HWID reset"}

@app.get("/admin/users")
def admin_users():
    return {"users": get_all_users()}

@app.get("/admin/keys")
def admin_keys():
    return {"keys": get_all_keys()}

@app.delete("/admin/delete-user/{user_id}")
def delete_user(user_id: int):
    with sqlite3.connect(DB_PATH) as conn:
        username_result = conn.execute("SELECT username FROM users WHERE id = ?", (user_id,)).fetchone()
        if username_result is None:
            raise HTTPException(status_code=404, detail="User not found.")
        username = username_result[0]

        conn.execute("DELETE FROM users WHERE id = ?", (user_id,))
        conn.execute("DELETE FROM sessions WHERE username = ?", (username,))
        conn.commit()  # Explicitly commit the transaction
        
    logging.info(f"Deleted user ID: {user_id}, username: {username}")
    return {"message": "User deleted"}

@app.get("/admin/export-users")
def export_users():
    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.execute("SELECT username, email, password, access_key, hwid FROM users")
        users = [
            {
                "username": u[0],
                "email": u[1],
                "password": u[2],
                "access_key": u[3],
                "hwid": u[4]
            }
            for u in cursor.fetchall()
        ]
    return {"users": users}

@app.put("/admin/toggle-blacklist/{user_id}")
def toggle_blacklist(user_id: int):
    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.execute("SELECT blacklisted FROM users WHERE id = ?", (user_id,))
        row = cursor.fetchone()
        if not row:
            raise HTTPException(status_code=404, detail="User not found.")
        new_value = 0 if row[0] else 1
        conn.execute("UPDATE users SET blacklisted = ? WHERE id = ?", (new_value, user_id))
    return {"message": f"User {'blacklisted' if new_value else 'unblacklisted'}."}

class KeyAssignment(BaseModel):
    user_id: int
    key: str

@app.put("/admin/assign-key")
def assign_key(data: KeyAssignment):
    if not access_key_exists(data.key):
        raise HTTPException(status_code=404, detail="Access key not found.")
    if access_key_in_use(data.key):
        raise HTTPException(status_code=400, detail="Access key already in use.")

    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.execute("UPDATE users SET access_key = ? WHERE id = ?", (data.key, data.user_id))
        if cursor.rowcount == 0:
            raise HTTPException(status_code=404, detail="User not found.")
    return {"message": "Access key assigned."}

@app.put("/admin/remove-key/{user_id}")
def remove_key(user_id: int):
    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.execute("UPDATE users SET access_key = NULL WHERE id = ?", (user_id,))
        if cursor.rowcount == 0:
            raise HTTPException(status_code=404, detail="User not found.")
    return {"message": "Access key removed from user."}