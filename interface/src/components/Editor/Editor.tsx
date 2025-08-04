import "./Editor.css"
import { useState, useEffect, useRef } from "react";
import { TerminalRounded } from "@mui/icons-material";
import { listen } from '@tauri-apps/api/event';
import { invoke } from "@tauri-apps/api/core";
import EditorTabs from "./EditorTabs";
import { ChevronDown } from "lucide-react";
import FileTree from "./FileTree";

type LogEffect = 'normal' | 'success' | 'warn' | 'error';

interface ConsoleLog {
    message: string;
    effect: LogEffect;
    timestamp: string;
}

export default function EditorGrid() {
    const [consoleOpen, setConsoleOpen] = useState(true);
    const [logs, setLogs] = useState<ConsoleLog[]>([]);
    const consoleRef = useRef<HTMLDivElement | null>(null);
    const consoleEndRef = useRef<HTMLDivElement | null>(null);
    const [isUserPinned, setIsUserPinned] = useState(true);
    const [isFirstRender, setIsFirstRender] = useState(true);
    const [isToggling, setIsToggling] = useState(false);
    const loggerStartedRef = useRef(false);

    useEffect(() => {
        const timer = setTimeout(() => {
            setIsFirstRender(false);
        }, 10);
        
        return () => clearTimeout(timer);
    }, []);

    const toggleConsole = () => {
        if (isToggling) return;
        
        setIsToggling(true);
        setConsoleOpen(!consoleOpen);
        
        setTimeout(() => {
            setIsToggling(false);
        }, 250);
    };    

    const handleClearConsole = () => {
        setLogs([]);
        consoleRef.current?.scrollTo({ top: 0, behavior: 'auto' });
    };

    useEffect(() => {
        const unlisten = listen<ConsoleLog>('console-log', (event) => {
            const now = new Date();
            const formattedTime = now.toLocaleTimeString('en-GB', { hour12: false });
        
            setLogs((prevLogs) => [
                ...prevLogs,
                { ...event.payload, timestamp: formattedTime }
            ]);
        });
    
        return () => {
            unlisten.then((f) => f());
        };
    }, []);

    useEffect(() => {
        (async () => {
            let token: string | null = null;

            try {
                token = await invoke("get_token");
            } catch {
                return null;
            }

            if (!token) return;

            const hwid = await invoke("get_hwid");
            const res = await fetch("http://127.0.0.1:8000/auth/token-login", {
                method: "POST",
                headers: {
                    Authorization: token, // Now TypeScript knows this is a string
                    "X-HWID": hwid as string,
                },
            });
            const data = await res.json();

            invoke("console_startup", { username: data.username });
        })();
    }, []);

    useEffect(() => {
        const consoleElement = consoleRef.current;

        const handleScroll = () => {
            if (!consoleElement) return;

            const { scrollTop, scrollHeight, clientHeight } = consoleElement;

            if (scrollTop + clientHeight >= scrollHeight - 5) {
                setIsUserPinned(true);
            } else {
                setIsUserPinned(false);
            }
        };

        consoleElement?.addEventListener('scroll', handleScroll);

        return () => {
            consoleElement?.removeEventListener('scroll', handleScroll);
        };
    }, []);

    useEffect(() => {
        if (consoleOpen && isUserPinned) {
            consoleEndRef.current?.scrollIntoView({ behavior: 'smooth' });
        }
    }, [logs]);

    useEffect(() => {
        if (consoleOpen && isUserPinned) {
            const scrollTimeout = setTimeout(() => {
                consoleEndRef.current?.scrollIntoView({ behavior: 'smooth' });
            }, 220);
            
            return () => clearTimeout(scrollTimeout);
        }
    }, [consoleOpen]);

    return (
        <div className="editor-grid">

            <div className={`editor-tabs-wrapper ${consoleOpen ? 'open' : 'closed'} ${isFirstRender ? 'no-transition' : ''}`}>
                <EditorTabs />
            </div>
            <FileTree/>

            <div className={`console-control ${consoleOpen ? 'open' : 'closed'} ${isFirstRender ? 'no-transition' : ''}`}>
                <button
                    className="console-toggle"
                    onClick={toggleConsole}
                    disabled={isToggling}
                >
                    <ChevronDown className="console-chevron" size={20}/>
                    <TerminalRounded className="console-icon" sx={{ fontSize: 20 }}/>
                    <span className="console-text">Console</span>
                </button>
                <button className="clear-console" onClick={handleClearConsole}>
                       <span color="currentColor" className="clear-text">Clear</span>
                    </button>
                <div className={`background-of-console-real ${consoleOpen ? 'open' : 'closed'}`}>
                    <div className={`console-background ${consoleOpen ? 'open' : 'closed'}`} ref={consoleRef}>
                        {logs.map((log, index) => (
                            <p key={index} className={`console-log-line ${log.effect}`}>
                                &gt; {log.timestamp} - {log.message}
                            </p>
                        ))}
                        <div ref={consoleEndRef}></div>
                    </div>
                </div>
            </div>
        </div>
    );
}