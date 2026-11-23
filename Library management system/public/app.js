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
<tr>
          <td><strong>${escapeHtml(l.loanId)}</strong></td>
          <td>${escapeHtml(l.copyId)}</td>
          <td>${escapeHtml(l.memberId)}</td>
          <td>${escapeHtml(l.issueDate)}</td>
          <td>${escapeHtml(l.dueDate)}</td>
          <td>${l.returnDate && l.returnDate !== 'NONE' ? escapeHtml(l.returnDate) : '-'}</td>
          <td>${statusBadge(l.status)}</td>
        </tr>`
      )
      .join('');
  }

  // Overview Fines Table
  const finesBody = $('ov-fines-body');
  if (!appState.fines || appState.fines.length === 0) {
    finesBody.innerHTML = '<tr><td colspan="6" class="muted">Khong co khoan phat nao.</td></tr>';
  } else {
    finesBody.innerHTML = appState.fines
      .map(
        (f) => `
        <tr>
          <td><strong>${escapeHtml(f.fineId)}</strong></td>
          <td>${escapeHtml(f.loanId)}</td>
          <td>${f.daysOverdue} ngay</td>
          <td>${Number(f.amount).toLocaleString('vi-VN')} d</td>
          <td>${f.paymentDate && f.paymentDate !== 'NONE' ? escapeHtml(f.paymentDate) : '-'}</td>
          <td>${statusBadge(f.paidStatus)}</td>
        </tr>`
      )
      .join('');
  }

  // 2. CATALOGUE TABLE
  renderCatalogue();

  // 3. CIRCULATION - RESERVATIONS TABLE (DONG BO MEMBER <-> LIBRARIAN)
  const resBody = $('cir-res-body');
  if (!appState.reservations || appState.reservations.length === 0) {
    resBody.innerHTML = '<tr><td colspan="8" class="muted">Chua co phieu dat giu cho nao.</td></tr>';
  } else {
    resBody.innerHTML = appState.reservations
      .map((r) => {
        const copyLabel = r.copyId && r.copyId !== 'NONE' ? `<strong>${escapeHtml(r.copyId)}</strong>` : '-';
        let statusHtml = statusBadge(r.status);
        if (!isStaff && r.status === 'Ready') {
          statusHtml += `<span class="sub-note">Sach san sang nhan tai quay</span>`;
        } else if (!isStaff && r.status === 'Pending') {
          statusHtml += `<span class="sub-note" style="color:#92400e;">Cho Thu thu xac nhan</span>`;
        }

        let actions = '-';
        if (isStaff) {
          const btns = [];
          if (r.status === 'Pending') {
            btns.push(
              `<button class="btn btn-teal btn-xs" onclick="handleHoldAction('approve','${escapeHtml(r.reservationId)}')">Duyet</button>`
            );
          }
          if (r.status === 'Pending' || r.status === 'Ready') {
            btns.push(
              `<button class="btn btn-primary btn-xs" onclick="handleHoldAction('fulfill','${escapeHtml(r.reservationId)}')">Giao sach</button>`
            );
            btns.push(
              `<button class="btn btn-danger btn-xs" onclick="handleHoldAction('cancel','${escapeHtml(r.reservationId)}')">Huy</button>`
            );
          }
          actions = btns.length ? `<div class="btn-row" style="margin:0;">${btns.join('')}</div>` : '-';
        } else if (r.status === 'Pending' || r.status === 'Ready') {
          actions = `<button class="btn btn-danger btn-xs" onclick="handleHoldAction('cancel','${escapeHtml(r.reservationId)}')">Huy dat cho</button>`;
        }

        return `
          <tr>
            <td><strong>${escapeHtml(r.reservationId)}</strong></td>
            <td>${escapeHtml(r.memberId)}</td>
            <td>${escapeHtml(r.bookId)}</td>
            <td>${copyLabel}</td>
            <td>${escapeHtml(r.requestDate)}</td>
            <td>${escapeHtml(r.expiryDate)}</td>
            <td>${statusHtml}</td>
            <td>${actions}</td>
          </tr>`;
      })
      .join('');
  }
