use sha2::{Digest, Sha256};
use winreg::enums::*;
use winreg::RegKey;
use wmi::{COMLibrary, WMIConnection};
use serde::Deserialize;
use crate::tpm;

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
pub async fn get_hwid() -> Result<String, String> {
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

    // 4. Combine and hash
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

    /*match tpm::get_tpm_sha256() {
        Ok(hash) => println!("[TPM] {}", hash),
        Err(e) => println!("[TPM] Error: {}", e),
    }
    println!("[HWID] {}", hwid_hash);*/
    Ok(hwid_hash)
}

#[tauri::command]
pub fn set_token(token: String) -> Result<(), String> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    let (key, _) = hkcu.create_subkey(REG_PATH).map_err(|e| e.to_string())?;
    key.set_value("Token", &token).map_err(|e| e.to_string())
}
#[tauri::command]
pub fn get_token() -> Result<String, String> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    let key = hkcu.open_subkey(REG_PATH).map_err(|e| e.to_string())?;
    key.get_value("Token").map_err(|e| e.to_string())
}

#[tauri::command]
pub fn delete_token() -> Result<(), String> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    hkcu.delete_subkey_all(REG_PATH).map_err(|e| e.to_string())
}