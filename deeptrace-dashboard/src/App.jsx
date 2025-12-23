import { useState, useEffect } from 'react'
import axios from 'axios'
import { ShieldAlert, Activity, Server } from 'lucide-react'

function App() {
  const [alerts, setAlerts] = useState([])
  const [stats, setStats] = useState({ total_alerts: 0, most_frequent_attack: 'None' })
  const [loading, setLoading] = useState(true)

  const API_URL = "http://localhost:8000"

  const fetchData = async () => {
    try {
      const alertsRes = await axios.get(`${API_URL}/alerts`)
      const statsRes = await axios.get(`${API_URL}/stats`)
      setAlerts(alertsRes.data)
      setStats(statsRes.data)
      setLoading(false)
    } catch (error) {
      console.error("Error fetching data:", error)
    }
  }

  // Poll API every 2 seconds
  useEffect(() => {
    fetchData()
    const interval = setInterval(fetchData, 2000)
    return () => clearInterval(interval)
  }, [])

  return (
    <div style={{ maxWidth: '1200px', margin: '0 auto', padding: '20px' }}>
      {/* Header */}
      <div style={{ display: 'flex', alignItems: 'center', marginBottom: '30px', borderBottom: '1px solid #334155', paddingBottom: '20px' }}>
        <ShieldAlert size={40} color="#ef4444" style={{ marginRight: '15px' }} />
        <div>
          <h1 style={{ margin: 0 }}>DeepTrace NIDS</h1>
          <span style={{ color: '#94a3b8' }}>Real-time Network Threat Monitor</span>
        </div>
      </div>

      {/* Stats Cards */}
      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: '20px', marginBottom: '40px' }}>
        <Card title="System Status" value="ONLINE" color="#22c55e" icon={<Server />} />
        <Card title="Total Threats Detected" value={stats.total_alerts} color="#f59e0b" icon={<Activity />} />
        <Card title="Top Attack Type" value={stats.most_frequent_attack} color="#ef4444" icon={<ShieldAlert />} />
      </div>

      {/* Alerts Table */}
      <h2 style={{ marginBottom: '15px' }}>Recent Alerts</h2>
      <div style={{ background: '#1e293b', borderRadius: '8px', overflow: 'hidden' }}>
        <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left' }}>
          <thead style={{ background: '#334155', color: '#e2e8f0' }}>
            <tr>
              <th style={{ padding: '15px' }}>Timestamp</th>
              <th style={{ padding: '15px' }}>Attack Type</th>
              <th style={{ padding: '15px' }}>Confidence</th>
              <th style={{ padding: '15px' }}>Source IP</th>
              <th style={{ padding: '15px' }}>Target IP</th>
            </tr>
          </thead>
          <tbody>
            {alerts.length === 0 ? (
              <tr><td colSpan="5" style={{ padding: '20px', textAlign: 'center', color: '#94a3b8' }}>No threats detected (yet)...</td></tr>
            ) : (
              alerts.map((alert) => (
                <tr key={alert.id} style={{ borderBottom: '1px solid #334155' }}>
                  <td style={{ padding: '15px', color: '#94a3b8' }}>{new Date(alert.timestamp).toLocaleTimeString()}</td>
                  <td style={{ padding: '15px', color: '#ef4444', fontWeight: 'bold' }}>{alert.attack_type}</td>
                  <td style={{ padding: '15px' }}>{(alert.confidence * 100).toFixed(1)}%</td>
                  <td style={{ padding: '15px', fontFamily: 'monospace' }}>{alert.src_ip}</td>
                  <td style={{ padding: '15px', fontFamily: 'monospace' }}>{alert.dst_ip}</td>
                </tr>
              ))
            )}
          </tbody>
        </table>
      </div>
    </div>
  )
}

// Simple Card Component
const Card = ({ title, value, color, icon }) => (
  <div style={{ background: '#1e293b', padding: '20px', borderRadius: '8px', display: 'flex', alignItems: 'center' }}>
    <div style={{ background: `${color}20`, padding: '10px', borderRadius: '50%', marginRight: '15px', color: color }}>
      {icon}
    </div>
    <div>
      <div style={{ color: '#94a3b8', fontSize: '0.9rem' }}>{title}</div>
      <div style={{ fontSize: '1.5rem', fontWeight: 'bold', color: 'white' }}>{value}</div>
    </div>
  </div>
)

export default App