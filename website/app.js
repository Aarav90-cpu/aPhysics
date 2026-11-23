/**
 * Copyright 2026 The aPhysics Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// aPhysics Web App — Interactive Portal & Documentation Viewer

const docsData = {
  getting_started: {
    title: "getting_started.md",
    content: `
      <h2>Getting Started with aPhysics</h2>
      <p>This guide walks you through setting up, building, and running aPhysics on your machine.</p>
      
      <h3>Prerequisites</h3>
      <ul>
        <li><strong>Clang 23+</strong>: aPhysics is strictly built with Clang (minimum version 23). GCC is not supported.</li>
        <li><strong>Bazel 9+</strong>: Bazel 9 is the required build tool (version 9.2.0 is pinned in <code>.bazelversion</code>).</li>
      </ul>

      <h3>Cloning & Building</h3>
      <pre><code># Clone via SSH
git clone git@github.com:Aarav90-cpu/aPhysics.git
cd aPhysics

# Build all targets
bazel build //...

# Run the physics binary
bazel run //:app

# Run the test suite
bazel test //...</code></pre>
    `
  },
  architecture: {
    title: "architecture.md",
    content: `
      <h2>Architecture: Zero-Copy Memory & Determinism</h2>
      <p>aPhysics is designed specifically to eliminate performance bottlenecks when streaming simulation data into Three.js inside Remotion video rendering pipelines.</p>
      
      <h3>1. Zero-Copy Memory Sharing</h3>
      <p>WebAssembly memory is backed by a contiguous <code>ArrayBuffer</code>. aPhysics allocates vertex buffers inside this memory and exports the memory offset pointer.</p>
      <pre><code>// Direct typed array view over WebAssembly linear memory
const positions = new Float32Array(
    wasmMemory.buffer,
    wasmInstance.exports.aphysics_get_positions(),
    vertexCount * 3
);

geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));

// On frame step:
wasmInstance.exports.aphysics_step(dt);
geometry.attributes.position.needsUpdate = true;</code></pre>

      <h3>2. Deterministic Simulation via Seeds</h3>
      <p>Setting a fixed 64-bit seed (e.g. <code>1337</code>) guarantees identical cloth wrinkles, crash deformation, and tears on any machine.</p>
    `
  },
  macos_testers: {
    title: "macos_testers.md",
    content: `
      <h2>Looking for macOS Testers!</h2>
      <p><strong>Platform Status</strong>: Linux (Tested & Supported) | macOS (Experimental & Untested)</p>
      <p>While Bazel 9 and Clang 23 support macOS (Apple Silicon arm64 & Intel x86_64), macOS builds are currently experimental and untested on physical hardware.</p>
      
      <h3>How to Test</h3>
      <pre><code>brew install bazelisk
git clone git@github.com:Aarav90-cpu/aPhysics.git
cd aPhysics
bazel build //...
bazel test //...</code></pre>

      <p>Please report any issues or compiler warnings on our GitHub Issues page!</p>
    `
  },
  agent_skills: {
    title: "agent_skills.md",
    content: `
      <h2>Agent Skills & Automated Tooling</h2>
      <p>aPhysics is designed from the ground up to be agent-friendly and compatible with modern AI development workflows and documentation crawlers.</p>
      
      <h3>AGENTS.md Guidelines</h3>
      <ul>
        <li>Strict tooling: Clang 23+ and Bazel 9+ only.</li>
        <li>Verification first: Always verify with <code>bazel build //...</code> and <code>bazel test //...</code>.</li>
        <li>Apache 2.0 license headers mandatory via <code>addlicense</code>.</li>
      </ul>

      <h3>fetchIt Compatibility</h3>
      <p>The documentation is formatted for full compatibility with <strong>fetchIt</strong>, the clean markdown web crawler built by Aarav.</p>
    `
  },
  agents: {
    title: "AGENTS.md",
    content: `
      <h2>AGENTS.md Instructions</h2>
      <p>Instructions for AI coding agents working in this repository:</p>
      <ul>
        <li><strong>Language</strong>: C (C17)</li>
        <li><strong>Compiler</strong>: Clang only (minimum version 23). Do not add GCC.</li>
        <li><strong>Build system</strong>: Bazel only (minimum version 9). Do not add Makefiles.</li>
        <li><strong>License</strong>: Apache 2.0. Every source file must have license headers.</li>
        <li><strong>Verification</strong>: Run Bazel build and tests before reporting done.</li>
      </ul>
    `
  }
};

// Search database items
const searchIndex = [
  { title: "Zero-Copy Memory", desc: "Share memory directly with Three.js via Float32Array", tab: "architecture" },
  { title: "Deterministic Seeds", desc: "Minecraft-style 64-bit seeds for reproducible physics", tab: "architecture" },
  { title: "macOS Tester Guide", desc: "Help test aPhysics on Apple Silicon and Intel Macs", tab: "macos" },
  { title: "Getting Started", desc: "Prerequisites, Bazel 9 & Clang 23 build instructions", tab: "docs", doc: "getting_started" },
  { title: "Clang 23 & Bazel 9", desc: "Strict build requirements and compiler flags", tab: "overview" },
  { title: "Cloth Simulation", desc: "Fabric drape, wrinkles, and tearing physics", tab: "overview" },
  { title: "Crash & Impact Dynamics", desc: "Collision deformation and fracture models", tab: "overview" },
  { title: "Agent Skills & Guidelines", desc: "Autonomous AI rules and fetchIt compatibility", tab: "agents" },
  { title: "About the Creator", desc: "Aarav / Arphd4, ARK OS, fetchIt, MusicSkills, YouTube", tab: "creator" }
];

// Switch active tab
function switchTab(tabId) {
  document.querySelectorAll('.tab-panel').forEach(panel => {
    panel.classList.remove('active');
  });
  document.querySelectorAll('.nav-btn').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.tab === tabId);
  });

  const activePanel = document.getElementById(`tab-${tabId}`);
  if (activePanel) {
    activePanel.classList.add('active');
    window.scrollTo({ top: 0, behavior: 'smooth' });
  }

  // Hide search dropdown if open
  document.getElementById('searchResults').classList.add('hidden');
}

// Copy clone command
function copyClone() {
  const code = document.getElementById('cloneCommand').innerText;
  navigator.clipboard.writeText(code).then(() => {
    const btn = document.querySelector('.btn-copy');
    btn.innerText = 'Copied!';
    setTimeout(() => { btn.innerText = 'Copy'; }, 2000);
  });
}

// Theme toggle
function toggleTheme() {
  const html = document.documentElement;
  const current = html.getAttribute('data-theme');
  const next = current === 'dark' ? 'light' : 'dark';
  html.setAttribute('data-theme', next);
  localStorage.setItem('theme', next);
}

// Search handler
function handleSearch(query) {
  const dropdown = document.getElementById('searchResults');
  if (!query || query.trim() === '') {
    dropdown.classList.add('hidden');
    dropdown.innerHTML = '';
    return;
  }

  const q = query.toLowerCase();
  const results = searchIndex.filter(item => 
    item.title.toLowerCase().includes(q) || item.desc.toLowerCase().includes(q)
  );

  if (results.length === 0) {
    dropdown.innerHTML = '<div class="search-item"><div class="search-item-desc">No matching results found</div></div>';
    dropdown.classList.remove('hidden');
    return;
  }

  dropdown.innerHTML = results.map(item => `
    <div class="search-item" onclick="selectSearchResult('${item.tab}', '${item.doc || ''}')">
      <div class="search-item-title">${item.title}</div>
      <div class="search-item-desc">${item.desc}</div>
    </div>
  `).join('');

  dropdown.classList.remove('hidden');
}

function selectSearchResult(tab, doc) {
  switchTab(tab);
  if (doc) {
    loadDoc(doc);
  }
}

// Load doc into viewer
function loadDoc(docKey) {
  const doc = docsData[docKey];
  if (!doc) return;

  document.querySelectorAll('.doc-link').forEach(btn => {
    btn.classList.toggle('active', btn.innerText.includes(docKey));
  });

  const viewer = document.getElementById('docViewer');
  viewer.innerHTML = doc.content;
}

// Initialize on DOM loaded
document.addEventListener('DOMContentLoaded', () => {
  const savedTheme = localStorage.getItem('theme') || 'dark';
  document.documentElement.setAttribute('data-theme', savedTheme);

  // Load default doc
  loadDoc('getting_started');

  // Handle hash routing
  const hash = window.location.hash.replace('#', '');
  if (hash && document.getElementById(`tab-${hash}`)) {
    switchTab(hash);
  }

  // Close search dropdown on click outside
  document.addEventListener('click', (e) => {
    if (!e.target.closest('.search-box')) {
      document.getElementById('searchResults').classList.add('hidden');
    }
  });
});
