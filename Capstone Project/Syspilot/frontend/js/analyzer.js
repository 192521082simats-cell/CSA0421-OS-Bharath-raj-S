/**
 * SysPilot: Module 2 - Intelligent Bottleneck & Conflict Analysis Frontend
 * Smooth, non-blocking updates with intelligent content diffing.
 */

let analyzerInterval = null;
let isAnalyzerActive = false;
let lastAlertsHash = '';
let lastConflictsHash = '';

function initAnalyzer() {
    fetchAnalysis();
    startAnalyzerPolling();
}

function startAnalyzerPolling() {
    if (analyzerInterval) clearInterval(analyzerInterval);
    // Relaxed 4-second polling prevents network & UI jitter
    analyzerInterval = setInterval(() => {
        if (isAnalyzerActive) {
            fetchAnalysis();
        }
    }, 4000);
}

function setAnalyzerActive(active) {
    isAnalyzerActive = active;
    if (active) {
        fetchAnalysis();
    }
}

async function fetchAnalysis() {
    try {
        const res = await fetch('/api/analysis/bottlenecks');
        const data = await res.json();
        if (data.status === 'success') {
            updateAnalysisUI(data.analysis);
        }
    } catch (e) {
        console.error('Error fetching analysis:', e);
    }
}

function updateAnalysisUI(rep) {
    // Health Score
    const scoreVal = document.getElementById('health-score-val');
    if (scoreVal) scoreVal.innerText = `${rep.health_score.toFixed(0)}%`;

    const statusBadge = document.getElementById('health-status-badge');
    if (statusBadge) {
        statusBadge.innerText = rep.general_status;
        statusBadge.className = 'status-pill ' + (rep.general_status === 'OPTIMAL' ? 'state-R' : (rep.general_status === 'STRESSED' ? 'sev-HIGH' : 'sev-CRITICAL'));
    }

    const headerScorePill = document.getElementById('header-health-score');
    if (headerScorePill) {
        headerScorePill.innerText = `Health: ${rep.health_score.toFixed(0)}% (${rep.general_status})`;
        headerScorePill.style.color = rep.general_status === 'OPTIMAL' ? 'var(--accent-green)' : (rep.general_status === 'STRESSED' ? 'var(--accent-orange)' : 'var(--accent-red)');
    }

    // Bottleneck Alerts List (Only update DOM if hash changed)
    const newAlertsHash = JSON.stringify(rep.alerts);
    const alertsContainer = document.getElementById('bottleneck-alerts-list');
    if (alertsContainer && newAlertsHash !== lastAlertsHash) {
        lastAlertsHash = newAlertsHash;
        if (rep.alerts.length === 0) {
            alertsContainer.innerHTML = `
                <div style="background-color:var(--bg-card); padding:24px; border-radius:8px; text-align:center; color:var(--accent-green); border:1px solid #238636;">
                    <div style="font-size:24px; margin-bottom:8px;">✓</div>
                    <div style="font-weight:700; font-size:16px;">Zero Critical Bottlenecks Detected</div>
                    <div style="color:var(--text-muted); font-size:13px; margin-top:4px;">
                        All subsystems (CPU queues, physical memory allocation, swap rates, and storage I/O wait) are operating inside nominal bounds.
                    </div>
                </div>
            `;
        } else {
            alertsContainer.innerHTML = '';
            rep.alerts.forEach(a => {
                const card = document.createElement('div');
                card.className = `alert-card ${a.severity}`;
                card.innerHTML = `
                    <div class="alert-title-row">
                        <div style="display:flex; align-items:center; gap:8px;">
                            <span class="severity-pill sev-${a.severity}">${a.severity}</span>
                            <span style="font-weight:700; font-size:15px;">${a.title}</span>
                            <span style="color:var(--text-muted); font-size:12px;">[${a.component}]</span>
                        </div>
                        <div style="font-family:var(--font-mono); font-size:12px; color:var(--text-muted);">
                            Metric: ${a.metric_value.toFixed(1)}% / Thr: ${a.threshold.toFixed(1)}%
                        </div>
                    </div>
                    <div style="font-size:13px; color:#c9d1d9; margin-top:8px;">
                        ${a.description}
                    </div>
                    ${a.culprit_pid ? `
                        <div style="font-size:12px; margin-top:6px; color:#58a6ff;">
                            Primary Culprit: <strong>PID ${a.culprit_pid} (${a.culprit_name})</strong>
                        </div>
                    ` : ''}
                    <div class="advice-box" style="margin-top:10px; padding:10px; font-size:12px;">
                        <strong>Remediation Recommendation:</strong> ${a.recommendation}
                    </div>
                `;
                alertsContainer.appendChild(card);
            });
        }
    }

    // Resource Conflicts List (Only update DOM if hash changed)
    const newConflictsHash = JSON.stringify(rep.conflicts);
    const conflictsContainer = document.getElementById('resource-conflicts-list');
    if (conflictsContainer && newConflictsHash !== lastConflictsHash) {
        lastConflictsHash = newConflictsHash;
        if (rep.conflicts.length === 0) {
            conflictsContainer.innerHTML = `
                <div style="background-color:var(--bg-card); padding:20px; border-radius:8px; text-align:center; color:var(--text-muted); border:1px solid var(--border-color); font-size:13px;">
                    No inter-process resource contention, lock starvation, or priority inversions detected.
                </div>
            `;
        } else {
            conflictsContainer.innerHTML = '';
            rep.conflicts.forEach(c => {
                const card = document.createElement('div');
                card.className = 'conflict-card';
                card.innerHTML = `
                    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:6px;">
                        <span style="font-weight:700; color:#d29922; font-size:14px;">⚠️ ${c.type}</span>
                        <span class="severity-pill sev-${c.severity}">${c.severity}</span>
                    </div>
                    <div style="font-size:13px; color:#e6edf3;">
                        ${c.details}
                    </div>
                    <div style="font-size:12px; color:#8b949e; margin-top:6px;">
                        <strong>Conflict Target:</strong> PID ${c.pid_a} ('${c.name_a}') ↔ PID ${c.pid_b} ('${c.name_b}')
                    </div>
                    <div style="margin-top:8px; font-size:12px; color:#58a6ff;">
                        <strong>OS Protocol Action:</strong> ${c.remedy}
                    </div>
                `;
                conflictsContainer.appendChild(card);
            });
        }
    }
}

async function triggerStress(type) {
    try {
        const res = await fetch('/api/workload/stress', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ type: type, intensity: 65.0 })
        });
        const data = await res.json();
        if (data.status === 'success') {
            fetchMetrics();
            fetchAnalysis();
        }
    } catch (e) {
        console.error('Stress test trigger error:', e);
    }
}

async function resetStress() {
    try {
        const res = await fetch('/api/workload/reset', { method: 'POST' });
        const data = await res.json();
        if (data.status === 'success') {
            fetchMetrics();
            fetchAnalysis();
        }
    } catch (e) {
        console.error('Reset error:', e);
    }
}
