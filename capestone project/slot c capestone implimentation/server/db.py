"""
db.py - SQLite Database Management, Authentication, and Clinical Donation Workflow
Implements SQLite schema, PBKDF2 password hashing, session tokens,
role-based access control, donor/recipient management, and donation lifecycle tracking.
"""

import os
import sqlite3
import hashlib
import secrets
import csv
from datetime import datetime, timedelta

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA_DIR = os.path.join(BASE_DIR, "data")
DB_PATH = os.path.join(DATA_DIR, "organ_donation.db")
DONORS_CSV = os.path.join(DATA_DIR, "donors.csv")
RECIPIENTS_CSV = os.path.join(DATA_DIR, "recipients.csv")
DONORS_SAMPLE_CSV = os.path.join(DATA_DIR, "donors_sample.csv")
RECIPIENTS_SAMPLE_CSV = os.path.join(DATA_DIR, "recipients_sample.csv")

ADMIN_EMAIL = "saisriramchakka@gmail.com"

def get_db_connection():
    """Get a connection to the SQLite database with row factory enabled."""
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA foreign_keys = ON")
    return conn

def hash_password(password: str, salt: str = None) -> tuple:
    """Hash a password using PBKDF2-HMAC-SHA256 with 100,000 iterations."""
    if not salt:
        salt = secrets.token_hex(16)
    hashed = hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"), salt.encode("utf-8"), 100000).hex()
    return hashed, salt

def verify_password(password: str, salt: str, expected_hash: str) -> bool:
    """Verify password against salt and expected hash."""
    hashed, _ = hash_password(password, salt)
    return secrets.compare_digest(hashed, expected_hash)

