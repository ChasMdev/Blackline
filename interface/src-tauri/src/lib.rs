use std::{env, fs::{self, OpenOptions}, io::{Read, Write}, path::{Path, PathBuf}, process::Command, sync::{Arc, Mutex}, thread, time::Duration};
use sysinfo::{ProcessesToUpdate, Signal, System};
use tauri::{AppHandle, Emitter, Manager, Runtime};
use tokio::time::sleep;

use crate::alerts::show_success_alert;

mod fs_commands;
mod alerts;
mod hwid;
mod accent_colors;
mod tpm;

#[derive(serde::Serialize, serde::Deserialize, Clone)]
#[serde(rename_all = "lowercase")]
enum LogEffect {
    Normal,
    Success,
    Warn,
    Error,
}

#[derive(serde::Serialize, Clone)]
pub struct ConsoleLog {
    message: String,
    effect: LogEffect,
}

static INJECTION_STATE: std::sync::OnceLock<Arc<Mutex<bool>>> = std::sync::OnceLock::new();

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
async fn console_startup(app: AppHandle, username: String) {
    sleep(Duration::from_millis(100)).await;
    let effect = LogEffect::Normal;
    let log = ConsoleLog {
        message: format!("Welcome to Blackline, {}", username),
        effect,
    };
    app.emit("console-log", log).unwrap();
}

pub fn setup_blackline_workspace() {
    let app_data_roaming = env::var("APPDATA").expect("APPDATA not found");

    let mut base_dir = PathBuf::from(app_data_roaming);
    base_dir.push("Blackline");

    if let Err(e) = fs::create_dir_all(&base_dir) {
        println!("Failed to create Blackline base folder: {:?}", e);
    }
    let subfolders = ["Scripts", "Workspace", "AutoExec", "bin"];

    for folder in subfolders.iter() {
        let mut sub_dir = base_dir.clone();
        sub_dir.push(folder);

        if let Err(e) = fs::create_dir_all(&sub_dir) {
            println!("Failed to create folder {:?}: {:?}", folder, e);
        }
    }
}

