import sqlite3 from "sqlite3";
import { open } from "sqlite";
import path from "path";

let dbInstance: any = null;

export async function getDb() {
  if (!dbInstance) {
    dbInstance = await open({
      filename: path.join(__dirname, "../users.db"),
      driver: sqlite3.Database,
    });
  }
  return dbInstance;
}

export async function initDb() {
  const db = await getDb();

  await db.exec(`
    CREATE TABLE IF NOT EXISTS users (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      username TEXT UNIQUE,
      email TEXT UNIQUE,
      password TEXT,
      access_key TEXT UNIQUE,
      hwid TEXT,
      blacklisted INTEGER DEFAULT 0
    );
  `);

  await db.exec(`
    CREATE TABLE IF NOT EXISTS access_keys (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      key TEXT UNIQUE
    );
  `);

  await db.exec(`
    CREATE TABLE IF NOT EXISTS sessions (
      token TEXT PRIMARY KEY,
      username TEXT,
      expires_at INTEGER
    );
  `);

  console.log("SQLite DB initialized");
}
