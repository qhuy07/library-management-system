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
 // 4. MEMBERS TABLE (STAFF)
  if (isStaff) {
    const rows = [];
    const isAdmin = currentUser.role === 'Administrator';
    const editBtn = (u) => isAdmin
      ? ` <button type="button" class="btn btn-mint btn-xs" style="margin-left:6px;" onclick="fillAdminEditUser('${escapeHtml(u.userId)}','${escapeHtml(u.fullName)}','${escapeHtml(u.email)}')">Sua</button>`
      : '';
    (appState.admins || []).forEach((a) => {
      rows.push(`<tr><td><strong>${escapeHtml(a.userId)}</strong></td><td>${escapeHtml(a.fullName)}</td><td>${escapeHtml(a.email)}</td><td>Administrator</td><td>Quan tri he thong${editBtn(a)}</td></tr>`);
    });
    (appState.librarians || []).forEach((l) => {
      rows.push(`<tr><td><strong>${escapeHtml(l.userId)}</strong></td><td>${escapeHtml(l.fullName)}</td><td>${escapeHtml(l.email)}</td><td>Librarian</td><td>Ma NV: ${escapeHtml(l.staffCode)} | Ca: ${escapeHtml(l.deskShift)}${editBtn(l)}</td></tr>`);
    });
    (appState.members || []).forEach((m) => {
      rows.push(`<tr><td><strong>${escapeHtml(m.userId)}</strong></td><td>${escapeHtml(m.fullName)}</td><td>${escapeHtml(m.email)}</td><td>Member</td><td>The: ${escapeHtml(m.membershipCardNo)} | Muon: ${m.activeLoansCount}/${m.maxQuota} | No: ${Number(m.totalUnpaidFines).toLocaleString('vi-VN')} d${editBtn(m)}</td></tr>`);
    });
    $('mem-table-body').innerHTML = rows.join('');

    // 5. REPORTS & POLICY
    if (appState.policy) {
      if (document.activeElement !== $('rep-max-days')) $('rep-max-days').value = appState.policy.defaultLoanDays;
      if (document.activeElement !== $('rep-fine-rate')) $('rep-fine-rate').value = appState.policy.fineRatePerDay;
    }
    $('rep-output').textContent = appState.auditReport || '';
  }
}

function renderCatalogue() {
  if (!appState) return;
  const kw = ($('cat-search').value || '').trim().toLowerCase();
  const tbody = $('cat-table-body');
  const filtered = (appState.books || []).filter((b) => {
    if (!kw) return true;
    return (
      b.bookId.toLowerCase().includes(kw) ||
      b.title.toLowerCase().includes(kw) ||
      b.author.toLowerCase().includes(kw)
    );
  });

  if (filtered.length === 0) {
    tbody.innerHTML = '<tr><td colspan="5" class="muted">Khong tim thay dau sach phu hop.</td></tr>';
    return;
  }

  tbody.innerHTML = filtered
    .map((b) => {
      const copies = (appState.bookCopies || []).filter((c) => c.bookId === b.bookId);
      const copyHtml = copies.length
        ? copies
            .map(
              (c) =>
                `<div style="margin-bottom:4px;"><code>${escapeHtml(c.copyId)}</code> ${statusBadge(c.copyStatus)}</div>`
            )
            .join('')
        : '<span class="muted">Chua co ban sao</span>';
      return `
        <tr>
          <td><strong>${escapeHtml(b.bookId)}</strong></td>
          <td>${escapeHtml(b.title)}</td>
          <td>${escapeHtml(b.author)}</td>
          <td>${escapeHtml(b.isbn)}</td>
          <td>${copyHtml}</td>
        </tr>`;
    })
    .join('');
}

window.handleHoldAction = async function (action, reservationId) {
  const params = { reservationId };
  if (action === 'fulfill' && $('cir-date').value) {
    params.date = $('cir-date').value;
  }
  const data = await apiPost(`/api/circulation/hold/${action}`, params);
  showToast(data.message || 'Da cap nhat trang thai dat cho.', !data.ok);
  await loadState();
};

function startApp(user) {
  currentUser = user;
  sessionStorage.setItem('nf_user', JSON.stringify(user));

  $('auth-screen').classList.add('hidden');
  $('app-shell').classList.remove('hidden');
  $('user-name').textContent = `${user.fullName} (${user.userId})`;
  $('user-role').textContent = user.role;

  applyRoleVisibility();
  switchTab('overview');
  loadState();

  clearInterval(pollTimer);
  pollTimer = setInterval(() => {
    const tag = document.activeElement?.tagName;
    if (currentUser && tag !== 'INPUT' && tag !== 'SELECT' && tag !== 'TEXTAREA') {
      loadState(true);
    }
  }, 5000);
}

