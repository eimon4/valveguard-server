require('dotenv').config();
const express = require('express');
const cors = require('cors');
const { createClient } = require('@supabase/supabase-js');
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3000;

// ── Supabase client (service-role key for server-side writes) ──────────────────
const supabaseUrl = process.env.SUPABASE_URL;
const supabaseServiceKey = process.env.SUPABASE_SERVICE_KEY;

if (!supabaseUrl || !supabaseServiceKey) {
  console.error('❌  Missing SUPABASE_URL or SUPABASE_SERVICE_KEY in .env');
  process.exit(1);
}

const supabase = createClient(supabaseUrl, supabaseServiceKey);

// ── Middleware ─────────────────────────────────────────────────────────────────
app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

// ── POST /api/phone ─────────────────────────────────────────────────────────
app.post('/api/phone', async (req, res) => {
  let { user_id, phone_number } = req.body;

  if (!user_id || !phone_number) {
    return res.status(400).json({ success: false, message: 'user_id and phone_number are required.' });
  }

  // Normalize phone number to E.164 format for Supabase (assuming Philippines +63)
  let formattedPhone = phone_number.trim();
  if (formattedPhone.startsWith('09')) {
    formattedPhone = '+63' + formattedPhone.substring(1);
  } else if (formattedPhone.startsWith('639')) {
    formattedPhone = '+' + formattedPhone;
  } else if (!formattedPhone.startsWith('+')) {
    formattedPhone = '+' + formattedPhone;
  }

  // 1. Update the user's metadata and primary phone number in Supabase Auth
  const { data, error } = await supabase.auth.admin.updateUserById(
    user_id,
    { 
      phone: formattedPhone,
      phone_confirm: true, // Automatically confirm so they don't need SMS verification
      user_metadata: { phone_number: formattedPhone } 
    }
  );

  if (error) {
    console.error('Auth update error:', error.message);
    return res.status(500).json({ success: false, message: `Failed to update phone number in auth: ${error.message}`, details: error.message });
  }

  // 2. Also update the phone number in the devices database table for this user
  const { error: dbError } = await supabase
    .from('devices')
    .update({ phone_number: formattedPhone })
    .eq('user_id', user_id);

  if (dbError) {
    console.error('Devices database update error:', dbError.message);
    return res.status(500).json({ success: false, message: 'Failed to update phone number in devices table.', details: dbError.message });
  }

  return res.status(200).json({ success: true, phone_number: formattedPhone });
});

// ── GET /api/phone/:userId ──────────────────────────────────────────────────
// Returns plain text phone number for the ESP32
app.get('/api/phone/:userId', async (req, res) => {
  const { userId } = req.params;

  const { data, error } = await supabase.auth.admin.getUserById(userId);

  if (error || !data || !data.user) {
    console.error('Auth fetch error:', error?.message);
    return res.status(404).send("Error: User not found");
  }

  const phone = data.user.user_metadata?.phone_number;
  
  if (!phone) {
    return res.status(404).send("Error: No phone number");
  }

  // Return as plain text for easy ESP32 parsing
  res.type('text/plain');
  return res.send(phone);
});

// ── Catch-all: serve HTML pages from /public ──────────────────────────────────
app.get('*', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// ── Start server ──────────────────────────────────────────────────────────────
app.listen(PORT, '0.0.0.0', () => {
  console.log(`🚀  ValveGuard server running at http://localhost:${PORT}`);
});
