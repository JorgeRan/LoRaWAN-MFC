import express from "express";
import { Server } from "socket.io";
import http from "http";
import https from "https";
import fs from "fs";
import fetch from "node-fetch";
import { decode } from "punycode";
import { Console } from "console";

const PORT = 3000;
const app = express();
app.use(express.json());

let server;
let usingHttps = false;
try {
  const options = {
    key: fs.readFileSync("localhost-key.pe"),
    cert: fs.readFileSync("localhost.pem"),
  };
  server = https.createServer(options, app);
  usingHttps = true;
} catch (err) {
  console.warn(
    "TLS key/cert not found or couldn't be read, falling back to HTTP:",
    err.message,
  );
  server = http.createServer(app);
}

const io = new Server(server);

const APP_ID_MFC_1 = "eerl-mfc";
const DEVICE_ID_MFC_1 = "mfc-node-01";

const APP_ID_MFC_2 = "eerl-mfc";
const DEVICE_ID_MFC_2 = "mfc-node-01";

const API_KEY =
  "NNSXS.ELCJY4CDOZIVNZAK2XKI7YDO4L3UI5MG43OXCSA.N22HW7G5ACVPRRIOLJIA2V3ZKG4YKN5BI73TVH4TKPKN7VKXDSRQ";

const TTN_API_URL = "http://172.17.52.12:1885/api/v3";

let gatewayTime = "";

let lastValue_1 = null;
let deviceState_1 = false;
let lastFlow_1 = 0.0;
let lastSetpoint_1 = 0.0;

let lastValue_2 = null;
let deviceState_2 = false;
let lastFlow_2 = 0.0;
let lastSetpoint_2 = 0.0;

function decodeUplink(bytes, setpoint_1, setpoint_2, flow_1, flow_2) {
  const results = [];

  const payloadType = bytes[0];

  if (payloadType === 0x1f) {
    // Error uplink: [0x1F, errorSource, code]
    if (bytes.length < 3) {
      console.warn(`Error uplink too short: expected 3 bytes, got ${bytes.length}`);
      return [results, setpoint_1, setpoint_2, flow_1, flow_2];
    }
    const errorSource = bytes[1];
    const errorCode = bytes[2];
    results.push({
      type: "error",
      errorSource: errorSource,
      errorCode: errorCode,
    });
  } else if (payloadType === 0x30) {
    // Heartbeat uplink: [0x30, 0x00]
    results.push({
      type: "heartbeat",
      message: "Device is alive",
    });

    
  } else if (payloadType === 0x20) {
    // Status uplink: [0x20, mfcId, setpoint(4 bytes), flow(4 bytes)]
    if (bytes.length < 10) {
      console.warn(`Status uplink too short: expected 10 bytes, got ${bytes.length}. Payload: ${JSON.stringify(bytes)}`);
      return [results, setpoint_1, setpoint_2, flow_1, flow_2];
    }
    const mfcId = bytes[1];
    const setpointBytes = bytes.slice(2, 6);
    const flowBytes = bytes.slice(6, 10);

    const setpointValue = bytesToFloat(setpointBytes);
    const flowValue = bytesToFloat(flowBytes);

    results.push({
      type: "status",
      mfcId: mfcId,
      setpoint: setpointValue.toFixed(2),
      flow: flowValue.toFixed(2),
      unit: "LN/min",
    });

    if (mfcId == 0) {
      setpoint_1 = setpointValue.toFixed(2);
      flow_1 = flowValue.toFixed(2);
    } else if (mfcId == 1) {
      setpoint_2 = setpointValue.toFixed(2);
      flow_2 = flowValue.toFixed(2);
    }
  }

  return [results, setpoint_1, setpoint_2, flow_1, flow_2];
}

function bytesToFloat(bytes) {
  const view = new DataView(new Uint8Array(bytes).buffer);
  return view.getFloat32(0, false);
}

app.get("/", (req, res) => {
  res.send(`
    <h1>MFC LoRaWAN Controller</h1>
    <h2>MFC BK</h2>
    <p>Time: ${gatewayTime} </p>
    <p>Last uplink: ${JSON.stringify(lastValue_1)}</p>
    <p>Device state: ${deviceState_1 ? "ON" : "OFF"}</p>
    <p>Current Flow: ${lastFlow_1} </p>
    <p>Setpoint: ${lastSetpoint_1} ln/min</p>

    <button onclick="sendSetpoint_1()">SET SETPOINT</button>

    


    <h2>MFC BL</h2>
    <p>Time: ${gatewayTime} </p>
    <p>Last uplink: ${JSON.stringify(lastValue_2)}</p>
    <p>Device state: ${deviceState_2 ? "ON" : "OFF"}</p>
    <p>Current Flow: ${lastFlow_2} </p>
    <p>Setpoint: ${lastSetpoint_2} ln/min</p>

    <button onclick="sendSetpoint_2()">SET SETPOINT</button>

    <script src="/socket.io/socket.io.js"></script>
    <script>
      const socket = io();
      socket.on("uplink", d => console.log("Live uplink:", d));

      function send_1(cmd) {
        fetch('/send-command-1', {
          method: 'POST',
          headers: {'Content-Type':'application/json'},
          body: JSON.stringify({command: cmd})
        });
      }

      function sendSetpoint_1() {
        const value = prompt("Enter float setpoint (ln/min) :");
        fetch('/setpoint-1', {
          method: 'POST',
          headers: {'Content-Type':'application/json'},
          body: JSON.stringify({value})
        });
      }
      function send_2(cmd) {
        fetch('/send-command-2', {
          method: 'POST',
          headers: {'Content-Type':'application/json'},
          body: JSON.stringify({command: cmd})
        });
      }

      function sendSetpoint_2() {
        const value = prompt("Enter float setpoint (ln/min) :");
        fetch('/setpoint-2', {
          method: 'POST',
          headers: {'Content-Type':'application/json'},
          body: JSON.stringify({value})
        });
      }
    </script>
  `);
});

