/**
 * app.js - Enhanced Application Logic for Smart Organ Donation System
 * Supports Role-Based Authentication (Admin, Donor, Recipient),
 * Complete Clinical Donation Workflow (Registration, Approval, Completion, Availability),
 * Interactive Bipartite Canvas Visualizer, Traversal Traces, and Zero-CDN Charts.
 */

// Application State
const state = {
  currentView: 'dashboard',
  currentUser: null,  // { id, email, name, role }
  token: localStorage.getItem('organ_auth_token') || null,
  userProfile: null,  // donor or recipient profile
  
  stats: null,
  donors: [],
  recipients: [],
  graphData: null,
  matchingResult: null,
  comparisonData: null,
  benchmarkHistory: [],
  
  // Admin Dashboard State
  admin: {
    activeTab: 'matches',
    stats: null,
    matches: [],
    donors: [],
    recipients: [],
    history: []
  },

  // Graph Canvas State
  graph: {
    canvas: null,
    ctx: null,
    zoom: 1,
    panX: 0,
    panY: 0,
    isDragging: false,
    dragStartX: 0,
    dragStartY: 0,
    hoveredNode: null,
    selectedNode: null,
    activeMatches: new Map(),
    traversalStep: -1,
    traversalNodes: [],
    animationTimer: null,
    showAllEdges: true
  }
};

const API_BASE = '';

// ============================================================================
// API Helper Functions
// ============================================================================

async function apiFetch(endpoint, options = {}) {
  const headers = { 'Content-Type': 'application/json', ...(options.headers || {}) };
  if (state.token) {
    headers['Authorization'] = `Bearer ${state.token}`;
  }

  try {
    const res = await fetch(`${API_BASE}${endpoint}`, {
      ...options,
      headers
    });
    const data = await res.json();
    if (!res.ok) {
      throw new Error(data.message || `HTTP ${res.status}`);
    }
    return data;
  } catch (err) {
    console.error(`API Error on ${endpoint}:`, err);
    throw err;
  }
}

// Toast Notifications
function showToast(message, type = 'info') {
  const container = document.getElementById('toast-container');
  if (!container) return;

  const toast = document.createElement('div');
  toast.className = `toast ${type}`;
  let icon = 'ℹ️';
  if (type === 'success') icon = '✓';
  if (type === 'error') icon = '⚠️';

  toast.innerHTML = `<span style="font-weight:700;">${icon}</span><span>${message}</span>`;
  container.appendChild(toast);

  setTimeout(() => {
    toast.style.opacity = '0';
    toast.style.transform = 'translateY(10px)';
    toast.style.transition = 'all 0.3s ease';
    setTimeout(() => toast.remove(), 300);
  }, 4000);
}

// ============================================================================
// Authentication & Session Management
// ============================================================================

async function checkAuthSession() {
  if (!state.token) {
    updateAuthHeaderUI();
    return;
  }

  try {
    const data = await apiFetch('/api/auth/me');
    state.currentUser = data.user;
    state.userProfile = data.profile;
    updateAuthHeaderUI();
    updateRoleNavUI();
  } catch (e) {
    // Expired or invalid token
    state.token = null;
    state.currentUser = null;
    state.userProfile = null;
    localStorage.removeItem('organ_auth_token');
    updateAuthHeaderUI();
    updateRoleNavUI();
  }
}

function updateAuthHeaderUI() {
  const container = document.getElementById('header-auth-bar');
  if (!container) return;

  if (state.currentUser) {
    const role = state.currentUser.role;
    const roleIcon = role === 'admin' ? '🛡️' : (role === 'donor' ? '🩸' : '🏥');
    container.innerHTML = `
      <div class="user-profile-pill">
        <span class="role-badge ${role}">${roleIcon} ${role}</span>
        <span>${state.currentUser.name}</span>
      </div>
      <button class="btn btn-primary btn-sm" onclick="goToMyDashboard()">My Dashboard</button>
      <button class="btn btn-outline btn-sm" onclick="logoutUser()">Logout</button>
    `;
  } else {
    container.innerHTML = `
      <button class="btn btn-primary btn-sm" onclick="openLoginModal()">Sign In</button>
      <button class="btn btn-outline btn-sm" onclick="openRegisterModal('donor')">Register</button>
    `;
  }
}

function updateRoleNavUI() {
  const role = state.currentUser ? state.currentUser.role : null;
  const adminNav = document.getElementById('nav-item-admin');
  const donorNav = document.getElementById('nav-item-donor');
  const recipNav = document.getElementById('nav-item-recipient');

  if (adminNav) adminNav.style.display = (role === 'admin') ? 'flex' : 'none';
  if (donorNav) donorNav.style.display = (role === 'donor') ? 'flex' : 'none';
  if (recipNav) recipNav.style.display = (role === 'recipient') ? 'flex' : 'none';
}

function goToMyDashboard() {
  if (!state.currentUser) {
    openLoginModal();
    return;
  }
  const role = state.currentUser.role;
  if (role === 'admin') switchView('admin');
  else if (role === 'donor') switchView('donor-dashboard');
  else if (role === 'recipient') switchView('recipient-dashboard');
  else switchView('dashboard');
}

async function logoutUser() {
  try {
    await apiFetch('/api/auth/logout', { method: 'POST' });
  } catch (e) {}
  state.token = null;
  state.currentUser = null;
  state.userProfile = null;
  localStorage.removeItem('organ_auth_token');
  updateAuthHeaderUI();
  updateRoleNavUI();
  showToast('Logged out successfully', 'info');
  switchView('dashboard');
}

