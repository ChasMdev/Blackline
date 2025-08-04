import express from "express";
import cors from "cors";
import bodyParser from "body-parser";
import authRoutes from "./routes/authRoutes";
import { initDb } from "./db";

const app = express();
const PORT = 8000;

app.use(cors());
app.use(bodyParser.json());

initDb(); // Initialize database + tables

app.use("/auth", authRoutes);

app.get("/", (req, res) => {
  res.send("Node Auth Server Running");
});

app.listen(PORT, "127.0.0.1", () => {
  console.log(`Server running on http://127.0.0.1:${PORT}`);
});