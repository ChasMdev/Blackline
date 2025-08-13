import "./App.css";
import { AlertProvider, useAlert } from "./components/AlertContext";
import { AlertComponent } from "./components/AlertComponent";
import { listen } from '@tauri-apps/api/event';
import { invoke } from "@tauri-apps/api/core";
import { useEffect, useRef, useState } from "react";
import Titlebar from "./titlebar";
import EditorGrid from "./components/Editor/Editor";
import { CircleX } from "lucide-react";

function AppContent() {
    const { showAlert } = useAlert();
    const listenerSetup = useRef(false);
    const [phase, setPhase] = useState<"mainwindow" | "blacklisted" | "nokey" | "serverdown" | "hwid" | "token">("mainwindow");
    const [_, setAccentColor] = useState("#D1FFD4");

    // Get accent color with fallback chain (custom -> system -> default)
    async function getAccentColor() {
        return { hex: '#D1FFD4', r: 209, g: 255, b: 212 };
    }

    // Set accent color
    async function setAccentColorValue(hexColor: string) {
        try {
            await invoke('set_accent_color', { hexColor });
        } catch (error) {
            console.error('Failed to set accent color:', error);
        }
    }

    // Get system accent color specifically
    async function getSystemAccentColor() {
        try {
            const color = await invoke('get_system_accent_color');
            return color;
        } catch (error) {
            console.error('Failed to get system accent color:', error);
            return null;
        }
    }

    // Get all accent colors (custom, system, default)
    async function getAllAccentColors() {
        try {
            const colors = await invoke('get_all_accent_colors');
            return colors;
        } catch (error) {
            console.error('Failed to get all accent colors:', error);
            return null;
        }
    }

    // Remove custom accent color (will fall back to system or default)
    async function removeAccentColor() {
        try {
            await invoke('remove_accent_color');
        } catch (error) {
            console.error('Failed to remove accent color:', error);
        }
    }

    useEffect(() => {
        (async () => {
            const colorData = await getAccentColor();
            // @ts-ignore
            setAccentColor(colorData.hex);

            // Apply the color to CSS custom properties
            // @ts-ignore
            document.documentElement.style.setProperty('--accent-color', colorData.hex);
            // @ts-ignore
            document.documentElement.style.setProperty('--accent-color-rgb', `${colorData.r}, ${colorData.g}, ${colorData.b}`);
        })();
    }, []);

  useEffect(() => {
    const handleContextMenu = (e: MouseEvent) => {
      e.preventDefault();
    };

    window.addEventListener("contextmenu", handleContextMenu);

    return () => {
      window.removeEventListener("contextmenu", handleContextMenu);
    };
  }, []);

  useEffect(() => {
    if (listenerSetup.current) return;
    
    let unlisten: (() => void) | undefined;
    let isActive = true;
    
    const setupListener = async () => {
      try {
        unlisten = await listen('show-alert', (event) => {
          if (!isActive) {
            console.log("Ignoring event from stale listener");
            return;
          }
          const payload = event.payload as { type: 'success' | 'warn' | 'error', title: string, message?: string };
          showAlert(payload.type, payload.title, payload.message);
        });
        listenerSetup.current = true;
      } catch (error) {
        console.error('Failed to set up alert listener:', error);
      }
    };
  
    setupListener();

    invoke("verify_and_download_files")
    .catch((err) => {
      console.error("Module download failed:", err);
      showAlert(
        "error",
        "Module Download Failed",
        typeof err === "string" ? err : "An unexpected error occurred."
      );
    });
  
    return () => {
      isActive = false;
      if (unlisten) {
        unlisten();
      }
      listenerSetup.current = false;
    };
  }, []);

  const getToken = async (): Promise<string | null> => {
      try {
          return await invoke("get_token");
      } catch (error) {
          return null;
      }
  };
  
  const clearToken = async () => {
      await invoke("delete_token");
  };

  // initialize loading screen n shit here
  useEffect(() => {
      (async () => {
          const token = await getToken();
          if (!token) {
              console.log("no token gg")
              setPhase("token");
              return;
          }
          const hwid = await invoke("get_hwid");
          let data;
          let res;
          try {
              res = await fetch("http://18.170.1.254:8000/auth/token-login", {
                  method: "POST",
                  headers: {
                      Authorization: token,
                      "X-HWID": hwid as string,
                  },
              });
          } catch (error) {
              setPhase("serverdown")
              return;
          }
          data = await res.json();

          if (!res.ok) {
              console.warn("Auto-login failed:", data.detail);

              if (data.detail) {
                  if (data.detail === "Invalid or expired token") {
                      setPhase("token")
                      await clearToken()
                  } else if (data.detail === "Incorrect HWID") {
                      setPhase("hwid")
                      await clearToken()
                  }
              }
              return;
          }
          if (data.blacklisted) {
              setPhase("blacklisted");
              await invoke ("delete_token")
              return;
          }
          if (!data.hasKey) {
              setPhase("nokey")
              return;
          }
      })();
  }, []);

  return (
      <div className="border">
          <div className="mainapp">
              <div className="glow-container">
                  <div className="glow-square" />
              </div>
              <AlertComponent/>
              <Titlebar/>
              <EditorGrid/>
          </div>
          <div className="loadingpage">
              <div className={"center-glow"}/>
              <div className={"loading-title"}>BLACKLINE</div>
              <svg width="38" height="38" viewBox="0 0 38 38" className="loading-wheel">
                  <circle cx="19" cy="19" r="16.5" fill="none" stroke="currentColor" strokeWidth="5" strokeLinecap="round" strokeDasharray="43.6 60.4" opacity="0.8"/>
              </svg>
              <div className="info-box">Initializing components</div>
          </div>
      </div>
  );
}

function App() {
    return (
        <AlertProvider>
            <AppContent/>
        </AlertProvider>
    );
}

export default App;