// Quick Demo Login Shortcut
async function triggerQuickLogin(role) {
  let email = '';
  let password = '';
  if (role === 'admin') {
    email = 'saisriramchakka@gmail.com';
    password = 'Admin@2026!';
  } else if (role === 'donor') {
    email = 'don-1001@donor.org';
    password = 'Password@123';
  } else if (role === 'recipient') {
    email = 'rec-2001@recipient.org';
    password = 'Password@123';
  }

  try {
    const res = await apiFetch('/api/auth/login', {
      method: 'POST',
      body: JSON.stringify({ email, password, role })
    });
    state.token = res.token;
    state.currentUser = res.user;
    state.userProfile = res.profile;
    localStorage.setItem('organ_auth_token', res.token);
    updateAuthHeaderUI();
    updateRoleNavUI();
    showToast(`Logged in as ${res.user.name} (${res.user.role.toUpperCase()})`, 'success');
    goToMyDashboard();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

// ============================================================================
// Navigation & View Routing
// ============================================================================

const viewTitles = {
  dashboard: { title: 'Dashboard Overview', subtitle: 'Real-time organ donor-recipient matching and allocation analytics' },
  admin: { title: 'Admin Command Center', subtitle: 'Authorized transplant workflow, donor availability, and match approvals' },
  'donor-dashboard': { title: 'Donor Personal Dashboard', subtitle: 'Manage your organ pledge, medical status, and availability' },
  'recipient-dashboard': { title: 'Recipient Clinical Dashboard', subtitle: 'Live tracking of your organ request and algorithmic match status' },
  donors: { title: 'Donor Registry', subtitle: 'Biological profiles and organ pledge availability' },
  recipients: { title: 'Recipient Waitlist', subtitle: 'Clinical waitlist prioritized by urgency tiers and waiting time' },
  graph: { title: 'Bipartite Compatibility Graph', subtitle: 'Bipartite network G = (V₁ ∪ V₂, E) with BFS & DFS traversal engines' },
  matching: { title: 'Matching Engine', subtitle: 'Maximum Weight Bipartite Matching vs Sequential FCFS Allocation' },
  comparison: { title: 'Algorithmic Comparison', subtitle: 'Empirical performance analysis: Graph-Based vs Traditional Greedy Baseline' },
  benchmark: { title: 'Scalability Benchmark', subtitle: 'Synthetic cohort stress tests evaluating computational scaling O(V · E) vs O(N²)' }
};

function switchView(viewName) {
  // Access control guards
  if (viewName === 'admin' && (!state.currentUser || state.currentUser.role !== 'admin')) {
    showToast('Admin dashboard requires authorized administrator login.', 'error');
    openLoginModal('admin');
    return;
  }
  if (viewName === 'donor-dashboard' && (!state.currentUser || state.currentUser.role !== 'donor')) {
    showToast('Please log in with your Donor account.', 'error');
    openLoginModal('donor');
    return;
  }
  if (viewName === 'recipient-dashboard' && (!state.currentUser || state.currentUser.role !== 'recipient')) {
    showToast('Please log in with your Recipient account.', 'error');
    openLoginModal('recipient');
    return;
  }

  state.currentView = viewName;

  document.querySelectorAll('.nav-item').forEach(item => {
    item.classList.toggle('active', item.dataset.view === viewName);
  });

  document.querySelectorAll('.view-page').forEach(page => {
    page.classList.toggle('active', page.id === `view-${viewName}`);
  });

  const meta = viewTitles[viewName] || { title: 'Smart Organ Donation System', subtitle: '' };
  document.getElementById('page-title').textContent = meta.title;
  document.getElementById('page-subtitle').textContent = meta.subtitle;

  // View loaders
  if (viewName === 'dashboard') loadDashboard();
  if (viewName === 'admin') loadAdminDashboard();
  if (viewName === 'donor-dashboard') loadDonorDashboard();
  if (viewName === 'recipient-dashboard') loadRecipientDashboard();
  if (viewName === 'donors') loadDonors();
  if (viewName === 'recipients') loadRecipients();
  if (viewName === 'graph') initGraphVisualizer();
  if (viewName === 'matching') loadMatchingView();
  if (viewName === 'comparison') loadComparisonView();
  if (viewName === 'benchmark') loadBenchmarkView();
}

// ============================================================================
// ADMIN DASHBOARD MODULE (Section 4, 5, 7)
// ============================================================================

async function loadAdminDashboard() {
  try {
    const statsRes = await apiFetch('/api/admin/stats');
    state.admin.stats = statsRes.stats;

    // Populate 7 KPI stats
    document.getElementById('admin-stat-total-donors').textContent = statsRes.stats.total_donors || 0;
    document.getElementById('admin-stat-total-recipients').textContent = statsRes.stats.total_recipients || 0;
    document.getElementById('admin-stat-available-donors').textContent = statsRes.stats.available_donors || 0;
    document.getElementById('admin-stat-unavailable-donors').textContent = statsRes.stats.unavailable_donors || 0;
    document.getElementById('admin-stat-total-requests').textContent = statsRes.stats.total_organ_requests || 0;
    document.getElementById('admin-stat-successful-donations').textContent = statsRes.stats.successful_donations || 0;
    document.getElementById('admin-stat-pending-matches').textContent = statsRes.stats.pending_matches || 0;

    // Load active tab content
    switchAdminTab(state.admin.activeTab || 'matches');
  } catch (err) {
    showToast(`Failed to load admin stats: ${err.message}`, 'error');
  }
}

function switchAdminTab(tabName) {
  state.admin.activeTab = tabName;
  document.querySelectorAll('.sub-tab').forEach(t => {
    t.classList.toggle('active', t.dataset.adminTab === tabName);
  });
  document.querySelectorAll('.admin-tab-content').forEach(c => {
    c.style.display = (c.id === `admin-tab-${tabName}`) ? 'block' : 'none';
  });

  if (tabName === 'matches') loadAdminMatches();
  if (tabName === 'donors') loadAdminDonors();
  if (tabName === 'recipients') loadAdminRecipients();
  if (tabName === 'history') loadAdminHistory();
}

async function loadAdminMatches() {
  const tbody = document.getElementById('admin-matches-tbody');
  tbody.innerHTML = '<tr><td colspan="8" style="text-align:center; padding:20px;">Loading proposed matches...</td></tr>';

  try {
    const res = await apiFetch('/api/admin/matches');
    state.admin.matches = res.matches || [];

    if (state.admin.matches.length === 0) {
      tbody.innerHTML = '<tr><td colspan="8" style="text-align:center; padding:24px; color:#64748b;">No proposed matches in database. Click "Re-Run Graph Matching" to generate candidates from active donors and recipients.</td></tr>';
      return;
    }

    tbody.innerHTML = '';
    state.admin.matches.forEach(m => {
      const tr = document.createElement('tr');
      const isPending = (m.status === 'PROPOSED');
      const isApproved = (m.status === 'APPROVED' || m.status === 'DONATION IN PROGRESS');
      const isCompleted = (m.status === 'DONATION COMPLETED');

      let actionHtml = '';
      if (isPending) {
        actionHtml = `
          <button class="btn btn-sm btn-primary" onclick="adminApproveMatch(${m.id})">Approve</button>
          <button class="btn btn-sm btn-outline" style="color:#ef4444;" onclick="adminRejectMatch(${m.id})">Reject</button>
        `;
      } else if (isApproved) {
        actionHtml = `
          <button class="btn btn-sm btn-success" onclick="openCompleteDonationModal(${m.id}, '${m.donor_id}', '${m.recipient_id}', '${m.organ}')">Complete Donation</button>
        `;
      } else if (isCompleted) {
        actionHtml = `<span style="font-size:11px; color:#15803d; font-weight:700;">✓ Completed</span>`;
      } else {
        actionHtml = `<span style="font-size:11px; color:#64748b;">${m.status}</span>`;
      }

      tr.innerHTML = `
        <td style="font-family:monospace; font-weight:700;">#${m.id}</td>
        <td>
          <strong>${m.donor_name || m.donor_id}</strong>
          <div style="font-size:11px; color:#64748b;">${m.donor_id} • ${m.donor_bg || ''} • ${m.donor_location || ''}</div>
        </td>
        <td>
          <strong>${m.recipient_name || m.recipient_id}</strong>
          <div style="font-size:11px; color:#64748b;">${m.recipient_id} • ${m.recipient_bg || ''} • ${m.recipient_location || ''}</div>
        </td>
        <td><span class="badge badge-organ">${m.organ}</span></td>
        <td><span class="badge badge-score ${getScoreClass(m.compatibility_score)}">${m.compatibility_score.toFixed(1)}%</span></td>
        <td><span class="badge ${getStatusBadgeClass(m.status)}">${m.status}</span></td>
        <td style="max-width:240px; font-size:11px; color:#475569; overflow:hidden; text-overflow:ellipsis; white-space:nowrap;" title="${m.explanation}">
          ${m.explanation || '-'}
        </td>
        <td style="text-align:right; white-space:nowrap;">
          ${actionHtml}
        </td>
      `;
      tbody.appendChild(tr);
    });
  } catch (err) {
    tbody.innerHTML = `<tr><td colspan="8" style="text-align:center; color:#ef4444;">Error loading matches: ${err.message}</td></tr>`;
  }
}

async function adminApproveMatch(matchId) {
  if (!confirm(`Approve Match #${matchId} and initiate transplant workflow?`)) return;
  try {
    await apiFetch(`/api/admin/matches/${matchId}/approve`, {
      method: 'POST',
      body: JSON.stringify({ notes: 'Clinical compatibility verified by transplant team' })
    });
    showToast(`Match #${matchId} approved. Workflow in progress.`, 'success');
    loadAdminDashboard();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

async function adminRejectMatch(matchId) {
  if (!confirm(`Reject Match #${matchId}?`)) return;
  try {
    await apiFetch(`/api/admin/matches/${matchId}/reject`, {
      method: 'POST',
      body: JSON.stringify({ notes: 'Rejected by transplant board' })
    });
    showToast(`Match #${matchId} rejected.`, 'info');
    loadAdminDashboard();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

function openCompleteDonationModal(matchId, donorId, recipientId, organ) {
  document.getElementById('cd-match-id').value = matchId;
  openModal('modal-complete-donation');
}

async function handleCompleteDonationSubmit(e) {
  e.preventDefault();
  const matchId = document.getElementById('cd-match-id').value;
  const hospital = document.getElementById('cd-hospital').value.trim();
  const surgeon = document.getElementById('cd-surgeon').value.trim();
  const notes = document.getElementById('cd-notes').value.trim();

  try {
    await apiFetch('/api/admin/donations/complete', {
      method: 'POST',
      body: JSON.stringify({ match_id: matchId, hospital, surgeon, notes })
    });
    closeModal('modal-complete-donation');
    showToast('Donation COMPLETED! Donor availability set to UNAVAILABLE.', 'success');
    loadAdminDashboard();
    loadDashboard();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

async function loadAdminDonors() {
  const search = document.getElementById('admin-donor-search')?.value || '';
  const status = document.getElementById('admin-donor-filter-status')?.value || '';
  const organ = document.getElementById('admin-donor-filter-organ')?.value || '';

  const params = new URLSearchParams();
  if (search) params.append('search', search);
  if (status) params.append('status', status);
  if (organ) params.append('organ', organ);

  const tbody = document.getElementById('admin-donors-tbody');
  tbody.innerHTML = '<tr><td colspan="8" style="text-align:center; padding:20px;">Loading donors...</td></tr>';

  try {
    const res = await apiFetch(`/api/admin/donors?${params.toString()}`);
    tbody.innerHTML = '';
    const donors = res.donors || [];

    if (donors.length === 0) {
      tbody.innerHTML = '<tr><td colspan="8" style="text-align:center; padding:20px;">No donors found</td></tr>';
      return;
    }

    donors.forEach(d => {
      const tr = document.createElement('tr');
      const isAvail = (d.status === 'AVAILABLE');
      tr.innerHTML = `
        <td><strong style="color:#0284c7;">${d.id}</strong></td>
        <td><strong>${d.name}</strong><div style="font-size:11px; color:#64748b;">${d.age}y / ${d.gender}</div></td>
        <td>${d.phone || '-'} <span style="font-size:11px; color:#64748b;">(${d.city}, ${d.state})</span></td>
        <td><span class="badge badge-blood">${d.blood_group}</span></td>
        <td><span class="badge badge-organ">${d.organ_type}</span></td>
        <td><span style="font-size:12px; color:#475569;">${d.health_condition || 'Normal'}</span></td>
        <td><span class="badge ${getStatusBadgeClass(d.status)}">${d.status}</span></td>
        <td style="text-align:right;">
          <select class="select-control" style="font-size:11px; padding:4px 8px;" onchange="changeDonorStatusAdmin('${d.id}', this.value)">
            <option value="AVAILABLE" ${d.status === 'AVAILABLE' ? 'selected' : ''}>Available</option>
            <option value="MATCHED" ${d.status === 'MATCHED' ? 'selected' : ''}>Matched</option>
            <option value="DONATION IN PROGRESS" ${d.status === 'DONATION IN PROGRESS' ? 'selected' : ''}>In Progress</option>
            <option value="DONATION COMPLETED" ${d.status === 'DONATION COMPLETED' ? 'selected' : ''}>Completed</option>
            <option value="UNAVAILABLE" ${d.status === 'UNAVAILABLE' ? 'selected' : ''}>Unavailable</option>
          </select>
        </td>
      `;
      tbody.appendChild(tr);
    });
  } catch (err) {
    tbody.innerHTML = `<tr><td colspan="8" style="color:#ef4444;">${err.message}</td></tr>`;
  }
}

async function changeDonorStatusAdmin(donorId, newStatus) {
  try {
    await apiFetch(`/api/admin/donors/${donorId}/status`, {
      method: 'PUT',
      body: JSON.stringify({ status: newStatus })
    });
    showToast(`Donor ${donorId} status updated to ${newStatus}`, 'success');
    loadAdminDashboard();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

async function loadAdminRecipients() {
  const search = document.getElementById('admin-recipient-search')?.value || '';
  const urgency = document.getElementById('admin-recipient-filter-urgency')?.value || '';
  const organ = document.getElementById('admin-recipient-filter-organ')?.value || '';

  const params = new URLSearchParams();
  if (search) params.append('search', search);
  if (urgency) params.append('urgency', urgency);
  if (organ) params.append('organ', organ);

  const tbody = document.getElementById('admin-recipients-tbody');
  tbody.innerHTML = '<tr><td colspan="8" style="text-align:center; padding:20px;">Loading recipients...</td></tr>';

  try {
    const res = await apiFetch(`/api/admin/recipients?${params.toString()}`);
    tbody.innerHTML = '';
    const recs = res.recipients || [];

    if (recs.length === 0) {
      tbody.innerHTML = '<tr><td colspan="8" style="text-align:center; padding:20px;">No recipients found</td></tr>';
      return;
    }

    recs.forEach(r => {
      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td><strong style="color:#047857;">${r.id}</strong></td>
        <td><strong>${r.name}</strong><div style="font-size:11px; color:#64748b;">${r.age}y / ${r.gender}</div></td>
        <td>${r.phone || '-'} <span style="font-size:11px; color:#64748b;">(${r.city})</span></td>
        <td><span class="badge badge-blood">${r.blood_group}</span></td>
        <td><span class="badge badge-organ">${r.required_organ}</span></td>
        <td><span class="badge badge-${getUrgencyClass(r.urgency_level)}">${getUrgencyLabel(r.urgency_level)}</span></td>
        <td>${r.hospital_name || 'Hospital'}<div style="font-size:11px; color:#64748b;">${r.doctor_details || ''}</div></td>
        <td><span class="badge ${getStatusBadgeClass(r.status)}">${r.status}</span></td>
      `;
      tbody.appendChild(tr);
    });
  } catch (err) {
    tbody.innerHTML = `<tr><td colspan="8" style="color:#ef4444;">${err.message}</td></tr>`;
  }
}

async function loadAdminHistory() {
  const tbody = document.getElementById('admin-history-tbody');
  tbody.innerHTML = '<tr><td colspan="9" style="text-align:center; padding:20px;">Loading history...</td></tr>';

  try {
    const res = await apiFetch('/api/admin/donations/history');
    tbody.innerHTML = '';
    const history = res.history || [];

    if (history.length === 0) {
      tbody.innerHTML = '<tr><td colspan="9" style="text-align:center; padding:24px; color:#64748b;">No completed organ donations recorded yet.</td></tr>';
      return;
    }

    history.forEach((h, idx) => {
      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td style="font-weight:700;">#${idx + 1}</td>
        <td><strong>${h.donor_name}</strong> <span style="font-size:11px; color:#0284c7;">(${h.donor_id})</span></td>
        <td><strong>${h.recipient_name}</strong> <span style="font-size:11px; color:#047857;">(${h.recipient_id})</span></td>
        <td><span class="badge badge-organ">${h.organ}</span></td>
        <td><span class="badge badge-score high">${(h.compatibility_score || 0).toFixed(1)}%</span></td>
        <td style="font-size:12px; color:#475569;">${h.completion_date}</td>
        <td>${h.hospital}</td>
        <td>${h.transplant_surgeon}</td>
        <td style="font-size:11px; color:#64748b;">${h.clinical_notes || '-'}</td>
      `;
      tbody.appendChild(tr);
    });
  } catch (err) {
    tbody.innerHTML = `<tr><td colspan="9" style="color:#ef4444;">${err.message}</td></tr>`;
  }
}

// ============================================================================
// DONOR PERSONAL DASHBOARD MODULE (Section 2, 5)
// ============================================================================

async function loadDonorDashboard() {
  try {
    const [profileRes, matchRes] = await Promise.all([
      apiFetch('/api/donor/profile'),
      apiFetch('/api/donor/matches')
    ]);

    const p = profileRes.profile;
    state.userProfile = p;

    if (!p) {
      showToast('Donor profile not found', 'error');
      return;
    }

    document.getElementById('donor-dash-welcome').textContent = `Welcome, ${p.name}`;
    document.getElementById('donor-dash-id').textContent = `Donor ID: ${p.id} • Registered: ${p.reg_date || '-'}`;
    
    // Status badge
    const badge = document.getElementById('donor-dash-status-badge');
    badge.textContent = p.status;
    badge.className = `badge ${getStatusBadgeClass(p.status)}`;

    // Profile details
    document.getElementById('donor-p-name').textContent = p.name;
    document.getElementById('donor-p-email').textContent = p.email;
    document.getElementById('donor-p-phone').textContent = p.phone || 'Not provided';
    document.getElementById('donor-p-age-gender').textContent = `${p.age} years / ${p.gender}`;
    document.getElementById('donor-p-bg').textContent = p.blood_group;
    document.getElementById('donor-p-organ').textContent = p.organ_type;
    document.getElementById('donor-p-location').textContent = `${p.city}, ${p.state || 'MA'}`;
    document.getElementById('donor-p-health').textContent = p.health_condition || 'Good';
    document.getElementById('donor-p-emergency').textContent = p.emergency_contact || 'None';

    // Matching progress
    const container = document.getElementById('donor-matches-container');
    const matches = matchRes.matches || [];

    if (matches.length > 0) {
      const m = matches[0];
      container.innerHTML = `
        <div style="background:#f0f9ff; border:1px solid #bae6fd; border-radius:10px; padding:16px;">
          <div style="display:flex; justify-content:space-between; align-items:center;">
            <span class="badge ${getStatusBadgeClass(m.status)}">${m.status}</span>
            <span class="badge badge-score high">Score: ${m.compatibility_score.toFixed(1)}%</span>
          </div>
          <h4 style="margin:10px 0 4px; color:#0f172a;">Matched Recipient: ${m.recipient_name}</h4>
          <p style="font-size:12px; color:#475569; margin-bottom:8px;">
            Organ: <strong>${m.organ}</strong> • Hospital: ${m.hospital_name || 'Metropolitan Center'}
          </p>
          <div style="font-size:11px; color:#64748b; background:white; padding:8px 12px; border-radius:6px; border:1px solid #e2e8f0;">
            ${m.explanation || 'Verified compatible by bipartite matching algorithm.'}
          </div>
        </div>
      `;
    } else if (p.status === 'DONATION COMPLETED' || p.status === 'UNAVAILABLE') {
      container.innerHTML = `
        <div style="text-align:center; padding:30px; color:#047857;">
          <div style="font-size:36px; margin-bottom:8px;">🏆</div>
          <div style="font-weight:700; font-size:15px;">Organ Donation Completed!</div>
          <p style="font-size:12px; color:#64748b; margin-top:6px;">Thank you for your life-saving contribution. Your donor availability is updated to UNAVAILABLE.</p>
        </div>
      `;
    } else {
      container.innerHTML = `
        <div style="text-align:center; padding:30px; color:#64748b;">
          <div style="font-size:32px; margin-bottom:8px;">⏳</div>
          <div style="font-weight:700; font-size:14px; color:#1e293b;">Active in Donor Registry</div>
          <p style="font-size:12px; margin-top:6px;">Your profile is actively included in graph matching runs. You will be notified here when an authorized transplant center matches your organ.</p>
        </div>
      `;
    }

  } catch (err) {
    showToast(err.message, 'error');
  }
}

// ============================================================================
// RECIPIENT PERSONAL DASHBOARD MODULE (Section 3)
// ============================================================================

async function loadRecipientDashboard() {
  try {
    const [profileRes, statusRes] = await Promise.all([
      apiFetch('/api/recipient/profile'),
      apiFetch('/api/recipient/match-status')
    ]);

    const r = profileRes.profile;
    state.userProfile = r;

    if (!r) {
      showToast('Recipient profile not found', 'error');
      return;
    }

    document.getElementById('rec-dash-welcome').textContent = `Welcome, ${r.name}`;
    document.getElementById('rec-dash-id').textContent = `Patient ID: ${r.id} • Registered: ${r.reg_date || '-'}`;
    
    // Urgency & wait badges
    const urgBadge = document.getElementById('rec-dash-urgency-badge');
    urgBadge.textContent = getUrgencyLabel(r.urgency_level);
    urgBadge.className = `badge badge-${getUrgencyClass(r.urgency_level)}`;

    document.getElementById('rec-dash-wait-badge').textContent = `${r.waiting_time || 0} Days on Waitlist`;

    // Profile details
    document.getElementById('rec-p-name').textContent = r.name;
    document.getElementById('rec-p-email').textContent = r.email;
    document.getElementById('rec-p-phone').textContent = r.phone || 'Not provided';
    document.getElementById('rec-p-age-gender').textContent = `${r.age} years / ${r.gender}`;
    document.getElementById('rec-p-bg').textContent = r.blood_group;
    document.getElementById('rec-p-organ').textContent = r.required_organ;
    document.getElementById('rec-p-location').textContent = `${r.city}, ${r.state || 'MA'}`;
    document.getElementById('rec-p-hospital').textContent = r.hospital_name || 'General Hospital';
    document.getElementById('rec-p-doctor').textContent = r.doctor_details || 'Transplant Staff';
    document.getElementById('rec-p-condition').textContent = r.medical_condition || 'Stable';

    // Live Matching Status
    const container = document.getElementById('rec-match-status-container');
    const matches = statusRes.matches || [];

    if (matches.length > 0) {
      const m = matches[0];
      container.innerHTML = `
        <div style="background:#ecfdf5; border:1px solid #a7f3d0; border-radius:10px; padding:18px;">
          <div style="display:flex; justify-content:space-between; align-items:center;">
            <span class="badge ${getStatusBadgeClass(m.status)}" style="font-size:12px;">${m.status}</span>
            <span class="badge badge-score high" style="font-size:13px;">Score: ${m.compatibility_score.toFixed(1)}%</span>
          </div>
          <h4 style="margin:12px 0 4px; color:#065f46; font-size:16px;">🎉 Potential Compatible Donor Identified!</h4>
          <p style="font-size:13px; color:#334155; margin-bottom:8px;">
            A viable <strong>${m.organ}</strong> donor has been paired via the Bipartite Graph Matching Algorithm.
          </p>
          <div style="background:white; border:1px solid #cbd5e1; border-radius:8px; padding:10px 14px; font-size:12px; color:#475569;">
            <strong>Transplant Center:</strong> ${r.hospital_name || 'Metropolitan Center'}<br>
            <strong>Status Note:</strong> Awaiting clinical review and authorized surgeon verification.
          </div>
        </div>
      `;
    } else {
      container.innerHTML = `
        <div style="text-align:center; padding:30px; color:#64748b;">
          <div style="font-size:32px; margin-bottom:8px;">🔍</div>
          <div style="font-weight:700; font-size:14px; color:#1e293b;">Searching Waitlist Registry</div>
          <p style="font-size:12px; margin-top:6px;">
            No compatible donor has been paired yet. The graph allocation engine continuously runs optimization cycles to find matching biological phenotypes.
          </p>
        </div>
      `;
    }

  } catch (err) {
    showToast(err.message, 'error');
  }
}

// ============================================================================
// DASHBOARD OVERVIEW & CORE GRAPH INTEGRATION (Section 6)
// ============================================================================

async function loadDashboard() {
  try {
    const [stats, matching] = await Promise.all([
      apiFetch('/api/stats'),
      apiFetch('/api/graph/match')
    ]);

    state.stats = stats;
    state.matchingResult = matching;

    document.getElementById('stat-donors').textContent = stats.total_donors || 0;
    document.getElementById('stat-recipients').textContent = stats.total_recipients || 0;
    document.getElementById('stat-critical').textContent = `${stats.critical_matched || 0} / ${stats.critical_total || 0}`;
    document.getElementById('stat-score').textContent = `${(stats.avg_score || 0).toFixed(1)}%`;
    document.getElementById('stat-edges').textContent = stats.total_edges || 0;
    document.getElementById('stat-matched').textContent = stats.matched_pairs || 0;

    const tbody = document.getElementById('dashboard-matches-tbody');
    tbody.innerHTML = '';
    const pairs = (matching.pairs || []).slice(0, 5);

    if (pairs.length === 0) {
      tbody.innerHTML = '<tr><td colspan="5" style="text-align:center; padding: 20px;">No allocation pairs active</td></tr>';
    } else {
      pairs.forEach(p => {
        const tr = document.createElement('tr');
        tr.style.cursor = 'pointer';
        tr.onclick = () => openMatchDetailModal(p);
        tr.innerHTML = `
          <td><strong>${p.donor_name}</strong> <span style="font-size:11px; color:#64748b;">(${p.donor_id})</span></td>
          <td><strong>${p.recipient_name}</strong> <span style="font-size:11px; color:#64748b;">(${p.recipient_id})</span></td>
          <td><span class="badge badge-organ">${p.organ}</span></td>
          <td><span class="badge badge-${getUrgencyClass(p.urgency_level)}">${p.urgency_str}</span></td>
          <td><span class="badge badge-score ${getScoreClass(p.compatibility_score)}">${p.compatibility_score.toFixed(1)}%</span></td>
        `;
        tbody.appendChild(tr);
      });
    }

    renderOrganDistribution();

  } catch (err) {
    console.error('Failed to load dashboard:', err);
  }
}

async function renderOrganDistribution() {
  try {
    const donors = await apiFetch('/api/donors');
    const container = document.getElementById('organ-distribution-bars');
    if (!container) return;

    const counts = {};
    const organs = ['Kidney', 'Liver', 'Heart', 'Lung', 'Pancreas', 'Cornea'];
    organs.forEach(o => counts[o] = 0);
    donors.forEach(d => {
      counts[d.organ_type] = (counts[d.organ_type] || 0) + 1;
    });

    const organIcons = {
      Kidney: '🫘', Liver: '🩸', Heart: '🫀', Lung: '🫁', Pancreas: '💉', Cornea: '👁️'
    };

    container.innerHTML = organs.map(org => {
      const count = counts[org] || 0;
      const pct = donors.length ? Math.round((count / donors.length) * 100) : 0;
      return `
        <div style="background:#f8fafc; border:1px solid #e2e8f0; border-radius:10px; padding:12px; text-align:center;">
          <div style="font-size:24px; margin-bottom:4px;">${organIcons[org] || '🩺'}</div>
          <div style="font-weight:700; font-size:13px; color:#1e293b;">${org}</div>
          <div style="font-size:18px; font-weight:800; color:#0284c7; margin:4px 0;">${count}</div>
          <div style="font-size:11px; color:#64748b;">${pct}% of Donors</div>
          <div style="height:4px; background:#e2e8f0; border-radius:2px; margin-top:8px; overflow:hidden;">
            <div style="height:100%; width:${pct}%; background:#0284c7;"></div>
          </div>
        </div>
      `;
    }).join('');
  } catch (e) {}
}

// ============================================================================
// DONORS & RECIPIENTS REGISTRIES (PUBLIC VIEWS)
// ============================================================================

async function loadDonors() {
  try {
    const donors = await apiFetch('/api/donors');
    state.donors = donors;
    filterAndRenderDonors();
  } catch (err) {
    console.error('Failed to load donors:', err);
  }
}

function filterAndRenderDonors() {
  const searchTerm = (document.getElementById('donor-search')?.value || '').toLowerCase();
  const organFilter = document.getElementById('donor-filter-organ')?.value || '';
  const bgFilter = document.getElementById('donor-filter-bg')?.value || '';
  const statusFilter = document.getElementById('donor-filter-status')?.value || '';

  const filtered = state.donors.filter(d => {
    const matchesSearch = !searchTerm ||
      d.id.toLowerCase().includes(searchTerm) ||
      d.name.toLowerCase().includes(searchTerm) ||
      (d.location || d.city || '').toLowerCase().includes(searchTerm) ||
      d.organ_type.toLowerCase().includes(searchTerm);
    const matchesOrgan = !organFilter || d.organ_type === organFilter;
    const matchesBg = !bgFilter || d.blood_group === bgFilter;
    const matchesStatus = !statusFilter || d.status === statusFilter;
    return matchesSearch && matchesOrgan && matchesBg && matchesStatus;
  });

  const tbody = document.getElementById('donors-table-body');
  if (!tbody) return;
  tbody.innerHTML = '';

  if (filtered.length === 0) {
    tbody.innerHTML = '<tr><td colspan="9" style="text-align:center; padding: 24px; color:#94a3b8;">No donors found matching criteria</td></tr>';
    return;
  }

  filtered.forEach(d => {
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td><span style="font-family:monospace; font-weight:700; color:#0284c7;">${d.id}</span></td>
      <td><strong>${d.name}</strong></td>
      <td>${d.age} / ${d.gender}</td>
      <td><span class="badge badge-blood">${d.blood_group}</span></td>
      <td><span class="badge badge-organ">${d.organ_type}</span></td>
      <td>${d.location || d.city || 'Boston'}</td>
      <td><span style="font-family:monospace; font-size:11px; color:#475569;" title="${d.hla_type}">${d.hla_type}</span></td>
      <td><span class="badge ${getStatusBadgeClass(d.status)}">${d.status}</span></td>
      <td style="font-size:12px; color:#64748b;">${d.reg_date || '-'}</td>
    `;
    tbody.appendChild(tr);
  });
}

async function loadRecipients() {
  try {
    const recipients = await apiFetch('/api/recipients');
    state.recipients = recipients;
    filterAndRenderRecipients();
  } catch (err) {
    console.error('Failed to load recipients:', err);
  }
}

function filterAndRenderRecipients() {
  const searchTerm = (document.getElementById('recipient-search')?.value || '').toLowerCase();
  const urgencyFilter = document.getElementById('recipient-filter-urgency')?.value || '';
  const organFilter = document.getElementById('recipient-filter-organ')?.value || '';
  const bgFilter = document.getElementById('recipient-filter-bg')?.value || '';

  const filtered = state.recipients.filter(r => {
    const matchesSearch = !searchTerm ||
      r.id.toLowerCase().includes(searchTerm) ||
      r.name.toLowerCase().includes(searchTerm) ||
      (r.location || r.city || '').toLowerCase().includes(searchTerm) ||
      r.required_organ.toLowerCase().includes(searchTerm);
    const matchesUrgency = !urgencyFilter || String(r.urgency_level) === urgencyFilter;
    const matchesOrgan = !organFilter || r.required_organ === organFilter;
    const matchesBg = !bgFilter || r.blood_group === bgFilter;
    return matchesSearch && matchesUrgency && matchesOrgan && matchesBg;
  });

  const tbody = document.getElementById('recipients-table-body');
  if (!tbody) return;
  tbody.innerHTML = '';

  if (filtered.length === 0) {
    tbody.innerHTML = '<tr><td colspan="10" style="text-align:center; padding: 24px; color:#94a3b8;">No recipients found</td></tr>';
    return;
  }

  filtered.forEach(r => {
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td><span style="font-family:monospace; font-weight:700; color:#059669;">${r.id}</span></td>
      <td><strong>${r.name}</strong></td>
      <td>${r.age} / ${r.gender}</td>
      <td><span class="badge badge-blood">${r.blood_group}</span></td>
      <td><span class="badge badge-organ">${r.required_organ}</span></td>
      <td><span class="badge badge-${getUrgencyClass(r.urgency_level)}">${getUrgencyLabel(r.urgency_level)}</span></td>
      <td><strong>${r.waiting_time}</strong> d</td>
      <td>${r.location || r.city || 'Boston'}</td>
      <td><span style="font-family:monospace; font-size:11px; color:#475569;" title="${r.hla_type}">${r.hla_type}</span></td>
      <td><span class="badge ${getStatusBadgeClass(r.status)}">${r.status}</span></td>
    `;
    tbody.appendChild(tr);
  });
}

// ============================================================================
// INTERACTIVE BIPARTITE GRAPH VISUALIZER & TRAVERSALS (Section 6)
// ============================================================================

async function initGraphVisualizer() {
  const canvas = document.getElementById('graph-canvas');
  if (!canvas) return;

  state.graph.canvas = canvas;
  state.graph.ctx = canvas.getContext('2d');

  resizeGraphCanvas();
  window.addEventListener('resize', resizeGraphCanvas);
  setupGraphInteractions();
  await loadGraphData();
}

function resizeGraphCanvas() {
  const canvas = state.graph.canvas;
  if (!canvas) return;

  const rect = canvas.getBoundingClientRect();
  const dpr = window.devicePixelRatio || 1;
  canvas.width = rect.width * dpr;
  canvas.height = (rect.height || 520) * dpr;
  state.graph.ctx.scale(dpr, dpr);
  renderGraph();
}

async function loadGraphData() {
  try {
    const [graphData, matchingData] = await Promise.all([
      apiFetch('/api/graph/data'),
      apiFetch('/api/graph/match')
    ]);

    state.graphData = graphData;

    state.graph.activeMatches.clear();
    (matchingData.pairs || []).forEach(p => {
      state.graph.activeMatches.set(p.donor_id, p.recipient_id);
    });

    const select = document.getElementById('traversal-start-donor');
    if (select) {
      select.innerHTML = '';
      const donors = graphData.nodes.filter(n => n.type === 'donor');
      donors.forEach(d => {
        const opt = document.createElement('option');
        opt.value = d.id;
        opt.textContent = `${d.id} - ${d.label} (${d.organ}, ${d.blood})`;
        select.appendChild(opt);
      });
    }

    layoutBipartiteNodes();
    renderGraph();
  } catch (err) {
    console.error('Failed to load graph data:', err);
  }
}

function layoutBipartiteNodes() {
  if (!state.graphData || !state.graph.canvas) return;

  const rect = state.graph.canvas.getBoundingClientRect();
  const width = rect.width || 800;
  const height = rect.height || 520;

  const donorNodes = state.graphData.nodes.filter(n => n.type === 'donor');
  const recipientNodes = state.graphData.nodes.filter(n => n.type === 'recipient');

  const leftX = 140;
  const rightX = width - 140;

  const donorSpacing = Math.max(28, (height - 80) / (donorNodes.length || 1));
  donorNodes.forEach((node, i) => {
    node.x = leftX;
    node.y = 50 + i * donorSpacing;
    node.radius = 12;
  });

  const recipientSpacing = Math.max(28, (height - 80) / (recipientNodes.length || 1));
  recipientNodes.forEach((node, i) => {
    node.x = rightX;
    node.y = 50 + i * recipientSpacing;
    node.radius = 12;
  });
}

function renderGraph() {
  const g = state.graph;
  if (!g.ctx || !g.canvas || !state.graphData) return;

  const rect = g.canvas.getBoundingClientRect();
  const width = rect.width;
  const height = rect.height || 520;

  const ctx = g.ctx;
  ctx.save();
  ctx.clearRect(0, 0, width, height);

  // Background Grid pattern
  ctx.strokeStyle = '#131e3a';
  ctx.lineWidth = 1;
  const gridSize = 40;
  for (let x = 0; x < width; x += gridSize) {
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, height);
    ctx.stroke();
  }
  for (let y = 0; y < height; y += gridSize) {
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(width, y);
    ctx.stroke();
  }

  ctx.translate(g.panX, g.panY);
  ctx.scale(g.zoom, g.zoom);

  const nodeMap = new Map();
  state.graphData.nodes.forEach(n => nodeMap.set(n.id, n));

  const showAll = document.getElementById('chk-show-all-edges')?.checked ?? true;
  state.graphData.edges.forEach(edge => {
    const src = nodeMap.get(edge.source);
    const tgt = nodeMap.get(edge.target);
    if (!src || !tgt) return;

    const isMatched = g.activeMatches.get(edge.source) === edge.target;
    const isHovered = (g.hoveredNode && (g.hoveredNode.id === edge.source || g.hoveredNode.id === edge.target));

    if (!showAll && !isMatched && !isHovered) return;

    ctx.beginPath();
    ctx.moveTo(src.x, src.y);
    const cp1x = src.x + (tgt.x - src.x) * 0.45;
    const cp1y = src.y;
    const cp2x = src.x + (tgt.x - src.x) * 0.55;
    const cp2y = tgt.y;
    ctx.bezierCurveTo(cp1x, cp1y, cp2x, cp2y, tgt.x, tgt.y);

    if (isMatched) {
      ctx.strokeStyle = '#fbbf24';
      ctx.lineWidth = 2.8;
      ctx.shadowColor = 'rgba(251, 191, 36, 0.7)';
      ctx.shadowBlur = 8;
      ctx.stroke();
      ctx.shadowBlur = 0;
    } else if (isHovered) {
      ctx.strokeStyle = '#38bdf8';
      ctx.lineWidth = 2.0;
      ctx.shadowColor = 'rgba(56, 189, 248, 0.8)';
      ctx.shadowBlur = 6;
      ctx.stroke();
      ctx.shadowBlur = 0;
    } else {
      ctx.strokeStyle = 'rgba(56, 189, 248, 0.12)';
      ctx.lineWidth = 1.0;
      ctx.stroke();
    }
  });

  state.graphData.nodes.forEach(node => {
    const isHovered = g.hoveredNode && g.hoveredNode.id === node.id;
    const isTraversalActive = g.traversalNodes.includes(node.id);

    ctx.save();
    ctx.beginPath();
    ctx.arc(node.x, node.y, isHovered ? node.radius + 4 : node.radius, 0, Math.PI * 2);

    if (node.type === 'donor') {
      ctx.fillStyle = isHovered ? '#7dd3fc' : '#38bdf8';
      ctx.shadowColor = '#38bdf8';
    } else {
      if (node.urgency === 1) {
        ctx.fillStyle = isHovered ? '#fca5a5' : '#f87171';
        ctx.shadowColor = '#ef4444';
      } else {
        ctx.fillStyle = isHovered ? '#6ee7b7' : '#34d399';
        ctx.shadowColor = '#10b981';
      }
    }

    if (isTraversalActive) {
      ctx.fillStyle = '#f59e0b';
      ctx.shadowColor = '#f59e0b';
      ctx.shadowBlur = 16;
    } else {
      ctx.shadowBlur = isHovered ? 14 : 6;
    }

    ctx.fill();
    ctx.lineWidth = 2;
    ctx.strokeStyle = '#ffffff';
    ctx.stroke();
    ctx.restore();

    ctx.font = '11px -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto';
    ctx.textBaseline = 'middle';

    if (node.type === 'donor') {
      ctx.fillStyle = '#e2e8f0';
      ctx.textAlign = 'right';
      ctx.fillText(`${node.id} (${node.organ}, ${node.blood})`, node.x - 18, node.y);
    } else {
      ctx.fillStyle = '#e2e8f0';
      ctx.textAlign = 'left';
      const urgTag = node.urgency === 1 ? ' [Critical]' : '';
      ctx.fillText(`${node.id} (${node.organ}, ${node.blood})${urgTag}`, node.x + 18, node.y);
    }
  });

  if (g.hoveredNode) {
    drawNodeTooltip(ctx, g.hoveredNode);
  }

  ctx.restore();
}

function drawNodeTooltip(ctx, node) {
  const isDonor = node.type === 'donor';
  const lines = [
    `${node.id}: ${node.label}`,
    `Type: ${isDonor ? 'Organ Donor' : 'Waiting Patient'}`,
    `Blood: ${node.blood} • Organ: ${node.organ}`,
    `Location: ${node.location} • Age: ${node.age} yrs`
  ];
  if (!isDonor) {
    lines.push(`Urgency: ${node.urgency_str || ('Tier ' + node.urgency)} • Wait: ${node.waiting} days`);
  }

  const padding = 10;
  const lineHeight = 16;
  const boxWidth = 220;
  const boxHeight = lines.length * lineHeight + padding * 2;

  let tx = node.x + (isDonor ? 24 : -boxWidth - 24);
  let ty = node.y - boxHeight / 2;

  ctx.save();
  ctx.fillStyle = 'rgba(15, 23, 42, 0.94)';
  ctx.strokeStyle = '#38bdf8';
  ctx.lineWidth = 1.5;
  ctx.shadowColor = 'rgba(0,0,0,0.5)';
  ctx.shadowBlur = 10;

  ctx.beginPath();
  ctx.roundRect ? ctx.roundRect(tx, ty, boxWidth, boxHeight, 8) : ctx.rect(tx, ty, boxWidth, boxHeight);
  ctx.fill();
  ctx.stroke();

  ctx.shadowBlur = 0;
  ctx.fillStyle = '#f8fafc';
  ctx.textAlign = 'left';
  ctx.font = '11px -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto';

  lines.forEach((line, i) => {
    if (i === 0) {
      ctx.font = 'bold 12px sans-serif';
      ctx.fillStyle = '#38bdf8';
    } else {
      ctx.font = '11px sans-serif';
      ctx.fillStyle = '#e2e8f0';
    }
    ctx.fillText(line, tx + padding, ty + padding + (i + 0.8) * lineHeight);
  });

  ctx.restore();
}

function setupGraphInteractions() {
  const canvas = state.graph.canvas;
  if (!canvas) return;

  canvas.onmousedown = (e) => {
    state.graph.isDragging = true;
    state.graph.dragStartX = e.clientX - state.graph.panX;
    state.graph.dragStartY = e.clientY - state.graph.panY;
  };

  window.onmousemove = (e) => {
    if (state.graph.isDragging) {
      state.graph.panX = e.clientX - state.graph.dragStartX;
      state.graph.panY = e.clientY - state.graph.dragStartY;
      renderGraph();
      return;
    }

    if (state.graphData && state.currentView === 'graph') {
      const rect = canvas.getBoundingClientRect();
      const mouseX = (e.clientX - rect.left - state.graph.panX) / state.graph.zoom;
      const mouseY = (e.clientY - rect.top - state.graph.panY) / state.graph.zoom;

      let found = null;
      for (const n of state.graphData.nodes) {
        const dx = mouseX - n.x;
        const dy = mouseY - n.y;
        if (dx * dx + dy * dy <= (n.radius + 6) * (n.radius + 6)) {
          found = n;
          break;
        }
      }

      if (found !== state.graph.hoveredNode) {
        state.graph.hoveredNode = found;
        renderGraph();
      }
    }
  };

  window.onmouseup = () => {
    state.graph.isDragging = false;
  };

  canvas.onwheel = (e) => {
    e.preventDefault();
    const zoomFactor = e.deltaY < 0 ? 1.1 : 0.9;
    const newZoom = Math.min(Math.max(0.4, state.graph.zoom * zoomFactor), 3.0);
    state.graph.zoom = newZoom;
    renderGraph();
  };

  const chk = document.getElementById('chk-show-all-edges');
  if (chk) chk.onchange = () => renderGraph();

  const btnReset = document.getElementById('btn-reset-graph-view');
  if (btnReset) {
    btnReset.onclick = () => {
      state.graph.zoom = 1;
      state.graph.panX = 0;
      state.graph.panY = 0;
      renderGraph();
    };
  }

  document.getElementById('btn-run-bfs')?.addEventListener('click', () => runGraphTraversal('bfs'));
  document.getElementById('btn-run-dfs')?.addEventListener('click', () => runGraphTraversal('dfs'));
}

async function runGraphTraversal(type = 'bfs') {
  const startDonor = document.getElementById('traversal-start-donor')?.value || 'DON-1001';
  const consoleEl = document.getElementById('traversal-log');
  const badgeEl = document.getElementById('traversal-stats-badge');

  consoleEl.textContent = `Executing ${type.toUpperCase()} traversal on C backend starting from ${startDonor}...`;

  try {
    const data = await apiFetch(`/api/graph/${type}?start=${encodeURIComponent(startDonor)}`);
    consoleEl.textContent = data.log || '// Traversal returned empty log';

    if (badgeEl) {
      badgeEl.style.display = 'inline-block';
      badgeEl.textContent = `${type.toUpperCase()}: ${data.visited_donors} Donors, ${data.visited_recipients} Recipients Traversed`;
    }

    const visited = [];
    const lines = (data.log || '').split('\n');
    lines.forEach(line => {
      const dMatch = line.match(/(DON-\d{4})/g);
      const rMatch = line.match(/(REC-\d{4})/g);
      if (dMatch) dMatch.forEach(id => { if (!visited.includes(id)) visited.push(id); });
      if (rMatch) rMatch.forEach(id => { if (!visited.includes(id)) visited.push(id); });
    });

    animateTraversalNodes(visited);
  } catch (err) {
    consoleEl.textContent = `// Traversal failed: ${err.message}`;
  }
}

function animateTraversalNodes(nodeIds) {
  if (state.graph.animationTimer) clearInterval(state.graph.animationTimer);
  state.graph.traversalNodes = [];
  let idx = 0;

  state.graph.animationTimer = setInterval(() => {
    if (idx < nodeIds.length) {
      state.graph.traversalNodes.push(nodeIds[idx]);
      idx++;
      renderGraph();
    } else {
      clearInterval(state.graph.animationTimer);
      state.graph.animationTimer = null;
      setTimeout(() => {
        state.graph.traversalNodes = [];
        renderGraph();
      }, 4000);
    }
  }, 350);
}

// ============================================================================
// MATCHING ENGINE & COMPARISON
// ============================================================================

async function loadMatchingView() {
  const algo = document.getElementById('matching-algo-select')?.value || 'graph';
  await executeMatching(algo);
}

async function executeMatching(algoType = 'graph') {
  const endpoint = algoType === 'traditional' ? '/api/traditional/match' : '/api/graph/match';
  try {
    const res = await apiFetch(endpoint);
    state.matchingResult = res;

    document.getElementById('match-kpi-pairs').textContent = res.pair_count || 0;
    document.getElementById('match-kpi-score').textContent = `${(res.avg_score || 0).toFixed(1)}%`;
    document.getElementById('match-kpi-critical').textContent = `${res.critical_matched || 0} / ${res.critical_total || 0}`;
    document.getElementById('match-kpi-time').textContent = `${(res.execution_time_us || 0).toFixed(1)} μs`;

    document.getElementById('matching-table-title').textContent = `${res.algorithm || 'Matching'} Results (${res.pair_count} Pairs)`;

    const tbody = document.getElementById('matching-results-tbody');
    tbody.innerHTML = '';
    const pairs = res.pairs || [];

    if (pairs.length === 0) {
      tbody.innerHTML = '<tr><td colspan="10" style="text-align:center; padding: 24px;">No pairs allocated</td></tr>';
      return;
    }

    pairs.forEach((p, index) => {
      const tr = document.createElement('tr');
      tr.style.cursor = 'pointer';
      tr.onclick = () => openMatchDetailModal(p);
      tr.innerHTML = `
        <td style="font-weight:700; color:#64748b;">#${index + 1}</td>
        <td>
          <strong>${p.donor_name}</strong>
          <div style="font-size:11px; font-family:monospace; color:#0284c7;">${p.donor_id}</div>
        </td>
        <td>
          <span class="badge badge-blood">${p.donor_bg}</span>
          <span style="font-size:12px; color:#64748b; margin-left:4px;">${p.donor_age}y</span>
        </td>
        <td>
          <strong>${p.recipient_name}</strong>
          <div style="font-size:11px; font-family:monospace; color:#059669;">${p.recipient_id}</div>
        </td>
        <td>
          <span class="badge badge-blood">${p.recipient_bg}</span>
          <span style="font-size:12px; color:#64748b; margin-left:4px;">${p.recipient_age}y</span>
        </td>
        <td><span class="badge badge-organ">${p.organ}</span></td>
        <td><span class="badge badge-${getUrgencyClass(p.urgency_level)}">${p.urgency_str}</span></td>
        <td>${p.waiting_time} d</td>
        <td><span class="badge badge-score ${getScoreClass(p.compatibility_score)}">${p.compatibility_score.toFixed(1)}%</span></td>
        <td style="max-width: 280px; font-size:12px; color:#475569; overflow:hidden; text-overflow:ellipsis; white-space:nowrap;" title="${p.explanation}">
          ${p.explanation}
        </td>
      `;
      tbody.appendChild(tr);
    });
  } catch (err) {
    console.error('Matching execution failed:', err);
  }
}

function openMatchDetailModal(pair) {
  document.getElementById('modal-match-title').textContent = `Clinical Allocation: ${pair.donor_id} ➔ ${pair.recipient_id}`;
  const body = document.getElementById('modal-match-body');
  
  body.innerHTML = `
    <div style="display:grid; grid-template-columns: 1fr 1fr; gap: 16px; margin-bottom: 20px;">
      <div style="background:#f0f9ff; border:1px solid #bae6fd; border-radius:10px; padding:14px;">
        <div style="font-size:11px; font-weight:700; color:#0284c7; text-transform:uppercase;">Donor Profile</div>
        <div style="font-size:16px; font-weight:800; color:#0f172a; margin-top:4px;">${pair.donor_name}</div>
        <div style="font-size:12px; color:#475569; margin-top:4px;">
          <strong>ID:</strong> ${pair.donor_id}<br>
          <strong>Blood:</strong> ${pair.donor_bg} • <strong>Age:</strong> ${pair.donor_age} yrs<br>
          <strong>Location:</strong> ${pair.donor_location}<br>
          <strong>Organ:</strong> ${pair.organ}
        </div>
      </div>

      <div style="background:#ecfdf5; border:1px solid #a7f3d0; border-radius:10px; padding:14px;">
        <div style="font-size:11px; font-weight:700; color:#059669; text-transform:uppercase;">Recipient Profile</div>
        <div style="font-size:16px; font-weight:800; color:#0f172a; margin-top:4px;">${pair.recipient_name}</div>
        <div style="font-size:12px; color:#475569; margin-top:4px;">
          <strong>ID:</strong> ${pair.recipient_id}<br>
          <strong>Blood:</strong> ${pair.recipient_bg} • <strong>Age:</strong> ${pair.recipient_age} yrs<br>
          <strong>Location:</strong> ${pair.recipient_location}<br>
          <strong>Urgency:</strong> ${pair.urgency_str} (Tier ${pair.urgency_level})<br>
          <strong>Waitlist:</strong> ${pair.waiting_time} days
        </div>
      </div>
    </div>

    <div style="margin-bottom: 16px;">
      <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:8px;">
        <span style="font-weight:700; font-size:13px; color:#1e293b;">Overall Compatibility Score:</span>
        <span class="badge badge-score ${getScoreClass(pair.compatibility_score)}" style="font-size:14px; padding:6px 12px;">
          ${pair.compatibility_score.toFixed(2)} / 100.0 pts
        </span>
      </div>
      <div style="height:8px; background:#e2e8f0; border-radius:4px; overflow:hidden;">
        <div style="height:100%; width:${pair.compatibility_score}%; background:linear-gradient(90deg, #0284c7, #10b981);"></div>
      </div>
    </div>

    <div style="background:#f8fafc; border:1px solid #e2e8f0; border-radius:10px; padding:14px;">
      <div style="font-weight:700; font-size:13px; color:#1e293b; margin-bottom:8px;">Clinical Scoring Rationale:</div>
      <p style="font-size:13px; line-height:1.7; color:#334155; margin:0;">
        ${pair.explanation}
      </p>
    </div>
  `;

  openModal('modal-match-detail');
}

async function loadComparisonView() {
  try {
    const comp = await apiFetch('/api/compare');
    state.comparisonData = comp;
    const sum = comp.summary || {};

    document.getElementById('comp-graph-score').textContent = `${(sum.graph_avg_score || 0).toFixed(1)}%`;
    document.getElementById('comp-graph-pairs').textContent = sum.graph_pairs || 0;
    document.getElementById('comp-graph-critical').textContent = `${sum.graph_critical_matched || 0} / ${sum.total_critical_recipients || 0} (100%)`;
    document.getElementById('comp-graph-total-score').textContent = (sum.graph_total_score || 0).toFixed(1);
    document.getElementById('comp-graph-comps').textContent = sum.graph_comparisons || 0;
    document.getElementById('comp-graph-time').textContent = `${(sum.graph_time_us || 0).toFixed(1)} μs`;

    document.getElementById('comp-trad-score').textContent = `${(sum.traditional_avg_score || 0).toFixed(1)}%`;
    document.getElementById('comp-trad-pairs').textContent = sum.traditional_pairs || 0;
    document.getElementById('comp-trad-critical').textContent = `${sum.traditional_critical_matched || 0} / ${sum.total_critical_recipients || 0}`;
    document.getElementById('comp-trad-total-score').textContent = (sum.traditional_total_score || 0).toFixed(1);
    document.getElementById('comp-trad-comps').textContent = sum.traditional_comparisons || 0;
    document.getElementById('comp-trad-time').textContent = `${(sum.traditional_time_us || 0).toFixed(1)} μs`;

    renderComparisonCharts(sum);
  } catch (err) {
    console.error('Failed to load comparison view:', err);
  }
}

function renderComparisonCharts(sum) {
  drawBarChart('chart-scores', {
    labels: ['Avg Compatibility (%)', 'Critical Matched'],
    datasets: [
      { label: 'Graph Algorithm', values: [sum.graph_avg_score || 0, sum.graph_critical_matched || 0], color: '#0284c7' },
      { label: 'Traditional FCFS', values: [sum.traditional_avg_score || 0, sum.traditional_critical_matched || 0], color: '#94a3b8' }
    ],
    yMax: 100
  });

  drawBarChart('chart-time', {
    labels: ['Comparisons Done', 'Time (μs)'],
    datasets: [
      { label: 'Graph Algorithm', values: [sum.graph_comparisons || 0, sum.graph_time_us || 0], color: '#10b981' },
      { label: 'Traditional FCFS', values: [sum.traditional_comparisons || 0, sum.traditional_time_us || 0], color: '#f59e0b' }
    ]
  });
}

async function loadBenchmarkView() {
  if (state.benchmarkHistory.length === 0) {
    await runBenchmark(50);
  } else {
    renderBenchmarkHistory();
  }
}

async function runBenchmark(size) {
  const targetSize = size || parseInt(document.getElementById('benchmark-size-select')?.value || '50', 10);
  const btn = document.getElementById('btn-run-benchmark');
  if (btn) btn.disabled = true;

  showToast(`Running benchmark on N = ${targetSize} cohort...`, 'info');

  try {
    const data = await apiFetch(`/api/benchmark?size=${targetSize}`);
    state.benchmarkHistory = state.benchmarkHistory.filter(h => h.benchmark_size !== data.benchmark_size);
    state.benchmarkHistory.push(data);
    state.benchmarkHistory.sort((a, b) => a.benchmark_size - b.benchmark_size);

    renderBenchmarkHistory();
    showToast(`Benchmark N = ${targetSize} completed!`, 'success');
  } catch (err) {
    console.error('Benchmark failed:', err);
  } finally {
    if (btn) btn.disabled = false;
  }
}

function renderBenchmarkHistory() {
  const tbody = document.getElementById('benchmark-results-tbody');
  if (!tbody) return;
  tbody.innerHTML = '';

  state.benchmarkHistory.forEach(item => {
    const trGraph = document.createElement('tr');
    trGraph.innerHTML = `
      <td rowspan="2" style="font-weight:800; font-size:14px; color:#0f172a; vertical-align:middle; border-right:1px solid #e2e8f0;">
        N = ${item.benchmark_size}
      </td>
      <td><span class="badge badge-score" style="background:#0284c7;">Graph-Based</span></td>
      <td><strong>${item.graph_pairs}</strong> pairs</td>
      <td><span class="badge badge-score high">${item.graph_avg_score.toFixed(1)}%</span></td>
      <td>${item.graph_comparisons.toLocaleString()}</td>
      <td style="font-family:monospace; font-weight:700; color:#0284c7;">${item.graph_time_us.toFixed(1)} μs</td>
    `;

    const trTrad = document.createElement('tr');
    trTrad.style.borderBottom = '2px solid #e2e8f0';
    trTrad.innerHTML = `
      <td><span class="badge" style="background:#f1f5f9; color:#475569;">Traditional FCFS</span></td>
      <td><strong>${item.traditional_pairs}</strong> pairs</td>
      <td><span class="badge badge-score">${item.traditional_avg_score.toFixed(1)}%</span></td>
      <td>${item.traditional_comparisons.toLocaleString()}</td>
      <td style="font-family:monospace; font-weight:700; color:#64748b;">${item.traditional_time_us.toFixed(1)} μs</td>
    `;

    tbody.appendChild(trGraph);
    tbody.appendChild(trTrad);
  });

  renderBenchmarkChart();
}

function renderBenchmarkChart() {
  const canvas = document.getElementById('chart-benchmark');
  if (!canvas || state.benchmarkHistory.length === 0) return;

  const labels = state.benchmarkHistory.map(h => `N=${h.benchmark_size}`);
  const graphTimes = state.benchmarkHistory.map(h => h.graph_time_us);
  const tradTimes = state.benchmarkHistory.map(h => h.traditional_time_us);

  drawLineChart('chart-benchmark', {
    labels,
    datasets: [
      { label: 'Graph Algorithm Time (μs)', values: graphTimes, color: '#0284c7' },
      { label: 'Traditional FCFS Time (μs)', values: tradTimes, color: '#f59e0b' }
    ]
  });
}

// ============================================================================
// Canvas Chart Engine (Zero CDN Dependency)
// ============================================================================

function drawBarChart(canvasId, config) {
  const canvas = document.getElementById(canvasId);
  if (!canvas) return;

  const ctx = canvas.getContext('2d');
  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();
  canvas.width = rect.width * dpr;
  canvas.height = (rect.height || 260) * dpr;
  ctx.scale(dpr, dpr);

  const w = rect.width;
  const h = rect.height || 260;
  ctx.clearRect(0, 0, w, h);

  const paddingLeft = 50;
  const paddingRight = 20;
  const paddingTop = 30;
  const paddingBottom = 40;
  const chartW = w - paddingLeft - paddingRight;
  const chartH = h - paddingTop - paddingBottom;

  let maxVal = config.yMax || 0;
  config.datasets.forEach(ds => {
    ds.values.forEach(v => { if (v > maxVal) maxVal = v; });
  });
  if (maxVal === 0) maxVal = 100;
  maxVal = Math.ceil(maxVal * 1.15);

  ctx.strokeStyle = '#f1f5f9';
  ctx.lineWidth = 1;
  ctx.fillStyle = '#94a3b8';
  ctx.font = '10px sans-serif';
  ctx.textAlign = 'right';

  const gridSteps = 4;
  for (let i = 0; i <= gridSteps; i++) {
    const yVal = Math.round((maxVal / gridSteps) * i);
    const yPos = paddingTop + chartH - (chartH / gridSteps) * i;
    ctx.beginPath();
    ctx.moveTo(paddingLeft, yPos);
    ctx.lineTo(w - paddingRight, yPos);
    ctx.stroke();
    ctx.fillText(String(yVal), paddingLeft - 8, yPos + 3);
  }

  const numGroups = config.labels.length;
  const groupWidth = chartW / numGroups;
  const numBars = config.datasets.length;
  const barWidth = Math.min(36, (groupWidth * 0.7) / numBars);
  const gap = 4;

  config.labels.forEach((label, gIdx) => {
    const groupCenterX = paddingLeft + gIdx * groupWidth + groupWidth / 2;
    const groupStartX = groupCenterX - ((numBars * barWidth + (numBars - 1) * gap) / 2);

    config.datasets.forEach((ds, dIdx) => {
      const val = ds.values[gIdx] || 0;
      const barH = (val / maxVal) * chartH;
      const bx = groupStartX + dIdx * (barWidth + gap);
      const by = paddingTop + chartH - barH;

      ctx.fillStyle = ds.color;
      ctx.beginPath();
      ctx.roundRect ? ctx.roundRect(bx, by, barWidth, barH, [4, 4, 0, 0]) : ctx.rect(bx, by, barWidth, barH);
      ctx.fill();

      ctx.fillStyle = '#1e293b';
      ctx.font = 'bold 10px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText(typeof val === 'number' && val % 1 !== 0 ? val.toFixed(1) : String(val), bx + barWidth / 2, by - 4);
    });

    ctx.fillStyle = '#475569';
    ctx.font = '11px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText(label, groupCenterX, h - 14);
  });

  let legendX = w - paddingRight;
  config.datasets.slice().reverse().forEach(ds => {
    ctx.font = '11px sans-serif';
    ctx.textAlign = 'right';
    ctx.fillStyle = '#475569';
    ctx.fillText(ds.label, legendX, 16);
    const textWidth = ctx.measureText(ds.label).width;

    ctx.fillStyle = ds.color;
    ctx.beginPath();
    ctx.arc(legendX - textWidth - 8, 12, 4, 0, Math.PI * 2);
    ctx.fill();

    legendX -= textWidth + 24;
  });
}

function drawLineChart(canvasId, config) {
  const canvas = document.getElementById(canvasId);
  if (!canvas) return;

  const ctx = canvas.getContext('2d');
  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();
  canvas.width = rect.width * dpr;
  canvas.height = (rect.height || 300) * dpr;
  ctx.scale(dpr, dpr);

  const w = rect.width;
  const h = rect.height || 300;
  ctx.clearRect(0, 0, w, h);

  const paddingLeft = 60;
  const paddingRight = 30;
  const paddingTop = 40;
  const paddingBottom = 40;
  const chartW = w - paddingLeft - paddingRight;
  const chartH = h - paddingTop - paddingBottom;

  let maxVal = 0;
  config.datasets.forEach(ds => {
    ds.values.forEach(v => { if (v > maxVal) maxVal = v; });
  });
  if (maxVal === 0) maxVal = 100;
  maxVal = Math.ceil(maxVal * 1.15);

  ctx.strokeStyle = '#f1f5f9';
  ctx.lineWidth = 1;
  ctx.fillStyle = '#94a3b8';
  ctx.font = '10px sans-serif';
  ctx.textAlign = 'right';

  const gridSteps = 4;
  for (let i = 0; i <= gridSteps; i++) {
    const yVal = Math.round((maxVal / gridSteps) * i);
    const yPos = paddingTop + chartH - (chartH / gridSteps) * i;
    ctx.beginPath();
    ctx.moveTo(paddingLeft, yPos);
    ctx.lineTo(w - paddingRight, yPos);
    ctx.stroke();
    ctx.fillText(String(yVal), paddingLeft - 8, yPos + 3);
  }

  const numPoints = config.labels.length;
  const stepX = numPoints > 1 ? chartW / (numPoints - 1) : chartW;

  config.datasets.forEach(ds => {
    ctx.strokeStyle = ds.color;
    ctx.lineWidth = 2.5;
    ctx.beginPath();

    const points = [];
    ds.values.forEach((val, i) => {
      const px = paddingLeft + i * stepX;
      const py = paddingTop + chartH - (val / maxVal) * chartH;
      points.push({ x: px, y: py, val });
      if (i === 0) ctx.moveTo(px, py);
      else ctx.lineTo(px, py);
    });
    ctx.stroke();

    if (points.length > 0) {
      ctx.lineTo(points[points.length - 1].x, paddingTop + chartH);
      ctx.lineTo(points[0].x, paddingTop + chartH);
      ctx.closePath();
      ctx.fillStyle = ds.color + '15';
      ctx.fill();
    }

    points.forEach(p => {
      ctx.fillStyle = '#ffffff';
      ctx.strokeStyle = ds.color;
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(p.x, p.y, 4, 0, Math.PI * 2);
      ctx.fill();
      ctx.stroke();

      ctx.fillStyle = '#1e293b';
      ctx.font = 'bold 10px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText(`${p.val.toFixed(0)} μs`, p.x, p.y - 8);
    });
  });

  config.labels.forEach((label, i) => {
    const px = paddingLeft + i * stepX;
    ctx.fillStyle = '#475569';
    ctx.font = '11px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText(label, px, h - 14);
  });

  let legendX = w - paddingRight;
  config.datasets.slice().reverse().forEach(ds => {
    ctx.font = '11px sans-serif';
    ctx.textAlign = 'right';
    ctx.fillStyle = '#475569';
    ctx.fillText(ds.label, legendX, 16);
    const textWidth = ctx.measureText(ds.label).width;

    ctx.fillStyle = ds.color;
    ctx.beginPath();
    ctx.arc(legendX - textWidth - 8, 12, 4, 0, Math.PI * 2);
    ctx.fill();

    legendX -= textWidth + 24;
  });
}

// ============================================================================
// Modals & Forms Handlers
// ============================================================================

function openModal(modalId) {
  const modal = document.getElementById(modalId);
  if (modal) modal.classList.add('active');
}

function closeModal(modalId) {
  const modal = document.getElementById(modalId);
  if (modal) modal.classList.remove('active');
}

function openLoginModal(defaultRole = 'donor') {
  document.getElementById('form-login').reset();
  document.getElementById('login-error-msg').style.display = 'none';

  // Set active role button
  document.querySelectorAll('#login-role-selector .role-option').forEach(opt => {
    opt.classList.toggle('active', opt.dataset.role === defaultRole);
  });
  document.getElementById('login-admin-notice').style.display = (defaultRole === 'admin') ? 'block' : 'none';

  if (defaultRole === 'admin') {
    document.getElementById('login-email').value = 'saisriramchakka@gmail.com';
  }

  openModal('modal-login');
}

function openRegisterModal(defaultRole = 'donor') {
  document.getElementById('form-register').reset();
  document.getElementById('reg-error-msg').style.display = 'none';

  setRegisterRole(defaultRole);
  openModal('modal-register');
}

function setRegisterRole(role) {
  document.querySelectorAll('#reg-role-selector .role-option').forEach(opt => {
    opt.classList.toggle('active', opt.dataset.role === role);
  });

  const isDonor = (role === 'donor');
  document.querySelectorAll('.donor-only-field').forEach(el => el.style.display = isDonor ? 'block' : 'none');
  document.querySelectorAll('.recipient-only-field').forEach(el => el.style.display = isDonor ? 'none' : 'block');
  document.getElementById('modal-register-title').textContent = isDonor ? 'Register as Organ Donor' : 'Register as Recipient Patient';
}

async function handleLoginSubmit(e) {
  e.preventDefault();
  const email = document.getElementById('login-email').value.trim();
  const password = document.getElementById('login-password').value;
  const activeRoleOpt = document.querySelector('#login-role-selector .role-option.active');
  const role = activeRoleOpt ? activeRoleOpt.dataset.role : 'donor';
  const errDiv = document.getElementById('login-error-msg');

  errDiv.style.display = 'none';

  try {
    const res = await apiFetch('/api/auth/login', {
      method: 'POST',
      body: JSON.stringify({ email, password, role })
    });

    state.token = res.token;
    state.currentUser = res.user;
    state.userProfile = res.profile;
    localStorage.setItem('organ_auth_token', res.token);

    closeModal('modal-login');
    updateAuthHeaderUI();
    updateRoleNavUI();
    showToast(res.message || 'Login successful!', 'success');
    goToMyDashboard();
  } catch (err) {
    errDiv.textContent = err.message;
    errDiv.style.display = 'block';
  }
}

async function handleRegisterSubmit(e) {
  e.preventDefault();
  const activeRoleOpt = document.querySelector('#reg-role-selector .role-option.active');
  const role = activeRoleOpt ? activeRoleOpt.dataset.role : 'donor';
  const errDiv = document.getElementById('reg-error-msg');

  const password = document.getElementById('reg-password').value;
  const confirmPassword = document.getElementById('reg-confirm-password').value;

  if (password !== confirmPassword) {
    errDiv.textContent = 'Passwords do not match.';
    errDiv.style.display = 'block';
    return;
  }

  const payload = {
    role,
    name: document.getElementById('reg-name').value.trim(),
    email: document.getElementById('reg-email').value.trim(),
    password,
    confirm_password: confirmPassword,
    age: parseInt(document.getElementById('reg-age').value, 10),
    gender: document.getElementById('reg-gender').value,
    phone: document.getElementById('reg-phone').value.trim(),
    blood_group: document.getElementById('reg-bg').value,
    city: document.getElementById('reg-city').value.trim(),
    state: document.getElementById('reg-state').value.trim(),
    address: document.getElementById('reg-address').value.trim(),
    security_question: document.getElementById('reg-security-question').value,
    security_answer: document.getElementById('reg-security-answer').value.trim()
  };

  if (role === 'donor') {
    payload.organ_willing_to_donate = document.getElementById('reg-donor-organ').value;
    payload.availability_status = document.getElementById('reg-donor-avail').value;
    payload.medical_history = document.getElementById('reg-donor-med-hist').value;
    payload.health_condition = document.getElementById('reg-donor-health').value;
    payload.emergency_contact = document.getElementById('reg-donor-emergency').value;
  } else {
    payload.required_organ = document.getElementById('reg-rec-organ').value;
    payload.urgency_level = parseInt(document.getElementById('reg-rec-urgency').value, 10);
    payload.medical_condition = document.getElementById('reg-rec-condition').value;
    payload.hospital_name = document.getElementById('reg-rec-hospital').value;
    payload.doctor_details = document.getElementById('reg-rec-doctor').value;
  }

  errDiv.style.display = 'none';

  try {
    const res = await apiFetch('/api/auth/register', {
      method: 'POST',
      body: JSON.stringify(payload)
    });

    state.token = res.token;
    state.currentUser = res.user;
    localStorage.setItem('organ_auth_token', res.token);

    closeModal('modal-register');
    updateAuthHeaderUI();
    updateRoleNavUI();
    showToast(res.message, 'success');
    goToMyDashboard();
  } catch (err) {
    errDiv.textContent = err.message;
    errDiv.style.display = 'block';
  }
}

// Helpers for badges and classes
function getUrgencyClass(level) {
  if (level === 1) return 'critical';
  if (level === 2) return 'urgent';
  return 'standard';
}

function getUrgencyLabel(level) {
  if (level === 1) return 'Critical (Tier 1)';
  if (level === 2) return 'Urgent (Tier 2)';
  return 'Standard (Tier 3)';
}

function getScoreClass(score) {
  if (score >= 85) return 'high';
  if (score >= 70) return 'medium';
  return 'low';
}

function getStatusBadgeClass(status) {
  const s = (status || '').toUpperCase();
  if (s === 'AVAILABLE') return 'badge-status-available';
  if (s === 'MATCHED') return 'badge-status-matched';
  if (s === 'DONATION IN PROGRESS' || s === 'APPROVED' || s === 'IN_PROGRESS') return 'badge-status-progress';
  if (s === 'DONATION COMPLETED' || s === 'COMPLETED' || s === 'TRANSPLANT COMPLETED') return 'badge-status-completed';
  if (s === 'UNAVAILABLE' || s === 'REJECTED') return 'badge-status-unavailable';
  return 'badge-standard';
}

// ============================================================================
// Global Initialization
// ============================================================================

document.addEventListener('DOMContentLoaded', async () => {
  // 1. Check existing Auth session
  await checkAuthSession();

  // 2. Navigation items
  document.querySelectorAll('.nav-item').forEach(item => {
    item.addEventListener('click', (e) => {
      e.preventDefault();
      const view = item.dataset.view;
      if (view) switchView(view);
    });
  });

  // 3. Admin sub tabs
  document.querySelectorAll('.sub-tab').forEach(t => {
    t.addEventListener('click', () => {
      switchAdminTab(t.dataset.adminTab);
    });
  });

  // 4. Modal Triggers & Form Submissions
  document.getElementById('form-login')?.addEventListener('submit', handleLoginSubmit);
  document.getElementById('form-register')?.addEventListener('submit', handleRegisterSubmit);
  document.getElementById('form-complete-donation')?.addEventListener('submit', handleCompleteDonationSubmit);

  // Role selectors in login & reg
  document.querySelectorAll('#login-role-selector .role-option').forEach(opt => {
    opt.addEventListener('click', () => {
      document.querySelectorAll('#login-role-selector .role-option').forEach(o => o.classList.remove('active'));
      opt.classList.add('active');
      const isAdmin = (opt.dataset.role === 'admin');
      document.getElementById('login-admin-notice').style.display = isAdmin ? 'block' : 'none';
      if (isAdmin) document.getElementById('login-email').value = 'saisriramchakka@gmail.com';
    });
  });

  document.querySelectorAll('#reg-role-selector .role-option').forEach(opt => {
    opt.addEventListener('click', () => setRegisterRole(opt.dataset.role));
  });

  document.getElementById('btn-goto-register')?.addEventListener('click', () => {
    closeModal('modal-login');
    openRegisterModal('donor');
  });

  // Modal close buttons
  document.querySelectorAll('[data-modal-close]').forEach(btn => {
    btn.addEventListener('click', () => closeModal(btn.dataset.modalClose));
  });

  document.querySelectorAll('.modal-overlay').forEach(overlay => {
    overlay.addEventListener('click', (e) => {
      if (e.target === overlay) overlay.classList.remove('active');
    });
  });

  // Forgot password flow
  document.getElementById('link-forgot-password')?.addEventListener('click', (e) => {
    e.preventDefault();
    closeModal('modal-login');
    document.getElementById('form-forgot-password').reset();
    document.getElementById('fp-step-2').style.display = 'none';
    document.getElementById('btn-fp-submit').style.display = 'none';
    document.getElementById('fp-status-msg').textContent = '';
    openModal('modal-forgot-password');
  });

  document.getElementById('btn-fp-lookup')?.addEventListener('click', async () => {
    const email = document.getElementById('fp-email').value.trim();
    const msg = document.getElementById('fp-status-msg');
    if (!email) {
      msg.textContent = 'Please enter your registered email.';
      msg.style.color = '#ef4444';
      return;
    }
    try {
      const res = await apiFetch('/api/auth/forgot-password/question', {
        method: 'POST',
        body: JSON.stringify({ email })
      });
      document.getElementById('fp-question-label').textContent = res.security_question;
      document.getElementById('fp-step-2').style.display = 'block';
      document.getElementById('btn-fp-submit').style.display = 'inline-block';
      msg.textContent = 'Security question retrieved.';
      msg.style.color = '#10b981';
    } catch (err) {
      msg.textContent = err.message;
      msg.style.color = '#ef4444';
    }
  });

  document.getElementById('form-forgot-password')?.addEventListener('submit', async (e) => {
    e.preventDefault();
    const email = document.getElementById('fp-email').value.trim();
    const security_answer = document.getElementById('fp-answer').value.trim();
    const new_password = document.getElementById('fp-new-pwd').value;
    const msg = document.getElementById('fp-status-msg');

    try {
      await apiFetch('/api/auth/forgot-password/reset', {
        method: 'POST',
        body: JSON.stringify({ email, security_answer, new_password })
      });
      closeModal('modal-forgot-password');
      showToast('Password reset successfully. You can now sign in.', 'success');
      openLoginModal();
    } catch (err) {
      msg.textContent = err.message;
      msg.style.color = '#ef4444';
    }
  });

  // Admin sync matches button
  document.getElementById('btn-admin-sync-matches')?.addEventListener('click', async () => {
    try {
      const res = await apiFetch('/api/admin/matches/sync', { method: 'POST' });
      showToast(`Graph matching executed. ${res.synced_pairs} proposed pairs synced.`, 'success');
      loadAdminMatches();
      loadAdminDashboard();
    } catch (err) {
      showToast(err.message, 'error');
    }
  });

  // Donor toggle status in Donor Dashboard
  document.getElementById('btn-donor-toggle-avail')?.addEventListener('click', async () => {
    const curr = state.userProfile ? state.userProfile.status : 'AVAILABLE';
    const nextStatus = (curr === 'AVAILABLE') ? 'UNAVAILABLE' : 'AVAILABLE';
    try {
      await apiFetch('/api/donor/availability', {
        method: 'PUT',
        body: JSON.stringify({ status: nextStatus })
      });
      showToast(`Donor status updated to ${nextStatus}`, 'success');
      loadDonorDashboard();
    } catch (err) {
      showToast(err.message, 'error');
    }
  });

  // Matching and Benchmark triggers
  document.getElementById('btn-run-matching')?.addEventListener('click', () => {
    const algo = document.getElementById('matching-algo-select')?.value || 'graph';
    executeMatching(algo);
  });

  document.getElementById('btn-run-benchmark')?.addEventListener('click', () => {
    const size = parseInt(document.getElementById('benchmark-size-select')?.value || '50', 10);
    runBenchmark(size);
  });

  document.getElementById('btn-reset-data')?.addEventListener('click', async () => {
    if (!confirm('Reset data back to original sample dataset?')) return;
    try {
      await apiFetch('/api/reset', { method: 'POST' });
      showToast('Sample dataset restored successfully!', 'success');
      switchView(state.currentView);
    } catch (e) {}
  });

  // Admin filter inputs
  document.getElementById('admin-donor-search')?.addEventListener('input', loadAdminDonors);
  document.getElementById('admin-donor-filter-status')?.addEventListener('change', loadAdminDonors);
  document.getElementById('admin-donor-filter-organ')?.addEventListener('change', loadAdminDonors);

  document.getElementById('admin-recipient-search')?.addEventListener('input', loadAdminRecipients);
  document.getElementById('admin-recipient-filter-urgency')?.addEventListener('change', loadAdminRecipients);
  document.getElementById('admin-recipient-filter-organ')?.addEventListener('change', loadAdminRecipients);

  // Initial load
  switchView('dashboard');
});
