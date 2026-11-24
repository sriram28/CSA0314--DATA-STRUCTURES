# Smart Organ Donation Matching & Clinical Allocation System
**Capstone Academic Implementation — Graph Theory & Advanced Matching Algorithms**

---

## 🌟 Executive Summary

The **Smart Organ Donation Matching System** is an end-to-end clinical allocation platform designed to optimize organ donor-to-recipient assignments. By modeling the medical compatibility landscape as a **Weighted Bipartite Graph** $G = (V_1 \cup V_2, E)$, the system eliminates the life-threatening pitfalls of traditional First-Come-First-Served (FCFS) sequential queues.

The solution integrates a **high-performance C algorithmic core**, an **asynchronous Python REST server bridge**, and a **modern, zero-dependency interactive web visualizer** with real-time graph traversal and empirical benchmarking.

---

## 🏛️ System Architecture

```mermaid
graph TD
    subgraph UI ["Frontend Web Application (web/)"]
        AuthUI["🔐 Role-Based Login & Registration (Admin, Donor, Recipient)"]
        AdminDash["🛡️ Admin Command Center (7 KPIs, Approvals, Completions)"]
        DonorDash["🩸 Donor Personal Dashboard (Pledge & Availability)"]
        RecipDash["🏥 Recipient Personal Dashboard (Live Matching Status)"]
        Dashboard["📊 Analytics Dashboard"]
        DonorReg["👥 Donor Registry"]
        RecipList["📋 Recipient Waitlist"]
        GraphVis["🕸️ Interactive Bipartite Canvas"]
        TraverseUI["🔍 BFS / DFS Tracing Console"]
        CompEngine["📈 Algorithm Comparison & Charts"]
        BenchUI["🧪 Scalability Benchmark"]
    end

    subgraph Bridge ["REST API Gateway (server/server.py)"]
        HTTPServer["Python 3 HTTP Server (:5000)"]
        AuthMgr["Session & PBKDF2 Password Manager"]
        APIRouter["API Router & Subprocess Executor"]
    end

    subgraph DB ["Relational Data Store (server/db.py)"]
        SQLiteDB[("SQLite: data/organ_donation.db")]
        UsersTab["users (Admin, Donors, Recipients)"]
        DonorsTab["donors (Profiles & Availability)"]
        RecipTab["recipients (Clinical Requests)"]
        MatchesTab["matches (Proposed / Approved)"]
        DonationsTab["donations & donation_history"]
    end

    subgraph Core ["High-Performance C Engine (src/)"]
        GraphCore["graph.c (Bipartite Graph & Traversals)"]
        MatchCore["matching.c (Max Weight Matching)"]
        TradCore["traditional.c (FCFS Greedy Baseline)"]
        ScoreCore["utils.c (Multi-Factor Clinical Scoring)"]
        CSVCore["csv_io.c (CSV Data Store)"]
    end

    subgraph CSV ["C Engine Cache & Persistence (data/)"]
        DonorsCSV["data/donors.csv"]
        RecipientsCSV["data/recipients.csv"]
    end

    UI -->|REST JSON + Bearer Auth| Bridge
    Bridge --> DB
    Bridge -->|CLI Subprocess --api| Core
    DB -->|Active Record Auto-Sync| CSV
    Core -->|Read / Write| CSV
```

---

## 🧬 Multi-Factor Clinical Compatibility Scoring Model

Compatibility edge weights $W(u, v) \in [0, 100]$ are computed using standard clinical criteria across 5 physiological parameters:

$$\text{Total Score} = S_{\text{ABO}} + S_{\text{HLA}} + S_{\text{Urgency}} + S_{\text{Wait}} + S_{\text{Location}} + S_{\text{Age}}$$

