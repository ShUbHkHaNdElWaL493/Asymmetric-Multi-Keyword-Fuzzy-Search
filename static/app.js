// Minimal FixedSizeList implementation (virtualized scrolling)
// Renders only visible rows + a small overscan buffer.
window.ReactWindowShim = (function () {
  const { useState, useRef, useCallback, useEffect, createElement: h } = React;

  function FixedSizeList({ height, itemCount, itemSize, children, width = '100%', style = {} }) {
    const containerRef = useRef(null);
    const [scrollTop, setScrollTop] = useState(0);
    const OVERSCAN = 5;

    const onScroll = useCallback((e) => {
      setScrollTop(e.currentTarget.scrollTop);
    }, []);

    const totalHeight = itemCount * itemSize;
    const startIndex = Math.max(0, Math.floor(scrollTop / itemSize) - OVERSCAN);
    const visibleCount = Math.ceil(height / itemSize);
    const endIndex = Math.min(itemCount - 1, startIndex + visibleCount + OVERSCAN * 2);

    const items = [];
    for (let i = startIndex; i <= endIndex; i++) {
      items.push(
        h('div', {
          key: i,
          style: {
            position: 'absolute',
            top: i * itemSize,
            height: itemSize,
            left: 0,
            right: 0,
          }
        }, children({ index: i, style: { height: itemSize } }))
      );
    }

    return h('div', {
      ref: containerRef,
      onScroll,
      style: { height, width, overflowY: 'auto', position: 'relative', ...style }
    },
      h('div', { style: { height: totalHeight, position: 'relative' } }, ...items)
    );
  }

  return { FixedSizeList };
})();

