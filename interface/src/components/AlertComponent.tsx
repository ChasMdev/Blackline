import React, { useEffect, useRef } from 'react';
import { useAlert } from './AlertContext';
import { CircleCheck, TriangleAlert, CircleX, X } from 'lucide-react';
import './Alert.css';

export const AlertComponent: React.FC = () => {
  const { alerts, dismissAlert } = useAlert();
  const alertRefs = useRef<Map<string, HTMLDivElement>>(new Map());
  const enteredAlerts = useRef<Set<string>>(new Set());
  
  useEffect(() => {
    alerts.forEach(alert => {
      const alertElement = alertRefs.current.get(alert.id);
      if (alertElement && !enteredAlerts.current.has(alert.id)) {
        alertElement.classList.add('alert-enter');
        enteredAlerts.current.add(alert.id);
      }
    });
    
    const currentAlertIds = new Set(alerts.map(a => a.id));
    enteredAlerts.current = new Set([...enteredAlerts.current].filter(id => currentAlertIds.has(id)));
  }, [alerts]);

  const getAlertColor = (type: string) => {
    switch (type) {
      case 'success': return ['#38A854', '#50fa7b'];
      case 'warn': return ['#949D2B', '#E1EF42'];
      case 'error': return ['#AF2F2F', '#FF5555'];
      default: return ['#38A854', '#50fa7b'];
    }
  };

  const getAlertIcon = (type: string) => {
    const color = getAlertColor(type);
    switch (type) {
      case 'success': return <CircleCheck size={20} color={color[1]} />;
      case 'warn': return <TriangleAlert size={20} color={color[1]} />;
      case 'error': return <CircleX size={20} color={color[1]} />;
      default: return <CircleCheck size={20} color={color[1]} />;
    }
  };

  const handleManualClose = (alertId: string) => {
    const alertElement = alertRefs.current.get(alertId);
    if (alertElement) {
      alertElement.classList.add('alert-exit-right');

      setTimeout(() => {
        dismissAlert(alertId);
      }, 300);
    }
  };

  const handleTimeoutClose = (alertId: string) => {
    const alertElement = alertRefs.current.get(alertId);
    if (alertElement) {
      alertElement.classList.add('alert-exit-right');

      setTimeout(() => {
        dismissAlert(alertId);
      }, 300);
    } else {
      dismissAlert(alertId);
    }
  };

  // Set up auto-dismiss for alerts with timeout
  useEffect(() => {
    const timeouts: number[] = [];
    
    alerts.forEach(alert => {
      if (alert.timeout && alert.timeout > 0) {
        const timeout = setTimeout(() => {
          handleTimeoutClose(alert.id);
        }, alert.timeout);
        timeouts.push(timeout);
      }
    });

    return () => {
      timeouts.forEach(timeout => clearTimeout(timeout));
    };
  }, [alerts]);

  return (
    <div className="alert-container">
      {alerts.map(alert => (
        <div 
          key={alert.id}
          className="alert"
          ref={el => {
            if (el) {
              alertRefs.current.set(alert.id, el);
            } else {
              alertRefs.current.delete(alert.id);
            }
          }}
          style={{
            transition: 'box-shadow 0.3s ease',
            color: getAlertColor(alert.type)[0],
            backgroundColor: `${getAlertColor(alert.type)[0]}66`
          }}
        >
          <div 
            className="alert-status-bar"
            style={{ backgroundColor: getAlertColor(alert.type)[0] }}
          />
          <div className="alert-content">
            <div className="alert-header">
              <div className="alert-icon">
                {getAlertIcon(alert.type)}
              </div>
              <div className="alert-title">{alert.title}</div>
            </div>
            {alert.message && (
              <div className="alert-message">{alert.message}</div>
            )}
            <button 
              className={`alert-close ${alert.message ? 'top-aligned' : 'centered'}`}
              onClick={() => handleManualClose(alert.id)}
              aria-label="Close alert"
            >
              <X size={16} />
            </button>
          </div>
        </div>
      ))}
    </div>
  );
};