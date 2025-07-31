use tauri::Emitter;

#[tauri::command]
pub async fn show_success_alert(app: tauri::AppHandle, title: String) {
    app.emit("show-alert", 
        serde_json::json!({
            "type": "success",
            "title": title
        })
    ).unwrap();
}

#[tauri::command]
pub async fn show_warn_alert(app: tauri::AppHandle, title: String) {
    app.emit("show-alert", 
        serde_json::json!({
            "type": "warn",
            "title": title
        })
    ).unwrap();
}

#[tauri::command]
pub async fn show_error_alert(app: tauri::AppHandle, title: String, message: String) {
    println!("{}", message);
    app.emit("show-alert", 
        serde_json::json!({
            "type": "error",
            "title": title,
            "message": message
        })
    ).unwrap();
}