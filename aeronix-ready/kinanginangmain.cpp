<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Soil & Climate Monitor + Fan Automation</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background-color: #0f172a;
      color: #f8fafc;
      padding: 20px;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    header { text-align: center; margin-bottom: 20px; }
    h1 { color: #38bdf8; font-size: 1.8rem; margin-bottom: 6px; }
    .status-bar {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      background: #1e293b;
      padding: 6px 14px;
      border-radius: 20px;
      font-size: 0.85rem;
      color: #94a3b8;
      border: 1px solid #334155;
    }
    .dot { height: 10px; width: 10px; background-color: #22c55e; border-radius: 50%; }

    /* Fan Banner Card */
    .fan-banner {
      width: 100%;
      max-width: 960px;
      background: #1e293b;
      border: 2px solid #334155;
      border-radius: 16px;
      padding: 20px;
      margin-bottom: 20px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      transition: all 0.3s ease;
    }
    .fan-banner.active {
      border-color: #38bdf8;
      background: rgba(56, 189, 248, 0.1);
    }
    .fan-info h2 { font-size: 1.2rem; margin-bottom: 4px; }
    .fan-reason { font-size: 0.9rem; color: #94a3b8; }
    .fan-state {
      font-size: 1.4rem;
      font-weight: 800;
      padding: 8px 20px;
      border-radius: 12px;
      background: #334155;
      color: #94a3b8;
    }
    .fan-banner.active .fan-state {
      background: #38bdf8;
      color: #0f172a;
    }

    /* Sensor Grid */
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(210px, 1fr));
      gap: 16px;
      width: 100%;
      max-width: 960px;
    }
    .card {
      background: #1e293b;
      border: 1px solid #334155;
      border-radius: 16px;
      padding: 24px;
      text-align: center;
      box-shadow: 0 4px 15px rgba(0, 0, 0, 0.25);
    }
    .label { font-size: 0.85rem; text-transform: uppercase; letter-spacing: 1px; color: #94a3b8; }
    .reading { font-size: 2.5rem; font-weight: 700; margin: 12px 0; }
    .unit { font-size: 1.1rem; color: #38bdf8; font-weight: 500; }
    .badge { display: inline-block; padding: 4px 12px; border-radius: 12px; font-size: 0.8rem; font-weight: 600; }
    .good { background: rgba(34, 197, 94, 0.15); color: #4ade80; }
    .warn { background: rgba(234, 179, 8, 0.15); color: #facc15; }
    .alert { background: rgba(239, 68, 68, 0.15); color: #f87171; }
    footer { margin-top: 25px; font-size: 0.8rem; color: #64748b; }
  </style>
</head>
<body>

  <header>
    <h1>Soil &amp; Climate Monitor</h1>
    <div class="status-bar">
      <span class="dot" id="conn-dot"></span>
      <span id="conn-text">Simulation Mode (No ESP32)</span>
    </div>
  </header>

  <!-- Automated DC12V Fan Status -->
  <div class="fan-banner" id="fan-box">
    <div class="fan-info">
      <h2>DC12V Ventilation Fan (Auto)</h2>
      <div class="fan-reason" id="fan-reason">Evaluating sensor thresholds...</div>
    </div>
    <div class="fan-state" id="fan-state">OFF</div>
  </div>

  <!-- Sensor Cards -->
  <div class="grid">
    <div class="card">
      <div class="label">Soil Moisture (40-70%)</div>
      <div class="reading"><span id="moisture">--</span><span class="unit"> %</span></div>
      <div class="badge good" id="moisture-status">Waiting...</div>
    </div>

    <div class="card">
      <div class="label">Temperature (18-30&deg;C)</div>
      <div class="reading"><span id="temp">--</span><span class="unit"> &deg;C</span></div>
      <div class="badge good" id="temp-status">Waiting...</div>
    </div>

    <div class="card">
      <div class="label">Humidity (50-75%)</div>
      <div class="reading"><span id="humidity">--</span><span class="unit"> %</span></div>
      <div class="badge good" id="humidity-status">Waiting...</div>
    </div>

    <div class="card">
      <div class="label">Soil pH (6.0-7.0)</div>
      <div class="reading"><span id="ph">--</span><span class="unit"> pH</span></div>
      <div class="badge good" id="ph-status">Waiting...</div>
    </div>
  </div>

  <footer>Last Updated: <span id="timestamp">--:--:--</span></footer>

  <script>
    const USE_SIMULATION = true; // Set to false when uploading to ESP32

    // Standard Threshold Constants
    const MAX_TEMP = 30.0;
    const MIN_TEMP = 18.0;
    const MAX_HUMIDITY = 75;
    const MIN_HUMIDITY = 50;
    const MAX_MOISTURE = 70;
    const MIN_MOISTURE = 40;
    const MIN_PH = 6.0;
    const MAX_PH = 7.0;

    function updateDashboard(data) {
      document.getElementById('moisture').innerText = data.moisture;
      document.getElementById('temp').innerText = data.temp;
      document.getElementById('humidity').innerText = data.humidity;
      document.getElementById('ph').innerText = data.ph;
      document.getElementById('timestamp').innerText = new Date().toLocaleTimeString();

      // 1. Soil Moisture Evaluation
      const mBadge = document.getElementById('moisture-status');
      if (data.moisture < MIN_MOISTURE) { mBadge.innerText = 'Too Dry (Water Needed)'; mBadge.className = 'badge alert'; }
      else if (data.moisture > MAX_MOISTURE) { mBadge.innerText = 'Too Wet (Drying)'; mBadge.className = 'badge warn'; }
      else { mBadge.innerText = 'Standard'; mBadge.className = 'badge good'; }

      // 2. Temperature Evaluation
      const tBadge = document.getElementById('temp-status');
      if (data.temp > MAX_TEMP) { tBadge.innerText = 'High Temp (Cooling)'; tBadge.className = 'badge alert'; }
      else if (data.temp < MIN_TEMP) { tBadge.innerText = 'Low Temp'; tBadge.className = 'badge warn'; }
      else { tBadge.innerText = 'Standard'; tBadge.className = 'badge good'; }

      // 3. Humidity Evaluation
      const hBadge = document.getElementById('humidity-status');
      if (data.humidity > MAX_HUMIDITY) { hBadge.innerText = 'High Humidity (Venting)'; hBadge.className = 'badge warn'; }
      else if (data.humidity < MIN_HUMIDITY) { hBadge.innerText = 'Low Humidity'; hBadge.className = 'badge warn'; }
      else { hBadge.innerText = 'Standard'; hBadge.className = 'badge good'; }

      // 4. Soil pH Evaluation
      const pBadge = document.getElementById('ph-status');
      if (data.ph < MIN_PH) { pBadge.innerText = 'Acidic (Needs Lime)'; pBadge.className = 'badge alert'; }
      else if (data.ph > MAX_PH) { pBadge.innerText = 'Alkaline (Needs Sulfur)'; pBadge.className = 'badge alert'; }
      else { pBadge.innerText = 'Standard'; pBadge.className = 'badge good'; }

      // 5. DC12V Fan Automation Logic
      let triggers = [];
      if (data.temp > MAX_TEMP) triggers.push(`Temp > ${MAX_TEMP}°C`);
      if (data.humidity > MAX_HUMIDITY) triggers.push(`Humidity > ${MAX_HUMIDITY}%`);
      if (data.moisture > MAX_MOISTURE) triggers.push(`Soil Moisture > ${MAX_MOISTURE}%`);

      // Use ESP32's reported fan state if available, otherwise compute locally
      const fanOn = (typeof data.fan !== 'undefined') ? data.fan : (triggers.length > 0);

      const fanBox = document.getElementById('fan-box');
      const fanState = document.getElementById('fan-state');
      const fanReason = document.getElementById('fan-reason');

      if (fanOn) {
        fanBox.classList.add('active');
        fanState.innerText = 'ON';
        fanReason.innerText = triggers.length > 0 
          ? `Active Trigger: ${triggers.join(' & ')}` 
          : 'Running to restore standard conditions';
      } else {
        fanBox.classList.remove('active');
        fanState.innerText = 'OFF';
        fanReason.innerText = 'All airflow-regulated parameters are within standard range';
      }
    }

    function fetchSensorData() {
      if (USE_SIMULATION) {
        const mockData = {
          moisture: Math.floor(Math.random() * 55) + 30,     // 30% - 85%
          temp: parseFloat((Math.random() * 14 + 21).toFixed(1)), // 21.0°C - 35.0°C
          humidity: Math.floor(Math.random() * 45) + 45,     // 45% - 90%
          ph: parseFloat((Math.random() * 2.6 + 5.2).toFixed(1))  // 5.2 - 7.8 pH
        };
        updateDashboard(mockData);
      } else {
        document.getElementById('conn-text').innerText = 'Connected to ESP32';
        fetch('/data')
          .then(res => res.json())
          .then(data => updateDashboard(data))
          .catch(() => {
            document.getElementById('conn-dot').style.backgroundColor = '#ef4444';
            document.getElementById('conn-text').innerText = 'ESP32 Disconnected';
          });
      }
    }

    fetchSensorData();
    setInterval(fetchSensorData, 3000);
  </script>
</body>
</html>