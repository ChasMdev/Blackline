use std::{env, path::{Path, PathBuf}, sync::mpsc, thread, time::Duration};
use notify::{Config, Event, RecommendedWatcher, RecursiveMode, Watcher};
use rfd::FileDialog;
use tauri::{AppHandle, Emitter, Runtime};
use lz4::EncoderBuilder;
use std::io::{Write, Read};
use bincode::{serialize, deserialize};
use std::fs;
use dirs;
use crate::alerts;
use sha2::{Sha256, Digest};
use reqwest::Client;

const FILES: &[&str] = &["Module.dll", "Injector.exe"];
const BUCKET_URL: &str = "https://pub-193dd1b0882e448aa2ca0b49c988dcf8.r2.dev";

const DEBUG: bool = true;

#[derive(serde::Serialize, serde::Deserialize, Clone)]
pub struct TabData {
    pub tabs: Vec<Tab>,
    pub active_tab_id: String,
}

#[derive(serde::Deserialize, serde::Serialize, Clone)]
pub struct Tab {
    pub id: String,
    pub name: String,
    pub content: String,
}

#[tauri::command]
async fn ensure_data_dir<R: Runtime>(_app: AppHandle<R>) -> Result<PathBuf, String> {
    #[cfg(target_os = "windows")]
    let path = dirs::data_dir()
        .expect("Failed to get app data dir")
        .join("Blackline")
        .join("bin");
        
    #[cfg(not(target_os = "windows"))]
    let path = dirs::config_dir()
        .expect("Failed to get config dir")
        .join("Blackline")
        .join("bin");
    
    if !path.exists() {
        std::fs::create_dir_all(&path).map_err(|e| e.to_string())?;
    }
    
    Ok(path)
}
#[tauri::command]
pub async fn save_tabs<R: Runtime>(
    app: AppHandle<R>,
    tabs: Vec<Tab>,
    active_tab_id: String,
) -> Result<(), String> {
    let data_dir = ensure_data_dir(app.clone()).await?;
    let file_path = data_dir.join("editor_tabs.bin");
    
    let data = TabData { tabs, active_tab_id };
    
    let serialized = serialize(&data)
        .map_err(|e| e.to_string())?;
    
    let mut encoder = EncoderBuilder::new()
        .level(12)
        .build(Vec::new())
        .map_err(|e| e.to_string())?;
    
    encoder.write_all(&serialized)
        .map_err(|e| e.to_string())?;
    
    let (compressed, result) = encoder.finish();
    result.map_err(|e| e.to_string())?;
    
    std::fs::write(&file_path, compressed)
        .map_err(|e| e.to_string())?;
    
    Ok(())
}
#[tauri::command]
pub async fn load_tabs<R: Runtime>(app: AppHandle<R>) -> Result<Option<TabData>, String> {
    let data_dir = ensure_data_dir(app.clone()).await?;
    let file_path = data_dir.join("editor_tabs.bin");
    
    if !file_path.exists() {
        return Ok(None);
    }
    
    let compressed = std::fs::read(&file_path)
        .map_err(|e| e.to_string())?;
    
    let mut decoder = lz4::Decoder::new(&compressed[..])
        .map_err(|e| e.to_string())?;
    let mut buffer = Vec::new();
    decoder.read_to_end(&mut buffer)
        .map_err(|e| e.to_string())?;
    
    let data: TabData = deserialize(&buffer)
        .map_err(|e| e.to_string())?;
    
    Ok(Some(data))
}
async fn get_scripts_directory(app: &AppHandle) -> Option<PathBuf> {
    let roaming_dir = match dirs::data_dir() {
        Some(dir) => dir,
        None => {
            alerts::show_error_alert(
                app.clone(),
                "AppData Not Found".into(),
                "Could not find the AppData directory.".into(),
            ).await;
            return None;
        }
    };

    let scripts_path = roaming_dir.join("blackline").join("scripts");

    if !scripts_path.exists() {
        if let Err(e) = fs::create_dir_all(&scripts_path) {
            alerts::show_error_alert(
                app.clone(),
                "Directory Creation Failed".into(),
                format!("Failed to create scripts directory:\n{}", simplify_error(&e.to_string())).into(),
            ).await;
            return None;
        }
    }

    Some(scripts_path)
}
#[tauri::command]
pub async fn open_file(app: AppHandle) -> Result<Option<(String, String)>, String> {
    let default_dir = match env::var("APPDATA") {
        Ok(appdata) => {
            let path = PathBuf::from(appdata).join("Blackline").join("Scripts");
            if let Err(e) = fs::create_dir_all(&path) {
                eprintln!("Warning: Failed to create Scripts directory: {}", e);
            }
            Some(path)
        }
        Err(_) => None,
    };

    let mut dialog = FileDialog::new()
        .add_filter("Lua Files", &["lua"])
        .add_filter("Luau Files", &["luau"])
        .add_filter("Text Files", &["txt"])
        .add_filter("All Files", &["*"]);

    if let Some(path) = default_dir {
        dialog = dialog.set_directory(path);
    }

    let file_path: Option<PathBuf> = dialog.pick_file();

    if let Some(path) = file_path {
        match fs::read_to_string(&path) {
            Ok(content) => {
                let file_name = path
                    .file_name()
                    .and_then(|name| name.to_str())
                    .unwrap_or("Unknown File")
                    .to_string();

                Ok(Some((file_name, content)))
            }
            Err(e) => {
                alerts::show_error_alert(
                    app,
                    "Failed to Open File".into(),
                    format!("Could not read the selected file:\n{}", simplify_error(&e.to_string())).into(),
                ).await;
                Ok(None)
            }
        }
    } else {
        Ok(None)
    }
}
#[tauri::command]
pub async fn save_file(app: AppHandle, file_name: String, content: String) -> Result<Option<String>, ()> {
    let scripts_dir = match get_scripts_directory(&app).await {
        Some(path) => path,
        None => return Err(()),
    };

    let final_file_name = if file_name.ends_with(".lua") || file_name.ends_with(".luau") {
        file_name
    } else {
        format!("{}.lua", file_name)
    };

    let file_path = scripts_dir.join(&final_file_name);

    if let Err(e) = fs::write(&file_path, content) {
        alerts::show_error_alert(
            app,
            "Failed to Save File".into(),
            format!("Could not write to file:\n{}", simplify_error(&e.to_string())).into(),
        ).await;
        return Err(());
    }

    Ok(Some(final_file_name))
}

