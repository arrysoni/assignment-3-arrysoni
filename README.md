# CMPSC 311 – Assignment 3: mdadm Linear Device (Write, Permissions, and Trace Replay)

## 📝 Overview

This assignment implements write functionality in a simulated multi-disk storage environment (JBOD) through the `mdadm` interface. You will extend your previous `mdadm_read` functionality by:

- Implementing a `mdadm_write()` function for block writes
- Managing write permissions through `mdadm_write_permission()` and `mdadm_revoke_write_permission()`
- Supporting unit tests and trace-based validations

The goal is to mimic a unified disk system with proper write safety, permission control, and trace-driven testing.

---

## ⚙️ Functions Implemented

### 1. `int mdadm_write(uint32_t start_addr, uint32_t write_len, const uint8_t *write_buf);`

- Writes `write_len` bytes from `write_buf` to the disk starting at `start_addr`
- Returns:
  - `-1` if address is out of bounds
  - `-2` if `write_len > 1024`
  - `-3` if system is unmounted
  - `-4` for other internal errors
  - `-5` if write permission is not enabled
  - Else: returns number of bytes written

---

### 2. `int mdadm_write_permission(void);`

- Enables write operations on the JBOD system
- Must be called before any write
- Returns `0` on success, `-1` on failure

---

### 3. `int mdadm_revoke_write_permission(void);`

- Revokes write permissions, making JBOD read-only
- Returns `0` on success, `-1` on failure

---

## 📦 Interaction with JBOD

Implemented functions use the `jbod_operation(op, *block)` interface with the following enum commands:

- `JBOD_WRITE_BLOCK`
- `JBOD_WRITE_PERMISSION`
- `JBOD_REVOKE_WRITE_PERMISSION`

Writes are only allowed:
- When the system is mounted
- When permission is granted
- When address and buffer length are valid

---

## 🧪 Testing

### 🔹 Unit Testing
Run:
```bash
./tester