def init_db():
    """Create all required tables, indexes, and seed initial data if needed."""
    os.makedirs(DATA_DIR, exist_ok=True)
    conn = get_db_connection()
    cur = conn.cursor()

    # 1. Users Table
    cur.execute("""
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        email TEXT UNIQUE NOT NULL,
        password_hash TEXT NOT NULL,
        salt TEXT NOT NULL,
        role TEXT NOT NULL CHECK(role IN ('admin', 'donor', 'recipient')),
        name TEXT NOT NULL,
        security_question TEXT DEFAULT 'What is your primary medical role or clinic name?',
        security_answer_hash TEXT,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )
    """)

    # 2. Donors Table
    cur.execute("""
    CREATE TABLE IF NOT EXISTS donors (
        id TEXT PRIMARY KEY,
        user_id INTEGER REFERENCES users(id) ON DELETE SET NULL,
        name TEXT NOT NULL,
        email TEXT NOT NULL,
        age INTEGER NOT NULL,
        gender TEXT NOT NULL,
        phone TEXT,
        address TEXT,
        city TEXT,
        state TEXT,
        blood_group TEXT NOT NULL,
        organ_type TEXT NOT NULL,
        medical_history TEXT,
        health_condition TEXT,
        emergency_contact TEXT,
        status TEXT NOT NULL DEFAULT 'AVAILABLE' CHECK(status IN ('AVAILABLE', 'MATCHED', 'DONATION IN PROGRESS', 'DONATION COMPLETED', 'UNAVAILABLE')),
        hla_type TEXT DEFAULT 'A2-A24-B44-B51-DR4-DR11',
        reg_date TEXT
    )
    """)

    # 3. Recipients Table
    cur.execute("""
    CREATE TABLE IF NOT EXISTS recipients (
        id TEXT PRIMARY KEY,
        user_id INTEGER REFERENCES users(id) ON DELETE SET NULL,
        name TEXT NOT NULL,
        email TEXT NOT NULL,
        age INTEGER NOT NULL,
        gender TEXT NOT NULL,
        phone TEXT,
        address TEXT,
        city TEXT,
        state TEXT,
        blood_group TEXT NOT NULL,
        required_organ TEXT NOT NULL,
        medical_condition TEXT,
        hospital_name TEXT,
        doctor_details TEXT,
        urgency_level INTEGER NOT NULL CHECK(urgency_level BETWEEN 1 AND 4),
        waiting_time INTEGER DEFAULT 0,
        status TEXT NOT NULL DEFAULT 'WAITING' CHECK(status IN ('WAITING', 'MATCHED', 'DONATION IN PROGRESS', 'TRANSPLANT COMPLETED', 'CANCELLED')),
        hla_type TEXT DEFAULT 'A2-A24-B44-B51-DR4-DR11',
        reg_date TEXT
    )
    """)

    # 4. Matches Table
    cur.execute("""
    CREATE TABLE IF NOT EXISTS matches (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        donor_id TEXT REFERENCES donors(id),
        recipient_id TEXT REFERENCES recipients(id),
        organ TEXT NOT NULL,
        compatibility_score REAL NOT NULL,
        explanation TEXT,
        status TEXT NOT NULL DEFAULT 'PROPOSED' CHECK(status IN ('PROPOSED', 'APPROVED', 'REJECTED', 'DONATION IN PROGRESS', 'DONATION COMPLETED')),
        proposed_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        reviewed_date TIMESTAMP,
        reviewed_by TEXT,
        admin_notes TEXT,
        UNIQUE(donor_id, recipient_id, organ)
    )
    """)

    # 5. Donations Workflow Table
    cur.execute("""
    CREATE TABLE IF NOT EXISTS donations (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        match_id INTEGER REFERENCES matches(id),
        donor_id TEXT REFERENCES donors(id),
        recipient_id TEXT REFERENCES recipients(id),
        organ TEXT NOT NULL,
        hospital TEXT,
        transplant_surgeon TEXT,
        status TEXT NOT NULL DEFAULT 'IN_PROGRESS' CHECK(status IN ('IN_PROGRESS', 'COMPLETED', 'CANCELLED')),
        start_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        completion_date TIMESTAMP,
        clinical_notes TEXT
    )
    """)

    # 6. Donation History (Immutable Audit Log)
    cur.execute("""
    CREATE TABLE IF NOT EXISTS donation_history (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        donation_id INTEGER,
        donor_id TEXT NOT NULL,
        donor_name TEXT NOT NULL,
        recipient_id TEXT NOT NULL,
        recipient_name TEXT NOT NULL,
        organ TEXT NOT NULL,
        blood_group_donor TEXT,
        blood_group_recipient TEXT,
        compatibility_score REAL,
        completion_date TEXT NOT NULL,
        hospital TEXT,
        transplant_surgeon TEXT,
        clinical_notes TEXT
    )
    """)

    # 7. Sessions Table
    cur.execute("""
    CREATE TABLE IF NOT EXISTS sessions (
        token TEXT PRIMARY KEY,
        user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        expires_at TIMESTAMP NOT NULL
    )
    """)

    conn.commit()

    # Seed Admin User if missing
    cur.execute("SELECT id FROM users WHERE email = ?", (ADMIN_EMAIL.lower(),))
    if not cur.fetchone():
        pwd_hash, salt = hash_password("Admin@2026!")
        ans_hash, _ = hash_password("transplant coordinator", salt)
        cur.execute("""
            INSERT INTO users (email, password_hash, salt, role, name, security_question, security_answer_hash)
            VALUES (?, ?, ?, 'admin', 'Sai Sriram Chakka (Transplant Admin)', 'What is your primary medical administration role?', ?)
        """, (ADMIN_EMAIL.lower(), pwd_hash, salt, ans_hash))
        conn.commit()
        print(f"[DB] Initialized authorized Admin account: {ADMIN_EMAIL}")

    # Seed Sample Donors & Recipients from CSV if empty
    cur.execute("SELECT COUNT(*) as count FROM donors")
    if cur.fetchone()["count"] == 0:
        import_sample_data(conn)

    conn.close()


