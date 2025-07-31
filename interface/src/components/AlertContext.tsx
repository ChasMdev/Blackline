import React, { createContext, useContext, useState } from 'react';

type AlertType = 'success' | 'warn' | 'error';

interface Alert {
  id: string;
  type: AlertType;
  title: string;
  message?: string;
  timeout?: number;
}

interface AlertContextType {
  alerts: Alert[];
  showAlert: (type: AlertType, title: string, message?: string, timeout?: number) => void;
  dismissAlert: (id: string) => void;
}

const AlertContext = createContext<AlertContextType>({
  alerts: [],
  showAlert: () => {},
  dismissAlert: () => {},
});

export const useAlert = () => useContext(AlertContext);

export const AlertProvider: React.FC<{children: React.ReactNode}> = ({ children }) => {
  const [alerts, setAlerts] = useState<Alert[]>([]);

  const showAlert = (type: AlertType, title: string, message?: string, timeout = 5000) => {
    const id = Math.random().toString(36).substring(2, 9);
    const newAlert = { id, type, title, message, timeout };
    
    setAlerts(prev => [...prev, newAlert]);
    
    if (timeout > 0) {
      setTimeout(() => dismissAlert(id), timeout);
    }
  };

  const dismissAlert = (id: string) => {
    setAlerts(prev => prev.filter(alert => alert.id !== id));
  };

  return (
    <AlertContext.Provider value={{ alerts, showAlert, dismissAlert }}>
      {children}
    </AlertContext.Provider>
  );
};