pub fn simplify_error(error: &str) -> String {
    if error.contains("Permission denied") {
        "Permission denied. Try running as administrator.".into()
    } else if error.contains("No such file or directory") {
        "The file was not found.".into()
    } else {
        error.lines().next().unwrap_or("Unknown error").into()
    }
}

fn get_bin_directory() -> Result<PathBuf, String> {
    dirs::data_dir()
        .ok_or("AppData not found".into())
        .map(|path| path.join("Blackline").join("bin"))
}

fn calculate_file_hash(path: &PathBuf) -> Result<String, String> {
    let data = fs::read(path).map_err(|e| format!("Failed to read file: {}", e))?;
    let mut hasher = Sha256::new();
    hasher.update(data);
    Ok(format!("{:x}", hasher.finalize()))
}

async fn download_file(url: &str) -> Result<Vec<u8>, String> {
    let client = Client::new();
    let response = client.get(url)
        .send().await
        .map_err(|e| format!("Download error: {}", e))?
        .error_for_status()
        .map_err(|e| format!("HTTP error: {}", e))?;

    response.bytes()
        .await
        .map(|b| b.to_vec())
        .map_err(|e| format!("Read error: {}", e))
}

#[tauri::command]
pub async fn verify_and_download_files(app: AppHandle) -> Result<(), ()> {
    if !DEBUG {
        let bin_dir = match get_bin_directory() {
            Ok(p) => p,
            Err(e) => {
                alerts::show_error_alert(app.clone(), "AppData Not Found".into(), e.into()).await;
                return Err(());
            }
        };

        if !bin_dir.exists() {
            if let Err(e) = fs::create_dir_all(&bin_dir) {
                alerts::show_error_alert(app.clone(), "Failed to Create Bin".into(), e.to_string().into()).await;
                return Err(());
            }
        }

        for file_name in FILES {
            let path = bin_dir.join(file_name);
            let url = format!("{}/{}", BUCKET_URL, file_name);

            let remote_bytes = match download_file(&url).await {
                Ok(bytes) => bytes,
                Err(e) => {
                    alerts::show_error_alert(
                        app.clone(),
                        format!("Failed to Download {}", file_name),
                        format!("Could not fetch from:\n{}\n\nReason:\n{}", url, e).into()
                    ).await;
                    return Err(());
                }
            };

            let new_hash = format!("{:x}", Sha256::digest(&remote_bytes));

            let current_hash = if path.exists() {
                match calculate_file_hash(&path) {
                    Ok(hash) => hash,
                    Err(e) => {
                        alerts::show_error_alert(
                            app.clone(),
                            format!("Hash Error: {}", file_name),
                            format!("Failed to hash file:\n{}", simplify_error(&e)).into()
                        ).await;
                        return Err(());
                    }
                }
            } else {
                String::new()
            };

            if current_hash != new_hash {
                if let Err(e) = fs::write(&path, &remote_bytes) {
                    alerts::show_error_alert(
                        app.clone(),
                        format!("Failed to Write {}", file_name),
                        e.to_string().into()
                    ).await;
                    return Err(());
                }

                alerts::show_success_alert(
                    app.clone(),
                    format!("Successfully updated {}", file_name)
                ).await;
            }
        }

        Ok(())
    } else {
        Ok(())
    }
}

#[derive(serde::Serialize, serde::Deserialize, Clone)]
pub struct FileNode {
    name: String,
    path: String,
    #[serde(rename = "type")]
    file_type: String,
    children: Option<Vec<FileNode>>,
}