app.post("/uplink", (req, res) => {
  try {
    const data = req.body;

    gatewayTime = data["uplink_message"]["received_at"];

    console.log(gatewayTime);
    if (data.uplink_message?.frm_payload) {
      const payload = Buffer.from(data.uplink_message.frm_payload, "base64");

      const arrayPayload = Array.from(payload);

      if (arrayPayload[0] == 32 && arrayPayload[1] == 0) {
        lastValue_1 = Array.from(payload);
      } else if (arrayPayload[0] == 32 && arrayPayload[1] == 1) {
        lastValue_2 = Array.from(payload);
      }

      const decodedUplink = decodeUplink(arrayPayload, lastSetpoint_1, lastSetpoint_2, lastFlow_1, lastFlow_2);

      const decoded = decodedUplink[0];
      lastSetpoint_1 = decodedUplink[1];
      lastSetpoint_2 = decodedUplink[2];
      lastFlow_1 = decodedUplink[3];
      lastFlow_2 = decodedUplink[4];

      console.log("Raw payload:", arrayPayload);
      console.log("Decoded:", decoded[0]);

      io.emit("uplink", decoded[0]);
    }

    res.sendStatus(200);
  } catch (err) {
    console.error("UPLINK ERROR:", err);
    res.sendStatus(400);
  }
});

async function sendDownlink(bytes, fPort = 15, mfc) {
  let url = "";
  const payload = {
    downlinks: [
      {
        frm_payload: Buffer.from(bytes).toString("base64"),
        f_port: fPort,
        priority: "NORMAL",
      },
    ],
  };

  // const url = `${TTN_API_URL}/as/applications/${APP_ID_MFC_1}/devices/${DEVICE_ID_MFC_1}/down/replace`;

  if (mfc == 1) {
    url = `${TTN_API_URL}/as/applications/${APP_ID_MFC_1}/devices/${DEVICE_ID_MFC_1}/down/replace`;
  } else if (mfc == 2) {
    url = `${TTN_API_URL}/as/applications/${APP_ID_MFC_2}/devices/${DEVICE_ID_MFC_2}/down/replace`;
  }

  const r = await fetch(url, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      Authorization: `Bearer ${API_KEY}`,
    },
    body: JSON.stringify(payload),
  });

  if (!r.ok) throw new Error(await r.text());
}

app.post("/send-command-1", async (req, res) => {
  const { command } = req.body;
  let bytes;

  if (command === "on") bytes = [1];
  else if (command === "off") bytes = [0];
  else if (command === "toggle") bytes = [2];
  else return res.status(400).json({ error: "Unknown command" });

  await sendDownlink(bytes, 1);

  if (command === "on") deviceState_1 = true;
  if (command === "off") deviceState_1 = false;
  if (command === "toggle") deviceState_1 = !deviceState_1;

  res.json({ ok: true });
});

app.post("/setpoint-1", async (req, res) => {
  const value = parseFloat(req.body.value);
  if (isNaN(value)) return res.status(400).json({ error: "Invalid float" });

  const buf = Buffer.alloc(6);
  buf[0] = 0x10;
  buf[1] = 0x00;  // MFC ID 0
  buf.writeFloatBE(value, 2);

  await sendDownlink([...buf], 15, 1);
  lastSetpoint_1 = value;

  res.json({ ok: true });
});

app.post("/send-command-2", async (req, res) => {
  const { command } = req.body;
  let bytes;

  if (command === "on") bytes = [1];
  else if (command === "off") bytes = [0];
  else if (command === "toggle") bytes = [2];
  else return res.status(400).json({ error: "Unknown command" });

  await sendDownlink(bytes, 2);

  if (command === "on") deviceState_1 = true;
  if (command === "off") deviceState_1 = false;
  if (command === "toggle") deviceState_1 = !deviceState_1;

  res.json({ ok: true });
});

app.post("/setpoint-2", async (req, res) => {
  const value = parseFloat(req.body.value);
  if (isNaN(value)) return res.status(400).json({ error: "Invalid float" });

  const buf = Buffer.alloc(6);
  buf[0] = 0x10;
  buf[1] = 0x01;  // MFC ID 1
  buf.writeFloatBE(value, 2);

  await sendDownlink([...buf], 15, 2);
  lastSetpoint_2 = value;

  res.json({ ok: true });
});

io.on("connection", (s) => {
  s.emit("initial", {
    lastValue_1,
    deviceState_1,
    lastSetpoint_1,
    lastValue_2,
    deviceState_2,
    lastSetpoint_2,
  });
});

server.listen(PORT, () => {
  const proto = usingHttps ? "https" : "http";
  console.log(`${proto}://localhost:${PORT}`);
});