(function () {
  'use strict';

  // ─── React destructuring ──────────────────────────────────────────
  const {
    useState, useEffect, useRef, useCallback,
    useContext, createContext, useReducer
  } = React;
  const h = React.createElement;
  const { FixedSizeList } = window.ReactWindowShim;

  // Add your Crow server URL here
  const API_BASE = '/';

  // ================================================================
  // MODULE 1 — GLOBAL STATE (Context + Reducer)
  // Holds: config params, dataset rows, index status, current page
  // ================================================================

  const AppContext = createContext(null);

  const initialState = {
    page: 1,
    config: { L: 16, M: 256 },    // Scheme parameters
    dataset: [],                  // Array of row objects
    indexed: false,               // Whether Build Index completed
  };

  function appReducer(state, action) {
    switch (action.type) {
      case 'SET_CONFIG':
        return { ...state, config: { ...state.config, ...action.payload } };
      case 'GO_PAGE':
        return { ...state, page: action.payload };
      case 'SET_DATASET':
        return { ...state, dataset: action.payload };
      case 'ADD_ROW': {
        const newRow = { id: Date.now(), docId: '', docName: '', keywords: [], checked: false };
        return { ...state, dataset: [...state.dataset, newRow] };
      }
      case 'UPDATE_ROW': {
        const { index, field, value } = action.payload;
        const updated = state.dataset.map((row, i) =>
          i === index ? { ...row, [field]: value } : row
        );
        return { ...state, dataset: updated };
      }
      case 'TOGGLE_CHECK': {
        const updated = state.dataset.map((row, i) =>
          i === action.payload ? { ...row, checked: !row.checked } : row
        );
        return { ...state, dataset: updated };
      }
      case 'TOGGLE_ALL': {
        const allChecked = state.dataset.every(r => r.checked);
        const updated = state.dataset.map(r => ({ ...r, checked: !allChecked }));
        return { ...state, dataset: updated };
      }
      case 'REMOVE_CHECKED':
        return { ...state, dataset: state.dataset.filter(r => !r.checked) };
      case 'SET_INDEXED':
        return { ...state, indexed: action.payload };
      case 'RESET':
        return { ...initialState };
      default:
        return state;
    }
  }

  function AppProvider({ children }) {
    const [state, dispatch] = useReducer(appReducer, initialState);
    return h(AppContext.Provider, { value: { state, dispatch } }, children);
  }

  function useApp() { return useContext(AppContext); }

  // ================================================================
  // MODULE 2 — SHARED UI COMPONENTS
  // ================================================================

  // ── App Header with breadcrumb navigation ─────────────────────
  function AppHeader() {
    const { state } = useApp();
    const steps = [
      { num: 1, label: 'CONFIGURE' },
      { num: 2, label: 'INDEX' },
      { num: 3, label: 'SEARCH' },
    ];
    return h('header', { className: 'app-header' },
      h('div', { className: 'app-logo' },
        'Asymmetric Multi-Keyword Fuzzy Search'
      ),
      h('nav', { className: 'breadcrumb' },
        steps.map((s, i) => [
          i > 0 && h('span', { key: `sep-${i}`, className: 'breadcrumb-sep' }, '›'),
          h('div', {
            key: s.num,
            className: `breadcrumb-step ${s.num === state.page ? 'active' : s.num < state.page ? 'done' : ''
              }`
          },
            h('span', { className: 'breadcrumb-dot' }),
            `${s.num}. ${s.label}`
          )
        ])
      )
    );
  }

  // ── Parameter slider row ──────────────────────────────────────
  function ParamRow({ label, desc, name, min, max, step, value, onChange }) {
    function handleSlider(e) { onChange(parseFloat(e.target.value)); }
    function handleInput(e) {
      const v = parseFloat(e.target.value);
      if (!isNaN(v)) onChange(Math.min(max, Math.max(min, v)));
    }
    return h('div', { className: 'param-row' },
      h('div', { className: 'param-label' },
        label,
        h('small', null, desc)
      ),
      h('input', {
        type: 'range', min, max, step,
        value,
        onChange: handleSlider,
        style: { accentColor: 'var(--accent)' }
      }),
      h('input', {
        type: 'number', min, max, step,
        value,
        onChange: handleInput,
        className: 'param-input'
      })
    );
  }

  // ── Keyword chip ──────────────────────────────────────────────
  function Chip({ label, highlight }) {
    return h('span', { className: `chip ${highlight ? 'match' : ''}`, title: label }, label);
  }

  // ================================================================
  // MODULE 3 — PAGE 1: CONFIGURATION
  // Lets the user set L, M via sliders + number inputs,
  // then saves them to global state and navigates to Page 2.
  // ================================================================
  function Page1Config() {
    const { state, dispatch } = useApp();
    const [local, setLocal] = useState({ ...state.config });
    const [loading, setLoading] = useState(false);

    function setParam(key, val) {
      setLocal(prev => ({ ...prev, [key]: val }));
    }

    // ── Sends LSH parameters to the Crow backend, then navigates ──
    async function handleNext() {
      setLoading(true); // Start loading spinner/state

      try {
        const res = await fetch('/api/config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ L: local.L, M: local.M }),
        });
        if (!res.ok) {
          const msg = await res.text().catch(() => res.statusText);
          throw new Error(`Server error ${res.status}: ${msg}`);
        }
        // Commit config to global state only after the server confirms
        dispatch({ type: 'SET_CONFIG', payload: local });
        dispatch({ type: 'GO_PAGE', payload: 2 });
      } catch (err) {
        alert('Failed to initialise search scheme on the server.\n\n' + err.message);
      } finally {
        setLoading(false); // Stop loading regardless of success/error
      }
    }

    const params = [
      {
        label: 'L — LSH Functions',
        desc: 'Number of hash functions',
        name: 'L', min: 1, max: 256, step: 1,
        value: local.L,
      },
      {
        label: 'M — Security Parameter',
        desc: 'Bit-array length',
        name: 'M', min: 1, max: 4096, step: 1,
        value: local.M,
      },
    ];

    return h('div', { className: 'page' },
      h('div', { className: 'page-title' }, '// CONFIGURATION'),
      h('p', { className: 'page-subtitle' },
        'Set the mathematical parameters that govern the search scheme. These control the trade-off between recall, precision, and computational cost.'
      ),

      h('div', { className: 'section-label' }, 'PARAMETERS'),

      h('div', { className: 'card' },
        h('div', { className: 'card-title' }, 'PARAMETER MATRIX'),
        params.map(p =>
          h(ParamRow, {
            key: p.name,
            ...p,
            onChange: val => setParam(p.name, val)
          })
        )
      ),

      // Live parameter summary
      h('div', { className: 'card' },
        h('div', { className: 'card-title' }, 'CURRENT VALUES'),
        h('div', { style: { display: 'flex', gap: 32, flexWrap: 'wrap' } },
          h('div', null,
            h('div', { style: { color: 'var(--text-muted)', fontSize: 11, fontFamily: 'var(--font-mono)', letterSpacing: '0.1em' } }, 'L'),
            h('div', { style: { fontSize: 32, fontFamily: 'var(--font-mono)', color: 'var(--accent)' } }, local.L)
          ),
          h('div', null,
            h('div', { style: { color: 'var(--text-muted)', fontSize: 11, fontFamily: 'var(--font-mono)', letterSpacing: '0.1em' } }, 'M'),
            h('div', { style: { fontSize: 32, fontFamily: 'var(--font-mono)', color: 'var(--accent)' } }, local.M)
          ),
          h('div', { style: { flex: 1, alignSelf: 'center', color: 'var(--text-muted)', fontSize: 12, fontFamily: 'var(--font-mono)', lineHeight: 1.8 } },
            `Expected buckets: ~${(local.L * local.M).toLocaleString()}`,
            h('br'),
            `Collision prob (est): ${(Math.exp(-local.L * 0.5)).toFixed(4)}`
          )
        )
      ),

      h('div', { className: 'action-row' },
        h('button', {
          className: 'btn btn-primary',
          onClick: handleNext,
          disabled: loading // Disable button while loading
        },
          // Swap the text based on the loading state
          loading ? 'CONFIGURING... ›' : 'CONFIGURE ›'
        )
      )
    );
  }

  // ================================================================
  // MODULE 4 — PAGE 2: DATA INGESTION & MANAGEMENT
  // Handles CSV drop/parse, virtualized data grid, row management,
  // and simulated Build Index progress bar.
  // ================================================================

  function Page2Data() {
    const { state, dispatch } = useApp();
    const { dataset, config } = state;

    const [dragging, setDragging] = useState(false);
    const [indexing, setIndexing] = useState(false);
    const [progress, setProgress] = useState(state.indexed ? 100 : 0);
    const [done, setDone] = useState(state.indexed);
    const fileRef = useRef(null);
    const intervalRef = useRef(null);

    // ── CSV PARSING ──────────────────────────────────────────────
    function parseCSV(file) {
      Papa.parse(file, {
        header: false,
        skipEmptyLines: true,
        complete: ({ data }) => {
          // Expected columns: [docId, docName, keywords(semicolon-separated)]
          const rows = data.map((cols, i) => ({
            id: i,
            docId: (cols[0] || '').trim(),
            docName: (cols[1] || '').trim(),
            keywords: (cols[2] || '').split(';').map(k => k.trim()).filter(Boolean),
            checked: false,
          }));
          dispatch({ type: 'SET_DATASET', payload: rows });
          dispatch({ type: 'SET_INDEXED', payload: false });
          setDone(false);
          setProgress(0);
          setIndexing(false);
        },
        error: (err) => alert('CSV parse error: ' + err.message)
      });
    }

    function onFileInput(e) {
      const f = e.target.files[0];
      if (f) parseCSV(f);
    }

    // ── DRAG & DROP ──────────────────────────────────────────────
    function onDragOver(e) { e.preventDefault(); setDragging(true); }
    function onDragLeave() { setDragging(false); }
    function onDrop(e) {
      e.preventDefault();
      setDragging(false);
      const f = e.dataTransfer.files[0];
      if (f && f.name.endsWith('.csv')) parseCSV(f);
      else alert('Please drop a .csv file.');
    }

    // ── ROW OPERATIONS ───────────────────────────────────────────
    const checkedCount = dataset.filter(r => r.checked).length;
    const allChecked = dataset.length > 0 && dataset.every(r => r.checked);

    function addRow() { dispatch({ type: 'ADD_ROW' }); }
    function removeSelected() { dispatch({ type: 'REMOVE_CHECKED' }); }
    function toggleAll() { dispatch({ type: 'TOGGLE_ALL' }); }

    function updateCell(index, field, value) {
      if (indexing || done) return;
      let parsed = value;
      if (field === 'keywords') {
        // Accept comma or semicolon separated input; store as array
        parsed = value.split(/[;,]/).map(k => k.trim()).filter(Boolean);
      }
      dispatch({ type: 'UPDATE_ROW', payload: { index, field, value: parsed } });
    }

    // ── BUILD INDEX — sends corpus to Crow backend ────────────────
    // The simulated setInterval is replaced by a real fetch request.
    // Progress bar animates while the request is in-flight, then
    // snaps to 100 % on success (or resets on failure).
    async function buildIndex() {
      if (dataset.length === 0) return alert('No data loaded.');

      setIndexing(true);
      setProgress(0);

      const payload = dataset.map(({ docId, docName, keywords }) => ({
        docId, docName, keywords,
      }));

      try {
        // 1. Kick off the indexing request without awaiting it immediately
        const buildPromise = fetch('/api/build', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload),
        });

        // 2. Start polling the backend for real progress every 500ms
        intervalRef.current = setInterval(async () => {
          try {
            const progressRes = await fetch('/api/progress');
            if (progressRes.ok) {
              const data = await progressRes.json();
              
              // Update UI with real data from the server
              if (data.progress !== undefined) setProgress(data.progress);
              
              // Failsafe: stop polling if backend says it's 100%
              if (data.progress >= 100) clearInterval(intervalRef.current);
            }
          } catch (pollErr) {
            console.warn('Polling failed:', pollErr);
          }
        }, 100);

        // 3. Now wait for the main build request to actually finish
        const res = await buildPromise;
        
        // Clean up the polling interval once the main request resolves
        clearInterval(intervalRef.current);

        if (!res.ok) {
          const msg = await res.text().catch(() => res.statusText);
          throw new Error(`Server error ${res.status}: ${msg}`);
        }

        // 4. Success — snap to 100% and unlock the next step
        setProgress(100);
        setIndexing(false);
        setDone(true);
        dispatch({ type: 'SET_INDEXED', payload: true });

      } catch (err) {
        clearInterval(intervalRef.current);
        setIndexing(false);
        setProgress(0);
        alert('Failed to build the index on the server.\n\n' + err.message);
      }
    }

    useEffect(() => () => clearInterval(intervalRef.current), []);

    // ── RESET INDEX ──────────────────────────────────────────────
    // Clears the built status, unlocking the data grid for editing
    async function resetIndex() {

      try {
        const res = await fetch('/api/reset', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(dataset.length)
        });

        clearInterval(intervalRef.current);

        if (!res.ok) {
          const msg = await res.text().catch(() => res.statusText);
          throw new Error(`Server error ${res.status}: ${msg}`);
        }

      } catch (err) {
        clearInterval(intervalRef.current);
        alert('Failed to build the index on the server.\n\n' + err.message);
      } finally {
        setDone(false);
        setProgress(0);
        dispatch({ type: 'SET_INDEXED', payload: false });
      }

    }

    // ── RENDER ───────────────────────────────────────────────────
    return h('div', { className: 'page' },
      h('div', { className: 'page-title' }, '// INDEXING'),
      h('p', { className: 'page-subtitle' },
        'Upload your document keywords as a CSV file or add documents manually.'
      ),

      // Config summary strip
      h('div', { className: 'config-strip' },
        h('span', { className: 'config-badge' }, 'L:', h('b', null, config.L)),
        h('span', { className: 'config-badge' }, 'M:', h('b', null, config.M)),
      ),

      // CSV Drop Zone (only shown when no data loaded or not done)
      !done && h('div', { className: 'section-label' }, 'CSV UPLOAD'),
      !done && h('div', {
        className: `drop-zone ${dragging ? 'drag-over' : ''}`,
        onDragOver, onDragLeave, onDrop,
        style: { marginBottom: 24 }
      },
        h('input', {
          ref: fileRef,
          type: 'file',
          accept: '.csv',
          onChange: onFileInput
        }),
        h('span', { className: 'drop-icon' }, '📂'),
        h('div', { className: 'drop-label' }, 'DRAG & DROP DOCUMENT KEYWORDS HERE'),
        h('div', { className: 'drop-hint' }, 'or click to browse. [.csv]')
      ),

      // Data Grid (always shown)
      h('div', null,
        h('div', { className: 'section-label' }, 'DATASET PREVIEW'),

        // Toolbar
        h('div', { className: 'toolbar' },
          h('div', { className: 'toolbar-stat' },
            'Total Entries: ', h('span', null, dataset.length.toLocaleString())
          ),
          checkedCount > 0 && h('div', { className: 'toolbar-stat' },
            'Selected: ', h('span', null, checkedCount)
          ),
          h('div', { className: 'toolbar-spacer' }),
          !done && !indexing && h('button', {
            className: 'btn btn-secondary btn-sm',
            onClick: addRow
          }, '+ ADD ROW'),
          !done && !indexing && checkedCount > 0 && h('button', {
            className: 'btn btn-danger btn-sm',
            onClick: removeSelected
          }, `REMOVE ${checkedCount} SELECTED`)
        ),

        // Grid
        h('div', { className: 'grid-wrapper' },
          // Header
          h('div', { className: 'grid-header' },
            h('div', { className: 'grid-header-cell' },
              h('input', {
                type: 'checkbox',
                checked: allChecked,
                disabled: indexing || done,
                onChange: toggleAll,
                style: { accentColor: 'var(--accent)' }
              })
            ),
            h('div', { className: 'grid-header-cell' }, 'DOC ID'),
            h('div', { className: 'grid-header-cell' }, 'DOCUMENT NAME'),
            h('div', { className: 'grid-header-cell' }, 'KEYWORDS'),
          ),
          // Virtualized rows — renders only visible subset
          h(FixedSizeList, {
            height: Math.min(dataset.length * 44, 400),
            itemCount: dataset.length,
            itemSize: 44,
          }, ({ index, style }) => {
            // Inlining the logic here prevents React from unmounting the inputs
            const row = dataset[index];
            if (!row) return null;

            return h('div', {
              key: row.id, // Added key here for proper DOM reconciliation
              className: `grid-row ${row.checked ? 'selected' : ''}`,
              style
            },
              // Checkbox cell
              h('div', { className: 'grid-cell', style: { justifyContent: 'center' } },
                h('input', {
                  type: 'checkbox',
                  checked: row.checked,
                  disabled: indexing || done,
                  onChange: () => dispatch({ type: 'TOGGLE_CHECK', payload: index })
                })
              ),
              // Doc ID cell
              h('div', { className: 'grid-cell' },
                h('input', {
                  className: 'cell-input',
                  value: row.docId,
                  disabled: indexing || done,
                  onChange: e => updateCell(index, 'docId', e.target.value),
                  placeholder: 'doc-id',
                  style: { fontFamily: 'var(--font-mono)', fontSize: 12 }
                })
              ),
              // Doc Name cell
              h('div', { className: 'grid-cell' },
                h('input', {
                  className: 'cell-input',
                  value: row.docName,
                  disabled: indexing || done,
                  onChange: e => updateCell(index, 'docName', e.target.value),
                  placeholder: 'Document name'
                })
              ),
              // Keywords cell — display as chips or input
              h('div', { className: 'grid-cell', style: { overflow: 'hidden' } },
                done || indexing
                  ? h('div', { className: 'chips-wrap' },
                    row.keywords.slice(0, 6).map((kw, ki) =>
                      h(Chip, { key: ki, label: kw, highlight: false })
                    ),
                    row.keywords.length > 6 && h('span', {
                      className: 'chip',
                      style: { color: 'var(--text-muted)' }
                    }, `+${row.keywords.length - 6}`)
                  )
                  : h('input', {
                    className: 'cell-input',
                    value: row.keywords.join('; '),
                    onChange: e => updateCell(index, 'keywords', e.target.value),
                    placeholder: 'keyword1; keyword2; ...'
                  })
              )
            );
          })
        ),

        // ── Progress Bar ─────────────────────────────────────────
        (indexing || done) && h('div', { className: 'progress-wrap' },
          h('div', { className: 'progress-header' },
            h('span', { className: 'progress-label' },
              done ? '✓ INDEX BUILT' : 'BUILDING INDEX...'
            ),
            h('span', { className: 'progress-pct' }, Math.round(progress) + '%')
          ),
          h('div', { className: 'progress-track' },
            h('div', { className: 'progress-fill', style: { width: progress + '%' } })
          ),
        ),

        // Action row
        h('div', { className: 'action-row' },
          !done && !indexing && h('button', {
            className: 'btn btn-primary',
            onClick: buildIndex,
            disabled: dataset.length === 0
          }, 'BUILD INDEX'),
          indexing && h('button', {
            className: 'btn btn-primary',
            disabled: true
          }, 'INDEXING...'),
          done && h('button', {
            className: 'btn btn-danger',
            style: { marginRight: 'auto' },
            onClick: resetIndex
          }, '↺ RESET'),
          done && h('button', {
            className: 'btn btn-success',
            onClick: () => dispatch({ type: 'GO_PAGE', payload: 3 })
          }, 'SEARCH ›')
        )
      )
    );
  }

  // ================================================================
  // MODULE 5 — PAGE 3: SEARCH INTERFACE
  // Query input, real LSH lookup via /api/search, result cards with
  // keyword highlighting, and "Start Over" to reset global state.
  // ================================================================

  const K = 10; // Top-k results constant

  function Page3Search() {
    const { state, dispatch } = useApp();
    const { dataset, config } = state;

    const [query, setQuery] = useState('');
    const [results, setResults] = useState(null);
    const [searched, setSearched] = useState(false);
    const [searching, setSearching] = useState(false);

    // ── Tokenise query string (kept for keyword highlighting) ─────
    function tokenize(str) {
      return str.toLowerCase().split(/[\s,;]+/).filter(Boolean);
    }

    // ── Build a lookup map from docId → dataset row ───────────────
    // This lets us O(1)-hydrate the server's { docId, score } results
    // with the local docName and keywords for rendering.
    function buildDocMap() {
      const map = new Map();
      dataset.forEach(row => map.set(String(row.docId), row));
      return map;
    }

    // ── Execute real LSH search via Crow backend ──────────────────
    async function runSearch() {
      if (!query.trim()) return;
      const queryTokens = tokenize(query);

      setSearching(true);

      try {
        const res = await fetch('/api/search', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          // Send both the raw string and the pre-tokenised array so the
          // server can use whichever it prefers.
          body: JSON.stringify({ query: query.trim(), tokens: queryTokens }),
        });

        if (!res.ok) {
          const msg = await res.text().catch(() => res.statusText);
          throw new Error(`Server error ${res.status}: ${msg}`);
        }

        // Expected response: [ { docId: string, score: number }, … ]
        const serverResults = await res.json();

        // Hydrate server results with local metadata (docName, keywords)
        const docMap = buildDocMap();
        const hydrated = serverResults
          .slice(0, K)                          // respect top-k cap
          .map(({ docId, score }) => {
            const localRow = docMap.get(String(docId)) || {};
            return {
              docId,
              docName: localRow.docName || '(untitled)',
              keywords: localRow.keywords || [],
              _score: typeof score === 'number' ? score : 0,
            };
          });

        setResults(hydrated);
        setSearched(true);

      } catch (err) {
        alert('Search request failed.\n\n' + err.message);
      } finally {
        setSearching(false);
      }
    }

    function handleKey(e) {
      if (e.key === 'Enter') runSearch();
    }

    const queryTokens = tokenize(query);

    // ── RENDER ───────────────────────────────────────────────
    return h('div', { className: 'page' },
      h('div', { className: 'page-title' }, '// SEARCHING'),
      h('p', { className: 'page-subtitle' },
        `Query the indexed documents using Asymmetric Multi-Keyword Fuzzy Search.`
      ),

      // Config + dataset summary
      h('div', { className: 'config-strip' },
        h('span', { className: 'config-badge' }, 'L:', h('b', null, config.L)),
        h('span', { className: 'config-badge' }, 'M:', h('b', null, config.M)),
        h('span', { className: 'config-badge' }, 'Index Size: ', h('b', null, dataset.length.toLocaleString(), ' document(s)')),
      ),

      // Search bar
      h('div', { className: 'search-bar-wrap' },
        h('input', {
          className: 'search-input',
          type: 'text',
          placeholder: 'Enter query keywords (e.g. neural network classification)',
          value: query,
          onChange: e => setQuery(e.target.value),
          onKeyDown: handleKey,
          autoFocus: true,
        }),
        h('button', {
          className: 'btn btn-primary search-btn',
          onClick: runSearch,
          disabled: searching,
        }, searching ? 'SEARCHING...' : '⌕ SEARCH')
      ),

      // Results
      searched && h('div', null,
        // Meta
        results && results.length > 0
          ? h('div', { className: 'results-meta' },
            'SHOWING',
            h('span', { className: 'count' }, ` TOP ${results.length} `),
            'MATCHES',
            h('div', { className: 'divider' }),
            `QUERY: "${query}"`
          )
          : h('div', { className: 'no-results' },
            '[ NO MATCHES FOUND ]',
            h('br'),
            h('span', { style: { fontSize: 11 } }, 'Try broader or different keywords.')
          ),

        // Result cards
        results && results.map((doc, rank) =>
          h('div', { key: doc.docId != null ? String(doc.docId) : rank, className: 'result-card' },
            h('div', { className: 'result-rank' },
              `RANK #${rank + 1}`,
              h('span', {
                style: {
                  marginLeft: 12,
                  color: 'var(--accent)',
                  fontFamily: 'var(--font-mono)',
                  fontSize: 10
                }
              },
                `SCORE: ${(doc._score * 100).toFixed(1)}%`
              )
            ),
            h('div', { className: 'result-name' }, doc.docName || '(untitled)'),
            h('div', { className: 'result-id' }, 'ID: ' + (doc.docId || '—')),
            h('div', { className: 'chips-wrap' },
              doc.keywords.map((kw, ki) => {
                // Highlight if any query token matches this keyword
                const isMatch = queryTokens.some(t =>
                  kw.toLowerCase().includes(t)
                );
                return h(Chip, { key: ki, label: kw, highlight: isMatch });
              })
            )
          )
        )
      ),

      // Actions
      h('div', { className: 'action-row', style: { marginTop: 40 } },
        h('button', {
          className: 'btn btn-secondary',
          onClick: () => dispatch({ type: 'GO_PAGE', payload: 2 })
        }, '‹ INDEX')
      )
    );
  }

  // ================================================================
  // MODULE 6 — APP ROOT
  // Routes between pages based on global state.page value.
  // ================================================================
  function App() {
    const { state } = useApp();
    return h('div', { style: { display: 'flex', flexDirection: 'column', minHeight: '100vh' } },
      h(AppHeader),
      state.page === 1 && h(Page1Config),
      state.page === 2 && h(Page2Data),
      state.page === 3 && h(Page3Search),
    );
  }

  // ── Mount ─────────────────────────────────────────────────────
  const rootEl = document.getElementById('root');
  const root = ReactDOM.createRoot(rootEl);
  root.render(h(AppProvider, null, h(App)));

})();