#[tauri::command]
pub fn list_file_tree() -> Result<Vec<FileNode>, String> {
    fn walk_dir(path: &Path) -> Vec<FileNode> {
        let mut entries = vec![];
        if let Ok(read_dir) = fs::read_dir(path) {
            for entry in read_dir.flatten() {
                let path = entry.path();
                let name = entry.file_name().to_string_lossy().into_owned();
                if path.is_dir() {
                    entries.push(FileNode {
                        name,
                        path: path.to_string_lossy().into_owned(),
                        file_type: "folder".into(),
                        children: Some(walk_dir(&path)),
                    });
                } else {
                    entries.push(FileNode {
                        name,
                        path: path.to_string_lossy().into_owned(),
                        file_type: "file".into(),
                        children: None,
                    });
                }
            }
        }
        entries
    }

    let base_path = dirs::config_dir()
        .or_else(dirs::data_dir)
        .unwrap()
        .join("Blackline");

    if !base_path.exists() {
        return Ok(vec![]);
    }

    let folders = vec![
        ("Scripts", "Scripts"),
        ("Workspace", "Workspace"), 
        ("AutoExec", "Auto Execute"),
    ];
    
    let mut result = vec![];

    for (fs_name, display_name) in folders {
        let path = base_path.join(fs_name);
        if path.exists() {
            result.push(FileNode {
                name: display_name.to_string(),
                path: path.to_string_lossy().into_owned(),
                file_type: "folder".into(),
                children: Some(walk_dir(&path)),
            });
        }
    }

    result.push(FileNode {
        name: "Github Gists".to_string(),
        path: format!("{}/github-gists", base_path.to_string_lossy()),
        file_type: "folder".into(),
        children: Some(vec![]),
    });

    Ok(result)
}

#[tauri::command]
pub fn start_file_watcher(app_handle: AppHandle) -> Result<(), String> {
    let base_path = dirs::config_dir()
        .or_else(dirs::data_dir)
        .unwrap()
        .join("Blackline");

    if !base_path.exists() {
        return Err("Base path does not exist".to_string());
    }

    let (tx, rx) = mpsc::channel();

    let mut watcher = RecommendedWatcher::new(
        move |res: Result<Event, notify::Error>| {
            if let Ok(event) = res {
                let _ = tx.send(event);
            }
        },
        Config::default(),
    ).map_err(|e| e.to_string())?;

    let folders = vec!["Scripts", "Workspace", "AutoExec"];
    for folder in folders {
        let path = base_path.join(folder);
        if path.exists() {
            watcher.watch(&path, RecursiveMode::Recursive)
                .map_err(|e| e.to_string())?;
        }
    }

    thread::spawn(move || {
        let mut last_event_time = std::time::Instant::now();
        
        let _watcher = watcher;
        
        for event in rx {
            let now = std::time::Instant::now();
            if now.duration_since(last_event_time) > Duration::from_millis(500) {
                last_event_time = now;

                let _ = app_handle.emit("file-tree-changed", ());
                
                println!("File system event: {:?}", event);
            }
        }
    });

    Ok(())
}

#[tauri::command]
pub fn rename_file(old_path: String, new_name: String) {
    let path = std::path::Path::new(&old_path);
    if let Some(parent) = path.parent() {
        let new_path = parent.join(new_name);
        std::fs::rename(path, new_path).unwrap();
    }
}

#[tauri::command]
pub fn delete_file(path: String) {
    let path = std::path::Path::new(&path);
    if path.is_file() {
        std::fs::remove_file(path).unwrap();
    } else if path.is_dir() {
        std::fs::remove_dir_all(path).unwrap();
    }
}

#[tauri::command]
pub fn clone_file(path: String) {
    use std::fs;
    use std::path::Path;

    let original = Path::new(&path);
    if let Some(parent) = original.parent() {
        let new_path = parent.join(format!(
            "{}_copy{}",
            original.file_stem().unwrap().to_string_lossy(),
            original.extension()
                .map(|e| format!(".{}", e.to_string_lossy()))
                .unwrap_or_default()
        ));
        fs::copy(&original, new_path).unwrap();
    }
}

fn truncate_path<P: AsRef<Path>>(full_path: P) -> String {
    let roots = ["Workspace", "Scripts", "Auto Execute"];

    let components: Vec<String> = full_path
        .as_ref()
        .components()
        .map(|c| c.as_os_str().to_string_lossy().to_string())
        .collect();

    for (i, part) in components.iter().enumerate() {
        if roots.iter().any(|root| part.eq_ignore_ascii_case(root)) {
            return components[i..].join("/");
        }
    }

    // fallback to filename
    PathBuf::from(full_path.as_ref())
        .file_name()
        .map(|name| name.to_string_lossy().to_string())
        .unwrap_or_else(|| full_path.as_ref().to_string_lossy().to_string())
}

#[tauri::command]
pub fn read_file(path: String) -> Result<String, String> {
    std::fs::read_to_string(&path)
        .map_err(|e| format!("Failed to read file {}: {}", truncate_path(path), e))
}