fn get_injection_state() -> Arc<Mutex<bool>> {
    INJECTION_STATE.get_or_init(|| Arc::new(Mutex::new(false))).clone()
}
#[tauri::command]
fn is_pipe_connected() -> bool {
    Path::new(r"\\.\pipe\BlacklineExecution").exists()
}
#[tauri::command]
fn is_roblox_running() -> bool {
    let mut system = System::new_all();
    system.refresh_processes(ProcessesToUpdate::All, true);
    let running = system
        .processes()
        .iter()
        .any(|(_, proc)| proc.name() == "RobloxPlayerBeta.exe");
    running
}
async fn monitor_roblox_process(app: AppHandle) {
    let mut system = System::new_all();
    
    loop {
        sleep(Duration::from_millis(1000)).await;
        
        let should_monitor = {
            let state = get_injection_state();
            let guard = state.lock().unwrap();
            let result = *guard;
            drop(guard);
            result
        };
        
        if !should_monitor {
            continue;
        }

        system.refresh_processes(ProcessesToUpdate::All, true);
        let roblox_running = system
            .processes()
            .iter()
            .any(|(_, proc)| proc.name() == "RobloxPlayerBeta.exe");
        
        if !roblox_running {
            {
                let state = get_injection_state();
                let mut guard = state.lock().unwrap();
                *guard = false;
                drop(guard);
            }
            
            alerts::show_warn_alert(
                app.clone(),
                "Roblox Closed".into(),
            ).await;
            
            break;
        }
    }
}
#[tauri::command]
async fn run_injector(app: AppHandle) -> Result<(), ()> {
    let data_dir = match dirs::data_dir() {
        Some(p) => p.join("blackline").join("bin"),
        None => {
            alerts::show_error_alert(
                app.clone(),
                "AppData Error".into(),
                "Could not resolve %AppData% directory.".into(),
            ).await;
            return Err(());
        }
    };

    let injector_path = data_dir.join("Injector.exe");
    let module_path = data_dir.join("Module.dll");
    let pipe_path = Path::new(r"\\.\pipe\BlacklineExecution");

    if pipe_path.exists() {
        alerts::show_warn_alert(
            app.clone(),
            "Already Injected!".into(),
        ).await;
        return Err(());
    }

    if !injector_path.exists() {
        alerts::show_error_alert(
            app.clone(),
            "Injector Missing".into(),
            "Injector.exe was not found in /bin.".into(),
        ).await;
        return Err(());
    }

    if !module_path.exists() {
        alerts::show_error_alert(
            app.clone(),
            "Module Missing".into(),
            "Module.dll was not found in /bin.".into(),
        ).await;
        return Err(());
    }

    let mut sys = sysinfo::System::new_all();
    sys.refresh_processes(ProcessesToUpdate::All, true);
    let roblox_pid = sys
        .processes()
        .iter()
        .find(|(_, p)| p.name() == "RobloxPlayerBeta.exe")
        .map(|(pid, _)| *pid);

    if roblox_pid.is_none() {
        alerts::show_warn_alert(
            app.clone(),
            "Roblox Not Running".into(),
        ).await;
        return Err(());
    }

    let status = Command::new(&injector_path)
        .arg("https://127.0.0.1:8000/token-login")
        .arg("Module.dll")
        .current_dir(&data_dir)
        .status();

    match status {
        Ok(code) if !code.success() => {
            alerts::show_error_alert(
                app.clone(),
                "Injector Failed".into(),
                format!("Injector exited with code: {}", code).into(),
            ).await;
            return Err(());
        }
        Err(e) => {
            alerts::show_error_alert(
                app.clone(),
                "Injector Error".into(),
                format!("Failed to start Injector.exe:\n{}", fs_commands::simplify_error(&e.to_string())).into(),
            ).await;
            return Err(());
        }
        _ => {}
    }

    for _ in 0..50 {
        if pipe_path.exists() {
            {
                let state = get_injection_state();
                let mut guard = state.lock().unwrap();
                *guard = true;
                drop(guard);
            }
            
            let app_clone = app.clone();
            tokio::spawn(async move {
                monitor_roblox_process(app_clone).await;
            });
            
            alerts::show_success_alert(app.clone(), "Successfully injected!".into()).await;
            return Ok(());
        }

        let mut sys = sysinfo::System::new();
        sys.refresh_processes(ProcessesToUpdate::All, true);

        let roblox_alive = sys.processes()
            .iter()
            .any(|(_, p)| p.name() == "RobloxPlayerBeta.exe");

        if !roblox_alive {
            alerts::show_error_alert(
                app.clone(),
                "Roblox Crashed!".into(),
                "Roblox closed before Blackline could initialize.".into(),
            ).await;
            return Err(());
        }

        sleep(Duration::from_millis(100)).await;
    }

    alerts::show_error_alert(
        app.clone(),
        "Injection Timeout".into(),
        "Module.dll did not signal readiness in time.".into(),
    ).await;
    Err(())
}
#[tauri::command]
async fn send_to_pipe(content: String, app: AppHandle) -> Result<(), ()> {
    let pipe_path = r"\\.\pipe\BlacklineExecution";
    let buffer = content.as_bytes();
    let len = (buffer.len() as u32).to_le_bytes();

    let mut retries = 5;
    let pipe = loop {
        match OpenOptions::new().read(true).write(true).open(pipe_path) {
            Ok(pipe) => break pipe,
            Err(_e) if retries > 0 => {
                retries -= 1;
                thread::sleep(Duration::from_millis(50));
            }
            Err(e) => {
                alerts::show_error_alert(
                    app.clone(),
                    "Execution Error".into(),
                    format!("BlacklineExecution pipe not found:\n{}", fs_commands::simplify_error(&e.to_string())).into(),
                ).await;
                return Err(());
            }
        }
    };

    let mut pipe = pipe;

    if let Err(e) = pipe.write_all(&len) {
        alerts::show_error_alert(
            app.clone(),
            "Pipe Write Failed".into(),
            format!("Failed to write header to pipe:\n{}", fs_commands::simplify_error(&e.to_string())).into(),
        ).await;
        return Err(());
    }

    if let Err(e) = pipe.write_all(buffer) {
        alerts::show_error_alert(
            app.clone(),
            "Pipe Write Failed".into(),
            format!("Could not send to pipe:\n{}", fs_commands::simplify_error(&e.to_string())).into(),
        ).await;
        return Err(());
    }

    let mut response = vec![0u8; 4096];
    match pipe.read(&mut response) {
        Ok(n) if n > 0 => {
            let msg = String::from_utf8_lossy(&response[..n]).to_string();
            println!("[Pipe Response] {}", msg);
            //log(&app, format!("[Pipe Response] {}", msg));
        }
        Ok(_) => {
            println!("[Pipe Response] Empty response from pipe");
            //log(&app, "[Pipe Response] Empty response from pipe".to_string());
        }
        Err(e) => {
            println!("[Pipe Response Error] {}", e);
            //log(&app, format!("[Pipe Response Error] {}", e));
        }
    }

    Ok(())
}

