const { createServer } = require('http');
const WebSocket = require('ws');
const url = require('url');

const PORT = process.env.PORT || 3001;

const server = createServer((req, res) => {
  if (req.url === '/') {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end('Server is running\n');
  } else {
    res.writeHead(404);
    res.end();
  }
});

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

wss.on('error', (error) => {
  console.error('WebSocket error:', error);
});

server.on('upgrade', (request, socket, head) => {
  const pathname = url.parse(request.url).pathname;
  console.log(`Upgrade request to ${pathname}`);

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
