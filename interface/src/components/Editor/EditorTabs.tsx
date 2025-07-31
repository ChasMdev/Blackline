import { useState, useEffect, useRef } from "react";
import Editor from "@monaco-editor/react";
import "./EditorTabs.css";
import { Plus, File, X, Play, Trash, FileInput, Save, Syringe, CircleX } from "lucide-react";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";

interface Tab {
    id: string;
    name: string;
    content: string;
}

interface ContextMenu {
    visible: boolean;
    x: number;
    y: number;
    tabId: string;
}

export default function EditorTabs() {
    const [tabs, setTabs] = useState<Tab[]>([]);
    const [activeTabId, setActiveTabId] = useState("");
    const [contextMenu, setContextMenu] = useState<ContextMenu>({ visible: false, x: 0, y: 0, tabId: "" });
    const [isRenaming, setIsRenaming] = useState<string | null>(null);
    const [renameValue, setRenameValue] = useState("");
    const [isLoaded, setIsLoaded] = useState(false);
    const tabBarRef = useRef<HTMLDivElement>(null);
    const [pipeConnected, setPipeConnected] = useState(false);

    const activeTab = tabs.find((tab) => tab.id === activeTabId);

    const openNewTab = (title: string | undefined, content: string | undefined) => {
        setTabs(currentTabs => {
            const tabName = title ? title : "Untitled Tab";
            const tabContent = content ? content : "-- Start typing!";
            
            // Check if a tab with the same name and content already exists
            const existingTab = currentTabs.find(tab => 
                tab.name === tabName && tab.content === tabContent
            );
            
            if (existingTab) {
                // Switch to the existing tab instead of creating a new one
                setActiveTabId(existingTab.id);
                saveTabs(currentTabs, existingTab.id);
                return currentTabs;
            }
            
            const newTab: Tab = {
                id: Date.now().toString(),
                name: tabName,
                content: tabContent,
            };
            
            const updatedTabs = [...currentTabs, newTab];
            setActiveTabId(newTab.id);
            saveTabs(updatedTabs, newTab.id);
            
            setTimeout(() => {
                if (tabBarRef.current) {
                    tabBarRef.current.scrollLeft = tabBarRef.current.scrollWidth;
                }
            }, 10);
            
            return updatedTabs;
        });
    };

    useEffect(() => {
        const initializeTabs = async () => {
            try {
                const savedData = await invoke<{ tabs: Tab[], active_tab_id: string }>("load_tabs");
                
                if (savedData && savedData.tabs.length > 0) {
                    setTabs(savedData.tabs);
                    setActiveTabId(savedData.active_tab_id);
                } else {
                    const defaultTab: Tab = { id: "1", name: "Untitled Tab", content: "-- Start typing!" };
                    setTabs([defaultTab]);
                    setActiveTabId("1");
                    await saveTabs([defaultTab], "1");
                }
            } catch (error) {
                console.error("Failed to load tabs:", error);
                // Fallback to default tab
                const defaultTab: Tab = { id: "1", name: "Untitled Tab", content: "-- Start typing!" };
                setTabs([defaultTab]);
                setActiveTabId("1");
            }
            setIsLoaded(true);
        };

        initializeTabs();
    }, []);

    useEffect(() => {
      const checkPipe = async () => {
        try {
          const connected = await invoke<boolean>("is_pipe_connected");
          setPipeConnected(connected);
        } catch (err) {
          console.error("Failed to check pipe:", err);
        }
      };

      checkPipe();
      const interval = setInterval(checkPipe, 1000);
      return () => clearInterval(interval);
    }, []);

    useEffect(() => {
        const setupListener = async () => {
            try {
                const unlisten = await listen<{ fileName: string; content: string; path: string }>(
                    "open-file-in-tab",
                    (event) => {
                        const { fileName, content, path } = event.payload;
                        console.log("Opening file:", fileName);
                        openNewTab(stripExtension(fileName), content);
                    }
                );
                return unlisten;
            } catch (error) {
                console.error("Failed to set up file open listener:", error);
                return null;
            }
        };

        setupListener().then((unlisten) => {
            return () => {
                if (unlisten) unlisten();
            };
        });
    }, []); // Remove tabs dependency

    useEffect(() => {
        const handleWheel = (e: WheelEvent) => {
            if (e.shiftKey && tabBarRef.current) {
                e.preventDefault();
                tabBarRef.current.scrollLeft += e.deltaY;
            }
        };

        const tabBar = tabBarRef.current;
        if (tabBar) {
            tabBar.addEventListener('wheel', handleWheel);
            return () => tabBar.removeEventListener('wheel', handleWheel);
        }
    }, []);

    useEffect(() => {
        const handleKeyDown = (e: KeyboardEvent) => {
            if (e.ctrlKey && e.key === 'w') {
                e.preventDefault();
                closeTab(activeTabId);
            } else if (e.key === 'F2') {
                e.preventDefault();
                startRename(activeTabId);
            }
        };

        document.addEventListener('keydown', handleKeyDown);
        return () => document.removeEventListener('keydown', handleKeyDown);
    }, [activeTabId, tabs.length]);

    useEffect(() => {
        const handleClickOutside = () => {
            setContextMenu({ visible: false, x: 0, y: 0, tabId: "" });
        };

        if (contextMenu.visible) {
            document.addEventListener('click', handleClickOutside);
            return () => document.removeEventListener('click', handleClickOutside);
        }
    }, [contextMenu.visible]);

    const handleEditorChange = (value: string | undefined) => {
        if (!activeTab || value === undefined) return;

        const updatedTabs = tabs.map((t) =>
            t.id === activeTab.id ? { ...t, content: value } : t
        );

        setTabs(updatedTabs);
        saveTabs(updatedTabs, activeTab.id);
    };

    const closeTab = (id: string) => {
        if (tabs.length <= 1) {
            return;
        }

        setTabs((prevTabs) => {
            const newTabs = prevTabs.filter((tab) => tab.id !== id);
            
            let newActiveId = activeTabId;
            if (id === activeTabId) {
                const index = prevTabs.findIndex((t) => t.id === id);
                const nextTab = newTabs[index] || newTabs[index - 1];
                newActiveId = nextTab?.id ?? "";
                setActiveTabId(newActiveId);
            }
            
            saveTabs(newTabs, newActiveId);
            
            return newTabs;
        });
    };

    const cloneTab = (id: string) => {
        const tabToClone = tabs.find(tab => tab.id === id);
        if (!tabToClone) return;

        const newTab: Tab = {
            id: Date.now().toString(),
            name: `${tabToClone.name} (Copy)`,
            content: tabToClone.content,
        };
    
        const updatedTabs = [...tabs, newTab];

        setTabs(updatedTabs);
        setActiveTabId(newTab.id);
        saveTabs(updatedTabs, newTab.id);
        
        setTimeout(() => {
            if (tabBarRef.current) {
                tabBarRef.current.scrollLeft = tabBarRef.current.scrollWidth;
            }
        }, 10);
    };

    const startRename = (id: string) => {
        const tab = tabs.find(t => t.id === id);
        if (tab) {
            setIsRenaming(id);
            setRenameValue(tab.name);
        }
    };
    const confirmRename = () => {
        if (!isRenaming || !renameValue.trim()) return;

        const updatedTabs = tabs.map(tab =>
            tab.id === isRenaming ? { ...tab, name: renameValue.trim() } : tab
        );

        setTabs(updatedTabs);
        saveTabs(updatedTabs, activeTabId);
        setIsRenaming(null);
        setRenameValue("");
    };
    const cancelRename = () => {
        setIsRenaming(null);
        setRenameValue("");
    };

    const clearTab = (id: string) => {
        const updatedTabs = tabs.map(tab =>
            tab.id === id ? { ...tab, content: "" } : tab
        );

        setTabs(updatedTabs);
        saveTabs(updatedTabs, activeTabId);
    };

    const handleRightClick = (e: React.MouseEvent, tabId: string) => {
        e.preventDefault();
        setContextMenu({
            visible: true,
            x: e.pageX - 10,
            y: e.pageY - 50,
            tabId: tabId
        });
    };

    const saveTabs = async (tabs: Tab[], activeTabId: string) => {
        try {
            await invoke("save_tabs", { tabs, activeTabId });
        } catch (err) {
            console.error("Failed to save tabs:", err);
        }
    };

    const handleOpenFile = async () => {
        try {
            const result = await invoke<[string, string] | null>("open_file");

            if (result) {
                const [fileName, content] = result;
                openNewTab(stripExtension(fileName), content);
            }
        } catch (error) {
            console.error("Failed to open file:", error);
        }
    };

    const handleSaveFile = async () => {
        if (!activeTab) return;

        try {
            const result = await invoke<string | null>("save_file", {
                fileName: stripExtension(activeTab.name),
                content: activeTab.content
            });

            if (result) {
                const updatedTabs = tabs.map(tab =>
                    tab.id === activeTab.id ? { ...tab, name: stripExtension(result) } : tab
                );

                setTabs(updatedTabs);
                saveTabs(updatedTabs, activeTab.id);
            }
        } catch (error) {
            console.error("Failed to save file:", error);
        }
    };

    function stripExtension(filename: string): string {
        return filename.replace(/\.[^/.]+$/, "");
    }

    const handleCloseRoblox = async () => {
        invoke('close_roblox')
    }

    return (
        <div className="editor-container">
            <div className="editor-island">
                <div className="divider-bar"/>
            </div>
            <div className="tab-bar" ref={tabBarRef}>
                {tabs.map((tab) => (
                    <div
                        key={tab.id}
                        data-tab-id={tab.id}
                        className={`tab ${tab.id === activeTabId ? "active" : ""}`}
                        onClick={() => {
                            setActiveTabId(tab.id);
                            saveTabs(tabs, tab.id);
                        }}
                        onContextMenu={(e) => handleRightClick(e, tab.id)}
                    >
                        <File size={20} className="tab-icon"/>
                        {isRenaming === tab.id ? (
                            <input
                                type="text"
                                value={renameValue}
                                onChange={(e) => setRenameValue(e.target.value)}
                                onBlur={confirmRename}
                                onKeyDown={(e) => {
                                    if (e.key === 'Enter') {
                                        confirmRename();
                                    } else if (e.key === 'Escape') {
                                        cancelRename();
                                    }
                                }}
                                className="tab-rename-input"
                                autoFocus
                                onClick={(e) => e.stopPropagation()}
                                style={{
                                    width: `${Math.max(80, renameValue.length * 8 + 16)}px`
                                }}
                            />
                        ) : (
                            <span className="tab-text">{tab.name}</span>
                        )}
                        {tabs.length > 1 && (
                            <button className="close-button" onClick={(e) => {
                                e.stopPropagation();
                                closeTab(tab.id);
                            }}>
                                <X size={16}/>
                            </button>
                        )}
                    </div>
                ))}
                <button className="new-tab-button" onClick={() => openNewTab(undefined, undefined)}>
                    <Plus size={16} />
                </button>
            </div>

            {contextMenu.visible && (
                <div 
                    className="context-menu"
                    style={{
                        position: 'absolute',
                        left: contextMenu.x,
                        top: contextMenu.y,
                        zIndex: 3
                    }}
                    onClick={(e) => e.stopPropagation()}
                >
                    <div className="context-menu-item" onClick={() => {
                        cloneTab(contextMenu.tabId);
                        setContextMenu({ visible: false, x: 0, y: 0, tabId: "" });
                    }}>
                        Clone Tab
                    </div>
                    <div className="context-menu-item" onClick={() => {
                        startRename(contextMenu.tabId);
                        setContextMenu({ visible: false, x: 0, y: 0, tabId: "" });
                    }}>
                        Rename Tab
                    </div>
                    <div className="context-menu-item" onClick={() => {
                        clearTab(contextMenu.tabId);
                        setContextMenu({ visible: false, x: 0, y: 0, tabId: "" });
                    }}>
                        Clear Tab
                    </div>
                    {tabs.length > 1 && (
                        <div className="context-menu-item" onClick={() => {
                            closeTab(contextMenu.tabId);
                            setContextMenu({ visible: false, x: 0, y: 0, tabId: "" });
                        }}>
                            Close Tab
                        </div>
                    )}
                </div>
            )}

            <div className="editor-wrapper">
                <Editor
                    className="code-editor monaco-force-size"
                    value={activeTab?.content}
                    defaultLanguage="lua"
                    onMount={(editor, monaco) => {
                        monaco.editor.defineTheme("transparent-theme", {
                            base: "vs-dark",
                            inherit: true,
                            rules: [],
                            colors: {
                                "editor.background": "#00000000",
                                "editorLineNumber.foreground": "#cccccc",
                                "editorLineNumber.activeForeground": "#ffffff",
                                "editor.selectionBackground": "#ffffff10",
                                "editor.inactiveSelectionBackground": "#ffffff08",
                                "editor.focusedStackFrameHighlightBackground": "#00000000",
                                "editor.lineHighlightBackground": "#00000000",
                                "editor.lineHighlightBorder": "#00000000",
                                "editor.selectionHighlightBorder": "#00000000",
                                "editor.rangeHighlightBorder": "#00000000"
                            },
                        });
                        monaco.editor.setTheme("transparent-theme");

                        editor.onMouseDown((e: { event: { rightButton: any; preventDefault: () => void; }; }) => {
                            if (e.event.rightButton) {
                                e.event.preventDefault();
                            }
                        });
                    }}
                    onChange={handleEditorChange}
                    options={{
                        fontFamily: "Cascadia Code",
                        fontSize: 14,
                        minimap: { enabled: false },
                        wordWrap: "off",
                        scrollBeyondLastLine: false,
                        contextMenu: false
                    }}
                />
            </div>
            <div className="button-container">
                <button
                    className="control-button"
                    onClick={async () => {
                        if (!activeTab) return;
                        try {
                            await invoke("send_to_pipe", { content: activeTab.content });
                        } catch (err) {
                            console.error("Execution failed:", err);
                        }
                    }}
                    disabled={!pipeConnected}
                    title={!pipeConnected ? "Attach before executing" : ""}
                >
                    <Play size={18} className="control-button-icon"/>
                    <span className="control-button-text">Execute</span>
                </button>
                <button className="control-button" onClick={() => clearTab(activeTabId)}><Trash size={18} className="control-button-icon"/><span className="control-button-text">Clear</span></button>
                <button className="control-button" onClick={handleOpenFile}><FileInput size={18} className="control-button-icon"/><span className="control-button-text">Open</span></button>
                <button className="control-button" onClick={handleSaveFile}><Save size={18} className="control-button-icon"/><span className="control-button-text">Save</span></button>
                <button className="control-button" onClick={handleCloseRoblox}><CircleX size={18} className="control-button-icon"/><span className="control-button-text">Kill Roblox</span></button>
                <button
                    className="control-button rightmost"
                    onClick={async () => {
                        const isRunning = await invoke<boolean>("is_roblox_running");
                        if (!isRunning) {
                            await invoke("show_warn_alert", {
                                type: "warn",
                                title: "Roblox Not Running"
                            });
                            return;
                        }
                      
                        try {
                            await invoke("run_injector");
                        } catch (err) {
                            console.error("Injector failed:", err);
                        }
                    }}
                    disabled={pipeConnected}
                    title={pipeConnected ? "Already injected" : ""}
                >
                  <Syringe size={18} className="control-button-icon"/>
                  <span className="control-button-text">Attach</span>
                </button>
            </div>
        </div>
    );
}