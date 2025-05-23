const WebSocket = require('ws');
const express = require('express');
const { createServer } = require('http');

const app = express();
const server = createServer(app);
const wss = new WebSocket.Server({ server });

const PORT = 3001;
let espSocket = null;

wss.on('connection', (ws, req) => {
  const clientIP = req.socket.remoteAddress;
  console.log(`🔌 New WebSocket connection from ${clientIP}`);

  ws.on('message', (msg) => {
    const message = msg.toString();
    console.log(`📩 Message from ${clientIP}:`, message);

    if (message === 'hello-from-esp32') {
      espSocket = ws;
      ws.send('👋 Welcome ESP32');
      console.log('✅ ESP32 registered as WebSocket client');
    }
  });

  ws.on('close', () => {
    console.log(`❌ Disconnected: ${clientIP}`);
    if (ws === espSocket) {
      espSocket = null;
      console.log('🔌 ESP32 disconnected');
    }
  });
});

// API to send message to ESP32
app.get('/send-to-esp32', (req, res) => {
  if (espSocket && espSocket.readyState === WebSocket.OPEN) {
    espSocket.send('🚨 Alert from server to ESP32!');
    return res.send('✅ Message sent to ESP32');
  } else {
    return res.status(404).send('❌ ESP32 not connected');
  }
});

// Use 0.0.0.0 to allow access from other devices on the same network
server.listen(PORT, '0.0.0.0', () => {
  console.log(`🚀 Server running at http://0.0.0.0:${PORT}`);
});
