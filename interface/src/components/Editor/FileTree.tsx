import React, { useEffect, useState, useRef } from "react";
import { ChevronRight, Folder, FolderOpen, ScrollText, Github, FastForward, FileJson2, FileDigit, FileText, File, FileImage, FileAudio2, FolderArchive } from "lucide-react";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import { emit } from "@tauri-apps/api/event";
import "./FileTree.css"

interface FileNode {
  name: string;
  path: string;
  type: "file" | "folder";
  children?: FileNode[];
}

const customIcons: Record<string, JSX.Element> = {
  "Scripts": <ScrollText size={18} />,
  "Github Gists": <Github size={18} />,
  "Auto Execute": <FastForward size={18} />,
};

const fileIcons = (ext: string) => {
  switch (ext) {
    case ".lua":
    case ".luau":
      return <FileJson2 size={18} />;
    case ".bin":
      return <FileDigit size={18} />;
    case ".txt":
      return <FileText size={18} />;
    case ".jpg":
    case ".jpeg":
    case ".gif":
    case ".png":
    case ".tiff":
    case ".svg":
        return <FileImage size={18} />;
    case ".mp3":
    case ".ogg":
    case ".wav":
        return <FileAudio2 size={18}/>
    case ".zip":
    case ".rar":
        return <FolderArchive size={18}/>
    default:
      return <File size={18} />;
  }
};

