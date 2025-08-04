import crypto from "crypto";
import bcrypt from "bcrypt";

export function generateAccessKey(length = 32): string {
    const chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    let key = "";
    for (let i = 0; i < length; i++) {
        const randomIndex = crypto.randomInt(chars.length);
        key += chars[randomIndex];
    }
    return key;
}

export async function hashPassword(password: string): Promise<string> {
    const saltRounds = 10;
    return await bcrypt.hash(password, saltRounds);
}

export async function verifyPassword(password: string, hashed: string): Promise<boolean> {
    return await bcrypt.compare(password, hashed);
}