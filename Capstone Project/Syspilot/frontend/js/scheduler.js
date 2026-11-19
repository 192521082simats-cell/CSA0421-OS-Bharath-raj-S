/**
 * SysPilot: Module 3 - CPU Scheduling Algorithms & OS Optimization Frontend
 * Highly responsive keyboard controls, instant input syncing, and seamless simulation.
 */

let currentWorkload = [];
let allPresets = {};

const COLOR_PALETTE = [
    '#38bdf8', '#818cf8', '#c084fc', '#f472b6', '#fb7185',
    '#34d399', '#fbbf24', '#a3e635', '#2dd4bf', '#60a5fa'
];

function initScheduler() {
    loadPresets();
    setupEventListeners();
}

async function loadPresets() {
    try {
        const res = await fetch('/api/scheduler/presets');
        const data = await res.json();
        if (data.status === 'success') {
            allPresets = data.presets;
            applyPreset('academic_standard');
        }
    } catch (e) {
        console.error('Error loading presets:', e);
    }
}

function applyPreset(presetKey) {
    const preset = allPresets[presetKey];
    if (!preset) return;

    currentWorkload = JSON.parse(JSON.stringify(preset.processes));
    const slider = document.getElementById('quantum-slider');
    if (slider) {
        slider.value = preset.quantum;
        document.getElementById('quantum-val').innerText = preset.quantum;
    }
    const desc = document.getElementById('preset-desc');
    if (desc) desc.innerText = preset.description;

    renderWorkloadTable();
    runSimulation();
}

function setupEventListeners() {
    document.getElementById('preset-select')?.addEventListener('change', (e) => {
        applyPreset(e.target.value);
    });

    document.getElementById('quantum-slider')?.addEventListener('input', (e) => {
        document.getElementById('quantum-val').innerText = e.target.value;
    });

    document.getElementById('btn-add-process')?.addEventListener('click', addProcessRow);
    document.getElementById('btn-randomize')?.addEventListener('click', randomizeWorkload);
    document.getElementById('btn-simulate')?.addEventListener('click', runSimulation);
    document.getElementById('btn-benchmark-all')?.addEventListener('click', runBenchmarkAll);
    document.getElementById('algo-select')?.addEventListener('change', runSimulation);
}

function renderWorkloadTable() {
    const tbody = document.getElementById('workload-table-body');
    if (!tbody) return;
    tbody.innerHTML = '';

    currentWorkload.forEach((p, idx) => {
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td style="width:75px;">
                <input type="number" class="table-input" value="${p.pid}" 
                       oninput="updateProcessProp(${idx}, 'pid', this.value)"
                       onkeydown="handleTableKey(event)">
            </td>
            <td style="min-width:160px;">
                <input type="text" class="table-input" value="${p.name}" 
                       oninput="updateProcessProp(${idx}, 'name', this.value)"
                       onkeydown="handleTableKey(event)">
            </td>
            <td style="width:105px;">
                <input type="number" class="table-input" min="0" value="${p.arrival}" 
                       oninput="updateProcessProp(${idx}, 'arrival', this.value)"
                       onkeydown="handleTableKey(event)">
            </td>
            <td style="width:105px;">
                <input type="number" class="table-input" min="1" value="${p.burst}" 
                       oninput="updateProcessProp(${idx}, 'burst', this.value)"
                       onkeydown="handleTableKey(event)">
            </td>
            <td style="width:95px;">
                <input type="number" class="table-input" min="1" max="10" value="${p.priority || 3}" 
                       oninput="updateProcessProp(${idx}, 'priority', this.value)"
                       onkeydown="handleTableKey(event)">
            </td>
            <td style="width:95px;">
                <input type="number" class="table-input" min="-20" max="19" value="${p.nice || 0}" 
                       oninput="updateProcessProp(${idx}, 'nice', this.value)"
                       onkeydown="handleTableKey(event)">
            </td>
            <td style="width:60px; text-align:center;">
                <button class="btn btn-danger btn-sm" onclick="deleteProcessRow(${idx})" title="Remove task">✕</button>
            </td>
        `;
        tbody.appendChild(tr);
    });
}

function handleTableKey(event) {
    if (event.key === 'Enter') {
        event.preventDefault();
        runSimulation();
    }
}

function updateProcessProp(idx, prop, val) {
    if (!currentWorkload[idx]) return;
    if (prop === 'name') {
        currentWorkload[idx][prop] = val;
    } else {
        const num = parseInt(val);
        currentWorkload[idx][prop] = isNaN(num) ? 0 : num;
    }
}

function addProcessRow() {
    const nextPid = currentWorkload.length > 0 ? Math.max(...currentWorkload.map(p => p.pid)) + 1 : 1;
    currentWorkload.push({
        pid: nextPid,
        name: `P${nextPid}`,
        arrival: Math.floor(Math.random() * 5),
        burst: Math.floor(Math.random() * 8) + 2,
        priority: Math.floor(Math.random() * 4) + 1,
        nice: 0
    });
    renderWorkloadTable();
    runSimulation();
}

function deleteProcessRow(idx) {
    if (currentWorkload.length <= 1) {
        alert("Must maintain at least 1 process in the scheduling queue.");
        return;
    }
    currentWorkload.splice(idx, 1);
    renderWorkloadTable();
    runSimulation();
}

function randomizeWorkload() {
    const count = 5;
    currentWorkload = [];
    for (let i = 1; i <= count; i++) {
        currentWorkload.push({
            pid: i,
            name: `Task_${i}`,
            arrival: Math.floor(Math.random() * 5),
            burst: Math.floor(Math.random() * 10) + 1,
            priority: Math.floor(Math.random() * 5) + 1,
            nice: Math.floor(Math.random() * 20) - 10
        });
    }
    renderWorkloadTable();
    runSimulation();
}

async function runSimulation() {
    const algo = document.getElementById('algo-select').value;
    const quantum = parseInt(document.getElementById('quantum-slider').value) || 4;

    try {
        const res = await fetch('/api/scheduler/simulate', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                algorithm: algo,
                quantum: quantum,
                processes: currentWorkload
            })
        });
        const data = await res.json();
        if (data.status === 'success') {
            renderSimulationResults(data.result);
        }
    } catch (e) {
        console.error('Simulation error:', e);
    }
}

function renderSimulationResults(res) {
    // 1. KPI Cards
    safeSetText('res-avg-wt', `${res.avg_waiting_time} ms`);
    safeSetText('res-avg-tat', `${res.avg_turnaround_time} ms`);
    safeSetText('res-avg-rt', `${res.avg_response_time} ms`);
    safeSetText('res-cpu-util', `${res.cpu_utilization_pct}%`);
    safeSetText('res-throughput', `${res.throughput} j/ms`);
    safeSetText('res-fairness', res.jains_fairness_index.toFixed(4));
    safeSetText('res-ctx-switches', res.context_switches);

    // 2. Advice Box
    safeSetText('res-advice-text', res.recommendation);

    // 3. Gantt Chart
    renderGanttChart(res.gantt, res.total_time);

    // 4. Detailed Process Metrics Table
    const tbody = document.getElementById('metrics-table-body');
    if (tbody) {
        tbody.innerHTML = '';
        res.processes.forEach(p => {
            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td style="font-weight:700; font-family:var(--font-mono);">${p.pid}</td>
                <td style="font-weight:600;">${p.name}</td>
                <td>${p.arrival}</td>
                <td>${p.burst}</td>
                <td>${p.priority || '-'}</td>
                <td>${p.start}</td>
                <td style="font-weight:700;">${p.completion}</td>
                <td style="color:#58a6ff; font-weight:700;">${p.wt} ms</td>
                <td style="color:#bc8cff; font-weight:700;">${p.tat} ms</td>
                <td>${p.rt} ms</td>
            `;
            tbody.appendChild(tr);
        });
    }
}