const FileTree: React.FC = () => {
  const [tree, setTree] = useState<FileNode[]>([]);
  const [expanded, setExpanded] = useState<Record<string, boolean>>({});
  const [closing, setClosing] = useState<Record<string, boolean>>({});
  const [contextMenu, setContextMenu] = useState<{
    x: number;
    y: number;
    node: FileNode | null;
  } | null>(null);
  const intervalRef = useRef<number | null>(null);
  const [isRenaming, setIsRenaming] = useState<string | null>(null);
  const [renameValue, setRenameValue] = useState("");

  const loadTree = async () => {
    try {
      const result = await invoke<FileNode[]>("list_file_tree");
      setTree(result);
    } catch (error) {
      await invoke("show_error_alert", {
          type: "error",
          title: "Failed to load file tree",
          message: `Failed to load file tree: ${error}`
        });
      setTree([]);
    }
  };

  useEffect(() => {
    loadTree();

    const setupEventListener = async () => {
      try {
        const unlisten = await listen("file-tree-changed", () => {
          loadTree();
        });
        return unlisten;
      } catch (error) {
        return null;
      }
    };

    const startPolling = () => {
      intervalRef.current = window.setInterval(loadTree, 2000);
    };

    setupEventListener().then((unlisten) => {
      if (!unlisten) {
        startPolling();
      }
      return () => {
        if (unlisten) unlisten();
        if (intervalRef.current) clearInterval(intervalRef.current);
      };
    });

    return () => {
      if (intervalRef.current) {
        window.clearInterval(intervalRef.current);
      }
    };
  }, []);

  useEffect(() => {
    const handleClickOutside = () => setContextMenu(null);
    window.addEventListener("click", handleClickOutside);
    return () => window.removeEventListener("click", handleClickOutside);
  }, []);

  const countVisibleChildren = (node: FileNode): number => {
    if (!node.children) return 0;
    let count = node.children.length;
    for (const child of node.children) {
      if (child.type === "folder" && expanded[child.path]) {
        count += countVisibleChildren(child);
      }
    }
    return count;
  };

  const toggleFolder = (path: string, visibleChildrenCount: number) => {
    const isCurrentlyExpanded = expanded[path] ?? false;
    if (isCurrentlyExpanded) {
      const closingDuration = 300;
      setClosing(prev => ({ ...prev, [path]: true }));
      setTimeout(() => {
        setExpanded(prev => ({ ...prev, [path]: false }));
      }, 50);
      setTimeout(() => {
        setClosing(prev => ({ ...prev, [path]: false }));
      }, closingDuration + 100);
    } else {
      setExpanded(prev => ({ ...prev, [path]: true }));
    }
  };

  const handleFileClick = async (node: FileNode) => {
    try {
      const content = await invoke<string>("read_file", { path: node.path });
      const fileName = node.name;

      await emit("open-file-in-tab", {
          fileName: fileName,
          content: content,
          path: node.path
      });
    } catch (error) {
      try {
        await invoke("show_error_alert", {
          type: "error",
          title: "Failed to open file",
          message: `Could not open ${node.name}: ${error}`
        });
      } catch (alertError) {
        console.error("Failed to show error alert:", alertError);
      }
    }
  };

  const startRenaming = (node: FileNode) => {
    setIsRenaming(node.path);
    setRenameValue(node.name);
  };

  const confirmRename = async () => {
    if (!isRenaming || renameValue.trim() === "") return;

    try {
      await invoke("rename_file", {
        oldPath: isRenaming,
        newName: renameValue.trim(),
      });
      await loadTree();
    } catch (err) {
      await invoke("show_error_alert", {
        type: "error",
        title: "Rename failed",
        message: err
      });
    } finally {
      setIsRenaming(null);
    }
  };

  const cancelRename = () => {
    setIsRenaming(null);
  };

  const handleContextAction = async (action: string, node: FileNode) => {
    switch (action) {
      case "Open":
        if (node.type === "file") handleFileClick(node);
        else toggleFolder(node.path, countVisibleChildren(node));
        break;

      case "Rename": {
        startRenaming(node);
        break;
      }

      case "Clone": {
        try {
          await invoke("clone_file", { path: node.path });
          loadTree();
        } catch (err) {
          await invoke("show_error_alert", {
            type: "error",
            title: "Clone failed",
            message: err
          });
        }
        break;
      }

      case "Delete": {
        try {
          await invoke("delete_file", { path: node.path });
          loadTree();
        } catch (err) {
          await invoke("show_error_alert", {
            type: "error",
            title: "Delete failed",
            message: err
          });
        }
        break;
      }
    }

    setContextMenu(null);
  };

  const sortChildren = (children: FileNode[]) => {
    return [...children].sort((a, b) => {
      if (a.type === "folder" && b.type === "file") return -1;
      if (a.type === "file" && b.type === "folder") return 1;
      return a.name.localeCompare(b.name);
    });
  };

  const renderNode = (node: FileNode, depth = 0, index = 0, parentClosing = false) => {
    const isExpanded = expanded[node.path] ?? false;
    const isClosing = closing[node.path] ?? false;
    const paddingLeft = 10 * depth;

    if (node.type === "folder") {
      const icon = customIcons[node.name] ?? (isExpanded ? <FolderOpen size={18} /> : <Folder size={18} />);
      const sortedChildren = node.children ? sortChildren(node.children) : [];
      const visibleChildrenCount = countVisibleChildren(node);

      return (
        <div key={node.path} className="folder-container">
          <button
            className="file-tree-folder"
            style={{
              paddingLeft,
              animationDelay: parentClosing ? "0ms" : `${index * 75}ms`
            }}
            onClick={() => toggleFolder(node.path, visibleChildrenCount)}
            onContextMenu={(e) => {
              if (["Scripts", "Workspace", "Auto Execute", "Github Gists"].includes(node.name)) return;
              e.preventDefault();
              setContextMenu({ x: e.clientX, y: e.clientY - 15, node });
            }}
          >
            <span
              className={`chevron ${isExpanded ? "expanded" : ""}`}
              style={{ visibility: sortedChildren.length > 0 ? "visible" : "hidden" }}
            >
              <ChevronRight size={18} />
            </span>
            <span className="folder-icon">{icon}</span>
            {isRenaming === node.path ? (
              <input
                type="text"
                value={renameValue}
                onChange={(e) => setRenameValue(e.target.value)}
                onBlur={confirmRename}
                onKeyDown={(e) => {
                  if (e.key === "Enter") confirmRename();
                  else if (e.key === "Escape") cancelRename();
                }}
                className="tab-rename-input-explorer"
                autoFocus
                onClick={(e) => e.stopPropagation()}
                style={{
                  width: `${Math.max(80, renameValue.length * 8 + 16)}px`
                }}
              />
            ) : (
              <span className="folder-name">{node.name}</span>
            )}
          </button>
          <div
            className={`folder-content ${isExpanded ? "expanded" : ""} ${isClosing ? "closing" : ""}`}
            style={{ transition: `max-height 400ms ease, opacity 200ms ease` }}
          >
            {(isExpanded || isClosing) &&
              sortedChildren.map((child, childIndex) =>
                renderNode(child, depth + 1, childIndex, isClosing || parentClosing)
              )}
          </div>
        </div>
      );
    } else {
      const ext = node.name.slice(node.name.lastIndexOf(".")).toLowerCase();
      return (
        <button
          className="file-tree-file"
          key={node.path}
          style={{
            paddingLeft: paddingLeft + 10 + 14,
            animationDelay: parentClosing ? "0ms" : `${index * 75}ms`
          }}
          onClick={() => handleFileClick(node)}
          onContextMenu={(e) => {
            e.preventDefault();
            setContextMenu({ x: e.clientX, y: e.clientY - 15, node });
          }}
        >
          {fileIcons(ext)}
          {isRenaming === node.path ? (
            <input
              type="text"
              value={renameValue}
              onChange={(e) => setRenameValue(e.target.value)}
              onBlur={confirmRename}
              onKeyDown={(e) => {
                if (e.key === "Enter") confirmRename();
                else if (e.key === "Escape") cancelRename();
              }}
              className="tab-rename-input-explorer"
              autoFocus
              onClick={(e) => e.stopPropagation()}
              style={{
                width: `${Math.max(80, renameValue.length * 8 + 16)}px`
              }}
            />
          ) : (
            <span className="file-name">{node.name}</span>
          )}
        </button>
      );
    }
  };

  return (
    <div className="file-tree">
      <div className="file-tree-content">
        <div className="file-tree-inner">
          {tree.map(folder => renderNode(folder))}
        </div>
      </div>
      {contextMenu && contextMenu.node && (
        <ul
          className="context-menu-explorer"
          style={{
            position: "fixed",
            top: contextMenu.y,
            left: contextMenu.x,
            borderRadius: 3,
            listStyle: "none",
            zIndex: 1000,
          }}
          onClick={() => setContextMenu(null)}
        >
          {["Open", "Rename", "Clone", "Delete"].filter(action => {
            // Hide "Clone" option for folders
            if (action === "Clone" && contextMenu.node?.type === "folder") {
              return false;
            }
            return true;
          }).map((action) => (
            <li
              key={action}
              style={{
                padding: "6px 10px",
                cursor: "pointer",
              }}
              onClick={() => handleContextAction(action, contextMenu.node!)}
              onMouseDown={(e) => e.stopPropagation()}
            >
              {action}
            </li>
          ))}
        </ul>
      )}
    </div>
  );
};

export default FileTree;