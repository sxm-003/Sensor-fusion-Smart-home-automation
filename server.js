const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const { Server } = require('socket.io');
const http = require('http');

// 1. Setup Web Server and Socket.io
const httpServer = http.createServer();
const io = new Server(httpServer, {
  cors: { origin: '*' }, // Allow any browser to connect
});

// 2. Setup Serial Port (Matches your VSCode Log)
// If this fails, double check the path in VSCode "Board Manager"
const port = new SerialPort({
  path: '/dev/tty.usbmodem14201',
  baudRate: 9600,
});

const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));

// 3. Handle Serial Data
port.on('open', () => {
  console.log('SERIAL PORT OPENED');
});

parser.on('data', (line) => {
  console.log('Arduino:', line);

  // Parse data to make it useful for the frontend
  let dataObj = {
    raw: line,
    timestamp: new Date().toLocaleTimeString(),
    type: 'info',
  };

  // Simple logic to tag the data type for the frontend
  if (line.includes('Light value')) dataObj.type = 'light';
  if (line.includes('Motion')) dataObj.type = 'motion';
  if (line.includes('Pre-off')) dataObj.type = 'timer';
  if (line.includes('Monitor state')) dataObj.type = 'state_change';

  // Send to Browser
  io.emit('arduino-data', dataObj);
});

// 4. Start Server
httpServer.listen(3000, () => {
  console.log('-----------------------------------------');
  console.log('Websocket Server running on port 3000');
  console.log('Open index.html in your browser');
  console.log('-----------------------------------------');
});
