// ws-server/server.js
const { createServer } = require('http');
const WebSocket = require('ws');

const PORT = process.env.PORT || 3001;

// Buat HTTP server kosong (tanpa Next.js)
const server = createServer();

// Buat WebSocket Server
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
