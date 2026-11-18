/**
 * SysPilot: Master Controller & App Orchestrator
 * Seamless tab routing, global keyboard shortcuts, and performance lifecycle management.
 */

document.addEventListener('DOMContentLoaded', () => {
    initTabs();
    initMonitor();
    initAnalyzer();
    initScheduler();
    initCodeViewer();
    startUptimeTicker();
    setupGlobalKeyboardShortcuts();
});

function initTabs() {
    const tabBtns = document.querySelectorAll('.tab-btn');
    tabBtns.forEach(btn => {
        btn.addEventListener('click', () => {
            const tabId = btn.getAttribute('data-tab');
            switchTab(tabId);
        });
    });
}

function switchTab(tabId) {
    const tabBtns = document.querySelectorAll('.tab-btn');
    tabBtns.forEach(b => {
        if (b.getAttribute('data-tab') === tabId) b.classList.add('active');
        else b.classList.remove('active');
    });

    document.querySelectorAll('.tab-panel').forEach(p => {
        if (p.id === tabId) p.classList.add('active');
        else p.classList.remove('active');
    });

    // Notify modules to throttle / resume telemetry
    if (typeof setMonitorActive === 'function') {
        setMonitorActive(tabId === 'tab-monitor');
    }
    if (typeof setAnalyzerActive === 'function') {
        setAnalyzerActive(tabId === 'tab-analyzer');
    }

    if (tabId === 'tab-code') {
        loadCodeFile('monitor.c');
    }
}

function setupGlobalKeyboardShortcuts() {
    window.addEventListener('keydown', (e) => {
        // Do not trigger tab hotkeys when user is actively typing in an input field
        const activeTag = document.activeElement ? document.activeElement.tagName.toLowerCase() : '';
        const isTyping = (activeTag === 'input' || activeTag === 'textarea' || activeTag === 'select');

        // Escape closes any open modal
        if (e.key === 'Escape') {
            const modal = document.getElementById('benchmark-modal');
            if (modal && modal.style.display !== 'none') {
                closeBenchmarkModal();
                return;
            }
        }

        if (isTyping) {
            return;
        }

        // Quick Tab Navigation Keys: 1, 2, 3, 4
        if (e.key === '1') switchTab('tab-monitor');
        else if (e.key === '2') switchTab('tab-analyzer');
        else if (e.key === '3') switchTab('tab-scheduler');
        else if (e.key === '4') switchTab('tab-code');
        // Space bar: Pause/resume telemetry when on Tab 1
        else if (e.key === ' ' && document.getElementById('tab-monitor').classList.contains('active')) {
            e.preventDefault();
            toggleTelemetryPause();
        }
    });
}

function startUptimeTicker() {
    let seconds = 0;
    setInterval(() => {
        seconds++;
        const mins = Math.floor(seconds / 60);
        const secs = seconds % 60;
        const uptimeEl = document.getElementById('session-uptime');
        if (uptimeEl) {
            uptimeEl.innerText = `${mins}m ${secs < 10 ? '0' : ''}${secs}s`;
        }
    }, 1000);
}

// C Code Viewer
async function initCodeViewer() {
    const fileItems = document.querySelectorAll('.code-file-item');
    fileItems.forEach(item => {
        item.addEventListener('click', () => {
            fileItems.forEach(i => i.classList.remove('active'));
            item.classList.add('active');
            const file = item.getAttribute('data-file');
            loadCodeFile(file);
        });
    });
}

async function loadCodeFile(filename) {
    try {
        const res = await fetch(`/api/c-code?file=${encodeURIComponent(filename)}`);
        const data = await res.json();
        if (data.status === 'success') {
            document.getElementById('code-display').innerText = data.content;
            document.getElementById('current-code-filename').innerText = filename;
        }
    } catch (e) {
        console.error('Error loading code file:', e);
    }
}

async function generateCapstoneReport() {
    try {
        const res = await fetch('/api/reports/capstone');
        const data = await res.json();
        if (data.status === 'success') {
            const blob = new Blob([data.report_markdown], { type: 'text/markdown' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'SysPilot_Capstone_Evaluation_Report.md';
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
            alert("SysPilot Capstone Performance & Evaluation Report generated and downloaded successfully!");
        }
    } catch (e) {
        console.error('Report error:', e);
    }
}
