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
    const [_, setAccentColor] = useState("#37C7EE");

    // Get accent color with fallback chain (custom -> system -> default)
    async function getAccentColor() {
        try {
            const color = await invoke('get_accent_color');
            return color;
        } catch (error) {
            console.error('Failed to get accent color:', error);
            // This shouldn't happen since we have fallbacks, but just in case
            return { hex: '#37C7EE', r: 55, g: 199, b: 238 };
        }
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
              res = await fetch("http://127.0.0.1:8000/token-login", {
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
          <div className="app">
              <div className="glow-container">
                  <div className="glow-square" />
              </div>
              <AlertComponent/>
              <Titlebar/>
              <EditorGrid/>
              <div className={`error-elements ${phase === "blacklisted" || phase === "nokey" || phase === "serverdown" || phase === "token" || phase === "hwid" ? "visible" : ""}`}>
                  <div className={`error-island ${phase}`}>
                      <CircleX size={30} className="error-icon"/>
                      <span className="error-title">There was an error!</span>
                      <span className={`error-info ${phase === "blacklisted" ? "blacklisted" : ""}`}>You have been blacklisted from using Blackline.  You can find out why and appeal it at our website, Blackline.gg or in our discord server, discord.gg/blackline</span>
                      <span className={`error-info ${phase === "nokey" ? "nokey" : ""}`}>Your account is not authorized to use Blackline at the moment.  Please purchase a key before continuing.</span>
                      <span className={`error-info ${phase === "serverdown" ? "serverdown" : ""}`}>The servers are currently experiencing issues.  Please try again later.</span>
                      <span className={`error-info ${phase === "token" ? "token" : ""}`}>Your log-in has expired or you were not previously logged in.  Please re-launch the loader and log-in again.</span>
                      <span className={`error-info ${phase === "hwid" ? "hwid" : ""}`}>Incorect HWID!</span>
                  </div>
              </div>
          </div>
      </div>
  );
}

function App() {
  return (
    <AlertProvider>
      <AppContent />
    </AlertProvider>
  );
}

export default App;
