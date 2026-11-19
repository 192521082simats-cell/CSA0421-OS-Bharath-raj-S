/**
 * SysPilot: Module 1 - Live Telemetry & Process Monitor Frontend
 * Optimized for zero-flicker, zero layout thrashing, and seamless input handling.
 */

let cpuHistory = Array(30).fill(15);
let memHistory = Array(30).fill(50);
let monitorInterval = null;
let isTelemetryPaused = false;
let isMonitorActive = true;

function initMonitor() {
    initCanvases();
    fetchMetrics();
    fetchProcesses();
    startMonitorPolling();

    window.addEventListener('resize', debounce(initCanvases, 250));
}

function debounce(func, wait) {
    let timeout;
    return function(...args) {
        clearTimeout(timeout);
        timeout = setTimeout(() => func.apply(this, args), wait);
    };
}

function startMonitorPolling() {
    if (monitorInterval) clearInterval(monitorInterval);
    // Relaxed 3-second interval prevents glitching and hanging
    monitorInterval = setInterval(() => {
        if (!isTelemetryPaused && isMonitorActive) {
            fetchMetrics();
            fetchProcesses();
        }
    }, 3000);
}

function setMonitorActive(active) {
    isMonitorActive = active;
    if (active && !isTelemetryPaused) {
        fetchMetrics();
        fetchProcesses();
    }
}

function toggleTelemetryPause() {
    isTelemetryPaused = !isTelemetryPaused;
    const btn = document.getElementById('btn-toggle-telemetry');
    if (btn) {
        if (isTelemetryPaused) {
            btn.innerHTML = '▶ Resume Telemetry';
            btn.classList.add('btn-primary');
        } else {
            btn.innerHTML = '⏸ Pause Telemetry';
            btn.classList.remove('btn-primary');
            fetchMetrics();
            fetchProcesses();
        }
    }
}

function initCanvases() {
    ['cpu-chart-canvas', 'mem-chart-canvas'].forEach(id => {
        const canvas = document.getElementById(id);
        if (canvas && canvas.parentElement) {
            canvas.width = canvas.parentElement.clientWidth || 500;
            canvas.height = canvas.parentElement.clientHeight || 140;
        }
    });
}

async function fetchMetrics() {
    try {
        const res = await fetch('/api/system/metrics');
        const data = await res.json();
        if (data.status === 'success') {
            updateMetricsUI(data);
        }
    } catch (e) {
        console.error('Error fetching metrics:', e);
    }
}

function updateMetricsUI(data) {
    const { cpu, memory, disk, active_stress } = data;

    // CPU Card
    safeText('cpu-val', cpu.total_pct.toFixed(1));
    const cpuBar = document.getElementById('cpu-bar');
    if (cpuBar) {
        cpuBar.style.width = `${cpu.total_pct}%`;
        cpuBar.className = 'progress-bar-fill' + (cpu.total_pct >= 85 ? ' critical' : (cpu.total_pct >= 70 ? ' warning' : ''));
    }
    safeText('cpu-user-val', `${cpu.user_pct.toFixed(1)}%`);
    safeText('cpu-sys-val', `${cpu.system_pct.toFixed(1)}%`);
    safeText('cpu-iowait-val', `${cpu.iowait_pct.toFixed(1)}%`);

    // Memory Card
    safeText('mem-val', memory.usage_pct.toFixed(1));
    const memBar = document.getElementById('mem-bar');
    if (memBar) {
        memBar.style.width = `${memory.usage_pct}%`;
        memBar.className = 'progress-bar-fill' + (memory.usage_pct >= 90 ? ' critical' : (memory.usage_pct >= 80 ? ' warning' : ''));
    }
    safeText('mem-used-val', `${(memory.used_mb / 1024).toFixed(2)} GB`);
    safeText('mem-total-val', `${(memory.total_mb / 1024).toFixed(2)} GB`);
    safeText('mem-swap-val', `${memory.swap_pct.toFixed(1)}%`);

    // Disk Card
    safeText('disk-read-val', (disk.read_kb_per_sec / 1024).toFixed(2));
    safeText('disk-write-val', (disk.write_kb_per_sec / 1024).toFixed(2));
    safeText('disk-iops-val', `${disk.read_iops + disk.write_iops} IOPS`);

    // Context Switches
    safeText('cs-val', cpu.context_switches.toLocaleString());
    safeText('procs-run-val', cpu.procs_running);
    safeText('procs-block-val', cpu.procs_blocked);

    // Active Stress Pill
    const stressPill = document.getElementById('active-stress-pill');
    if (stressPill) {
        if (active_stress) {
            stressPill.style.display = 'inline-flex';
            stressPill.innerText = `ACTIVE STRESS: ${active_stress.toUpperCase().replace('_', ' ')}`;
        } else {
            stressPill.style.display = 'none';
        }
    }

    // In-place Per-core Bars
    updateCoresUI(cpu.per_core);

    // Update Waveforms
    cpuHistory.push(cpu.total_pct);
    cpuHistory.shift();
    memHistory.push(memory.usage_pct);
    memHistory.shift();
    drawWaveform('cpu-chart-canvas', cpuHistory, '#38bdf8');
    drawWaveform('mem-chart-canvas', memHistory, '#bc8cff');
}

