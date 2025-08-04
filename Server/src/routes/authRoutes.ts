import express, { Request, Response } from "express";
import { generateAccessKey, hashPassword, verifyPassword } from "../utils";
import { getDb } from "../db";
import crypto from "crypto";

const router = express.Router();
(async () => {
    const db = await getDb();

    function createToken(username: string, duration = 3600): string {
        const token = crypto.randomBytes(32).toString("hex");
        const expires = Math.floor(Date.now() / 1000) + duration;
        db.run("INSERT INTO sessions (token, username, expires_at) VALUES (?, ?, ?)", [token, username, expires]);
        return token;
    }

    // POST /signup
    router.post("/signup", async (req: Request, res: Response) => {
        const { username, email, password, access_key } = req.body;

        const exists = await db.get("SELECT 1 FROM users WHERE username = ? OR email = ?", [username, email]);
        if (exists) return res.status(400).json({ detail: "User already exists" });

        const validKey = await db.get("SELECT 1 FROM access_keys WHERE key = ?", [access_key]);
        if (!validKey) return res.status(403).json({ detail: "Invalid access key" });

        const keyUsed = await db.get("SELECT 1 FROM users WHERE access_key = ?", [access_key]);
        if (keyUsed) return res.status(403).json({ detail: "Access key already used" });

        const hashed = await hashPassword(password);
        await db.run("INSERT INTO users (username, email, password, access_key) VALUES (?, ?, ?, ?)", [
            username,
            email,
            hashed,
            access_key,
        ]);

        return res.json({ message: "Signup successful" });
    });

    // POST /login
    router.post("/login", async (req: Request, res: Response) => {
        const { username, password } = req.body;
        const hwid = req.header("X-HWID");
        if (!hwid) return res.status(400).json({ detail: "HWID missing" });

        const row = await db.get("SELECT password, access_key, hwid, blacklisted FROM users WHERE username = ?", [
            username,
        ]);

        if (!row) return res.status(404).json({ detail: "User not found" });
        const match = await verifyPassword(password, row.password);
        if (!match) return res.status(403).json({ detail: "Incorrect password" });

        if (!row.hwid) {
            await db.run("UPDATE users SET hwid = ? WHERE username = ?", [hwid, username]);
        } else if (row.hwid !== hwid) {
            return res.status(403).json({ detail: "Incorrect HWID" });
        }

        const token = createToken(username);

        return res.json({
            message: "Login successful",
            token,
            blacklisted: Boolean(row.blacklisted),
            hasKey: Boolean(row.access_key),
        });
    });

    // POST /token-login
    router.post("/token-login", async (req: Request, res: Response) => {
        const token = req.header("Authorization");
        const hwid = req.header("X-HWID");
        if (!token || !hwid) return res.status(400).json({ detail: "Missing token or HWID" });

        const session = await db.get("SELECT username, expires_at FROM sessions WHERE token = ?", [token]);
        if (!session) return res.status(401).json({ detail: "Invalid or expired token" });

        const now = Math.floor(Date.now() / 1000);
        if (now > session.expires_at) return res.status(401).json({ detail: "Token expired" });

        const user = await db.get("SELECT hwid, blacklisted, access_key FROM users WHERE username = ?", [
            session.username,
        ]);
        if (!user || (user.hwid && user.hwid !== hwid))
            return res.status(403).json({ detail: "Incorrect HWID" });

        if (!user.hwid) {
            await db.run("UPDATE users SET hwid = ? WHERE username = ?", [hwid, session.username]);
        }

        // Extend session if expiring in 3 days
        if (session.expires_at - now < 3 * 86400) {
            const newExpiry = now + 7 * 86400;
            await db.run("UPDATE sessions SET expires_at = ? WHERE token = ?", [newExpiry, token]);
        }

        console.log(`Token login successful for user: ${session.username}`);

        return res.json({
            message: "Token login successful",
            username: session.username,
            blacklisted: Boolean(user.blacklisted),
            hasKey: Boolean(user.access_key),
        });
    });

    // POST /generate-key
    router.post("/generate-key", async (req: Request, res: Response) => {
        const key = generateAccessKey();
        await db.run("INSERT INTO access_keys (key) VALUES (?)", [key]);
        return res.json({ access_key: key });
    });
})();

// Utility for creating a token and inserting it into sessions


export default router;
