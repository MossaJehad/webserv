# Commit Message Convention

This repository enforces commit messages through a Git `commit-msg` hook.

## Required format

```text
<type>(#TASK-ID): <description>
```

Example:

```text
feat(#DVA-55): added new payment method to the payment portal
```

## Rules

- Allowed types: `feat`, `fix`, `chore`, `docs`, `style`, `refactor`, `perf`, `test`, `build`, `ci`, `revert`
- `TASK-ID` must look like `DVA-55` (uppercase key + dash + number)
- Description length must be between 10 and 250 characters

## Setup (team install)

Run the installer once after cloning:

```bash
bash scripts/install-hooks.sh
```

What it does:

- Marks `.githooks/commit-msg` as executable
- Configures `core.hooksPath` to `.githooks` for this local repository

You can verify the setup with:

```bash
git config --get core.hooksPath
```

Expected output:

```text
.githooks
```

## Manual setup alternative

```bash
chmod +x .githooks/commit-msg
git config core.hooksPath .githooks
```

## Valid commit message examples

```text
feat(#DVA-55): added new payment method to the payment portal
fix(#DVA-99): prevent duplicate checkout requests in API gateway
docs(#DVA-12): clarify deployment steps for staging environment
```

## Invalid commit message examples

```text
feat(DVA-55): missing # before task id
feature(#DVA-55): invalid type (feature is not in allowed list)
fix(#dva-55): task id key must be uppercase
chore(#DVA-10): short
refactor(#DVA-77): this description is intentionally made very long to exceed the maximum length limit by adding many extra words that do not contribute any useful context and therefore should be rejected by the hook because it is beyond two hundred and fifty characters in total length.
```