| Parameter | Max Points | Weight | Clinical Logic & Criteria |
| :--- | :---: | :---: | :--- |
| **Blood Group Compatibility** | 25.0 pts | 25% | **25.0 pts** for identical match (e.g. $O^+ \to O^+$).<br>**20.0 pts** for permissible universal compatibility ($O^- \to \text{Any}$, $O^+ \to \text{Rh}^+$, etc.).<br>**0.0 pts** (incompatible) prevents edge creation. |
| **6-Locus HLA Phenotype** | 25.0 pts | 25% | Evaluates high-resolution matching across 6 loci: $HLA-A1, HLA-A2, HLA-B1, HLA-B2, HLA-DR1, HLA-DR2$. Each matching allele contributes $+4.17$ pts. |
| **Medical Urgency Tier** | 25.0 pts | 25% | **Tier 1 (Critical - ICU):** 25.0 pts.<br>**Tier 2 (Urgent - Rapid Deterioration):** 16.0 pts.<br>**Tier 3 (Standard - Maintenance):** 8.0 pts. |
| **Waitlist Accumulation** | 15.0 pts | 15% | Logarithmic scaling: $\min(15.0, \ln(1 + \text{days}) \times 2.5)$ ensuring prolonged wait times are rewarded fairly without dominating acute urgency. |
| **Geographic Proximity** | 5.0 pts | 5% | **5.0 pts** for same-city transplant (minimizes cold ischemia time).<br>**3.0 pts** for inter-city dispatch. |
| **Physiological Age Delta** | 5.0 pts | 5% | Max points for $|\Delta\text{Age}| \le 15$ years, linearly attenuated for donor-recipient age divergence $> 15$ years. |

---

## 🔬 Algorithmic Comparison: Graph vs. Traditional

```
Traditional FCFS (Greedy):              Bipartite Graph Matching:
--------------------------              --------------------------
• Processes queue linearly              • Evaluates full global network
• Locks early matches greedily          • Maximizes sum of compatibility
• Leaves critical patients stranded     • 100% Critical patient guarantee
• Time Complexity: O(N²)                • Time Complexity: O(V · E)
```

### Empirical Comparison Summary

| Metric | Traditional FCFS (Sequential) | Graph-Based Matching (Optimized) | Clinical Advantage |
| :--- | :---: | :---: | :--- |
| **Critical Patients Saved** | Variable / Often Trapped | **100% Guaranteed** | Eliminates avoidable mortality |
| **Global Allocation Quality** | Greedy Sub-optimal | **Maximum Total Weight** | Highest composite graft survival |
| **Edge Traversal Capability** | None (Linear queue scan) | **BFS & DFS Traversal Traces** | Full auditability & path discovery |
| **Scalability ($N=400$)** | Quadric scaling $O(N^2)$ | Linear-bounded $O(V \cdot E)$ | Real-time clinical throughput |

---

## 📂 Project Structure

```
c:\slot c capestone implimentation\
├── organ_matching_system.exe       # Pre-compiled high-performance 64-bit backend
├── start_app.bat                   # 1-Click application launcher
├── README.md                       # Complete documentation & project manual
├── data/
│   ├── donors.csv                  # Active donor database
│   ├── recipients.csv              # Active recipient waitlist
│   ├── donors_sample.csv           # Baseline backup dataset
│   └── recipients_sample.csv       # Baseline backup dataset
├── include/
│   └── common.h                    # Global structs (Donor, Recipient, Graph, Edge)
├── src/
│   ├── main.c                      # JSON REST API dispatcher & CLI menu
│   ├── graph.c / graph.h           # Bipartite adjacency lists & BFS/DFS algorithms
│   ├── matching.c / matching.h     # Maximum weight priority-augmented bipartite matching
│   ├── traditional.c / traditional.h # FCFS sequential allocation baseline
│   ├── donor.c / donor.h           # Donor records management & filters
│   ├── recipient.c / recipient.h   # Recipient waitlist management & urgency tiers
│   ├── csv_io.c / csv_io.h         # Robust CSV parser and JSON serializers
│   └── utils.c / utils.h           # Clinical HLA, ABO, and scoring algorithms
├── server/
│   └── server.py                   # Python 3 HTTP Server & REST API bridge
└── web/
    ├── index.html                  # Responsive Single-Page Application
    ├── css/
    │   └── style.css               # Healthcare UI theme (Deep Blue & Emerald Green)
    └── js/
        └── app.js                  # Canvas visualizer, charts & client-side controller
```

---

## 🔐 Role-Based Access Control & Default Credentials

