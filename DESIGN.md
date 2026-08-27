# DESIGN.md — webserv

## 1. Aesthetic Direction & Persona
- **Persona**: **Modern Systems Console / High-Precision Developer Dashboard**.
- **Atmosphere**: Technical, crisp, engineered, and reliable. Combines dark slate surfaces with high-contrast method badges, glowing telemetry indicators, and clean monospace metadata.
- **Anti-Patterns & Slop Avoidance**:
  - ❌ No generic SaaS purple gradients (`#8b5cf6 -> #ec4899`).
  - ❌ No sluggish heavy animations or unoptimized DOM transitions.
  - ❌ No external CDN imports (Google Fonts, FontAwesome, Tailwind CDN) — must render identically 100% offline.
  - ❌ No ambiguous low-contrast text on dark backgrounds.

---

## 2. Design Tokens & Color System

### Surface & Background Tokens
- `color-bg-canvas`: `#090d16` (Deep space canvas)
- `color-bg-card`: `#111827` (Card & container surface)
- `color-bg-elevated`: `#1f293d` (Hover states & elevated sub-cards)
- `color-border-subtle`: `#1e293b` (Quiet section borders)
- `color-border-strong`: `#334155` (Input borders, active tab outlines)

### Typography & Content Tokens
- `color-text-primary`: `#f8fafc` (Headings, primary values, active tab text)
- `color-text-secondary`: `#94a3b8` (Subtitles, labels, descriptions)
- `color-text-muted`: `#64748b` (Placeholders, metadata timestamps)

### Accent & Semantic State Tokens
- **Brand / Accent**: `#38bdf8` (Cyan primary highlight, active badges)
- **HTTP GET / Success**: `#10b981` (Emerald green badge, 200 OK status)
- **HTTP POST / Creation**: `#3b82f6` (Cobalt blue badge, 201 Created status)
- **HTTP DELETE / Danger**: `#ef4444` (Crimson red badge, removal actions, 4xx/5xx errors)
- **HTTP Redirection / Warning**: `#f59e0b` (Amber badge, 301/302 redirects, warnings)

---

## 3. Typography Hierarchy
- **Primary UI Font**: `-apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif`
- **Code & Endpoint Monospace**: `ui-monospace, "SF Mono", "Cascadia Code", "Fira Code", Menlo, monospace`

| Element | Size | Weight | Line Height | Tracking |
| :--- | :--- | :--- | :--- | :--- |
| **Header Title** | `32px / 2rem` | `700 (Bold)` | `1.2` | `-0.02em` |
| **Section Headings (H2)** | `20px / 1.25rem` | `600 (Semibold)` | `1.3` | `-0.01em` |
| **Card Headings (H3)** | `15px / 0.95rem` | `600 (Semibold)` | `1.4` | `0` |
| **Body & Labels** | `14px / 0.875rem` | `400 / 500` | `1.5` | `0` |
| **Badges & Monospace** | `12px / 0.75rem` | `600 (Semibold)` | `1.0` | `0.05em (Uppercase)` |

---

## 4. Component Standards

### Tabs & Navigation
- Segmented pill container with dark slate background (`#0f172a`).
- Active state highlighted with `#0284c7` fill or crisp cyan bottom indicator and `#ffffff` text.
- Keyboard accessible and fast switching without DOM re-renders.

### Method Badges
- Standardized HTTP badges with distinct semantic colors:
  - `.badge-get` → Background: `rgba(16, 185, 129, 0.15)`, Text: `#34d399`
  - `.badge-post` → Background: `rgba(59, 130, 246, 0.15)`, Text: `#60a5fa`
  - `.badge-delete` → Background: `rgba(239, 68, 68, 0.15)`, Text: `#f87171`

### Input Fields & Upload Dropzones
- Background: `#0f172a` with `1px solid #334155` border.
- Focus: `outline: none; border-color: #38bdf8; box-shadow: 0 0 0 2px rgba(56, 189, 248, 0.2);`.
- Custom file selector button matching primary button styling.

### Live Response / Output Consoles
- Dark background (`#090d16`) with inner monospace padding, syntax-friendly coloring, and scrollable horizontal overflow.

---

## 5. Responsive Behavior
- Max container width: `960px` centered with fluid mobile padding (`16px`).
- Grid breakdowns: Auto-fitting responsive cards (`minmax(280px, 1fr)`).
- Tab bar smoothly wraps on smaller viewports.
