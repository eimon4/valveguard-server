# ValveGuard – LPG Gas Leak Detection & Monitoring System

A full-stack gas leak monitoring web app built with **HTML + Tailwind CSS**, **Node.js + Express**, and **Supabase**.

---

## 📁 Folder Structure

```
capst/
├── public/
│   ├── index.html           ← Landing page
│   ├── login.html           ← User login
│   ├── register.html        ← User registration
│   ├── forgot-password.html ← Email password reset
│   └── dashboard.html       ← Gas monitoring dashboard
├── .env.example             ← Environment variables template
├── .gitignore
├── package.json
├── schema.sql               ← Supabase database schema
└── server.js                ← Node.js Express backend
```

---

## ⚙️ Prerequisites

- [Node.js](https://nodejs.org/) v18 or later
- A [Supabase](https://supabase.com/) account and project

---

## 🚀 Setup Instructions

### Step 1 – Clone / navigate to the project folder

```bash
cd capst
```

### Step 2 – Install Node.js dependencies

```bash
npm install
```

### Step 3 – Set up Supabase

1. Go to [https://supabase.com](https://supabase.com) and create a new project.
2. In your Supabase project, open **SQL Editor** and run the contents of `schema.sql`.
   This creates the `gas_logs` table with the correct structure.
3. Supabase Auth is enabled by default. No extra setup needed for login/register/forgot-password.

### Step 4 – Configure environment variables

Copy `.env.example` to `.env`:

```bash
copy .env.example .env
```

Open `.env` and fill in your Supabase credentials:

```
SUPABASE_URL=https://your-project-id.supabase.co
SUPABASE_SERVICE_KEY=your-service-role-secret-key
PORT=3000
```

- **SUPABASE_URL** → Project Settings → API → Project URL
- **SUPABASE_SERVICE_KEY** → Project Settings → API → `service_role` secret key (**keep this private!**)

### Step 5 – Update Supabase credentials in HTML files

The HTML pages use the **anon/public** key (safe for browser). Open each of these files and replace the placeholder values:

| File | Variables to update |
|------|---------------------|
| `public/login.html` | `SUPABASE_URL`, `SUPABASE_ANON` |
| `public/register.html` | `SUPABASE_URL`, `SUPABASE_ANON` |
| `public/forgot-password.html` | `SUPABASE_URL`, `SUPABASE_ANON` |
| `public/dashboard.html` | `SUPABASE_URL`, `SUPABASE_ANON` |

Your **anon key** is found at: Project Settings → API → `anon` public key.

Example (replace in each HTML file):

```js
const SUPABASE_URL  = 'https://your-project-id.supabase.co';
const SUPABASE_ANON = 'your-anon-public-key';
```

### Step 6 – Run the development server

```bash
npm run dev
```

Or, without nodemon:

```bash
npm start
```

### Step 7 – Open the website

Open your browser and go to:

```
http://localhost:3000
```

---

## 🌐 Pages

| URL | Page |
|-----|------|
| `http://localhost:3000/` | Landing Page |
| `http://localhost:3000/login.html` | Login |
| `http://localhost:3000/register.html` | Register |
| `http://localhost:3000/forgot-password.html` | Forgot Password |
| `http://localhost:3000/dashboard.html` | Dashboard |

---

## 🔌 API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| `POST` | `/api/gas-log` | Insert a new gas reading |
| `GET` | `/api/gas-logs` | Fetch all gas readings (latest first) |

### POST `/api/gas-log` – Payload Example

```json
{
  "gas_value": 450,
  "status": "Leak Detected"
}
```

Valid status values: `"Leak Detected"` or `"Safe"`

---

## 🧪 Simulation

Since the ESP32 hardware is not yet connected, use the **Dashboard simulation buttons**:

- **⚠️ Simulate Gas Leak** → sends a random gas_value between 300–700 with status `"Leak Detected"`
- **✅ Simulate Safe Reading** → sends a random gas_value between 100–250 with status `"Safe"`

---

## 🗄️ Database Schema (`gas_logs` table)

| Column | Type | Description |
|--------|------|-------------|
| `id` | UUID | Primary key (auto-generated) |
| `gas_value` | INTEGER | Raw sensor reading |
| `status` | TEXT | `"Leak Detected"` or `"Safe"` |
| `created_at` | TIMESTAMPTZ | Auto-set to UTC timestamp |

---

## 🔒 Security Notes

- The backend uses the **service-role** key (via `.env`) for database writes — never expose this key in the browser.
- The HTML pages use the **anon** key for Supabase Auth only — this is safe to include in client-side code.
- Row-Level Security (RLS) is enabled on `gas_logs`.
- API inputs are validated: `gas_value` must be a non-negative integer; `status` must be one of the two valid values.
- The dashboard page redirects unauthenticated users to `login.html`.

---

## 🛠️ Tech Stack

| Layer | Technology |
|-------|-----------|
| Frontend | HTML5 + Tailwind CSS (CDN) |
| Backend | Node.js + Express |
| Database | Supabase (PostgreSQL) |
| Auth | Supabase Auth |

---

## 📡 ESP32 Integration (Future)

When the hardware is ready, program the ESP32 to send HTTP POST requests to:

```
POST http://<your-server-ip>:3000/api/gas-log
Content-Type: application/json

{ "gas_value": <sensor_reading>, "status": "Leak Detected" }
```
