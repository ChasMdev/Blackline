import { Minus, X } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';
import "./titlebar.css"

export default function Titlebar() {
  return (
    <div className="titlebar">
      <div className="titlebar-left">
        <span className="titlebar-title">BLACKLINE</span>
      </div>
      <div className="titlebar-drag" />
      <div className="titlebar-buttons">
        <button onClick={() => invoke('minimize_app')} className="titlebar-btn minimize">
          <Minus color="currentColor" />
        </button>
        <button onClick={() => invoke('exit_app')} className="titlebar-btn close">
          <X color="currentColor" />
        </button>
      </div>
      <div className='divider'/>
    </div>
  );
}