function logout() {
  currentUser = null;
  appState = null;
  sessionStorage.removeItem('nf_user');
  clearInterval(pollTimer);
  $('app-shell').classList.add('hidden');
  $('auth-screen').classList.remove('hidden');
  $('login-status').textContent = 'Log out of the system.';
}

// EVENT LISTENERS
$('login-form').addEventListener('submit', async (e) => {
  e.preventDefault();
  const data = await apiPost('/api/login', {
    email: $('login-email').value.trim(),
    password: $('login-pass').value
  });
  if (data.ok && data.user) {
    $('login-status').textContent = '';
    startApp(data.user);
  } else {
    $('login-status').textContent = data.message || 'Log in fail!';
  }
});

$('btn-go-register').addEventListener('click', () => {
  $('login-card').classList.add('hidden');
  $('register-card').classList.remove('hidden');
  $('reg-status').textContent = '';
});

$('btn-back-login').addEventListener('click', () => {
  $('register-card').classList.add('hidden');
  $('login-card').classList.remove('hidden');
});

$('register-form').addEventListener('submit', async (e) => {
  e.preventDefault();
  const data = await apiPost('/api/register', {
    name: $('reg-name').value.trim(),
    email: $('reg-email').value.trim(),
    password: $('reg-pass').value,
    confirmPassword: $('reg-confirm').value
  });
  if (data.ok) {
    $('login-email').value = $('reg-email').value.trim();
    $('login-pass').value = '';
    $('register-card').classList.add('hidden');
    $('login-card').classList.remove('hidden');
    $('login-status').style.color = '#1e4d3b';
    $('login-status').textContent = data.message + ' Please log in!';
  } else {
    $('reg-status').textContent = data.message || 'Register fail!';
  }
});

$('btn-logout').addEventListener('click', logout);

document.querySelectorAll('.nav-btn').forEach((btn) => {
  btn.addEventListener('click', () => {
    switchTab(btn.dataset.tab);
    loadState(true);
  });
});

$('cat-search').addEventListener('input', renderCatalogue);

$('btn-add-book').addEventListener('click', async () => {
  const data = await apiPost('/api/books/add', {
    bookId: $('cat-book-id').value.trim(),
    title: $('cat-title').value.trim(),
    author: $('cat-author').value.trim(),
    copyId: $('cat-copy-id').value.trim(),
    barcode: $('cat-barcode').value.trim()
  });
  showToast(data.message, !data.ok);
  await loadState();
});

$('btn-upd-book').addEventListener('click', async () => {
  const data = await apiPost('/api/books/update', {
    bookId: $('cat-book-id').value.trim(),
    title: $('cat-title').value.trim(),
    author: $('cat-author').value.trim()
  });
  showToast(data.message, !data.ok);
  await loadState();
});

$('btn-del-book').addEventListener('click', async () => {
  const data = await apiPost('/api/books/delete', {
    bookId: $('cat-book-id').value.trim()
  });
  showToast(data.message, !data.ok);
  await loadState();
});

window.deleteCopyById = async function (copyId) {
  const data = await apiPost('/api/copies/delete', { copyId });
  showToast(data.message, !data.ok);
  await loadState();
};

$('btn-del-copy').addEventListener('click', async () => {
  await window.deleteCopyById($('cat-copy-id').value.trim());
});
$('btn-hold')?.addEventListener('click', async () => {
  const data = await apiPost('/api/circulation/hold', {
    bookId: $('cir-book-id').value.trim()
  });
  showToast(data.message, !data.ok);
  await loadState();
});

$('btn-return').addEventListener('click', async () => {
  const data = await apiPost('/api/circulation/return', {
    loanId: $('cir-loan-id').value.trim(),
    date: $('cir-date').value
  });
  showToast(data.message, !data.ok);
  await loadState();
});

$('btn-renew').addEventListener('click', async () => {
  const data = await apiPost('/api/circulation/renew', {
    loanId: $('cir-loan-id').value.trim()
  });
  showToast(data.message, !data.ok);
  await loadState();
});

$('btn-pay-fine').addEventListener('click', async () => {
  const data = await apiPost('/api/circulation/pay-fine', {
    fineId: $('cir-fine-id').value.trim()
  });
  showToast(data.message, !data.ok);
  await loadState();
});