function safeSetText(id, text) {
    const el = document.getElementById(id);
    if (el) el.innerText = text;
}

function renderGanttChart(events, totalTime) {
    const container = document.getElementById('gantt-chart-bar');
    const ticksContainer = document.getElementById('gantt-ticks');
    if (!container || !ticksContainer) return;

    container.innerHTML = '';
    ticksContainer.innerHTML = '';

    if (!events || events.length === 0 || totalTime <= 0) return;

    events.forEach((e) => {
        const duration = e.end - e.start;
        const widthPct = (duration / totalTime) * 100;
        const slice = document.createElement('div');
        slice.className = 'gantt-slice';
        slice.style.width = `${widthPct}%`;

        if (e.pid === 0) {
            slice.style.backgroundColor = '#30363d';
            slice.innerHTML = `<span style="font-size:10px; color:#8b949e;">IDLE</span><span style="font-size:9px; opacity:0.8;">[${e.start}-${e.end}]</span>`;
        } else {
            const color = COLOR_PALETTE[(e.pid - 1) % COLOR_PALETTE.length];
            slice.style.backgroundColor = color;
            slice.innerHTML = `<span style="font-size:11px; text-shadow:0 1px 2px rgba(0,0,0,0.6);">${e.name}</span><span style="font-size:9px; opacity:0.85;">[${e.start}-${e.end}]</span>`;
        }
        slice.title = `${e.name}: Start ${e.start} -> End ${e.end} (Duration ${duration} ms)`;
        container.appendChild(slice);
    });

    ticksContainer.innerHTML = `<span>0 ms</span><span>${Math.floor(totalTime / 2)} ms</span><span>${totalTime} ms</span>`;
}

async function runBenchmarkAll() {
    const quantum = parseInt(document.getElementById('quantum-slider').value) || 4;
    try {
        const res = await fetch('/api/scheduler/benchmark-all', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                quantum: quantum,
                processes: currentWorkload
            })
        });
        const data = await res.json();
        if (data.status === 'success') {
            renderBenchmarkModal(data.benchmark);
        }
    } catch (e) {
        console.error('Benchmark error:', e);
    }
}

function renderBenchmarkModal(benchmark) {
    const modal = document.getElementById('benchmark-modal');
    const tbody = document.getElementById('benchmark-table-body');
    if (!modal || !tbody) return;

    tbody.innerHTML = '';
    benchmark.forEach(b => {
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td style="font-weight:700;">${b.algorithm}</td>
            <td style="color:#58a6ff; font-weight:700;">${b.avg_waiting_time} ms</td>
            <td style="color:#bc8cff; font-weight:700;">${b.avg_turnaround_time} ms</td>
            <td>${b.avg_response_time} ms</td>
            <td>${b.context_switches}</td>
            <td style="color:#3fb950; font-weight:700;">${b.jains_fairness_index.toFixed(4)}</td>
            <td>${b.cpu_utilization_pct}%</td>
        `;
        tbody.appendChild(tr);
    });

    modal.style.display = 'flex';
}

function closeBenchmarkModal() {
    const modal = document.getElementById('benchmark-modal');
    if (modal) modal.style.display = 'none';
}
