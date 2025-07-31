import { invoke } from "@tauri-apps/api/core";
import "./App.css";
import Titlebar from "./components/Titlebar";
import { EyeOff, Eye, CircleX } from "lucide-react";
import { useEffect, useState } from "react";

function App() {
    const [showPassword, setShowPassword] = useState(false);
    const [password, setPassword] = useState("");
    const [username, setUsername] = useState("");
    const [error, setError] = useState("");
    const [phase, setPhase] = useState<"login" | "bootstrap" | "blacklisted" | "nokey">("login");
    const [progress, setProgress] = useState(0);
    const [downloading, setDownloading] = useState("Preparing...");
    const [_, setAccentColor] = useState("#37C7EE");

    const gifOptions = ["/loader1.gif", "/loader2.gif"];
    const [backgroundGif] = useState(() => {
        return gifOptions[Math.floor(Math.random() * gifOptions.length)];
    });

    const saveToken = async (token: string) => {
        await invoke("set_token", { token });
    };

    const getToken = async (): Promise<string | null> => {
        try {
            return await invoke("get_token");
        } catch {
            return null;
        }
    };

    const clearToken = async () => {
        await invoke("delete_token");
    };

    // Get accent color with fallback chain (custom -> system -> default)
    async function getAccentColor() {
        // base loader accent color off of background rather than system / custom (works better with backgrounds)
        console.log(backgroundGif);

        if (backgroundGif === "/loader1.gif") {
            return { hex: '#37C7EE', r: 55, g: 199, b: 238 };
        } else {
            return { hex: '#D1FFD4', r: 209, g: 255, b: 212 };
        }
        /*try {
            const color = await invoke('get_accent_color');
            console.log('Accent color:', color);
            return color;
        } catch (error) {
            console.error('Failed to get accent color:', error);
            // This shouldn't happen since we have fallbacks, but just in case
            return { hex: '#37C7EE', r: 55, g: 199, b: 238 };
        }*/
    }

    // Initialize accent color on app load
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

    const handleSignIn = async () => {
        if (!username) return setError("Username is required.");
        if (!password) return setError("Password is required.");

        setError("");

        let hwid;
        try {
            hwid = await invoke("get_hwid");
        } catch {
            return setError("Unable to fetch HWID");
        }

        try {
            const res = await fetch("http://127.0.0.1:8000/login", {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                    "X-HWID": hwid as string,
                },
                body: JSON.stringify({ username, password }),
            });

            const data = await res.json();

            console.log(data)

            if (!res.ok) {
                await clearToken();
                return setError(data.detail || "Login failed");
            }

            // Save token if login successful
            if (data.token) {
                await saveToken(data.token);
            }

            if (data.blacklisted) {
                setPhase("blacklisted");
                await clearToken();
                return;
            }

            if (!data.hasKey) {
                setPhase("nokey")
                return;
            }

            console.log("✅ Logged in");

            setTimeout(() => {
                setPhase("bootstrap");
                // Simulate download
                let current = 0;
                const interval = setInterval(() => {
                    current += 1;
                    setProgress(current);
                    setDownloading(`File_${current}.bin`);
                    if (current >= 100) clearInterval(interval);
                }, 30);
            }, 400); // allow animations to complete
        } catch (e) {
            setError("Network error");
        }
    };

    // Auto-login effect - now depends on store being initialized
    useEffect(() => {
        (async () => {
            const token = await getToken();
            if (!token) return;

            const hwid = await invoke("get_hwid");

            const res = await fetch("http://127.0.0.1:8000/token-login", {
                method: "POST",
                headers: {
                    Authorization: token,
                    "X-HWID": hwid as string,
                },
            });

            const data = await res.json();

            if (!res.ok) {
                console.warn("Auto-login failed:", data.detail);
                await clearToken();
                return;
            }

            if (data.blacklisted) {
                setPhase("blacklisted");
                await clearToken()
                return;
            }

            if (!data.hasKey) {
                setPhase("nokey")
                return;
            }

            setTimeout(() => {
                setPhase("bootstrap");
                let current = 0;
                const interval = setInterval(() => {
                    current += 1;
                    setProgress(current);
                    setDownloading(`File_${current}.bin`);
                    if (current >= 100) clearInterval(interval);
                }, 30);
            }, 400);
        })();
    }, []);

    return (
        <div className="border">
            <div className="container">
                <div className="glow-container">
                    <div className="glow-square" />
                </div>
                <Titlebar/>
                <img src={backgroundGif} alt="Loading..." className="background-gif" />

                <div className={`center-island ${phase}`}>

                    <div className={`login-elements ${phase === "login" || phase === "blacklisted" || phase === "nokey" ? "visible" : ""}`}>
                        <div className="login-title">Log In</div>

                        <input type="text" className="login-input" placeholder="Username" value={username} onChange={(e) => setUsername(e.target.value)} />

                        <div className="password-container">
                            <div className={`password-input-wrapper ${showPassword ? "fade-in" : "fade-out"}`}>
                                <input
                                    type={showPassword ? "text" : "password"}
                                    className="password-input"
                                    placeholder="Password"
                                    value={password}
                                    onChange={(e) => setPassword(e.target.value)}
                                />
                            </div>
                            <div className="eye-icon" onClick={() => setShowPassword(!showPassword)}>
                                {showPassword ? <EyeOff size={18} /> : <Eye size={18} />}
                            </div>
                        </div>

                        <div className="forgot-password">Forgot Password?</div>

                        {error && <div className="error-message">{error}</div>}

                        <button className="sign-in-button" onClick={() => handleSignIn()}>Sign In</button>
                    </div>
                    <div className={`bootstrap-elements ${phase === "bootstrap" ? "visible" : ""}`}>
                        <div className="bootstrap-title">Downloading Files</div>

                        <div className="progress-bar-container">
                            <div className="progress-bar-fill" style={{ width: `${progress}%` }} />
                        </div>

                        <div className="bootstrap-status">{downloading}</div>
                        <div className="bootstrap-percent">{progress}%</div>
                    </div>
                </div>
                <div className={`error-elements ${phase === "blacklisted" || phase === "nokey" ? "visible" : ""}`}>
                    <div className={`error-island ${phase}`}>
                        <CircleX size={30} className="error-icon"/>
                        <span className="error-title">There was an error!</span>
                        <span className={`error-info ${phase === "nokey" ? "nokey" : ""}`}>Your account is not authorized to use Blackline at the moment.  Please purchase a key before continuing.</span>
                        <span className={`error-info ${phase === "blacklisted" ? "blacklisted" : ""}`}>You have been blacklisted from using Blackline.  You can find out why and appeal it at our website, Blackline.gg or in our discord server, discord.gg/blackline</span>
                    </div>
                </div>
            </div>
        </div>
    );
}

export default App;