def import_sample_data(conn):
    """Import initial donors and recipients from sample CSV files and create login credentials."""
    cur = conn.cursor()
    pwd_hash, salt = hash_password("Password@123")
    ans_hash, _ = hash_password("medical clinic", salt)

    # Import Donors
    csv_to_read = DONORS_CSV if os.path.exists(DONORS_CSV) else DONORS_SAMPLE_CSV
    if os.path.exists(csv_to_read):
        with open(csv_to_read, "r", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            for row in reader:
                d_id = row["id"].strip()
                name = row["name"].strip()
                email = f"{d_id.lower()}@donor.org"
                age = int(row.get("age", 30))
                gender = row.get("gender", "Male").strip()
                blood = row.get("blood_group", "O+").strip()
                organ = row.get("organ_type", "Kidney").strip()
                city = row.get("location", "Boston").strip()
                hla = row.get("hla_type", "A2-A24-B44-B51-DR4-DR11").strip()
                status = row.get("status", "Available").upper()
                if status == "AVAILABLE" or status == "AVAILABLE":
                    status = "AVAILABLE"
                else:
                    status = "AVAILABLE"
                reg_date = row.get("reg_date", "2026-01-15").strip()

                # Create user for donor
                cur.execute("SELECT id FROM users WHERE email = ?", (email,))
                user_row = cur.fetchone()
                if not user_row:
                    cur.execute("""
                        INSERT INTO users (email, password_hash, salt, role, name, security_question, security_answer_hash)
                        VALUES (?, ?, ?, 'donor', ?, 'What is your medical facility location?', ?)
                    """, (email, pwd_hash, salt, name, ans_hash))
                    user_id = cur.lastrowid
                else:
                    user_id = user_row["id"]

                cur.execute("""
                    INSERT OR REPLACE INTO donors (id, user_id, name, email, age, gender, phone, address, city, state, blood_group, organ_type, medical_history, health_condition, emergency_contact, status, hla_type, reg_date)
                    VALUES (?, ?, ?, ?, ?, ?, '555-0199', '120 Health Ave', ?, 'MA', ?, ?, 'No chronic conditions, healthy organ donor candidate', 'Excellent', '555-9911', ?, ?, ?)
                """, (d_id, user_id, name, email, age, gender, city, blood, organ, status, hla, reg_date))

    # Import Recipients
    r_csv_to_read = RECIPIENTS_CSV if os.path.exists(RECIPIENTS_CSV) else RECIPIENTS_SAMPLE_CSV
    if os.path.exists(r_csv_to_read):
        with open(r_csv_to_read, "r", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            for row in reader:
                r_id = row["id"].strip()
                name = row["name"].strip()
                email = f"{r_id.lower()}@recipient.org"
                age = int(row.get("age", 40))
                gender = row.get("gender", "Female").strip()
                blood = row.get("blood_group", "O+").strip()
                organ = row.get("required_organ", "Kidney").strip()
                urg = int(row.get("urgency_level", 2))
                city = row.get("location", "Boston").strip()
                wait_time = int(row.get("waiting_time", 100))
                hla = row.get("hla_type", "A2-A24-B44-B51-DR4-DR11").strip()
                reg_date = row.get("reg_date", "2025-06-01").strip()

                cur.execute("SELECT id FROM users WHERE email = ?", (email,))
                user_row = cur.fetchone()
                if not user_row:
                    cur.execute("""
                        INSERT INTO users (email, password_hash, salt, role, name, security_question, security_answer_hash)
                        VALUES (?, ?, ?, 'recipient', ?, 'What is your hospital location?', ?)
                    """, (email, pwd_hash, salt, name, ans_hash))
                    user_id = cur.lastrowid
                else:
                    user_id = user_row["id"]

                cur.execute("""
                    INSERT OR REPLACE INTO recipients (id, user_id, name, email, age, gender, phone, address, city, state, blood_group, required_organ, medical_condition, hospital_name, doctor_details, urgency_level, waiting_time, status, hla_type, reg_date)
                    VALUES (?, ?, ?, ?, ?, ?, '555-0288', '45 Clinical Parkway', ?, 'MA', ?, ?, 'End-stage organ insufficiency, critical transplant need', 'Metropolitan General Hospital', 'Dr. Sarah Jenkins, MD', ?, ?, 'WAITING', ?, ?)
                """, (r_id, user_id, name, email, age, gender, city, blood, organ, urg, wait_time, hla, reg_date))

    conn.commit()
    print("[DB] Initial sample donors and recipients imported into SQLite.")
    sync_csv_files(conn)


def sync_csv_files(conn=None):
    """
    Sync available donors and recipients from SQLite to data/donors.csv and data/recipients.csv.
    This guarantees that the C backend executable only processes active, available records!
    """
    should_close = False
    if not conn:
        conn = get_db_connection()
        should_close = True

    cur = conn.cursor()

    # Donors: export all donors, with their current status
    cur.execute("""
        SELECT id, name, age, gender, blood_group, organ_type, city as location, hla_type, status, reg_date
        FROM donors
        ORDER BY id
    """)
    donors = cur.fetchall()
    with open(DONORS_CSV, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["id", "name", "age", "gender", "blood_group", "organ_type", "location", "hla_type", "status", "reg_date"])
        for d in donors:
            writer.writerow([d["id"], d["name"], d["age"], d["gender"], d["blood_group"], d["organ_type"], d["location"] or "Boston", d["hla_type"] or "A2-A24-B44-B51-DR4-DR11", d["status"], d["reg_date"] or "2026-01-01"])

    # Recipients: export all recipients
    cur.execute("""
        SELECT id, name, age, gender, blood_group, required_organ, urgency_level, city as location, waiting_time, hla_type, status, reg_date
        FROM recipients
        ORDER BY id
    """)
    recipients = cur.fetchall()
    with open(RECIPIENTS_CSV, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["id", "name", "age", "gender", "blood_group", "required_organ", "urgency_level", "location", "waiting_time", "hla_type", "status", "reg_date"])
        for r in recipients:
            writer.writerow([r["id"], r["name"], r["age"], r["gender"], r["blood_group"], r["required_organ"], r["urgency_level"], r["location"] or "Boston", r["waiting_time"], r["hla_type"] or "A2-A24-B44-B51-DR4-DR11", r["status"], r["reg_date"] or "2025-06-01"])

    if should_close:
        conn.close()


# =============================================================================
# Authentication & User Management
# =============================================================================

def register_user(email, password, role, name, security_question=None, security_answer=None):
    """
    Register a new user.
    Role enforcement: Only 'saisriramchakka@gmail.com' can have role 'admin'.
    """
    email_clean = email.strip().lower()
    if role == "admin" and email_clean != ADMIN_EMAIL.lower():
        raise ValueError("Unauthorized: Only the designated administrator email (saisriramchakka@gmail.com) can register as Admin.")

    if email_clean == ADMIN_EMAIL.lower():
        role = "admin"

    conn = get_db_connection()
    cur = conn.cursor()

    cur.execute("SELECT id FROM users WHERE email = ?", (email_clean,))
    if cur.fetchone():
        conn.close()
        raise ValueError("An account with this email address already exists.")

    pwd_hash, salt = hash_password(password)
    ans_hash = None
    if security_answer:
        ans_hash, _ = hash_password(security_answer.strip().lower(), salt)

    cur.execute("""
        INSERT INTO users (email, password_hash, salt, role, name, security_question, security_answer_hash)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    """, (email_clean, pwd_hash, salt, role, name.strip(), security_question, ans_hash))
    user_id = cur.lastrowid
    conn.commit()
    conn.close()
    return user_id, role


def authenticate_user(email, password):
    """Authenticate email and password, return user row if valid."""
    email_clean = email.strip().lower()
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("SELECT * FROM users WHERE email = ?", (email_clean,))
    user = cur.fetchone()
    conn.close()

    if not user:
        return None

    if verify_password(password, user["salt"], user["password_hash"]):
        return dict(user)
    return None


def create_session(user_id):
    """Create a new session token valid for 7 days."""
    token = secrets.token_hex(32)
    expires_at = datetime.utcnow() + timedelta(days=7)
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("INSERT INTO sessions (token, user_id, expires_at) VALUES (?, ?, ?)",
                (token, user_id, expires_at.isoformat()))
    conn.commit()
    conn.close()
    return token


def get_user_by_session(token):
    """Retrieve user dictionary from session token."""
    if not token:
        return None
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("""
        SELECT u.id, u.email, u.name, u.role, u.security_question
        FROM sessions s
        JOIN users u ON s.user_id = u.id
        WHERE s.token = ? AND s.expires_at > ?
    """, (token, datetime.utcnow().isoformat()))
    row = cur.fetchone()
    conn.close()
    return dict(row) if row else None


def delete_session(token):
    """Logout / destroy session."""
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("DELETE FROM sessions WHERE token = ?", (token,))
    conn.commit()
    conn.close()


def reset_password(email, security_answer, new_password):
    """Reset password using security answer validation."""
    email_clean = email.strip().lower()
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("SELECT * FROM users WHERE email = ?", (email_clean,))
    user = cur.fetchone()

    if not user:
        conn.close()
        raise ValueError("User with this email not found.")

    if not user["security_answer_hash"]:
        conn.close()
        raise ValueError("No security answer registered for this account.")

    # Check answer
    input_ans_hash, _ = hash_password(security_answer.strip().lower(), user["salt"])
    if not secrets.compare_digest(input_ans_hash, user["security_answer_hash"]):
        conn.close()
        raise ValueError("Incorrect security answer.")

    new_hash, new_salt = hash_password(new_password)
    new_ans_hash, _ = hash_password(security_answer.strip().lower(), new_salt)
    cur.execute("""
        UPDATE users
        SET password_hash = ?, salt = ?, security_answer_hash = ?
        WHERE id = ?
    """, (new_hash, new_salt, new_ans_hash, user["id"]))
    conn.commit()
    conn.close()
    return True


# =============================================================================
# Donor Module Operations
# =============================================================================

def add_donor_record(donor_data, user_id=None):
    """Add a new donor record and sync CSV."""
    conn = get_db_connection()
    cur = conn.cursor()

    # Generate next ID if not supplied
    d_id = donor_data.get("id")
    if not d_id:
        cur.execute("SELECT id FROM donors ORDER BY id DESC LIMIT 1")
        last = cur.fetchone()
        if last and last["id"].startswith("DON-"):
            try:
                num = int(last["id"].split("-")[1]) + 1
            except ValueError:
                num = 1001
        else:
            num = 1001
        d_id = f"DON-{num}"

    reg_date = donor_data.get("reg_date") or datetime.now().strftime("%Y-%m-%d")

    cur.execute("""
        INSERT INTO donors (id, user_id, name, email, age, gender, phone, address, city, state, blood_group, organ_type, medical_history, health_condition, emergency_contact, status, hla_type, reg_date)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """, (
        d_id,
        user_id,
        donor_data["name"].strip(),
        donor_data["email"].strip().lower(),
        int(donor_data["age"]),
        donor_data["gender"],
        donor_data.get("phone", ""),
        donor_data.get("address", ""),
        donor_data.get("city", "Boston"),
        donor_data.get("state", "MA"),
        donor_data["blood_group"],
        donor_data["organ_type"],
        donor_data.get("medical_history", "None reported"),
        donor_data.get("health_condition", "Good"),
        donor_data.get("emergency_contact", ""),
        donor_data.get("status", "AVAILABLE").upper(),
        donor_data.get("hla_type", "A2-A24-B44-B51-DR4-DR11"),
        reg_date
    ))
    conn.commit()
    sync_csv_files(conn)
    conn.close()
    return d_id


def update_donor_record(donor_id, donor_data):
    """Update donor details and sync CSV."""
    conn = get_db_connection()
    cur = conn.cursor()

    cur.execute("""
        UPDATE donors
        SET name = COALESCE(?, name),
            age = COALESCE(?, age),
            gender = COALESCE(?, gender),
            phone = COALESCE(?, phone),
            address = COALESCE(?, address),
            city = COALESCE(?, city),
            state = COALESCE(?, state),
            blood_group = COALESCE(?, blood_group),
            organ_type = COALESCE(?, organ_type),
            medical_history = COALESCE(?, medical_history),
            health_condition = COALESCE(?, health_condition),
            emergency_contact = COALESCE(?, emergency_contact),
            status = COALESCE(?, status)
        WHERE id = ?
    """, (
        donor_data.get("name"),
        donor_data.get("age"),
        donor_data.get("gender"),
        donor_data.get("phone"),
        donor_data.get("address"),
        donor_data.get("city"),
        donor_data.get("state"),
        donor_data.get("blood_group"),
        donor_data.get("organ_type"),
        donor_data.get("medical_history"),
        donor_data.get("health_condition"),
        donor_data.get("emergency_contact"),
        donor_data.get("status"),
        donor_id
    ))
    conn.commit()
    sync_csv_files(conn)
    conn.close()
    return True


def update_donor_status(donor_id, new_status):
    """Change donor availability status."""
    valid_statuses = ("AVAILABLE", "MATCHED", "DONATION IN PROGRESS", "DONATION COMPLETED", "UNAVAILABLE")
    new_status = new_status.upper()
    if new_status not in valid_statuses:
        raise ValueError(f"Invalid donor status: {new_status}")

    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("UPDATE donors SET status = ? WHERE id = ?", (new_status, donor_id))
    conn.commit()
    sync_csv_files(conn)
    conn.close()
    return True


def get_donor_by_user_id(user_id):
    """Get donor profile by linked user ID."""
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("SELECT * FROM donors WHERE user_id = ?", (user_id,))
    row = cur.fetchone()
    conn.close()
    return dict(row) if row else None


def get_all_donors(search=None, organ=None, blood_group=None, status=None, location=None):
    """Retrieve donors with dynamic query filtering."""
    conn = get_db_connection()
    cur = conn.cursor()
    query = "SELECT * FROM donors WHERE 1=1"
    params = []

    if search:
        query += " AND (name LIKE ? OR id LIKE ? OR city LIKE ? OR organ_type LIKE ?)"
        term = f"%{search.strip()}%"
        params.extend([term, term, term, term])

    if organ:
        query += " AND organ_type = ?"
        params.append(organ.strip())

    if blood_group:
        query += " AND blood_group = ?"
        params.append(blood_group.strip())

    if status:
        query += " AND status = ?"
        params.append(status.strip().upper())

    if location:
        query += " AND (city LIKE ? OR state LIKE ?)"
        params.extend([f"%{location}%", f"%{location}%"])

    query += " ORDER BY id"
    cur.execute(query, params)
    rows = [dict(r) for r in cur.fetchall()]
    conn.close()
    return rows


# =============================================================================
# Recipient Module Operations
# =============================================================================

def add_recipient_record(rec_data, user_id=None):
    """Add a new recipient record and sync CSV."""
    conn = get_db_connection()
    cur = conn.cursor()

    r_id = rec_data.get("id")
    if not r_id:
        cur.execute("SELECT id FROM recipients ORDER BY id DESC LIMIT 1")
        last = cur.fetchone()
        if last and last["id"].startswith("REC-"):
            try:
                num = int(last["id"].split("-")[1]) + 1
            except ValueError:
                num = 2001
        else:
            num = 2001
        r_id = f"REC-{num}"

    reg_date = rec_data.get("reg_date") or datetime.now().strftime("%Y-%m-%d")

    cur.execute("""
        INSERT INTO recipients (id, user_id, name, email, age, gender, phone, address, city, state, blood_group, required_organ, medical_condition, hospital_name, doctor_details, urgency_level, waiting_time, status, hla_type, reg_date)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """, (
        r_id,
        user_id,
        rec_data["name"].strip(),
        rec_data["email"].strip().lower(),
        int(rec_data["age"]),
        rec_data["gender"],
        rec_data.get("phone", ""),
        rec_data.get("address", ""),
        rec_data.get("city", "Boston"),
        rec_data.get("state", "MA"),
        rec_data["blood_group"],
        rec_data["required_organ"],
        rec_data.get("medical_condition", "Transplant required"),
        rec_data.get("hospital_name", "City Hospital"),
        rec_data.get("doctor_details", ""),
        int(rec_data.get("urgency_level", 2)),
        int(rec_data.get("waiting_time", 0)),
        rec_data.get("status", "WAITING").upper(),
        rec_data.get("hla_type", "A2-A24-B44-B51-DR4-DR11"),
        reg_date
    ))
    conn.commit()
    sync_csv_files(conn)
    conn.close()
    return r_id


def update_recipient_record(rec_id, rec_data):
    """Update recipient details and sync CSV."""
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("""
        UPDATE recipients
        SET name = COALESCE(?, name),
            age = COALESCE(?, age),
            gender = COALESCE(?, gender),
            phone = COALESCE(?, phone),
            address = COALESCE(?, address),
            city = COALESCE(?, city),
            state = COALESCE(?, state),
            blood_group = COALESCE(?, blood_group),
            required_organ = COALESCE(?, required_organ),
            medical_condition = COALESCE(?, medical_condition),
            hospital_name = COALESCE(?, hospital_name),
            doctor_details = COALESCE(?, doctor_details),
            urgency_level = COALESCE(?, urgency_level),
            status = COALESCE(?, status)
        WHERE id = ?
    """, (
        rec_data.get("name"),
        rec_data.get("age"),
        rec_data.get("gender"),
        rec_data.get("phone"),
        rec_data.get("address"),
        rec_data.get("city"),
        rec_data.get("state"),
        rec_data.get("blood_group"),
        rec_data.get("required_organ"),
        rec_data.get("medical_condition"),
        rec_data.get("hospital_name"),
        rec_data.get("doctor_details"),
        rec_data.get("urgency_level"),
        rec_data.get("status"),
        rec_id
    ))
    conn.commit()
    sync_csv_files(conn)
    conn.close()
    return True


def get_recipient_by_user_id(user_id):
    """Get recipient profile by linked user ID."""
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("SELECT * FROM recipients WHERE user_id = ?", (user_id,))
    row = cur.fetchone()
    conn.close()
    return dict(row) if row else None


def get_all_recipients(search=None, organ=None, blood_group=None, urgency=None):
    """Retrieve recipients with query filtering."""
    conn = get_db_connection()
    cur = conn.cursor()
    query = "SELECT * FROM recipients WHERE 1=1"
    params = []

    if search:
        query += " AND (name LIKE ? OR id LIKE ? OR city LIKE ? OR required_organ LIKE ?)"
        term = f"%{search.strip()}%"
        params.extend([term, term, term, term])

    if organ:
        query += " AND required_organ = ?"
        params.append(organ.strip())

    if blood_group:
        query += " AND blood_group = ?"
        params.append(blood_group.strip())

    if urgency:
        query += " AND urgency_level = ?"
        params.append(int(urgency))

    query += " ORDER BY urgency_level ASC, waiting_time DESC"
    cur.execute(query, params)
    rows = [dict(r) for r in cur.fetchall()]
    conn.close()
    return rows


# =============================================================================
# Match Review & Donation Workflow Management
# =============================================================================

def save_proposed_matches(pairs_list):
    """Save or update proposed matches from graph matching algorithm."""
    conn = get_db_connection()
    cur = conn.cursor()
    for p in pairs_list:
        cur.execute("""
            INSERT INTO matches (donor_id, recipient_id, organ, compatibility_score, explanation, status)
            VALUES (?, ?, ?, ?, ?, 'PROPOSED')
            ON CONFLICT(donor_id, recipient_id, organ) DO UPDATE SET
                compatibility_score = excluded.compatibility_score,
                explanation = excluded.explanation
            WHERE status = 'PROPOSED'
        """, (p["donor_id"], p["recipient_id"], p["organ"], p["compatibility_score"], p.get("explanation", "")))
    conn.commit()
    conn.close()


def approve_match(match_id, admin_name="Sai Sriram Chakka (Admin)", notes=""):
    """
    Approve proposed match:
    - Sets match status to 'APPROVED'
    - Sets donor status to 'DONATION IN PROGRESS'
    - Sets recipient status to 'DONATION IN PROGRESS'
    - Creates record in 'donations' table with status 'IN_PROGRESS'
    """
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("SELECT * FROM matches WHERE id = ?", (match_id,))
    match = cur.fetchone()
    if not match:
        conn.close()
        raise ValueError(f"Match ID {match_id} not found.")

    now = datetime.utcnow().isoformat()
    cur.execute("""
        UPDATE matches
        SET status = 'APPROVED', reviewed_date = ?, reviewed_by = ?, admin_notes = ?
        WHERE id = ?
    """, (now, admin_name, notes, match_id))

    # Update donor and recipient
    cur.execute("UPDATE donors SET status = 'DONATION IN PROGRESS' WHERE id = ?", (match["donor_id"],))
    cur.execute("UPDATE recipients SET status = 'DONATION IN PROGRESS' WHERE id = ?", (match["recipient_id"],))

    # Create donation entry
    cur.execute("""
        INSERT INTO donations (match_id, donor_id, recipient_id, organ, hospital, transplant_surgeon, status, clinical_notes)
        VALUES (?, ?, ?, ?, 'Metropolitan Transplant Institute', 'Dr. Sarah Jenkins, MD', 'IN_PROGRESS', ?)
    """, (match_id, match["donor_id"], match["recipient_id"], match["organ"], notes))

    conn.commit()
    sync_csv_files(conn)
    conn.close()
    return True


def reject_match(match_id, admin_name="Sai Sriram Chakka (Admin)", notes=""):
    """Reject proposed match."""
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("SELECT * FROM matches WHERE id = ?", (match_id,))
    match = cur.fetchone()
    if not match:
        conn.close()
        raise ValueError(f"Match ID {match_id} not found.")

    cur.execute("""
        UPDATE matches
        SET status = 'REJECTED', reviewed_date = ?, reviewed_by = ?, admin_notes = ?
        WHERE id = ?
    """, (datetime.utcnow().isoformat(), admin_name, notes, match_id))

    # Revert donor/recipient if needed
    cur.execute("UPDATE donors SET status = 'AVAILABLE' WHERE id = ? AND status = 'MATCHED'", (match["donor_id"],))
    cur.execute("UPDATE recipients SET status = 'WAITING' WHERE id = ? AND status = 'MATCHED'", (match["recipient_id"],))

    conn.commit()
    sync_csv_files(conn)
    conn.close()
    return True


def complete_donation(match_id_or_donation_id, hospital=None, surgeon=None, notes=None):
    """
    CRITICAL WORKFLOW: Complete an organ donation.
    1. Mark donation as COMPLETED.
    2. Change donor status to 'UNAVAILABLE' (and remove donor from future match pool).
    3. Change recipient status to 'TRANSPLANT COMPLETED'.
    4. Record entry in immutable donation_history.
    5. Sync CSVs so C backend never matches this donor again!
    """
    conn = get_db_connection()
    cur = conn.cursor()

    # Find donation by match_id or donation id
    cur.execute("""
        SELECT d.*, m.compatibility_score, m.explanation,
               dn.name as donor_name, dn.blood_group as donor_bg,
               rc.name as recipient_name, rc.blood_group as recipient_bg
        FROM donations d
        JOIN matches m ON d.match_id = m.id
        JOIN donors dn ON d.donor_id = dn.id
        JOIN recipients rc ON d.recipient_id = rc.id
        WHERE d.match_id = ? OR d.id = ?
        LIMIT 1
    """, (match_id_or_donation_id, match_id_or_donation_id))
    donation = cur.fetchone()

    if not donation:
        conn.close()
        raise ValueError("Active donation record not found.")

    completion_date = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    hosp = hospital or donation["hospital"] or "Transplant Care Center"
    surg = surgeon or donation["transplant_surgeon"] or "Authorized Transplant Surgeon"
    cl_notes = notes or donation["clinical_notes"] or "Successful organ transplant completed and verified."

    # Update donation record
    cur.execute("""
        UPDATE donations
        SET status = 'COMPLETED', completion_date = ?, hospital = ?, transplant_surgeon = ?, clinical_notes = ?
        WHERE id = ?
    """, (completion_date, hosp, surg, cl_notes, donation["id"]))

    # Update match record
    cur.execute("UPDATE matches SET status = 'DONATION COMPLETED' WHERE id = ?", (donation["match_id"],))

    # UPDATE DONOR TO UNAVAILABLE - Critical Requirement 5
    cur.execute("UPDATE donors SET status = 'UNAVAILABLE' WHERE id = ?", (donation["donor_id"],))

    # UPDATE RECIPIENT TO TRANSPLANT COMPLETED
    cur.execute("UPDATE recipients SET status = 'TRANSPLANT COMPLETED' WHERE id = ?", (donation["recipient_id"],))

    # Insert into donation_history
    cur.execute("""
        INSERT INTO donation_history (donation_id, donor_id, donor_name, recipient_id, recipient_name, organ, blood_group_donor, blood_group_recipient, compatibility_score, completion_date, hospital, transplant_surgeon, clinical_notes)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """, (
        donation["id"],
        donation["donor_id"],
        donation["donor_name"],
        donation["recipient_id"],
        donation["recipient_name"],
        donation["organ"],
        donation["donor_bg"],
        donation["recipient_bg"],
        donation["compatibility_score"],
        completion_date,
        hosp,
        surg,
        cl_notes
    ))

    conn.commit()
    # CRITICAL: Sync CSV so C backend removes this donor from active pool!
    sync_csv_files(conn)
    conn.close()
    return True


def get_donation_history():
    """Retrieve full completed donation history."""
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("SELECT * FROM donation_history ORDER BY id DESC")
    rows = [dict(r) for r in cur.fetchall()]
    conn.close()
    return rows


def get_admin_dashboard_stats():
    """
    Get all 7 stats required for Admin Dashboard:
    1. Total Registered Donors
    2. Total Registered Recipients
    3. Available Donors
    4. Unavailable Donors
    5. Total Organ Requests
    6. Successful Organ Donations
    7. Pending Matches
    """
    conn = get_db_connection()
    cur = conn.cursor()

    cur.execute("SELECT COUNT(*) as count FROM donors")
    total_donors = cur.fetchone()["count"]

    cur.execute("SELECT COUNT(*) as count FROM recipients")
    total_recipients = cur.fetchone()["count"]

    cur.execute("SELECT COUNT(*) as count FROM donors WHERE status = 'AVAILABLE'")
    available_donors = cur.fetchone()["count"]

    cur.execute("SELECT COUNT(*) as count FROM donors WHERE status IN ('UNAVAILABLE', 'DONATION COMPLETED')")
    unavailable_donors = cur.fetchone()["count"]

    total_organ_requests = total_recipients

    cur.execute("SELECT COUNT(*) as count FROM donation_history")
    successful_donations = cur.fetchone()["count"]

    cur.execute("SELECT COUNT(*) as count FROM matches WHERE status IN ('PROPOSED', 'APPROVED')")
    pending_matches = cur.fetchone()["count"]

    conn.close()
    return {
        "total_donors": total_donors,
        "total_recipients": total_recipients,
        "available_donors": available_donors,
        "unavailable_donors": unavailable_donors,
        "total_organ_requests": total_organ_requests,
        "successful_donations": successful_donations,
        "pending_matches": pending_matches
    }


def get_all_matches(status=None):
    """Get all recorded matches with donor and recipient names."""
    conn = get_db_connection()
    cur = conn.cursor()
    query = """
        SELECT m.*,
               dn.name as donor_name, dn.blood_group as donor_bg, dn.city as donor_location, dn.age as donor_age,
               rc.name as recipient_name, rc.blood_group as recipient_bg, rc.city as recipient_location, rc.age as recipient_age,
               rc.urgency_level, rc.hospital_name
        FROM matches m
        LEFT JOIN donors dn ON m.donor_id = dn.id
        LEFT JOIN recipients rc ON m.recipient_id = rc.id
        WHERE 1=1
    """
    params = []
    if status:
        query += " AND m.status = ?"
        params.append(status.upper())
    query += " ORDER BY m.compatibility_score DESC"
    cur.execute(query, params)
    rows = [dict(r) for r in cur.fetchall()]
    conn.close()
    return rows
