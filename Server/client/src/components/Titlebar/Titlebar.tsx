import React, { useState } from 'react';
import './Titlebar.css';

function Titlebar() {
  const [selectedNav, setSelectedNav] = useState('Database'); // Default selection

  const navButtons = ['Database', 'Management', 'Sign Up'];

  return (
    <div className="titlebar">
        <div className="titlebar-left">
              <div className="logo-1">BLACKLINE</div>
              <div className="logo-2">Admin</div>
        </div>

        <div className="titlebar-center">
              {navButtons.map((button) => (
                <button 
                  key={button}
                  className={`nav-btn ${selectedNav === button ? 'nav-btn-selected' : ''}`}
                  onClick={() => setSelectedNav(button)}
                >
                  {button}
                </button>
              ))}
        </div>

        <div className="titlebar-right">
              <button className="login-btn">Login</button>
        </div>
    </div>
  );
}

export default Titlebar;