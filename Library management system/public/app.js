let currentUser = null;
let appState = null;
let activeTab = 'overview';
let pollTimer = null;

const $ = (id) => document.getElementById(id);

function escapeHtml(str) {
  return String(str ?? '')
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function showToast(msg, isError = false) {
  const el = $('toast');
  el.textContent = msg;
  el.classList.remove('hidden', 'error');
  if (isError) el.classList.add('error');
  clearTimeout(el._t);
  el._t = setTimeout(() => el.classList.add('hidden'), 5000);
}

async function apiPost(url, params = {}) {
  const headers = {};
  if (currentUser) headers['X-User-Id'] = currentUser.userId;
  const res = await fetch(url, {
    method: 'POST',
    headers,
    body: new URLSearchParams(params)
  });
  return await res.json();
}

async function loadState(silent = false) {
  if (!currentUser) return;
  try {
    const res = await fetch('/api/state', {
      headers: { 'X-User-Id': currentUser.userId }
    });
    if (!res.ok) return;
    appState = await res.json();
    renderAll();
  } catch (err) {
    if (!silent) showToast('Khong the ket noi den server!', true);
  }
}

function applyRoleVisibility() {
  const role = currentUser?.role || 'Member';
  const isStaff = role === 'Librarian' || role === 'Administrator';
  const isAdmin = role === 'Administrator';

  document.querySelectorAll('.staff-only').forEach((el) => {
    el.classList.toggle('hidden', !isStaff);
  });
  document.querySelectorAll('.admin-only').forEach((el) => {
    el.classList.toggle('hidden', !isAdmin);
  });
  document.querySelectorAll('.member-only').forEach((el) => {
    el.classList.toggle('hidden', isStaff);
  });

  const optLib = $('opt-role-librarian');
  if (optLib) optLib.disabled = !isAdmin;
}

function switchTab(tab) {
  activeTab = tab;
  document.querySelectorAll('.nav-btn').forEach((b) => {
    b.classList.toggle('active', b.dataset.tab === tab);
  });
  document.querySelectorAll('.tab-panel').forEach((p) => {
    p.classList.toggle('hidden', p.id !== `tab-${tab}`);
  });
  const names = {
    overview: 'Overview',
    catalogue: 'Catalogue',
    circulation: 'Circulation',
    members: 'Members',
    reports: 'Reports',
    profile: 'Profile (Thong tin ca nhan)'
  };
  $('breadcrumb').textContent = `Diamond Library > ${names[tab] || tab}`;
}

function statusBadge(status) {
  const map = {
    Active: 'badge-warn',
    Returned: 'badge-ok',
    Overdue: 'badge-danger',
    Paid: 'badge-ok',
    Unpaid: 'badge-danger',
    Available: 'badge-ok',
    Borrowed: 'badge-warn',
    Reserved: 'badge-info',
    Pending: 'badge-warn',
    Ready: 'badge-info',
    Fulfilled: 'badge-ok',
    Cancelled: 'badge-muted',
    Expired: 'badge-danger'
  };
  return `<span class="badge ${map[status] || 'badge-muted'}">${escapeHtml(status)}</span>`;
}

function renderAll() {
  if (!appState || !currentUser) return;

  $('server-clock').textContent = appState.serverDateTime || appState.serverDate || '--';
  if (!$('cir-date').value && appState.serverDate) {
    $('cir-date').value = appState.serverDate;
  }

  const isStaff = currentUser.role === 'Librarian' || currentUser.role === 'Administrator';

  // 1. OVERVIEW KPIs (dong bo dung theo role Member vs Staff)
  const activeLoans = (appState.loans || []).filter((l) => l.status !== 'Returned').length;
  const availCopies = (appState.bookCopies || []).filter((c) => c.copyStatus === 'Available').length;
  const overdueLoans = (appState.loans || []).filter((l) => {
    if (l.status === 'Returned') return false;
    if (l.status === 'Overdue') return true;
    return appState.serverDate && l.dueDate < appState.serverDate;
  }).length;

  $('kpi-label-loans').textContent = isStaff ? 'Loans active' : 'My active loans';
  $('kpi-loans').textContent = activeLoans;
  $('kpi-avail').textContent = availCopies;
  $('kpi-overdue').textContent = overdueLoans;

  if (isStaff) {
    $('kpi-label-members').textContent = 'Active members';
    $('kpi-members').textContent = (appState.members || []).length;
  } else {
    const activeHolds = (appState.reservations || []).filter(
      (r) => r.status === 'Pending' || r.status === 'Ready'
    ).length;
    $('kpi-label-members').textContent = 'My active holds';
    $('kpi-members').textContent = activeHolds;
  }

  // Overview Loans Table
  const loansBody = $('ov-loans-body');
  if (!appState.loans || appState.loans.length === 0) {
    loansBody.innerHTML = '<tr><td colspan="7" class="muted">Khong co phieu muon nao.</td></tr>';
  } else {
    loansBody.innerHTML = appState.loans
      .map(
        (l) => `