#[tauri::command]
async fn close_roblox(app: AppHandle) {
    let program_name = "RobloxPlayerBeta.exe";
    let mut system = System::new_all();
    system.refresh_all();

    let found_process = false;
    
    for process in system.processes().values() {
        if process.name().eq_ignore_ascii_case(program_name) {
            println!(
                "Attempting to kill process '{}' (PID: {})",
                process.name().to_string_lossy(),
                process.pid()
            );

            if process.kill_with(Signal::Kill).is_some() {
                show_success_alert(
                    app,
                    format!("Killed Roblox (PID: {})", process.pid()),
                ).await;
                return; // Exit function after successful kill
            } else {
                let err_msg = format!(
                    "Failed to kill Roblox (PID: {}). Possible reasons:\n\
                     - Insufficient permissions\n\
                     - Process already exited\n\
                     - Signal not supported on this platform",
                    process.pid()
                );
                alerts::show_error_alert(
                    app,
                    format!("Failed to kill roblox!"),
                    err_msg,
                ).await;
                return; // Exit function after error
            }
        }
    }
    
    if !found_process {
        alerts::show_error_alert(
            app,
            format!("Process not found!"),
            format!("Could not find {} running", program_name),
        ).await;
    }
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    setup_blackline_workspace();
    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![
            exit_app,
            minimize_app,
            console_startup,
            is_pipe_connected,
            is_roblox_running,
            run_injector,
            send_to_pipe,
            close_roblox,

            fs_commands::verify_and_download_files,
            fs_commands::save_tabs,
            fs_commands::load_tabs,
            fs_commands::open_file,
            fs_commands::save_file,
            fs_commands::list_file_tree,
            fs_commands::start_file_watcher,
            fs_commands::rename_file,
            fs_commands::delete_file,
            fs_commands::clone_file,
            fs_commands::read_file,
            alerts::show_error_alert,
            alerts::show_warn_alert,
            alerts::show_success_alert,
            hwid::get_hwid,
            hwid::set_token,
            hwid::get_token,
            hwid::delete_token,
            accent_colors::set_accent_color,
            accent_colors::get_accent_color,
            accent_colors::remove_accent_color,
            accent_colors::get_system_accent_color,
            accent_colors::get_all_accent_colors,

            tpm::get_tpm_hashes
        ])
        .setup(|app| {
            // Start file watcher when app starts
            let app_handle = app.handle().clone();
            if let Err(e) = fs_commands::start_file_watcher(app_handle) {
                eprintln!("Failed to start file watcher: {}", e);
            }
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
