const { createServer } = require('http');
const WebSocket = require('ws');
const PORT = process.env.PORT || 3001;
const server = createServer();
const wss = new WebSocket.Server({ server });
const clients = new Set();
wss.on('connection', (ws) => {
  console.log('🌐 New client connected');
  clients.add(ws);
  ws.on('message', (data) => {
    console.log('📦 Data received:', data);
    for (let client of clients) {
      if (client.readyState === WebSocket.OPEN && client !== ws) {
        client.send(data);
      }
    }
  });
  ws.on('close', () => {
    clients.delete(ws);
    console.log('❌ Client disconnected');
  });
});
server.listen(PORT, () => {
  console.log(`🚀 WebSocket server running at http://localhost:${PORT}`);
});
// const express = require('express');
// const { createServer } = require('http');
// const WebSocket = require('ws');
// const bodyParser = require('body-parser');

// const app = express();
// const server = createServer(app);
// const wss = new WebSocket.Server({ server });

// const PORT = process.env.PORT || 3001;
// const clients = new Set();

// app.use(bodyParser.json());

// wss.on('connection', (ws) => {
//   console.log("🧩 Web client connected");
//   clients.add(ws);
//   ws.on('close', () => clients.delete(ws));
// });

// // ✅ Endpoint untuk menerima data dari ESP32
// app.post('/esp32', (req, res) => {
//   const data = req.body;
//   console.lSog('📩 Data dari ESP32:', data);

//   // Broadcast ke semua client
//   for (let client of clients) {
//     if (client.readyState === WebSocket.OPEN) {
//       client.send(JSON.stringify(data));
//     }
//   }

//   res.sendStatus(200); // Kirim respon sukses ke ESP
// });
// app.get('/latest-sensor-data', (req, res) => {
//   if (latestSensorData) {
//     res.json(latestSensorData);
//   } else {
//     res.status(404).json({ error: "No sensor data available yet." });
//   }
// });


// server.listen(PORT, () => {
//   console.log(`🚀 Server running at http://localhost:${PORT}`);
// });