function safeText(id, text) {
    const el = document.getElementById(id);
    if (el && el.innerText !== text.toString()) {
        el.innerText = text;
    }
}

function updateCoresUI(cores) {
    const container = document.getElementById('cores-container');
    if (!container) return;

    if (container.children.length !== cores.length) {
        container.innerHTML = '';
        cores.forEach((_, idx) => {
            const item = document.createElement('div');
            item.className = 'core-item';
            item.id = `core-item-${idx}`;
            item.innerHTML = `
                <div class="core-label">
                    <span>Core #${idx}</span>
                    <span id="core-val-${idx}" style="font-weight:700;">0%</span>
                </div>
                <div class="progress-bar-wrap" style="height:4px; margin-top:6px;">
                    <div id="core-bar-${idx}" class="progress-bar-fill" style="width:0%"></div>
                </div>
            `;
            container.appendChild(item);
        });
    }

    cores.forEach((pct, idx) => {
        const valEl = document.getElementById(`core-val-${idx}`);
        const barEl = document.getElementById(`core-bar-${idx}`);
        if (valEl && valEl.innerText !== `${pct}%`) {
            valEl.innerText = `${pct}%`;
            valEl.style.color = pct > 80 ? '#f85149' : '#f0f6fc';
        }
        if (barEl) {
            barEl.style.width = `${pct}%`;
            barEl.className = 'progress-bar-fill' + (pct > 80 ? ' critical' : (pct > 65 ? ' warning' : ''));
        }
    });
}

function drawWaveform(canvasId, dataPoints, color) {
    const canvas = document.getElementById(canvasId);
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const width = canvas.width;
    const height = canvas.height;

    ctx.clearRect(0, 0, width, height);

    // Grid lines
    ctx.strokeStyle = '#21262d';
    ctx.lineWidth = 1;
    for (let y = 0; y <= height; y += (height / 4)) {
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(width, y);
        ctx.stroke();
    }

    // Line
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    ctx.beginPath();
    const step = width / (dataPoints.length - 1);

    dataPoints.forEach((val, i) => {
        const x = i * step;
        const y = height - ((val / 100) * (height - 10)) - 5;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    });
    ctx.stroke();

    // Area fill
    ctx.lineTo(width, height);
    ctx.lineTo(0, height);
    ctx.closePath();
    const grad = ctx.createLinearGradient(0, 0, 0, height);
    grad.addColorStop(0, color.replace(')', ', 0.25)').replace('rgb', 'rgba'));
    grad.addColorStop(1, 'rgba(13, 17, 23, 0.02)');
    ctx.fillStyle = grad;
    ctx.fill();
}

async function fetchProcesses() {
    try {
        const res = await fetch('/api/system/processes');
        const data = await res.json();
        if (data.status === 'success') {
            window.lastProcs = data.processes || [];
            renderProcessTable(window.lastProcs);
        }
    } catch (e) {
        console.error('Error fetching processes:', e);
    }
}

function filterProcessTable() {
    if (window.lastProcs) {
        renderProcessTable(window.lastProcs);
    }
}

function renderProcessTable(procs) {
    const tbody = document.getElementById('proc-table-body');
    if (!tbody) return;

    // Do not interfere if user is actively searching
    const searchInput = document.getElementById('proc-search');
    const searchTerm = (searchInput?.value || '').toLowerCase();
    const stateFilter = document.getElementById('proc-state-filter')?.value || 'ALL';

    const filtered = procs.filter(p => {
        const matchesSearch = p.name.toLowerCase().includes(searchTerm) || p.pid.toString().includes(searchTerm);
        const matchesState = (stateFilter === 'ALL') || (p.state === stateFilter);
        return matchesSearch && matchesState;
    });

    // In-place row updates or rebuild only when necessary
    tbody.innerHTML = '';
    filtered.forEach(p => {
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td style="font-family:var(--font-mono); font-weight:700;">${p.pid}</td>
            <td style="font-weight:600;">${p.name}</td>
            <td><span class="state-badge state-${p.state}">${p.state}</span></td>
            <td>${p.nice}</td>
            <td>${p.priority}</td>
            <td>${p.threads}</td>
            <td>
                <div style="display:flex; align-items:center; gap:8px;">
                    <span style="font-family:var(--font-mono); min-width:44px;">${p.cpu_pct.toFixed(1)}%</span>
                    <div class="progress-bar-wrap" style="height:6px; width:70px; margin:0;">
                        <div class="progress-bar-fill ${p.cpu_pct > 50 ? 'critical' : (p.cpu_pct > 25 ? 'warning' : '')}" style="width:${Math.min(100, p.cpu_pct * 1.5)}%"></div>
                    </div>
                </div>
            </td>
            <td style="font-family:var(--font-mono);">${p.rss_mb.toFixed(1)} MB</td>
            <td style="font-family:var(--font-mono);">${p.io_mb_s.toFixed(1)} MB/s</td>
        `;
        tbody.appendChild(tr);
    });

    const badge = document.getElementById('proc-count-badge');
    if (badge) badge.innerText = `${filtered.length} active`;
}
