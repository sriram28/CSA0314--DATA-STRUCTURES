#!/usr/bin/env python3
"""
server.py - Enhanced HTTP Server & REST API Gateway for Smart Organ Donation System
Connects HTML/CSS/JS frontend to SQLite database (server/db.py) and
high-performance compiled C backend executable (organ_matching_system.exe).
Supports Role-Based Authentication (Admin, Donor, Recipient), Donation Workflow Lifecycle,
and Advanced Graph Matching Algorithms.
"""

import http.server
import json
import os
import socket
import subprocess
import sys
import threading
import time
import urllib.parse
import webbrowser

# Add current directory to path for db module
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
if CURRENT_DIR not in sys.path:
    sys.path.insert(0, CURRENT_DIR)

import db

PORT = 5000
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WEB_DIR = os.path.join(BASE_DIR, "web")
C_EXE = os.path.join(BASE_DIR, "organ_matching_system.exe")


class OrganMatchingHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    def _send_json_response(self, status_code, data_obj_or_str):
        self.send_response(status_code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Session-Token")
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        self.end_headers()

        if isinstance(data_obj_or_str, (dict, list)):
            data_str = json.dumps(data_obj_or_str, default=str)
        else:
            data_str = str(data_obj_or_str)

        self.wfile.write(data_str.encode("utf-8"))

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Session-Token")
        self.end_headers()

    def get_auth_user(self):
        """Extract user dictionary from session token in headers."""
        auth_header = self.headers.get("Authorization", "")
        token = None
        if auth_header.startswith("Bearer "):
            token = auth_header[7:].strip()
        elif "X-Session-Token" in self.headers:
            token = self.headers.get("X-Session-Token", "").strip()

        if not token:
            return None
        return db.get_user_by_session(token)

    def run_c_backend(self, api_args):
        """Execute the C backend executable and return its stdout output"""
        cmd = [C_EXE, "--api"] + api_args
        try:
            result = subprocess.run(
                cmd,
                cwd=BASE_DIR,
                capture_output=True,
                text=True,
                encoding="utf-8",
                timeout=30
            )
            if result.returncode == 0:
                output = result.stdout.strip()
                if not output:
                    return json.dumps({"status": "success"})
                return output
            else:
                return json.dumps({
                    "status": "error",
                    "message": f"C Backend error: {result.stderr.strip() or 'Process exited with non-zero code'}"
                })
        except Exception as e:
            return json.dumps({"status": "error", "message": f"Execution failed: {str(e)}"})

    # =========================================================================
    # GET Handlers
    # =========================================================================
    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        query = urllib.parse.parse_qs(parsed.query)

        # 1. Auth check: /api/auth/me
        if path == "/api/auth/me":
            user = self.get_auth_user()
            if not user:
                self._send_json_response(401, {"status": "error", "message": "Not authenticated"})
                return

            profile = None
            if user["role"] == "donor":
                profile = db.get_donor_by_user_id(user["id"])
            elif user["role"] == "recipient":
                profile = db.get_recipient_by_user_id(user["id"])

            self._send_json_response(200, {
                "status": "success",
                "user": user,
                "profile": profile
            })
            return

        # 2. Donor Personal Dashboard: /api/donor/profile
        if path == "/api/donor/profile":
            user = self.get_auth_user()
            if not user or user["role"] != "donor":
                self._send_json_response(403, {"status": "error", "message": "Unauthorized donor access"})
                return
            profile = db.get_donor_by_user_id(user["id"])
            self._send_json_response(200, {"status": "success", "profile": profile})
            return

        # 3. Donor Matches / Status: /api/donor/matches
        if path == "/api/donor/matches":
            user = self.get_auth_user()
            if not user or user["role"] != "donor":
                self._send_json_response(403, {"status": "error", "message": "Unauthorized donor access"})
                return
            donor = db.get_donor_by_user_id(user["id"])
            if not donor:
                self._send_json_response(200, {"status": "success", "matches": []})
                return

            all_matches = db.get_all_matches()
            donor_matches = [m for m in all_matches if m["donor_id"] == donor["id"]]
            self._send_json_response(200, {"status": "success", "donor": donor, "matches": donor_matches})
            return

        # 4. Recipient Personal Dashboard: /api/recipient/profile
        if path == "/api/recipient/profile":
            user = self.get_auth_user()
            if not user or user["role"] != "recipient":
                self._send_json_response(403, {"status": "error", "message": "Unauthorized recipient access"})
                return
            profile = db.get_recipient_by_user_id(user["id"])
            self._send_json_response(200, {"status": "success", "profile": profile})
            return

        # 5. Recipient Match Status: /api/recipient/match-status
        if path == "/api/recipient/match-status":
            user = self.get_auth_user()
            if not user or user["role"] != "recipient":
                self._send_json_response(403, {"status": "error", "message": "Unauthorized recipient access"})
                return
            recipient = db.get_recipient_by_user_id(user["id"])
            if not recipient:
                self._send_json_response(200, {"status": "success", "has_match": False})
                return

            all_matches = db.get_all_matches()
            rec_matches = [m for m in all_matches if m["recipient_id"] == recipient["id"]]
            self._send_json_response(200, {
                "status": "success",
                "recipient": recipient,
                "has_match": len(rec_matches) > 0,
                "matches": rec_matches
            })
            return

        # 6. Admin Stats: /api/admin/stats
        if path == "/api/admin/stats":
            stats = db.get_admin_dashboard_stats()
            self._send_json_response(200, {"status": "success", "stats": stats})
            return

        # 7. Admin Donors: /api/admin/donors
        if path == "/api/admin/donors":
            search = query.get("search", [None])[0]
            organ = query.get("organ", [None])[0]
            blood = query.get("blood_group", [None])[0]
            status = query.get("status", [None])[0]
            location = query.get("location", [None])[0]
            donors = db.get_all_donors(search=search, organ=organ, blood_group=blood, status=status, location=location)
            self._send_json_response(200, {"status": "success", "donors": donors})
            return

        # 8. Admin Recipients: /api/admin/recipients
        if path == "/api/admin/recipients":
            search = query.get("search", [None])[0]
            organ = query.get("organ", [None])[0]
            blood = query.get("blood_group", [None])[0]
            urgency = query.get("urgency", [None])[0]
            recipients = db.get_all_recipients(search=search, organ=organ, blood_group=blood, urgency=urgency)
            self._send_json_response(200, {"status": "success", "recipients": recipients})
            return

        # 9. Admin Matches List: /api/admin/matches
        if path == "/api/admin/matches":
            status = query.get("status", [None])[0]
            matches = db.get_all_matches(status=status)
            self._send_json_response(200, {"status": "success", "matches": matches})
            return

        # 10. Admin Completed Donation History: /api/admin/donations/history
        if path == "/api/admin/donations/history":
            history = db.get_donation_history()
            self._send_json_response(200, {"status": "success", "history": history})
            return

        # 11. Core C Algorithm Routes (Preserved 100%)
        if path == "/api/stats":
            output = self.run_c_backend(["get_stats"])
            self._send_json_response(200, output)
            return

        if path == "/api/donors":
            # Return active donors from DB
            donors = db.get_all_donors()
            self._send_json_response(200, donors)
            return

        if path == "/api/recipients":
            # Return active recipients from DB
            recipients = db.get_all_recipients()
            self._send_json_response(200, recipients)
            return

        if path == "/api/graph/data":
            output = self.run_c_backend(["get_graph_data"])
            self._send_json_response(200, output)
            return

        if path == "/api/graph/match":
            output = self.run_c_backend(["run_graph_matching"])
            # Auto-sync proposed matches to database
            try:
                data = json.loads(output)
                if "pairs" in data:
                    db.save_proposed_matches(data["pairs"])
            except Exception:
                pass
            self._send_json_response(200, output)
            return

        if path == "/api/graph/bfs":
            start_id = query.get("start", ["DON-1001"])[0]
            output = self.run_c_backend(["run_bfs", start_id])
            self._send_json_response(200, output)
            return

        if path == "/api/graph/dfs":
            start_id = query.get("start", ["DON-1001"])[0]
            output = self.run_c_backend(["run_dfs", start_id])
            self._send_json_response(200, output)
            return

        if path == "/api/traditional/match":
            output = self.run_c_backend(["run_traditional_matching"])
            self._send_json_response(200, output)
            return

        if path == "/api/compare":
            output = self.run_c_backend(["compare_performance"])
            self._send_json_response(200, output)
            return

        if path == "/api/benchmark":
            size = query.get("size", ["50"])[0]
            output = self.run_c_backend(["benchmark", size])
            self._send_json_response(200, output)
            return

        # Serve static web files
        if path == "/":
            self.path = "/index.html"
        return super().do_GET()

    # =========================================================================
    # POST Handlers
    # =========================================================================
    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path

        content_length = int(self.headers.get("Content-Length", 0))
        body_str = self.rfile.read(content_length).decode("utf-8") if content_length > 0 else "{}"
        try:
            body = json.loads(body_str) if body_str else {}
        except Exception:
            body = {}

        # 1. Login: /api/auth/login
        if path == "/api/auth/login":
            email = body.get("email", "").strip().lower()
            password = body.get("password", "")
            requested_role = body.get("role", "").lower()

            if not email or not password:
                self._send_json_response(400, {"status": "error", "message": "Email and password are required"})
                return

            user = db.authenticate_user(email, password)
            if not user:
                self._send_json_response(401, {"status": "error", "message": "Invalid email or password"})
                return

            # Check role matching
            if requested_role and user["role"] != requested_role:
                self._send_json_response(403, {
                    "status": "error",
                    "message": f"Account role mismatch. This account is registered as a '{user['role']}', not '{requested_role}'."
                })
                return

            # Strict Admin security
            if user["role"] == "admin" and user["email"].lower() != db.ADMIN_EMAIL.lower():
                self._send_json_response(403, {"status": "error", "message": "Unauthorized administrator access"})
                return

            token = db.create_session(user["id"])
            profile = None
            if user["role"] == "donor":
                profile = db.get_donor_by_user_id(user["id"])
            elif user["role"] == "recipient":
                profile = db.get_recipient_by_user_id(user["id"])

            self._send_json_response(200, {
                "status": "success",
                "message": f"Welcome back, {user['name']}!",
                "token": token,
                "user": {
                    "id": user["id"],
                    "email": user["email"],
                    "name": user["name"],
                    "role": user["role"]
                },
                "profile": profile
            })
            return

        # 2. Registration: /api/auth/register
        if path == "/api/auth/register":
            role = body.get("role", "donor").lower()
            name = body.get("full_name") or body.get("name")
            email = body.get("email", "").strip().lower()
            password = body.get("password", "")
            confirm_pwd = body.get("confirm_password", "")
            sec_q = body.get("security_question", "What is your primary medical facility?")
            sec_ans = body.get("security_answer", "General Hospital")

            if not name or not email or not password:
                self._send_json_response(400, {"status": "error", "message": "Name, email, and password are required."})
                return

            if password != confirm_pwd:
                self._send_json_response(400, {"status": "error", "message": "Passwords do not match."})
                return

            if len(password) < 6:
                self._send_json_response(400, {"status": "error", "message": "Password must be at least 6 characters."})
                return

            try:
                user_id, actual_role = db.register_user(
                    email=email,
                    password=password,
                    role=role,
                    name=name,
                    security_question=sec_q,
                    security_answer=sec_ans
                )

                profile_id = None
                if actual_role == "donor":
                    donor_data = {
                        "name": name,
                        "email": email,
                        "age": body.get("age", 30),
                        "gender": body.get("gender", "Male"),
                        "phone": body.get("phone", ""),
                        "address": body.get("address", ""),
                        "city": body.get("city", "Boston"),
                        "state": body.get("state", "MA"),
                        "blood_group": body.get("blood_group", "O+"),
                        "organ_type": body.get("organ_willing_to_donate") or body.get("organ_type", "Kidney"),
                        "medical_history": body.get("medical_history", "Healthy donor candidate"),
                        "health_condition": body.get("health_condition", "Good"),
                        "emergency_contact": body.get("emergency_contact", ""),
                        "status": body.get("availability_status", "AVAILABLE").upper()
                    }
                    profile_id = db.add_donor_record(donor_data, user_id=user_id)

                elif actual_role == "recipient":
                    rec_data = {
                        "name": name,
                        "email": email,
                        "age": body.get("age", 40),
                        "gender": body.get("gender", "Female"),
                        "phone": body.get("phone", ""),
                        "address": body.get("address", ""),
                        "city": body.get("city", "Boston"),
                        "state": body.get("state", "MA"),
                        "blood_group": body.get("blood_group", "O+"),
                        "required_organ": body.get("required_organ", "Kidney"),
                        "medical_condition": body.get("medical_condition", "End-stage organ condition"),
                        "hospital_name": body.get("hospital_name", "Metropolitan General Hospital"),
                        "doctor_details": body.get("doctor_details", "Dr. Jenkins"),
                        "urgency_level": int(body.get("urgency_level", 2)),
                        "waiting_time": int(body.get("waiting_time", 0)),
                        "status": "WAITING"
                    }
                    profile_id = db.add_recipient_record(rec_data, user_id=user_id)

                token = db.create_session(user_id)
                self._send_json_response(200, {
                    "status": "success",
                    "message": f"Registration successful! Your {actual_role.capitalize()} profile #{profile_id} is active.",
                    "token": token,
                    "user": {"id": user_id, "email": email, "name": name, "role": actual_role},
                    "profile_id": profile_id
                })
            except Exception as e:
                self._send_json_response(400, {"status": "error", "message": str(e)})
            return

        # 3. Logout: /api/auth/logout
        if path == "/api/auth/logout":
            auth_header = self.headers.get("Authorization", "")
            if auth_header.startswith("Bearer "):
                token = auth_header[7:].strip()
                db.delete_session(token)
            self._send_json_response(200, {"status": "success", "message": "Logged out successfully"})
            return

        # 4. Forgot Password - Retrieve Question: /api/auth/forgot-password/question
        if path == "/api/auth/forgot-password/question":
            email = body.get("email", "").strip().lower()
            conn = db.get_db_connection()
            cur = conn.cursor()
            cur.execute("SELECT security_question FROM users WHERE email = ?", (email,))
            u = cur.fetchone()
            conn.close()
            if u and u["security_question"]:
                self._send_json_response(200, {"status": "success", "security_question": u["security_question"]})
            else:
                self._send_json_response(404, {"status": "error", "message": "Account not found or no security question set"})
            return

        # 5. Forgot Password - Reset: /api/auth/forgot-password/reset
        if path == "/api/auth/forgot-password/reset":
            email = body.get("email", "").strip().lower()
            answer = body.get("security_answer", "")
            new_pwd = body.get("new_password", "")
            if not email or not answer or not new_pwd:
                self._send_json_response(400, {"status": "error", "message": "All fields are required"})
                return
            try:
                db.reset_password(email, answer, new_pwd)
                self._send_json_response(200, {"status": "success", "message": "Password reset successfully. Please log in."})
            except Exception as e:
                self._send_json_response(400, {"status": "error", "message": str(e)})
            return

        # 6. Admin Match Sync from C backend: /api/admin/matches/sync
        if path == "/api/admin/matches/sync":
            user = self.get_auth_user()
            if not user or user["role"] != "admin":
                self._send_json_response(403, {"status": "error", "message": "Admin authorization required"})
                return

            output = self.run_c_backend(["run_graph_matching"])
            try:
                res = json.loads(output)
                if "pairs" in res:
                    db.save_proposed_matches(res["pairs"])
                self._send_json_response(200, {"status": "success", "synced_pairs": len(res.get("pairs", []))})
            except Exception as e:
                self._send_json_response(400, {"status": "error", "message": f"Sync failed: {str(e)}"})
            return

        # 7. Admin Approve Match: /api/admin/matches/<id>/approve
        if path.startswith("/api/admin/matches/") and path.endswith("/approve"):
            user = self.get_auth_user()
            if not user or user["role"] != "admin":
                self._send_json_response(403, {"status": "error", "message": "Admin authorization required"})
                return

            parts = path.split("/")
            match_id = int(parts[4])
            notes = body.get("notes", "Approved by Transplant Board")
            try:
                db.approve_match(match_id, admin_name=user["name"], notes=notes)
                self._send_json_response(200, {"status": "success", "message": f"Match #{match_id} approved. Donation initiated."})
            except Exception as e:
                self._send_json_response(400, {"status": "error", "message": str(e)})
            return

        # 8. Admin Reject Match: /api/admin/matches/<id>/reject
        if path.startswith("/api/admin/matches/") and path.endswith("/reject"):
            user = self.get_auth_user()
            if not user or user["role"] != "admin":
                self._send_json_response(403, {"status": "error", "message": "Admin authorization required"})
                return

            parts = path.split("/")
            match_id = int(parts[4])
            notes = body.get("notes", "Rejected by Transplant Board")
            try:
                db.reject_match(match_id, admin_name=user["name"], notes=notes)
                self._send_json_response(200, {"status": "success", "message": f"Match #{match_id} rejected."})
            except Exception as e:
                self._send_json_response(400, {"status": "error", "message": str(e)})
            return

        # 9. Admin Complete Donation: /api/admin/donations/complete
        if path == "/api/admin/donations/complete":
            user = self.get_auth_user()
            if not user or user["role"] != "admin":
                self._send_json_response(403, {"status": "error", "message": "Admin authorization required"})
                return

            match_id = body.get("match_id")
            hospital = body.get("hospital", "Metropolitan Hospital Center")
            surgeon = body.get("surgeon", "Chief Transplant Surgeon")
            notes = body.get("notes", "Organ successfully harvested and transplanted.")

            try:
                db.complete_donation(match_id, hospital=hospital, surgeon=surgeon, notes=notes)
                self._send_json_response(200, {
                    "status": "success",
                    "message": "Donation marked COMPLETED. Donor status updated to UNAVAILABLE and archived to donation history."
                })
            except Exception as e:
                self._send_json_response(400, {"status": "error", "message": str(e)})
            return

        # 10. Legacy / Core routes
        if path == "/api/donors":
            try:
                d_id = db.add_donor_record(body)
                self._send_json_response(200, {"status": "success", "message": "Donor registered", "id": d_id})
            except Exception as e:
                self._send_json_response(400, {"status": "error", "message": str(e)})
            return

        if path == "/api/recipients":
            try:
                r_id = db.add_recipient_record(body)
                self._send_json_response(200, {"status": "success", "message": "Recipient registered", "id": r_id})
            except Exception as e:
                self._send_json_response(400, {"status": "error", "message": str(e)})
            return

        if path == "/api/reset":
            # Reset C data and re-import sample datasets
            self.run_c_backend(["reset_data"])
            conn = db.get_db_connection()
            db.import_sample_data(conn)
            conn.close()
            self._send_json_response(200, {"status": "success", "message": "Sample datasets successfully restored"})
            return

        self._send_json_response(404, {"status": "error", "message": "Route not found"})

    # =========================================================================
    # PUT Handlers
    # =========================================================================
    def do_PUT(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path

        content_length = int(self.headers.get("Content-Length", 0))
        body_str = self.rfile.read(content_length).decode("utf-8") if content_length > 0 else "{}"
        try:
            body = json.loads(body_str) if body_str else {}
        except Exception:
            body = {}

        # 1. Donor Profile Update: /api/donor/profile
        if path == "/api/donor/profile":
            user = self.get_auth_user()
            if not user or user["role"] != "donor":
                self._send_json_response(403, {"status": "error", "message": "Unauthorized donor access"})
                return

            donor = db.get_donor_by_user_id(user["id"])
            if not donor:
                self._send_json_response(404, {"status": "error", "message": "Donor profile not found"})
                return

            db.update_donor_record(donor["id"], body)
            updated = db.get_donor_by_user_id(user["id"])
            self._send_json_response(200, {"status": "success", "message": "Profile updated successfully", "profile": updated})
            return

        # 2. Donor Availability Status Toggle: /api/donor/availability
        if path == "/api/donor/availability":
            user = self.get_auth_user()
            if not user or user["role"] != "donor":
                self._send_json_response(403, {"status": "error", "message": "Unauthorized donor access"})
                return

            donor = db.get_donor_by_user_id(user["id"])
            if not donor:
                self._send_json_response(404, {"status": "error", "message": "Donor profile not found"})
                return

            new_status = body.get("status", "AVAILABLE").upper()
            db.update_donor_status(donor["id"], new_status)
            self._send_json_response(200, {"status": "success", "message": f"Availability changed to {new_status}"})
            return

        # 3. Recipient Profile Update: /api/recipient/profile
        if path == "/api/recipient/profile":
            user = self.get_auth_user()
            if not user or user["role"] != "recipient":
                self._send_json_response(403, {"status": "error", "message": "Unauthorized recipient access"})
                return

            recipient = db.get_recipient_by_user_id(user["id"])
            if not recipient:
                self._send_json_response(404, {"status": "error", "message": "Recipient profile not found"})
                return

            db.update_recipient_record(recipient["id"], body)
            updated = db.get_recipient_by_user_id(user["id"])
            self._send_json_response(200, {"status": "success", "message": "Profile updated successfully", "profile": updated})
            return

        # 4. Admin Update Donor Status: /api/admin/donors/<id>/status
        if path.startswith("/api/admin/donors/") and path.endswith("/status"):
            user = self.get_auth_user()
            if not user or user["role"] != "admin":
                self._send_json_response(403, {"status": "error", "message": "Admin authorization required"})
                return

            parts = path.split("/")
            donor_id = parts[4]
            new_status = body.get("status", "AVAILABLE").upper()
            try:
                db.update_donor_status(donor_id, new_status)
                self._send_json_response(200, {"status": "success", "message": f"Donor {donor_id} status updated to {new_status}"})
            except Exception as e:
                self._send_json_response(400, {"status": "error", "message": str(e)})
            return

        # 5. Admin / General Update Donor: /api/donors/<id>
        if path.startswith("/api/donors/"):
            donor_id = path.split("/")[-1]
            try:
                db.update_donor_record(donor_id, body)
                self._send_json_response(200, {"status": "success", "message": f"Donor {donor_id} updated"})
            except Exception as e:
                self._send_json_response(400, {"status": "error", "message": str(e)})
            return

        # 6. Admin / General Update Recipient: /api/recipients/<id>
        if path.startswith("/api/recipients/"):
            recipient_id = path.split("/")[-1]
            try:
                db.update_recipient_record(recipient_id, body)
                self._send_json_response(200, {"status": "success", "message": f"Recipient {recipient_id} updated"})
            except Exception as e:
                self._send_json_response(400, {"status": "error", "message": str(e)})
            return

        self._send_json_response(404, {"status": "error", "message": "Route not found"})

    # =========================================================================
    # DELETE Handlers
    # =========================================================================
    def do_DELETE(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path

        if path.startswith("/api/donors/"):
            donor_id = path.split("/")[-1]
            conn = db.get_db_connection()
            cur = conn.cursor()
            cur.execute("DELETE FROM donors WHERE id = ?", (donor_id,))
            conn.commit()
            db.sync_csv_files(conn)
            conn.close()
            self._send_json_response(200, {"status": "success", "message": f"Donor {donor_id} deleted"})
            return

        if path.startswith("/api/recipients/"):
            recipient_id = path.split("/")[-1]
            conn = db.get_db_connection()
            cur = conn.cursor()
            cur.execute("DELETE FROM recipients WHERE id = ?", (recipient_id,))
            conn.commit()
            db.sync_csv_files(conn)
            conn.close()
            self._send_json_response(200, {"status": "success", "message": f"Recipient {recipient_id} deleted"})
            return

        self._send_json_response(404, {"status": "error", "message": "Route not found"})


def find_available_port(preferred_port=5000):
    """Check if preferred_port is available; if not, find the next free port."""
    for p in range(preferred_port, preferred_port + 50):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            try:
                s.bind(("127.0.0.1", p))
                return p
            except OSError:
                continue
    return preferred_port


def main():
    # Initialize SQLite Database & Sample Data
    db.init_db()

    preferred_port = PORT
    if len(sys.argv) > 1:
        try:
            preferred_port = int(sys.argv[1])
        except ValueError:
            pass

    port = find_available_port(preferred_port)
    server_address = ("127.0.0.1", port)

    try:
        httpd = http.server.HTTPServer(server_address, OrganMatchingHandler)
    except OSError as err:
        print(f"Error binding to port {port}: {err}")
        return

    print("==================================================================")
    print("  Smart Organ Donation System Web Server Running")
    print(f"  Local URL: http://localhost:{port}")
    print(f"  Backend: C Executable ({C_EXE})")
    print("  Database: SQLite (data/organ_donation.db)")
    print(f"  Authorized Admin Email: {db.ADMIN_EMAIL}")
    print("==================================================================")
    print(f"\n👉 Open this URL in your browser: http://localhost:{port}\n")

    if "--no-browser" not in sys.argv:
        def open_browser():
            time.sleep(0.8)
            webbrowser.open_new_tab(f"http://localhost:{port}")
        threading.Thread(target=open_browser, daemon=True).start()

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nServer shutting down gracefully.")
        httpd.server_close()


if __name__ == "__main__":
    main()
