use std::env;
use tauri::{AppHandle, Manager, Runtime};
use sha2::{Digest, Sha256};
use winreg::enums::*;
use winreg::RegKey;
use wmi::{COMLibrary, WMIConnection};
use serde::{Serialize, Deserialize};

mod tpm;

const REG_PATH: &str = r"Software\Blackline";

#[derive(Deserialize, Debug)]
#[serde(rename = "Win32_BaseBoard")]
struct BaseBoard {
    #[serde(rename = "SerialNumber")]
    serial_number: Option<String>, // Should be Option to handle missing values
}

#[derive(Deserialize, Debug)]
#[serde(rename = "Win32_Processor")]
struct Processor {
    #[serde(rename = "ProcessorId")]
    processor_id: Option<String>, // WMI field is "ProcessorId", not "processor_id"
}

#[derive(Deserialize, Debug)]
#[serde(rename = "Win32_LogicalDisk")]
struct OSDrive {
    #[serde(rename = "VolumeSerialNumber")]
    volume_serial_number: Option<String>,
}


#[tauri::command]
fn exit_app<R: Runtime>(app: AppHandle<R>) {
    if let Some(window) = app.get_webview_window("main") {
        let _ = window.close();
    }
}
#[tauri::command]
fn minimize_app<R: Runtime>(app: AppHandle<R>) {
    if let Some(window) = app.get_webview_window("main") {
        let _ = window.minimize();
    }
}

#[tauri::command]
async fn get_hwid() -> Result<String, String> {
    // 3. Query system info via WMI
    let com_con = COMLibrary::new().map_err(|e| format!("COM init failed: {}", e))?;
    let wmi_con = WMIConnection::new(com_con.into()).map_err(|e| format!("WMI init failed: {}", e))?;

    let baseboards: Vec<BaseBoard> = wmi_con.query().map_err(|e| format!("Baseboard query failed: {}", e))?;
    let cpus: Vec<Processor> = wmi_con.query().map_err(|e| format!("CPU query failed: {}", e))?;
    let drives: Vec<OSDrive> = wmi_con
        .raw_query("SELECT VolumeSerialNumber FROM Win32_LogicalDisk WHERE DeviceID = 'C:'")
        .map_err(|e| format!("Drive query failed: {}", e))?;

    let mb = baseboards.get(0)
        .and_then(|b| b.serial_number.as_ref())
        .unwrap_or(&String::new())
        .clone();
    
    let cpu = cpus.get(0)
        .and_then(|c| c.processor_id.as_ref())
        .unwrap_or(&String::new())
        .clone();

    let drive_serial = drives
        .get(0)
        .and_then(|d| d.volume_serial_number.as_ref())
        .unwrap_or(&String::new())
        .clone();

    let hwid_hash = match tpm::get_tpm_sha256() {
        Ok(tpm_hash) if !tpm_hash.is_empty() => {
            let combined = format!("{}|{}|{}|{}", tpm_hash, mb, cpu, drive_serial);
            let mut hasher = Sha256::new();
            hasher.update(combined.as_bytes());
            format!("{:x}", hasher.finalize())
        }
        _ => {
            let combined = format!("{}|{}|{}", mb, cpu, drive_serial);
            let mut hasher = Sha256::new();
            hasher.update(combined.as_bytes());
            format!("{:x}", hasher.finalize())
        }
    };

    match tpm::get_tpm_sha256() {
        Ok(hash) => println!("[TPM] {}", hash),
        Err(e) => println!("[TPM] Error: {}", e),
    }
    println!("[HWID] {}", hwid_hash);

    Ok(hwid_hash)
}

#[tauri::command]
fn set_token(token: String) -> Result<(), String> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    let (key, _) = hkcu.create_subkey(REG_PATH).map_err(|e| e.to_string())?;
    key.set_value("Token", &token).map_err(|e| e.to_string())
}
#[tauri::command]
fn get_token() -> Result<String, String> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    let key = hkcu.open_subkey(REG_PATH).map_err(|e| e.to_string())?;
    key.get_value("Token").map_err(|e| e.to_string())
}

#[tauri::command]
fn delete_token() -> Result<(), String> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    let key = hkcu.open_subkey(REG_PATH).map_err(|e| e.to_string())?;
    key.delete_subkey("Token").map_err(|e| e.to_string())
}

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

const DEFAULT_ACCENT_COLOR: &str = "#37C7EE";

#[tauri::command]
fn set_accent_color(hex_color: String) -> Result<(), String> {
    // Validate the color format
    let accent_color = AccentColor::new(hex_color)?;

    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    let key = hkcu.create_subkey(REG_PATH)
        .map_err(|e| format!("Failed to create registry key: {}", e))?
        .0;

    // Store the hex color as a string
    key.set_value("AccentColor", &accent_color.hex)
        .map_err(|e| format!("Failed to set accent color: {}", e))?;

    Ok(())
}

#[tauri::command]
fn get_accent_color() -> Result<AccentColor, String> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);

    // First try to get custom accent color
    if let Ok(key) = hkcu.open_subkey(REG_PATH) {
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

#[tauri::command]
fn remove_accent_color() -> Result<(), String> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    let key = hkcu.open_subkey(REG_PATH)
        .map_err(|_| "Registry key not found")?;

    key.delete_value("AccentColor")
        .map_err(|e| format!("Failed to remove accent color: {}", e))?;

    Ok(())
}

#[tauri::command]
fn get_system_accent_color() -> Result<AccentColor, String> {
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
#[tauri::command]
fn get_all_accent_colors() -> Result<serde_json::Value, String> {
    let mut result = serde_json::json!({});

    // Try to get custom accent color
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    if let Ok(key) = hkcu.open_subkey(REG_PATH) {
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

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_store::Builder::default().build())
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![
            exit_app,
            minimize_app,
            get_hwid,
            set_token,
            get_token,
            delete_token,
            set_accent_color,
            get_accent_color,
            remove_accent_color,
            get_system_accent_color,
            get_all_accent_colors
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
