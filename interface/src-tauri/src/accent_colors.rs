use tauri::command;
use winreg::enums::*;
use winreg::RegKey;
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
pub struct AccentColor {
    pub hex: String,
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

impl AccentColor {
    pub fn new(hex: String) -> Result<Self, String> {
        // Remove # if present
        let hex_clean = hex.trim_start_matches('#');

        if hex_clean.len() != 6 {
            return Err("Invalid hex color format".to_string());
        }

        let r = u8::from_str_radix(&hex_clean[0..2], 16)
            .map_err(|_| "Invalid hex color format")?;
        let g = u8::from_str_radix(&hex_clean[2..4], 16)
            .map_err(|_| "Invalid hex color format")?;
        let b = u8::from_str_radix(&hex_clean[4..6], 16)
            .map_err(|_| "Invalid hex color format")?;

        Ok(AccentColor {
            hex: format!("#{}", hex_clean),
            r,
            g,
            b,
        })
    }
}

const REGISTRY_KEY_PATH: &str = "SOFTWARE\\Blackline"; // Replace with your app name
const DEFAULT_ACCENT_COLOR: &str = "#37C7EE";

#[command]
pub fn set_accent_color(hex_color: String) -> Result<(), String> {
    // Validate the color format
    let accent_color = AccentColor::new(hex_color)?;

    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    let key = hkcu.create_subkey(REGISTRY_KEY_PATH)
        .map_err(|e| format!("Failed to create registry key: {}", e))?
        .0;

    // Store the hex color as a string
    key.set_value("AccentColor", &accent_color.hex)
        .map_err(|e| format!("Failed to set accent color: {}", e))?;

    Ok(())
}

#[command]
pub fn get_accent_color() -> Result<AccentColor, String> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);

    // First try to get custom accent color
    if let Ok(key) = hkcu.open_subkey(REGISTRY_KEY_PATH) {
        if let Ok(hex_color) = key.get_value::<String, _>("AccentColor") {
            if let Ok(color) = AccentColor::new(hex_color) {
                return Ok(color);
            }
        }
    }

    // Fall back to system accent color
    if let Ok(system_color) = get_system_accent_color() {
        return Ok(system_color);
    }

    // Finally fall back to default color
    AccentColor::new(DEFAULT_ACCENT_COLOR.to_string())
}

#[command]
pub fn remove_accent_color() -> Result<(), String> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    let key = hkcu.open_subkey(REGISTRY_KEY_PATH)
        .map_err(|_| "Registry key not found")?;

    key.delete_value("AccentColor")
        .map_err(|e| format!("Failed to remove accent color: {}", e))?;

    Ok(())
}

#[command]
pub fn get_system_accent_color() -> Result<AccentColor, String> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    let key = hkcu.open_subkey("SOFTWARE\\Microsoft\\Windows\\DWM")
        .map_err(|_| "Cannot access Windows DWM settings")?;

    // Windows stores accent color as DWORD (ABGR format)
    let accent_color_dword: u32 = key.get_value("AccentColor")
        .map_err(|_| "System accent color not found")?;

    // Extract RGB from ABGR format
    let r = (accent_color_dword & 0xFF) as u8;
    let g = ((accent_color_dword >> 8) & 0xFF) as u8;
    let b = ((accent_color_dword >> 16) & 0xFF) as u8;

    let hex = format!("#{:02X}{:02X}{:02X}", r, g, b);

    Ok(AccentColor { hex, r, g, b })
}

// Optional: Get both custom and system accent colors
#[command]
pub fn get_all_accent_colors() -> Result<serde_json::Value, String> {
    let mut result = serde_json::json!({});

    // Try to get custom accent color
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    if let Ok(key) = hkcu.open_subkey(REGISTRY_KEY_PATH) {
        if let Ok(hex_color) = key.get_value::<String, _>("AccentColor") {
            if let Ok(custom) = AccentColor::new(hex_color) {
                result["custom"] = serde_json::to_value(custom)
                    .map_err(|e| format!("Failed to serialize custom color: {}", e))?;
            }
        }
    }

    if result["custom"].is_null() {
        result["custom"] = serde_json::Value::Null;
    }

    // Try to get system accent color
    match get_system_accent_color() {
        Ok(system) => {
            result["system"] = serde_json::to_value(system)
                .map_err(|e| format!("Failed to serialize system color: {}", e))?;
        }
        Err(_) => {
            result["system"] = serde_json::Value::Null;
        }
    }

    // Add default color
    let default_color = AccentColor::new(DEFAULT_ACCENT_COLOR.to_string())
        .map_err(|e| format!("Failed to create default color: {}", e))?;
    result["default"] = serde_json::to_value(default_color)
        .map_err(|e| format!("Failed to serialize default color: {}", e))?;

    Ok(result)
}