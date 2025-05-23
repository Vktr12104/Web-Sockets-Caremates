// ws-server/server.js
const { createServer } = require('http');
const WebSocket = require('ws');
const url = require('url');

const PORT = process.env.PORT || 3001;
const server = createServer(); // No response, pure upgrade

// Ubah ke noServer
const wss = new WebSocket.Server({ noServer: true });

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

// Handle HTTP Upgrade untuk path /ws
server.on('upgrade', (request, socket, head) => {
  const pathname = url.parse(request.url).pathname;

  if (pathname === '/ws') {
    wss.handleUpgrade(request, socket, head, (ws) => {
      wss.emit('connection', ws, request);
    });
  } else {
    socket.destroy();
  }
});

server.listen(PORT, () => {
  console.log(`🚀 WebSocket server running at http://localhost:${PORT}/ws`);
});
