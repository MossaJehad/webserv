#!/usr/bin/env bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel 2>/dev/null || true)"

if [[ -z "$repo_root" ]]; then
  echo "Error: run this script from inside a Git repository." >&2
  exit 1
fi

cd "$repo_root"

if [[ ! -f ".githooks/commit-msg" ]]; then
  echo "Error: .githooks/commit-msg was not found." >&2
  exit 1
fi

chmod +x ".githooks/commit-msg"
git config core.hooksPath ".githooks"

echo "Git hooks are installed for this repository."
echo "Configured core.hooksPath to: $(git config --get core.hooksPath)"
echo "Try: git commit -m 'feat(#DVA-55): added new payment method to the payment portal'"