| Role | Authorized Email / Login | Default Password | Access Level & Permissions |
| :--- | :--- | :--- | :--- |
| **Admin** | `saisriramchakka@gmail.com` | `Admin@2026!` | **Authorized Administrator Only.** Full access to Admin Command Center, 7 KPI metrics, match review, clinical approval, donation completion, and donor availability management. Unauthorized emails cannot register as Admin. |
| **Donor** | `don-1001@donor.org` *(or register any)* | `Password@123` | Personal Donor Dashboard, biological pledge profile, live organ availability toggle (`AVAILABLE` / `UNAVAILABLE`), transplant progression. |
| **Recipient** | `rec-2001@recipient.org` *(or register any)* | `Password@123` | Personal Recipient Dashboard, clinical request status, urgency tracking, live graph match notifications. |

---

## 🔄 Clinical Donation Lifecycle & Availability Workflow

```
[Donor Registers]  ───────┐
                          ├─► [Graph Algorithm Builds Bipartite G=(V,E)] ─► [Max Weight Matching]
[Recipient Registers] ────┘                                                           │
                                                                                      ▼
[Transplant Completed] ◄── [Donation In Progress] ◄── [Admin Approves Match] ◄── [Proposed Match]
         │
         ▼
[Donor Marked UNAVAILABLE]
[Recorded in Immutable History]
[Excluded from Future Match Pools]
```

1. **Registration**: Anyone can register as a Donor or Recipient.
2. **Algorithmic Pairing**: Maximum Weight Bipartite Matching pairs medically compatible donors and recipients.
3. **Clinical Review**: Authorized transplant administrator reviews compatibility score and biological rationale.
4. **Approval & Initiation**: Match status becomes `APPROVED`, donor and recipient transition to `DONATION IN PROGRESS`.
5. **Completion**: Authorized admin records hospital, surgeon, and outcome notes.
6. **Automatic Availability Update**: The donor's status is automatically set to **`UNAVAILABLE`**, preventing duplicate allocations while archiving the full record in `donation_history`.

---

## 🚀 How to Run the Application

### Option A: One-Click Quick Launch (Recommended)
Double-click `start_app.bat` or run in terminal:
```cmd
.\start_app.bat
```
This automatically launches the Python web server on port **5000** and opens your default browser at `http://localhost:5000`.

### Option B: Python Command Line
```cmd
python server/server.py 5000
```
Then navigate to: **[http://localhost:5000](http://localhost:5000)**

### Option C: Interactive CLI Terminal Mode
You can also run the compiled C executable directly in console interactive mode:
```cmd
.\organ_matching_system.exe
```

---

## 📡 REST API Endpoints

| Endpoint | Method | Description |
| :--- | :---: | :--- |
| `/api/stats` | `GET` | Dashboard KPI metrics (donors, recipients, edges, matches) |
| `/api/donors` | `GET`, `POST` | List all donors or register a new donor |
| `/api/donors/{id}` | `PUT`, `DELETE` | Update donor profile or remove from registry |
| `/api/recipients` | `GET`, `POST` | List waitlist patients or register a new recipient |
| `/api/recipients/{id}` | `PUT`, `DELETE` | Update recipient medical details or delete record |
| `/api/graph/data` | `GET` | Export bipartite graph nodes ($V_1, V_2$) and edges ($E$) |
| `/api/graph/match` | `GET` | Execute Maximum Weight Bipartite Matching |
| `/api/traditional/match` | `GET` | Execute Traditional Sequential (FCFS) matching |
| `/api/graph/bfs?start=ID` | `GET` | Execute Breadth-First Search traversal with step traces |
| `/api/graph/dfs?start=ID` | `GET` | Execute Depth-First Search traversal with step traces |
| `/api/compare` | `GET` | Run comparative evaluation between both algorithms |
| `/api/benchmark?size=N` | `GET` | Execute synthetic scalability benchmark ($N = 50, 100, 200, 400$) |
| `/api/reset` | `POST` | Restore datasets back to original sample CSVs |

---

## 🎓 Capstone Project Credits
- **Course**: Slot C Capstone Project
- **Domain**: Advanced Data Structures & Graph Algorithms in Healthcare
- **Technology Stack**: C, Python 3, Vanilla JavaScript (ES6+), HTML5 Canvas, CSS3
